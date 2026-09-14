//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2022-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#ifndef BOOST_MOVE_DETAIL_SEARCH_HPP
#define BOOST_MOVE_DETAIL_SEARCH_HPP

#include <boost/move/detail/iterator_traits.hpp>

#if defined(BOOST_CLANG) || (defined(BOOST_GCC) && (BOOST_GCC >= 40600))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

namespace boost {
namespace movelib {

template <class RandIt, class T, class Compare>
RandIt lower_bound
   (RandIt first, const RandIt last, const T& key, Compare comp)
{
   typedef typename iter_size<RandIt>::type size_type;
   size_type len = size_type(last - first);
   RandIt middle;

   while (len) {
      size_type step = size_type(len >> 1);
      middle = first;
      middle += step;

      if (comp(*middle, key)) {
         first = ++middle;
         len = size_type(len - (step + 1));
      }
      else{
         len = step;
      }
   }
   return first;
}

template <class RandIt, class T, class Compare>
RandIt upper_bound
   (RandIt first, const RandIt last, const T& key, Compare comp)
{
   typedef typename iter_size<RandIt>::type size_type;
   size_type len = size_type(last - first);
   RandIt middle;

   while (len) {
      size_type step = size_type(len >> 1);
      middle = first;
      middle += step;

      if (!comp(key, *middle)) {
         first = ++middle;
         len = size_type(len - (step + 1));
      }
      else{
         len = step;
      }
   }
   return first;
}


//First element of [first, last) that is greater than "key". Tries exponential
//probe to limit the range and a binary search to find the right key position.
//Useful when the element is expected to be close to "first".
template <class RandIt, class T, class Compare>
RandIt gallop_upper_bound
   (RandIt first, const RandIt last, const T& key, Compare comp)
{
   typedef typename iter_size<RandIt>::type size_type;
   size_type const len = size_type(last - first);
   size_type prev = 0u, step = 1u;

   while(step < len && !comp(key, first[step])){
      prev = step;
      step = size_type(step*2u);
   }
   if(step > len){
      step = len;
   }
   return boost::movelib::upper_bound(first+prev, first+step, key, comp);
}

}  //namespace movelib {
}  //namespace boost {

#if defined(BOOST_CLANG) || (defined(BOOST_GCC) && (BOOST_GCC >= 40600))
#pragma GCC diagnostic pop
#endif

#endif   //#define BOOST_MOVE_DETAIL_SEARCH_HPP
