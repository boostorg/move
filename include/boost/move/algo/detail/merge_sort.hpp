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

//! \file

#ifndef BOOST_MOVE_DETAIL_MERGE_SORT_HPP
#define BOOST_MOVE_DETAIL_MERGE_SORT_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/move/detail/config_begin.hpp>
#include <boost/move/detail/workaround.hpp>

#include <boost/move/utility_core.hpp>
#include <boost/move/algo/move.hpp>
#include <boost/move/algo/detail/merge.hpp>
#include <boost/move/detail/iterator_traits.hpp>
#include <boost/move/adl_move_swap.hpp>
#include <boost/move/detail/destruct_n.hpp>
#include <boost/move/algo/detail/insertion_sort.hpp>
#include <cassert>

#if defined(BOOST_CLANG) || (defined(BOOST_GCC) && (BOOST_GCC >= 40600))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

namespace boost {
namespace movelib {

// @cond

static const unsigned MergeSortInsertionSortThreshold = 16;

// Recursive top-down stable sort with no additional memory, moves elements by rotating.
// O(N log^2 N) in the worst case.
template <class RandIt, class Compare>
void stable_sort_bufferless_ONlogN2_recursive(RandIt first, RandIt last, Compare comp)
{
   typedef typename iter_size<RandIt>::type  size_type;
   if (size_type(last - first) <= size_type(MergeSortInsertionSortThreshold)) {
      insertion_sort(first, last, comp);
      return;
   }
   RandIt middle = first + (last - first) / 2;
   stable_sort_bufferless_ONlogN2_recursive(first, middle, comp);
   stable_sort_bufferless_ONlogN2_recursive(middle, last, comp);
   merge_bufferless_ONlogN_recursive
      (first, middle, last, size_type(middle - first), size_type(last - middle), comp);
}

// Insertion sorts consecutive runs of "step" elements, or of
// MergeSortInsertionSortThreshold when that is shorter, and returns the length
// the runs really got. The tail run is whatever is left over.
template<class RandIt, class Compare>
typename iter_size<RandIt>::type
   insertion_sort_step
      ( RandIt const first
      , typename iter_size<RandIt>::type const length
      , typename iter_size<RandIt>::type const step
      , Compare comp)
{
   typedef typename iter_size<RandIt>::type size_type;
   size_type const s = step < size_type(MergeSortInsertionSortThreshold)
                          ? step : size_type(MergeSortInsertionSortThreshold);
   size_type m = 0;

   while((length - m) > s){
      insertion_sort(first+m, first+m+s, comp);
      m = size_type(m + s);
   }
   insertion_sort(first+m, first+length, comp);
   return s;
}

// Recursive bottom-up stable sort with no additional memory, moves elements by rotating.
// insertion sorts short runs and then merges adjacent runs with merge_bufferless,
// doubling the run length until it covers everything. O(N log^2 N) in the worst case.
template<class RandIt, class Compare>
void stable_sort_bufferless_ONlogN2
   ( RandIt const first, RandIt const last, Compare comp)
{
   typedef typename iter_size<RandIt>::type       size_type;

   size_type L = size_type(last - first);

   //Sort the shortest runs, the step returns their length, which is the first
   //merge level below
   size_type h = insertion_sort_step(first, L, size_type(MergeSortInsertionSortThreshold), comp);

   for(bool do_merge = L > h; do_merge; h = size_type(h*2)){
      do_merge = (L - h) > h;
      size_type p0 = 0;
      if(do_merge){
         size_type const h_2 = size_type(2*h);
         while((L-p0) > h_2){
            merge_bufferless(first+p0, first+p0+h, first+p0+h_2, comp);
            p0 = size_type(p0 + h_2);
         }
      }
      if((L-p0) > h){
         merge_bufferless(first+p0, first+p0+h, last, comp);
      }
   }
}

// @endcond

template<class RandIt, class RandIt2, class Compare>
void merge_sort_copy( RandIt first, RandIt last
                   , RandIt2 dest, Compare comp)
{
   typedef typename iter_size<RandIt>::type         size_type;
   
   size_type const count = size_type(last - first);
   if(count <= MergeSortInsertionSortThreshold){
      insertion_sort_copy(first, last, dest, comp);
   }
   else{
      size_type const half = size_type(count/2u);
      merge_sort_copy(first + half, last        , dest+half   , comp);
      merge_sort_copy(first       , first + half, first + half, comp);
      merge_with_right_placed
         ( first + half, first + half + half
         , dest, dest+half, dest + count
         , comp);
   }
}

template<class RandIt, class RandItRaw, class Compare>
void merge_sort_uninitialized_copy( RandIt first, RandIt last
                                 , RandItRaw uninitialized
                                 , Compare comp)
{
   typedef typename iter_size<RandIt>::type       size_type;
   typedef typename iterator_traits<RandIt>::value_type value_type;

   size_type const count = size_type(last - first);
   if(count <= MergeSortInsertionSortThreshold){
      insertion_sort_uninitialized_copy(first, last, uninitialized, comp);
   }
   else{
      size_type const half = count/2;
      merge_sort_uninitialized_copy(first + half, last, uninitialized + half, comp);
      destruct_n<value_type, RandItRaw> d(uninitialized+half);
      d.incr(size_type(count-half));
      merge_sort_copy(first, first + half, first + half, comp);
      uninitialized_merge_with_right_placed
         ( first + half, first + half + half
         , uninitialized, uninitialized+half, uninitialized+count
         , comp);
      d.release();
   }
}

template<class RandIt, class RandItRaw, class Compare>
void merge_sort( RandIt first, RandIt last, Compare comp
               , RandItRaw uninitialized)
{
   typedef typename iter_size<RandIt>::type       size_type;
   typedef typename iterator_traits<RandIt>::value_type      value_type;

   size_type const count = size_type(last - first);
   if(count <= MergeSortInsertionSortThreshold){
      insertion_sort(first, last, comp);
   }
   else{
      size_type const half = size_type(count/2u);
      size_type const rest = size_type(count -  half);
      RandIt const half_it = first + half;
      RandIt const rest_it = first + rest;

      merge_sort_uninitialized_copy(half_it, last, uninitialized, comp);
      destruct_n<value_type, RandItRaw> d(uninitialized);
      d.incr(rest);
      merge_sort_copy(first, half_it, rest_it, comp);
      merge_with_right_placed
         ( uninitialized, uninitialized + rest
         , first, rest_it, last, antistable<Compare>(comp));
   }
}

///@cond

template<class RandIt, class RandItRaw, class Compare>
void merge_sort_with_constructed_buffer( RandIt first, RandIt last, Compare comp, RandItRaw buffer)
{
   typedef typename iter_size<RandIt>::type       size_type;

   size_type const count = size_type(last - first);
   if(count <= MergeSortInsertionSortThreshold){
      insertion_sort(first, last, comp);
   }
   else{
      size_type const half = size_type(count/2);
      size_type const rest = size_type(count -  half);
      RandIt const half_it = first + half;
      RandIt const rest_it = first + rest;

      merge_sort_copy(half_it, last, buffer, comp);
      merge_sort_copy(first, half_it, rest_it, comp);
      merge_with_right_placed
         (buffer, buffer + rest
         , first, rest_it, last, antistable<Compare>(comp));
   }
}

template<typename RandIt, typename Pointer,
         typename Distance, typename Compare>
void stable_sort_ONlogN_recursive(RandIt first, RandIt last, Pointer buffer, Distance buffer_size, Compare comp)
{
   typedef typename iter_size<RandIt>::type  size_type;
   if (size_type(last - first) <= size_type(MergeSortInsertionSortThreshold)) {
      insertion_sort(first, last, comp);
   }
   else {
      const size_type len = size_type(last - first) / 2u;
      const RandIt middle = first + len;
      if (len > ((buffer_size+1)/2)){
         stable_sort_ONlogN_recursive(first, middle, buffer, buffer_size, comp);
         stable_sort_ONlogN_recursive(middle, last, buffer, buffer_size, comp);
      }
      else{
         merge_sort_with_constructed_buffer(first, middle, comp, buffer);
         merge_sort_with_constructed_buffer(middle, last, comp, buffer);
      }
      merge_adaptive_ONlogN_recursive(first, middle, last,
         size_type(middle - first),
         size_type(last - middle),
         buffer, buffer_size,
         comp);
   }
}

template<typename BidirectionalIterator, typename Compare, typename RandRawIt>
void stable_sort_adaptive_ONlogN2(BidirectionalIterator first,
		                           BidirectionalIterator last,
		                           Compare comp,
                                 RandRawIt uninitialized,
                                 std::size_t uninitialized_len)
{
   typedef typename iterator_traits<BidirectionalIterator>::value_type  value_type;

   ::boost::movelib::adaptive_xbuf<value_type, RandRawIt> xbuf(uninitialized, uninitialized_len);
   xbuf.initialize_until(uninitialized_len, *first);
   stable_sort_ONlogN_recursive(first, last, uninitialized, uninitialized_len, comp);
}

///@endcond

}} //namespace boost {  namespace movelib{

#if defined(BOOST_CLANG) || (defined(BOOST_GCC) && (BOOST_GCC >= 40600))
#pragma GCC diagnostic pop
#endif

#include <boost/move/detail/config_end.hpp>

#endif //#ifndef BOOST_MOVE_DETAIL_MERGE_SORT_HPP
