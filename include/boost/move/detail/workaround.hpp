//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2014-2014. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_MOVE_DETAIL_WORKAROUND_HPP
#define BOOST_MOVE_DETAIL_WORKAROUND_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#if    !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
   #define BOOST_MOVE_PERFECT_FORWARDING
#endif

#if defined(__has_feature)
   #define BOOST_MOVE_HAS_FEATURE __has_feature
#else
   #define BOOST_MOVE_HAS_FEATURE(x) 0
#endif

#if BOOST_MOVE_HAS_FEATURE(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
   #define BOOST_MOVE_ADDRESS_SANITIZER_ON
#endif

//Macros for documentation purposes. For code, expands to the argument
#define BOOST_MOVE_IMPDEF(TYPE) TYPE
#define BOOST_MOVE_SEEDOC(TYPE) TYPE
#define BOOST_MOVE_DOC0PTR(TYPE) TYPE
#define BOOST_MOVE_DOC1ST(TYPE1, TYPE2) TYPE2
#define BOOST_MOVE_I ,
#define BOOST_MOVE_DOCIGN(T1) T1

#if defined(__GNUC__) && (__GNUC__ == 4) && (__GNUC_MINOR__ < 5) && !defined(__clang__)
   //Pre-standard rvalue binding rules
   #define BOOST_MOVE_OLD_RVALUE_REF_BINDING_RULES
#elif defined(_MSC_VER) && (_MSC_VER == 1600)
   //Standard rvalue binding rules but with some bugs
   #define BOOST_MOVE_MSVC_10_MEMBER_RVALUE_REF_BUG
   #define BOOST_MOVE_MSVC_AUTO_MOVE_RETURN_BUG
#elif defined(_MSC_VER) && (_MSC_VER == 1700)
   #define BOOST_MOVE_MSVC_AUTO_MOVE_RETURN_BUG
#endif

//#define BOOST_MOVE_DISABLE_FORCEINLINE

#if defined(BOOST_MOVE_DISABLE_FORCEINLINE)
   #define BOOST_MOVE_FORCEINLINE inline
#elif defined(BOOST_MOVE_FORCEINLINE_IS_BOOST_FORCELINE)
   #define BOOST_MOVE_FORCEINLINE BOOST_FORCEINLINE
#elif defined(BOOST_MSVC) && (_MSC_VER < 1900 || defined(_DEBUG))
   //"__forceinline" and MSVC seems to have some bugs in old versions and in debug mode
   #define BOOST_MOVE_FORCEINLINE inline
#elif defined(BOOST_GCC) && (__GNUC__ <= 5)
   //Older GCCs have problems with forceinline
   //Clang can have code bloat issues with forceinline, see
   //https://lists.boost.org/boost-users/2023/04/91445.php and
   //https://github.com/llvm/llvm-project/issues/62202
   #define BOOST_MOVE_FORCEINLINE inline
#else
   #define BOOST_MOVE_FORCEINLINE BOOST_FORCEINLINE
#endif

namespace boost {
namespace movelib {

template <typename T1>
BOOST_FORCEINLINE BOOST_CXX14_CONSTEXPR void ignore(T1 const&)
{}

}} //namespace boost::movelib {

#if !(defined BOOST_NO_EXCEPTIONS)
#    define BOOST_MOVE_TRY { try
#    define BOOST_MOVE_CATCH(x) catch(x)
#    define BOOST_MOVE_RETHROW throw;
#    define BOOST_MOVE_CATCH_END }
#else
#    if !defined(BOOST_MSVC) || BOOST_MSVC >= 1900
#        define BOOST_MOVE_TRY { if (true)
#        define BOOST_MOVE_CATCH(x) else if (false)
#    else
// warning C4127: conditional expression is constant
#        define BOOST_MOVE_TRY { \
             __pragma(warning(push)) \
             __pragma(warning(disable: 4127)) \
             if (true) \
             __pragma(warning(pop))
#        define BOOST_MOVE_CATCH(x) else \
             __pragma(warning(push)) \
             __pragma(warning(disable: 4127)) \
             if (false) \
             __pragma(warning(pop))
#    endif
#    define BOOST_MOVE_RETHROW
#    define BOOST_MOVE_CATCH_END }
#endif

