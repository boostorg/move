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
//
// Runs adaptive_sort with the combinations of range length, number of different
// keys and additional memory that select a different path in the algorithm.
//
//////////////////////////////////////////////////////////////////////////////

#ifdef NDEBUG
#undef NDEBUG
#endif

#define BOOST_MOVE_ADAPTIVE_SORT_INVARIANTS

#include <cstdlib>   //std::srand
#include <iostream>  //std::cout

#include <boost/config.hpp>
#include <boost/core/lightweight_test.hpp>

#include <boost/move/algo/adaptive_sort.hpp>
#include <boost/move/core.hpp>
#include <boost/move/unique_ptr.hpp>

#include "adaptive_test_util.hpp"

using boost::movelib::detail_adaptive::ceil_sqrt_multiple;

//The stack buffer is disabled (StackBytes == 0) so that the additional memory is
//exactly the requested one and every memory dependent path can be reached.
void test_sort(std::size_t len, std::size_t num_keys, std::size_t xbuf_len, pattern_t pattern)
{
   boost::movelib::unique_ptr<order_move_type[]> elements(new order_move_type[len]);
   fill_pattern(elements.get(), len, num_keys, pattern);

   raw_buffer<order_move_type> xbuf(xbuf_len);
   boost::movelib::adaptive_sort<0u>
      (elements.get(), elements.get()+len, order_type_less(), xbuf.data(), xbuf_len);

   if(!is_order_type_ordered(elements.get(), len)){
      std::cout << "   N: " << len << ", Keys: " << num_keys << ", Buf: " << xbuf_len
                << ", Pattern: " << pattern_name(pattern) << '\n';
      BOOST_ERROR("adaptive_sort did not produce a stable order");
   }
}

//Internal buffer length that adaptive_sort_build_params calculates for this range
std::size_t internal_buffer_len(std::size_t len, std::size_t xbuf_len)
{
   std::size_t l_intbuf = ceil_sqrt_multiple(len);
   while(xbuf_len >= std::size_t(l_intbuf*2u)){
      l_intbuf = std::size_t(l_intbuf*2u);
   }
   return l_intbuf;
}

//Minimum number of keys adaptive_sort_build_params needs to run the ideal algorithm
std::size_t min_ideal_keys(std::size_t len, std::size_t l_intbuf)
{
   std::size_t n_keys = std::size_t(l_intbuf-1u);
   while(n_keys >= (len-l_intbuf-n_keys)/l_intbuf){
      --n_keys;
   }
   return std::size_t(n_keys+1u);
}

//Sorts "len" elements with the amounts of additional memory and the numbers of
//different keys that make the algorithm change its strategy:
//
// - Memory: none, less than the internal buffer, as much as the internal buffer
//   (the collected keys need not be unique), enough to also hold the integral keys
//   (no key is collected at all), enough to double the internal buffer and enough
//   to merge sort the whole range.
//
// - Keys: less than the four needed to combine blocks (rotation based sort), enough
//   to reduce the buffer and the key count, the minimum for the ideal algorithm and
//   enough to build double sized blocks.
void test_key_and_buffer_combinations(std::size_t len, pattern_t pattern)
{
   const std::size_t csqrtlen = ceil_sqrt_multiple(len);
   const std::size_t merge_sort_len = std::size_t(len-len/2u);
   const std::size_t xbuf_lens[] =
      { 0u, 1u, std::size_t(csqrtlen/2u), std::size_t(csqrtlen-1u), csqrtlen
      , std::size_t(csqrtlen+csqrtlen/2u), std::size_t(2u*csqrtlen), std::size_t(4u*csqrtlen)
      , std::size_t(len/4u), merge_sort_len };

   for(std::size_t i = 0; i != sizeof(xbuf_lens)/sizeof(*xbuf_lens); ++i){
      const std::size_t xbuf_len = xbuf_lens[i];

      //With that much memory the whole range is merge sorted, so keys are irrelevant
      if(xbuf_len >= merge_sort_len){
         test_sort(len, 1u, merge_sort_len, pattern);
         test_sort(len, len, merge_sort_len, pattern);
         continue;
      }

      const std::size_t l_intbuf   = internal_buffer_len(len, xbuf_len);
      const std::size_t n_min_keys = min_ideal_keys(len, l_intbuf);
      const std::size_t key_counts[] =
         { 1u, 2u, 3u, 4u, 5u
         , n_min_keys, std::size_t(n_min_keys+1u)
         , std::size_t(n_min_keys+l_intbuf-1u), std::size_t(n_min_keys+l_intbuf)
         , std::size_t(2u*l_intbuf-1u), std::size_t(2u*l_intbuf), std::size_t(2u*l_intbuf+1u)
         , len };

      for(std::size_t j = 0; j != sizeof(key_counts)/sizeof(*key_counts); ++j){
         if(key_counts[j] && key_counts[j] <= len){
            test_sort(len, key_counts[j], xbuf_len, pattern);
         }
      }
   }
}

