//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2026-2026. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#ifndef BOOST_MOVE_DETAIL_STD_NEW_FWD_HPP
#define BOOST_MOVE_DETAIL_STD_NEW_FWD_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif

#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <cstddef>   //std::size_t

//Declarations of std::nothrow_t, std::nothrow and the nothrow allocation and deallocation functions,
//so that "new (std::nothrow) T" can be used without including <new>. They are redeclarations of
//the entities declared in <new>, so they must be the same:
//
// - std::nothrow_t, std::nothrow and std::align_val_t are declared in namespace std, not in the
//   versioned inline namespace of libc++ (std::__1) or libstdc++, so BOOST_MOVE_STD_NS_BEG is not used.
// - The calling convention (__cdecl in the MSVC STL, as /Gz or /Gr change the default) and the
//   exception specification (throw() in C++03) must be the same.
// - The deallocation functions are called if the constructor throws: without them the memory leaks.
// - The align_val_t overloads are used for over-aligned types: without them the alignment is ignored.

#if defined(BOOST_DINKUMWARE_STDLIB) && defined(__CRTDECL)
#  define BOOST_MOVE_STD_NEW_FWD_CC __CRTDECL   //__cdecl (__clrcall with /clr:pure)
#elif defined(BOOST_DINKUMWARE_STDLIB)
#  define BOOST_MOVE_STD_NEW_FWD_CC __cdecl
#else
#  define BOOST_MOVE_STD_NEW_FWD_CC
#endif

//The same exception specification as the standard library declarations, which depends on
//the library version and the language mode
#if defined(_LIBCPP_VERSION) && defined(_NOEXCEPT)
#  define BOOST_MOVE_STD_NEW_FWD_NOEXCEPT _NOEXCEPT                //noexcept or throw()
#elif defined(BOOST_GNU_STDLIB) && defined(_GLIBCXX_USE_NOEXCEPT)
#  define BOOST_MOVE_STD_NEW_FWD_NOEXCEPT _GLIBCXX_USE_NOEXCEPT    //noexcept or throw()
#elif defined(BOOST_DINKUMWARE_STDLIB) && defined(_MSC_VER) && (_MSC_VER >= 1910)
#  define BOOST_MOVE_STD_NEW_FWD_NOEXCEPT noexcept                 //<vcruntime_new.h>
#elif defined(BOOST_DINKUMWARE_STDLIB) && defined(_MSC_VER) && (_MSC_VER >= 1900)
#  define BOOST_MOVE_STD_NEW_FWD_NOEXCEPT throw()                  //<vcruntime_new.h> in MSVC 14.0
#elif defined(BOOST_DINKUMWARE_STDLIB) && defined(_HAS_EXCEPTIONS) && !_HAS_EXCEPTIONS
#  define BOOST_MOVE_STD_NEW_FWD_NOEXCEPT                          //_THROW0() in <new> (MSVC 9.0 - 12.0)
#elif defined(BOOST_DINKUMWARE_STDLIB)
#  define BOOST_MOVE_STD_NEW_FWD_NOEXCEPT throw()                  //_THROW0() in <new> (MSVC 9.0 - 12.0)
#else
#  define BOOST_MOVE_STD_NEW_FWD_NOEXCEPT BOOST_NOEXCEPT_OR_NOTHROW
#endif

//The same annotations as the MSVC declarations of the nothrow operator new: some compilers
//(e.g. MSVC 9.0) warn in <new> (C4985) if a previous declaration has different attributes.
//The annotations use the name of the size parameter.
#if defined(BOOST_DINKUMWARE_STDLIB) && defined(_MSC_VER)
#  define BOOST_MOVE_STD_NEW_FWD_SIZE std::size_t _Size
#  if (_MSC_VER >= 1900) && defined(_VCRT_ALLOCATOR)
#     define BOOST_MOVE_STD_NEW_FWD_NEW_ATTR _Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(_Size) _VCRT_ALLOCATOR
#  elif (_MSC_VER >= 1900)
#     define BOOST_MOVE_STD_NEW_FWD_NEW_ATTR _Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(_Size)
#  elif (_MSC_VER >= 1700)
#     define BOOST_MOVE_STD_NEW_FWD_NEW_ATTR _Ret_maybenull_ _Post_writable_byte_size_(_Size)
#  else
#     define BOOST_MOVE_STD_NEW_FWD_NEW_ATTR _Ret_opt_bytecap_(_Size)
#  endif
#else
#  define BOOST_MOVE_STD_NEW_FWD_SIZE std::size_t
#  define BOOST_MOVE_STD_NEW_FWD_NEW_ATTR
#endif

//Visibility and DLL import attributes of the standard library declarations
#if defined(_LIBCPP_VERSION) && defined(_LIBCPP_OVERRIDABLE_FUNC_VIS)
#  define BOOST_MOVE_STD_NEW_FWD_FUNC_VIS _LIBCPP_OVERRIDABLE_FUNC_VIS
#else
#  define BOOST_MOVE_STD_NEW_FWD_FUNC_VIS BOOST_SYMBOL_VISIBLE
#endif

