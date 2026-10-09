//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2017-2026.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#ifndef BOOST_MOVE_DETAIL_DUO_HPP
#define BOOST_MOVE_DETAIL_DUO_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif

#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

namespace boost {
namespace move_detail {

//A simple pair-like class to avoid including <utility>
template<class T1, class T2>
struct duo
{
   duo()
      : first(), second()
   {}

   duo(const T1 &t1, const T2 &t2)
      : first(t1), second(t2)
   {}

   T1 first;
   T2 second;
};

}  //namespace move_detail {
}  //namespace boost {

#endif   //#ifndef BOOST_MOVE_DETAIL_DUO_HPP
