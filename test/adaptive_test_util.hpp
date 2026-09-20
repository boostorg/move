//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_MOVE_TEST_ADAPTIVE_TEST_UTIL_HPP
#define BOOST_MOVE_TEST_ADAPTIVE_TEST_UTIL_HPP

#include <boost/move/algo/detail/adaptive_sort_merge.hpp>
#include <boost/move/detail/force_ptr.hpp>
#include <boost/move/detail/workaround.hpp>
#include <boost/move/unique_ptr.hpp>
#include <cassert>
#include <cstddef>

#include "order_type.hpp"
#include "random_shuffle.hpp"

//Number of elements that fit in the stack buffer that adaptive_sort and
//adaptive_merge reserve by default (see AdaptiveDefaultStackBytes). Test lengths
//are expressed as fractions of this value instead of literals, so that they stay
//inside the buffer if the constant or the size of the element type changes.
static const std::size_t stack_buffer_elements =
   boost::movelib::detail_adaptive::AdaptiveDefaultStackBytes / sizeof(order_move_type);

//The smallest fraction used by the tests below must still hold one element
BOOST_MOVE_STATIC_ASSERT(stack_buffer_elements >= 8u);

//Length of the sorted runs the adaptive algorithms start from. Test lengths are
//written as multiples of it so that they keep leaving the same remainders in the
//blocks of a combination step if the threshold changes.
static const std::size_t insertion_sort_threshold =
   boost::movelib::MergeSortInsertionSortThreshold;

//Number of integral keys adaptive_sort tags blocks with in a local array. A
//combination step needing more tags than this uses the collected keys instead.
static const std::size_t local_key_count =
   boost::movelib::detail_adaptive::AdaptiveLocalKeyCount;

//Lengths below and just above the insertion sort threshold, and the medium
//lengths that follow them, where every block remainder still appears
static const std::size_t small_len_end  = 3u*insertion_sort_threshold;
static const std::size_t medium_len_end = 19u*insertion_sort_threshold;

//A length needing several combination steps, used as the reference size of the
//tests that do not sweep a range of lengths
static const std::size_t medium_len = 64u*insertion_sort_threshold;

//A combination step of this length needs more tags than local_key_count, because
//the number of tags a step uses grows with the square root of the length
static const std::size_t long_len = 2u*local_key_count*local_key_count;

//Key distributions. The number of different keys of a range determines how many
//unique values the adaptive algorithms can collect, and the distribution determines
//how much both halves of each merge step interleave.
enum pattern_t
{
   pattern_random,      //Keys in random order
   pattern_sorted,      //Keys in ascending order
   pattern_reverse,     //Keys in descending order
   pattern_organ_pipe,  //Ascending in the first half, descending in the second one
   pattern_sawtooth,    //Several ascending runs
   pattern_count
};

inline const char *pattern_name(pattern_t pattern)
{
   static const char *names[pattern_count] =
      { "random", "sorted", "reverse", "organ_pipe", "sawtooth" };
   return names[pattern];
}

//Writes in [elements, elements+count) exactly "num_keys" different keys,
//distributed according to "pattern". Values are not modified.
inline void fill_keys
   (order_move_type *elements, std::size_t count, std::size_t num_keys, pattern_t pattern)
{
   if(!count){
      return;
   }
   assert(num_keys && num_keys <= count);
   boost::movelib::unique_ptr<std::size_t[]> keys(new std::size_t[count]);

   //Ascending sequence holding all the keys. The rest of the patterns are
   //permutations of it, so they hold all the keys too.
   for(std::size_t i = 0; i != count; ++i){
      keys[i] = (i*num_keys)/count;
   }

   if(pattern == pattern_random){
      ::random_shuffle(keys.get(), keys.get()+count);
   }
   else if(pattern != pattern_sorted){
      boost::movelib::unique_ptr<std::size_t[]> permuted(new std::size_t[count]);
      if(pattern == pattern_reverse){
         for(std::size_t i = 0; i != count; ++i){
            permuted[i] = keys[count-1u-i];
         }
      }
      else if(pattern == pattern_organ_pipe){
         for(std::size_t i = 0; i != count; ++i){
            permuted[(i & 1u) ? (count-1u-i/2u) : (i/2u)] = keys[i];
         }
      }
      else{  //pattern_sawtooth: deal the ascending sequence in several runs
         const std::size_t n_runs = count < 8u ? 1u : 8u;
         std::size_t pos = 0;
         for(std::size_t run = 0; run != n_runs; ++run){
            for(std::size_t i = run; i < count; i += n_runs){
               permuted[pos++] = keys[i];
            }
         }
      }
      for(std::size_t i = 0; i != count; ++i){
         keys[i] = permuted[i];
      }
   }

   for(std::size_t i = 0; i != count; ++i){
      elements[i].key = keys[i];
   }
}

//Numbers the elements of each key in range order, so that a stable algorithm
//leaves the values of equal keys in ascending order.
inline void assign_vals(order_move_type *elements, std::size_t count, std::size_t max_keys)
{
   if(!count){
      return;
   }
   boost::movelib::unique_ptr<std::size_t[]> key_reps(new std::size_t[max_keys]);
   for(std::size_t i = 0; i != max_keys; ++i){
      key_reps[i] = 0u;
   }
   for(std::size_t i = 0; i != count; ++i){
      assert(elements[i].key < max_keys);
      elements[i].val = key_reps[elements[i].key]++;
   }
}

inline void fill_pattern
   (order_move_type *elements, std::size_t count, std::size_t num_keys, pattern_t pattern)
{
   fill_keys(elements, count, num_keys, pattern);
   assign_vals(elements, count, num_keys);
}

//Uninitialized storage for the additional memory the adaptive algorithms accept
template<class T>
class raw_buffer
{
   public:
   explicit raw_buffer(std::size_t count)
      : m_storage(count ? new char[sizeof(T)*count] : 0)
   {}

   T *data() const
   {  return m_storage.get() ? boost::move_detail::force_ptr<T*>(m_storage.get()) : (T*)0;  }

   private:
   boost::movelib::unique_ptr<char[]> m_storage;
};

#endif   //BOOST_MOVE_TEST_ADAPTIVE_TEST_UTIL_HPP
