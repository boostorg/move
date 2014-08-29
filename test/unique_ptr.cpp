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
#include <boost/static_assert.hpp>
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

namespace bml = ::boost::movelib;

//A deleter that can only default constructed
template <class T>
class def_constr_deleter
{
   int state_;
   def_constr_deleter(const def_constr_deleter&);
   def_constr_deleter& operator=(const def_constr_deleter&);

   public:
   typedef typename ::boost::move_detail::remove_extent<T>::type element_type;
   static const bool is_array = ::boost::move_detail::is_array<T>::value;

   def_constr_deleter() : state_(5) {}

   explicit def_constr_deleter(int s) : state_(s) {}

   int state() const {return state_;}

   void set_state(int s) {state_ = s;}

   void operator()(element_type* p) const
   {  is_array ? delete []p : delete p;   }

   void operator()(element_type* p)
   {  ++state_;   is_array ? delete []p : delete p;  }
};

//A deleter that can be copy constructed
template <class T>
class copy_constr_deleter
{
   int state_;

   public:
   typedef typename ::boost::move_detail::remove_extent<T>::type element_type;
   static const bool is_array = ::boost::move_detail::is_array<T>::value;

   copy_constr_deleter() : state_(5) {}

   template<class U>
   copy_constr_deleter(const copy_constr_deleter<U>&
      , typename boost::move_detail::enable_def_del<U, T>::type* =0)
   {  state_ = 5; }

   explicit copy_constr_deleter(int s) : state_(s) {}

   template <class U>
   typename boost::move_detail::enable_def_del<U, T, copy_constr_deleter&>::type
      operator=(const copy_constr_deleter<U> &d)
   {
      state_ = d.state();
      return *this;
   }

   int state() const          {return state_;}

   void set_state(int s)      {state_ = s;}

   void operator()(element_type* p) const
   {  is_array ? delete []p : delete p;   }

   void operator()(element_type* p)
   {  ++state_;   is_array ? delete []p : delete p;  }
};

//A deleter that can be only move constructed
template <class T>
class move_constr_deleter
{
   int state_;

   BOOST_MOVABLE_BUT_NOT_COPYABLE(move_constr_deleter)

   public:
   typedef typename ::boost::move_detail::remove_extent<T>::type element_type;
   static const bool is_array = ::boost::move_detail::is_array<T>::value;

   move_constr_deleter() : state_(5) {}

   move_constr_deleter(BOOST_RV_REF(move_constr_deleter) r)
      : state_(r.state_)
   {  r.state_ = 0;  }

   explicit move_constr_deleter(int s) : state_(s) {}

   template <class U>
   move_constr_deleter(BOOST_RV_REF(move_constr_deleter<U>) d
      , typename boost::move_detail::enable_def_del<U, T>::type* =0)
      : state_(d.state())
   { d.set_state(0);  }

   move_constr_deleter& operator=(BOOST_RV_REF(move_constr_deleter) r)
   {
      state_ = r.state_;
      r.state_ = 0;
      return *this;
   }

   template <class U>
   typename boost::move_detail::enable_def_del<U, T, move_constr_deleter&>::type
      operator=(BOOST_RV_REF(move_constr_deleter<U>) d)
   {
      state_ = d.state();
      d.set_state(0);
      return *this;
   }

   int state() const          {return state_;}

   void set_state(int s)      {state_ = s;}

   void operator()(element_type* p) const
   {  is_array ? delete []p : delete p;   }

   void operator()(element_type* p)
   {  ++state_;   is_array ? delete []p : delete p;  }

   friend bool operator==(const move_constr_deleter& x, const move_constr_deleter& y)
      {return x.state_ == y.state_;}
};

//A base class containing state with a static instance counter
struct A
{
   int state_;
   static int count;

   A()               : state_(999)      {++count;}
   explicit A(int i) : state_(i)        {++count;}
   A(const A& a)     : state_(a.state_) {++count;}
   A& operator=(const A& a) { state_ = a.state_; return *this; }
   void set(int i)   {state_ = i;}
   virtual ~A()      {--count;}
   friend bool operator==(const A& x, const A& y) { return x.state_ == y.state_; }
};

int A::count = 0;

//A class derived from A with a static instance counter
struct B
   : public A
{
   static int count;
   B() {++count;}
   B(const B&) {++count;}
   virtual ~B() {--count;}
};

int B::count = 0;

void reset_counters();

BOOST_STATIC_ASSERT((::boost::move_detail::is_convertible<B, A>::value));

//Incomplete Type
struct I;
void check(int i);
I* get();
I* get_array(int i);

template <class T, class D = bml::default_delete<T> >
struct J
{
   typedef bml::unique_ptr<T, D> unique_ptr_type;
   typedef typename unique_ptr_type::element_type element_type;
   bml::unique_ptr<T, D> a_;
   J() {}
   explicit J(element_type*a) : a_(a) {}
   ~J();

   element_type* get() const {return a_.get();}
   D& get_deleter() {return a_.get_deleter();}
};

////////////////////////////////
//       pointer_type
////////////////////////////////
namespace pointer_type {

struct Deleter
{
   struct pointer {};
};

// Test unique_ptr::pointer type
void test()
{
   //Single unique_ptr
   {
   typedef bml::unique_ptr<int> P;
   BOOST_STATIC_ASSERT((boost::move_detail::is_same<P::pointer, int*>::value));
   }
   {
   typedef bml::unique_ptr<int, Deleter> P;
   BOOST_STATIC_ASSERT((boost::move_detail::is_same<P::pointer, Deleter::pointer>::value));
   }
   //Array unique_ptr
   {
   typedef bml::unique_ptr<int[]> P;
   BOOST_STATIC_ASSERT((boost::move_detail::is_same<P::pointer, int*>::value));
   }
   {
   typedef bml::unique_ptr<int[], Deleter> P;
   BOOST_STATIC_ASSERT((boost::move_detail::is_same<P::pointer, Deleter::pointer>::value));
   }
}

}  //namespace pointer_type {

////////////////////////////////
//   unique_ptr_asgn_move_convert01
////////////////////////////////
namespace unique_ptr_asgn_move_convert01 {

void test()
{
   //Single unique_ptr
   reset_counters();
   {
   bml::unique_ptr<B> s(new B);
   A* p = s.get();
   bml::unique_ptr<A> s2(new A);
   BOOST_TEST(A::count == 2);
   s2 = boost::move(s);
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s.get() == 0);
   BOOST_TEST(A::count == 1);
   BOOST_TEST(B::count == 1);
   }
   BOOST_TEST(A::count == 0);
   BOOST_TEST(B::count == 0);

   reset_counters();
   //Array unique_ptr, only from the same CV qualified pointers
   {
   bml::unique_ptr<A[]> s(new A[2]);
   A* p = s.get();
   bml::unique_ptr<const A[]> s2(new const A[2]);
   BOOST_TEST(A::count == 4);
   s2 = boost::move(s);
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s.get() == 0);
   BOOST_TEST(A::count == 2);
   }
   BOOST_TEST(A::count == 0);
}

}  //namespace unique_ptr_asgn_move_convert01{