#if defined(_LIBCPP_VERSION) && defined(_LIBCPP_EXPORTED_FROM_ABI)
#  define BOOST_MOVE_STD_NEW_FWD_OBJ_VIS _LIBCPP_EXPORTED_FROM_ABI
#elif defined(_LIBCPP_VERSION) && defined(_LIBCPP_FUNC_VIS)
#  define BOOST_MOVE_STD_NEW_FWD_OBJ_VIS _LIBCPP_FUNC_VIS
#else
#  define BOOST_MOVE_STD_NEW_FWD_OBJ_VIS BOOST_SYMBOL_VISIBLE
#endif

#if defined(_MSC_VER)
#  pragma warning (push)
#  pragma warning (disable : 4643)  //Forward declaring 'X' in namespace std is not permitted by the C++ Standard
#  pragma warning (disable : 28251) //Inconsistent annotation (/analyze): <new> uses SAL annotations
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wredundant-decls"
#endif

#if defined(BOOST_DINKUMWARE_STDLIB)
extern "C++" {   //as in <vcruntime_new.h>, so that the declarations are attached to the global module
#endif

namespace std {

struct nothrow_t;
extern BOOST_MOVE_STD_NEW_FWD_OBJ_VIS const nothrow_t nothrow;

#if defined(__cpp_aligned_new)
enum class align_val_t : std::size_t;
#endif

}  //namespace std {

BOOST_MOVE_STD_NEW_FWD_FUNC_VIS BOOST_MOVE_STD_NEW_FWD_NEW_ATTR void* BOOST_MOVE_STD_NEW_FWD_CC operator new  (BOOST_MOVE_STD_NEW_FWD_SIZE, const std::nothrow_t&) BOOST_MOVE_STD_NEW_FWD_NOEXCEPT;
BOOST_MOVE_STD_NEW_FWD_FUNC_VIS BOOST_MOVE_STD_NEW_FWD_NEW_ATTR void* BOOST_MOVE_STD_NEW_FWD_CC operator new[](BOOST_MOVE_STD_NEW_FWD_SIZE, const std::nothrow_t&) BOOST_MOVE_STD_NEW_FWD_NOEXCEPT;
BOOST_MOVE_STD_NEW_FWD_FUNC_VIS void  BOOST_MOVE_STD_NEW_FWD_CC operator delete  (void*, const std::nothrow_t&) BOOST_MOVE_STD_NEW_FWD_NOEXCEPT;
BOOST_MOVE_STD_NEW_FWD_FUNC_VIS void  BOOST_MOVE_STD_NEW_FWD_CC operator delete[](void*, const std::nothrow_t&) BOOST_MOVE_STD_NEW_FWD_NOEXCEPT;

#if defined(__cpp_aligned_new)
BOOST_MOVE_STD_NEW_FWD_FUNC_VIS BOOST_MOVE_STD_NEW_FWD_NEW_ATTR void* BOOST_MOVE_STD_NEW_FWD_CC operator new  (BOOST_MOVE_STD_NEW_FWD_SIZE, std::align_val_t, const std::nothrow_t&) BOOST_MOVE_STD_NEW_FWD_NOEXCEPT;
BOOST_MOVE_STD_NEW_FWD_FUNC_VIS BOOST_MOVE_STD_NEW_FWD_NEW_ATTR void* BOOST_MOVE_STD_NEW_FWD_CC operator new[](BOOST_MOVE_STD_NEW_FWD_SIZE, std::align_val_t, const std::nothrow_t&) BOOST_MOVE_STD_NEW_FWD_NOEXCEPT;
BOOST_MOVE_STD_NEW_FWD_FUNC_VIS void  BOOST_MOVE_STD_NEW_FWD_CC operator delete  (void*, std::align_val_t, const std::nothrow_t&) BOOST_MOVE_STD_NEW_FWD_NOEXCEPT;
BOOST_MOVE_STD_NEW_FWD_FUNC_VIS void  BOOST_MOVE_STD_NEW_FWD_CC operator delete[](void*, std::align_val_t, const std::nothrow_t&) BOOST_MOVE_STD_NEW_FWD_NOEXCEPT;
#endif

#if defined(BOOST_DINKUMWARE_STDLIB)
}  //extern "C++" {
#endif

#if defined(_MSC_VER)
#  pragma warning (pop)
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif

//BOOST_MOVE_STD_NEW_FWD_CC and BOOST_MOVE_STD_NEW_FWD_NOEXCEPT are not undefined: replacements of the global
//allocation and deallocation functions must use the same calling convention and exception specification.
#undef BOOST_MOVE_STD_NEW_FWD_FUNC_VIS
#undef BOOST_MOVE_STD_NEW_FWD_OBJ_VIS
#undef BOOST_MOVE_STD_NEW_FWD_SIZE
#undef BOOST_MOVE_STD_NEW_FWD_NEW_ATTR

#endif   //#ifndef BOOST_MOVE_DETAIL_STD_NEW_FWD_HPP
