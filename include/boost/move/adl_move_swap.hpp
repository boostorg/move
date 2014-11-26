//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright 2007, 2008 Steven Watanabe, Joseph Gauterin, Niels Dekker
// (C) Copyright Ion Gaztanaga 2005-2013. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_MOVE_ADL_MOVE_SWAP_HPP
#define BOOST_MOVE_ADL_MOVE_SWAP_HPP

#if defined(_MSC_VER)
#  pragma once
#endif

//Based on Boost.Core's swap.
//Many thanks to Steven Watanabe, Joseph Gauterin and Niels Dekker.

#include <boost/config.hpp>
#include <cstddef> //for std::size_t

//Try to avoid including <algorithm>, as it's quite big
#if defined(_MSC_VER) && defined(BOOST_DINKUMWARE_STDLIB)
   #include <utility>   //Dinkum libraries define std::swap in utility which is lighter than algorithm
#elif defined(BOOST_GNU_STDLIB)
   #if (__GNUC__ < 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ < 3))
      #include <bits/stl_algobase.h>   //algobase is lighter than <algorithm>
   #elif ((__GNUC__ == 4) && (__GNUC_MINOR__ == 3))
      //In GCC 4.3 a tiny stl_move.h was created with swap and move utilities
      #include <bits/stl_move.h>   //algobase is much lighter than <algorithm>
   #else
      //In GCC 4.4 stl_move.h was renamed to move.h
      #include <bits/move.h>   //algobase is much lighter than <algorithm>
   #endif
#elif defined(_LIBCPP_VERSION)
   #include <type_traits>  //The initial import of libc++ defines std::swap and still there
#else //Fallback
   #include <algorithm>
#endif

#include <boost/move/utility_core.hpp> //for boost::move

namespace boost_move_adl_swap{

template<class T>
void swap_proxy(T& left, T& right, typename boost::move_detail::enable_if_c<!boost::move_detail::has_move_emulation_enabled_impl<T>::value>::type* = 0)
{
   //use std::swap if argument dependent lookup fails
   //Use using directive ("using namespace xxx;") instead as some older compilers
   //don't do ADL with using declarations ("using ns::func;").
   using namespace std;
   swap(left, right);
}

template<class T>
void swap_proxy(T& left, T& right, typename boost::move_detail::enable_if_c<boost::move_detail::has_move_emulation_enabled_impl<T>::value>::type* = 0)
{
   T tmp(::boost::move(left));
   left = ::boost::move(right);
   right = ::boost::move(tmp);
}

template<class T, std::size_t N>
void swap_proxy(T (& left)[N], T (& right)[N])
{
   for (std::size_t i = 0; i < N; ++i){
      ::boost_move_adl_swap::swap_proxy(left[i], right[i]);
   }
}

}  //namespace boost_move_adl_swap {

namespace boost{

// adl_move_swap has two template arguments, instead of one, to
// avoid ambiguity when swapping objects of a Boost type that does
// not have its own boost::swap overload.
template<class T>
void adl_move_swap(T& left, T& right)
{
   ::boost_move_adl_swap::swap_proxy(left, right);
}

}  //namespace boost{

#endif   //#ifndef BOOST_MOVE_ADL_MOVE_SWAP_HPP
