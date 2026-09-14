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
// Runs adaptive_merge with the combinations of range lengths, number of different
// keys, key overlapping and additional memory that select a different path in the
// algorithm.
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

#include <boost/move/algo/adaptive_merge.hpp>
#include <boost/move/algo/detail/merge_sort.hpp>
#include <boost/move/core.hpp>
#include <boost/move/unique_ptr.hpp>

#include "adaptive_test_util.hpp"

using boost::movelib::detail_adaptive::ceil_sqrt;
using boost::movelib::detail_adaptive::adaptive_merge_n_keys_without_external_keys;

//How the keys of the second range relate to the keys of the first one
enum overlap_t
{
   overlap_full,  //Both ranges hold the same keys, so they fully interleave
   overlap_half,  //The second range starts in the middle of the keys of the first one
   overlap_none,  //Every key of the second range is above the keys of the first one
   overlap_count
};

//The stack buffer is disabled (StackBytes == 0) so that the additional memory is
//exactly the requested one and every memory dependent path can be reached.
void test_merge( std::size_t len1, std::size_t len2, std::size_t num_keys1, std::size_t num_keys2
               , overlap_t overlap, std::size_t xbuf_len)
{
   const std::size_t len = std::size_t(len1+len2);
   boost::movelib::unique_ptr<order_move_type[]> elements(new order_move_type[len]);

   fill_keys(elements.get(), len1, num_keys1, pattern_random);
   fill_keys(elements.get()+len1, len2, num_keys2, pattern_random);

   const std::size_t key_offset = overlap == overlap_none
                              ? num_keys1
                              : (overlap == overlap_half ? std::size_t(num_keys1/2u) : 0u);
   for(std::size_t i = 0; i != len2; ++i){
      elements[len1+i].key += key_offset;
   }

   const std::size_t max_key2 = std::size_t(key_offset+num_keys2);
   assign_vals(elements.get(), len, num_keys1 > max_key2 ? num_keys1 : max_key2);

   //adaptive_merge requires both ranges to be sorted
   raw_buffer<order_move_type> sort_buf(len1 > len2 ? len1 : len2);
   boost::movelib::merge_sort(elements.get(), elements.get()+len1, order_type_less(), sort_buf.data());
   boost::movelib::merge_sort(elements.get()+len1, elements.get()+len, order_type_less(), sort_buf.data());

   raw_buffer<order_move_type> xbuf(xbuf_len);
   boost::movelib::adaptive_merge<0u>
      ( elements.get(), elements.get()+len1, elements.get()+len
      , order_type_less(), xbuf.data(), xbuf_len);

   if(!is_order_type_ordered(elements.get(), len)){
      std::cout << "   N1: " << len1 << ", N2: " << len2 << ", Keys1: " << num_keys1
                << ", Keys2: " << num_keys2 << ", Overlap: " << int(overlap)
                << ", Buf: " << xbuf_len << '\n';
      BOOST_ERROR("adaptive_merge did not produce a stable order");
   }
}

//Number of unique values adaptive_merge_impl tries to collect from the first range.
//It is zero when the additional memory also holds the integral keys of the blocks.
std::size_t keys_to_collect(std::size_t len1, std::size_t len2, std::size_t xbuf_len)
{
   std::size_t l_block = ceil_sqrt(std::size_t(len1+len2));
   const std::size_t l_intbuf = xbuf_len >= l_block ? 0u : l_block;

   //Each element of the memory that is left after the block buffer holds this
   //many integral keys
   const std::size_t keys_per_element = sizeof(order_move_type)/sizeof(std::size_t);
   if(!l_intbuf && (xbuf_len-l_block)*keys_per_element >= (len1/l_block + len2/l_block)){
      return 0u;
   }

   if(xbuf_len > l_block){
      l_block = xbuf_len;
   }
   return std::size_t
      (l_intbuf + adaptive_merge_n_keys_without_external_keys(l_block, len1, len2, l_intbuf));
}