////////////////////////////////
//   unique_ptr_asgn_move_convert02
////////////////////////////////

namespace unique_ptr_asgn_move_convert02{

void test()
{
   //Single unique_ptr
   reset_counters();
   {
   bml::unique_ptr<B, move_constr_deleter<B> > s(new B);
   A* p = s.get();
   bml::unique_ptr<A, move_constr_deleter<A> > s2(new A);
   BOOST_TEST(A::count == 2);
   s2 = (boost::move(s));
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s.get() == 0);
   BOOST_TEST(A::count == 1);
   BOOST_TEST(B::count == 1);
   BOOST_TEST(s2.get_deleter().state() == 5);
   BOOST_TEST(s.get_deleter().state() == 0);
   }
   BOOST_TEST(A::count == 0);
   BOOST_TEST(B::count == 0);

   //Array unique_ptr, only from the same CV qualified pointers
   reset_counters();
   {
   bml::unique_ptr<A[], move_constr_deleter<A[]> > s(new A[2]);
   A* p = s.get();
   bml::unique_ptr<const A[], move_constr_deleter<const A[]> > s2(new const A[2]);
   BOOST_TEST(A::count == 4);
   s2 = (boost::move(s));
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s.get() == 0);
   BOOST_TEST(A::count == 2);
   BOOST_TEST(s2.get_deleter().state() == 5);
   BOOST_TEST(s.get_deleter().state() == 0);
   }
   BOOST_TEST(A::count == 0);
}

}  //namespace unique_ptr_asgn_move_convert02{

////////////////////////////////
//   unique_ptr_asgn_move_convert03
////////////////////////////////

namespace unique_ptr_asgn_move_convert03{

// test converting move assignment with reference deleters

void test()
{
   //Single unique_ptr
   reset_counters();
   {
   copy_constr_deleter<B> db(5);
   bml::unique_ptr<B, copy_constr_deleter<B>&> s(new B, db);
   A* p = s.get();
   copy_constr_deleter<A> da(6);
   bml::unique_ptr<A, copy_constr_deleter<A>&> s2(new A, da);
   s2 = boost::move(s);
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s.get() == 0);
   BOOST_TEST(A::count == 1);
   BOOST_TEST(B::count == 1);
   BOOST_TEST(s2.get_deleter().state() == 5);
   }
   BOOST_TEST(A::count == 0);
   BOOST_TEST(B::count == 0);

   //Array unique_ptr, only from the same CV qualified pointers
   reset_counters();
   {
   copy_constr_deleter<A[]> db(5);
   bml::unique_ptr<A[], copy_constr_deleter<A[]>&> s(new A[2], db);
   A* p = s.get();
   copy_constr_deleter<const A[]> da(6);
   bml::unique_ptr<const A[], copy_constr_deleter<const A[]>&> s2(new const A[2], da);
   BOOST_TEST(A::count == 4);
   s2 = boost::move(s);
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s.get() == 0);
   BOOST_TEST(A::count == 2);
   BOOST_TEST(s2.get_deleter().state() == 5);
   }
   BOOST_TEST(A::count == 0);
   BOOST_TEST(B::count == 0);
}

}  //namespace unique_ptr_asgn_move_convert03{

////////////////////////////////
//   unique_ptr_asgn_move01
////////////////////////////////
namespace unique_ptr_asgn_move01 {

void test()
{
   //Single unique_ptr
   reset_counters();
   {
   bml::unique_ptr<A> s1(new A);
   A* p = s1.get();
   bml::unique_ptr<A> s2(new A);
   BOOST_TEST(A::count == 2);
   s2 = boost::move(s1);
   BOOST_TEST(A::count == 1);
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s1.get() == 0);
   }
   BOOST_TEST(A::count == 0);
   {
   bml::unique_ptr<A, move_constr_deleter<A> > s1(new A);
   A* p = s1.get();
   bml::unique_ptr<A, move_constr_deleter<A> > s2(new A);
   BOOST_TEST(A::count == 2);
   s2 = boost::move(s1);
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s1.get() == 0);
   BOOST_TEST(A::count == 1);
   BOOST_TEST(s2.get_deleter().state() == 5);
   BOOST_TEST(s1.get_deleter().state() == 0);
   }
   BOOST_TEST(A::count == 0);
   {
   copy_constr_deleter<A> d1(5);
   bml::unique_ptr<A, copy_constr_deleter<A>&> s1(new A, d1);
   A* p = s1.get();
   copy_constr_deleter<A> d2(6);
   bml::unique_ptr<A, copy_constr_deleter<A>&> s2(new A, d2);
   s2 = boost::move(s1);
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s1.get() == 0);
   BOOST_TEST(A::count == 1);
   BOOST_TEST(d1.state() == 5);
   BOOST_TEST(d2.state() == 5);
   }
   BOOST_TEST(A::count == 0);

   //Array unique_ptr
   reset_counters();
   {
   bml::unique_ptr<A[]> s1(new A[2]);
   A* p = s1.get();
   bml::unique_ptr<A[]> s2(new A[2]);
   BOOST_TEST(A::count == 4);
   s2 = boost::move(s1);
   BOOST_TEST(A::count == 2);
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s1.get() == 0);
   }
   BOOST_TEST(A::count == 0);
   {
   bml::unique_ptr<A[], move_constr_deleter<A[]> > s1(new A[2]);
   A* p = s1.get();
   bml::unique_ptr<A[], move_constr_deleter<A[]> > s2(new A[2]);
   BOOST_TEST(A::count == 4);
   s2 = boost::move(s1);
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s1.get() == 0);
   BOOST_TEST(A::count == 2);
   BOOST_TEST(s2.get_deleter().state() == 5);
   BOOST_TEST(s1.get_deleter().state() == 0);
   }
   BOOST_TEST(A::count == 0);
   {
   copy_constr_deleter<A[]> d1(5);
   bml::unique_ptr<A[], copy_constr_deleter<A[]>&> s1(new A[2], d1);
   A* p = s1.get();
   copy_constr_deleter<A[]> d2(6);
   bml::unique_ptr<A[], copy_constr_deleter<A[]>&> s2(new A[2], d2);
   BOOST_TEST(A::count == 4);
   s2 = boost::move(s1);
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s1.get() == 0);
   BOOST_TEST(A::count == 2);
   BOOST_TEST(d1.state() == 5);
   BOOST_TEST(d2.state() == 5);
   }
   BOOST_TEST(A::count == 0);
}

}  //unique_ptr_asgn_move01

////////////////////////////////
//   unique_ptr_ctor_default01
////////////////////////////////

namespace unique_ptr_ctor_default01{

// default unique_ptr ctor should only require default deleter ctor

void test()
{
   //Single unique_ptr
   {
   bml::unique_ptr<int> p;
   BOOST_TEST(p.get() == 0);
   }
   {
   bml::unique_ptr<int, def_constr_deleter<int> > p;
   BOOST_TEST(p.get() == 0);
   BOOST_TEST(p.get_deleter().state() == 5);
   }
   //Array unique_ptr
   {
   bml::unique_ptr<int[]> p;
   BOOST_TEST(p.get() == 0);
   }
   {
   bml::unique_ptr<int[], def_constr_deleter<int[]> > p;
   BOOST_TEST(p.get() == 0);
   BOOST_TEST(p.get_deleter().state() == 5);
   }
}

}  //namespace unique_ptr_ctor_default01{

