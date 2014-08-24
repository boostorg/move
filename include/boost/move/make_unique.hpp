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

#include <boost/move/detail/config_begin.hpp>
#include <boost/move/detail/workaround.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/move/unique_ptr.hpp>
#include <cstddef>   //for std::size_t
#include <boost/move/detail/meta_utils.hpp>

#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
#include <boost/preprocessor/iteration/local.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/cat.hpp>
#endif

//!\file
//! Defines "make_unique" functions, which are factories to create instances
//! of unique_ptr depending on the passed arguments.
//!
//! This header can be a bit heavyweight in C++03 compilers due to the use of the
//! preprocessor library, that's why it's a a separate header from <tt>unique_ptr.hpp</tt>
 
namespace boost{

#if !defined(BOOST_MOVE_DOXYGEN_INVOKED)

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

#endif   //!defined(BOOST_MOVE_DOXYGEN_INVOKED)

namespace movelib {

#if defined(BOOST_MOVE_DOXYGEN_INVOKED) || !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

//! <b>Remarks</b>: This function shall not participate in overload resolution unless T is not an array.
//!
//! <b>Returns</b>: <tt>unique_ptr<T>(new T(std::forward<Args>(args)...))</tt>.
template<class T, class... Args>
inline
   #if defined(BOOST_MOVE_DOXYGEN_INVOKED)
   unique_ptr<T>
   #else
   typename ::boost::move_detail::unique_ptr_if<T>::t_is_not_array
   #endif
   make_unique(BOOST_FWD_REF(Args)... args)
{  return unique_ptr<T>(new T(::boost::forward<Args>(args)...));  }

#else
   #ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
      #define BOOST_MOVE_PP_PARAM_LIST(z, n, data) \
      BOOST_PP_CAT(P, n) && BOOST_PP_CAT(p, n) \
      //!
   #else
      #define BOOST_MOVE_PP_PARAM_LIST(z, n, data) \
      const BOOST_PP_CAT(P, n) & BOOST_PP_CAT(p, n) \
      //!
   #endif   //#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

   #define BOOST_MOVE_PP_PARAM_FORWARD(z, n, data) \
   ::boost::forward< BOOST_PP_CAT(P, n) >( BOOST_PP_CAT(p, n) ) \
   //!

   #define BOOST_MOVE_MAX_CONSTRUCTOR_PARAMETERS 10

   #define BOOST_PP_LOCAL_MACRO(n) \
   template<class T BOOST_PP_ENUM_TRAILING_PARAMS(n, class P) > \
   typename ::boost::move_detail::unique_ptr_if<T>::t_is_not_array \
      make_unique(BOOST_PP_ENUM(n, BOOST_MOVE_PP_PARAM_LIST, _)) \
   {  return unique_ptr<T>(new T(BOOST_PP_ENUM(n, BOOST_MOVE_PP_PARAM_FORWARD, _)));  } \
   //!

   #define BOOST_PP_LOCAL_LIMITS (0, BOOST_MOVE_MAX_CONSTRUCTOR_PARAMETERS)
   #include BOOST_PP_LOCAL_ITERATE()

#endif


//! <b>Remarks</b>: This function shall not participate in overload resolution unless T is an array of 
//!   unknown bound.
//!
//! <b>Returns</b>: <tt>unique_ptr<T>(new remove_extent_t<T>[n]())</tt>.
template<class T>
inline
   #if defined(BOOST_MOVE_DOXYGEN_INVOKED)
   unique_ptr<T>
   #else
   typename ::boost::move_detail::unique_ptr_if<T>::t_is_array_of_unknown_bound
   #endif
   make_unique(std::size_t n)
{
    typedef typename ::boost::move_detail::remove_extent<T>::type U;
    return unique_ptr<T>(new U[n]());
}

#if defined(BOOST_MOVE_DOXYGEN_INVOKED) || !defined(BOOST_NO_CXX11_DELETED_FUNCTIONS)

//! <b>Remarks</b>: This function shall not participate in overload resolution unless T is
//!   an array of known bound.
template<class T, class... Args>
   #if defined(BOOST_MOVE_DOXYGEN_INVOKED)
   unspecified
   #else
   typename ::boost::move_detail::unique_ptr_if<T>::t_is_array_of_known_bound 
   #endif
   make_unique(BOOST_FWD_REF(Args) ...) = delete;
#endif

}  //namespace movelib {

}  //namespace boost{

#include <boost/move/detail/config_end.hpp>

#endif   //#ifndef BOOST_MOVE_MAKE_UNIQUE_HPP_INCLUDED