//Merges two ranges with the amounts of additional memory and the numbers of
//different keys that make the algorithm change its strategy:
//
// - Memory: none, less than the internal buffer, as much as the internal buffer
//   (no unique value is needed for it), enough to also hold the integral keys
//   (no key is collected at all) and enough to merge both ranges with a buffer.
//
// - Keys: less than the four needed to combine blocks (rotation based merge), enough
//   to reduce the block length and the key count and all the requested ones.
void test_key_and_buffer_combinations(std::size_t len1, std::size_t len2, overlap_t overlap)
{
   const std::size_t csqrtlen = ceil_sqrt(std::size_t(len1+len2));
   const std::size_t l_min = len1 < len2 ? len1 : len2;
   const std::size_t xbuf_lens[] =
      { 0u, 1u, std::size_t(csqrtlen/2u), std::size_t(csqrtlen-1u), csqrtlen
      , std::size_t(csqrtlen+csqrtlen/2u), std::size_t(2u*csqrtlen)
      , std::size_t(l_min-1u), l_min };

   for(std::size_t i = 0; i != sizeof(xbuf_lens)/sizeof(*xbuf_lens); ++i){
      const std::size_t xbuf_len = xbuf_lens[i];
      std::size_t key_counts[10];
      std::size_t n_key_counts = 0;

      //A buffered merge or a rotation based merge is used, so no key is collected
      if(xbuf_len >= l_min || len1 <= 2u*csqrtlen || len2 <= 2u*csqrtlen){
         key_counts[n_key_counts++] = 1u;
         key_counts[n_key_counts++] = len1;
      }
      //No unique value is collected, the keys are integers in the additional memory
      else if(!keys_to_collect(len1, len2, xbuf_len)){
         key_counts[n_key_counts++] = 1u;
         key_counts[n_key_counts++] = len1;
      }
      else{
         const std::size_t to_collect = keys_to_collect(len1, len2, xbuf_len);
         key_counts[n_key_counts++] = 1u;
         key_counts[n_key_counts++] = 2u;
         key_counts[n_key_counts++] = 3u;
         key_counts[n_key_counts++] = 4u;
         key_counts[n_key_counts++] = 5u;
         key_counts[n_key_counts++] = std::size_t(to_collect/2u);
         key_counts[n_key_counts++] = std::size_t(to_collect-1u);
         key_counts[n_key_counts++] = to_collect;
         key_counts[n_key_counts++] = std::size_t(to_collect+1u);
         key_counts[n_key_counts++] = len1;
      }

      for(std::size_t j = 0; j != n_key_counts; ++j){
         const std::size_t num_keys1 = key_counts[j];
         if(num_keys1 && num_keys1 <= len1){
            const std::size_t num_keys2 = num_keys1 < len2 ? num_keys1 : len2;
            test_merge(len1, len2, num_keys1, num_keys2, overlap, xbuf_len);
         }
      }
   }
}

//Empty ranges, single elements and ranges below the block length
void test_small_lengths()
{
   for(std::size_t len1 = 0; len1 != small_len_end; ++len1){
      for(std::size_t len2 = 0; len2 != small_len_end; ++len2){
         const std::size_t max_keys1 = len1 ? len1 : 1u;
         const std::size_t max_keys2 = len2 ? len2 : 1u;
         for(std::size_t xbuf_len = 0; xbuf_len != 5u; ++xbuf_len){
            for(int o = 0; o != overlap_count; ++o){
               const overlap_t overlap = static_cast<overlap_t>(o);
               test_merge(len1, len2, 1u, 1u, overlap, xbuf_len);
               test_merge(len1, len2, max_keys1, max_keys2, overlap, xbuf_len);
            }
         }
      }
   }
}

//Every length of a range of medium lengths, so that the last block of the
//combination step holds every possible number of elements
void test_medium_lengths()
{
   for(std::size_t len = small_len_end; len != medium_len_end; ++len){
      const std::size_t csqrtlen = ceil_sqrt(len);
      const std::size_t xbuf_lens[] = { 0u, std::size_t(csqrtlen/2u), csqrtlen };
      for(std::size_t len1 = 1u; len1 != len; ++len1){
         const std::size_t len2 = std::size_t(len-len1);
         for(std::size_t i = 0; i != sizeof(xbuf_lens)/sizeof(*xbuf_lens); ++i){
            for(int o = 0; o != overlap_count; ++o){
               const overlap_t overlap = static_cast<overlap_t>(o);
               test_merge(len1, len2, 1u, 1u, overlap, xbuf_lens[i]);
               test_merge(len1, len2, len1, len2, overlap, xbuf_lens[i]);
            }
         }
      }
   }
}

