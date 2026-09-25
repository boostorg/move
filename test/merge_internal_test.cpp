//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2026-2026.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////

// Tests the merge building blocks of <boost/move/algo/detail/merge.hpp>

#include "internal_test_util.hpp"

#include <boost/config.hpp>

#include <boost/move/algo/detail/merge.hpp>
#include <boost/move/algo/predicate.hpp>
#include <boost/move/core.hpp>
#include <boost/move/unique_ptr.hpp>
#include <boost/move/make_unique.hpp>
#include <boost/move/detail/force_ptr.hpp>

#include <boost/core/lightweight_test.hpp>

#include <algorithm>   //std::merge, std::equal, std::transform
#include <cstddef>

using boost::movelib::move_op;
using boost::movelib::swap_op;
using boost::movelib::antistable;

//How many buffer lengths are tried
static const std::size_t BufferSlackCases = 3u;

//A swap loses no value, so after a swap_op merge the buffer must still hold what it held.
//(A function, not a constant condition, to avoid MSVC warning C4127)
template<class Op>
bool keeps_buffer_values(Op)
{  return false;  }

inline bool keeps_buffer_values(swap_op)
{  return true;  }

//Splits the "n" ordered positions in two sorted ranges following "mask"
struct merge_case
{
   void build(std::size_t n, unsigned long mask, key_pattern pattern)
   {
      len1 = len2 = 0u;
      for(std::size_t pos = 0u; pos != n; ++pos){
         if(mask & (1ul << pos)){
            range1[len1].key = key_of(pos, pattern);
            range1[len1].val = len1;
            ++len1;
         }
         else{
            range2[len2].key = key_of(pos, pattern);
            range2[len2].val = Range2ValBase + len2;
            ++len2;
         }
      }
   }

   //The result the algorithm must produce
   void expected(kv *out, bool range1_first) const
   {
      if(range1_first){
         std::merge(range1, range1+len1, range2, range2+len2, out, less_type());
      }
      else{
         std::merge(range2, range2+len2, range1, range1+len1, out, less_type());
      }
   }

   kv range1[MaxLen];
   kv range2[MaxLen];
   std::size_t len1;
   std::size_t len2;
};

///////////////////////////////////////////////////////////////////////////////
//
//                              op_merge_left
//
//    [  buffer  ][ range 1 ][ range 2 ]  ->  [   merged   ][  buffer  ]
//
///////////////////////////////////////////////////////////////////////////////

template<class Op, class Compare>
void test_op_merge_left_one(const merge_case &c, std::size_t l_buf, Op op, Compare comp, bool range1_first)
{
   std::size_t const n_data = c.len1 + c.len2;
   std::size_t const n      = l_buf + n_data;

   guarded_array arr(n);
   test_type *const p = arr.data();
   fill_buffer(p, l_buf);
   fill_from(p+l_buf, c.range1, c.len1);
   fill_from(p+l_buf+c.len1, c.range2, c.len2);

   boost::movelib::op_merge_left(p, p+l_buf, p+l_buf+c.len1, p+n, comp, op);

   kv exp[2u*MaxLen];
   c.expected(exp, range1_first);
   BOOST_TEST(std::equal(p, p+n_data, exp, same_element()));
   BOOST_TEST(arr.guards_intact());
   if(keeps_buffer_values(op)){
      BOOST_TEST(is_buffer_permutation(p+n_data, l_buf));
   }
}

void test_op_merge_left()
{
   less_type comp;
   antistable<less_type> acomp(comp);

   for(std::size_t n = 0u; n <= MaxLen; ++n){
      for(unsigned long mask = 0u; mask != (1ul << n); ++mask){
         for(unsigned pat = 0u; pat != max_key_pattern; ++pat){
            merge_case c;
            c.build(n, mask, key_pattern(pat));
            //op_merge_left needs the buffer to be at least as long as range 2.
            for(std::size_t extra = 0u; extra != BufferSlackCases; ++extra){
               std::size_t const l_buf = c.len2 + extra;
               if(!l_buf){
                  continue;      //No buffer, nothing to merge into
               }
               test_op_merge_left_one(c, l_buf, move_op(), comp, true);
               test_op_merge_left_one(c, l_buf, swap_op(), comp, true);
               test_op_merge_left_one(c, l_buf, move_op(), acomp, false);
               test_op_merge_left_one(c, l_buf, swap_op(), acomp, false);
            }
         }
      }
   }
}