#ifndef BOOST_NO_CXX11_STATIC_ASSERT
#  ifndef BOOST_NO_CXX11_VARIADIC_MACROS
#     define BOOST_MOVE_STATIC_ASSERT( ... ) static_assert(__VA_ARGS__, #__VA_ARGS__)
#  else
#     define BOOST_MOVE_STATIC_ASSERT( B ) static_assert(B, #B)
#  endif
#else
namespace boost {
namespace move_detail {

template<bool B>
struct STATIC_ASSERTION_FAILURE;

template<>
struct STATIC_ASSERTION_FAILURE<true>{};

template<unsigned> struct static_assert_test {};

}}

#define BOOST_MOVE_STATIC_ASSERT(B) \
         typedef ::boost::move_detail::static_assert_test<\
            (unsigned)sizeof(::boost::move_detail::STATIC_ASSERTION_FAILURE<bool(B)>)>\
               BOOST_JOIN(boost_move_static_assert_typedef_, __LINE__) BOOST_ATTRIBUTE_UNUSED

#endif

#if !defined(__has_cpp_attribute) || defined(__CUDACC__)
#define BOOST_MOVE_HAS_MSVC_ATTRIBUTE(ATTR) 0
#else
#define BOOST_MOVE_HAS_MSVC_ATTRIBUTE(ATTR) __has_cpp_attribute(msvc::ATTR)
#endif

// See https://devblogs.microsoft.com/cppblog/improving-the-state-of-debug-performance-in-c/
// for details on how MSVC has improved debug experience, specifically for move/forward-like utilities
#if BOOST_MOVE_HAS_MSVC_ATTRIBUTE(intrinsic)
#define BOOST_MOVE_INTRINSIC_CAST [[msvc::intrinsic]]
#else
#define BOOST_MOVE_INTRINSIC_CAST BOOST_MOVE_FORCEINLINE
#endif

//BOOST_MOVE_TRIVIAL_ABI expands to the "trivial_abi" attribute when the compiler supports
//it (Clang). Define BOOST_MOVE_DISABLE_TRIVIAL_ABI to opt out
#if !defined(BOOST_MOVE_DISABLE_TRIVIAL_ABI) && defined(__has_attribute)
#  if __has_attribute(trivial_abi)
#     define BOOST_MOVE_TRIVIAL_ABI __attribute__((trivial_abi))
#  endif
#endif
#if !defined(BOOST_MOVE_TRIVIAL_ABI)
#  define BOOST_MOVE_TRIVIAL_ABI
#endif

#if defined(__has_builtin)
#if __has_builtin(__builtin_launder)
   #define BOOST_MOVE_HAS_BUILTIN_LAUNDER
#endif
#endif

//BOOST_MOVE_CXX20_CONSTEXPR expands to "constexpr" when the C++20 constexpr rules are available:
#if defined(__cpp_constexpr) && (__cpp_constexpr >= 201907L) && \
    defined(__cpp_constexpr_dynamic_alloc) && (__cpp_constexpr_dynamic_alloc >= 201907L)
#  define BOOST_MOVE_HAS_CXX20_CONSTEXPR
#  define BOOST_MOVE_CXX20_CONSTEXPR constexpr
#else
#  define BOOST_MOVE_CXX20_CONSTEXPR
#endif

//BOOST_MOVE_TEST_CONSTINIT is defined, for tests only, when the compiler can check that a
//variable is constant initialized (a C++11 constexpr constructor is really constexpr):
//C++20 "constinit", or the Clang and GCC (>= 10) extensions available in C++11 mode.
#if !defined(BOOST_NO_CXX11_CONSTEXPR)
#  if defined(__cpp_constinit) && (__cpp_constinit >= 201907L)
#     define BOOST_MOVE_TEST_CONSTINIT constinit
#  elif defined(__clang__) && defined(__has_cpp_attribute)
#     if __has_cpp_attribute(clang::require_constant_initialization)
#        define BOOST_MOVE_TEST_CONSTINIT [[clang::require_constant_initialization]]
#     endif
#  elif defined(BOOST_GCC) && (BOOST_GCC >= 100000)
#     define BOOST_MOVE_TEST_CONSTINIT __constinit
#  endif
#endif

#endif   //#ifndef BOOST_MOVE_DETAIL_WORKAROUND_HPP
