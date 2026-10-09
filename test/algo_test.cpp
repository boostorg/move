//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2007-2026.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/move/algo/detail/set_difference.hpp>
#include <boost/move/iterator.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cassert>
#include <cstddef>
#include <iterator>
#include "order_type.hpp"

//Tests all the algorithms of set_difference.hpp with the same cases:
//
// - set_difference, set_unique_difference: with order_perf_type (copy) and
//   with order_move_type through move iterators (move).
// - inplace_set_difference, inplace_set_unique_difference,
//   set_unique_difference_partition, inplace_set_unique_difference_partition:
//   with order_move_type.
//
//order_move_type is movable but not copyable: the algorithms can not copy it.
//It asserts on self-move assignment. Moved-from objects have
//order_move_type::moved_constr_mark or order_move_type::moved_assign_mark as key.
//
//Each case runs with raw pointers and with a minimal forward iterator. The results
//are compared with a simple reference implementation. "key" is the compared value
//and "val" is the index of the element in range 1, to check the order of
//equivalent elements.

//////////////////////////////////////////////////////////////////////////////
//Element utilities

struct copy_tag {};
struct move_tag {};

inline bool is_moved_from(const order_perf_type &)
{
   return false;
}

inline bool is_moved_from(const order_move_type &o)
{
   return o.key == order_move_type::moved_constr_mark || o.key == order_move_type::moved_assign_mark;
}

inline copy_tag source_tag(const order_perf_type *)
{  return copy_tag();  }

inline move_tag source_tag(const order_move_type *)
{  return move_tag();  }

//Reads range 1 with copies or with moves
template<class It>
It source(It it, copy_tag)
{  return it;  }

template<class It>
boost::move_iterator<It> source(It it, move_tag)
{  return boost::make_move_iterator(it);  }

inline void reset_copies(const order_perf_type *)
{  order_perf_type::num_copy = 0u;  }

inline void reset_copies(const order_move_type *)
{}

inline void check_copies(const order_perf_type *, std::size_t n)
{  BOOST_TEST(order_perf_type::num_copy == n);  }

//order_move_type can not be copied
inline void check_copies(const order_move_type *, std::size_t)
{}

//Compares keys, counts comparisons and checks that moved-from objects are not compared
template<class T>
struct counted_less
{
   bool operator()(const T &a, const T &b) const
   {
      BOOST_TEST(!is_moved_from(a));
      BOOST_TEST(!is_moved_from(b));
      ++num_compare;
      return a.key < b.key;
   }

   static std::size_t num_compare;
};

template<class T>
std::size_t counted_less<T>::num_compare = 0u;

//////////////////////////////////////////////////////////////////////////////
//A forward iterator to test that only forward iterator operations are used
template<class T>
class fwd_iterator
{
   T *p_;

   public:
   typedef std::forward_iterator_tag   iterator_category;
   typedef T                           value_type;
   typedef std::ptrdiff_t              difference_type;
   typedef T*                          pointer;
   typedef T&                          reference;

   fwd_iterator()
      : p_()
   {}

   explicit fwd_iterator(T *p)
      : p_(p)
   {}

   T &operator*() const
   {  return *p_;  }

   fwd_iterator &operator++()
   {  ++p_; return *this;  }

   fwd_iterator operator++(int)
   {  fwd_iterator r(*this); ++p_; return r;  }

   T *base() const
   {  return p_;  }

   friend bool operator==(const fwd_iterator &a, const fwd_iterator &b)
   {  return a.p_ == b.p_;  }

   friend bool operator!=(const fwd_iterator &a, const fwd_iterator &b)
   {  return a.p_ != b.p_;  }
};

//Iterator types used to call the algorithms: raw pointers or forward iterators
template<class T, bool UseFwdIterator>
struct iterators;

template<class T>
struct iterators<T, false>
{
   typedef T *it1_t;
   typedef const T *it2_t;

   static it1_t it1(T *p)              {  return p;  }
   static it2_t it2(const T *p)        {  return p;  }
   static T *base(it1_t p)             {  return p;  }
};

template<class T>
struct iterators<T, true>
{
   typedef fwd_iterator<T> it1_t;
   typedef fwd_iterator<const T> it2_t;

   static it1_t it1(T *p)              {  return it1_t(p);  }
   static it2_t it2(const T *p)        {  return it2_t(p);  }
   static T *base(it1_t p)             {  return p.base();  }
};

