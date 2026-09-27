//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2026-2026.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/move/unique_ptr.hpp>

//The constructors and observers that are not valid for the deleter or element type
//do not participate in overload resolution for std::unique since C++17.
//Boost.Move's unique_ptr implements the same behavior if the compiler supports the
//minimum requirements

#if defined(BOOST_MOVE_UNIQUE_PTR_SFINAE_CONSTRAINTS) && !defined(BOOST_NO_CXX11_HDR_TYPE_TRAITS)

#include <type_traits>
#include <cstddef>

namespace bml = ::boost::movelib;

struct del
{
   void operator()(int *p) const { delete p; }
};

//Not default constructible
struct nodef_del
{
   explicit nodef_del(int) {}
   void operator()(int *p) const { delete p; }
};

//Move-only
struct move_del
{
   move_del() {}
   move_del(move_del &&) {}
   move_del(const move_del &) = delete;
   void operator()(int *p) const { delete p; }
};

typedef void (*fp_del)(int *);

template<class T> T declval_();

//Detection of the observers
template<class U, class = decltype(*declval_<const U&>())>
char has_deref(int);
template<class U> int has_deref(...);

template<class U, class = decltype(declval_<const U&>().operator->())>
char has_arrow(int);
template<class U> int has_arrow(...);

template<class U, class = decltype(declval_<const U&>()[0])>
char has_index(int);
template<class U> int has_index(...);

////////////////////////////////
//   Default and nullptr constructors
////////////////////////////////
BOOST_MOVE_STATIC_ASSERT(( std::is_default_constructible<bml::unique_ptr<int> >::value));
BOOST_MOVE_STATIC_ASSERT(( std::is_default_constructible<bml::unique_ptr<int, del> >::value));
BOOST_MOVE_STATIC_ASSERT((!std::is_default_constructible<bml::unique_ptr<int, fp_del> >::value));
BOOST_MOVE_STATIC_ASSERT((!std::is_default_constructible<bml::unique_ptr<int, del&> >::value));
BOOST_MOVE_STATIC_ASSERT((!std::is_default_constructible<bml::unique_ptr<int, nodef_del> >::value));

BOOST_MOVE_STATIC_ASSERT(( std::is_constructible<bml::unique_ptr<int>, std::nullptr_t>::value));
BOOST_MOVE_STATIC_ASSERT((!std::is_constructible<bml::unique_ptr<int, fp_del>, std::nullptr_t>::value));
BOOST_MOVE_STATIC_ASSERT((!std::is_constructible<bml::unique_ptr<int, del&>, std::nullptr_t>::value));
BOOST_MOVE_STATIC_ASSERT((!std::is_constructible<bml::unique_ptr<int, nodef_del>, std::nullptr_t>::value));

////////////////////////////////
//   Pointer constructor
////////////////////////////////
BOOST_MOVE_STATIC_ASSERT(( std::is_constructible<bml::unique_ptr<int>, int*>::value));
BOOST_MOVE_STATIC_ASSERT(( std::is_constructible<bml::unique_ptr<int[]>, int*>::value));
BOOST_MOVE_STATIC_ASSERT((!std::is_constructible<bml::unique_ptr<int, fp_del>, int*>::value));
BOOST_MOVE_STATIC_ASSERT((!std::is_constructible<bml::unique_ptr<int, del&>, int*>::value));
BOOST_MOVE_STATIC_ASSERT((!std::is_constructible<bml::unique_ptr<int, nodef_del>, int*>::value));

////////////////////////////////
//   Pointer and deleter constructors
////////////////////////////////
//Deleter copied from an lvalue
BOOST_MOVE_STATIC_ASSERT(( std::is_constructible<bml::unique_ptr<int, del>, int*, const del&>::value));
BOOST_MOVE_STATIC_ASSERT(( std::is_constructible<bml::unique_ptr<int, fp_del>, int*, fp_del>::value));
BOOST_MOVE_STATIC_ASSERT(( std::is_constructible<bml::unique_ptr<int, nodef_del>, int*, const nodef_del&>::value));
BOOST_MOVE_STATIC_ASSERT((!std::is_constructible<bml::unique_ptr<int, move_del>, int*, const move_del&>::value));
BOOST_MOVE_STATIC_ASSERT(( std::is_constructible<bml::unique_ptr<int, del>, std::nullptr_t, const del&>::value));
BOOST_MOVE_STATIC_ASSERT((!std::is_constructible<bml::unique_ptr<int, move_del>, std::nullptr_t, const move_del&>::value));

//Deleter moved from an rvalue
BOOST_MOVE_STATIC_ASSERT(( std::is_constructible<bml::unique_ptr<int, del>, int*, del>::value));
BOOST_MOVE_STATIC_ASSERT(( std::is_constructible<bml::unique_ptr<int, move_del>, int*, move_del>::value));
BOOST_MOVE_STATIC_ASSERT(( std::is_constructible<bml::unique_ptr<int, move_del>, std::nullptr_t, move_del>::value));

//Reference deleters bind to lvalues, but never to rvalues (the reference would dangle)
BOOST_MOVE_STATIC_ASSERT(( std::is_constructible<bml::unique_ptr<int, del&>, int*, del&>::value));
BOOST_MOVE_STATIC_ASSERT(( std::is_constructible<bml::unique_ptr<int, const del&>, int*, const del&>::value));
BOOST_MOVE_STATIC_ASSERT((!std::is_constructible<bml::unique_ptr<int, del&>, int*, del>::value));
BOOST_MOVE_STATIC_ASSERT((!std::is_constructible<bml::unique_ptr<int, const del&>, int*, del>::value));
BOOST_MOVE_STATIC_ASSERT((!std::is_constructible<bml::unique_ptr<int, const del&>, int*, const del>::value));
BOOST_MOVE_STATIC_ASSERT(( std::is_constructible<bml::unique_ptr<int, del&>, std::nullptr_t, del&>::value));
BOOST_MOVE_STATIC_ASSERT((!std::is_constructible<bml::unique_ptr<int, del&>, std::nullptr_t, del>::value));
BOOST_MOVE_STATIC_ASSERT((!std::is_constructible<bml::unique_ptr<int, const del&>, std::nullptr_t, del>::value));

////////////////////////////////
//   Observers
////////////////////////////////
BOOST_MOVE_STATIC_ASSERT((sizeof(has_deref<bml::unique_ptr<int> >(0))    == 1u));
BOOST_MOVE_STATIC_ASSERT((sizeof(has_deref<bml::unique_ptr<int[]> >(0))  != 1u));
BOOST_MOVE_STATIC_ASSERT((sizeof(has_arrow<bml::unique_ptr<int> >(0))    == 1u));
BOOST_MOVE_STATIC_ASSERT((sizeof(has_arrow<bml::unique_ptr<int[]> >(0))  != 1u));
BOOST_MOVE_STATIC_ASSERT((sizeof(has_index<bml::unique_ptr<int[]> >(0))  == 1u));
BOOST_MOVE_STATIC_ASSERT((sizeof(has_index<bml::unique_ptr<int[2]> >(0)) == 1u));
BOOST_MOVE_STATIC_ASSERT((sizeof(has_index<bml::unique_ptr<int> >(0))    != 1u));

#endif   //defined(BOOST_MOVE_UNIQUE_PTR_SFINAE_CONSTRAINTS) && !defined(BOOST_NO_CXX11_HDR_TYPE_TRAITS)

int main()
{
   return 0;
}