///////////////////////////////////////////////////////////////////////////////
//
//                              op_merge_right
//
//    [ range 1 ][ range 2 ][  buffer  ]  ->  [  buffer  ][   merged   ]
//
///////////////////////////////////////////////////////////////////////////////

template<class Op, class Compare>
void test_op_merge_right_one(const merge_case &c, std::size_t l_buf, Op op, Compare comp, bool range1_first)
{
   std::size_t const n_data = c.len1 + c.len2;
   std::size_t const n      = n_data + l_buf;

   guarded_array arr(n);
   test_type *const p = arr.data();
   fill_from(p, c.range1, c.len1);
   fill_from(p+c.len1, c.range2, c.len2);
   fill_buffer(p+n_data, l_buf);

   boost::movelib::op_merge_right(p, p+c.len1, p+n_data, p+n, comp, op);

   kv exp[2u*MaxLen];
   c.expected(exp, range1_first);
   BOOST_TEST(std::equal(p+l_buf, p+l_buf+n_data, exp, same_element()));
   BOOST_TEST(arr.guards_intact());
   if(keeps_buffer_values(op)){
      BOOST_TEST(is_buffer_permutation(p, l_buf));
   }
}

void test_op_merge_right()
{
   less_type comp;
   antistable<less_type> acomp(comp);

   for(std::size_t n = 0u; n <= MaxLen; ++n){
      for(unsigned long mask = 0u; mask != (1ul << n); ++mask){
         for(unsigned pat = 0u; pat != max_key_pattern; ++pat){
            merge_case c;
            c.build(n, mask, key_pattern(pat));
            //Mirror of op_merge_left: the buffer must hold range 1
            for(std::size_t extra = 0u; extra != BufferSlackCases; ++extra){
               std::size_t const l_buf = c.len1 + extra;
               if(!l_buf){
                  continue;
               }
               test_op_merge_right_one(c, l_buf, move_op(), comp, true);
               test_op_merge_right_one(c, l_buf, swap_op(), comp, true);
               test_op_merge_right_one(c, l_buf, move_op(), acomp, false);
               test_op_merge_right_one(c, l_buf, swap_op(), acomp, false);
            }
         }
      }
   }
}

///////////////////////////////////////////////////////////////////////////////
//
//                     op_merge_with_right_placed
//
//    range 1 elsewhere,   [   free   ][ range 2 ]  ->  [    merged    ]
//
///////////////////////////////////////////////////////////////////////////////

template<class Op, class Compare>
void test_op_merge_with_right_placed_one(const merge_case &c, Op op, Compare comp, bool range1_first)
{
   std::size_t const n = c.len1 + c.len2;

   guarded_array dest(n), outside(c.len1);
   test_type *const d = dest.data();
   test_type *const o = outside.data();

   fill_from(o, c.range1, c.len1);
   fill_buffer(d, c.len1);               //The free run, overwritten by the merge
   fill_from(d+c.len1, c.range2, c.len2);

   boost::movelib::op_merge_with_right_placed(o, o+c.len1, d, d+c.len1, d+n, comp, op);

   kv exp[2u*MaxLen];
   c.expected(exp, range1_first);
   BOOST_TEST(std::equal(d, d+n, exp, same_element()));
   BOOST_TEST(dest.guards_intact());
   BOOST_TEST(outside.guards_intact());
}

//    [ range 1 ][   free   ],   range 2 elsewhere  ->  [    merged    ]
template<class Op, class Compare>
void test_op_merge_with_left_placed_one(const merge_case &c, Op op, Compare comp, bool range1_first)
{
   std::size_t const n = c.len1 + c.len2;

   guarded_array dest(n), outside(c.len2);
   test_type *const d = dest.data();
   test_type *const o = outside.data();

   fill_from(d, c.range1, c.len1);
   fill_buffer(d+c.len1, c.len2);        //The free run, overwritten by the merge
   fill_from(o, c.range2, c.len2);

   boost::movelib::op_merge_with_left_placed(d, d+c.len1, d+n, o, o+c.len2, comp, op);

   kv exp[2u*MaxLen];
   c.expected(exp, range1_first);
   BOOST_TEST(std::equal(d, d+n, exp, same_element()));
   BOOST_TEST(dest.guards_intact());
   BOOST_TEST(outside.guards_intact());
}