////////////////////////////////
//   unique_ptr_ctor_default02
////////////////////////////////

namespace unique_ptr_ctor_default02{

// default unique_ptr ctor shouldn't require complete type

void test()
{
   //Single unique_ptr
   reset_counters();
   {
   J<I> s;
   BOOST_TEST(s.get() == 0);
   }
   check(0);
   {
   J<I, def_constr_deleter<I> > s;
   BOOST_TEST(s.get() == 0);
   BOOST_TEST(s.get_deleter().state() == 5);
   }
   check(0);
   //Array unique_ptr
   reset_counters();
   {
   J<I[]> s;
   BOOST_TEST(s.get() == 0);
   }
   check(0);
   {
   J<I[], def_constr_deleter<I[]> > s;
   BOOST_TEST(s.get() == 0);
   BOOST_TEST(s.get_deleter().state() == 5);
   }
   check(0);
}

}  //namespace unique_ptr_ctor_default02{

////////////////////////////////
//   unique_ptr_ctor_move_convert01
////////////////////////////////

namespace unique_ptr_ctor_move_convert01{

// test converting move ctor.  Should only require a MoveConstructible deleter, or if
//    deleter is a reference, not even that.
// Explicit version

void test()
{
   //Single unique_ptr
   reset_counters();
   {
   bml::unique_ptr<B> s(new B);
   A* p = s.get();
   bml::unique_ptr<A> s2(boost::move(s));
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s.get() == 0);
   BOOST_TEST(A::count == 1);
   BOOST_TEST(B::count == 1);
   }
   BOOST_TEST(A::count == 0);
   BOOST_TEST(B::count == 0);
   //Array unique_ptr, only from the same CV qualified pointers
   reset_counters();
   {
   bml::unique_ptr<A[]> s(new A[2]);
   A* p = s.get();
   bml::unique_ptr<const volatile A[]> s2(boost::move(s));
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s.get() == 0);
   BOOST_TEST(A::count == 2);
   }
   BOOST_TEST(A::count == 0);
}

}  //namespace unique_ptr_ctor_move_convert01{

////////////////////////////////
//   unique_ptr_ctor_move_convert02
////////////////////////////////

namespace unique_ptr_ctor_move_convert02{

// test converting move ctor.  Should only require a MoveConstructible deleter, or if
//    deleter is a reference, not even that.
// Explicit version

void test()
{
   //Single unique_ptr
   reset_counters();
   BOOST_STATIC_ASSERT((boost::move_detail::is_convertible<B, A>::value));
   {
   bml::unique_ptr<B, move_constr_deleter<B> > s(new B);
   A* p = s.get();
   bml::unique_ptr<A, move_constr_deleter<A> > s2(boost::move(s));
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s.get() == 0);
   BOOST_TEST(A::count == 1);
   BOOST_TEST(B::count == 1);
   BOOST_TEST(s2.get_deleter().state() == 5);
   BOOST_TEST(s.get_deleter().state() == 0);
   }
   BOOST_TEST(A::count == 0);
   BOOST_TEST(B::count == 0);
   //Array unique_ptr, only from the same CV qualified pointers
   reset_counters();
   {
   bml::unique_ptr<const A[], move_constr_deleter<const A[]> > s(new const A[2]);
   const A* p = s.get();
   bml::unique_ptr<const volatile A[], move_constr_deleter<const volatile A[]> > s2(boost::move(s));
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s.get() == 0);
   BOOST_TEST(A::count == 2);
   BOOST_TEST(s2.get_deleter().state() == 5);
   BOOST_TEST(s.get_deleter().state() == 0);
   }
   BOOST_TEST(A::count == 0);
   BOOST_TEST(B::count == 0);
}

}  //namespace unique_ptr_ctor_move_convert02{

////////////////////////////////
//   unique_ptr_ctor_move_convert03
////////////////////////////////

namespace unique_ptr_ctor_move_convert03{

// test converting move ctor.  Should only require a MoveConstructible deleter, or if
//    deleter is a reference, not even that.
// Explicit version

void test()
{
   //Single unique_ptr
   reset_counters();
   {
   def_constr_deleter<A> d;
   bml::unique_ptr<B, def_constr_deleter<A>&> s(new B, d);
   A* p = s.get();
   bml::unique_ptr<A, def_constr_deleter<A>&> s2(boost::move(s));
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s.get() == 0);
   BOOST_TEST(A::count == 1);
   BOOST_TEST(B::count == 1);
   d.set_state(6);
   BOOST_TEST(s2.get_deleter().state() == d.state());
   BOOST_TEST(s.get_deleter().state() ==  d.state());
   }
   BOOST_TEST(A::count == 0);
   BOOST_TEST(B::count == 0);
   //Array unique_ptr, only from the same CV qualified pointers
   reset_counters();
   {
   def_constr_deleter<volatile A[]> d;
   bml::unique_ptr<A[], def_constr_deleter<volatile A[]>&> s(new A[2], d);
   A* p = s.get();
   bml::unique_ptr<volatile A[], def_constr_deleter<volatile A[]>&> s2(boost::move(s));
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s.get() == 0);
   BOOST_TEST(A::count == 2);
   d.set_state(6);
   BOOST_TEST(s2.get_deleter().state() == d.state());
   BOOST_TEST(s.get_deleter().state() ==  d.state());
   }
   BOOST_TEST(A::count == 0);
}

}  //namespace unique_ptr_ctor_move_convert03{

////////////////////////////////
//   unique_ptr_ctor_move_convert04
////////////////////////////////

namespace unique_ptr_ctor_move_convert04{

// test converting move ctor. Should only require a MoveConstructible deleter, or if
//    deleter is a reference, not even that.
// implicit version

void test()
{
   //Single unique_ptr
   reset_counters();
   {
   bml::unique_ptr<B> s(new B);
   A* p = s.get();
   bml::unique_ptr<A> s2(boost::move(s));
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s.get() == 0);
   BOOST_TEST(A::count == 1);
   BOOST_TEST(B::count == 1);
   }
   BOOST_TEST(A::count == 0);
   BOOST_TEST(B::count == 0);
   //Array unique_ptr, only from the same CV qualified pointers
   reset_counters();
   {
   bml::unique_ptr<A[]> s(new A[2]);
   A* p = s.get();
   bml::unique_ptr<const A[]> s2(boost::move(s));
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s.get() == 0);
   BOOST_TEST(A::count == 2);
   }
   BOOST_TEST(A::count == 0);
}

}  //namespace unique_ptr_ctor_move_convert04{

////////////////////////////////
//   unique_ptr_ctor_move_convert05
////////////////////////////////

