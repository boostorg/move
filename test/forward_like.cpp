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

#include <boost/move/utility_core.hpp>
#include <boost/move/detail/meta_utils.hpp>
#include <boost/core/lightweight_test.hpp>

//A copyable and movable type that records how it was constructed
class member
{
   BOOST_COPYABLE_AND_MOVABLE(member)

   public:
   enum origin_t { value_ctor, copy_ctor, move_ctor };
   int value;
   origin_t origin;

   explicit member(int v = 0) : value(v), origin(value_ctor) {}
   member(const member &o) : value(o.value), origin(copy_ctor) {}
   member(BOOST_RV_REF(member) o) : value(o.value), origin(move_ctor) {  o.value = -1;  }
   member &operator=(BOOST_COPY_ASSIGN_REF(member) o) {  value = o.value; return *this;  }
   member &operator=(BOOST_RV_REF(member) o) {  value = o.value; o.value = -1; return *this;  }
};

//An aggregate-like movable type whose members are forwarded
class holder
{
   BOOST_COPYABLE_AND_MOVABLE(holder)

   public:
   member m;
   int i;

   explicit holder(int v = 0) : m(v), i(v) {}
   holder(const holder &o) : m(o.m), i(o.i) {}
   holder(BOOST_RV_REF(holder) o) : m(::boost::move(o.m)), i(o.i) {}
   holder &operator=(BOOST_COPY_ASSIGN_REF(holder) o) {  m = o.m; i = o.i; return *this;  }
   holder &operator=(BOOST_RV_REF(holder) o) {  m = ::boost::move(o.m); i = o.i; return *this;  }
};

//Typical use: forward the members of a forwarding reference argument
template<class T>
member make_member(BOOST_FWD_REF(T) t)
{  return member(::boost::forward_like<T>(t.m));  }

template<class T>
int get_int(BOOST_FWD_REF(T) t)
{  return ::boost::forward_like<T>(t.i);  }

void test_forwarding_reference()
{
   //Non-const lvalue: copy, the source is not modified
   {
      holder h(1);
      member r(make_member(h));
      BOOST_TEST_EQ(r.value, 1);
      BOOST_TEST(r.origin == member::copy_ctor);
      BOOST_TEST_EQ(h.m.value, 1);
      BOOST_TEST_EQ(get_int(h), 1);
   }
   //Const lvalue: copy
   {
      const holder h(2);
      member r(make_member(h));
      BOOST_TEST_EQ(r.value, 2);
      BOOST_TEST(r.origin == member::copy_ctor);
      BOOST_TEST_EQ(h.m.value, 2);
   }
   //Moved argument: move
   {
      holder h(3);
      member r(make_member(::boost::move(h)));
      BOOST_TEST_EQ(r.value, 3);
      BOOST_TEST(r.origin == member::move_ctor);
      BOOST_TEST_EQ(h.m.value, -1);
      BOOST_TEST_EQ(get_int(::boost::move(h)), 3);
   }
}

void test_explicit_reference()
{
   holder h(4);
   //Lvalue reference: lvalue
   {
      member r(::boost::forward_like<holder&>(h.m));
      BOOST_TEST(r.origin == member::copy_ctor);
      BOOST_TEST_EQ(h.m.value, 4);
   }
   {
      member r(::boost::forward_like<const holder&>(h.m));
      BOOST_TEST(r.origin == member::copy_ctor);
      BOOST_TEST_EQ(h.m.value, 4);
   }
   //The result can be modified when the reference is not const
   ::boost::forward_like<holder&>(h.m).value = 5;
   BOOST_TEST_EQ(h.m.value, 5);
}

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

template<class T, class U>
void test_same_type(U &&)
{  BOOST_TEST((::boost::move_detail::is_same<T, U&&>::value));  }

void test_cxx11_types()
{
   using ::boost::forward_like;
   member m;
   const member cm;
   //Value category of T, const merged from T and from x (as std::forward_like)
   test_same_type<member&>(forward_like<holder&>(m));
   test_same_type<const member&>(forward_like<const holder&>(m));
   test_same_type<const member&>(forward_like<holder&>(cm));
   test_same_type<member&&>(forward_like<holder>(m));
   test_same_type<member&&>(forward_like<holder&&>(m));
   test_same_type<const member&&>(forward_like<const holder>(m));
   test_same_type<const member&&>(forward_like<const holder&&>(m));
   test_same_type<const member&&>(forward_like<holder&&>(cm));
   //x can be an rvalue
   test_same_type<member&>(forward_like<holder&>(member()));
   test_same_type<member&&>(forward_like<holder>(member()));

   //A non-reference T moves
   member r(forward_like<holder>(m));
   BOOST_TEST(r.origin == member::move_ctor);
}

#endif   //#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

int main()
{
   test_forwarding_reference();
   test_explicit_reference();
   #if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
   test_cxx11_types();
   #endif
   return ::boost::report_errors();
}