void test_op_merge_with_placed()
{
   less_type comp;
   antistable<less_type> acomp(comp);

   for(std::size_t n = 0u; n <= MaxLen; ++n){
      for(unsigned long mask = 0u; mask != (1ul << n); ++mask){
         for(unsigned pat = 0u; pat != max_key_pattern; ++pat){
            merge_case c;
            c.build(n, mask, key_pattern(pat));
            test_op_merge_with_right_placed_one(c, move_op(), comp, true);
            test_op_merge_with_right_placed_one(c, swap_op(), comp, true);
            test_op_merge_with_right_placed_one(c, move_op(), acomp, false);
            test_op_merge_with_left_placed_one (c, move_op(), comp, true);
            test_op_merge_with_left_placed_one (c, swap_op(), comp, true);
            test_op_merge_with_left_placed_one (c, move_op(), acomp, false);
         }
      }
   }
}

///////////////////////////////////////////////////////////////////////////////
//
//                 uninitialized_merge_with_right_placed
//
//    The free run in front of range 2 is raw memory here, so the merge has to
//    construct the elements it puts there and destroy them again if something
//    throws. order_perf_type counts the objects that are alive, which is what
//    tells a leak from a double destruction.
//
///////////////////////////////////////////////////////////////////////////////

template<class Compare>
void test_uninitialized_merge_one(const merge_case &c, Compare comp, bool range1_first)
{
   std::size_t const n = c.len1 + c.len2;
   if(!n){
      return;
   }

   boost::ulong_long_type const live_before = order_perf_type::num_elements;

   //Raw storage for the whole destination. Only the range 2 part is constructed
   boost::movelib::unique_ptr<char[]> raw(new char[sizeof(order_perf_type)*n]);
   order_perf_type *const d = boost::move_detail::force_ptr<order_perf_type*>(raw.get());

   boost::movelib::unique_ptr<order_perf_type[]> outside
      (boost::movelib::make_unique<order_perf_type[]>(c.len1));
   order_perf_type *const o = outside.get();
   fill_from(o, c.range1, c.len1);
   for(std::size_t i = 0u; i != c.len2; ++i){
      ::new(static_cast<void*>(d+c.len1+i)) order_perf_type();
   }
   fill_from(d+c.len1, c.range2, c.len2);

   boost::movelib::uninitialized_merge_with_right_placed(o, o+c.len1, d, d+c.len1, d+n, comp);

   kv exp[2u*MaxLen];
   c.expected(exp, range1_first);
   BOOST_TEST(std::equal(d, d+n, exp, same_element()));
   //The whole destination must be constructed now, and nothing else
   BOOST_TEST(order_perf_type::num_elements == live_before + n + c.len1);

   for(std::size_t i = 0u; i != n; ++i){
      d[i].~order_perf_type();
   }
   outside.reset();
   BOOST_TEST(order_perf_type::num_elements == live_before);
}

void test_uninitialized_merge_with_right_placed()
{
   less_type comp;
   antistable<less_type> acomp(comp);

   for(std::size_t n = 0u; n <= MaxLen; ++n){
      for(unsigned long mask = 0u; mask != (1ul << n); ++mask){
         for(unsigned pat = 0u; pat != max_key_pattern; ++pat){
            merge_case c;
            c.build(n, mask, key_pattern(pat));
            test_uninitialized_merge_one(c, comp, true);
            test_uninitialized_merge_one(c, acomp, false);
         }
      }
   }
}

///////////////////////////////////////////////////////////////////////////////
//
//                        op_buffered_merge
//
//    Merges in place with a buffer that holds the shorter half. Both halves
//    are tried as the shorter one, and the buffer is given the smallest
//    capacity the algorithm accepts as well as a larger one.
//
///////////////////////////////////////////////////////////////////////////////

template<class Compare>
void test_buffered_merge_one(const merge_case &c, std::size_t cap, Compare comp, bool range1_first)
{
   std::size_t const n = c.len1 + c.len2;

   guarded_array arr(n);
   test_type *const p = arr.data();
   fill_from(p, c.range1, c.len1);
   fill_from(p+c.len1, c.range2, c.len2);

   boost::movelib::unique_ptr<char[]> raw(new char[sizeof(test_type)*cap]);
   boost::movelib::adaptive_xbuf<test_type, test_type*, std::size_t> xbuf
      (boost::move_detail::force_ptr<test_type*>(raw.get()), cap);

   boost::movelib::op_buffered_merge(p, p+c.len1, p+n, comp, move_op(), xbuf);
   xbuf.clear();

   kv exp[2u*MaxLen];
   c.expected(exp, range1_first);
   BOOST_TEST(std::equal(p, p+n, exp, same_element()));
   BOOST_TEST(arr.guards_intact());
}

