//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2026-2026.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////

// Tests the sorting algorithms of <boost/move/algo/detail/insertion_sort.hpp>,
// <merge_sort.hpp>, <heap_sort.hpp> and <pdqsort.hpp> one by one. The ones
// <adaptive_sort_merge.hpp> defines, and adaptive_sort itself, are not here.
//
// Short cases walk every permutation of the keys, crossed with every
// key_pattern, so every order and every arrangement of ties is tried. The
// thresholds of these algorithms are at 16 or 32 elements, which no exhaustive
// case reaches, so longer shuffled cases follow to get into the recursion.

#include "internal_test_util.hpp"
#include "random_shuffle.hpp"

#include <boost/config.hpp>

#include <boost/move/algo/detail/insertion_sort.hpp>
#include <boost/move/algo/detail/merge_sort.hpp>
#include <boost/move/algo/detail/heap_sort.hpp>
#include <boost/move/algo/detail/pdqsort.hpp>
#include <boost/move/core.hpp>
#include <boost/move/unique_ptr.hpp>
#include <boost/move/make_unique.hpp>
#include <boost/move/detail/force_ptr.hpp>

#include <boost/core/lightweight_test.hpp>

#include <algorithm>   //std::stable_sort, std::next_permutation, std::adjacent_find
#include <cstdlib>     //std::srand
#include <cstddef>

using boost::movelib::move_op;
using boost::movelib::swap_op;

//The largest length whose permutations are all walked. 8! is 40320, times
//max_key_pattern and the algorithms under test, which is still a few seconds.
static const std::size_t MaxPermLen = 8u;

//Lengths that reach past the thresholds of the algorithms, where they stop
//insertion sorting and start halving. Every one is tried with several shuffles.
static const std::size_t LongLens[] = { 17u, 33u, 64u, 101u, 200u };
static const std::size_t NumLongLens = sizeof(LongLens)/sizeof(*LongLens);
static const std::size_t ShufflesPerLen = 4u;

///////////////////////////////////////////////////////////////////////////////
//
//                              Test cases
//
///////////////////////////////////////////////////////////////////////////////

//One sequence to sort, with the result it must produce
class sort_case
{
   sort_case(const sort_case &);
   sort_case &operator=(const sort_case &);

   public:
   explicit sort_case(std::size_t n)
      : m_in(boost::movelib::make_unique<kv[]>(n))
      , m_exp(boost::movelib::make_unique<kv[]>(n))
      , m_size(n)
   {}

   //Takes the keys as they are now and gives every element the value of its
   //place, so that a stable algorithm has to give them back in that same order
   void set_keys_and_close(const std::size_t *keys)
   {
      for(std::size_t i = 0u; i != m_size; ++i){
         m_in[i].key = keys[i];
         m_in[i].val = i;
      }
      std::copy(m_in.get(), m_in.get()+m_size, m_exp.get());
      std::stable_sort(m_exp.get(), m_exp.get()+m_size, less_type());
   }

   const kv *in()  const {  return m_in.get();   }
   const kv *exp() const {  return m_exp.get();  }
   std::size_t size() const {  return m_size;  }

   private:
   boost::movelib::unique_ptr<kv[]> m_in;
   boost::movelib::unique_ptr<kv[]> m_exp;
   std::size_t m_size;
};

//True if the range holds the same elements as the input, in key order. This is
//all that can be asked of an algorithm that is not stable.
inline bool is_sorted_permutation_of(const test_type *got, const kv *in, std::size_t n)
{
   if(!n){
      return true;      //Nothing was given to the algorithm, nothing to check
   }
   if(std::adjacent_find(got, got+n, out_of_key_order()) != (got+n)){
      return false;
   }

   //Same elements as the input: sorting both by key and value makes the two
   //sequences equal whatever order the algorithm left the equivalent ones in
   boost::movelib::unique_ptr<kv[]> a(boost::movelib::make_unique<kv[]>(n));
   boost::movelib::unique_ptr<kv[]> b(boost::movelib::make_unique<kv[]>(n));
   std::transform(got, got+n, a.get(), to_kv());
   std::copy(in, in+n, b.get());
   std::sort(a.get(), a.get()+n, kv_less_full());
   std::sort(b.get(), b.get()+n, kv_less_full());
   return std::equal(a.get(), a.get()+n, b.get());
}

///////////////////////////////////////////////////////////////////////////////
//
//                      The algorithms, one by one
//
///////////////////////////////////////////////////////////////////////////////

//Names the algorithm each case is run through, so that a failure says which
enum sort_algo
{
   algo_insertion_sort,
   algo_insertion_sort_copy,
   algo_insertion_sort_op_swap,
   algo_insertion_sort_uninit_copy,
   algo_merge_sort,
   algo_merge_sort_copy,
   algo_merge_sort_uninit_copy,
   algo_merge_sort_constructed_buf,
   algo_stable_sort_bufferless_rec,
   algo_stable_sort_bufferless,
   algo_stable_sort_adaptive,
   algo_heap_sort,
   algo_pdqsort,
   max_sort_algo
};