namespace unique_ptr_ctor_move_convert05{

// test converting move ctor.  Should only require a MoveConstructible deleter, or if
//    deleter is a reference, not even that.
// Implicit version

void test()
{
   //Single unique_ptr
   reset_counters();
   {
   bml::unique_ptr<B, move_constr_deleter<B> > s(new B);
   A* p = s.get();
   bml::unique_ptr<A, move_constr_deleter<A> > s2(boost::move(s));
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s.get() == 0);
   BOOST_TEST(A::count == 1);
   BOOST_TEST(B::count == 1);
   BOOST_TEST(s2.get_deleter().state() == 5);
   BOOST_TEST(s.get_deleter().state() == 0);
   }
   BOOST_TEST(A::count == 0);
   BOOST_TEST(B::count == 0);
   //Array unique_ptr, only from the same CV qualified pointers
   reset_counters();
   {
   bml::unique_ptr<const A[], move_constr_deleter<const A[]> > s(new const A[2]);
   const A* p = s.get();
   bml::unique_ptr<const volatile A[], move_constr_deleter<const volatile A[]> > s2(boost::move(s));
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s.get() == 0);
   BOOST_TEST(A::count == 2);
   BOOST_TEST(s2.get_deleter().state() == 5);
   BOOST_TEST(s.get_deleter().state() == 0);
   }
   BOOST_TEST(A::count == 0);
}

}  //namespace unique_ptr_ctor_move_convert05{

////////////////////////////////
//   unique_ptr_ctor_move_convert06
////////////////////////////////

namespace unique_ptr_ctor_move_convert06{

// test converting move ctor.  Should only require a MoveConstructible deleter, or if
//    deleter is a reference, not even that.
// Implicit version

void test()
{
   //Single unique_ptr
   reset_counters();
   {
   def_constr_deleter<A> d;
   bml::unique_ptr<B, def_constr_deleter<A>&> s(new B, d);
   A* p = s.get();
   bml::unique_ptr<A, def_constr_deleter<A>&> s2(boost::move(s));
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s.get() == 0);
   BOOST_TEST(A::count == 1);
   BOOST_TEST(B::count == 1);
   d.set_state(6);
   BOOST_TEST(s2.get_deleter().state() == d.state());
   BOOST_TEST(s.get_deleter().state() ==  d.state());
   }
   BOOST_TEST(A::count == 0);
   BOOST_TEST(B::count == 0);
   //Array unique_ptr, only from the same CV qualified pointers
   reset_counters();
   {
   def_constr_deleter<const volatile A[]> d;
   bml::unique_ptr<volatile A[], def_constr_deleter<const volatile A[]>&> s(new volatile A[2], d);
   volatile A* p = s.get();
   bml::unique_ptr<const volatile A[], def_constr_deleter<const volatile A[]>&> s2(boost::move(s));
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s.get() == 0);
   BOOST_TEST(A::count == 2);
   d.set_state(6);
   BOOST_TEST(s2.get_deleter().state() == d.state());
   BOOST_TEST(s.get_deleter().state() ==  d.state());
   }
   BOOST_TEST(A::count == 0);
}

}  //namespace unique_ptr_ctor_move_convert06{

////////////////////////////////
//   unique_ptr_ctor_move01
////////////////////////////////

namespace unique_ptr_ctor_move01{

// test converting move ctor.  Should only require a MoveConstructible deleter, or if
//    deleter is a reference, not even that.
// Implicit version

void test()
{
   //Single unique_ptr
   reset_counters();
   {
   bml::unique_ptr<A> s(new A);
   A* p = s.get();
   bml::unique_ptr<A> s2 = boost::move(s);
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s.get() == 0);
   BOOST_TEST(A::count == 1);
   }
   BOOST_TEST(A::count == 0);
   {
   bml::unique_ptr<A, move_constr_deleter<A> > s(new A);
   A* p = s.get();
   bml::unique_ptr<A, move_constr_deleter<A> > s2 = boost::move(s);
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s.get() == 0);
   BOOST_TEST(A::count == 1);
   BOOST_TEST(s2.get_deleter().state() == 5);
   BOOST_TEST(s.get_deleter().state() == 0);
   }
   BOOST_TEST(A::count == 0);
   {
   def_constr_deleter<A> d;
   bml::unique_ptr<A, def_constr_deleter<A>&> s(new A, d);
   A* p = s.get();
   bml::unique_ptr<A, def_constr_deleter<A>&> s2 = boost::move(s);
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s.get() == 0);
   BOOST_TEST(A::count == 1);
   d.set_state(6);
   BOOST_TEST(s2.get_deleter().state() == d.state());
   BOOST_TEST(s.get_deleter().state() ==  d.state());
   }
   BOOST_TEST(A::count == 0);
   //Array unique_ptr
   reset_counters();
   {
   bml::unique_ptr<A[]> s(new A[2]);
   A* p = s.get();
   bml::unique_ptr<A[]> s2 = boost::move(s);
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s.get() == 0);
   BOOST_TEST(A::count == 2);
   }
   BOOST_TEST(A::count == 0);
   {
   bml::unique_ptr<A[], move_constr_deleter<A[]> > s(new A[2]);
   A* p = s.get();
   bml::unique_ptr<A[], move_constr_deleter<A[]> > s2 = boost::move(s);
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s.get() == 0);
   BOOST_TEST(A::count == 2);
   BOOST_TEST(s2.get_deleter().state() == 5);
   BOOST_TEST(s.get_deleter().state() == 0);
   }
   BOOST_TEST(A::count == 0);
   {
   def_constr_deleter<A[]> d;
   bml::unique_ptr<A[], def_constr_deleter<A[]>&> s(new A[2], d);
   A* p = s.get();
   bml::unique_ptr<A[], def_constr_deleter<A[]>&> s2 = boost::move(s);
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s.get() == 0);
   BOOST_TEST(A::count == 2);
   d.set_state(6);
   BOOST_TEST(s2.get_deleter().state() == d.state());
   BOOST_TEST(s.get_deleter().state() ==  d.state());
   }
   BOOST_TEST(A::count == 0);
}

}  //namespace unique_ptr_ctor_move01{

////////////////////////////////
//   unique_ptr_ctor_move02
////////////////////////////////

namespace unique_ptr_ctor_move02{

// test move ctor.  Should only require a MoveConstructible deleter, or if
//    deleter is a reference, not even that.

bml::unique_ptr<A> source1()
{  return bml::unique_ptr<A>(new A);  }

bml::unique_ptr<A[]> source1_array()
{  return bml::unique_ptr<A[]> (new A[2]);  }

void sink1(bml::unique_ptr<A>)
{}

void sink1_array(bml::unique_ptr<A[]>)
{}

bml::unique_ptr<A, move_constr_deleter<A> > source2()
{  return bml::unique_ptr<A, move_constr_deleter<A> > (new A);  }

bml::unique_ptr<A[], move_constr_deleter<A[]> > source2_array()
{  return bml::unique_ptr<A[], move_constr_deleter<A[]> >(new A[2]);  }

void sink2(bml::unique_ptr<A, move_constr_deleter<A> >)
{}

void sink2_array(bml::unique_ptr<A[], move_constr_deleter<A[]> >)
{}

bml::unique_ptr<A, def_constr_deleter<A>&> source3()
{
   static def_constr_deleter<A> d;
   return bml::unique_ptr<A, def_constr_deleter<A>&>(new A, d);
}

bml::unique_ptr<A[], def_constr_deleter<A[]>&> source3_array()
{
   static def_constr_deleter<A[]> d;
   return bml::unique_ptr<A[], def_constr_deleter<A[]>&>(new A[2], d);
}

void sink3(bml::unique_ptr<A, def_constr_deleter<A>&> )
{}

void sink3_array(bml::unique_ptr<A[], def_constr_deleter<A[]>&> )
{}

void test()
{
   //Single unique_ptr
   reset_counters(); 
   sink1(source1());
   sink2(source2());
   sink3(source3());
   BOOST_TEST(A::count == 0);
   //Array unique_ptr
   reset_counters();
   sink1_array(source1_array());
   sink2_array(source2_array());
   sink3_array(source3_array());
   BOOST_TEST(A::count == 0);
}

}  //namespace unique_ptr_ctor_move02{