//////////////////////////////////////////////////////////////////////////////
//Simple deterministic random generator
class test_rand
{
   unsigned long state_;

   public:
   explicit test_rand(unsigned long seed)
      : state_(seed)
   {}

   std::size_t operator()(std::size_t n)
   {
      state_ = (state_ * 1103515245ul + 12345ul) & 0x7FFFFFFFul;
      return std::size_t(state_ >> 8u) % n;
   }
};

const std::size_t MaxSize = 64u;
//Array size: one more position to check that nothing is written past the end
const std::size_t ArraySize = MaxSize + 1u;
const std::size_t SentinelKey = 998u;
const std::size_t SentinelVal = 999u;

//////////////////////////////////////////////////////////////////////////////
//Reference implementation. Stores the indexes of the elements of range 1
//that each algorithm keeps or moves out.
struct reference_result
{
   //set_difference: for a group of m elements of range 1 with n equivalent
   //elements in range 2, the last max(m-n, 0) elements of the group.
   std::size_t diff[MaxSize];
   std::size_t diff_size;
   //Unique difference: the first element of each group without
   //equivalent elements in range 2.
   std::size_t d[MaxSize];
   std::size_t d_size;
   //Rest: all other elements of range 1.
   std::size_t r[MaxSize];
   std::size_t r_size;
   std::size_t num_groups;
   bool in_diff[MaxSize];
   bool in_d[MaxSize];
};

void reference_set_difference
   ( const std::size_t *keys1, std::size_t n1, const std::size_t *keys2, std::size_t n2
   , reference_result &res)
{
   res.diff_size = res.d_size = res.r_size = res.num_groups = 0u;
   std::size_t b = 0u;
   while(b != n1){
      std::size_t e = b;
      while(e != n1 && keys1[e] == keys1[b]){
         ++e;
      }
      std::size_t n = 0u;
      for(std::size_t j = 0; j != n2; ++j){
         if(keys2[j] == keys1[b])
            ++n;
      }
      ++res.num_groups;
      for(std::size_t i = b; i != e; ++i){
         res.in_diff[i] = (i - b) >= n;
         if(res.in_diff[i])
            res.diff[res.diff_size++] = i;
         res.in_d[i] = i == b && n == 0u;
         if(res.in_d[i])
            res.d[res.d_size++] = i;
         else
            res.r[res.r_size++] = i;
      }
      b = e;
   }
}

//////////////////////////////////////////////////////////////////////////////
//Checks

//Fills [0, n) with the keys and the indexes and [n, ArraySize) with sentinels
template<class T>
void fill_range(T *p, const std::size_t *keys, std::size_t n)
{
   for(std::size_t i = 0; i != ArraySize; ++i){
      p[i].key = i < n ? keys[i] : SentinelKey;
      p[i].val = i < n ? i : SentinelVal;
   }
}

//Checks that p holds the elements of range 1 with the indexes in idx
//and that nothing was written after them.
template<class T>
void check_elements
   (const T *p, const std::size_t *idx, std::size_t n, const std::size_t *keys1)
{
   for(std::size_t i = 0; i != n; ++i){
      BOOST_TEST(p[i].key == keys1[idx[i]]);
      BOOST_TEST(p[i].val == idx[i]);
   }
}

template<class T>
void check_sentinels(const T *p, std::size_t n)
{
   for(std::size_t i = n; i != ArraySize; ++i){
      BOOST_TEST(p[i].key == SentinelKey);
      BOOST_TEST(p[i].val == SentinelVal);
   }
}

//Checks that the elements of range 1 marked in moved are moved-from
//and that the other elements keep their original value.
template<class T>
void check_sources
   (const T *p, const bool *moved, std::size_t n1, const std::size_t *keys1)
{
   for(std::size_t i = 0; i != n1; ++i){
      if(moved[i]){
         BOOST_TEST(is_moved_from(p[i]));
      }
      else{
         BOOST_TEST(p[i].key == keys1[i]);
         BOOST_TEST(p[i].val == i);
      }
   }
   check_sentinels(p, n1);
}

