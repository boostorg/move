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

//MSVC 12.0 (Visual 2013) can not default the move operations
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && !defined(BOOST_NO_CXX11_DEFAULTED_MOVES)

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

#endif   //!defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && !defined(BOOST_NO_CXX11_DEFAULTED_MOVES)

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
   #if !defined(BOOST_NO_CXX11_DEFAULTED_MOVES)
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_move_constructible<pod_deleted_move_ctor>::value));
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_trivially_move_constructible<pod_deleted_move_ctor>::value));
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_nothrow_move_constructible<pod_deleted_move_ctor>::value));
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_move_assignable<pod_deleted_move_assign>::value));
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_trivially_move_assignable<pod_deleted_move_assign>::value));
   BOOST_MOVE_STATIC_ASSERT(!(boost::move_detail::is_nothrow_move_assignable<pod_deleted_move_assign>::value));
   //The other members of these types are still trivial. Some compilers (e.g. GCC 4.8)
   //do not report these types as POD, so check only if the intrinsic is available.
   #if defined(BOOST_MOVE_HAS_TRIVIAL_COPY)
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_trivially_copy_constructible<pod_deleted_move_ctor>::value));
   #endif
   #if defined(BOOST_MOVE_HAS_TRIVIAL_MOVE_ASSIGN)
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_trivially_move_assignable<pod_deleted_move_ctor>::value));
   #endif
   #if defined(BOOST_MOVE_HAS_TRIVIAL_ASSIGN)
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_trivially_copy_assignable<pod_deleted_move_assign>::value));
   #endif
   #if defined(BOOST_MOVE_HAS_TRIVIAL_MOVE_CONSTRUCTOR)
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::is_trivially_move_constructible<pod_deleted_move_assign>::value));
   #endif
   #endif   //!defined(BOOST_NO_CXX11_DEFAULTED_MOVES)
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

namespace is_nothrow_swappable_test
{

//If the macro is defined, there are enough intrinsics and languages features
#if defined(BOOST_MOVE_TT_CXX11_IS_NOTHROW_SWAPPABLE)

//Empty class with a move constructor that can throw: std::swap can throw
struct empty_throwing_move
{
   empty_throwing_move() {}
   empty_throwing_move(const empty_throwing_move&) {}
   empty_throwing_move(empty_throwing_move&&) noexcept(false) {}
   empty_throwing_move& operator=(const empty_throwing_move&) { return *this; }
   empty_throwing_move& operator=(empty_throwing_move&&) noexcept(false) { return *this; }
};

//Moves are noexcept, but the swap found by argument dependent lookup can throw
struct throwing_adl_swap
{
   int i;
   friend void swap(throwing_adl_swap&, throwing_adl_swap&) noexcept(false) {}
};

//Moves can throw, but the swap found by argument dependent lookup is noexcept
struct nothrow_adl_swap
{
   nothrow_adl_swap() {}
   nothrow_adl_swap(const nothrow_adl_swap&) {}
   nothrow_adl_swap(nothrow_adl_swap&&) noexcept(false) {}
   nothrow_adl_swap& operator=(const nothrow_adl_swap&) { return *this; }
   nothrow_adl_swap& operator=(nothrow_adl_swap&&) noexcept(false) { return *this; }
   friend void swap(nothrow_adl_swap&, nothrow_adl_swap&) noexcept {}
};

#endif   //#if defined(BOOST_MOVE_TT_CXX11_IS_NOTHROW_SWAPPABLE)

void test()
{
   using boost::move_detail::is_nothrow_swappable;
   BOOST_MOVE_STATIC_ASSERT((is_nothrow_swappable<int>::value));
   BOOST_MOVE_STATIC_ASSERT((is_nothrow_swappable<int*>::value));
   #if defined(BOOST_MOVE_IS_POD)
      BOOST_MOVE_STATIC_ASSERT((is_nothrow_swappable<pod_struct>::value));
   #endif
   #if defined(BOOST_MOVE_TT_CXX11_IS_NOTHROW_SWAPPABLE)
      BOOST_MOVE_STATIC_ASSERT(!(is_nothrow_swappable<empty_throwing_move>::value));
      BOOST_MOVE_STATIC_ASSERT(!(is_nothrow_swappable<throwing_adl_swap>::value));
      BOOST_MOVE_STATIC_ASSERT((is_nothrow_swappable<nothrow_adl_swap>::value));
      #if defined(BOOST_MOVE_TT_CXX11_IS_COPY_ASSIGNABLE) && !defined(BOOST_NO_CXX11_DEFAULTED_FUNCTIONS)
         //Not swappable: the copy assignment is deleted and there is no move assignment
         BOOST_MOVE_STATIC_ASSERT(!(is_nothrow_swappable<pod_deleted_copy_assign>::value));
      #endif   //defined(BOOST_MOVE_TT_CXX11_IS_COPY_ASSIGNABLE) && !defined(BOOST_NO_CXX11_DEFAULTED_FUNCTIONS)
   #endif   //BOOST_MOVE_TT_CXX11_IS_NOTHROW_SWAPPABLE
}

}  //namespace is_nothrow_swappable_test

