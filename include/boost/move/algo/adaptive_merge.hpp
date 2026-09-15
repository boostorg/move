//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2015-2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_MOVE_ADAPTIVE_MERGE_HPP
#define BOOST_MOVE_ADAPTIVE_MERGE_HPP

#include <boost/move/detail/config_begin.hpp>
#include <boost/move/algo/detail/adaptive_sort_merge.hpp>
#include <boost/move/detail/type_traits.hpp>
#include <cassert>

#if defined(BOOST_CLANG) || (defined(BOOST_GCC) && (BOOST_GCC >= 40600))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#endif

namespace boost {
namespace movelib {

///@cond
namespace detail_adaptive {

//Combines the blocks tagging them with "keys" compared with "key_comp", which are
//either the values collected from the range compared with "comp", or integers
//compared with less()
template<class RandItKeys, class KeyCompare, class RandIt, class Compare, class XBuf>
inline void adaptive_merge_combine_blocks_with_keys
   ( RandItKeys const keys
   , KeyCompare key_comp
   , RandIt const first_data
   , typename iter_size<RandIt>::type const len
   , typename iter_size<RandIt>::type const l_combine
   , typename iter_size<RandIt>::type const l_combine1
   , typename iter_size<RandIt>::type const l_block
   , bool use_internal_buf
   , bool xbuf_used
   , Compare comp
   , XBuf & xbuf
   )
{
   typedef typename iter_size<RandIt>::type       size_type;
   boost::movelib::ignore(len);

   size_type n_block_a, n_block_b, l_irreg1, l_irreg2;
   combine_params( keys, key_comp, l_combine
                 , l_combine1, l_block, xbuf
                 , n_block_a, n_block_b, l_irreg1, l_irreg2);   //Outputs
   if(xbuf_used){
      op_merge_blocks_with_buf
         ( keys, key_comp, first_data, l_block, l_irreg1, n_block_a, n_block_b
         , l_irreg2, comp, move_op(), xbuf.data());
      BOOST_MOVE_ADAPTIVE_SORT_PRINT_L1("   A mrg xbf: ", len);
   }
   else if(use_internal_buf){
      op_merge_blocks_with_buf
         ( keys, key_comp, first_data, l_block, l_irreg1, n_block_a, n_block_b
         , l_irreg2, comp, swap_op(), first_data-l_block);
      BOOST_MOVE_ADAPTIVE_SORT_PRINT_L2("   A mrg buf: ", len);
   }
   else{
      merge_blocks_bufferless
         (keys, key_comp, first_data, l_block, l_irreg1, n_block_a, n_block_b, l_irreg2, comp);
      BOOST_MOVE_ADAPTIVE_SORT_PRINT_L1("   A mrg nbf: ", len);
   }
}

template<class RandIt, class Compare, class XBuf>
inline void adaptive_merge_combine_blocks( RandIt first
                                      , typename iter_size<RandIt>::type len1
                                      , typename iter_size<RandIt>::type len2
                                      , typename iter_size<RandIt>::type collected
                                      , typename iter_size<RandIt>::type n_keys
                                      , typename iter_size<RandIt>::type l_block
                                      , bool use_internal_buf
                                      , bool xbuf_used
                                      , Compare comp
                                      , XBuf & xbuf
                                      )
{
   typedef typename iter_size<RandIt>::type       size_type;

   size_type const len = size_type(len1+len2);
   size_type const l_combine  = size_type(len-collected);
   size_type const l_combine1 = size_type(len1-collected);

   if(n_keys){
      RandIt const first_data = first+collected;
      RandIt const keys = first;
      BOOST_MOVE_ADAPTIVE_SORT_PRINT_L2("   A combine: ", len);
      if(xbuf_used){
         if(xbuf.size() < l_block){
            xbuf.initialize_until(l_block, *first);
         }
         assert(xbuf.size() >= l_block);
      }
      //Tag with integers held in a local array instead of collected values,
      //makes comparisons trivial and avoids additional collecting.
      size_type const upper_n_keys = size_type(l_combine/l_block + 1u);
      if(upper_n_keys <= AdaptiveLocalKeyCount){
         unsigned char uint_keys[AdaptiveLocalKeyCount];
         adaptive_merge_combine_blocks_with_keys
            ( uint_keys, less(), first_data, len, l_combine, l_combine1, l_block
            , use_internal_buf, xbuf_used, comp, xbuf);
      }
      else{
         adaptive_merge_combine_blocks_with_keys
            ( keys, comp, first_data, len, l_combine, l_combine1, l_block
            , use_internal_buf, xbuf_used, comp, xbuf);
      }
   }
   else{
      //No value was collected, the keys are integers stored in the additional
      //memory, just after the block buffer (see adaptive_merge_n_keys_intbuf)
      xbuf.shrink_to_fit(l_block);
      if(xbuf.size() < l_block){
         xbuf.initialize_until(l_block, *first);
      }
      size_type *const uint_keys = xbuf.template aligned_trailing<size_type>(l_block);
      size_type n_block_a, n_block_b, l_irreg1, l_irreg2;
      combine_params( uint_keys, less(), l_combine
                     , l_combine1, l_block, xbuf
                     , n_block_a, n_block_b, l_irreg1, l_irreg2, true);   //Outputs
      BOOST_MOVE_ADAPTIVE_SORT_PRINT_L2("   A combine: ", len);
      assert(xbuf.size() >= l_block);
      op_merge_blocks_with_buf
         (uint_keys, less(), first, l_block, l_irreg1, n_block_a, n_block_b, l_irreg2, comp, move_op(), xbuf.data());
      xbuf.clear();
      BOOST_MOVE_ADAPTIVE_SORT_PRINT_L1("   A mrg buf: ", len);
   }
}

template<class RandIt, class Compare, class XBuf>
inline void adaptive_merge_final_merge( RandIt first
                                      , typename iter_size<RandIt>::type len1
                                      , typename iter_size<RandIt>::type len2
                                      , typename iter_size<RandIt>::type collected
                                      , typename iter_size<RandIt>::type l_intbuf
                                      , typename iter_size<RandIt>::type //l_block
                                      , bool //use_internal_buf
                                      , bool xbuf_used
                                      , Compare comp
                                      , XBuf & xbuf
                                      )
{
   typedef typename iter_size<RandIt>::type       size_type;

   size_type n_keys = size_type(collected-l_intbuf);
   size_type len = size_type(len1+len2);
   //If integral keys were used, nothing was collected and there is nothing to merge back
   if (!xbuf_used || n_keys) {
      xbuf.clear();
      const size_type middle = xbuf_used && n_keys ? n_keys: collected;
      unstable_sort(first, first + middle, comp, xbuf);
      BOOST_MOVE_ADAPTIVE_SORT_PRINT_L2("   A k/b srt: ", len);
      stable_merge(first, first + middle, first + len, comp, xbuf);
   }
   BOOST_MOVE_ADAPTIVE_SORT_PRINT_L1("   A fin mrg: ", len);
}

template<class SizeType>
inline static SizeType adaptive_merge_n_keys_without_external_keys(SizeType l_block, SizeType len1, SizeType len2, SizeType l_intbuf)
{
   typedef SizeType size_type;
   //This is the minimum number of keys to implement the ideal algorithm
   size_type n_keys = size_type(len1/l_block + len2/l_block);
   const size_type second_half_blocks = size_type(len2/l_block);
   const size_type first_half_aux = size_type(len1 - l_intbuf);
   while(n_keys >= ((first_half_aux-n_keys)/l_block + second_half_blocks)){
      --n_keys;
   }
   ++n_keys;
   return n_keys;
}

//Number of blocks, that is, the number of keys the ideal algorithm needs when the
//keys are not collected from the first range
template<class SizeType>
inline static SizeType adaptive_merge_n_keys_with_external_keys(SizeType l_block, SizeType len1, SizeType len2, SizeType l_intbuf)
{
   typedef SizeType size_type;
   return size_type((len1-l_intbuf)/l_block + len2/l_block);
}

//True if the additional memory can hold a buffer of "l_block" elements and, in the
//space that is left after it, one integral key per block
template<class SizeType, class Xbuf>
inline bool adaptive_merge_xbuf_holds_keys(SizeType l_block, SizeType len1, SizeType len2, Xbuf & xbuf)
{
   return xbuf.template supports_aligned_trailing<SizeType>
      (l_block, adaptive_merge_n_keys_with_external_keys(l_block, len1, len2, SizeType(0u)));
}

template<class SizeType, class Xbuf>
inline SizeType adaptive_merge_n_keys_intbuf(SizeType &rl_block, SizeType len1, SizeType len2, Xbuf & xbuf, SizeType &l_intbuf_inout)
{
   typedef SizeType size_type;
   size_type l_block = rl_block;
   size_type l_intbuf = xbuf.capacity() >= l_block ? 0u : l_block;
   size_type n_keys = 0u;

   //If the additional memory holds the block buffer and the integral keys of all the
   //blocks, no unique value has to be collected from the first range.
   if(adaptive_merge_xbuf_holds_keys(l_block, len1, len2, xbuf)){
      //Binary search the biggest block that has room for integral keys.
      size_type lo = l_block, hi = xbuf.capacity();
      while(lo < hi){
         size_type const mid = size_type(lo + size_type(hi - lo + 1)/2u);
         if(adaptive_merge_xbuf_holds_keys(mid, len1, len2, xbuf))
            lo = mid;
         else
            hi = size_type(mid - 1u);
      }
      l_block = lo;
   }
   else{
      //The whole additional memory is used as block buffer and the keys are unique
      //values collected from the first range
      if (xbuf.capacity() > l_block){
         l_block = xbuf.capacity();
      }

      //This is the minimum number of keys to implement the ideal algorithm
      n_keys = adaptive_merge_n_keys_without_external_keys(l_block, len1, len2, l_intbuf);
      assert(n_keys >= ((len1-l_intbuf-n_keys)/l_block + len2/l_block));
   }

   l_intbuf_inout = l_intbuf;
   rl_block = l_block;
   return n_keys;
}

//Rotation-based merge that takes advantage of any additional memory
template<class RandIt, class Compare, class XBuf>
inline void adaptive_merge_rotation_merge
   ( RandIt const first, RandIt const middle, RandIt const last
   , typename iter_size<RandIt>::type const len1
   , typename iter_size<RandIt>::type const len2
   , Compare comp
   , XBuf & xbuf)
{
   typedef typename iter_size<RandIt>::type size_type;
   size_type const cap = xbuf.capacity();
   if (len1 && len2) {
      if (!cap) {
         //merge_bufferless_ON2 is rotation-based. The squared term is paid only
         //on the short range, so once min(len1,len2) <= 2*ceil_sqrt(len) ON2 beats
         //ONlogN.
         if (min_value<size_type>(len1, len2) <= size_type(2u*ceil_sqrt(size_type(len1+len2))))
            merge_bufferless_ON2(first, middle, last, comp);
         else
            merge_bufferless(first, middle, last, comp);
      }
      else {
         //The buffer might hold values from a previous step
         xbuf.clear();
         xbuf.initialize_until(cap, *first);
         merge_adaptive_ONlogN_recursive(first, middle, last, len1, len2, xbuf.data(), cap, comp);
         xbuf.clear();
      }
   }
}

// Main explanation of the merge algorithm.
//
// csqrtlen = ceil(sqrt(len));
//
// * First, csqrtlen [to be used as buffer] + (len/csqrtlen - 1) [to be used as keys] => to_collect
//   unique elements are extracted from elements to be sorted and placed in the beginning of the range.
//
// * Step "combine_blocks": the leading (len1-to_collect) elements plus trailing len2 elements
//   are merged with a non-trivial ("smart") algorithm to form an ordered range trailing "len-to_collect" elements.
//
//   Explanation of the "combine_blocks" step:
//
//         * Trailing [first+to_collect, first+len1) elements are divided in groups of cqrtlen elements.
//           Remaining elements that can't form a group are grouped in front of those elements.
//         * Trailing [first+len1, first+len1+len2) elements are divided in groups of cqrtlen elements.
//           Remaining elements that can't form a group are grouped in the back of those elements.
//         * In parallel the following two steps are performed:
//             *  Groups are selection-sorted by first or last element (depending whether they are going
//                to be merged to left or right) and keys are reordered accordingly as an imitation-buffer.
//             * Elements of each block pair are merged using the csqrtlen buffer taking into account
//                if they belong to the first half or second half (marked by the key).
//
// * In the final merge step leading "to_collect" elements are merged with rotations
//   with the rest of merged elements in the "combine_blocks" step.
//
// Corner cases:
//
// * If no "to_collect" elements can be extracted:
//
//    * If more than a minimum number of elements is extracted
//      then reduces the number of elements used as buffer and keys in the
//      and "combine_blocks" steps. If "combine_blocks" has no enough keys due to this reduction
//      then uses a rotation based smart merge.
//
//    * If the minimum number of keys can't be extracted, a rotation-based merge is performed.
//
// * If auxiliary memory is more or equal than min(len1, len2), a buffered merge is performed.
//
// * If the len1 or len2 are less than 2*csqrtlen then a rotation-based merge is performed.
//
// * If auxiliary memory is available, it replaces the csqrtlen buffer and "combine_blocks"
//   uses blocks as long as that memory. Only the keys are extracted from the range.
//
// * If auxiliary memory can also hold one integral key per block, then no element is
//   extracted at all and "combine_blocks" uses those integral keys.
template<class RandIt, class Compare, class XBuf>
void adaptive_merge_impl
   ( RandIt first
   , typename iter_size<RandIt>::type len1
   , typename iter_size<RandIt>::type len2
   , Compare comp
   , XBuf & xbuf
   )
{
   typedef typename iter_size<RandIt>::type size_type;

   if(xbuf.capacity() >= min_value<size_type>(len1, len2)){
      buffered_merge( first, first+len1
                    , first + len1+len2, comp, xbuf);
   }
   else{
      const size_type len = size_type(len1+len2);
      //Calculate ideal parameters and try to collect needed unique keys
      size_type l_block = size_type(ceil_sqrt(len));

      //One range is not big enough to extract keys and the internal buffer so a
      //rotation-based based merge will do just fine
      if(len1 <= l_block*2 || len2 <= l_block*2){
         adaptive_merge_rotation_merge(first, first+len1, first+len1+len2, len1, len2, comp, xbuf);
         return;
      }

      //Detail the number of keys and internal buffer. If xbuf has enough memory, no
      //internal buffer is needed so l_intbuf will remain 0.
      size_type l_intbuf = 0;
      size_type n_keys = adaptive_merge_n_keys_intbuf(l_block, len1, len2, xbuf, l_intbuf);
      size_type const to_collect = size_type(l_intbuf+n_keys);
      //Try to extract needed unique values from the first range
      size_type const collected  = collect_unique(first, first+len1, to_collect, comp, xbuf, collect_sorted_t());
      BOOST_MOVE_ADAPTIVE_SORT_PRINT_L1("\n   A collect: ", len);

      //Not the minimum number of keys is not available on the first range, so fallback to rotations.
      //The range has very few distinct keys here, and the rotation-based merge exploits the long
      //equal runs with binary searches, so it beats adaptive_merge_rotation_merge in that case
      //(measured: at 3 distinct values a 0.25*sqrt(N) buffer costs 18% more comparisons for no
      //time gain), and the additional memory is not worth using.
      if(collected != to_collect && collected < 4){
         merge_bufferless(first, first+collected, first+len1, comp);
         merge_bufferless(first, first + len1, first + len1 + len2, comp);
         return;
      }

      //If not enough keys but more than minimum, adjust the internal buffer and key count
      bool use_internal_buf = collected == to_collect;
      if (!use_internal_buf){
         l_intbuf = 0u;
         n_keys = collected;
         l_block  = lblock_for_combine(l_intbuf, n_keys, len, use_internal_buf);
         //If use_internal_buf is false, then then internal buffer will be zero and rotation-based combination will be used
         l_intbuf = use_internal_buf ? l_block : 0u;
      }

      bool const xbuf_used = collected == to_collect && xbuf.capacity() >= l_block;
      //Merge trailing elements using smart merges
      adaptive_merge_combine_blocks(first, len1, len2, collected,   n_keys, l_block, use_internal_buf, xbuf_used, comp, xbuf);
      //Merge buffer and keys with the rest of the values
      adaptive_merge_final_merge   (first, len1, len2, collected, l_intbuf, l_block, use_internal_buf, xbuf_used, comp, xbuf);
   }
}

//Runs the merge with an internal buffer of StackBytes bytes held on the stack.
//It is a separate function so that the storage only exists in this frame.
template<std::size_t StackBytes, class RandIt, class Compare>
void adaptive_merge_with_stack_buffer
   ( RandIt first, typename iter_size<RandIt>::type len1
   , typename iter_size<RandIt>::type len2, Compare comp)
{
   typedef typename iterator_traits<RandIt>::value_type   value_type;
   typedef typename iter_size<RandIt>::type                size_type;

   typename ::boost::move_detail::aligned_storage
      < StackBytes
      , ::boost::move_detail::alignment_of<value_type>::value>::type storage;

   //The xbuf destructor destroys whatever is left in the storage
   adaptive_xbuf<value_type, value_type*, size_type> xbuf
      ( static_cast<value_type*>(static_cast<void*>(&storage))
      , (stack_buffer_capacity<StackBytes, size_type, value_type>()));
   adaptive_merge_impl(first, len1, len2, comp, xbuf);
}

//Reduces the ranges to merge and then runs the merge with the stack buffer if
//it is bigger than the supplied storage, and with the supplied storage otherwise.
template<std::size_t StackBytes, class RandIt, class Compare>
void adaptive_merge_dispatch
   ( RandIt first, RandIt middle, RandIt last, Compare comp
   , typename iterator_traits<RandIt>::value_type* uninitialized
   , typename iter_size<RandIt>::type uninitialized_len)
{
   typedef typename iter_size<RandIt>::type             size_type;
   typedef typename iterator_traits<RandIt>::value_type value_type;

   if (first == middle || middle == last){
      return;
   }

   //Reduce the ranges to merge if possible. Binary searches are used instead of
   //linear scans because the whole point of the trim is to exploit ranges that
   //are already (nearly) in place, and those are exactly the ranges a linear
   //scan traverses completely.
   RandIt first_high(middle);
   --first_high;
   if (!comp(*middle, *first_high)){
      return;   //Both ranges are already in order
   }
   //Leading elements of the first range below the first element of the second
   //one are already in place. upper_bound keeps equal elements of the first
   //range before those of the second one, preserving stability.
   first = boost::movelib::upper_bound(first, middle, *middle, comp);
   //Trailing elements of the second range above the last element of the first
   //one are already in place too. lower_bound keeps equal elements of the
   //second range after those of the first one.
   last  = boost::movelib::lower_bound(middle, last, *first_high, comp);

   size_type const len1 = size_type(middle - first);
   size_type const len2 = size_type(last - middle);

   if( StackBytes && sizeof(value_type) <= StackBytes
    && uninitialized_len < (stack_buffer_capacity<StackBytes, size_type, value_type>()) ){
      adaptive_merge_with_stack_buffer<StackBytes>(first, len1, len2, comp);
   }
   else{
      adaptive_xbuf<value_type, value_type*, size_type> xbuf(uninitialized, size_type(uninitialized_len));
      adaptive_merge_impl(first, len1, len2, comp, xbuf);
   }
}

}  //namespace detail_adaptive {

///@endcond

//! <b>Effects</b>: Merges two consecutive sorted ranges [first, middle) and [middle, last)
//!   into one sorted range [first, last) according to the given comparison function comp.
//!   The algorithm is stable (if there are equivalent elements in the original two ranges,
//!   the elements from the first range (preserving their original order) precede the elements
//!   from the second range (preserving their original order).
//!
//! <b>Requires</b>:
//!   - RandIt must meet the requirements of ValueSwappable and RandomAccessIterator.
//!   - The type of dereferenced RandIt must meet the requirements of MoveAssignable and MoveConstructible.
//!
//! <b>Parameters</b>:
//!   - first: the beginning of the first sorted range. 
//!   - middle: the end of the first sorted range and the beginning of the second
//!   - last: the end of the second sorted range
//!   - comp: comparison function object which returns true if the first argument is is ordered before the second.
//!   - uninitialized, uninitialized_len: raw storage starting on "uninitialized", able to hold "uninitialized_len"
//!      elements of type iterator_traits<RandIt>::value_type. Maximum performance is achieved when uninitialized_len
//!      is min(std::distance(first, middle), std::distance(middle, last)).
//!
//! <b>Throws</b>: If comp throws or the move constructor, move assignment or swap of the type
//!   of dereferenced RandIt throws.
//!
//! <b>Complexity</b>: Always K x O(N) comparisons and move assignments/constructors/swaps.
//!   Constant factor for comparisons and data movement is minimized when uninitialized_len
//!   is min(std::distance(first, middle), std::distance(middle, last)).
//!   Pretty good enough performance is achieved when uninitialized_len is
//!   ceil(sqrt(std::distance(first, last)))*2.
//!
//! <b>Note</b>: A constant amount of stack (see AdaptiveDefaultStackBytes) is
//!   also used as an internal buffer when it is bigger than "uninitialized_len",
//!   which keeps the O(1) extra memory guarantee. Use the overload that takes
//!   an explicit "StackBytes" to change or disable that buffer.
//!
//! <b>Caution</b>: Experimental implementation, not production-ready.
template<class RandIt, class Compare>
void adaptive_merge( RandIt first, RandIt middle, RandIt last, Compare comp
                , typename iterator_traits<RandIt>::value_type* uninitialized = 0
                , typename iter_size<RandIt>::type uninitialized_len = 0)
{
   ::boost::movelib::detail_adaptive::adaptive_merge_dispatch
      < ::boost::movelib::detail_adaptive::AdaptiveDefaultStackBytes>
         (first, middle, last, comp, uninitialized, uninitialized_len);
}

//! <b>Effects</b>: Same as the overload above, but "StackBytes" bytes of stack
//!   are used as the internal buffer whenever that is bigger than the supplied
//!   storage. Since it is a constant amount of memory, the O(1) extra memory
//!   guarantee is preserved. Small merges are several times faster with it,
//!   because they can avoid the block based algorithm altogether.
//!
//! <b>Parameters</b>:
//!   - StackBytes: size in bytes of the stack buffer. It must be given
//!      explicitly. Zero disables the stack buffer, and so does a
//!      value_type bigger than "StackBytes".
//!
//! <b>Caution</b>: Experimental implementation, not production-ready.
template<std::size_t StackBytes, class RandIt, class Compare>
void adaptive_merge( RandIt first, RandIt middle, RandIt last, Compare comp
                , typename iterator_traits<RandIt>::value_type* uninitialized = 0
                , typename iter_size<RandIt>::type uninitialized_len = 0)
{
   ::boost::movelib::detail_adaptive::adaptive_merge_dispatch<StackBytes>
      (first, middle, last, comp, uninitialized, uninitialized_len);
}

}  //namespace movelib {
}  //namespace boost {

#if defined(BOOST_CLANG) || (defined(BOOST_GCC) && (BOOST_GCC >= 40600))
#pragma GCC diagnostic pop
#endif

#include <boost/move/detail/config_end.hpp>

#endif   //#define BOOST_MOVE_ADAPTIVE_MERGE_HPP