//Checks the part of range 1 that an in place algorithm does not keep:
//each element is moved-from or an original element of range 1, and no
//element of range 1 appears twice in range 1.
template<class T>
void check_no_duplicates(const T *p, std::size_t n1, const std::size_t *keys1)
{
   bool seen[MaxSize] = {};
   for(std::size_t i = 0; i != n1; ++i){
      if(!is_moved_from(p[i])){
         BOOST_TEST(p[i].val < n1);
         if(p[i].val < n1){
            BOOST_TEST(p[i].key == keys1[p[i].val]);
            BOOST_TEST(!seen[p[i].val]);
            seen[p[i].val] = true;
         }
      }
   }
   check_sentinels(p, n1);
}

template<class T>
void check_moved_from(const T *p, std::size_t n)
{
   for(std::size_t i = 0; i != n; ++i){
      BOOST_TEST(is_moved_from(p[i]));
   }
}

//The partition algorithms return a duo. This function deduces the type
//so the test does not depend on the return type.
template<class Duo, class T>
void check_partition_ret(const Duo &ret, const T *first, const T *out)
{
   BOOST_TEST(&*ret.first == first);
   BOOST_TEST(&*ret.second == out);
}

//Comparison bounds. The bound of the partition algorithms is documented.
//The bounds of the other algorithms are not documented: they are a
//regression check. set_difference does at most 2 comparisons for each
//step in range 1 or range 2. set_unique_difference also compares each
//element of range 1 with the first element of its group.
inline std::size_t max_compare_difference(std::size_t n1, std::size_t n2)
{
   return 2u*(n1 + n2);
}

inline std::size_t max_compare_unique_difference(std::size_t n1, std::size_t n2)
{
   return 3u*n1 + 2u*n2;
}

inline std::size_t max_compare_partition(std::size_t n1, std::size_t n2, std::size_t num_groups)
{
   return n1 ? n1 + n2 + 2u*num_groups - 1u : 0u;
}

//////////////////////////////////////////////////////////////////////////////
//Tests for each algorithm

template<class T, bool UseFwdIterator>
void test_set_difference
   ( const std::size_t *keys1, std::size_t n1, const std::size_t *keys2, std::size_t n2
   , const reference_result &ref, const bool *moved)
{
   typedef iterators<T, UseFwdIterator> its;
   T range1[ArraySize];
   T range2[ArraySize];
   T out[ArraySize];
   fill_range(range1, keys1, n1);
   fill_range(range2, keys2, n2);
   fill_range(out, keys1, 0u);
   const T *const r2 = range2;
   reset_copies(r2);
   counted_less<T>::num_compare = 0u;
   typename its::it1_t ret = boost::movelib::set_difference
      ( source(its::it1(range1), source_tag(r2))
      , source(its::it1(range1 + n1), source_tag(r2))
      , its::it2(r2), its::it2(r2 + n2), its::it1(out), counted_less<T>());
   BOOST_TEST(its::base(ret) == out + ref.diff_size);
   check_elements(out, ref.diff, ref.diff_size, keys1);
   check_sentinels(out, ref.diff_size);
   check_sources(range1, moved, n1, keys1);
   check_copies(r2, ref.diff_size);
   BOOST_TEST(counted_less<T>::num_compare <= max_compare_difference(n1, n2));
}

template<bool UseFwdIterator>
void test_inplace_set_difference
   ( const std::size_t *keys1, std::size_t n1, const std::size_t *keys2, std::size_t n2
   , const reference_result &ref)
{
   typedef order_move_type T;
   typedef iterators<T, UseFwdIterator> its;
   T range1[ArraySize];
   T range2[ArraySize];
   fill_range(range1, keys1, n1);
   fill_range(range2, keys2, n2);
   const T *const r2 = range2;
   counted_less<T>::num_compare = 0u;
   typename its::it1_t ret = boost::movelib::inplace_set_difference
      ( its::it1(range1), its::it1(range1 + n1)
      , its::it2(r2), its::it2(r2 + n2), counted_less<T>());
   BOOST_TEST(its::base(ret) == range1 + ref.diff_size);
   check_elements(range1, ref.diff, ref.diff_size, keys1);
   check_no_duplicates(range1, n1, keys1);
   BOOST_TEST(counted_less<T>::num_compare <= max_compare_difference(n1, n2));
}

