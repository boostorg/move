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
#include <boost/move/detail/type_traits.hpp>

#include <boost/move/detail/std_ns_begin.hpp>

BOOST_MOVE_STD_NS_BEG

struct input_iterator_tag;
struct forward_iterator_tag;
struct bidirectional_iterator_tag;
struct random_access_iterator_tag;
struct output_iterator_tag;

template<class Iterator>
struct iterator_traits;

//Declare it only if the standard library defines it (libstdc++ >= 10, libc++ >= 10)
#if (  (defined(BOOST_GNU_STDLIB) && defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 10) && (__cplusplus > 201703L))\
    || (defined(_LIBCPP_VERSION) && (_LIBCPP_VERSION >= 10000) && (_LIBCPP_STD_VER > 17))\
    || (defined(BOOST_DINKUMWARE_STDLIB) && defined(__cpp_lib_concepts))\
    || (!defined(BOOST_GNU_STDLIB) && !defined(_LIBCPP_VERSION) && !defined(BOOST_DINKUMWARE_STDLIB) && (__cplusplus >= 202002L))\
    )
#  define BOOST_MOVE_CONTIGUOUS_ITERATOR_TAG
struct contiguous_iterator_tag;

#endif

BOOST_MOVE_STD_NS_END

#include <boost/move/detail/std_ns_end.hpp>

