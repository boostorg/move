//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2014. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_MOVE_MAKE_UNIQUE_HPP_INCLUDED
#define BOOST_MOVE_MAKE_UNIQUE_HPP_INCLUDED

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
#include <boost/move/unique_ptr.hpp>
#include <cstddef>   //for std::size_t
#include <boost/move/detail/type_traits.hpp>
#include <boost/move/detail/std_new_fwd.hpp>   //std::nothrow, without including <new>
#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
#  include <boost/move/detail/fwd_macros.hpp>
#endif

//!\file
//! Defines "make_unique" functions, which are factories to create instances
//! of unique_ptr depending on the passed arguments.
//!
//! This header can be a bit heavyweight in C++03 compilers due to the use of the
//! preprocessor library, that's why it's a a separate header from <tt>unique_ptr.hpp</tt>
 
#if !defined(BOOST_MOVE_DOXYGEN_INVOKED)

namespace boost{
namespace move_detail {

//Compile time switch between
//single element, unknown bound array
//and known bound array
template<class T>
struct unique_ptr_if
{
   typedef ::boost::movelib::unique_ptr<T> t_is_not_array;
};

template<class T>
struct unique_ptr_if<T[]>
{
   typedef ::boost::movelib::unique_ptr<T[]> t_is_array_of_unknown_bound;
};

template<class T, std::size_t N>
struct unique_ptr_if<T[N]>
{
   typedef void t_is_array_of_known_bound;
};

}  //namespace move_detail {
}  //namespace boost{

#endif   //!defined(BOOST_MOVE_DOXYGEN_INVOKED)