template<class T, bool UseFwdIterator>
void test_set_unique_difference
   ( const std::size_t *keys1, std::size_t n1, const std::size_t *keys2, std::size_t n2
   , const reference_result &ref, const bool *moved)
{
   typedef iterators<T, UseFwdIterator> its;
   T range1[ArraySize];
   T range2[ArraySize];
   T out[ArraySize];
   fill_range(range1, keys1, n1);
   fill_range(range2, keys2, n2);
   fill_range(out, keys1, 0u);
   const T *const r2 = range2;
   reset_copies(r2);
   counted_less<T>::num_compare = 0u;
   typename its::it1_t ret = boost::movelib::set_unique_difference
      ( source(its::it1(range1), source_tag(r2))
      , source(its::it1(range1 + n1), source_tag(r2))
      , its::it2(r2), its::it2(r2 + n2), its::it1(out), counted_less<T>());
   BOOST_TEST(its::base(ret) == out + ref.d_size);
   check_elements(out, ref.d, ref.d_size, keys1);
   check_sentinels(out, ref.d_size);
   check_sources(range1, moved, n1, keys1);
   check_copies(r2, ref.d_size);
   BOOST_TEST(counted_less<T>::num_compare <= max_compare_unique_difference(n1, n2));
}

template<bool UseFwdIterator>
void test_inplace_set_unique_difference
   ( const std::size_t *keys1, std::size_t n1, const std::size_t *keys2, std::size_t n2
   , const reference_result &ref)
{
   typedef order_move_type T;
   typedef iterators<T, UseFwdIterator> its;
   T range1[ArraySize];
   T range2[ArraySize];
   fill_range(range1, keys1, n1);
   fill_range(range2, keys2, n2);
   const T *const r2 = range2;
   counted_less<T>::num_compare = 0u;
   typename its::it1_t ret = boost::movelib::inplace_set_unique_difference
      ( its::it1(range1), its::it1(range1 + n1)
      , its::it2(r2), its::it2(r2 + n2), counted_less<T>());
   BOOST_TEST(its::base(ret) == range1 + ref.d_size);
   check_elements(range1, ref.d, ref.d_size, keys1);
   check_no_duplicates(range1, n1, keys1);
   BOOST_TEST(counted_less<T>::num_compare <= max_compare_unique_difference(n1, n2));
}

template<bool UseFwdIterator>
void test_set_unique_difference_partition
   ( const std::size_t *keys1, std::size_t n1, const std::size_t *keys2, std::size_t n2
   , const reference_result &ref)
{
   typedef order_move_type T;
   typedef iterators<T, UseFwdIterator> its;
   T range1[ArraySize];
   T range2[ArraySize];
   T out[ArraySize];
   fill_range(range1, keys1, n1);
   fill_range(range2, keys2, n2);
   fill_range(out, keys1, 0u);
   const T *const r2 = range2;
   counted_less<T>::num_compare = 0u;
   check_partition_ret
      ( boost::movelib::set_unique_difference_partition
         ( its::it1(range1), its::it1(range1 + n1)
         , its::it2(r2), its::it2(r2 + n2), its::it1(out), counted_less<T>())
      , range1 + ref.r_size, out + ref.d_size);
   check_elements(out, ref.d, ref.d_size, keys1);
   check_sentinels(out, ref.d_size);
   check_elements(range1, ref.r, ref.r_size, keys1);
   check_moved_from(range1 + ref.r_size, n1 - ref.r_size);
   check_sentinels(range1, n1);
   BOOST_TEST(counted_less<T>::num_compare <= max_compare_partition(n1, n2, ref.num_groups));
}

template<bool UseFwdIterator>
void test_inplace_set_unique_difference_partition
   ( const std::size_t *keys1, std::size_t n1, const std::size_t *keys2, std::size_t n2
   , const reference_result &ref)
{
   typedef order_move_type T;
   typedef iterators<T, UseFwdIterator> its;
   T range1[ArraySize];
   T range2[ArraySize];
   T removed[ArraySize];
   fill_range(range1, keys1, n1);
   fill_range(range2, keys2, n2);
   fill_range(removed, keys1, 0u);
   const T *const r2 = range2;
   counted_less<T>::num_compare = 0u;
   check_partition_ret
      ( boost::movelib::inplace_set_unique_difference_partition
         ( its::it1(range1), its::it1(range1 + n1)
         , its::it2(r2), its::it2(r2 + n2), its::it1(removed), counted_less<T>())
      , range1 + ref.d_size, removed + ref.r_size);
   check_elements(range1, ref.d, ref.d_size, keys1);
   check_moved_from(range1 + ref.d_size, n1 - ref.d_size);
   check_sentinels(range1, n1);
   check_elements(removed, ref.r, ref.r_size, keys1);
   check_sentinels(removed, ref.r_size);
   BOOST_TEST(counted_less<T>::num_compare <= max_compare_partition(n1, n2, ref.num_groups));
}