void test_buffered_merge()
{
   less_type comp;
   antistable<less_type> acomp(comp);

   for(std::size_t n = 0u; n <= MaxLen; ++n){
      for(unsigned long mask = 0u; mask != (1ul << n); ++mask){
         for(unsigned pat = 0u; pat != max_key_pattern; ++pat){
            merge_case c;
            c.build(n, mask, key_pattern(pat));
            //The buffer must hold the shorter half
            std::size_t const l_min = c.len1 < c.len2 ? c.len1 : c.len2;
            for(std::size_t extra = 0u; extra != BufferSlackCases; ++extra){
               test_buffered_merge_one(c, l_min+extra, comp, true);
               test_buffered_merge_one(c, l_min+extra, acomp, false);
            }
         }
      }
   }
}

///////////////////////////////////////////////////////////////////////////////
//
//                  merge_bufferless_ON2 / _ONlogN_recursive
//
//    Both merge in place with no additional memory at all.
//
///////////////////////////////////////////////////////////////////////////////

template<class Compare>
void test_bufferless_one(const merge_case &c, Compare comp, bool range1_first, bool recursive)
{
   std::size_t const n = c.len1 + c.len2;

   guarded_array arr(n);
   test_type *const p = arr.data();
   fill_from(p, c.range1, c.len1);
   fill_from(p+c.len1, c.range2, c.len2);

   if(recursive){
      boost::movelib::merge_bufferless_ONlogN_recursive(p, p+c.len1, p+n, c.len1, c.len2, comp);
   }
   else{
      boost::movelib::merge_bufferless_ON2(p, p+c.len1, p+n, comp);
   }

   kv exp[2u*MaxLen];
   c.expected(exp, range1_first);
   BOOST_TEST(std::equal(p, p+n, exp, same_element()));
   BOOST_TEST(arr.guards_intact());
}

void test_bufferless_merges()
{
   less_type comp;
   antistable<less_type> acomp(comp);

   for(std::size_t n = 0u; n <= MaxLen; ++n){
      for(unsigned long mask = 0u; mask != (1ul << n); ++mask){
         for(unsigned pat = 0u; pat != max_key_pattern; ++pat){
            merge_case c;
            c.build(n, mask, key_pattern(pat));
            test_bufferless_one(c, comp,  true,  false);
            test_bufferless_one(c, acomp, false, false);
            test_bufferless_one(c, comp,  true,  true);
            test_bufferless_one(c, acomp, false, true);
         }
      }
   }
}

//merge_bufferless_ONlogN_recursive only halves the ranges above
//MergeBufferlessONLogNRotationThreshold, so a longer case is needed to reach
//the recursion and the rotation that splits it.
void test_bufferless_recursive_long()
{
   less_type comp;
   std::size_t const n = 200u;

   for(std::size_t len1 = 0u; len1 <= n; len1 += 7u){
      std::size_t const len2 = n - len1;
      guarded_array arr(n);
      test_type *const p = arr.data();
      //Interleave the two halves so that the merge really has work to do
      for(std::size_t i = 0u; i != len1; ++i){
         p[i].key = 2u*i;
         p[i].val = i;
      }
      for(std::size_t i = 0u; i != len2; ++i){
         p[len1+i].key = 2u*i + 1u;
         p[len1+i].val = Range2ValBase + i;
      }

      boost::movelib::unique_ptr<kv[]> exp(boost::movelib::make_unique<kv[]>(n));
      {
         boost::movelib::unique_ptr<kv[]> r1(boost::movelib::make_unique<kv[]>(n));
         boost::movelib::unique_ptr<kv[]> r2(boost::movelib::make_unique<kv[]>(n));
         std::transform(p, p+len1, r1.get(), to_kv());
         std::transform(p+len1, p+len1+len2, r2.get(), to_kv());
         std::merge(r1.get(), r1.get()+len1, r2.get(), r2.get()+len2, exp.get(), comp);
      }

      boost::movelib::merge_bufferless_ONlogN_recursive(p, p+len1, p+n, len1, len2, comp);

      BOOST_TEST(std::equal(p, p+n, exp.get(), same_element()));
      BOOST_TEST(arr.guards_intact());
   }
}

///////////////////////////////////////////////////////////////////////////////
//
//                        merge_adaptive_ONsqrtN
//
//    Merges in place by rotating the elements of range 1 past the data of
//    range 2 they interleave with, in groups of about sqrt(len1).
//
///////////////////////////////////////////////////////////////////////////////

