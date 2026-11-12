//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2014-2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/move/adl_move_swap.hpp>
#include <boost/move/core.hpp>
#include <boost/core/lightweight_test.hpp>
#include <vector>
#include <iterator>
#include <cstddef>

class swap_stats
{
   public:
   static void reset_stats()
   {
      member_swap_calls = 0;
      friend_swap_calls = 0;
      move_cnstor_calls = 0;
      move_assign_calls = 0;
      copy_cnstor_calls = 0;
      copy_assign_calls = 0;
   }

   static unsigned int member_swap_calls;
   static unsigned int friend_swap_calls;
   static unsigned int move_cnstor_calls;
   static unsigned int move_assign_calls;
   static unsigned int copy_cnstor_calls;
   static unsigned int copy_assign_calls;
};

unsigned int swap_stats::member_swap_calls = 0;
unsigned int swap_stats::friend_swap_calls = 0;
unsigned int swap_stats::move_cnstor_calls = 0;
unsigned int swap_stats::move_assign_calls = 0;
unsigned int swap_stats::copy_cnstor_calls = 0;
unsigned int swap_stats::copy_assign_calls = 0;

class movable : public swap_stats
{
   BOOST_MOVABLE_BUT_NOT_COPYABLE(movable)
   public:
   movable()                                 {}
   movable(BOOST_RV_REF(movable))            { ++move_cnstor_calls; }
   movable & operator=(BOOST_RV_REF(movable)){ ++move_assign_calls; return *this; }
   friend void swap(movable &, movable &)    { ++friend_swap_calls; }
};

class movable_swap_member : public swap_stats
{
   BOOST_MOVABLE_BUT_NOT_COPYABLE(movable_swap_member)
   public:
   movable_swap_member()                                             {}
   movable_swap_member(BOOST_RV_REF(movable_swap_member))            { ++move_cnstor_calls; }
   movable_swap_member & operator=(BOOST_RV_REF(movable_swap_member)){ ++move_assign_calls; return *this; }
   void swap(movable_swap_member &)                                  { ++member_swap_calls; }
   friend void swap(movable_swap_member &, movable_swap_member &)    { ++friend_swap_calls; }
};

//Movable class with a member swap, but no swap found by ADL
class movable_only_member_swap : public swap_stats
{
   BOOST_MOVABLE_BUT_NOT_COPYABLE(movable_only_member_swap)
   public:
   movable_only_member_swap()                                                  {}
   movable_only_member_swap(BOOST_RV_REF(movable_only_member_swap))            { ++move_cnstor_calls; }
   movable_only_member_swap & operator=(BOOST_RV_REF(movable_only_member_swap)){ ++move_assign_calls; return *this; }
   void swap(movable_only_member_swap &)                                       { ++member_swap_calls; }
};

class copyable : public swap_stats
{
   public:
   copyable()                                {}
   copyable(const copyable &)                { ++copy_cnstor_calls; }
   copyable & operator=(const copyable&)     { ++copy_assign_calls; return *this; }
   void swap(copyable &)                     { ++member_swap_calls; }
   friend void swap(copyable &, copyable &)  { ++friend_swap_calls; }
};

class no_swap : public swap_stats
{
   private: unsigned m_state;
   public:
   explicit no_swap(unsigned i): m_state(i){}
   no_swap(const no_swap &x)               { m_state = x.m_state; ++copy_cnstor_calls; }
   no_swap & operator=(const no_swap& x)   { m_state = x.m_state; ++copy_assign_calls; return *this; }
   void swap(no_swap &)                    { ++member_swap_calls; }
   friend bool operator==(const no_swap &x, const no_swap &y) {  return x.m_state == y.m_state; }
   friend bool operator!=(const no_swap &x, const no_swap &y) {  return !(x==y); }
};

namespace adl_ns {

//A pointer to this class has a swap found by ADL, it must be called
struct ptr_swap_class {};

//A class with no swap found by ADL, swapped with moves
struct value_class
{
   int i;
};

unsigned int ptr_swap_calls = 0;

void swap(ptr_swap_class *&x, ptr_swap_class *&y)
{
   ptr_swap_class *const t = x; x = y; y = t;
   ++ptr_swap_calls;
}

enum enum_type { enum_a, enum_b };

//A base class with a swap found by ADL
struct base_class
{
   int b;
};

unsigned int base_swap_calls = 0;

void swap(base_class &x, base_class &y)
{
   const int t = x.b; x.b = y.b; y.b = t;
   ++base_swap_calls;
}

//The swap of the base class only swaps the base part. The swap idiom
//("using std::swap; swap(x, y);") selects std::swap, so the class is swapped with moves
struct derived_class : base_class
{
   int d;
};

//A proxy to a bit and an iterator that returns the proxy by value (like std::vector<bool>::iterator)
struct bit_proxy
{
   unsigned *p;
   unsigned mask;
   operator bool() const { return (*p & mask) != 0; }
};

unsigned int bit_proxy_swap_calls = 0;

void swap(bit_proxy x, bit_proxy y)
{
   const bool bx = x, by = y;
   *x.p = by ? (*x.p | x.mask) : (*x.p & ~x.mask);
   *y.p = bx ? (*y.p | y.mask) : (*y.p & ~y.mask);
   ++bit_proxy_swap_calls;
}

struct bit_iterator
{
   typedef std::random_access_iterator_tag   iterator_category;
   typedef bool                              value_type;
   typedef std::ptrdiff_t                    difference_type;
   typedef bit_proxy *                       pointer;
   typedef bit_proxy                         reference;

