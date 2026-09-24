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

static int deleted = 0;

struct empty_deleter
{
   void operator()(int *p) const { delete p; ++deleted; }
};

#if !defined(BOOST_NO_CXX11_FINAL)

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

#endif   //#if !defined(BOOST_NO_CXX11_FINAL)

int main()
{
   #if !defined(BOOST_NO_CXX11_FINAL)
   test_final_deleter();
   #endif
   //Empty base optimization is kept for non-final deleters
   BOOST_TEST(sizeof(boost::movelib::unique_ptr<int, empty_deleter>) == sizeof(int*));
   BOOST_TEST(sizeof(boost::movelib::unique_ptr<int>) == sizeof(int*));
   return boost::report_errors();
}