////////////////////////////////
//   unique_ptr_ctor_pointer_deleter01
////////////////////////////////

namespace unique_ptr_ctor_pointer_deleter01{

// test move ctor.  Should only require a MoveConstructible deleter, or if
//    deleter is a reference, not even that.

// unique_ptr(pointer, deleter()) only requires MoveConstructible deleter

void test()
{
   //Single unique_ptr
   reset_counters();
   {
   A* p = new A;
   BOOST_TEST(A::count == 1);
   move_constr_deleter<A> d;
   bml::unique_ptr<A, move_constr_deleter<A> > s(p, ::boost::move(d));
   BOOST_TEST(s.get() == p);
   BOOST_TEST(s.get_deleter().state() == 5);
   bml::unique_ptr<A, move_constr_deleter<A> > s2(s.release(), move_constr_deleter<A>(6));
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s2.get_deleter().state() == 6);
   }
   BOOST_TEST(A::count == 0);
   //Array unique_ptr
   reset_counters();
   {
   A* p = new A[2];
   BOOST_TEST(A::count == 2);
   move_constr_deleter<A[]> d;
   bml::unique_ptr<A[], move_constr_deleter<A[]> > s(p, ::boost::move(d));
   BOOST_TEST(s.get() == p);
   BOOST_TEST(s.get_deleter().state() == 5);
   bml::unique_ptr<A[], move_constr_deleter<A[]> > s2(s.release(), move_constr_deleter<A[]>(6));
   BOOST_TEST(s2.get() == p);
   BOOST_TEST(s2.get_deleter().state() == 6);
   }
   BOOST_TEST(A::count == 0);
}

}  //namespace unique_ptr_ctor_pointer_deleter01{

////////////////////////////////
//   unique_ptr_ctor_pointer_deleter02
////////////////////////////////

namespace unique_ptr_ctor_pointer_deleter02{

// unique_ptr(pointer, d) requires CopyConstructible deleter

void test()
{
   //Single unique_ptr
   reset_counters();
   {
   A* p = new A;
   BOOST_TEST(A::count == 1);
   copy_constr_deleter<A> d;
   bml::unique_ptr<A, copy_constr_deleter<A> > s(p, d);
   BOOST_TEST(s.get() == p);
   BOOST_TEST(s.get_deleter().state() == 5);
   d.set_state(6);
   BOOST_TEST(s.get_deleter().state() == 5);
   }
   BOOST_TEST(A::count == 0);
   //Array unique_ptr
   reset_counters();
   {
   A* p = new A[2];
   BOOST_TEST(A::count == 2);
   copy_constr_deleter<A[]> d;
   bml::unique_ptr<A, copy_constr_deleter<A[]> > s(p, d);
   BOOST_TEST(s.get() == p);
   BOOST_TEST(s.get_deleter().state() == 5);
   d.set_state(6);
   BOOST_TEST(s.get_deleter().state() == 5);
   }
   BOOST_TEST(A::count == 0);
}

}  //namespace unique_ptr_ctor_pointer_deleter02{

////////////////////////////////
//   unique_ptr_ctor_pointer_deleter03
////////////////////////////////

namespace unique_ptr_ctor_pointer_deleter03{

// unique_ptr<T, D&>(pointer, d) does not requires CopyConstructible deleter

void test()
{
   //Single unique_ptr
   reset_counters();
   {
   A* p = new A;
   BOOST_TEST(A::count == 1);
   def_constr_deleter<A> d;
   bml::unique_ptr<A, def_constr_deleter<A>&> s(p, d);
   BOOST_TEST(s.get() == p);
   BOOST_TEST(s.get_deleter().state() == 5);
   d.set_state(6);
   BOOST_TEST(s.get_deleter().state() == 6);
   }
   BOOST_TEST(A::count == 0);
   //Array unique_ptr
   reset_counters();
   {
   A* p = new A[2];
   BOOST_TEST(A::count == 2);
   def_constr_deleter<A[]> d;
   bml::unique_ptr<A[], def_constr_deleter<A[]>&> s(p, d);
   BOOST_TEST(s.get() == p);
   BOOST_TEST(s.get_deleter().state() == 5);
   d.set_state(6);
   BOOST_TEST(s.get_deleter().state() == 6);
   }
   BOOST_TEST(A::count == 0);
}

}  //namespace unique_ptr_ctor_pointer_deleter03{

////////////////////////////////
//   unique_ptr_ctor_pointer_deleter04
////////////////////////////////

namespace unique_ptr_ctor_pointer_deleter04{

// unique_ptr<T, const D&>(pointer, d) does not requires CopyConstructible deleter

void test()
{
   //Single unique_ptr
   reset_counters();
   {
   A* p = new A;
   BOOST_TEST(A::count == 1);
   def_constr_deleter<A> d;
   bml::unique_ptr<A, const def_constr_deleter<A>&> s(p, d);
   BOOST_TEST(s.get() == p);
   BOOST_TEST(s.get_deleter().state() == 5);
   }
   BOOST_TEST(A::count == 0);
   //Array unique_ptr
   reset_counters();
   {
   A* p = new A[2];
   BOOST_TEST(A::count == 2);
   def_constr_deleter<A[]> d;
   bml::unique_ptr<A[], const def_constr_deleter<A[]>&> s(p, d);
   BOOST_TEST(s.get() == p);
   BOOST_TEST(s.get_deleter().state() == 5);
   }
   BOOST_TEST(A::count == 0);
}

}  //namespace unique_ptr_ctor_pointer_deleter04{

////////////////////////////////
//   unique_ptr_ctor_pointer_deleter05
////////////////////////////////

