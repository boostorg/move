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
//! This header defines core utilities to ease the development
//! of move-aware functions. This header minimizes dependencies
//! from other libraries.

#ifndef BOOST_MOVE_MOVE_UTILITY_CORE_HPP
#define BOOST_MOVE_MOVE_UTILITY_CORE_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/move/detail/config_begin.hpp>
#include <boost/move/detail/workaround.hpp>  //forceinline
#include <boost/move/core.hpp>
#include <boost/move/detail/meta_utils.hpp>

#if !defined(BOOST_MOVE_DOXYGEN_INVOKED)

namespace boost{
namespace move_detail{

//forward_like: U with the const qualification of T added ("merge" model of std::forward_like)
template<class T, class U>
struct forward_like_merge_const
{  typedef U type;   };

template<class T, class U>
struct forward_like_merge_const<const T, U>
{  typedef const U type;   };

}  //namespace move_detail{
}  //namespace boost{

#endif   //#if !defined(BOOST_MOVE_DOXYGEN_INVOKED)

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && !defined(BOOST_MOVE_DOXYGEN_INVOKED)

   namespace boost {

   template<class T>
   struct enable_move_utility_emulation
   {
      static const bool value = true;
   };
    
   //////////////////////////////////////////////////////////////////////////////
   //
   //                            move()
   //
   //////////////////////////////////////////////////////////////////////////////

   template <class T>
   BOOST_MOVE_FORCEINLINE typename ::boost::move_detail::enable_if_and
      < T &
      , enable_move_utility_emulation<T>
      , has_move_emulation_disabled<T>
      >::type
         move(T& x) BOOST_NOEXCEPT
   {
      return x;
   }

   template <class T>
   BOOST_MOVE_FORCEINLINE typename ::boost::move_detail::enable_if_and
      < rv<T>&
      , enable_move_utility_emulation<T>
      , has_move_emulation_enabled<T>
      >::type
         move(T& x) BOOST_NOEXCEPT
   {
      return *BOOST_MOVE_TO_RV_CAST(::boost::rv<T>*, ::boost::move_detail::addressof(x) );
   }

   template <class T>
   BOOST_MOVE_FORCEINLINE typename ::boost::move_detail::enable_if_and
      < rv<T>&
      , enable_move_utility_emulation<T>
      , has_move_emulation_enabled<T>
      >::type
         move(rv<T>& x) BOOST_NOEXCEPT
   {
      return x;
   }

   //////////////////////////////////////////////////////////////////////////////
   //
   //                            forward()
   //
   //////////////////////////////////////////////////////////////////////////////

   template <class T>
   BOOST_MOVE_FORCEINLINE typename ::boost::move_detail::enable_if_and
      < T &
      , enable_move_utility_emulation<T>
      , ::boost::move_detail::is_rv<T>
      >::type
         forward(const typename ::boost::move_detail::identity<T>::type &x) BOOST_NOEXCEPT
   {
      return const_cast<T&>(x);
   }

   template <class T>
   BOOST_MOVE_FORCEINLINE typename ::boost::move_detail::enable_if_and
      < const T &
      , enable_move_utility_emulation<T>
      , ::boost::move_detail::is_not_rv<T>
      >::type
         forward(const typename ::boost::move_detail::identity<T>::type &x) BOOST_NOEXCEPT
   {
      return x;
   }

   //////////////////////////////////////////////////////////////////////////////
   //
   //                        move_if_not_lvalue_reference()
   //
   //////////////////////////////////////////////////////////////////////////////

   template <class T>
   BOOST_MOVE_FORCEINLINE typename ::boost::move_detail::enable_if_and
      < T &
      , enable_move_utility_emulation<T>
      , ::boost::move_detail::is_rv<T>
      >::type
         move_if_not_lvalue_reference(const typename ::boost::move_detail::identity<T>::type &x) BOOST_NOEXCEPT
   {
      return const_cast<T&>(x);
   }

   template <class T>
   BOOST_MOVE_FORCEINLINE typename ::boost::move_detail::enable_if_and
      < typename ::boost::move_detail::add_lvalue_reference<T>::type
      , enable_move_utility_emulation<T>
      , ::boost::move_detail::is_not_rv<T>
      , ::boost::move_detail::or_
         < ::boost::move_detail::is_lvalue_reference<T>
         , has_move_emulation_disabled<T>
         >
      >::type
         move_if_not_lvalue_reference(typename ::boost::move_detail::remove_reference<T>::type &x) BOOST_NOEXCEPT
   {
      return x;
   }

   template <class T>
   BOOST_MOVE_FORCEINLINE typename ::boost::move_detail::enable_if_and
      < rv<T>&
      , enable_move_utility_emulation<T>
      , ::boost::move_detail::is_not_rv<T>
      , ::boost::move_detail::and_
         < ::boost::move_detail::not_< ::boost::move_detail::is_lvalue_reference<T> >
         , has_move_emulation_enabled<T>
         >
      >::type
         move_if_not_lvalue_reference(typename ::boost::move_detail::remove_reference<T>::type &x) BOOST_NOEXCEPT
   {
      return ::boost::move(x);
   }

   //////////////////////////////////////////////////////////////////////////////
   //
   //                            forward_like()
   //
   //////////////////////////////////////////////////////////////////////////////

   namespace move_detail {

   //T is ::boost::rv<P> or a reference to it
   template<class T>
   struct forward_like_moves
   {
      static const bool value = is_rv<T>::value || is_rvalue_reference<T>::value;
   };

   }  //namespace move_detail {

   //T is ::boost::rv<P> (as deduced by BOOST_FWD_REF(T) for a moved argument): move x.
   //BOOST_FWD_REF binds the argument to a const reference, so x is usually const: as
   //boost::forward, remove the const qualification.
   template <class T, class U>
   BOOST_MOVE_FORCEINLINE typename ::boost::move_detail::enable_if_c
      < ::boost::move_detail::forward_like_moves<T>::value
      , typename ::boost::move_detail::if_c
         < ::boost::has_move_emulation_enabled<U>::value, ::boost::rv<U>&, U& >::type
      >::type
         forward_like(const U &x) BOOST_NOEXCEPT
   {
      return ::boost::move(const_cast<U&>(x));
   }

   //T is an lvalue reference: x as an lvalue, const if T refers to a const type
   template <class T, class U>
   BOOST_MOVE_FORCEINLINE typename ::boost::move_detail::enable_if_c
      < !::boost::move_detail::forward_like_moves<T>::value &&
        ::boost::move_detail::is_lvalue_reference<T>::value
      , typename ::boost::move_detail::forward_like_merge_const
         <typename ::boost::move_detail::remove_reference<T>::type, U>::type &
      >::type
         forward_like(U &x) BOOST_NOEXCEPT
   {
      return x;
   }

   //T is not a reference and not ::boost::rv<P> (as deduced by BOOST_FWD_REF(T) for
   //an lvalue argument, which is bound to a const reference): x as a const lvalue
   template <class T, class U>
   BOOST_MOVE_FORCEINLINE typename ::boost::move_detail::enable_if_c
      < !::boost::move_detail::forward_like_moves<T>::value &&
        !::boost::move_detail::is_lvalue_reference<T>::value
      , const U &
      >::type
         forward_like(U &x) BOOST_NOEXCEPT
   {
      return x;
   }

   }  //namespace boost

