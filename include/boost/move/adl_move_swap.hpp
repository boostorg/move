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

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

//Based on Boost.Core's swap.
//Many thanks to Steven Watanabe, Joseph Gauterin and Niels Dekker.
#include <cstddef> //for std::size_t
#include <boost/move/detail/workaround.hpp>  //forceinline
#include <boost/move/utility_core.hpp> //for boost::move
#include <boost/move/detail/type_traits.hpp> //for is_adl_swappable

#if !defined(BOOST_MOVE_DOXYGEN_INVOKED)

//std::swap is never called (and no standard header is included). A swap found by argument
//dependent lookup is used (this includes std::swap for types associated with namespace std),
//otherwise a move-based swap is used, as std::swap does. std::swap specializations for
//user types are not used (C++20 does not allow them). The behavior is the same in all
//C++ standards, including C++03 with Boost.Move's move emulation.
namespace boost_move_adl_swap{

//Hides the "swap" functions of the enclosing namespaces, so swap(x, y) only
//finds the candidates found by argument dependent lookup (see is_adl_swappable)
void swap();

template<class T>
BOOST_MOVE_FORCEINLINE BOOST_MOVE_CXX20_CONSTEXPR
   typename boost::move_detail::enable_if_c<boost::move_detail::is_adl_swappable<T>::value, void>::type
      swap_proxy(T& x, T& y)
{  swap(x, y);  }

template<class T>
BOOST_MOVE_FORCEINLINE BOOST_MOVE_CXX20_CONSTEXPR
   typename boost::move_detail::enable_if_c<!boost::move_detail::is_adl_swappable<T>::value, void>::type
      swap_proxy(T& x, T& y)
{  T t(::boost::move(x)); x = ::boost::move(y); y = ::boost::move(t);  }

template<class T, std::size_t N>
BOOST_MOVE_CXX20_CONSTEXPR void swap_proxy(T (& x)[N], T (& y)[N])
{
   for (std::size_t i = 0; i < N; ++i){
      ::boost_move_adl_swap::swap_proxy(x[i], y[i]);
   }
}

}  //namespace boost_move_adl_swap{

#endif   //!defined(BOOST_MOVE_DOXYGEN_INVOKED)

namespace boost{

//! Exchanges the values of a and b, using Argument Dependent Lookup (ADL) to select a
//! specialized swap function if available.
//! If no specialized swap function is available, a move-based swap
//! is called
template<class T>
BOOST_MOVE_FORCEINLINE BOOST_MOVE_CXX20_CONSTEXPR void adl_move_swap(T& x, T& y)
{
   ::boost_move_adl_swap::swap_proxy(x, y);
}

//! Exchanges elements between range [first1, last1) and another range starting at first2
//! using boost::adl_move_swap.
//! 
//! Parameters:
//!   first1, last1   -   the first range of elements to swap
//!   first2   -   beginning of the second range of elements to swap
//!
//! Type requirements:
//!   - ForwardIt1, ForwardIt2 must meet the requirements of ForwardIterator.
//!   - The types of dereferenced ForwardIt1 and ForwardIt2 must meet the
//!     requirements of Swappable
//!
//! Return value: Iterator to the element past the last element exchanged in the range
//! beginning with first2.
template<class ForwardIt1, class ForwardIt2>
ForwardIt2 adl_move_swap_ranges(ForwardIt1 first1, ForwardIt1 last1, ForwardIt2 first2)
{
    while (first1 != last1) {
      ::boost::adl_move_swap(*first1, *first2);
      ++first1;
      ++first2;
    }
   return first2;
}

template<class BidirIt1, class BidirIt2>
BidirIt2 adl_move_swap_ranges_backward(BidirIt1 first1, BidirIt1 last1, BidirIt2 last2)
{
   while (first1 != last1) {
      ::boost::adl_move_swap(*(--last1), *(--last2));
   }
   return last2;
}

template<class ForwardIt1, class ForwardIt2>
void adl_move_iter_swap(ForwardIt1 a, ForwardIt2 b)
{
   boost::adl_move_swap(*a, *b); 
}

}  //namespace boost{

#endif   //#ifndef BOOST_MOVE_ADL_MOVE_SWAP_HPP
