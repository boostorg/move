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
#include <boost/config.hpp>

namespace bml = ::boost::movelib;

namespace {

struct counted
{
   static int alive;
   counted()  { ++alive; }
   ~counted() { --alive; }
};
int counted::alive = 0;

//A deleter that is not trivially copyable: the attribute must be dropped for it
struct stateful_deleter
{
   int *counter;
   stateful_deleter(int *c) : counter(c) {}
   stateful_deleter(const stateful_deleter &o) : counter(o.counter) { ++*counter; }
   void operator()(counted *p) const { delete p; }
};

//Passing by value: with trivial_abi the callee destroys the parameter
counted *sink(bml::unique_ptr<counted> p)
{  return p.get();  }

bml::unique_ptr<counted> source()
{  return bml::unique_ptr<counted>(new counted);  }

}  //namespace {

int main()
{
   //Behaviour must not change whether or not the attribute is honoured
   {
      bml::unique_ptr<counted> p(new counted);
      BOOST_TEST(counted::alive == 1);
      counted *raw = p.get();
      BOOST_TEST(sink(boost::move(p)) == raw);
      BOOST_TEST(counted::alive == 0);
      BOOST_TEST(!p);
   }
   {
      bml::unique_ptr<counted> p = source();
      BOOST_TEST(counted::alive == 1);
   }
   BOOST_TEST(counted::alive == 0);
   {
      int copies = 0;
      bml::unique_ptr<counted, stateful_deleter> p(new counted, stateful_deleter(&copies));
      bml::unique_ptr<counted, stateful_deleter> q(boost::move(p));
      BOOST_TEST(counted::alive == 1);
      BOOST_TEST(!p);
   }
   BOOST_TEST(counted::alive == 0);

   //Clang honours trivial_abi with Itanium-family C++ ABIs (Linux, macOS, MinGW, Cygwin),
   //not with the Microsoft ABI. The old __is_trivially_relocatable builtin reports
   //"trivial for the purposes of calls", which is exactly what the attribute grants.
   #if defined(__clang__) && !defined(_MSC_VER) && defined(__has_builtin)
   #  if __has_builtin(__is_trivially_relocatable)
   #     if defined(__has_attribute)
   #        if __has_attribute(trivial_abi)
   #pragma clang diagnostic push
   #pragma clang diagnostic ignored "-Wdeprecated-builtins"
   typedef bml::unique_ptr<counted> plain_ptr;
   typedef bml::unique_ptr<counted[]> array_ptr;
   typedef bml::unique_ptr<counted, stateful_deleter> stateful_ptr;
   #           if defined(BOOST_MOVE_DISABLE_TRIVIAL_ABI)
   //Opted out: unique_ptr has a non-trivial move constructor and no attribute
   BOOST_MOVE_STATIC_ASSERT(!__is_trivially_relocatable(plain_ptr));
   BOOST_MOVE_STATIC_ASSERT(!__is_trivially_relocatable(array_ptr));
   #           else
   BOOST_MOVE_STATIC_ASSERT(__is_trivially_relocatable(plain_ptr));
   BOOST_MOVE_STATIC_ASSERT(__is_trivially_relocatable(array_ptr));
   #           endif
   //Never with a deleter that is not trivial for calls
   BOOST_MOVE_STATIC_ASSERT(!__is_trivially_relocatable(stateful_ptr));
   #pragma clang diagnostic pop
   #        endif
   #     endif
   #  endif
   #endif

   return boost::report_errors();
}