//The default StackBytes reserves a buffer in the stack when the supplied
//additional memory is smaller than that buffer. adaptive_merge merges with the
//buffer alone when it holds the smaller range, so the first lengths are twice a
//fraction of the buffer, leaving each half inside it, while the last ones exceed
//it and the buffer becomes the additional memory of the adaptive algorithm.
void test_stack_buffer()
{
   const std::size_t lens[] =
      //Halves that fit in the stack buffer
      { 2u*(stack_buffer_elements/8u)
      , 2u*(stack_buffer_elements/4u)
      , 2u*(stack_buffer_elements/2u)
      , 2u*(stack_buffer_elements-1u)
      //Halves bigger than the stack buffer
      , 4u*stack_buffer_elements
      , 16u*stack_buffer_elements
      , 128u*stack_buffer_elements };

   //The smaller of both ranges is the one compared against the stack buffer, so
   //the last length of the first group must fit in it and the first length of
   //the second group must not
   BOOST_MOVE_STATIC_ASSERT(stack_buffer_elements/8u > 0u);
   BOOST_MOVE_STATIC_ASSERT((2u*(stack_buffer_elements-1u))/2u < stack_buffer_elements);
   BOOST_MOVE_STATIC_ASSERT((4u*stack_buffer_elements)/2u > stack_buffer_elements);

   for(std::size_t i = 0; i != sizeof(lens)/sizeof(*lens); ++i){
      const std::size_t len  = lens[i];
      const std::size_t len1 = std::size_t(len/2u);
      boost::movelib::unique_ptr<order_move_type[]> elements(new order_move_type[len]);
      fill_pattern(elements.get(), len, len, pattern_random);
      raw_buffer<order_move_type> sort_buf(len);
      boost::movelib::merge_sort(elements.get(), elements.get()+len1, order_type_less(), sort_buf.data());
      boost::movelib::merge_sort(elements.get()+len1, elements.get()+len, order_type_less(), sort_buf.data());

      boost::movelib::adaptive_merge
         (elements.get(), elements.get()+len1, elements.get()+len, order_type_less());
      if(!is_order_type_ordered(elements.get(), len)){
         std::cout << "   N: " << len << ", stack buffer: " << stack_buffer_elements << '\n';
         BOOST_ERROR("adaptive_merge with stack buffer did not produce a stable order");
      }
   }
}

void instantiate_smalldiff_iterators()
{
   typedef randit<int, short> short_rand_it_t;
   boost::movelib::adaptive_merge<0u>
      (short_rand_it_t(), short_rand_it_t(), short_rand_it_t(), less_int());

   typedef randit<int, signed char> schar_rand_it_t;
   boost::movelib::adaptive_merge<0u>
      (schar_rand_it_t(), schar_rand_it_t(), schar_rand_it_t(), less_int());
}

int main()
{
   instantiate_smalldiff_iterators();

   std::srand(0);

   test_small_lengths();
   test_medium_lengths();
   test_stack_buffer();

   //Balanced and unbalanced range lengths, leaving different remainders in the
   //blocks of the combination step
   const std::size_t t = insertion_sort_threshold;
   const std::size_t m = medium_len;
   const std::size_t len_pairs[][2] =
      { {1u, m},                        {m, 1u}
      , {std::size_t(t/2u), m},         {m, std::size_t(t/2u)}
      , {std::size_t(m/8u), std::size_t(m-m/8u)}
      , {std::size_t(m/2u), std::size_t(m/2u)}
      , {std::size_t(m-3u), std::size_t(m+9u)}
      , {std::size_t(128u*t), std::size_t(128u*t)}
      , {std::size_t(256u*t+3u), std::size_t(128u*t+5u)} };

   for(std::size_t i = 0; i != sizeof(len_pairs)/sizeof(*len_pairs); ++i){
      for(int o = 0; o != overlap_count; ++o){
         test_key_and_buffer_combinations
            (len_pairs[i][0], len_pairs[i][1], static_cast<overlap_t>(o));
      }
   }

   //Ranges holding a very different number of keys
   const std::size_t unbalanced_bufs[] =
      { 0u, std::size_t(stack_buffer_elements/2u), stack_buffer_elements
      , std::size_t(8u*stack_buffer_elements) };
   for(std::size_t i = 0; i != sizeof(unbalanced_bufs)/sizeof(*unbalanced_bufs); ++i){
      test_merge(m, m, 1u, m, overlap_full, unbalanced_bufs[i]);
      test_merge(m, m, m, 1u, overlap_full, unbalanced_bufs[i]);
      test_merge(m, m, 7u, m, overlap_half, unbalanced_bufs[i]);
      test_merge(m, m, m, 7u, overlap_half, unbalanced_bufs[i]);
   }

   //Long ranges, with the keys collected from the first range and with the
   //integral keys held in the additional memory
   const std::size_t long_merge_len = std::size_t(long_len/2u);
   test_merge(long_merge_len, long_merge_len, long_merge_len, long_merge_len, overlap_full, 0u);
   test_merge(long_merge_len, long_merge_len, long_merge_len, long_merge_len, overlap_full
             , std::size_t(2u*ceil_sqrt(std::size_t(2u*long_merge_len))));

   return boost::report_errors();
}