inline const char *name_of(sort_algo a)
{
   switch(a){
      case algo_insertion_sort:            return "insertion_sort";
      case algo_insertion_sort_copy:       return "insertion_sort_copy";
      case algo_insertion_sort_op_swap:    return "insertion_sort_op(swap_op)";
      case algo_insertion_sort_uninit_copy:return "insertion_sort_uninitialized_copy";
      case algo_merge_sort:                return "merge_sort";
      case algo_merge_sort_copy:           return "merge_sort_copy";
      case algo_merge_sort_uninit_copy:    return "merge_sort_uninitialized_copy";
      case algo_merge_sort_constructed_buf:return "merge_sort_with_constructed_buffer";
      case algo_stable_sort_bufferless_rec:return "stable_sort_bufferless_ONlogN2_recursive";
      case algo_stable_sort_bufferless:    return "stable_sort_bufferless_ONlogN2";
      case algo_stable_sort_adaptive:      return "stable_sort_adaptive_ONlogN2";
      case algo_heap_sort:                 return "heap_sort";
      case algo_pdqsort:                   return "pdqsort";
      case max_sort_algo:                  break;   //Not an algorithm, only how many
   }
   return "?";
}

//The ones that promise nothing about the order of equivalent elements
inline bool is_stable_algo(sort_algo a)
{
   return a != algo_heap_sort && a != algo_pdqsort;
}

void run_one(sort_algo algo, const sort_case &c)
{
   using namespace boost::movelib;

   std::size_t const n    = c.size();
   std::size_t const half = n/2u + (n&1u);   //What the buffered ones ask for
   less_type comp;

   //The sequence to sort, and a second array for the ones that sort into a
   //different place than they read from
   guarded_array arr(n);
   guarded_array out(n);
   test_type *const p = arr.data();
   test_type *const q = out.data();
   fill_from(p, c.in(), n);
   fill_buffer(q, n);

   //Raw storage for the ones that take uninitialized memory
   boost::movelib::unique_ptr<char[]> raw(new char[sizeof(test_type)*n]);
   test_type *const rawp = boost::move_detail::force_ptr<test_type*>(raw.get());

   test_type *result = p;     //Where the sorted sequence is left
   bool destroy_raw  = false;

   switch(algo){
      case algo_insertion_sort:
         insertion_sort(p, p+n, comp);
      break;
      case algo_insertion_sort_copy:
         insertion_sort_copy(p, p+n, q, comp);
         result = q;
      break;
      case algo_insertion_sort_op_swap:
         insertion_sort_op(p, p+n, q, comp, swap_op());
         result = q;
      break;
      case algo_insertion_sort_uninit_copy:
         insertion_sort_uninitialized_copy(p, p+n, rawp, comp);
         result = rawp;
         destroy_raw = true;
      break;
      case algo_merge_sort:
         merge_sort(p, p+n, comp, rawp);
      break;
      case algo_merge_sort_copy:
         merge_sort_copy(p, p+n, q, comp);
         result = q;
      break;
      case algo_merge_sort_uninit_copy:
         merge_sort_uninitialized_copy(p, p+n, rawp, comp);
         result = rawp;
         destroy_raw = true;
      break;
      case algo_merge_sort_constructed_buf:
         //The buffer must hold ceil(n/2) constructed elements
         merge_sort_with_constructed_buffer(p, p+n, comp, q);
      break;
      case algo_stable_sort_bufferless_rec:
         stable_sort_bufferless_ONlogN2_recursive(p, p+n, comp);
      break;
      case algo_stable_sort_bufferless:
         stable_sort_bufferless_ONlogN2(p, p+n, comp);
      break;
      case algo_stable_sort_adaptive:
         stable_sort_adaptive_ONlogN2(p, p+n, comp, rawp, half);
      break;
      case algo_heap_sort:
         heap_sort(p, p+n, key_less());
      break;
      case algo_pdqsort:
         pdqsort(p, p+n, key_less());
      break;
      case max_sort_algo:
      break;
   }

   bool ok;
   if(is_stable_algo(algo)){
      ok = std::equal(result, result+n, c.exp(), same_element());
   }
   else{
      ok = is_sorted_permutation_of(result, c.in(), n);
   }

   if(!ok){
      BOOST_ERROR(name_of(algo));
   }
   BOOST_TEST(arr.guards_intact());
   BOOST_TEST(out.guards_intact());

   //Sorting into a second array with a swap leaves that array's old contents
   //where the input was, and loses none of them
   if(algo == algo_insertion_sort_op_swap){
      BOOST_TEST(is_buffer_permutation(p, n));
   }

   if(destroy_raw){
      for(std::size_t i = 0u; i != n; ++i){
         rawp[i].~test_type();
      }
   }
}

