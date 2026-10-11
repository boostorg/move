//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2015-2026.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#ifndef BOOST_MOVE_DETAIL_ROTATE_HPP
#define BOOST_MOVE_DETAIL_ROTATE_HPP

#include <boost/move/detail/config_begin.hpp>
#include <boost/move/adl_move_swap.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/move/detail/iterator_traits.hpp>

#if defined(BOOST_CLANG) || (defined(BOOST_GCC) && (BOOST_GCC >= 40600))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

namespace boost {
namespace movelib {

//Modified version from "An Optimal In-Place Array Rotation Algorithm", Ching-Kuang Shene
template<typename Unsigned>
Unsigned gcd(Unsigned x, Unsigned y)
{
   if(0 == ((x &(x-1)) | (y & (y-1)))){
      return x < y ? x : y;
   }
   else{
      Unsigned z = 1;
      while((!(x&1)) & (!(y&1))){
         z = Unsigned(z << 1);
         x = Unsigned(x >> 1);
         y = Unsigned(y >> 1);
      }
      while(x && y){
         if(!(x&1))
            x = Unsigned(x >> 1);
         else if(!(y&1))
            y = Unsigned (y >> 1);
         else if(x >=y)
            x = Unsigned((x-y) >> 1u);
         else
            y = Unsigned((y-x) >> 1);
      }
      return Unsigned(z*(x+y));
   }
}

template<typename RandIt>
RandIt rotate_gcd(RandIt first, RandIt middle, RandIt last)
{
   typedef typename iter_size<RandIt>::type size_type;
   typedef typename iterator_traits<RandIt>::value_type value_type;

   if(first == middle)
      return last;
   if(middle == last)
      return first;
   const size_type middle_pos = size_type(middle - first);
   RandIt ret = last - middle_pos;
   if (middle == ret){
      boost::adl_move_swap_ranges(first, middle, middle);
   }
   else{
      const size_type length = size_type(last - first);
      for( RandIt it_i(first), it_gcd(it_i + gcd(length, middle_pos))
         ; it_i != it_gcd
         ; ++it_i){
         value_type temp(boost::move(*it_i));
         RandIt it_j = it_i;
         RandIt it_k = it_j+middle_pos;
         do{
            *it_j = boost::move(*it_k);
            it_j = it_k;
            size_type const left = size_type(last - it_j);
            it_k = left > middle_pos ? it_j + middle_pos : first + middle_pos - left;
         } while(it_k != it_i);
         *it_j = boost::move(temp);
      }
   }
   return ret;
}

}  //namespace movelib {
}  //namespace boost {

#if defined(BOOST_CLANG) || (defined(BOOST_GCC) && (BOOST_GCC >= 40600))
#pragma GCC diagnostic pop
#endif

#include <boost/move/detail/config_end.hpp>

#endif   //#define BOOST_MOVE_DETAIL_ROTATE_HPP