#else    //#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && !defined(BOOST_MOVE_DOXYGEN_INVOKED)

   #if defined(BOOST_MOVE_USE_STANDARD_LIBRARY_MOVE)
      #include <utility>

      namespace boost{

      using ::std::move;
      using ::std::forward;

      }  //namespace boost

   #else //!BOOST_MOVE_USE_STANDARD_LIBRARY_MOVE

      namespace boost {

      //! This trait's internal boolean `value` is false in compilers with rvalue references
      //! and true in compilers without rvalue references.
      //!
      //! A user can specialize this trait for a type T to false to SFINAE out `move` and `forward`
      //! so that the user can define a different move emulation for that type in namespace boost
      //! (e.g. another Boost library for its types) and avoid any overload ambiguity.
      template<class T>
      struct enable_move_utility_emulation
      {
         static const bool value = false;
      };

      //////////////////////////////////////////////////////////////////////////////
      //
      //                                  move
      //
      //////////////////////////////////////////////////////////////////////////////

      #if defined(BOOST_MOVE_DOXYGEN_INVOKED)
         //! This function provides a way to convert a reference into a rvalue reference
         //! in compilers with rvalue references. For other compilers if `T` is Boost.Move
         //! enabled type then it converts `T&` into <tt>::boost::rv&lt;T&gt; &</tt> so that
         //! move emulation is activated, else it returns `T &`.
         template <class T>
         rvalue_reference move(input_reference) noexcept;

      #elif defined(BOOST_MOVE_OLD_RVALUE_REF_BINDING_RULES)

         //Old move approach, lvalues could bind to rvalue references
         template <class T>
         BOOST_MOVE_FORCEINLINE typename ::boost::move_detail::remove_reference<T>::type && move(T&& t) BOOST_NOEXCEPT
         {  return t;   }

      #else //BOOST_MOVE_OLD_RVALUE_REF_BINDING_RULES

         template <class T>
         BOOST_MOVE_INTRINSIC_CAST BOOST_MOVE_CXX20_CONSTEXPR
         typename ::boost::move_detail::remove_reference<T>::type && move(T&& t) BOOST_NOEXCEPT
         { return static_cast<typename ::boost::move_detail::remove_reference<T>::type &&>(t); }

      #endif   //BOOST_MOVE_OLD_RVALUE_REF_BINDING_RULES

      //////////////////////////////////////////////////////////////////////////////
      //
      //                                  forward
      //
      //////////////////////////////////////////////////////////////////////////////


      #if defined(BOOST_MOVE_DOXYGEN_INVOKED)
         //! This function provides limited form of forwarding that is usually enough for
         //! in-place construction and avoids the exponential overloading for
         //! achieve the limited forwarding in C++03.
         //!
         //! For compilers with rvalue references this function provides perfect forwarding.
         //!
         //! Otherwise:
         //! * If input_reference binds to const ::boost::rv&lt;T&gt; & then it output_reference is
         //!   ::boost::rv&lt;T&gt; &
         //!
         //! * Else, output_reference is equal to input_reference.
         template <class T> output_reference forward(input_reference) noexcept;
      #elif defined(BOOST_MOVE_OLD_RVALUE_REF_BINDING_RULES)

         //Old move approach, lvalues could bind to rvalue references

         template <class T>
         BOOST_MOVE_FORCEINLINE T&& forward(typename ::boost::move_detail::identity<T>::type&& t) BOOST_NOEXCEPT
         {  return t;   }

      #else //Old move

         template <class T>
         BOOST_MOVE_INTRINSIC_CAST BOOST_MOVE_CXX20_CONSTEXPR
         T&& forward(typename ::boost::move_detail::remove_reference<T>::type& t) BOOST_NOEXCEPT
         {  return static_cast<T&&>(t);   }

         template <class T>
         BOOST_MOVE_INTRINSIC_CAST BOOST_MOVE_CXX20_CONSTEXPR
         T&& forward(typename ::boost::move_detail::remove_reference<T>::type&& t) BOOST_NOEXCEPT
         {
            //"boost::forward<T> error: 'T' is a lvalue reference, can't forward as rvalue.";
            BOOST_MOVE_STATIC_ASSERT(!boost::move_detail::is_lvalue_reference<T>::value);
            return static_cast<T&&>(t);
         }

      #endif   //BOOST_MOVE_DOXYGEN_INVOKED

      }  //namespace boost {

   #endif   //BOOST_MOVE_USE_STANDARD_LIBRARY_MOVE

   //////////////////////////////////////////////////////////////////////////////
   //
   //                         move_if_not_lvalue_reference
   //
   //////////////////////////////////////////////////////////////////////////////

   namespace boost {

   #if defined(BOOST_MOVE_DOXYGEN_INVOKED)
      //! <b>Effects</b>: Calls `boost::move` if `input_reference` is not a lvalue reference.
      //!   Otherwise returns the reference
      template <class T> output_reference move_if_not_lvalue_reference(input_reference) noexcept;
   #elif defined(BOOST_MOVE_OLD_RVALUE_REF_BINDING_RULES)

      //Old move approach, lvalues could bind to rvalue references

      template <class T>
      BOOST_MOVE_FORCEINLINE T&& move_if_not_lvalue_reference(typename ::boost::move_detail::identity<T>::type&& t) BOOST_NOEXCEPT
      {  return t;   }

   #else //Old move

      template <class T>
      BOOST_MOVE_INTRINSIC_CAST BOOST_MOVE_CXX20_CONSTEXPR T&& move_if_not_lvalue_reference(typename ::boost::move_detail::remove_reference<T>::type& t) BOOST_NOEXCEPT
      {  return static_cast<T&&>(t);   }

      template <class T>
      BOOST_MOVE_INTRINSIC_CAST BOOST_MOVE_CXX20_CONSTEXPR T&& move_if_not_lvalue_reference(typename ::boost::move_detail::remove_reference<T>::type&& t) BOOST_NOEXCEPT
      {
         //"boost::forward<T> error: 'T' is a lvalue reference, can't forward as rvalue.";
         BOOST_MOVE_STATIC_ASSERT(!boost::move_detail::is_lvalue_reference<T>::value);
         return static_cast<T&&>(t);
      }

   #endif   //BOOST_MOVE_DOXYGEN_INVOKED

   //////////////////////////////////////////////////////////////////////////////
   //
   //                            forward_like
   //
   //////////////////////////////////////////////////////////////////////////////

   #if defined(BOOST_MOVE_DOXYGEN_INVOKED)
      //! Forwards x (usually a member or a base of an object of type T) with the value category
      //! of T, as C++23 <code>std::forward_like</code>: the result is an lvalue if T is an lvalue
      //! reference, otherwise an rvalue. The result is const if x is const or if
      //! <code>remove_reference<T>::type</code> is const.
      //!
      //! Typical use, where T is the template parameter of a forwarding reference:
      //! <code>template<class T> void f(BOOST_FWD_REF(T) t) { g(boost::forward_like<T>(t.member)); }</code>
      //!
      //! <b>Note</b>: as <code>std::forward_like</code>, the category of the object x refers to
      //! is changed: if x is a reference member, the object it refers to is forwarded as an rvalue.
      template <class T> output_reference forward_like(input_reference) noexcept;
   #else

      namespace move_detail {

      template<class T, class U>
      struct forward_like_type
      {
         typedef typename forward_like_merge_const
            < typename remove_reference<T>::type
            , typename remove_reference<U>::type>::type  cv_type;
         typedef typename if_c< is_lvalue_reference<T>::value, cv_type &, cv_type && >::type type;
      };

      }  //namespace move_detail {

      template <class T, class U>
      BOOST_MOVE_INTRINSIC_CAST BOOST_MOVE_CXX20_CONSTEXPR
      typename ::boost::move_detail::forward_like_type<T, U>::type forward_like(U &&x) BOOST_NOEXCEPT
      {  return static_cast<typename ::boost::move_detail::forward_like_type<T, U>::type>(x);  }

   #endif   //BOOST_MOVE_DOXYGEN_INVOKED

   }  //namespace boost {

#endif   //BOOST_NO_CXX11_RVALUE_REFERENCES

#if !defined(BOOST_MOVE_DOXYGEN_INVOKED)

namespace boost{
namespace move_detail{

template <typename T>
typename boost::move_detail::add_rvalue_reference<T>::type declval() BOOST_NOEXCEPT;

}  //namespace move_detail{
}  //namespace boost{

#endif   //#if !defined(BOOST_MOVE_DOXYGEN_INVOKED)


#include <boost/move/detail/config_end.hpp>

#endif //#ifndef BOOST_MOVE_MOVE_UTILITY_CORE_HPP
