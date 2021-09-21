//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2014-2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////

//! \file

#ifndef BOOST_MOVE_DETAIL_ITERATOR_TRAITS_HPP
#define BOOST_MOVE_DETAIL_ITERATOR_TRAITS_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <cstddef>
#include <iterator>
#include <boost/move/detail/type_traits.hpp>

#include <boost/move/detail/std_ns_begin.hpp>
BOOST_MOVE_STD_NS_BEG

struct input_iterator_tag;
struct forward_iterator_tag;
struct bidirectional_iterator_tag;
struct random_access_iterator_tag;
struct output_iterator_tag;
struct contiguous_iterator_tag;

BOOST_MOVE_STD_NS_END
#include <boost/move/detail/std_ns_end.hpp>

namespace boost{  namespace movelib{

template <typename T>
struct iterator_traits : std::iterator_traits<T>
{
   typedef typename std::iterator_traits<T>::difference_type difference_type;
   typedef typename std::iterator_traits<T>::pointer pointer;
   typedef typename std::iterator_traits<T>::reference reference;
   typedef typename std::iterator_traits<T>::iterator_category iterator_category;
   typedef typename std::iterator_traits<T>::value_type value_type;

   typedef typename boost::move_detail::make_unsigned<difference_type>::type size_type;
};

}} //namespace boost {  namespace movelib{

#endif //#ifndef BOOST_MOVE_DETAIL_ITERATOR_TRAITS_HPP
