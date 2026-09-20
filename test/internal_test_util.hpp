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

// Utilities shared by merge_internal_test and sort_internal_test, which check
// the building blocks of the merge and sort algorithms one by one.
//
// Both describe their data as (key, value) pairs: the key is what the
// algorithms compare, and the value records where the element came from, so
// that stability can be checked and not only the order.

#ifndef BOOST_MOVE_TEST_INTERNAL_TEST_UTIL_HPP
#define BOOST_MOVE_TEST_INTERNAL_TEST_UTIL_HPP

#include "order_type.hpp"

#include <boost/config.hpp>
#include <boost/move/unique_ptr.hpp>
#include <boost/move/make_unique.hpp>

#include <algorithm>   //std::sort, std::find_if, std::transform, std::adjacent_find
#include <cstddef>

typedef order_move_type          test_type;
typedef order_type_less          less_type;

//The largest length whose cases are all walked. What "all" means is up to each
//test: every interleaving of two ranges, or every permutation of one.
static const std::size_t MaxLen = 9u;

//Keys the buffer is filled with. They are above every key of the data, so an
//element that leaks from the buffer into the merged range is always detected as
//an order error too, not only as a wrong value.
static const std::size_t BufKeyBase = 1000000u;

//Marks the elements that must not be touched, before and after the array
static const std::size_t GuardKey = 2000000u;

//Values of range 2 start here, so that the range an element comes from can be
//told from its value alone
static const std::size_t Range2ValBase = 1000u;

///////////////////////////////////////////////////////////////////////////////
//
//                          Describing the data
//
///////////////////////////////////////////////////////////////////////////////

struct kv
{
   std::size_t key;
   std::size_t val;
};

//Only the key takes part in the comparison, as in the elements themselves, so
//that order_type_less orders a "kv" the same way it orders a test_type
inline bool operator<(const kv &l, const kv &r)
{  return l.key < r.key;  }

//Both halves take part in the equality, which is what tells two elements of
//equivalent key apart
inline bool operator==(const kv &l, const kv &r)
{  return l.key == r.key && l.val == r.val;  }

//Compares by key alone, like order_type_less, but order_type.hpp does not give
//this one the boost::movelib::is_sorted overload it gives order_type_less. That
//overload also requires stability, which an algorithm that does not promise it
//fails in its own assertions, so those algorithms are tested through this
//comparison and the ones that do promise it through order_type_less, whose
//assertions then check the stability as well.
struct key_less
{
   template<class T, class U>
   bool operator()(const T &a, const U &b) const
   {  return a.key < b.key;  }
};

//Orders by key and by value, which is a total order over the data of these
//tests. Used to compare two sequences as multisets, for the algorithms that do
//not promise an order among equivalent elements.
struct kv_less_full
{
   bool operator()(const kv &l, const kv &r) const
   {  return l.key < r.key || (l.key == r.key && l.val < r.val);  }
};

//How the keys of a case are chosen. Equivalent keys are what makes the
//difference between an algorithm that is stable and one that is only sorted,
//so every case is run with each of these.
enum key_pattern
{
   keys_all_distinct,      //Every key different, no tie to resolve
   keys_in_pairs,          //Two equivalent keys in a row
   keys_in_runs_of_three,  //Three equivalent keys in a row
   keys_all_equivalent,    //One single key, every comparison a tie
   max_key_pattern
};

inline std::size_t key_of(std::size_t pos, key_pattern pattern)
{
   //No "default", so that a pattern added to the enum and not to this switch is
   //a warning and not a silent fall back to one of the others
   switch(pattern){
      case keys_all_distinct:     return pos;
      case keys_in_pairs:         return pos/2u;
      case keys_in_runs_of_three: return pos/3u;
      case keys_all_equivalent:   break;
      case max_key_pattern:       break;   //Not a pattern, only how many there are
   }
   return 0u;
}

///////////////////////////////////////////////////////////////////////////////
//
//                              Checking
//
///////////////////////////////////////////////////////////////////////////////

