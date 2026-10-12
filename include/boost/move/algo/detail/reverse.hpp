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
#ifndef BOOST_MOVE_DETAIL_REVERSE_HPP
#define BOOST_MOVE_DETAIL_REVERSE_HPP

#include <boost/move/detail/config_begin.hpp>
#include <boost/move/adl_move_swap.hpp>

namespace boost {
namespace movelib {

//Reverses the order of the elements in [first, last), swapping them with adl_move_swap
template<class BidirIt>
void reverse(BidirIt first, BidirIt last)
{
   while (first != last && first != --last) {
      boost::adl_move_iter_swap(first, last);
      ++first;
   }
}

}  //namespace movelib {
}  //namespace boost {

#include <boost/move/detail/config_end.hpp>

#endif   //#define BOOST_MOVE_DETAIL_REVERSE_HPP
