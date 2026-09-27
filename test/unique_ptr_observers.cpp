//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Howard Hinnant 2009
// (C) Copyright Ion Gaztanaga 2014-2014.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/move/utility_core.hpp>
#include <boost/move/unique_ptr.hpp>
#include <boost/core/lightweight_test.hpp>

//////////////////////////////////////////////
//
// The initial implementation of these tests
// was written by Howard Hinnant. 
//
// These test were later refactored grouping
// and porting them to Boost.Move.
//
// Many thanks to Howard for releasing his C++03
// unique_ptr implementation with such detailed
// test cases.
//
//////////////////////////////////////////////

#include "unique_ptr_test_utils_beg.hpp"

namespace bml = ::boost::movelib;

////////////////////////////////
//   unique_ptr_observers_dereference
////////////////////////////////

namespace unique_ptr_observers_dereference{

//A pointer whose dereference can throw
struct throwing_deref_ptr
{
   throwing_deref_ptr() : p() {}
   throwing_deref_ptr(int *q) : p(q) {}
   int& operator*() const { return *p; }
   int *p;
};

struct throwing_deref_deleter
{
   typedef throwing_deref_ptr pointer;
   void operator()(throwing_deref_ptr q) const { delete q.p; }
};

void test()
{
   //Single unique_ptr
   {
   bml::unique_ptr<int> p(new int(3));
   BOOST_TEST(*p == 3);
   }
   //Unbounded array unique_ptr
   {
   int *pi = new int[2];
   pi[0] = 3;
   pi[1] = 4;
   bml::unique_ptr<int[]> p(pi);
   BOOST_TEST(p[0] == 3);
   BOOST_TEST(p[1] == 4);
   }
   //Bounded array unique_ptr
   {
   int *pi = new int[2];
   pi[0] = 3;
   pi[1] = 4;
   bml::unique_ptr<int[2]> p(pi);
   BOOST_TEST(p[0] == 3);
   BOOST_TEST(p[1] == 4);
   }
   #if !defined(BOOST_NO_CXX11_NOEXCEPT)
   BOOST_MOVE_STATIC_ASSERT(( noexcept(*::boost::move_detail::declval<const bml::unique_ptr<int>&>()) ));
   #endif
   #if defined(BOOST_MOVE_UNIQUE_PTR_CONDITIONAL_DEREF_NOEXCEPT)
   //operator* is noexcept if the dereference of pointer is noexcept (LWG 2762)
   BOOST_MOVE_STATIC_ASSERT(( !noexcept(*::boost::move_detail::declval<const bml::unique_ptr<int, throwing_deref_deleter>&>()) ));
   #endif
}

}  //namespace unique_ptr_observers_dereference{

////////////////////////////////
//   unique_ptr_observers_dereference
////////////////////////////////

namespace unique_ptr_observers_explicit_bool{

void test()
{
   //Single unique_ptr
   {
   bml::unique_ptr<int> p(new int(3));
   if (p)
      ;
   else
      BOOST_TEST(false);
   if (!p)
      BOOST_TEST(false);
   }
   {
   bml::unique_ptr<int> p;
   if (!p)
      ;
   else
      BOOST_TEST(false);
   if (p)
      BOOST_TEST(false);
   }
   //Unbounded array unique_ptr
   {
   bml::unique_ptr<int[]> p(new int[2]);
   if (p)
      ;
   else
      BOOST_TEST(false);
   if (!p)
      BOOST_TEST(false);
   }
   {
   bml::unique_ptr<int[]> p;
   if (!p)
      ;
   else
      BOOST_TEST(false);
   if (p)
      BOOST_TEST(false);
   }
   //Bounded array unique_ptr
   {
   bml::unique_ptr<int[2]> p(new int[2]);
   if (p)
      ;
   else
      BOOST_TEST(false);
   if (!p)
      BOOST_TEST(false);
   }
   {
   bml::unique_ptr<int[2]> p;
   if (!p)
      ;
   else
      BOOST_TEST(false);
   if (p)
      BOOST_TEST(false);
   }
   //Explicit conversions and logical operators
   {
   bml::unique_ptr<int> p(new int(3)), p2;
   BOOST_TEST(static_cast<bool>(p));
   BOOST_TEST(!static_cast<bool>(p2));
   BOOST_TEST(p && !p2);
   BOOST_TEST(p2 || p);
   BOOST_TEST(p ? true : false);
   }
   #if !defined(BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS)
   //The conversion to bool is explicit
   BOOST_MOVE_STATIC_ASSERT((!::boost::move_detail::is_convertible<bml::unique_ptr<int>, bool>::value));
   BOOST_MOVE_STATIC_ASSERT((!::boost::move_detail::is_convertible<bml::unique_ptr<int[]>, bool>::value));
   #endif
}

}  //namespace unique_ptr_observers_explicit_bool{

