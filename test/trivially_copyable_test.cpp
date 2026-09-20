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

//Checks that small by-value types (iterators, deleters) are trivially copyable
//so that compilers pass and return them in registers instead of by invisible reference.

#include <boost/move/detail/reverse_iterator.hpp>
#include <boost/move/iterator.hpp>
#include <boost/move/default_delete.hpp>
#include <boost/move/algo/predicate.hpp>
#include <boost/move/detail/type_traits.hpp>
#include <boost/move/detail/workaround.hpp>
#include <functional>

//Without compiler intrinsics the Boost.Move traits fall back to is_pod, which is
//false for any class type, so the checks are only meaningful when intrinsics exist.
#if defined(BOOST_MOVE_HAS_TRIVIAL_COPY) && defined(BOOST_MOVE_HAS_TRIVIAL_ASSIGN) && defined(BOOST_MOVE_HAS_TRIVIAL_DESTRUCTOR)

namespace bml = ::boost::movelib;

template<class T>
struct is_trivially_copyable
{
   static const bool value = ::boost::move_detail::is_trivially_copy_constructible<T>::value
                          && ::boost::move_detail::is_trivially_copy_assignable<T>::value
                          && ::boost::move_detail::is_trivially_destructible<T>::value;
};

//Iterators
BOOST_MOVE_STATIC_ASSERT(is_trivially_copyable<bml::reverse_iterator<int*> >::value);
BOOST_MOVE_STATIC_ASSERT(is_trivially_copyable<bml::reverse_iterator<const int*> >::value);
BOOST_MOVE_STATIC_ASSERT(is_trivially_copyable< ::boost::move_iterator<int*> >::value);

//Deleters
BOOST_MOVE_STATIC_ASSERT(is_trivially_copyable<bml::default_delete<int> >::value);
BOOST_MOVE_STATIC_ASSERT(is_trivially_copyable<bml::default_delete<int[]> >::value);

//Comparator wrappers used by the sort/merge algorithms
typedef std::less<int> trivial_comp;
BOOST_MOVE_STATIC_ASSERT(is_trivially_copyable<bml::negate<trivial_comp> >::value);
BOOST_MOVE_STATIC_ASSERT(is_trivially_copyable<bml::inverse<trivial_comp> >::value);
BOOST_MOVE_STATIC_ASSERT(is_trivially_copyable<bml::inverse<bml::negate<trivial_comp> > >::value);
BOOST_MOVE_STATIC_ASSERT(is_trivially_copyable<bml::antistable<trivial_comp> >::value);
BOOST_MOVE_STATIC_ASSERT(is_trivially_copyable<bml::antistable<bml::inverse<trivial_comp> > >::value);

#endif   //BOOST_MOVE_HAS_TRIVIAL_COPY && BOOST_MOVE_HAS_TRIVIAL_ASSIGN && BOOST_MOVE_HAS_TRIVIAL_DESTRUCTOR

int main()
{
   return 0;
}