   unsigned *p;
   unsigned bit;

   reference operator*() const { bit_proxy r = { p, 1u << bit }; return r; }
   bit_iterator &operator++() { ++bit; return *this; }
   bit_iterator &operator--() { --bit; return *this; }
   friend bool operator==(const bit_iterator &a, const bit_iterator &b) { return a.bit == b.bit; }
   friend bool operator!=(const bit_iterator &a, const bit_iterator &b) { return a.bit != b.bit; }
};

}  //namespace adl_ns {

//Iterators that return a proxy by value call the swap found by ADL with the proxies
void test_iter_swap_proxy()
{
   {
      unsigned bits = 1u;  //bit 0 set
      adl_ns::bit_iterator b0 = { &bits, 0 }, b1 = { &bits, 1 }, b2 = { &bits, 2 }, b4 = { &bits, 4 };
      ::boost::adl_move_iter_swap(b0, b1);
      BOOST_TEST(bits == 2u);
      BOOST_TEST(adl_ns::bit_proxy_swap_calls == 1);
      //[b0, b2) <-> [b2, b4): bits 0 and 1 are swapped with bits 2 and 3
      BOOST_TEST(::boost::adl_move_swap_ranges(b0, b2, b2) == b4);
      BOOST_TEST(bits == 8u);
      BOOST_TEST(adl_ns::bit_proxy_swap_calls == 3);
      //[b2, b4) <-> [b0, b2) backwards
      BOOST_TEST(::boost::adl_move_swap_ranges_backward(b2, b4, b2) == b0);
      BOOST_TEST(bits == 2u);
      BOOST_TEST(adl_ns::bit_proxy_swap_calls == 5);
   }
   {
      std::vector<bool> v;
      v.push_back(true);
      v.push_back(false);
      ::boost::adl_move_iter_swap(v.begin(), v.begin() + 1);
      BOOST_TEST(!v[0] && v[1]);
      ::boost::adl_move_swap_ranges(v.begin(), v.begin() + 1, v.begin() + 1);
      BOOST_TEST(v[0] && !v[1]);
   }
   {  //Iterators that return a reference swap the objects
      int a[2] = { 1, 2 };
      ::boost::adl_move_iter_swap(&a[0], &a[1]);
      BOOST_TEST(a[0] == 2 && a[1] == 1);
   }
}

#if defined(BOOST_MOVE_HAS_CXX20_CONSTEXPR)

//Types with no swap found by ADL are swapped in constant expressions, as std::swap is not called
//(it is not constexpr in some C++20 standard libraries)
constexpr bool test_constexpr_swap()
{
   int i = 1, j = 2;
   ::boost::adl_move_swap(i, j);
   int *pi = &i, *pj = &j;
   ::boost::adl_move_swap(pi, pj);
   adl_ns::enum_type ea = adl_ns::enum_a, eb = adl_ns::enum_b;
   ::boost::adl_move_swap(ea, eb);
   adl_ns::value_class va = {1}, vb = {2};
   ::boost::adl_move_swap(va, vb);
   int ia[2] = {1, 2}, ib[2] = {3, 4};
   ::boost::adl_move_swap(ia, ib);
   return i == 2 && j == 1 && pi == &j && pj == &i && ea == adl_ns::enum_b && eb == adl_ns::enum_a &&
          va.i == 2 && vb.i == 1 && ia[0] == 3 && ia[1] == 4 && ib[0] == 1 && ib[1] == 2;
}

BOOST_MOVE_STATIC_ASSERT(test_constexpr_swap());

#endif   //#if defined(BOOST_MOVE_HAS_CXX20_CONSTEXPR)