//Ranges below and just above the insertion sort threshold
void test_small_lengths()
{
   for(std::size_t len = 0; len != small_len_end; ++len){
      const std::size_t xbuf_lens[] = { 0u, 1u, std::size_t(len/4u), std::size_t(len/2u), len };
      const std::size_t max_keys = len ? len : 1u;
      for(std::size_t num_keys = 1u; num_keys <= max_keys; ++num_keys){
         for(std::size_t i = 0; i != sizeof(xbuf_lens)/sizeof(*xbuf_lens); ++i){
            test_sort(len, num_keys, xbuf_lens[i], pattern_random);
         }
      }
   }
}

//Every length of a range of medium lengths, so that the last block of each
//combination step holds every possible number of elements
void test_medium_lengths()
{
   for(std::size_t len = small_len_end; len != medium_len_end; ++len){
      //Rotate the pattern so that the sweep also varies how the halves interleave
      const pattern_t pattern = static_cast<pattern_t>(len % pattern_count);
      const std::size_t csqrtlen = ceil_sqrt_multiple(len);
      const std::size_t xbuf_lens[] =
         { 0u, std::size_t(csqrtlen/2u), csqrtlen, std::size_t(2u*csqrtlen) };
      const std::size_t key_counts[] =
         { 2u, 4u, csqrtlen, std::size_t(2u*csqrtlen), len };

      for(std::size_t i = 0; i != sizeof(xbuf_lens)/sizeof(*xbuf_lens); ++i){
         for(std::size_t j = 0; j != sizeof(key_counts)/sizeof(*key_counts); ++j){
            if(key_counts[j] <= len){
               test_sort(len, key_counts[j], xbuf_lens[i], pattern);
            }
         }
      }
   }
}

//The default StackBytes reserves a buffer in the stack when the supplied
//additional memory is smaller than that buffer. Lengths below that buffer hold
//the whole range in it, lengths above it use it as the additional memory of the
//adaptive algorithm, so both sides of the limit are tested.
void test_stack_buffer()
{
   const std::size_t lens[] =
      //Ranges that fit in the stack buffer
      { stack_buffer_elements/8u
      , stack_buffer_elements/4u
      , stack_buffer_elements/2u
      , stack_buffer_elements-1u
      //Ranges bigger than the stack buffer
      , 2u*stack_buffer_elements
      , 8u*stack_buffer_elements
      , 64u*stack_buffer_elements };

   //The smallest length must still hold one element, the last length of the
   //first group must fit in the stack buffer and the first length of the second
   //group must not
   BOOST_MOVE_STATIC_ASSERT(stack_buffer_elements/8u > 0u);
   BOOST_MOVE_STATIC_ASSERT(stack_buffer_elements-1u < stack_buffer_elements);
   BOOST_MOVE_STATIC_ASSERT(2u*stack_buffer_elements > stack_buffer_elements);

   for(std::size_t i = 0; i != sizeof(lens)/sizeof(*lens); ++i){
      const std::size_t len = lens[i];
      boost::movelib::unique_ptr<order_move_type[]> elements(new order_move_type[len]);
      fill_pattern(elements.get(), len, len, pattern_random);
      boost::movelib::adaptive_sort(elements.get(), elements.get()+len, order_type_less());
      if(!is_order_type_ordered(elements.get(), len)){
         std::cout << "   N: " << len << ", stack buffer: " << stack_buffer_elements << '\n';
         BOOST_ERROR("adaptive_sort with stack buffer did not produce a stable order");
      }
   }
}

void instantiate_smalldiff_iterators()
{
   typedef randit<int, short> short_rand_it_t;
   boost::movelib::adaptive_sort<0u>(short_rand_it_t(), short_rand_it_t(), less_int());

   typedef randit<int, signed char> schar_rand_it_t;
   boost::movelib::adaptive_sort<0u>(schar_rand_it_t(), schar_rand_it_t(), less_int());
}

int main()
{
   instantiate_smalldiff_iterators();

   std::srand(0);

   test_small_lengths();
   test_medium_lengths();
   test_stack_buffer();

   //Lengths leaving different remainders in the blocks of each combination step.
   //They double from the insertion sort threshold, most of them shifted by a few
   //elements so that the last block of a step is only partially filled
   const std::size_t t = insertion_sort_threshold;
   const std::size_t lens[] =
      { std::size_t(t+1u),       std::size_t(2u*t+1u),   std::size_t(4u*t)
      , std::size_t(8u*t+1u),    std::size_t(16u*t),     std::size_t(32u*t-1u)
      , std::size_t(64u*t),      std::size_t(128u*t+1u), std::size_t(256u*t+3u) };
   for(std::size_t i = 0; i != sizeof(lens)/sizeof(*lens); ++i){
      test_key_and_buffer_combinations(lens[i], pattern_random);
   }

   for(int p = 0; p != pattern_count; ++p){
      test_key_and_buffer_combinations(medium_len, static_cast<pattern_t>(p));
   }

   //A combination step of a long range needs more tags than local_key_count, so the
   //collected keys are used instead of the local array of integral tags
   test_sort(long_len, long_len, 0u, pattern_random);
   //Same length, but the integral keys are held in the additional memory
   test_sort(long_len, long_len, std::size_t(2u*ceil_sqrt_multiple(long_len)), pattern_random);

   return boost::report_errors();
}
