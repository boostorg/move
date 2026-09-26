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
#include <boost/move/make_unique.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/core/lightweight_test.hpp>

namespace bml = ::boost::movelib;

//A deleter with a constexpr default constructor, but a call operator that is not constexpr.
//A unique_ptr that owns nothing never calls the deleter, so it is still usable
//in constant initialization and (C++20) constant destruction
struct nonconstexpr_call_deleter
{
   BOOST_CONSTEXPR nonconstexpr_call_deleter() : state(0) {}
   void operator()(int *p) const { delete p; }
   int state;
};

////////////////////////////////
//   C++11: the default and nullptr constructors are constexpr, so unique_ptr
//   objects with static storage duration are constant-initialized
////////////////////////////////
namespace constinit_test{

#if defined(BOOST_MOVE_TEST_CONSTINIT)

BOOST_MOVE_TEST_CONSTINIT bml::unique_ptr<int> g_single;
BOOST_MOVE_TEST_CONSTINIT bml::unique_ptr<int> g_single_nullptr(nullptr);
BOOST_MOVE_TEST_CONSTINIT bml::unique_ptr<int[]> g_array;
BOOST_MOVE_TEST_CONSTINIT bml::unique_ptr<int, nonconstexpr_call_deleter> g_custom_deleter;

#endif   //#if defined(BOOST_MOVE_TEST_CONSTINIT)

void test()
{
   #if defined(BOOST_MOVE_TEST_CONSTINIT)
   BOOST_TEST(!g_single && !g_single_nullptr && !g_array && !g_custom_deleter);
   #endif
}

}  //namespace constinit_test{