//////////////////////////////////////////////////////////////////////////////
//Runs all the algorithms with the sorted keys

template<bool UseFwdIterator>
void test_all_algorithms
   ( const std::size_t *keys1, std::size_t n1, const std::size_t *keys2, std::size_t n2
   , const reference_result &ref)
{
   const bool not_moved[MaxSize] = {};
   test_set_difference<order_perf_type, UseFwdIterator>(keys1, n1, keys2, n2, ref, not_moved);
   test_set_difference<order_move_type, UseFwdIterator>(keys1, n1, keys2, n2, ref, ref.in_diff);
   test_inplace_set_difference<UseFwdIterator>(keys1, n1, keys2, n2, ref);
   test_set_unique_difference<order_perf_type, UseFwdIterator>(keys1, n1, keys2, n2, ref, not_moved);
   test_set_unique_difference<order_move_type, UseFwdIterator>(keys1, n1, keys2, n2, ref, ref.in_d);
   test_inplace_set_unique_difference<UseFwdIterator>(keys1, n1, keys2, n2, ref);
   test_set_unique_difference_partition<UseFwdIterator>(keys1, n1, keys2, n2, ref);
   test_inplace_set_unique_difference_partition<UseFwdIterator>(keys1, n1, keys2, n2, ref);
}

void test_case(const std::size_t *keys1, std::size_t n1, const std::size_t *keys2, std::size_t n2)
{
   reference_result ref;
   reference_set_difference(keys1, n1, keys2, n2, ref);
   test_all_algorithms<false>(keys1, n1, keys2, n2, ref);
   test_all_algorithms<true> (keys1, n1, keys2, n2, ref);
}

template<std::size_t N1, std::size_t N2>
void test_keys(const std::size_t (&keys1)[N1], const std::size_t (&keys2)[N2])
{
   test_case(keys1, N1, keys2, N2);
}

//Tests the keys as range 1 and as range 2, with an empty range
template<std::size_t N>
void test_keys_one_empty(const std::size_t (&keys)[N])
{
   test_case(keys, N, 0, 0u);
   test_case(0, 0u, keys, N);
}

//////////////////////////////////////////////////////////////////////////////
//Fixed cases