namespace unique_ptr_ctor_pointer_deleter05{

// unique_ptr(pointer, deleter) should work with derived pointers
// or same (cv aside) types for array unique_ptrs

void test()
{
   //Single unique_ptr
   reset_counters();
   {
   B* p = new B;
   BOOST_TEST(A::count == 1);
   BOOST_TEST(B::count == 1);
   bml::unique_ptr<A, copy_constr_deleter<A> > s(p, copy_constr_deleter<A>());
   BOOST_TEST(s.get() == p);
   BOOST_TEST(s.get_deleter().state() == 5);
   }
   BOOST_TEST(A::count == 0);
   BOOST_TEST(B::count == 0);
   //Array unique_ptr
   reset_counters();
   {
   A* p = new A[2];
   BOOST_TEST(A::count == 2);
   bml::unique_ptr<const A[], copy_constr_deleter<const A[]> > s(p, copy_constr_deleter<const A[]>());
   BOOST_TEST(s.get() == p);
   BOOST_TEST(s.get_deleter().state() == 5);
   }
   BOOST_TEST(A::count == 0);
   BOOST_TEST(B::count == 0);
}

}  //namespace unique_ptr_ctor_pointer_deleter05{

////////////////////////////////
//   unique_ptr_ctor_pointer_deleter06
////////////////////////////////

namespace unique_ptr_ctor_pointer_deleter06{

// unique_ptr(pointer, deleter) should work with function pointers
// unique_ptr<void> should work

bool my_free_called = false;

void my_free(void*)
{
    my_free_called = true;
}

void test()
{
   {
   int i = 0;
   bml::unique_ptr<void, void (*)(void*)> s(&i, my_free);
   BOOST_TEST(s.get() == &i);
   BOOST_TEST(s.get_deleter() == my_free);
   BOOST_TEST(!my_free_called);
   }
   BOOST_TEST(my_free_called);
}

}  //namespace unique_ptr_ctor_pointer_deleter06{

////////////////////////////////
//   unique_ptr_ctor_pointer01
////////////////////////////////

namespace unique_ptr_ctor_pointer01{

// unique_ptr(pointer) ctor should only require default deleter ctor

void test()
{
   //Single unique_ptr
   reset_counters();
   {
   A* p = new A;
   BOOST_TEST(A::count == 1);
   bml::unique_ptr<A> s(p);
   BOOST_TEST(s.get() == p);
   }
   BOOST_TEST(A::count == 0);
   {
   A* p = new A;
   BOOST_TEST(A::count == 1);
   bml::unique_ptr<A, def_constr_deleter<A> > s(p);
   BOOST_TEST(s.get() == p);
   BOOST_TEST(s.get_deleter().state() == 5);
   }
   BOOST_TEST(A::count == 0);
   //Array unique_ptr
   reset_counters();
   {
   A* p = new A[2];
   BOOST_TEST(A::count == 2);
   bml::unique_ptr<A[]> s(p);
   BOOST_TEST(s.get() == p);
   }
   BOOST_TEST(A::count == 0);
   {
   A* p = new A[2];
   BOOST_TEST(A::count == 2);
   bml::unique_ptr<A[], def_constr_deleter<A[]> > s(p);
   BOOST_TEST(s.get() == p);
   BOOST_TEST(s.get_deleter().state() == 5);
   }
   BOOST_TEST(A::count == 0);
}

}  //namespace unique_ptr_ctor_pointer01{

////////////////////////////////
//   unique_ptr_ctor_pointer02
////////////////////////////////

namespace unique_ptr_ctor_pointer02{

// unique_ptr(pointer) ctor shouldn't require complete type

void test()
{
   //Single unique_ptr
   reset_counters();
   {
   I* p = get();
   check(1);
   J<I> s(p);
   BOOST_TEST(s.get() == p);
   }
   check(0);
   {
   I* p = get();
   check(1);
   J<I, def_constr_deleter<I> > s(p);
   BOOST_TEST(s.get() == p);
   BOOST_TEST(s.get_deleter().state() == 5);
   }
   check(0);
   //Array unique_ptr
   reset_counters();
   {
   I* p = get_array(2);
   check(2);
   J<I[]> s(p);
   BOOST_TEST(s.get() == p);
   }
   check(0);
   {
   I* p = get_array(2);
   check(2);
   J<I[], def_constr_deleter<I[]> > s(p);
   BOOST_TEST(s.get() == p);
   BOOST_TEST(s.get_deleter().state() == 5);
   }
   check(0);
}

}  //namespace unique_ptr_ctor_pointer02{

////////////////////////////////
//   unique_ptr_ctor_pointer03
////////////////////////////////

namespace unique_ptr_ctor_pointer03{

// unique_ptr(pointer) ctor should work with derived pointers
// or same types (cv aside) for unique_ptr<arrays>

void test()
{
   //Single unique_ptr
   reset_counters();
   {
   B* p = new B;
   BOOST_TEST(A::count == 1);
   BOOST_TEST(B::count == 1);
   bml::unique_ptr<A> s(p);
   BOOST_TEST(s.get() == p);
   }
   BOOST_TEST(A::count == 0);
   BOOST_TEST(B::count == 0);
   {
   B* p = new B;
   BOOST_TEST(A::count == 1);
   BOOST_TEST(B::count == 1);
   bml::unique_ptr<A, def_constr_deleter<A> > s(p);
   BOOST_TEST(s.get() == p);
   BOOST_TEST(s.get_deleter().state() == 5);
   }
   BOOST_TEST(A::count == 0);
   BOOST_TEST(B::count == 0);
   //Array unique_ptr
   reset_counters();
   {
   A* p = new A[2];
   BOOST_TEST(A::count == 2);
   bml::unique_ptr<const A[]> s(p);
   BOOST_TEST(s.get() == p);
   }
   BOOST_TEST(A::count == 0);
   {
   const A* p = new const A[2];
   BOOST_TEST(A::count == 2);
   bml::unique_ptr<const volatile A[], def_constr_deleter<const volatile A[]> > s(p);
   BOOST_TEST(s.get() == p);
   BOOST_TEST(s.get_deleter().state() == 5);
   }
   BOOST_TEST(A::count == 0);
}

}  //namespace unique_ptr_ctor_pointer03{

////////////////////////////////
//   unique_ptr_dtor_null
////////////////////////////////

namespace unique_ptr_dtor_null{

// The deleter is not called if get() == 0

void test()
{
   //Single unique_ptr
   def_constr_deleter<int> d;
   BOOST_TEST(d.state() == 5);
   {
   bml::unique_ptr<int, def_constr_deleter<int>&> p(0, d);
   BOOST_TEST(p.get() == 0);
   BOOST_TEST(&p.get_deleter() == &d);
   }
   BOOST_TEST(d.state() == 5);
}

}  //namespace unique_ptr_dtor_null{

////////////////////////////////
//   unique_ptr_modifiers_release
////////////////////////////////

namespace unique_ptr_modifiers_release{

void test()
{
   //Single unique_ptr
   {
   bml::unique_ptr<int> p(new int(3));
   int* i = p.get();
   int* j = p.release();
   BOOST_TEST(p.get() == 0);
   BOOST_TEST(i == j);
   }
   //Array unique_ptr
   {
   bml::unique_ptr<int[]> p(new int[2]);
   int* i = p.get();
   int* j = p.release();
   BOOST_TEST(p.get() == 0);
   BOOST_TEST(i == j);
   }
}

}  //namespace unique_ptr_modifiers_release{

////////////////////////////////
//   unique_ptr_modifiers_reset1
////////////////////////////////