namespace is_unsigned_test
{

enum unsigned_enum { unsigned_enum_value = 1u };
enum signed_enum   { signed_enum_value = 1    };

void test()
{
   using boost::move_detail::is_unsigned;
   //Unsigned integer types and bool
   BOOST_MOVE_STATIC_ASSERT((is_unsigned<bool>::value));
   BOOST_MOVE_STATIC_ASSERT((is_unsigned<unsigned char>::value));
   BOOST_MOVE_STATIC_ASSERT((is_unsigned<unsigned short>::value));
   BOOST_MOVE_STATIC_ASSERT((is_unsigned<unsigned int>::value));
   BOOST_MOVE_STATIC_ASSERT((is_unsigned<unsigned long>::value));
   BOOST_MOVE_STATIC_ASSERT((is_unsigned<const unsigned int>::value));
   BOOST_MOVE_STATIC_ASSERT((is_unsigned<volatile unsigned int>::value));
   BOOST_MOVE_STATIC_ASSERT((is_unsigned<std::size_t>::value));
   #ifdef BOOST_HAS_LONG_LONG
   BOOST_MOVE_STATIC_ASSERT((is_unsigned< ::boost::ulong_long_type>::value));
   BOOST_MOVE_STATIC_ASSERT(!(is_unsigned< ::boost::long_long_type>::value));
   #endif
   #ifndef BOOST_NO_CXX11_CHAR16_T
   BOOST_MOVE_STATIC_ASSERT((is_unsigned<char16_t>::value));
   #endif
   #ifndef BOOST_NO_CXX11_CHAR32_T
   BOOST_MOVE_STATIC_ASSERT((is_unsigned<char32_t>::value));
   #endif
   //char and wchar_t depend on the platform
   BOOST_MOVE_STATIC_ASSERT((is_unsigned<char>::value == (char(0) < char(-1))));
   #ifndef BOOST_NO_INTRINSIC_WCHAR_T
   BOOST_MOVE_STATIC_ASSERT((is_unsigned<wchar_t>::value == (wchar_t(0) < wchar_t(-1))));
   #endif
   //Everything else is not unsigned
   BOOST_MOVE_STATIC_ASSERT(!(is_unsigned<signed char>::value));
   BOOST_MOVE_STATIC_ASSERT(!(is_unsigned<short>::value));
   BOOST_MOVE_STATIC_ASSERT(!(is_unsigned<int>::value));
   BOOST_MOVE_STATIC_ASSERT(!(is_unsigned<long>::value));
   BOOST_MOVE_STATIC_ASSERT(!(is_unsigned<float>::value));
   BOOST_MOVE_STATIC_ASSERT(!(is_unsigned<double>::value));
   BOOST_MOVE_STATIC_ASSERT(!(is_unsigned<long double>::value));
   BOOST_MOVE_STATIC_ASSERT(!(is_unsigned<void>::value));
   BOOST_MOVE_STATIC_ASSERT(!(is_unsigned<unsigned int*>::value));
   BOOST_MOVE_STATIC_ASSERT(!(is_unsigned<unsigned int&>::value));
   BOOST_MOVE_STATIC_ASSERT(!(is_unsigned<unsigned_enum>::value));
   BOOST_MOVE_STATIC_ASSERT(!(is_unsigned<signed_enum>::value));
   BOOST_MOVE_STATIC_ASSERT(!(is_unsigned<trivial_but_not_pod>::value));
}

}  //namespace is_unsigned_test

namespace aligned_storage_test
{

template<class Storage, std::size_t Align>
struct test_static_object
{
   static void test()
   {
      static Storage static_object;
      BOOST_TEST(reinterpret_cast<std::size_t>(&static_object) % Align == 0);
   }
};

template<std::size_t Len, std::size_t Align>
void test_one()
{
   typedef typename boost::move_detail::aligned_storage<Len, Align>::type storage_t;
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::alignment_of<storage_t>::value >= Align));
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::alignment_of<storage_t>::value % Align == 0));
   BOOST_MOVE_STATIC_ASSERT((sizeof(storage_t) >= Len));
   //The alignment of real objects: a local, an array element and a static object
   storage_t local;
   storage_t array[2];
   BOOST_TEST(reinterpret_cast<std::size_t>(&local) % Align == 0);
   BOOST_TEST(reinterpret_cast<std::size_t>(&array[1]) % Align == 0);
   test_static_object<storage_t, Align>::test();
}

template<std::size_t Align>
struct test_alignment
{
   static void test()
   {
      test_one<1, Align>();
      test_one<Align, Align>();
      test_one<Align + 1, Align>();
      test_one<3 * Align, Align>();
      test_alignment<Align * 2>::test();
   }
};

//Alignments from 1 to 4096
template<>
struct test_alignment<4096*2>
{
   static void test() {}
};

void test()
{
   test_alignment<1>::test();
   //Default alignment
   typedef boost::move_detail::aligned_storage<sizeof(double)>::type default_storage_t;
   BOOST_MOVE_STATIC_ASSERT((boost::move_detail::alignment_of<default_storage_t>::value >=
                             boost::move_detail::alignment_of<boost::move_detail::max_align_t>::value));
}

}  //namespace aligned_storage_test

int main()
{
   trivially_memcopyable_test::test();
   is_pod_test::test();
   trivial_but_not_pod_test::test();
   pod_with_deleted_member_test::test();
   std_pair_test::test();
   is_nothrow_swappable_test::test();
   is_unsigned_test::test();
   aligned_storage_test::test();
   boost::report_errors();
}