//stable_sort_adaptive_ONlogN2 takes a buffer of any size and halves the range
//until what is left fits in it, so the capacity is swept from empty to longer
//than the range.
void test_buffer_capacities(const sort_case &c)
{
   using namespace boost::movelib;

   std::size_t const n        = c.size();
   std::size_t const half_len = n/2u + (n&1u);
   less_type comp;

   std::size_t const caps[] = { 0u, 1u, half_len/2u, half_len ? half_len-1u : 0u
                              , half_len, half_len+1u, n, n+1u };

   for(std::size_t ci = 0u; ci != sizeof(caps)/sizeof(*caps); ++ci){
      std::size_t const cap = caps[ci];

      {
         guarded_array arr(n);
         test_type *const p = arr.data();
         fill_from(p, c.in(), n);
         boost::movelib::unique_ptr<char[]> raw(new char[sizeof(test_type)*(cap+1u)]);
         stable_sort_adaptive_ONlogN2
            (p, p+n, comp, boost::move_detail::force_ptr<test_type*>(raw.get()), cap);
         if(!std::equal(p, p+n, c.exp(), same_element())){
            BOOST_ERROR("stable_sort_adaptive_ONlogN2");
         }
         BOOST_TEST(arr.guards_intact());
      }
   }
}

//insertion_sort_step sorts consecutive runs and returns their length, which is
//all it promises: the range as a whole is not sorted unless one run covers it
void test_insertion_sort_step_runs(const sort_case &c)
{
   using namespace boost::movelib;
   std::size_t const n = c.size();
   if(!n){
      return;
   }
   less_type comp;

   for(std::size_t step = 1u; step <= 2u*MergeSortInsertionSortThreshold; step *= 2u){
      guarded_array arr(n);
      test_type *const p = arr.data();
      fill_from(p, c.in(), n);

      std::size_t const s = insertion_sort_step(p, n, step, comp);
      BOOST_TEST(s == (step < std::size_t(MergeSortInsertionSortThreshold)
                         ? step : std::size_t(MergeSortInsertionSortThreshold)));

      //Every run of "s" elements must be left stably sorted
      for(std::size_t base = 0u; base < n; base += s){
         std::size_t const run = (n - base) < s ? (n - base) : s;
         BOOST_TEST(std::adjacent_find(p+base, p+base+run, out_of_stable_order())
                       == (p+base+run));
      }
      BOOST_TEST(arr.guards_intact());
   }
}

///////////////////////////////////////////////////////////////////////////////
//
//                              Drivers
//
///////////////////////////////////////////////////////////////////////////////

void test_all_permutations()
{
   std::size_t keys[MaxPermLen];

   for(std::size_t n = 0u; n <= MaxPermLen; ++n){
      for(unsigned pat = 0u; pat != max_key_pattern; ++pat){
         for(std::size_t i = 0u; i != n; ++i){
            keys[i] = key_of(i, key_pattern(pat));
         }
         //The keys are already ascending, which is where next_permutation has
         //to start to walk them all
         do{
            sort_case c(n);
            c.set_keys_and_close(keys);
            for(unsigned a = 0u; a != max_sort_algo; ++a){
               run_one(sort_algo(a), c);
            }
            test_buffer_capacities(c);
            test_insertion_sort_step_runs(c);
         }while(n && std::next_permutation(keys, keys+n));
      }
   }
}

void test_long_shuffled()
{
   std::srand(0);

   for(std::size_t li = 0u; li != NumLongLens; ++li){
      std::size_t const n = LongLens[li];
      boost::movelib::unique_ptr<std::size_t[]> keys
         (boost::movelib::make_unique<std::size_t[]>(n));

      for(unsigned pat = 0u; pat != max_key_pattern; ++pat){
         for(std::size_t sh = 0u; sh != ShufflesPerLen; ++sh){
            for(std::size_t i = 0u; i != n; ++i){
               keys[i] = key_of(i, key_pattern(pat));
            }
            ::random_shuffle(keys.get(), keys.get()+n);

            sort_case c(n);
            c.set_keys_and_close(keys.get());
            for(unsigned a = 0u; a != max_sort_algo; ++a){
               run_one(sort_algo(a), c);
            }
            test_buffer_capacities(c);
            test_insertion_sort_step_runs(c);
         }
      }
   }
}

//A sequence that is already sorted, and one that is exactly reversed, are the
//shapes an adaptive algorithm treats apart, and the ones a shuffle almost never
//produces
void test_sorted_and_reversed()
{
   for(std::size_t li = 0u; li != NumLongLens; ++li){
      std::size_t const n = LongLens[li];
      boost::movelib::unique_ptr<std::size_t[]> keys
         (boost::movelib::make_unique<std::size_t[]>(n));

      for(unsigned pat = 0u; pat != max_key_pattern; ++pat){
         for(unsigned reversed = 0u; reversed != 2u; ++reversed){
            for(std::size_t i = 0u; i != n; ++i){
               keys[i] = key_of(reversed ? (n-1u-i) : i, key_pattern(pat));
            }
            sort_case c(n);
            c.set_keys_and_close(keys.get());
            for(unsigned a = 0u; a != max_sort_algo; ++a){
               run_one(sort_algo(a), c);
            }
            test_buffer_capacities(c);
            test_insertion_sort_step_runs(c);
         }
      }
   }
}

int main()
{
   test_all_permutations();
   test_long_shuffled();
   test_sorted_and_reversed();

   return ::boost::report_errors();
}