namespace unique_ptr_modifiers_reset1{

void test()
{
   //Single unique_ptr
   reset_counters();
   {  //reset()
   bml::unique_ptr<A> p(new A);
   BOOST_TEST(A::count == 1);
   A* i = p.get();
   (void)i;
   p.reset();
   BOOST_TEST(A::count == 0);
   BOOST_TEST(p.get() == 0);
   }
   BOOST_TEST(A::count == 0);
   {  //reset(p)
   bml::unique_ptr<A> p(new A);
   BOOST_TEST(A::count == 1);
   A* i = p.get();
   (void)i;
   p.reset(new A);
   BOOST_TEST(A::count == 1);
   }
   BOOST_TEST(A::count == 0);
   {  //reset(0)
   bml::unique_ptr<A> p(new A);
   BOOST_TEST(A::count == 1);
   A* i = p.get();
   (void)i;
   p.reset(0);
   BOOST_TEST(A::count == 0);
   BOOST_TEST(p.get() == 0);
   }
   BOOST_TEST(A::count == 0);
   //Array unique_ptr
   reset_counters();
   {  //reset()
   bml::unique_ptr<A[]> p(new A[2]);
   BOOST_TEST(A::count == 2);
   A* i = p.get();
   (void)i;
   p.reset();
   BOOST_TEST(A::count == 0);
   BOOST_TEST(p.get() == 0);
   }
   BOOST_TEST(A::count == 0);
   {  //reset(p)
   bml::unique_ptr<A[]> p(new A[2]);
   BOOST_TEST(A::count == 2);
   A* i = p.get();
   (void)i;
   p.reset(new A[3]);
   BOOST_TEST(A::count == 3);
   }
   BOOST_TEST(A::count == 0);
   {  //reset(0)
   bml::unique_ptr<A[]> p(new A[2]);
   BOOST_TEST(A::count == 2);
   A* i = p.get();
   (void)i;
   p.reset(0);
   BOOST_TEST(A::count == 0);
   BOOST_TEST(p.get() == 0);
   }
   BOOST_TEST(A::count == 0);
}

}  //namespace unique_ptr_modifiers_reset1{

////////////////////////////////
//   unique_ptr_modifiers_reset2
////////////////////////////////

namespace unique_ptr_modifiers_reset2{

void test()
{
   //Single unique_ptr
   reset_counters();
   {
   bml::unique_ptr<A> p(new A);
   BOOST_TEST(A::count == 1);
   BOOST_TEST(B::count == 0);
   A* i = p.get();
   (void)i;
   p.reset(new B);
   BOOST_TEST(A::count == 1);
   BOOST_TEST(B::count == 1);
   }
   BOOST_TEST(A::count == 0);
   BOOST_TEST(B::count == 0);
   {
   bml::unique_ptr<A> p(new B);
   BOOST_TEST(A::count == 1);
   BOOST_TEST(B::count == 1);
   A* i = p.get();
   (void)i;
   p.reset(new B);
   BOOST_TEST(A::count == 1);
   BOOST_TEST(B::count == 1);
   }
   BOOST_TEST(A::count == 0);
   BOOST_TEST(B::count == 0);
   //Array unique_ptr
   reset_counters();
   {
   bml::unique_ptr<const volatile A[]> p(new const A[2]);
   BOOST_TEST(A::count == 2);
   const volatile A* i = p.get();
   (void)i;
   p.reset(new volatile A[3]);
   BOOST_TEST(A::count == 3);
   }
   BOOST_TEST(A::count == 0);
   {
   bml::unique_ptr<const A[]> p(new A[2]);
   BOOST_TEST(A::count == 2);
   const A* i = p.get();
   (void)i;
   p.reset(new const A[3]);
   BOOST_TEST(A::count == 3);
   }
   BOOST_TEST(A::count == 0);
}

}  //unique_ptr_modifiers_reset2


////////////////////////////////
//   unique_ptr_modifiers
////////////////////////////////

namespace unique_ptr_modifiers_swap{

// test swap

void test()
{
   //Single unique_ptr
   reset_counters();
   {
   A* p1 = new A(1);
   move_constr_deleter<A> d1(1);
   bml::unique_ptr<A, move_constr_deleter<A> > s1(p1, ::boost::move(d1));
   A* p2 = new A(2);
   move_constr_deleter<A> d2(2);
   bml::unique_ptr<A, move_constr_deleter<A> > s2(p2, ::boost::move(d2));
   BOOST_TEST(s1.get() == p1);
   BOOST_TEST(*s1 == A(1));
   BOOST_TEST(s1.get_deleter().state() == 1);
   BOOST_TEST(s2.get() == p2);
   BOOST_TEST(*s2 == A(2));
   BOOST_TEST(s2.get_deleter().state() == 2);
   swap(s1, s2);
   BOOST_TEST(s1.get() == p2);
   BOOST_TEST(*s1 == A(2));
   BOOST_TEST(s1.get_deleter().state() == 2);
   BOOST_TEST(s2.get() == p1);
   BOOST_TEST(*s2 == A(1));
   BOOST_TEST(s2.get_deleter().state() == 1);
   }
   //Array unique_ptr
   reset_counters();
   {
   A* p1 = new A[2];
   p1[0].set(1);
   p1[1].set(2);
   move_constr_deleter<A[]> d1(1);
   bml::unique_ptr<A[], move_constr_deleter<A[]> > s1(p1, ::boost::move(d1));
   A* p2 = new A[2];
   p2[0].set(3);
   p2[1].set(4);
   move_constr_deleter<A[]> d2(2);
   bml::unique_ptr<A[], move_constr_deleter<A[]> > s2(p2, ::boost::move(d2));
   BOOST_TEST(s1.get() == p1);
   BOOST_TEST(s1[0] == A(1));
   BOOST_TEST(s1[1] == A(2));
   BOOST_TEST(s1.get_deleter().state() == 1);
   BOOST_TEST(s2.get() == p2);
   BOOST_TEST(s2[0] == A(3));
   BOOST_TEST(s2[1] == A(4));
   BOOST_TEST(s2.get_deleter().state() == 2);
   swap(s1, s2);
   BOOST_TEST(s1.get() == p2);
   BOOST_TEST(s1[0] == A(3));
   BOOST_TEST(s1[1] == A(4));
   BOOST_TEST(s1.get_deleter().state() == 2);
   BOOST_TEST(s2.get() == p1);
   BOOST_TEST(s2[0] == A(1));
   BOOST_TEST(s2[1] == A(2));
   BOOST_TEST(s2.get_deleter().state() == 1);
   }
}

}  //namespace unique_ptr_modifiers_swap{

////////////////////////////////
//   unique_ptr_observers_dereference
////////////////////////////////

