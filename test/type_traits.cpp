//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2015-2015.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/move/detail/type_traits.hpp>
#include <boost/move/core.hpp>
#include <boost/core/lightweight_test.hpp>
#include <utility>

//
//       pod_struct
//
#if defined(BOOST_MOVE_IS_POD)
struct pod_struct
{
   int i;
   float f;
};
#endif

//
//       deleted_copy_and_assign_type
//
#if !defined(BOOST_NO_CXX11_DELETED_FUNCTIONS)

struct deleted_copy_and_assign_type
{
   deleted_copy_and_assign_type(const deleted_copy_and_assign_type&) = delete;
   deleted_copy_and_assign_type & operator=(const deleted_copy_and_assign_type&) = delete;
};

#endif   //defined(BOOST_NO_CXX11_DELETED_FUNCTIONS)

//
//       POD types with one deleted special member: the is_pod shortcut must not hide it
//
#if !defined(BOOST_NO_CXX11_DELETED_FUNCTIONS) && !defined(BOOST_NO_CXX11_DEFAULTED_FUNCTIONS)

struct pod_deleted_copy_ctor
{
   pod_deleted_copy_ctor() = default;
   pod_deleted_copy_ctor(const pod_deleted_copy_ctor&) = delete;
   pod_deleted_copy_ctor& operator=(const pod_deleted_copy_ctor&) = default;
   int i;
};

struct pod_deleted_copy_assign
{
   pod_deleted_copy_assign() = default;
   pod_deleted_copy_assign(const pod_deleted_copy_assign&) = default;
   pod_deleted_copy_assign& operator=(const pod_deleted_copy_assign&) = delete;
   int i;
};

struct pod_deleted_default_ctor
{
   pod_deleted_default_ctor() = delete;
   int i;
};

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

struct pod_deleted_move_ctor
{
   pod_deleted_move_ctor() = default;
   pod_deleted_move_ctor(const pod_deleted_move_ctor&) = default;
   pod_deleted_move_ctor(pod_deleted_move_ctor&&) = delete;
   pod_deleted_move_ctor& operator=(const pod_deleted_move_ctor&) = default;
   pod_deleted_move_ctor& operator=(pod_deleted_move_ctor&&) = default;
   int i;
};

struct pod_deleted_move_assign
{
   pod_deleted_move_assign() = default;
   pod_deleted_move_assign(const pod_deleted_move_assign&) = default;
   pod_deleted_move_assign(pod_deleted_move_assign&&) = default;
   pod_deleted_move_assign& operator=(const pod_deleted_move_assign&) = default;
   pod_deleted_move_assign& operator=(pod_deleted_move_assign&&) = delete;
   int i;
};

#endif   //!defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

#endif   //!defined(BOOST_NO_CXX11_DELETED_FUNCTIONS) && !defined(BOOST_NO_CXX11_DEFAULTED_FUNCTIONS)

//
//       trivial_but_not_pod: user-provided default constructor, everything else trivial
//
struct trivial_but_not_pod
{
   trivial_but_not_pod() : i(0) {}
   int i;
};

//
//       boost_move_type
//
class boost_move_type
{
   BOOST_MOVABLE_BUT_NOT_COPYABLE(boost_move_type)
   public:
   boost_move_type(BOOST_RV_REF(boost_move_type)){}
   boost_move_type & operator=(BOOST_RV_REF(boost_move_type)){ return *this; }
};

namespace is_pod_test
{

void test()
{
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_pod<int>::value));
   #if defined(BOOST_MOVE_IS_POD)
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_pod<pod_struct>::value));
   #endif
}

}  //namespace is_pod_test

namespace trivially_memcopyable_test {

void test()
{
   #if !defined(BOOST_NO_CXX11_DELETED_FUNCTIONS)
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_trivially_copy_constructible<deleted_copy_and_assign_type>::value));
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_trivially_copy_assignable<deleted_copy_and_assign_type>::value));
   #endif   //#if !defined(BOOST_NO_CXX11_DELETED_FUNCTIONS)
   //boost_move_type
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_trivially_copy_constructible<boost_move_type>::value));
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_trivially_copy_assignable<boost_move_type>::value));
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_copy_constructible<boost_move_type>::value));
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_copy_assignable<boost_move_type>::value));
   //POD
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_trivially_copy_constructible<int>::value));
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_trivially_copy_assignable<int>::value));
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_copy_constructible<int>::value));
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_copy_assignable<int>::value));
   #if defined(BOOST_MOVE_IS_POD)
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_trivially_copy_constructible<pod_struct>::value));
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_trivially_copy_assignable<pod_struct>::value));
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_copy_constructible<pod_struct>::value));
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_copy_assignable<pod_struct>::value));
   #endif
}

}  //namespace trivially_memcopyable_test {

namespace trivial_but_not_pod_test
{

void test()
{
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_pod<trivial_but_not_pod>::value));
   #if defined(BOOST_MOVE_HAS_TRIVIAL_COPY)
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_trivially_copy_constructible<trivial_but_not_pod>::value));
   #endif
   #if defined(BOOST_MOVE_HAS_TRIVIAL_ASSIGN)
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_trivially_copy_assignable<trivial_but_not_pod>::value));
   #endif
   #if defined(BOOST_MOVE_HAS_TRIVIAL_DESTRUCTOR)
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_trivially_destructible<trivial_but_not_pod>::value));
   #endif
   #if defined(BOOST_MOVE_HAS_TRIVIAL_MOVE_CONSTRUCTOR)
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_trivially_move_constructible<trivial_but_not_pod>::value));
   #endif
   #if defined(BOOST_MOVE_HAS_TRIVIAL_MOVE_ASSIGN)
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_trivially_move_assignable<trivial_but_not_pod>::value));
   #endif
   #if defined(BOOST_MOVE_HAS_NOTHROW_COPY)
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_nothrow_copy_constructible<trivial_but_not_pod>::value));
   #endif
   #if defined(BOOST_MOVE_HAS_NOTHROW_MOVE_CONSTRUCTOR)
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_nothrow_move_constructible<trivial_but_not_pod>::value));
   #endif
}

}  //namespace trivial_but_not_pod_test {