////////////////////////////////
//   unique_ptr_observers_get
////////////////////////////////

namespace unique_ptr_observers_get{

void test()
{
   //Single unique_ptr
   {
   int* p = new int;
   bml::unique_ptr<int> s(p);
   BOOST_TEST(s.get() == p);
   }
   //Unbounded array unique_ptr
   {
   int* p = new int[2];
   bml::unique_ptr<int[]> s(p);
   BOOST_TEST(s.get() == p);
   }
   {
   int* p = new int[2];
   bml::unique_ptr<int[2]> s(p);
   BOOST_TEST(s.get() == p);
   }
   //Bounded array unique_ptr
   {
   int *pi = new int[2];
   pi[0] = 3;
   pi[1] = 4;
   bml::unique_ptr<int[2]> p(pi);
   BOOST_TEST(p[0] == 3);
   BOOST_TEST(p[1] == 4);
   }
}

}  //namespace unique_ptr_observers_get{

////////////////////////////////
//   unique_ptr_observers_get_deleter
////////////////////////////////

namespace unique_ptr_observers_get_deleter{

struct Deleter
{
   void operator()(void*) {}

   int test() {return 5;}
   int test() const {return 6;}
};

void test()
{
   //Single unique_ptr
   {
   bml::unique_ptr<int, Deleter> p;
   BOOST_TEST(p.get_deleter().test() == 5);
   }
   {
   const bml::unique_ptr<int, Deleter> p;
   BOOST_TEST(p.get_deleter().test() == 6);
   }
   //Unbounded array unique_ptr
   {
   bml::unique_ptr<int[], Deleter> p;
   BOOST_TEST(p.get_deleter().test() == 5);
   }
   {
   const bml::unique_ptr<int[], Deleter> p;
   BOOST_TEST(p.get_deleter().test() == 6);
   }
   //Bounded array unique_ptr
   {
   bml::unique_ptr<int[2], Deleter> p;
   BOOST_TEST(p.get_deleter().test() == 5);
   }
   {
   const bml::unique_ptr<int[2], Deleter> p;
   BOOST_TEST(p.get_deleter().test() == 6);
   }
}

}  //namespace unique_ptr_observers_get_deleter{

////////////////////////////////
//   unique_ptr_observers_op_arrow
////////////////////////////////

namespace unique_ptr_observers_op_arrow{

void test()
{
   //Single unique_ptr
   {
   bml::unique_ptr<A> p(new A);
   BOOST_TEST(p->state_ == 999);
   }
}

}  //namespace unique_ptr_observers_op_arrow{


namespace unique_ptr_observers_op_index{

void test()
{
   //Unbounded array unique_ptr
   {
   A *pa = new A[2];
   //pa[0] is left default constructed
   pa[1].set(888);
   bml::unique_ptr<A[]> p(pa);
   BOOST_TEST(p[0].state_ == 999);
   BOOST_TEST(p[1].state_ == 888);
   }
   //Bounded array unique_ptr
   {
   A *pa = new A[2];
   //pa[0] is left default constructed
   pa[1].set(888);
   bml::unique_ptr<A[2]> p(pa);
   BOOST_TEST(p[0].state_ == 999);
   BOOST_TEST(p[1].state_ == 888);
   }
}

}  //namespace unique_ptr_observers_op_index{

////////////////////////////////
//             main
////////////////////////////////
int main()
{
   //Observers
   unique_ptr_observers_dereference::test();
   unique_ptr_observers_explicit_bool::test();
   unique_ptr_observers_get::test();
   unique_ptr_observers_get_deleter::test();
   unique_ptr_observers_op_arrow::test();
   unique_ptr_observers_op_index::test();

   //Test results
   return boost::report_errors();

}

#include "unique_ptr_test_utils_end.hpp"