namespace boost{
namespace movelib {

#if defined(BOOST_MOVE_DOXYGEN_INVOKED) || !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

//! <b>Remarks</b>: This function shall not participate in overload resolution unless T is not an array.
//!
//! <b>Returns</b>: <tt>unique_ptr&lt;T&gt;(new T(std::forward&lt;Args&gt;(args)...))</tt>.
template<class T, class... Args>
inline BOOST_MOVE_CXX20_CONSTEXPR BOOST_MOVE_DOC1ST(unique_ptr<T>, 
   typename ::boost::move_detail::unique_ptr_if<T>::t_is_not_array)
      make_unique(BOOST_FWD_REF(Args)... args)
{  return unique_ptr<T>(new T(::boost::forward<Args>(args)...));  }

//! <b>Remarks</b>: This function shall not participate in overload resolution unless T is not an array.
//!
//! <b>Returns</b>: <tt>unique_ptr&lt;T&gt;(new T(std::nothrow)(std::forward&lt;Args&gt;(args)...))</tt>.
template<class T, class... Args>
inline BOOST_MOVE_DOC1ST(unique_ptr<T>, 
   typename ::boost::move_detail::unique_ptr_if<T>::t_is_not_array)
      make_unique_nothrow(BOOST_FWD_REF(Args)... args)
{  return unique_ptr<T>(new (std::nothrow)T(::boost::forward<Args>(args)...));  }

#else
   #define BOOST_MOVE_MAKE_UNIQUE_CODE(N)\
      template<class T BOOST_MOVE_I##N BOOST_MOVE_CLASS##N>\
      typename ::boost::move_detail::unique_ptr_if<T>::t_is_not_array\
         make_unique( BOOST_MOVE_UREF##N)\
      {  return unique_ptr<T>( new T( BOOST_MOVE_FWD##N ) );  }\
      \
      template<class T BOOST_MOVE_I##N BOOST_MOVE_CLASS##N>\
      typename ::boost::move_detail::unique_ptr_if<T>::t_is_not_array\
         make_unique_nothrow( BOOST_MOVE_UREF##N)\
      {  return unique_ptr<T>( new (std::nothrow)T ( BOOST_MOVE_FWD##N ) );  }\
      //
   BOOST_MOVE_ITERATE_0TO9(BOOST_MOVE_MAKE_UNIQUE_CODE)
   #undef BOOST_MOVE_MAKE_UNIQUE_CODE

#endif

//! <b>Remarks</b>: This function shall not participate in overload resolution unless T is not an array.
//!
//! <b>Returns</b>: <tt>unique_ptr&lt;T&gt;(new T)</tt> (default initialization)
template<class T>
inline BOOST_MOVE_CXX20_CONSTEXPR BOOST_MOVE_DOC1ST(unique_ptr<T>, 
   typename ::boost::move_detail::unique_ptr_if<T>::t_is_not_array)
      make_unique_definit()
{
    return unique_ptr<T>(new T);
}

//! <b>Remarks</b>: This function shall not participate in overload resolution unless T is not an array.
//!
//! <b>Returns</b>: <tt>unique_ptr&lt;T&gt;(new T(std::nothrow)</tt> (default initialization)
template<class T>
inline BOOST_MOVE_DOC1ST(unique_ptr<T>, 
   typename ::boost::move_detail::unique_ptr_if<T>::t_is_not_array)
      make_unique_nothrow_definit()
{
    return unique_ptr<T>(new (std::nothrow)T);
}

//! <b>Remarks</b>: This function shall not participate in overload resolution unless T is not an array.
//!
//! <b>Returns</b>: <tt>unique_ptr&lt;T&gt;(new T)</tt> (default initialization). Same as
//!   <tt>make_unique_definit&lt;T&gt;()</tt> and C++20's <tt>std::make_unique_for_overwrite&lt;T&gt;()</tt>.
template<class T>
inline BOOST_MOVE_CXX20_CONSTEXPR BOOST_MOVE_DOC1ST(unique_ptr<T>,
   typename ::boost::move_detail::unique_ptr_if<T>::t_is_not_array)
      make_unique_for_overwrite()
{
    return unique_ptr<T>(new T);
}

//! <b>Remarks</b>: This function shall not participate in overload resolution unless T is an array of 
//!   unknown bound.
//!
//! <b>Returns</b>: <tt>unique_ptr&lt;T&gt;(new remove_extent_t&lt;T&gt;[n]())</tt> (value initialization)
template<class T>
inline BOOST_MOVE_CXX20_CONSTEXPR_ARRAY_VINIT BOOST_MOVE_DOC1ST(unique_ptr<T>,
   typename ::boost::move_detail::unique_ptr_if<T>::t_is_array_of_unknown_bound)
      make_unique(std::size_t n)
{
    typedef typename ::boost::move_detail::remove_extent<T>::type U;
    return unique_ptr<T>(new U[n]());
}

//! <b>Remarks</b>: This function shall not participate in overload resolution unless T is an array of 
//!   unknown bound.
//!
//! <b>Returns</b>: <tt>unique_ptr&lt;T&gt;(new (std::nothrow)remove_extent_t&lt;T&gt;[n]())</tt> (value initialization)
template<class T>
inline BOOST_MOVE_DOC1ST(unique_ptr<T>, 
   typename ::boost::move_detail::unique_ptr_if<T>::t_is_array_of_unknown_bound)
      make_unique_nothrow(std::size_t n)
{
    typedef typename ::boost::move_detail::remove_extent<T>::type U;
    return unique_ptr<T>(new (std::nothrow)U[n]());
}

//! <b>Remarks</b>: This function shall not participate in overload resolution unless T is an array of 
//!   unknown bound.
//!
//! <b>Returns</b>: <tt>unique_ptr&lt;T&gt;(new remove_extent_t&lt;T&gt;[n])</tt> (default initialization)
template<class T>
inline BOOST_MOVE_CXX20_CONSTEXPR BOOST_MOVE_DOC1ST(unique_ptr<T>, 
   typename ::boost::move_detail::unique_ptr_if<T>::t_is_array_of_unknown_bound)
      make_unique_definit(std::size_t n)
{
    typedef typename ::boost::move_detail::remove_extent<T>::type U;
    return unique_ptr<T>(new U[n]);
}

//! <b>Remarks</b>: This function shall not participate in overload resolution unless T is an array of 
//!   unknown bound.
//!
//! <b>Returns</b>: <tt>unique_ptr&lt;T&gt;(new (std::nothrow)remove_extent_t&lt;T&gt;[n])</tt> (default initialization)
template<class T>
inline BOOST_MOVE_DOC1ST(unique_ptr<T>, 
   typename ::boost::move_detail::unique_ptr_if<T>::t_is_array_of_unknown_bound)
      make_unique_nothrow_definit(std::size_t n)
{
    typedef typename ::boost::move_detail::remove_extent<T>::type U;
    return unique_ptr<T>(new (std::nothrow) U[n]);
}

//! <b>Remarks</b>: This function shall not participate in overload resolution unless T is an array of
//!   unknown bound.
//!
//! <b>Returns</b>: <tt>unique_ptr&lt;T&gt;(new remove_extent_t&lt;T&gt;[n])</tt> (default initialization). Same as
//!   <tt>make_unique_definit&lt;T&gt;(n)</tt> and C++20's <tt>std::make_unique_for_overwrite&lt;T&gt;(n)</tt>.
template<class T>
inline BOOST_MOVE_CXX20_CONSTEXPR BOOST_MOVE_DOC1ST(unique_ptr<T>,
   typename ::boost::move_detail::unique_ptr_if<T>::t_is_array_of_unknown_bound)
      make_unique_for_overwrite(std::size_t n)
{
    typedef typename ::boost::move_detail::remove_extent<T>::type U;
    return unique_ptr<T>(new U[n]);
}

#if !defined(BOOST_NO_CXX11_DELETED_FUNCTIONS)

//! <b>Remarks</b>: This function shall not participate in overload resolution unless T is
//!   an array of known bound.
template<class T, class... Args>
inline BOOST_MOVE_DOC1ST(unspecified, 
   typename ::boost::move_detail::unique_ptr_if<T>::t_is_array_of_known_bound)
      make_unique(BOOST_FWD_REF(Args) ...) = delete;

//! <b>Remarks</b>: This function shall not participate in overload resolution unless T is
//!   an array of known bound.
template<class T, class... Args>
inline BOOST_MOVE_DOC1ST(unspecified, 
   typename ::boost::move_detail::unique_ptr_if<T>::t_is_array_of_known_bound)
      make_unique_definit(BOOST_FWD_REF(Args) ...) = delete;

//! <b>Remarks</b>: This function shall not participate in overload resolution unless T is
//!   an array of known bound.
template<class T, class... Args>
inline BOOST_MOVE_DOC1ST(unspecified,
   typename ::boost::move_detail::unique_ptr_if<T>::t_is_array_of_known_bound)
      make_unique_for_overwrite(BOOST_FWD_REF(Args) ...) = delete;

//! <b>Remarks</b>: This function shall not participate in overload resolution unless T is
//!   an array of known bound.
template<class T, class... Args>
inline BOOST_MOVE_DOC1ST(unspecified, 
   typename ::boost::move_detail::unique_ptr_if<T>::t_is_array_of_known_bound)
      make_unique_nothrow(BOOST_FWD_REF(Args) ...) = delete;

//! <b>Remarks</b>: This function shall not participate in overload resolution unless T is
//!   an array of known bound.
template<class T, class... Args>
inline BOOST_MOVE_DOC1ST(unspecified, 
   typename ::boost::move_detail::unique_ptr_if<T>::t_is_array_of_known_bound)
      make_unique_nothrow_definit(BOOST_FWD_REF(Args) ...) = delete;

#endif

}  //namespace movelib {

}  //namespace boost{

#include <boost/move/detail/config_end.hpp>

#endif   //#ifndef BOOST_MOVE_MAKE_UNIQUE_HPP_INCLUDED
