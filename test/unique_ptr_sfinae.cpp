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

//Move constructible and move assignable, not copyable
struct move_assign_del
{
   move_assign_del() {}
   move_assign_del(move_assign_del &&) {}
   move_assign_del& operator=(move_assign_del &&) { return *this; }
   void operator()(int *p) const { delete p; }
};

//Move constructible, not assignable
struct noassign_del
{
   noassign_del() {}
   noassign_del(const noassign_del &) {}
   noassign_del& operator=(const noassign_del &) = delete;
   void operator()(int *p) const { delete p; }
};

//Not move constructible, assignable
struct nomove_del
{
   nomove_del() {}
   nomove_del(const nomove_del &) = delete;
   nomove_del& operator=(const nomove_del &) { return *this; }
   void operator()(int *p) const { delete p; }
};

//Not movable, but swappable with a swap found by argument dependent lookup
struct adl_swap_del
{
   adl_swap_del() {}
   adl_swap_del(const adl_swap_del &) = delete;
   adl_swap_del& operator=(const adl_swap_del &) = delete;
   void operator()(int *p) const { delete p; }
   friend void swap(adl_swap_del &, adl_swap_del &) {}
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

#if defined(BOOST_MOVE_UNIQUE_PTR_MOVE_CONSTRAINTS)

////////////////////////////////
//   Move constructor and move assignment (LWG 2899)
////////////////////////////////
BOOST_MOVE_STATIC_ASSERT(( std::is_move_constructible<bml::unique_ptr<int> >::value));
BOOST_MOVE_STATIC_ASSERT(( std::is_move_assignable<bml::unique_ptr<int> >::value));
BOOST_MOVE_STATIC_ASSERT(( std::is_move_constructible<bml::unique_ptr<int[]> >::value));
BOOST_MOVE_STATIC_ASSERT(( std::is_move_assignable<bml::unique_ptr<int[]> >::value));
BOOST_MOVE_STATIC_ASSERT(( std::is_move_constructible<bml::unique_ptr<int, move_assign_del> >::value));
BOOST_MOVE_STATIC_ASSERT(( std::is_move_assignable<bml::unique_ptr<int, move_assign_del> >::value));
BOOST_MOVE_STATIC_ASSERT(( std::is_move_constructible<bml::unique_ptr<int, fp_del> >::value));
BOOST_MOVE_STATIC_ASSERT(( std::is_move_assignable<bml::unique_ptr<int, fp_del> >::value));
BOOST_MOVE_STATIC_ASSERT(( std::is_move_constructible<bml::unique_ptr<int, noassign_del> >::value));
BOOST_MOVE_STATIC_ASSERT((!std::is_move_assignable<bml::unique_ptr<int, noassign_del> >::value));
BOOST_MOVE_STATIC_ASSERT((!std::is_move_constructible<bml::unique_ptr<int, nomove_del> >::value));
BOOST_MOVE_STATIC_ASSERT(( std::is_move_assignable<bml::unique_ptr<int, nomove_del> >::value));
//Reference deleters: the reference is copied, and the referred deleter is copy assigned
BOOST_MOVE_STATIC_ASSERT(( std::is_move_constructible<bml::unique_ptr<int, del&> >::value));
BOOST_MOVE_STATIC_ASSERT(( std::is_move_assignable<bml::unique_ptr<int, del&> >::value));
BOOST_MOVE_STATIC_ASSERT(( std::is_move_constructible<bml::unique_ptr<int, noassign_del&> >::value));
BOOST_MOVE_STATIC_ASSERT((!std::is_move_assignable<bml::unique_ptr<int, noassign_del&> >::value));
BOOST_MOVE_STATIC_ASSERT((!std::is_move_assignable<bml::unique_ptr<int, const del&> >::value));
//Copies are never possible
BOOST_MOVE_STATIC_ASSERT((!std::is_copy_constructible<bml::unique_ptr<int, del> >::value));
BOOST_MOVE_STATIC_ASSERT((!std::is_copy_assignable<bml::unique_ptr<int, del> >::value));

////////////////////////////////
//   Free swap (found by argument dependent lookup)
////////////////////////////////
BOOST_MOVE_STATIC_ASSERT((::boost::move_detail::is_adl_swappable<bml::unique_ptr<int> >::value));
BOOST_MOVE_STATIC_ASSERT((::boost::move_detail::is_adl_swappable<bml::unique_ptr<int, move_assign_del> >::value));
BOOST_MOVE_STATIC_ASSERT((::boost::move_detail::is_adl_swappable<bml::unique_ptr<int, adl_swap_del> >::value));
BOOST_MOVE_STATIC_ASSERT((::boost::move_detail::is_adl_swappable<bml::unique_ptr<int, del&> >::value));
BOOST_MOVE_STATIC_ASSERT((!::boost::move_detail::is_adl_swappable<bml::unique_ptr<int, noassign_del> >::value));
BOOST_MOVE_STATIC_ASSERT((!::boost::move_detail::is_adl_swappable<bml::unique_ptr<int, nomove_del> >::value));

#endif   //defined(BOOST_MOVE_UNIQUE_PTR_MOVE_CONSTRAINTS)

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

#if defined(BOOST_MOVE_UNIQUE_PTR_MOVE_CONSTRAINTS)

//The move operations are templates: check that they are used to move, also to return by value
bml::unique_ptr<int, move_assign_del> make_up(int i)
{
   bml::unique_ptr<int, move_assign_del> up(new int(i));
   return up;
}

bool test_move()
{
   bml::unique_ptr<int, move_assign_del> a(make_up(1));
   bml::unique_ptr<int, move_assign_del> b(::boost::move(a));
   bool ok = !a && b && *b == 1;
   a = make_up(2);
   ok = ok && a && *a == 2;
   a = ::boost::move(b);
   ok = ok && !b && a && *a == 1;
   swap(a, b);
   ok = ok && !a && b && *b == 1;
   bml::unique_ptr<int[]> c(new int[2]);
   int *const p = c.get();
   bml::unique_ptr<int[]> d(::boost::move(c));
   c = ::boost::move(d);
   return ok && c.get() == p && !d;
}

#endif   //defined(BOOST_MOVE_UNIQUE_PTR_MOVE_CONSTRAINTS)

#endif   //defined(BOOST_MOVE_UNIQUE_PTR_SFINAE_CONSTRAINTS) && !defined(BOOST_NO_CXX11_HDR_TYPE_TRAITS)

int main()
{
   #if defined(BOOST_MOVE_UNIQUE_PTR_SFINAE_CONSTRAINTS) && !defined(BOOST_NO_CXX11_HDR_TYPE_TRAITS) && \
       defined(BOOST_MOVE_UNIQUE_PTR_MOVE_CONSTRAINTS)
   if(!test_move())
      return 1;
   #endif
   return 0;
}
