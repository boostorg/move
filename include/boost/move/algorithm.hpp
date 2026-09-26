//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2012-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////

//! \file

#ifndef BOOST_MOVE_ALGORITHM_HPP
#define BOOST_MOVE_ALGORITHM_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/move/detail/config_begin.hpp>

#include <boost/move/utility_core.hpp>
#include <boost/move/iterator.hpp>
#include <boost/move/algo/move.hpp>

#include <boost/move/detail/iterator_traits.hpp>
#include <boost/move/detail/addressof.hpp>
#include <boost/move/detail/type_traits.hpp>
#include <cstring>   //memmove
#include <new>       //placement new

namespace boost {

/// @cond

namespace move_detail {

//Copy and uninitialized copy loops, so that <algorithm> and <memory> are not needed.
//Pointers to trivially copyable types are copied with memmove, as std::copy does.
//(not volatile types: memmove can not be used with them)
template<class T>
struct is_memmove_copy_assignable
{
   static const bool value = is_trivially_copy_assignable<T>::value &&
                             is_same<T, typename remove_cv<T>::type>::value;
};

template<class T>
struct is_memmove_copy_constructible
{
   static const bool value = is_trivially_copy_constructible<T>::value &&
                             is_same<T, typename remove_cv<T>::type>::value;
};

template<class T>
inline T* memmove_range(const T* f, const T* l, T* r)
{
   const std::size_t n = static_cast<std::size_t>(l - f);
   if (n){
      //void pointers: T can be trivially copyable but not trivially assignable
      //(uninitialized copy), which some compilers warn about (-Wclass-memaccess)
      std::memmove(static_cast<void*>(r), static_cast<const void*>(f), n*sizeof(T));
   }
   return r + n;
}

template<class I, class F>
inline F copy_range(I f, I l, F r)
{
   for (; f != l; ++r, ++f){
      *r = *f;
   }
   return r;
}

template<class T>
inline typename enable_if_c<is_memmove_copy_assignable<T>::value, T*>::type
   copy_range(T* f, T* l, T* r)
{  return ::boost::move_detail::memmove_range<T>(f, l, r);  }

template<class T>
inline typename enable_if_c<is_memmove_copy_assignable<T>::value, T*>::type
   copy_range(const T* f, const T* l, T* r)
{  return ::boost::move_detail::memmove_range<T>(f, l, r);  }

template<class I, class F>
inline F uninitialized_copy_range(I f, I l, F r)
{
   typedef typename ::boost::movelib::iterator_traits<F>::value_type value_type;

   F back = r;
   BOOST_MOVE_TRY{
      for (; f != l; ++r, ++f){
         ::new(static_cast<void*>(::boost::move_detail::addressof(*r))) value_type(*f);
      }
   }
   BOOST_MOVE_CATCH(...){
      for (; back != r; ++back){
         ::boost::move_detail::addressof(*back)->~value_type();
      }
      BOOST_MOVE_RETHROW;
   }
   BOOST_MOVE_CATCH_END
   return r;
}

template<class T>
inline typename enable_if_c<is_memmove_copy_constructible<T>::value, T*>::type
   uninitialized_copy_range(T* f, T* l, T* r)
{  return ::boost::move_detail::memmove_range<T>(f, l, r);  }

template<class T>
inline typename enable_if_c<is_memmove_copy_constructible<T>::value, T*>::type
   uninitialized_copy_range(const T* f, const T* l, T* r)
{  return ::boost::move_detail::memmove_range<T>(f, l, r);  }

}  //namespace move_detail {

/// @endcond

//////////////////////////////////////////////////////////////////////////////
//
//                            uninitialized_copy_or_move
//
//////////////////////////////////////////////////////////////////////////////

namespace move_detail {

template
<typename I,   // I models InputIterator
typename F>   // F models ForwardIterator
inline F uninitialized_move_move_iterator(I f, I l, F r
//                             ,typename ::boost::move_detail::enable_if< has_move_emulation_enabled<typename I::value_type> >::type* = 0
)
{
   return ::boost::uninitialized_move(f, l, r);
}
/*
template
<typename I,   // I models InputIterator
typename F>   // F models ForwardIterator
F uninitialized_move_move_iterator(I f, I l, F r,
                                   typename ::boost::move_detail::disable_if< has_move_emulation_enabled<typename I::value_type> >::type* = 0)
{
   return std::uninitialized_copy(f.base(), l.base(), r);
}
*/
}  //namespace move_detail {

template
<typename I,   // I models InputIterator
typename F>   // F models ForwardIterator
inline F uninitialized_copy_or_move(I f, I l, F r,
                             typename ::boost::move_detail::enable_if< move_detail::is_move_iterator<I> >::type* = 0)
{
   return ::boost::move_detail::uninitialized_move_move_iterator(f, l, r);
}

//////////////////////////////////////////////////////////////////////////////
//
//                            copy_or_move
//
//////////////////////////////////////////////////////////////////////////////

namespace move_detail {

template
<typename I,   // I models InputIterator
typename F>   // F models ForwardIterator
inline F move_move_iterator(I f, I l, F r
//                             ,typename ::boost::move_detail::enable_if< has_move_emulation_enabled<typename I::value_type> >::type* = 0
)
{
   return ::boost::move(f, l, r);
}
/*
template
<typename I,   // I models InputIterator
typename F>   // F models ForwardIterator
F move_move_iterator(I f, I l, F r,
                                   typename ::boost::move_detail::disable_if< has_move_emulation_enabled<typename I::value_type> >::type* = 0)
{
   return std::copy(f.base(), l.base(), r);
}
*/

}  //namespace move_detail {

/// @cond

template
<typename I,   // I models InputIterator
typename F>   // F models ForwardIterator
inline F copy_or_move(I f, I l, F r,
                             typename ::boost::move_detail::enable_if< move_detail::is_move_iterator<I> >::type* = 0)
{
   return ::boost::move_detail::move_move_iterator(f, l, r);
}

/// @endcond

//! <b>Effects</b>:
//!   \code
//!   for (; first != last; ++result, ++first)
//!      new (static_cast<void*>(&*result))
//!         typename iterator_traits<ForwardIterator>::value_type(*first);
//!   \endcode
//!
//! <b>Returns</b>: result
//!
//! <b>Note</b>: This function is provided because
//!   <i>std::uninitialized_copy</i> from some STL implementations
//!    is not compatible with <i>move_iterator</i>
template
<typename I,   // I models InputIterator
typename F>   // F models ForwardIterator
inline F uninitialized_copy_or_move(I f, I l, F r
   /// @cond
   ,typename ::boost::move_detail::disable_if< move_detail::is_move_iterator<I> >::type* = 0
   /// @endcond
   )
{
   return ::boost::move_detail::uninitialized_copy_range(f, l, r);
}

//! <b>Effects</b>:
//!   \code
//!   for (; first != last; ++result, ++first)
//!      *result = *first;
//!   \endcode
//!
//! <b>Returns</b>: result
//!
//! <b>Note</b>: This function is provided because
//!   <i>std::uninitialized_copy</i> from some STL implementations
//!    is not compatible with <i>move_iterator</i>
template
<typename I,   // I models InputIterator
typename F>   // F models ForwardIterator
inline F copy_or_move(I f, I l, F r
   /// @cond
   ,typename ::boost::move_detail::disable_if< move_detail::is_move_iterator<I> >::type* = 0
   /// @endcond
   )
{
   return ::boost::move_detail::copy_range(f, l, r);
}

}  //namespace boost {

#include <boost/move/detail/config_end.hpp>

#endif //#ifndef BOOST_MOVE_ALGORITHM_HPP