int main()
{
   {  //movable
      movable x, y;
      swap_stats::reset_stats();
      ::boost::adl_move_swap(x, y);
      //This should call friend swap via ADL (in all C++ standards)
      BOOST_TEST(swap_stats::friend_swap_calls == 1);
      BOOST_TEST(swap_stats::member_swap_calls == 0);
      BOOST_TEST(swap_stats::member_swap_calls == 0);
      BOOST_TEST(swap_stats::move_cnstor_calls == 0);
      BOOST_TEST(swap_stats::move_assign_calls == 0);
      BOOST_TEST(swap_stats::copy_cnstor_calls == 0);
      BOOST_TEST(swap_stats::copy_assign_calls == 0);
   }
   {  //movable_swap_member
      movable_swap_member x, y;
      swap_stats::reset_stats();
      ::boost::adl_move_swap(x, y);
      //This should call friend swap via ADL (in all C++ standards)
      BOOST_TEST(swap_stats::friend_swap_calls == 1);
      BOOST_TEST(swap_stats::member_swap_calls == 0);
      BOOST_TEST(swap_stats::move_cnstor_calls == 0);
      BOOST_TEST(swap_stats::move_assign_calls == 0);
      BOOST_TEST(swap_stats::copy_cnstor_calls == 0);
      BOOST_TEST(swap_stats::copy_assign_calls == 0);
   }
   {  //movable_only_member_swap
      movable_only_member_swap x, y;
      swap_stats::reset_stats();
      ::boost::adl_move_swap(x, y);
      //No swap found by ADL: move-based swap (in all C++ standards), the member swap is not called
      BOOST_TEST(swap_stats::friend_swap_calls == 0);
      BOOST_TEST(swap_stats::member_swap_calls == 0);
      BOOST_TEST(swap_stats::move_cnstor_calls == 1);
      BOOST_TEST(swap_stats::move_assign_calls == 2);
      BOOST_TEST(swap_stats::copy_cnstor_calls == 0);
      BOOST_TEST(swap_stats::copy_assign_calls == 0);
   }
   {  //copyable
      copyable x, y;
      swap_stats::reset_stats();
      ::boost::adl_move_swap(x, y);
      //This should call friend swap via ADL
      BOOST_TEST(swap_stats::friend_swap_calls == 1);
      BOOST_TEST(swap_stats::member_swap_calls == 0);
      BOOST_TEST(swap_stats::move_cnstor_calls == 0);
      BOOST_TEST(swap_stats::move_assign_calls == 0);
      BOOST_TEST(swap_stats::copy_cnstor_calls == 0);
      BOOST_TEST(swap_stats::copy_assign_calls == 0);
   }
   {  //no_swap
      no_swap x(1), y(2), x_back(x), y_back(y);
      swap_stats::reset_stats();
      ::boost::adl_move_swap(x, y);
      //No swap found by ADL: move-based swap, which uses copies (the member swap is not called)
      BOOST_TEST(swap_stats::friend_swap_calls == 0);
      BOOST_TEST(swap_stats::member_swap_calls == 0);
      BOOST_TEST(swap_stats::move_cnstor_calls == 0);
      BOOST_TEST(swap_stats::move_assign_calls == 0);
      BOOST_TEST(swap_stats::copy_cnstor_calls == 1);
      BOOST_TEST(swap_stats::copy_assign_calls == 2);
      BOOST_TEST(x == y_back);
      BOOST_TEST(y == x_back);
      BOOST_TEST(x != y);
   }
   {  //scalar types
      int i = 1, j = 2;
      ::boost::adl_move_swap(i, j);
      BOOST_TEST(i == 2 && j == 1);
      int *pi = &i, *pj = &j;
      ::boost::adl_move_swap(pi, pj);
      BOOST_TEST(pi == &j && pj == &i);
      adl_ns::enum_type ea = adl_ns::enum_a, eb = adl_ns::enum_b;
      ::boost::adl_move_swap(ea, eb);
      BOOST_TEST(ea == adl_ns::enum_b && eb == adl_ns::enum_a);
   }
   {  //pointer with a swap found by ADL
      adl_ns::ptr_swap_class a, b;
      adl_ns::ptr_swap_class *pa = &a, *pb = &b;
      ::boost::adl_move_swap(pa, pb);
      BOOST_TEST(adl_ns::ptr_swap_calls == 1);
      BOOST_TEST(pa == &b && pb == &a);
   }
   {  //derived class of a class with a swap found by ADL: move-based swap of the whole object
      adl_ns::derived_class x, y;
      x.b = 1; x.d = 10;
      y.b = 2; y.d = 20;
      ::boost::adl_move_swap(x, y);
      BOOST_TEST(adl_ns::base_swap_calls == 0);
      BOOST_TEST(x.b == 2 && x.d == 20);
      BOOST_TEST(y.b == 1 && y.d == 10);
      //The base class uses its swap
      ::boost::adl_move_swap(static_cast<adl_ns::base_class&>(x), static_cast<adl_ns::base_class&>(y));
      BOOST_TEST(adl_ns::base_swap_calls == 1);
      BOOST_TEST(x.b == 1 && x.d == 20);
      BOOST_TEST(y.b == 2 && y.d == 10);
   }
   test_iter_swap_proxy();
   #if defined(BOOST_MOVE_HAS_CXX20_CONSTEXPR)
   BOOST_TEST(test_constexpr_swap());
   #endif
   return ::boost::report_errors();
}