void test_fixed()
{
   //Range 2 of the hand-written cases: 0, 2, 4, ..., 18
   const std::size_t even[] = { 0, 2, 4, 6, 8, 10, 12, 14, 16, 18 };

   //Empty ranges
   test_case(0, 0u, 0, 0u);
   {
      const std::size_t k1[] = { 1, 2, 2, 3 };
      test_keys_one_empty(k1);
      test_keys_one_empty(even);
   }
   //Some elements found in range 2
   {
      const std::size_t k1[] = { 0, 1, 3, 4 };
      const std::size_t k2[] = { 0, 1, 1, 3, 4 };
      const std::size_t k3[] = { 0, 1, 1, 3, 4, 4, 21, 21, 23, 23 };
      test_keys(k1, even);
      test_keys(k2, even);
      test_keys(k3, even);
   }
   //All elements of range 1 found in range 2
   {
      const std::size_t k1[] = { 0, 2, 4, 6 };
      const std::size_t k2[] = { 0, 2, 2, 4, 6 };
      const std::size_t k3[] = { 0, 0, 0, 2, 2, 4, 6, 6, 6, 6, 6 };
      test_keys(k1, even);
      test_keys(k2, even);
      test_keys(k3, even);
   }
   //No element of range 1 found in range 2
   {
      const std::size_t k1[] = { 1, 3, 5, 7 };
      const std::size_t k2[] = { 1, 3, 5, 7, 7 };
      const std::size_t k3[] = { 1, 3, 3, 5, 7, 7, 7 };
      const std::size_t k4[] = { 1, 1, 3, 3, 5, 7, 7, 7 };
      const std::size_t k5[] = { 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21 };
      const std::size_t k6[] = { 1, 1, 3, 3, 5, 5, 7, 7, 9, 9, 11, 11, 13, 13, 15, 15, 17, 17, 19, 19, 21, 21 };
      test_keys(k1, even);
      test_keys(k2, even);
      test_keys(k3, even);
      test_keys(k4, even);
      test_keys(k5, even);
      test_keys(k6, even);
      test_keys(even, k5);
   }
   //Found elements only at the start or only at the end
   {
      const std::size_t k1[] = { 0, 2, 4, 5, 7 };
      const std::size_t k2[] = { 0, 2, 4, 4, 5, 7 };
      const std::size_t k3[] = { 1, 3, 4, 6, 8 };
      const std::size_t k4[] = { 1, 3, 4, 4, 6, 8, 8, 8 };
      test_keys(k1, even);
      test_keys(k2, even);
      test_keys(k3, even);
      test_keys(k4, even);
   }
   //Range 1 before and after range 2
   {
      const std::size_t k1[] = { 1, 2, 3 };
      const std::size_t k2[] = { 10, 11 };
      test_keys(k1, k2);
      test_keys(k2, k1);
   }
   //All elements equivalent
   {
      const std::size_t k1[] = { 4, 4, 4, 4 };
      const std::size_t k2[] = { 4, 4 };
      const std::size_t k3[] = { 3 };
      const std::size_t k4[] = { 5 };
      test_keys(k1, k2);
      test_keys(k2, k1);
      test_keys(k1, k3);
      test_keys(k1, k4);
      test_keys_one_empty(k1);
   }
   //Range 1 equal to range 2
   {
      const std::size_t k1[] = { 1, 2, 3, 4 };
      const std::size_t k2[] = { 1, 1, 2, 2, 3, 3 };
      test_keys(k1, k1);
      test_keys(k2, k2);
   }
   //Groups at the start, in the middle and at the end
   {
      const std::size_t k1[] = { 1, 1, 1, 2, 3, 5, 5, 6, 8, 9, 9 };
      const std::size_t k2[] = { 2, 4, 6 };
      const std::size_t k3[] = { 1, 5, 9 };
      const std::size_t k4[] = { 0, 3, 8, 10 };
      const std::size_t k5[] = { 1, 1, 5, 9, 9, 9 };
      test_keys(k1, k2);
      test_keys(k1, k3);
      test_keys(k1, k4);
      test_keys(k1, k5);
   }
   //Range 2 longer than range 1
   {
      const std::size_t k1[] = { 3, 3, 7 };
      const std::size_t k2[] = { 0, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
      test_keys(k1, k2);
   }
   //Range 2 shorter than range 1 and with equivalent elements
   {
      const std::size_t k1[] = { 0, 1, 2, 2, 3, 4, 5, 6, 7, 7, 8 };
      const std::size_t k2[] = { 2, 2, 2, 7 };
      test_keys(k1, k2);
   }
   //In place fast paths: kept elements only at the start, kept elements
   //only at the end, and nothing removed. A self-move assignment
   //of order_move_type asserts.
   {
      const std::size_t k1[] = { 1, 2, 3, 4, 5, 6 };
      const std::size_t k2[] = { 4, 5, 6 };
      const std::size_t k3[] = { 1, 2, 3 };
      const std::size_t k4[] = { 0, 7 };
      test_keys(k1, k2);
      test_keys(k1, k3);
      test_keys(k1, k4);
   }
}

//////////////////////////////////////////////////////////////////////////////
//Random cases compared with the reference implementation

void test_random()
{
   for(unsigned long seed = 1u; seed != 41u; ++seed){
      test_rand rnd(seed);
      for(std::size_t iter = 0; iter != 200u; ++iter){
         std::size_t keys1[MaxSize];
         std::size_t keys2[MaxSize];
         const std::size_t n1 = rnd(MaxSize + 1u);
         const std::size_t n2 = rnd(MaxSize + 1u);
         //Small key steps produce many equivalent elements
         const std::size_t max_step = 2u + rnd(MaxSize/4u);
         std::size_t k = rnd(4u);
         for(std::size_t i = 0; i != n1; ++i){
            k += rnd(3u) ? rnd(max_step) : 0u;
            keys1[i] = k;
         }
         k = rnd(4u);
         for(std::size_t i = 0; i != n2; ++i){
            k += rnd(3u) ? rnd(max_step) : 0u;
            keys2[i] = k;
         }
         test_case(keys1, n1, keys2, n2);
      }
   }
}

int main()
{
   test_fixed();
   test_random();
   return boost::report_errors();
}
