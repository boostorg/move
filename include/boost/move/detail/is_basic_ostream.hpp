//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2026-2026. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#ifndef BOOST_MOVE_DETAIL_IS_BASIC_OSTREAM_HPP
#define BOOST_MOVE_DETAIL_IS_BASIC_OSTREAM_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif

#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

//is_basic_ostream<S> and is_basic_istream<S> detect if S is std::basic_ostream (std::basic_istream)
//or a class derived from it, without including <iosfwd>. Stream operators can then take the stream
//type as a template parameter, constrained with these traits, instead of std::basic_ostream<C, T>.
//
//A class derived from std::basic_ostream inherits its injected-class-name, so S::basic_ostream
//names the std::basic_ostream base (whatever the std namespace is, e.g. std::__1 in libc++).
//In a nested-name-specifier only types are found, so S::basic_ostream::char_type is also valid
//if S is std::basic_ostream itself.
//
//The traits use a sizeof test, because some compilers (e.g. MSVC 9.0 and 10.0) do not remove
//a function from overload resolution if the nested type is used in its signature.

namespace boost {
namespace move_detail {

template<class S>
struct is_basic_ostream
{
   template<class U> static char test(typename U::basic_ostream::char_type*);
   template<class U> static int  test(...);
   static const bool value = sizeof(test<S>(0)) == sizeof(char);
};

template<class S>
struct is_basic_istream
{
   template<class U> static char test(typename U::basic_istream::char_type*);
   template<class U> static int  test(...);
   static const bool value = sizeof(test<S>(0)) == sizeof(char);
};

}  //namespace move_detail {
}  //namespace boost {

#endif   //#ifndef BOOST_MOVE_DETAIL_IS_BASIC_OSTREAM_HPP