////////////////////////////////
//   C++20: with BOOST_MOVE_HAS_CXX20_CONSTEXPR unique_ptr, default_delete and make_unique can
//   be used in constant expressions. Tested both at compile-time and runtime
////////////////////////////////
namespace cxx20_constexpr_test{

#if defined(BOOST_MOVE_HAS_CXX20_CONSTEXPR)

//A unique_ptr that owns nothing can be a constexpr variable (requires constant destruction)
struct constexpr_empty_deleter
{
   constexpr constexpr_empty_deleter() : state(0) {}
   constexpr void operator()(int *p) const { delete p; }
   int state;
};

constexpr bml::unique_ptr<int> g_single;
constexpr bml::unique_ptr<int> g_single_nullptr(nullptr);
constexpr bml::unique_ptr<int[]> g_array;
constexpr bml::unique_ptr<int, constexpr_empty_deleter> g_custom_deleter;
constexpr bml::unique_ptr<int, nonconstexpr_call_deleter> g_nonconstexpr_call_deleter;

//Empty unique_ptr objects created and destroyed in a constant expression
template<class T, class D>
constexpr bool create_and_destroy()
{
   bml::unique_ptr<T, D> p;
   bml::unique_ptr<T, D> q(nullptr);
   return !p && !q;
}

//Owning a single object: construction, observers, reset and release
constexpr bool test_single()
{
   bml::unique_ptr<int> p(new int(42));
   bool ok = p && p.get() != nullptr && *p == 42;
   *p = 11;
   ok = ok && *p == 11;
   p.reset(new int(17));         //deletes 11
   ok = ok && *p == 17;
   int *raw = p.release();
   ok = ok && !p && *raw == 17;     //releases 17
   delete raw;
   p.reset(new int(13));         //replaces 17 with 13
   p.reset();                       //deletes 13
   ok = ok && !p;
   p.reset(new int(19));
   p = nullptr;                     //deletes 19
   return ok && !p;
}

//Owning an array
constexpr bool test_array()
{
   bml::unique_ptr<int[]> a(new int[3]{1, 2, 3});
   bool ok = a[0] == 1 && a[1] == 2 && a[2] == 3;
   a[1] = 20;
   ok = ok && a[1] == 20;
   a.reset(new int[2]{4, 5});
   return ok && a[0] == 4 && a[1] == 5;
}

//Value-initialized arrays ("new T[n]()", used by make_unique<T[]>). Only constexpr
//for compilers that support them (see BOOST_MOVE_HAS_CXX20_CONSTEXPR_ARRAY_VINIT)
BOOST_MOVE_CXX20_CONSTEXPR_ARRAY_VINIT bool test_array_value_init()
{
   bml::unique_ptr<int[]> a(new int[2]());
   bml::unique_ptr<int[]> m = bml::make_unique<int[]>(4);
   return a[0] == 0 && a[1] == 0 && m[0] == 0 && m[3] == 0;
}

//Move construction, move assignment and swap
constexpr bool test_move_and_swap()
{
   bml::unique_ptr<int> p(new int(5));
   bml::unique_ptr<int> q(boost::move(p));
   bool ok = !p && *q == 5;
   bml::unique_ptr<int> r;
   r = boost::move(q);              //r owned nothing
   ok = ok && !q && *r == 5;
   bml::unique_ptr<int> s(new int(6));
   r = boost::move(s);              //deletes the int owned by r
   ok = ok && !s && *r == 6;
   bml::unique_ptr<int> t(new int(7));
   r.swap(t);
   ok = ok && *r == 7 && *t == 6;
   swap(r, t);
   return ok && *r == 6 && *t == 7;
}

//Converting move construction and assignment through a constexpr virtual destructor.
//GCC 10 can't evaluate delete through a virtual destructor in a constant expression
#if !(defined(BOOST_GCC) && (BOOST_GCC < 110000))
#define BOOST_MOVE_TEST_CONSTEXPR_VIRTUAL_DTOR
#endif

struct base
{
   constexpr base(int v) : value(v) {}
   constexpr virtual ~base() {}
   constexpr virtual int get() const { return value; }
   int value;
};

struct derived : base
{
   constexpr derived(int v) : base(v) {}
   constexpr ~derived() override {}
   constexpr int get() const override { return value * 10; }
};

constexpr bool test_conversion()
{
   bml::unique_ptr<derived> d(new derived(4));
   bml::unique_ptr<base> b(boost::move(d));     //deleted through base with a virtual destructor
   bool ok = !d && b->get() == 40;
   bml::unique_ptr<base> b2;
   b2 = bml::unique_ptr<derived>(new derived(5));
   ok = ok && b2->get() == 50;
   bml::unique_ptr<int> i(new int(8));
   bml::unique_ptr<const int> c(boost::move(i));
   return ok && !i && *c == 8;
}

struct coordinates
{
   constexpr coordinates(int a, int b) : x(a), y(b) {}
   int x, y;
};

constexpr bool test_make_unique()
{
   bml::unique_ptr<int> p = bml::make_unique<int>(9);
   bml::unique_ptr<coordinates> d = bml::make_unique<coordinates>(3, 4);
   bml::unique_ptr<int> u = bml::make_unique_definit<int>();
   *u = 11;
   bml::unique_ptr<int[]> ua = bml::make_unique_definit<int[]>(2);
   ua[0] = 1;
   ua[1] = 2;
   return *p == 9 && d->x == 3 && d->y == 4 && *u == 11 && ua[0] + ua[1] == 3;
}


constexpr void function_deleter(int *p) { delete p; }

struct counting_deleter
{
   constexpr counting_deleter() : count(0) {}
   constexpr void operator()(int *p) { delete p; ++count; }
   int count;
};

constexpr bool test_custom_deleter()
{
   bml::unique_ptr<int, counting_deleter> p(new int(1));
   p.reset(new int(2));
   bool ok = p.get_deleter().count == 1;
   p.reset();
   ok = ok && p.get_deleter().count == 2;

   counting_deleter d;
   {
      bml::unique_ptr<int, counting_deleter&> r(new int(3), d);
      ok = ok && &r.get_deleter() == &d;
   }
   ok = ok && d.count == 1;

   bml::unique_ptr<int, void(*)(int*)> f(new int(4), &function_deleter);
   ok = ok && *f == 4 && f.get_deleter() == &function_deleter;
   return ok;
}

//Comparisons
constexpr bool test_comparisons()
{
   bml::unique_ptr<int> e;
   bml::unique_ptr<int> p(new int(99));
   bml::unique_ptr<int> q(new int(101));
   return  e == nullptr && nullptr == e && !(e != nullptr) &&
           p != q && !(p == q) &&
           !(e < nullptr) && e <= nullptr && e >= nullptr && !(e > nullptr);
}

BOOST_MOVE_STATIC_ASSERT(!g_single && !g_single_nullptr && !g_array && !g_custom_deleter);
BOOST_MOVE_STATIC_ASSERT(!g_nonconstexpr_call_deleter);
BOOST_MOVE_STATIC_ASSERT((create_and_destroy<int, bml::default_delete<int> >()));
BOOST_MOVE_STATIC_ASSERT((create_and_destroy<int[], bml::default_delete<int[]> >()));
BOOST_MOVE_STATIC_ASSERT((create_and_destroy<int, nonconstexpr_call_deleter>()));
BOOST_MOVE_STATIC_ASSERT(test_single());
BOOST_MOVE_STATIC_ASSERT(test_array());
#if defined(BOOST_MOVE_HAS_CXX20_CONSTEXPR_ARRAY_VINIT)
BOOST_MOVE_STATIC_ASSERT(test_array_value_init());
#endif   //BOOST_MOVE_HAS_CXX20_CONSTEXPR_ARRAY_VINIT
BOOST_MOVE_STATIC_ASSERT(test_move_and_swap());
#if defined(BOOST_MOVE_TEST_CONSTEXPR_VIRTUAL_DTOR)
BOOST_MOVE_STATIC_ASSERT(test_conversion());
#endif   //BOOST_MOVE_TEST_CONSTEXPR_VIRTUAL_DTOR
BOOST_MOVE_STATIC_ASSERT(test_make_unique());
BOOST_MOVE_STATIC_ASSERT(test_custom_deleter());
BOOST_MOVE_STATIC_ASSERT(test_comparisons());

#endif   //#if defined(BOOST_MOVE_HAS_CXX20_CONSTEXPR)

void test()
{
   #if defined(BOOST_MOVE_HAS_CXX20_CONSTEXPR)
   BOOST_TEST(!g_single && !g_single_nullptr && !g_array && !g_custom_deleter);
   BOOST_TEST(!g_nonconstexpr_call_deleter);
   BOOST_TEST((create_and_destroy<int, bml::default_delete<int> >()));
   BOOST_TEST((create_and_destroy<int[], bml::default_delete<int[]> >()));
   BOOST_TEST((create_and_destroy<int, nonconstexpr_call_deleter>()));
   BOOST_TEST(test_single());
   BOOST_TEST(test_array());
   BOOST_TEST(test_array_value_init());
   BOOST_TEST(test_move_and_swap());
   BOOST_TEST(test_conversion());
   BOOST_TEST(test_make_unique());
   BOOST_TEST(test_custom_deleter());
   BOOST_TEST(test_comparisons());
   #endif
}

}  //namespace cxx20_constexpr_test{

int main()
{
   constinit_test::test();
   cxx20_constexpr_test::test();
   return boost::report_errors();
}
