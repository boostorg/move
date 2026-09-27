//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2026-2026.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////
//unique_ptr.hpp is included before Boost.ContainerHash to test that hash_value
//does not depend on the include order
#include <boost/move/unique_ptr.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/config.hpp>

//Boost.ContainerHash and Boost.Unordered require C++11
#if BOOST_CXX_VERSION >= 201103L

#include <boost/container_hash/hash.hpp>
#include <boost/unordered_set.hpp>
#include <boost/container/vector.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cstddef>

namespace bml = ::boost::movelib;

//A deleter that does not delete, so that two unique_ptrs can store the same pointer
struct no_delete
{
   template<class T>
   void operator()(T*) const {}
};

//A pointer with its own hash_value
struct fancy_ptr
{
   fancy_ptr() : p() {}
   fancy_ptr(int *q) : p(q) {}
   #if !defined(BOOST_NO_CXX11_NULLPTR)
   fancy_ptr(std::nullptr_t) : p() {}
   #endif
   int& operator*() const { return *p; }
   operator bool() const { return p != 0; }
   friend bool operator==(const fancy_ptr &a, const fancy_ptr &b) { return a.p == b.p; }
   friend bool operator!=(const fancy_ptr &a, const fancy_ptr &b) { return a.p != b.p; }
   friend std::size_t hash_value(const fancy_ptr &x) { return reinterpret_cast<std::size_t>(x.p) + 1u; }
   int *p;
};

struct fancy_deleter
{
   typedef fancy_ptr pointer;
   void operator()(fancy_ptr q) const { delete q.p; }
};

void test_hash()
{
   //The hash is the hash of the stored pointer
   {
   int *pi = new int(1);
   bml::unique_ptr<int> p(pi);
   BOOST_TEST(boost::hash<bml::unique_ptr<int> >()(p) == boost::hash<int*>()(pi));
   BOOST_TEST(hash_value(p) == boost::hash<int*>()(pi));
   bml::unique_ptr<int> n;
   BOOST_TEST(boost::hash<bml::unique_ptr<int> >()(n) == boost::hash<int*>()(0));
   }
   //Arrays
   {
   int *pi = new int[2];
   bml::unique_ptr<int[]> p(pi);
   BOOST_TEST(boost::hash<bml::unique_ptr<int[]> >()(p) == boost::hash<int*>()(pi));
   }
   //const element type
   {
   int *pi = new int(2);
   bml::unique_ptr<const int> p(pi);
   BOOST_TEST(boost::hash<bml::unique_ptr<const int> >()(p) == boost::hash<const int*>()(pi));
   }
   //A pointer type with its own hash_value
   {
   int *pi = new int(3);
   typedef bml::unique_ptr<int, fancy_deleter> up_t;
   up_t p(pi);
   BOOST_TEST(boost::hash<up_t>()(p) == hash_value(fancy_ptr(pi)));
   }
}

void test_hash_range()
{
   //boost::hash_range on a Boost.Container container of unique_ptr
   boost::container::vector<bml::unique_ptr<int> > v;
   int *raw[2];
   for(std::size_t i = 0; i != 2; ++i){
      raw[i] = new int((int)i);
      v.push_back(bml::unique_ptr<int>(raw[i]));
   }
   BOOST_TEST(boost::hash_range(v.begin(), v.end()) == boost::hash_range(&raw[0], &raw[0] + 2));
}

void test_unordered()
{
   int values[3] = { 0, 1, 2 };
   typedef bml::unique_ptr<int, no_delete> up_t;
   boost::unordered_set<up_t> s;
   for(std::size_t i = 0; i != 3; ++i){
      s.insert(up_t(&values[i]));
   }
   BOOST_TEST(s.size() == 3u);
   //A unique_ptr that stores the same pointer is equal and has the same hash
   for(std::size_t i = 0; i != 3; ++i){
      up_t key(&values[i]);
      BOOST_TEST(s.find(key) != s.end());
   }
   int other = 3;
   BOOST_TEST(s.find(up_t(&other)) == s.end());
}

int main()
{
   test_hash();
   test_hash_range();
   test_unordered();
   return boost::report_errors();
}

#else

int main()
{
   return 0;
}

#endif