template<class Compare>
void test_merge_adaptive_ONsqrtN_one(const merge_case &c, std::size_t cap, Compare comp, bool range1_first)
{
   std::size_t const n = c.len1 + c.len2;

   guarded_array arr(n);
   test_type *const p = arr.data();
   fill_from(p, c.range1, c.len1);
   fill_from(p+c.len1, c.range2, c.len2);

   //The buffer holds constructed elements
   guarded_array buf(cap);
   fill_buffer(buf.data(), cap);

   boost::movelib::merge_adaptive_ONsqrtN(p, p+c.len1, p+n, comp, buf.data(), cap);

   kv exp[2u*MaxLen];
   c.expected(exp, range1_first);
   BOOST_TEST(std::equal(p, p+n, exp, same_element()));
   BOOST_TEST(arr.guards_intact());
   BOOST_TEST(buf.guards_intact());
}

void test_merge_adaptive_ONsqrtN()
{
   less_type comp;
   antistable<less_type> acomp(comp);

   for(std::size_t n = 2u; n <= MaxLen; ++n){
      for(unsigned long mask = 0u; mask != (1ul << n); ++mask){
         for(unsigned pat = 0u; pat != max_key_pattern; ++pat){
            merge_case c;
            c.build(n, mask, key_pattern(pat));
            if(!c.len1 || !c.len2){
               continue;   //Both ranges must be non-empty, that is the precondition
            }
            //A capacity of zero makes this a merge with no additional memory,
            //and one of "n" is more than any group can need
            for(std::size_t cap = 0u; cap <= n; ++cap){
               test_merge_adaptive_ONsqrtN_one(c, cap, comp,  true);
               test_merge_adaptive_ONsqrtN_one(c, cap, acomp, false);
            }
         }
      }
   }
}

//A range of MaxLen elements is merged in two or three groups, which is not
//enough to reach the step where the elements of range 1 that are still
//unmerged are fewer than a whole group, nor the one where a group fits in the
//buffer but the whole of range 1 does not.
void test_merge_adaptive_ONsqrtN_many_groups()
{
   less_type comp;
   std::size_t const n = 300u;
   std::size_t const len1_cases[] = { 1u, 2u, 7u, 31u, 100u, 149u };
   std::size_t const cap_cases[]  = { 0u, 1u, 3u, 8u, 64u };

   for(std::size_t i = 0u; i != sizeof(len1_cases)/sizeof(*len1_cases); ++i){
      std::size_t const len1 = len1_cases[i];
      std::size_t const len2 = n - len1;

      for(std::size_t j = 0u; j != sizeof(cap_cases)/sizeof(*cap_cases); ++j){
         std::size_t const cap = cap_cases[j];

         guarded_array arr(n);
         test_type *const p = arr.data();
         //Spread range 1 over the whole of range 2, so that every group has to
         //be rotated past a part of the data
         for(std::size_t k = 0u; k != len1; ++k){
            p[k].key = k*(len2/len1 + 1u);
            p[k].val = k;
         }
         for(std::size_t k = 0u; k != len2; ++k){
            p[len1+k].key = k;
            p[len1+k].val = Range2ValBase + k;
         }

         guarded_array buf(cap);
         fill_buffer(buf.data(), cap);

         boost::movelib::unique_ptr<kv[]> exp(boost::movelib::make_unique<kv[]>(n));
         {
            boost::movelib::unique_ptr<kv[]> r1(boost::movelib::make_unique<kv[]>(n));
            boost::movelib::unique_ptr<kv[]> r2(boost::movelib::make_unique<kv[]>(n));
            std::transform(p, p+len1, r1.get(), to_kv());
            std::transform(p+len1, p+n, r2.get(), to_kv());
            std::merge(r1.get(), r1.get()+len1, r2.get(), r2.get()+len2, exp.get(), comp);
         }

         boost::movelib::merge_adaptive_ONsqrtN(p, p+len1, p+n, comp, buf.data(), cap);

         BOOST_TEST(std::equal(p, p+n, exp.get(), same_element()));
         BOOST_TEST(arr.guards_intact());
         BOOST_TEST(buf.guards_intact());
      }
   }
}

int main()
{
   test_op_merge_left();
   test_op_merge_right();
   test_op_merge_with_placed();
   test_uninitialized_merge_with_right_placed();
   test_buffered_merge();
   test_bufferless_merges();
   test_bufferless_recursive_long();
   test_merge_adaptive_ONsqrtN();
   test_merge_adaptive_ONsqrtN_many_groups();

   return ::boost::report_errors();
}
