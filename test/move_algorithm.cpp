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

#include <boost/move/algorithm.hpp>
#include <boost/container/vector.hpp>
#include <boost/core/lightweight_test.hpp>
#include "../example/movable.hpp"

//Counts the live objects and the copy assignments, and can throw in the copy constructor
struct counted
{
   static int live;
   static int assignments;
   static int throw_at;   //copy constructions left before throwing (<0: never)

   int v;
   explicit counted(int i = 0) : v(i) { ++live; }
   counted(const counted &o) : v(o.v)
   {
      #ifndef BOOST_NO_EXCEPTIONS
      if (throw_at == 0){
         throw_at = -1;
         throw 1;
      }
      #endif
      if (throw_at > 0)
         --throw_at;
      ++live;
   }
   counted &operator=(const counted &o) { v = o.v; ++assignments; return *this; }
   ~counted() { --live; }
};

int counted::live = 0;
int counted::assignments = 0;
int counted::throw_at = -1;

//Trivially copy constructible, but not assignable
struct not_assignable
{
   int v;

   BOOST_DELETED_FUNCTION(not_assignable &operator=(const not_assignable &))
};

namespace bc = ::boost::container;

////////////////////////////////
//          move
////////////////////////////////
void test_move()
{
   //Default construct 10 movable objects
   bc::vector<movable> v(10);
   bc::vector<movable> v2(10);

   //Move to v2
   BOOST_TEST(boost::move(v.begin(), v.end(), v2.begin()) == v2.end());

   //Test values have been moved
   BOOST_TEST(v[0].moved());
   BOOST_TEST(v2.size() == 10u);
   BOOST_TEST(!v2[0].moved());
}

////////////////////////////////
//          move_backward
////////////////////////////////
void test_move_backward()
{
   bc::vector<movable> v(10);
   bc::vector<movable> v2(10);

   //Move to v from the end
   BOOST_TEST(boost::move_backward(v2.begin(), v2.end(), v.end()) == v.begin());

   //Test values have been moved
   BOOST_TEST(v2[1].moved());
   BOOST_TEST(v.size() == 10u);
   BOOST_TEST(!v[1].moved());
}

////////////////////////////////
//          copy_or_move
////////////////////////////////
void test_copy_or_move()
{
   //Trivially copyable: pointers (also from const) and an empty range
   int src[5] = { 1, 2, 3, 4, 5 };
   int dst[5] = { 0, 0, 0, 0, 0 };
   BOOST_TEST(boost::copy_or_move(&src[0], &src[0] + 5, &dst[0]) == &dst[0] + 5);
   for (int i = 0; i != 5; ++i)
      BOOST_TEST(dst[i] == i + 1);
   const int *csrc = &src[0];
   int dst2[5] = { 0, 0, 0, 0, 0 };
   BOOST_TEST(boost::copy_or_move(csrc + 1, csrc + 4, &dst2[0]) == &dst2[0] + 3);
   BOOST_TEST(dst2[0] == 2);
   BOOST_TEST(dst2[2] == 4);
   BOOST_TEST(dst2[3] == 0);
   BOOST_TEST(boost::copy_or_move(&src[0], &src[0], &dst2[0]) == &dst2[0]);

   //Not trivially copyable: the elements are assigned
   counted csrc2[3] = { counted(7), counted(8), counted(9) };
   counted cdst[3];
   counted::assignments = 0;
   BOOST_TEST(boost::copy_or_move(&csrc2[0], &csrc2[0] + 3, &cdst[0]) == &cdst[0] + 3);
   BOOST_TEST(counted::assignments == 3);
   BOOST_TEST(cdst[2].v == 9);

   //Iterators that are not pointers
   bc::vector<int> vsrc(&src[0], &src[0] + 5), vdst(5);
   BOOST_TEST(boost::copy_or_move(vsrc.begin(), vsrc.end(), vdst.begin()) == vdst.end());
   BOOST_TEST(vdst[4] == 5);
}

////////////////////////////////
//    uninitialized_copy_or_move
////////////////////////////////
void test_uninitialized_copy_or_move()
{
   //Trivially copyable
   int src[4] = { 1, 2, 3, 4 };
   int dst[4];
   BOOST_TEST(boost::uninitialized_copy_or_move(&src[0], &src[0] + 4, &dst[0]) == &dst[0] + 4);
   BOOST_TEST(dst[0] == 1);
   BOOST_TEST(dst[3] == 4);

   //Trivially copy constructible, but not assignable
   not_assignable cm[2] = { { 5 }, { 6 } };
   typedef boost::move_detail::aligned_storage<sizeof(not_assignable) * 2>::type storage_t;
   storage_t storage;
   not_assignable *const cmdst = static_cast<not_assignable*>(static_cast<void*>(&storage));
   BOOST_TEST(boost::uninitialized_copy_or_move(&cm[0], &cm[0] + 2, cmdst) == cmdst + 2);
   BOOST_TEST(cmdst[0].v == 5);
   BOOST_TEST(cmdst[1].v == 6);

   //Not trivially copyable: the elements are constructed, and destroyed if a construction throws
   {
      counted csrc[4] = { counted(1), counted(2), counted(3), counted(4) };
      typedef boost::move_detail::aligned_storage<sizeof(counted) * 4>::type cstorage_t;
      cstorage_t cstorage;
      counted *const cdst = static_cast<counted*>(static_cast<void*>(&cstorage));
      const int live = counted::live;
      BOOST_TEST(boost::uninitialized_copy_or_move(&csrc[0], &csrc[0] + 4, cdst) == cdst + 4);
      BOOST_TEST(counted::live == live + 4);
      BOOST_TEST(cdst[3].v == 4);
      for (int i = 0; i != 4; ++i)
         cdst[i].~counted();
      #ifndef BOOST_NO_EXCEPTIONS
      counted::throw_at = 2;   //the third copy construction throws
      bool thrown = false;
      try{
         boost::uninitialized_copy_or_move(&csrc[0], &csrc[0] + 4, cdst);
      }
      catch(int){
         thrown = true;
      }
      BOOST_TEST(thrown);
      //The two constructed objects were destroyed
      BOOST_TEST(counted::live == live);
      #endif
   }
   BOOST_TEST(counted::live == 0);
}

int main()
{
   test_move();
   test_move_backward();
   test_copy_or_move();
   test_uninitialized_copy_or_move();
   return boost::report_errors();
}