namespace pod_with_deleted_member_test
{

void test()
{
   //The trivial and nothrow traits are still true for POD types
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_trivially_default_constructible<int>::value));
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_trivially_move_constructible<int>::value));
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_trivially_move_assignable<int>::value));
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_nothrow_default_constructible<int>::value));
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_nothrow_copy_constructible<int>::value));
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_nothrow_move_constructible<int>::value));
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_nothrow_copy_assignable<int>::value));
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_nothrow_move_assignable<int>::value));
   #if defined(BOOST_MOVE_IS_POD)
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_trivially_default_constructible<pod_struct>::value));
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_trivially_move_constructible<pod_struct>::value));
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_trivially_move_assignable<pod_struct>::value));
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_nothrow_copy_constructible<pod_struct>::value));
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_nothrow_move_constructible<pod_struct>::value));
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_nothrow_copy_assignable<pod_struct>::value));
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_nothrow_move_assignable<pod_struct>::value));
   #endif

   #if !defined(BOOST_NO_CXX11_DELETED_FUNCTIONS) && !defined(BOOST_NO_CXX11_DEFAULTED_FUNCTIONS)
   #if defined(BOOST_MOVE_TT_CXX11_IS_COPY_CONSTRUCTIBLE)
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_copy_constructible<pod_deleted_copy_ctor>::value));
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_trivially_copy_constructible<pod_deleted_copy_ctor>::value));
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_nothrow_copy_constructible<pod_deleted_copy_ctor>::value));
   #endif
   #if defined(BOOST_MOVE_TT_CXX11_IS_COPY_ASSIGNABLE)
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_copy_assignable<pod_deleted_copy_assign>::value));
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_trivially_copy_assignable<pod_deleted_copy_assign>::value));
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_nothrow_copy_assignable<pod_deleted_copy_assign>::value));
   #endif
   #if defined(BOOST_MOVE_TT_CXX11_IS_MOVE_CONSTRUCTIBLE_OR_ASSIGNABLE)
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_default_constructible<pod_deleted_default_ctor>::value));
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_trivially_default_constructible<pod_deleted_default_ctor>::value));
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_nothrow_default_constructible<pod_deleted_default_ctor>::value));
   //The move traits use the copy operation, like std traits, when there is no move operation
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_trivially_move_constructible<pod_deleted_copy_ctor>::value));
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_trivially_move_assignable<pod_deleted_copy_assign>::value));
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_move_constructible<pod_deleted_move_ctor>::value));
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_trivially_move_constructible<pod_deleted_move_ctor>::value));
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_nothrow_move_constructible<pod_deleted_move_ctor>::value));
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_move_assignable<pod_deleted_move_assign>::value));
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_trivially_move_assignable<pod_deleted_move_assign>::value));
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_nothrow_move_assignable<pod_deleted_move_assign>::value));
   //The other members of these types are still trivial
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_trivially_copy_constructible<pod_deleted_move_ctor>::value));
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_trivially_move_assignable<pod_deleted_move_ctor>::value));
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_trivially_copy_assignable<pod_deleted_move_assign>::value));
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_trivially_move_constructible<pod_deleted_move_assign>::value));
   #endif
   #endif   //!defined(BOOST_NO_CXX11_DELETED_FUNCTIONS) && !defined(BOOST_NO_CXX11_DEFAULTED_FUNCTIONS)
}

}  //namespace pod_with_deleted_member_test

namespace std_pair_test
{

void test()
{
   using boost::move_detail::is_trivially_copy_assignable;
   using boost::move_detail::is_trivially_move_assignable;
   BOOST_MOVE_STATIC_ASSERT((is_trivially_copy_assignable<std::pair<int, int> >::value));
   BOOST_MOVE_STATIC_ASSERT((is_trivially_move_assignable<std::pair<int, int> >::value));
   //Assignment of a pair assigns through reference members
   BOOST_MOVE_STATIC_ASSERT(!(is_trivially_copy_assignable<std::pair<int&, int> >::value));
   BOOST_MOVE_STATIC_ASSERT(!(is_trivially_move_assignable<std::pair<int&, int> >::value));
   BOOST_MOVE_STATIC_ASSERT(!(is_trivially_copy_assignable<std::pair<int, int&> >::value));
   BOOST_MOVE_STATIC_ASSERT(!(is_trivially_move_assignable<std::pair<int, int&> >::value));
   //Assignment of a pair with a const member is deleted
   BOOST_MOVE_STATIC_ASSERT(!(is_trivially_copy_assignable<std::pair<const int, int> >::value));
   BOOST_MOVE_STATIC_ASSERT(!(is_trivially_move_assignable<std::pair<const int, int> >::value));
   BOOST_MOVE_STATIC_ASSERT(!(is_trivially_copy_assignable<std::pair<int, const int> >::value));
   BOOST_MOVE_STATIC_ASSERT(!(is_trivially_move_assignable<std::pair<int, const int> >::value));
}

}  //namespace std_pair_test

int main()
{
   trivially_memcopyable_test::test();
   is_pod_test::test();
   trivial_but_not_pod_test::test();
   pod_with_deleted_member_test::test();
   std_pair_test::test();
   boost::report_errors();
}
