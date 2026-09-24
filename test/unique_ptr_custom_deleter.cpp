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
#include <boost/move/utility_core.hpp>
#include <boost/core/lightweight_test.hpp>

//Tests the storage of custom deleters: class deleters are stored as a base
//class (empty base optimization); final classes, unions and non-class
//deleters (e.g. function pointers) can not be base classes and are stored as members

static int deleted = 0;

//VS2012 supports "final" but has no intrinsic to detect it, so final deleters can not be used
#if !defined(BOOST_NO_CXX11_FINAL) && !(defined(BOOST_MSVC) && (BOOST_MSVC < 1800))
#define BOOST_MOVE_TEST_FINAL_DELETER
#endif

struct empty_deleter
{
   void operator()(int *p) const { delete p; ++deleted; }
};

#if defined(BOOST_MOVE_TEST_FINAL_DELETER)

struct final_deleter final
{
   void operator()(int *p) const { delete p; ++deleted; }
};

struct final_state_deleter final
{
   int id;
   final_state_deleter() : id(0) {}
   explicit final_state_deleter(int i) : id(i) {}
   void operator()(int *p) const { delete p; deleted += id; }
};

void test_final_deleter()
{
   deleted = 0;
   {
      boost::movelib::unique_ptr<int, final_deleter> p(new int(1));
      boost::movelib::unique_ptr<int, final_deleter> q(boost::move(p));
      BOOST_TEST(!p && q);
      q.reset(new int(2));
      BOOST_TEST(deleted == 1);
   }
   BOOST_TEST(deleted == 2);

   deleted = 0;
   {
      final_state_deleter fd(10);
      boost::movelib::unique_ptr<int, final_state_deleter> p(new int(1), fd);
      BOOST_TEST(p.get_deleter().id == 10);

      final_deleter local;
      boost::movelib::unique_ptr<int, final_deleter&> r(new int(3), local);
      BOOST_TEST(&r.get_deleter() == &local);
   }
   BOOST_TEST(deleted == 11);
}

#endif   //#if defined(BOOST_MOVE_TEST_FINAL_DELETER)

//Deleters that are not classes (or are unions) can not be a base class either,
//so unique_ptr stores them as a member

void function_deleter(int *p) { delete p; ++deleted; }

#if defined(__cpp_noexcept_function_type)
void noexcept_function_deleter(int *p) noexcept { delete p; ++deleted; }
#endif

union union_deleter
{
   int dummy;
   void operator()(int *p) const { delete p; ++deleted; }
};

template<class D>
void test_function_pointer_deleter(D d)
{
   deleted = 0;
   {
      boost::movelib::unique_ptr<int, D> p(new int(1), d);
      BOOST_TEST(p.get_deleter() == d);
      boost::movelib::unique_ptr<int, D> q(boost::move(p));
      BOOST_TEST(!p && q && q.get_deleter() == d);
      q.reset(new int(2));
      BOOST_TEST(deleted == 1);
   }
   BOOST_TEST(deleted == 2);
}

void test_union_deleter()
{
   deleted = 0;
   {
      union_deleter u;
      u.dummy = 0;
      boost::movelib::unique_ptr<int, union_deleter> p(new int(1), u);
      boost::movelib::unique_ptr<int, union_deleter> q(boost::move(p));
      q.reset(new int(2));
      BOOST_TEST(deleted == 1);
   }
   BOOST_TEST(deleted == 2);
}

int main()
{
   #if defined(BOOST_MOVE_TEST_FINAL_DELETER)
   test_final_deleter();
   #endif
   test_function_pointer_deleter(&function_deleter);
   #if defined(__cpp_noexcept_function_type)
   test_function_pointer_deleter(&noexcept_function_deleter);
   #endif
   test_union_deleter();
   //Empty base optimization is kept for non-final deleters
   BOOST_TEST(sizeof(boost::movelib::unique_ptr<int, empty_deleter>) == sizeof(int*));
   BOOST_TEST(sizeof(boost::movelib::unique_ptr<int>) == sizeof(int*));
   return boost::report_errors();
}
