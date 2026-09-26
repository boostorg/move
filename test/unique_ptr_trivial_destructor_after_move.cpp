//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/move/unique_ptr.hpp>
#include <boost/move/traits.hpp>
#include <boost/move/detail/type_traits.hpp>

namespace bml = ::boost::movelib;

namespace {

//A deleter with a destructor that is not trivial
struct nontrivial_deleter
{
   ~nontrivial_deleter() {}
   void operator()(int *p) const { delete p; }
};

//A deleter with a destructor that is not trivial, but that has
//no effect on a moved-from object (it specializes the trait)
struct nontrivial_movable_deleter
{
   ~nontrivial_movable_deleter() {}
   void operator()(int *p) const { delete p; }
};

//A fancy pointer with a trivial destructor
struct trivial_ptr
{
   int *p;
};

struct trivial_ptr_deleter
{
   typedef trivial_ptr pointer;
   void operator()(pointer) const {}
};

//A fancy pointer with a destructor that is not trivial
struct nontrivial_ptr
{
   ~nontrivial_ptr() {}
   int *p;
};

struct nontrivial_ptr_deleter
{
   typedef nontrivial_ptr pointer;
   void operator()(pointer) const {}
};

}  //namespace {

namespace boost {

template<>
struct has_trivial_destructor_after_move<nontrivial_movable_deleter>
{
   static const bool value = true;
};

}  //namespace boost {

template<class T, class D>
struct up_has_trivial_dtor_after_move
   : ::boost::has_trivial_destructor_after_move< bml::unique_ptr<T, D> >
{};

//unique_ptr has a destructor that is not trivial
BOOST_MOVE_STATIC_ASSERT(!::boost::move_detail::is_trivially_destructible< bml::unique_ptr<int> >::value);

//These results do not need the BOOST_MOVE_HAS_TRIVIAL_DESTRUCTOR intrinsic:
//the deleter and the pointer are scalar types, references or specializations
BOOST_MOVE_STATIC_ASSERT((up_has_trivial_dtor_after_move<int, void(*)(int*)>::value));
BOOST_MOVE_STATIC_ASSERT((up_has_trivial_dtor_after_move<int, nontrivial_deleter&>::value));
BOOST_MOVE_STATIC_ASSERT((up_has_trivial_dtor_after_move<int, const nontrivial_deleter&>::value));
BOOST_MOVE_STATIC_ASSERT((up_has_trivial_dtor_after_move<int, nontrivial_movable_deleter>::value));
BOOST_MOVE_STATIC_ASSERT((up_has_trivial_dtor_after_move<int[], nontrivial_movable_deleter>::value));
BOOST_MOVE_STATIC_ASSERT(!(up_has_trivial_dtor_after_move<int, nontrivial_deleter>::value));
BOOST_MOVE_STATIC_ASSERT(!(up_has_trivial_dtor_after_move<int[], nontrivial_deleter>::value));
BOOST_MOVE_STATIC_ASSERT(!(up_has_trivial_dtor_after_move<int, nontrivial_ptr_deleter>::value));
BOOST_MOVE_STATIC_ASSERT(!(up_has_trivial_dtor_after_move<int, nontrivial_ptr_deleter&>::value));

//Class types with trivial destructors are detected only with the intrinsic
#if defined(BOOST_MOVE_HAS_TRIVIAL_DESTRUCTOR)
BOOST_MOVE_STATIC_ASSERT((up_has_trivial_dtor_after_move<int, bml::default_delete<int> >::value));
BOOST_MOVE_STATIC_ASSERT((up_has_trivial_dtor_after_move<int[], bml::default_delete<int[]> >::value));
BOOST_MOVE_STATIC_ASSERT((up_has_trivial_dtor_after_move<int[2], bml::default_delete<int[2]> >::value));
BOOST_MOVE_STATIC_ASSERT((up_has_trivial_dtor_after_move<int, trivial_ptr_deleter>::value));
BOOST_MOVE_STATIC_ASSERT((up_has_trivial_dtor_after_move<int, trivial_ptr_deleter&>::value));
#endif

int main()
{
   return 0;
}