namespace unique_ptr_observers_dereference{

void test()
{
   //Single unique_ptr
   {
   bml::unique_ptr<int> p(new int(3));
   BOOST_TEST(*p == 3);
   }
   //Array unique_ptr
   {
   int *pi = new int[2];
   pi[0] = 3;
   pi[1] = 4;
   bml::unique_ptr<int[]> p(pi);
   BOOST_TEST(p[0] == 3);
   BOOST_TEST(p[1] == 4);
   }
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
   //Array unique_ptr
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
   //Array unique_ptr
   {
   int* p = new int[2];
   bml::unique_ptr<int[]> s(p);
   BOOST_TEST(s.get() == p);
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
   //Array unique_ptr
   {
   bml::unique_ptr<int[], Deleter> p;
   BOOST_TEST(p.get_deleter().test() == 5);
   }
   {
   const bml::unique_ptr<int[], Deleter> p;
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
   //Single unique_ptr
   {
   A *pa = new A[2];
   //pa[0] is left default constructed
   pa[1].set(888);
   bml::unique_ptr<A[]> p(pa);
   BOOST_TEST(p[0].state_ == 999);
   BOOST_TEST(p[1].state_ == 888);
   }
}

}  //namespace unique_ptr_observers_op_index{

////////////////////////////////
//   unique_ptr_zero
////////////////////////////////
namespace unique_ptr_zero {

// test initialization/assignment from zero

void test()
{
   //Single unique_ptr
   reset_counters();
   {
   bml::unique_ptr<A> s2(0);
   BOOST_TEST(A::count == 0);
   }
   BOOST_TEST(A::count == 0);
   {
   bml::unique_ptr<A> s2(new A);
   BOOST_TEST(A::count == 1);
   s2 = 0;
   BOOST_TEST(A::count == 0);
   BOOST_TEST(s2.get() == 0);
   }
   BOOST_TEST(A::count == 0);

   //Array unique_ptr
   {
   bml::unique_ptr<A[]> s2(0);
   BOOST_TEST(A::count == 0);
   }
   BOOST_TEST(A::count == 0);
   {
   bml::unique_ptr<A[]> s2(new A[2]);
   BOOST_TEST(A::count == 2);
   s2 = 0;
   BOOST_TEST(A::count == 0);
   BOOST_TEST(s2.get() == 0);
   }
   BOOST_TEST(A::count == 0);
}

}  //namespace unique_ptr_zero {


////////////////////////////////
//       unique_ptr_nullptr
////////////////////////////////

namespace unique_ptr_nullptr{

void test()
{
   #if !defined(BOOST_NO_CXX11_NULLPTR)
   //Single unique_ptr
   reset_counters();
   {
      bml::unique_ptr<A> p(new A);
      BOOST_TEST(A::count == 1);
      A* i = p.get();
      (void)i;
      p.reset(nullptr);
      BOOST_TEST(A::count == 0);
      BOOST_TEST(p.get() == 0);
   }
   BOOST_TEST(A::count == 0);
   {
      bml::unique_ptr<A> p(new A);
      BOOST_TEST(A::count == 1);
      A* i = p.get();
      (void)i;
      p = nullptr;
      BOOST_TEST(A::count == 0);
      BOOST_TEST(p.get() == 0);
   }
   BOOST_TEST(A::count == 0);

   {
      bml::unique_ptr<A> pi(nullptr);
      BOOST_TEST(pi.get() == nullptr);
      BOOST_TEST(pi.get() == 0);
   }
   BOOST_TEST(A::count == 0);
   {
      bml::unique_ptr<A> pi(nullptr, bml::unique_ptr<A>::deleter_type());
      BOOST_TEST(pi.get() == nullptr);
      BOOST_TEST(pi.get() == 0);
   }
   BOOST_TEST(A::count == 0);

   //Array unique_ptr
   reset_counters();
   {
      bml::unique_ptr<A[]> p(new A[2]);
      BOOST_TEST(A::count == 2);
      A* i = p.get();
      (void)i;
      p.reset(nullptr);
      BOOST_TEST(A::count == 0);
      BOOST_TEST(p.get() == 0);
   }
   BOOST_TEST(A::count == 0);
   {
      bml::unique_ptr<A[]> p(new A[2]);
      BOOST_TEST(A::count == 2);
      A* i = p.get();
      (void)i;
      p = nullptr;
      BOOST_TEST(A::count == 0);
      BOOST_TEST(p.get() == 0);
   }
   BOOST_TEST(A::count == 0);
   {
      bml::unique_ptr<A[]> pi(nullptr);
      BOOST_TEST(pi.get() == nullptr);
      BOOST_TEST(pi.get() == 0);
   }
   BOOST_TEST(A::count == 0);
   {
      bml::unique_ptr<A[]> pi(nullptr, bml::unique_ptr<A[]>::deleter_type());
      BOOST_TEST(pi.get() == nullptr);
      BOOST_TEST(pi.get() == 0);
   }
   BOOST_TEST(A::count == 0);

   //Array element
   #endif
}

}  //namespace unique_ptr_nullptr{

////////////////////////////////
//             main
////////////////////////////////
int main()
{
   //General
  pointer_type::test();

   //Assignment
   unique_ptr_asgn_move_convert01::test();
   unique_ptr_asgn_move_convert02::test();
   unique_ptr_asgn_move_convert03::test();
   unique_ptr_asgn_move01::test();

   //Constructor
   unique_ptr_ctor_default01::test();
   unique_ptr_ctor_default02::test();
   unique_ptr_ctor_move_convert01::test();
   unique_ptr_ctor_move_convert02::test();
   unique_ptr_ctor_move_convert03::test();
   unique_ptr_ctor_move_convert04::test();
   unique_ptr_ctor_move_convert05::test();
   unique_ptr_ctor_move_convert06::test();
   unique_ptr_ctor_move01::test();
   unique_ptr_ctor_move02::test();
   unique_ptr_ctor_pointer_deleter01::test();
   unique_ptr_ctor_pointer_deleter02::test();
   unique_ptr_ctor_pointer_deleter03::test();
   unique_ptr_ctor_pointer_deleter04::test();
   unique_ptr_ctor_pointer_deleter05::test();
   unique_ptr_ctor_pointer_deleter06::test();
   unique_ptr_ctor_pointer01::test();
   unique_ptr_ctor_pointer02::test();
   unique_ptr_ctor_pointer03::test();

   //Destructor
   unique_ptr_dtor_null::test();

   //Modifiers
   unique_ptr_modifiers_release::test();
   unique_ptr_modifiers_reset1::test();
   unique_ptr_modifiers_reset2::test();
   unique_ptr_modifiers_swap::test();

   //Observers
   unique_ptr_observers_dereference::test();
   unique_ptr_observers_explicit_bool::test();
   unique_ptr_observers_get::test();
   unique_ptr_observers_get_deleter::test();
   unique_ptr_observers_op_arrow::test();
   unique_ptr_observers_op_index::test();

   //nullptr
   unique_ptr_zero::test();
   unique_ptr_nullptr::test();

   //Test results
   return boost::report_errors();

}

//Define the incomplete I type and out of line functions

struct I
{
   static int count;
   I() {++count;}
   I(const A&) {++count;}
   ~I() {--count;}
};

int I::count = 0;

I* get() {return new I;}
I* get_array(int i) {return new I[i];}

void check(int i)
{
   BOOST_TEST(I::count == i);
}

template <class T, class D>
J<T, D>::~J() {}

void reset_counters()
{  A::count = 0; B::count = 0; I::count = 0; }