//An array with a guard element before and after it, so that a write outside the
//range the algorithm is given does not go unnoticed. A length of zero is fine:
//the guards are there whatever the length is.
class guarded_array
{
   guarded_array(const guarded_array &);
   guarded_array &operator=(const guarded_array &);

   public:
   explicit guarded_array(std::size_t n)
      : m_mem(boost::movelib::make_unique<test_type[]>(n+2u)), m_size(n)
   {
      m_mem[0].key       = GuardKey;
      m_mem[0].val       = GuardKey;
      m_mem[n+1u].key    = GuardKey;
      m_mem[n+1u].val    = GuardKey;
   }

   test_type *data() const  {  return m_mem.get()+1;  }
   std::size_t size() const {  return m_size;  }

   bool guards_intact() const
   {
      return m_mem[0].key == GuardKey && m_mem[0].val == GuardKey
          && m_mem[m_size+1u].key == GuardKey && m_mem[m_size+1u].val == GuardKey;
   }

   private:
   boost::movelib::unique_ptr<test_type[]> m_mem;
   std::size_t m_size;
};

template<class T>
void fill_from(T *dest, const kv *src, std::size_t n)
{
   for(std::size_t i = 0u; i != n; ++i){
      dest[i].key = src[i].key;
      dest[i].val = src[i].val;
   }
}

inline void fill_buffer(test_type *dest, std::size_t n)
{
   for(std::size_t i = 0u; i != n; ++i){
      dest[i].key = BufKeyBase + i;
      dest[i].val = BufKeyBase + i;
   }
}

//An element and the (key, value) pair that describes it hold the same thing
struct same_element
{
   template<class T>
   bool operator()(const T &l, const kv &r) const
   {  return l.key == r.key && l.val == r.val;  }
};

//The (key, value) pair that describes an element
struct to_kv
{
   template<class T>
   kv operator()(const T &e) const
   {
      kv r;
      r.key = e.key;
      r.val = e.val;
      return r;
   }
};

//The key of an element
struct to_key
{
   template<class T>
   std::size_t operator()(const T &e) const
   {  return e.key;  }
};

//For std::adjacent_find: true when two consecutive elements are not in key
//order, which is all an algorithm that is not stable has to leave behind
struct out_of_key_order
{
   template<class T, class U>
   bool operator()(const T &a, const U &b) const
   {  return b.key < a.key;  }
};

//For std::adjacent_find: true when two consecutive elements are not in the
//order a stable algorithm must leave them, that is by key and, among
//equivalent keys, by the value that says where they came from
struct out_of_stable_order
{
   template<class T, class U>
   bool operator()(const T &a, const U &b) const
   {  return b.key < a.key || (a.key == b.key && !(a.val < b.val));  }
};

//For std::find_if: true when an element is not one of those fill_buffer wrote
struct not_buffer_element
{
   template<class T>
   bool operator()(const T &e) const
   {  return e.key != e.val;  }
};

//For std::adjacent_find: true when two values do not follow each other
struct not_consecutive
{
   bool operator()(std::size_t a, std::size_t b) const
   {  return b != a + 1u;  }
};

//True if the range holds the "n" values fill_buffer wrote, in any order. The
//algorithms that rotate a hole leave them shifted by one place with respect to
//the ones that swap, so no caller may depend on their order, and neither may
//these tests. (std::is_permutation would say this in one call, but it is C++11
//and these tests build as C++03 too.)
inline bool is_buffer_permutation(const test_type *p, std::size_t n)
{
   if(!n){
      return true;      //Nothing was given to the algorithm, nothing to check
   }
   //fill_buffer writes the same in both halves
   if(std::find_if(p, p+n, not_buffer_element()) != (p+n)){
      return false;
   }

   boost::movelib::unique_ptr<std::size_t[]> got(new std::size_t[n]);
   std::transform(p, p+n, got.get(), to_key());
   std::sort(got.get(), got.get()+n);

   //Every value is there and only once when, once sorted, they run one after
   //the other from the first one fill_buffer wrote
   return got[0] == BufKeyBase
       && std::adjacent_find(got.get(), got.get()+n, not_consecutive()) == (got.get()+n);
}

#endif   //BOOST_MOVE_TEST_INTERNAL_TEST_UTIL_HPP
