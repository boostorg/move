//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright David Abrahams, Vicente Botet, Ion Gaztanaga 2009.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/move/iterator.hpp>
#include <boost/move/detail/meta_utils.hpp>
#include <boost/container/vector.hpp>
#include <boost/core/lightweight_test.hpp>
#include "../example/movable.hpp"
#include "../example/copymovable.hpp"
#include <vector>
#include <cstddef>
#include <iterator>

//Iterator whose operator* returns a proxy by value
struct proxy_iterator
{
   struct proxy
   {
      bool *p;
      operator bool() const { return *p; }
   };

   typedef bool                              value_type;
   typedef bool*                             pointer;
   typedef proxy                             reference;
   typedef std::ptrdiff_t                    difference_type;
   typedef std::random_access_iterator_tag   iterator_category;

   bool *p;
   reference operator*() const                     { proxy r = { p };     return r; }
   reference operator[](difference_type n) const   { proxy r = { p + n }; return r; }
};

//Checks the reference type of move_iterator
void test_reference_type()
{
   using ::boost::move_detail::is_same;
   #if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
   BOOST_MOVE_STATIC_ASSERT((is_same<boost::move_iterator<int*>::reference, int&&>::value));
   BOOST_MOVE_STATIC_ASSERT((is_same<boost::move_iterator<const int*>::reference, const int&&>::value));
   BOOST_MOVE_STATIC_ASSERT((is_same<boost::move_iterator<std::vector<int>::const_iterator>::reference, const int&&>::value));
   BOOST_MOVE_STATIC_ASSERT((is_same<boost::move_iterator<boost::move_iterator<int*> >::reference, int&&>::value));
   #else
   BOOST_MOVE_STATIC_ASSERT((is_same<boost::move_iterator<movable*>::reference, ::boost::rv<movable>&>::value));
   BOOST_MOVE_STATIC_ASSERT((is_same<boost::move_iterator<const copy_movable*>::reference, const copy_movable&>::value));
   BOOST_MOVE_STATIC_ASSERT((is_same<boost::move_iterator<int*>::reference, int&>::value));
   #endif
   //A proxy is returned by value
   BOOST_MOVE_STATIC_ASSERT((is_same<boost::move_iterator<proxy_iterator>::reference, proxy_iterator::proxy>::value));
}

//Elements of a non-const range are moved, and the returned references
//refer to the elements (not to temporaries)
void test_non_const_iterator()
{
   movable a[2];
   boost::move_iterator<movable*> it(a);
   movable m(*it);
   BOOST_TEST(a[0].moved() && !m.moved());
   movable m2(it[1]);
   BOOST_TEST(a[1].moved() && !m2.moved());

   int ia[3] = { 1, 2, 3 };
   boost::move_iterator<int*> ii(ia);
   #if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
   int&& r0 = *ii;
   int&& r2 = ii[2];
   BOOST_TEST(&r0 == &ia[0]);
   BOOST_TEST(&r2 == &ia[2]);
   //Nested move_iterator: the base reference is already an rvalue reference
   boost::move_iterator<boost::move_iterator<int*> > nit(ii);
   int&& n1 = nit[1];
   BOOST_TEST(&n1 == &ia[1]);
   #else
   BOOST_TEST(&*ii == &ia[0]);
   BOOST_TEST(&ii[2] == &ia[2]);
   #endif
}

//Elements of a const range can not be moved, so they are copied
void test_const_iterator()
{
   const copy_movable a[2];
   boost::move_iterator<const copy_movable*> it(a);
   copy_movable c(*it);
   BOOST_TEST(!a[0].moved() && !c.moved());
   copy_movable c2(it[1]);
   BOOST_TEST(!a[1].moved() && !c2.moved());

   const int ia[2] = { 4, 5 };
   boost::move_iterator<const int*> ii(ia);
   BOOST_TEST(*ii == 4 && ii[1] == 5);
   #if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
   const int&& r1 = ii[1];
   BOOST_TEST(&r1 == &ia[1]);
   #else
   BOOST_TEST(&ii[1] == &ia[1]);
   #endif

   std::vector<int> v(3, 7);
   const std::vector<int>& cv = v;
   std::vector<int> v2(boost::make_move_iterator(cv.begin()), boost::make_move_iterator(cv.end()));
   BOOST_TEST(v2.size() == 3u && v2[2] == 7 && v[2] == 7);
}

//A proxy returned by value is returned by value, so it does not dangle
void test_proxy_iterator()
{
   bool b[2] = { true, false };
   proxy_iterator pi = { b };
   boost::move_iterator<proxy_iterator> mi(pi);
   BOOST_TEST(bool(*mi) == true);
   BOOST_TEST(bool(mi[1]) == false);

   std::vector<bool> vb(4, true);
   vb[2] = false;
   boost::move_iterator<std::vector<bool>::iterator> mvb(vb.begin());
   BOOST_TEST(bool(*mvb) == true);
   BOOST_TEST(bool(mvb[2]) == false);
}

//Conversion and comparison between move iterators of different types
void test_conversion_and_mixed_comparison()
{
   using ::boost::move_detail::is_convertible;
   typedef boost::move_iterator<int*>        mit;
   typedef boost::move_iterator<const int*>  cmit;
   BOOST_MOVE_STATIC_ASSERT((is_convertible<mit, cmit>::value));
   BOOST_MOVE_STATIC_ASSERT(!(is_convertible<cmit, mit>::value));

   int a[3] = { 1, 2, 3 };
   mit  b(a), e(a + 3);
   cmit cb(b);
   BOOST_TEST(cb.base() == a);
   BOOST_TEST(*cb == 1);
   BOOST_TEST(cb == b);
   BOOST_TEST(b == cb);
   BOOST_TEST(!(cb != b));
   BOOST_TEST(cb < e);
   BOOST_TEST(cb <= e);
   BOOST_TEST(e > cb);
   BOOST_TEST(e >= cb);
   BOOST_TEST(e - cb == 3);
   BOOST_TEST(cb - e == -3);
}

int main()
{
   test_reference_type();
   test_non_const_iterator();
   test_const_iterator();
   test_proxy_iterator();
   test_conversion_and_mixed_comparison();

   namespace bc = ::boost::container;
   //Default construct 10 movable objects
   bc::vector<movable> v(10);

   //Test default constructed value
   BOOST_TEST(!v[0].moved());

   //Move values
   bc::vector<movable> v2
      (boost::make_move_iterator(v.begin()), boost::make_move_iterator(v.end()));

   //Test values have been moved
   BOOST_TEST(v[0].moved());
   BOOST_TEST(v2.size() == 10);

   //Move again
   v.assign(boost::make_move_iterator(v2.begin()), boost::make_move_iterator(v2.end()));

   //Test values have been moved
   BOOST_TEST(v2[0].moved());
   BOOST_TEST(!v[0].moved());

   return ::boost::report_errors();
}