namespace boost{  namespace movelib{

//! Iterators use this tag as "iterator_concept" to be detected by is_contiguous_iterator
//! in any C++ standard mode.
//!
//! If the standard library defines std::contiguous_iterator_tag, it is a typedef of
//! std::contiguous_iterator_tag. Otherwise it is an empty class (not derived from
//! std::random_access_iterator_tag, to avoid including <iterator>).
#if defined(BOOST_MOVE_CONTIGUOUS_ITERATOR_TAG)
typedef std::contiguous_iterator_tag contiguous_iterator_tag;
#else
struct contiguous_iterator_tag {};
#endif

namespace iter_detail {

typedef char yes_type;
struct no_type { char dummy[2]; };

template<class T>
struct type_tag {};

#define BOOST_MOVE_ITER_DETAIL_HAS_MEMBER_TYPE(NAME)   template<class T>   struct has_##NAME   {      template<class U> static yes_type test(type_tag<typename U::NAME>*);      template<class U> static no_type  test(...);      static const bool value = sizeof(test<T>(0)) == sizeof(yes_type);   };//

BOOST_MOVE_ITER_DETAIL_HAS_MEMBER_TYPE(value_type)
BOOST_MOVE_ITER_DETAIL_HAS_MEMBER_TYPE(pointer)
BOOST_MOVE_ITER_DETAIL_HAS_MEMBER_TYPE(reference)
BOOST_MOVE_ITER_DETAIL_HAS_MEMBER_TYPE(iterator_category)
BOOST_MOVE_ITER_DETAIL_HAS_MEMBER_TYPE(iterator_concept)

#undef BOOST_MOVE_ITER_DETAIL_HAS_MEMBER_TYPE

//An iterator without member types (e.g. C++20 std::common_iterator) obtains them from
//a std::iterator_traits specialization. The header that defines that specialization
//also defines std::iterator_traits, so the forward declaration is complete when used.
template<class Iterator, bool = has_value_type<Iterator>::value>
struct iter_difference_impl
{
   typedef typename Iterator::difference_type type;
};

template<class Iterator>
struct iter_difference_impl<Iterator, false>
{
   typedef typename std::iterator_traits<Iterator>::difference_type type;
};

template<class Iterator, bool = has_value_type<Iterator>::value>
struct iter_value_impl
{
   typedef typename Iterator::value_type type;
};

#if !defined(BOOST_NO_CXX11_DECLTYPE)
//Equivalent to std::declval<T&>(), only used in unevaluated operands
template<class T>
T& iter_lvalue_declval() BOOST_NOEXCEPT;
#endif

//An iterator without a member value_type (e.g. C++20 std::common_iterator) obtains it
//from the type returned by operator*. If Iterator is not dereferenceable, there is no
//"type" member, so that a deduction guide or an overload that uses iter_value<int>
//is discarded instead of giving a hard error.
#if !defined(BOOST_NO_CXX11_DECLTYPE) && !defined(BOOST_NO_SFINAE_EXPR)

template<class Iterator, class = void>
struct iter_value_impl_deref
{};

template<class Iterator>
struct iter_value_impl_deref
   < Iterator
   , typename ::boost::move_detail::voider<decltype(*iter_lvalue_declval<Iterator>())>::type>
{
   typedef typename ::boost::move_detail::remove_cvref
      <decltype(*iter_lvalue_declval<Iterator>())>::type type;
};

template<class Iterator>
struct iter_value_impl<Iterator, false>
   : iter_value_impl_deref<Iterator>
{};

#else

template<class Iterator>
struct iter_value_impl<Iterator, false>
{};

#endif

template<class Iterator, bool = has_value_type<Iterator>::value>
struct iter_category_impl
{
   typedef typename Iterator::iterator_category type;
};

template<class Iterator>
struct iter_category_impl<Iterator, false>
{
   typedef typename std::iterator_traits<Iterator>::iterator_category type;
};

//C++20: "pointer" is void if Iterator does not define it
template<class Iterator, bool = has_pointer<Iterator>::value>
struct iter_pointer
{
   typedef typename Iterator::pointer type;
};

template<class Iterator>
struct iter_pointer<Iterator, false>
{
   typedef void type;
};

//C++20: "reference" is iter_reference_t if Iterator does not define it
template<class Iterator, bool = has_reference<Iterator>::value>
struct iter_reference
{
   typedef typename Iterator::reference type;
};

template<class Iterator>
struct iter_reference<Iterator, false>
{
   #if !defined(BOOST_NO_CXX11_DECLTYPE)
   typedef decltype(*iter_lvalue_declval<Iterator>()) type;
   #else
   typedef typename Iterator::value_type &type;
   #endif
};

template<class Iterator, bool = has_value_type<Iterator>::value>
struct iterator_traits_impl
{
   typedef typename Iterator::difference_type            difference_type;
   typedef typename Iterator::value_type                 value_type;
   typedef typename iter_pointer<Iterator>::type         pointer;
   typedef typename iter_reference<Iterator>::type       reference;
   typedef typename Iterator::iterator_category          iterator_category;
};

template<class Iterator>
struct iterator_traits_impl<Iterator, false>
   : std::iterator_traits<Iterator>
{};

//ITER_CONCEPT(Iterator): Iterator::iterator_concept if defined,
//else Iterator::iterator_category if defined, else not contiguous.
template<class Iterator, bool = has_iterator_concept<Iterator>::value, bool = has_iterator_category<Iterator>::value>
struct iter_concept
{
   typedef typename Iterator::iterator_concept type;
};

template<class Iterator>
struct iter_concept<Iterator, false, true>
{
   typedef typename Iterator::iterator_category type;
};

template<class Iterator>
struct iter_concept<Iterator, false, false>
{
   typedef void type;
};

//contiguous_iterator_tag can be incomplete (forward declared std::contiguous_iterator_tag):
//the conversion from a tag pointer only needs the complete type of the tag that is tested.
yes_type is_contiguous_tag(contiguous_iterator_tag*);
no_type  is_contiguous_tag(...);

}  //namespace iter_detail {

template<class T>
struct iter_difference
   : iter_detail::iter_difference_impl<T>
{};

template<class T>
struct iter_difference<T*>
{
   typedef std::ptrdiff_t type;
};

template<class T>
struct iter_value
   : iter_detail::iter_value_impl<T>
{};

template<class T>
struct iter_value<T*>
{
   typedef T type;
};

//Remove cv qualifiers, as in C++20
template<class T>
struct iter_value<const T*>
{
   typedef T type;
};

template<class T>
struct iter_value<volatile T*>
{
   typedef T type;
};

template<class T>
struct iter_value<const volatile T*>
{
   typedef T type;
};

template<class T>
struct iter_category
   : iter_detail::iter_category_impl<T>
{};

template<class T>
struct iter_category<T*>
{
   typedef std::random_access_iterator_tag type;
};

template<class Iterator>
struct iterator_traits
   : iter_detail::iterator_traits_impl<Iterator>
{};

//! Value is true if Iterator is a contiguous iterator. It does not include
//! <iterator> or <concepts>: it is true for pointers and for iterators whose
//! ITER_CONCEPT is boost::movelib::contiguous_iterator_tag or a class derived from it.
//! If the standard library defines std::contiguous_iterator_tag, the result is the
//! same as std::contiguous_iterator for iterators that model it. It does not check
//! the semantic requirements (e.g. std::to_address).
template<class Iterator>
struct is_contiguous_iterator
{
   static const bool value = sizeof(iter_detail::is_contiguous_tag
      (static_cast<typename iter_detail::iter_concept<Iterator>::type*>(0))) == sizeof(iter_detail::yes_type);
};

template<class T>
struct is_contiguous_iterator<T*>
{
   static const bool value = true;
};

template<class T>
struct iterator_traits<T*>
{
   typedef std::ptrdiff_t                    difference_type;
   typedef T                                 value_type;
   typedef T*                                pointer;
   typedef T&                                reference;
   typedef std::random_access_iterator_tag   iterator_category;
   typedef contiguous_iterator_tag           iterator_concept;
};

template<class T>
struct iterator_traits<const T*>
{
   typedef std::ptrdiff_t                    difference_type;
   typedef T                                 value_type;
   typedef const T*                          pointer;
   typedef const T&                          reference;
   typedef std::random_access_iterator_tag   iterator_category;
   typedef contiguous_iterator_tag           iterator_concept;
};

template<class T>
struct iterator_traits<volatile T*>
{
   typedef std::ptrdiff_t                    difference_type;
   typedef T                                 value_type;
   typedef volatile T*                       pointer;
   typedef volatile T&                       reference;
   typedef std::random_access_iterator_tag   iterator_category;
   typedef contiguous_iterator_tag           iterator_concept;
};

template<class T>
struct iterator_traits<const volatile T*>
{
   typedef std::ptrdiff_t                    difference_type;
   typedef T                                 value_type;
   typedef const volatile T*                 pointer;
   typedef const volatile T&                 reference;
   typedef std::random_access_iterator_tag   iterator_category;
   typedef contiguous_iterator_tag           iterator_concept;
};

}} //namespace boost::movelib

#include <boost/move/detail/type_traits.hpp>

namespace boost {
namespace movelib {

template<class T>
struct iter_size
   : boost::move_detail::
      make_unsigned<typename iter_difference<T>::type >
{};

}}  //namespace boost move_detail {

#endif //#ifndef BOOST_MOVE_DETAIL_ITERATOR_TRAITS_HPP
