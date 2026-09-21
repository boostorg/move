//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2015-2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////

//#define BOOST_MOVE_ADAPTIVE_SORT_STATS
//#define BOOST_MOVE_ADAPTIVE_SORT_STATS_LEVEL 2

#include <algorithm> //std::inplace_merge
#include <cstdio>    //std::printf
#include <iostream>  //std::cout
#include <boost/container/vector.hpp>  //boost::container::vector

#include <boost/config.hpp>
#include <cstdlib>

#include <boost/move/unique_ptr.hpp>
#include <boost/move/detail/nsec_clock.hpp>
#include <boost/move/detail/force_ptr.hpp>

#include "order_type.hpp"
#include "random_shuffle.hpp"
#include "bench_util.hpp"

using boost::move_detail::cpu_timer;
using boost::move_detail::nanosecond_type;

void print_stats(const char *str, boost::ulong_long_type element_count)
{
   std::printf( "%sCmp:%8.04f Cpy:%9.04f\n", str
              , double(order_perf_type::num_compare)/double(element_count)
              , double(order_perf_type::num_copy)/double(element_count));
}

#include <boost/move/algo/adaptive_merge.hpp>
#include <boost/move/algo/detail/merge.hpp>
#include <boost/move/core.hpp>

//split_count == 0 means an even split. Any other value is used as the length of the
//first range, so that lopsided inputs can be benchmarked: adaptive_merge switches to
//a rotation-based merge once min(len1,len2) <= 2*ceil_sqrt(len), and that regime is
//invisible to an even split.
template<class T, class Compare>
std::size_t generate_elements(boost::container::vector<T> &elements, std::size_t L, std::size_t NK, Compare comp, std::size_t split_count = 0)
{
   elements.resize(L);
   boost::movelib::unique_ptr<std::size_t[]> key_reps(new std::size_t[NK ? NK : L]);

   std::srand(0);
   for (std::size_t i = 0; i < (NK ? NK : L); ++i) {
      key_reps[i] = 0;
   }
   for (std::size_t i = 0; i < L; ++i) {
      std::size_t  key = NK ? (i % NK) : i;
      elements[i].key = key;
   }
   ::random_shuffle(elements.data(), elements.data() + L);
   ::random_shuffle(elements.data(), elements.data() + L);

   for (std::size_t i = 0; i < L; ++i) {
      elements[i].val = key_reps[elements[i].key]++;
   }
   if (!split_count || split_count >= L) {
      split_count = L / 2;
   }
   std::stable_sort(elements.data(), elements.data() + split_count, comp);
   std::stable_sort(elements.data() + split_count, elements.data() + L, comp);
   return split_count;
}

template<class T, class Compare>
void adaptive_merge_buffered(T *elements, T *mid, T *last, Compare comp, std::size_t BufLen)
{
   boost::movelib::unique_ptr<char[]> mem(new char[sizeof(T)*BufLen]);
   boost::movelib::adaptive_merge(elements, mid, last, comp, boost::move_detail::force_ptr<T*>(mem.get()), BufLen);
}

//Same as above but with no stack buffer, to measure what the stack buffer is worth
template<class T, class Compare>
void adaptive_merge_buffered_nostack(T *elements, T *mid, T *last, Compare comp, std::size_t BufLen)
{
   boost::movelib::unique_ptr<char[]> mem(new char[sizeof(T)*BufLen]);
   boost::movelib::adaptive_merge<0>(elements, mid, last, comp, boost::move_detail::force_ptr<T*>(mem.get()), BufLen);
}

template<class T, class Compare>
void std_like_adaptive_merge_buffered(T *elements, T *mid, T *last, Compare comp, std::size_t BufLen)
{
   boost::movelib::unique_ptr<char[]> mem(new char[sizeof(T)*BufLen]);
   boost::movelib::merge_adaptive_ONlogN(elements, mid, last, comp, boost::move_detail::force_ptr<T*>(mem.get()), BufLen);
}

enum AlgoType
{
   StdMerge,
   AdaptMerge,
   AdaptMergeNoStk,
   SqrtHAdaptMerge,
   SqrtHAdaptMergeNoStk,
   SqrtAdaptMerge,
   SqrtAdaptMergeNoStk,
   Sqrt2AdaptMerge,
   Sqrt2AdaptMergeNoStk,
   QuartAdaptMerge,
   QuartAdaptMergeNoStk,
   StdInplaceMerge,
   MergeBuflessON2,
   MergeAdaptONsqrtN,
   StdLkSqrtHAdaptMerge,
   StdLkSqrtAdaptMerge,
   StdLkSqrt2AdaptMerge,
   StdLkQuartAdaptMerge,
   MaxMerge
};

const char *AlgoNames [] = { "StdMerge             "
                           , "AdaptMerge           "
                           , "AdaptMergeNoStk      "
                           , "SqrtHAdaptMerge      "
                           , "SqrtHAdaptMergeNoStk "
                           , "SqrtAdaptMerge       "
                           , "SqrtAdaptMergeNoStk  "
                           , "Sqrt2AdaptMerge      "
                           , "Sqrt2AdaptMergeNoStk "
                           , "QuartAdaptMerge      "
                           , "QuartAdaptMergeNoStk "
                           , "StdInplaceMerge      "
                           , "MergeBuflessON2      "
                           , "MergeAdaptONsqrtN    "
                           , "StdLkSqrtHAdaptMerge "
                           , "StdLkSqrtAdaptMerge  "
                           , "StdLkSqrt2AdaptMerge "
                           , "StdLkQuartAdaptMerge "
                           };

BOOST_MOVE_STATIC_ASSERT((sizeof(AlgoNames)/sizeof(*AlgoNames)) == MaxMerge);

template<class T>
void run_merge_algo(T *elements, std::size_t element_count, std::size_t split_pos, std::size_t alg)
{
   switch(alg)
   {
      case StdMerge:
         std::inplace_merge(elements, elements+split_pos, elements+element_count, order_type_less());
      break;
      case AdaptMerge:
         boost::movelib::adaptive_merge(elements, elements+split_pos, elements+element_count, order_type_less());
      break;
      case AdaptMergeNoStk:
         boost::movelib::adaptive_merge<0>(elements, elements+split_pos, elements+element_count, order_type_less());
      break;
      case SqrtHAdaptMerge:
         adaptive_merge_buffered( elements, elements+split_pos, elements+element_count, order_type_less()
                            , boost::movelib::detail_adaptive::ceil_sqrt_multiple(element_count)/2+1);
      break;
      case SqrtHAdaptMergeNoStk:
         adaptive_merge_buffered_nostack( elements, elements+split_pos, elements+element_count, order_type_less()
                            , boost::movelib::detail_adaptive::ceil_sqrt_multiple(element_count)/2+1);
      break;
      case SqrtAdaptMerge:
         adaptive_merge_buffered( elements, elements+split_pos, elements+element_count, order_type_less()
                            , boost::movelib::detail_adaptive::ceil_sqrt_multiple(element_count));
      break;
      case SqrtAdaptMergeNoStk:
         adaptive_merge_buffered_nostack( elements, elements+split_pos, elements+element_count, order_type_less()
                            , boost::movelib::detail_adaptive::ceil_sqrt_multiple(element_count));
      break;
      case Sqrt2AdaptMerge:
         adaptive_merge_buffered( elements, elements+split_pos, elements+element_count, order_type_less()
                            , 2*boost::movelib::detail_adaptive::ceil_sqrt_multiple(element_count));
      break;
      case Sqrt2AdaptMergeNoStk:
         adaptive_merge_buffered_nostack( elements, elements+split_pos, elements+element_count, order_type_less()
                            , 2*boost::movelib::detail_adaptive::ceil_sqrt_multiple(element_count));
      break;
      case QuartAdaptMerge:
         adaptive_merge_buffered( elements, elements+split_pos, elements+element_count, order_type_less()
                            , (element_count)/4+1);
      break;
      case QuartAdaptMergeNoStk:
         adaptive_merge_buffered_nostack( elements, elements+split_pos, elements+element_count, order_type_less()
                            , (element_count)/4+1);
      break;
      case StdInplaceMerge:
         boost::movelib::merge_bufferless_ONlogN(elements, elements+split_pos, elements+element_count, order_type_less());
      break;
      case MergeBuflessON2:
         boost::movelib::merge_bufferless_ON2(elements, elements+split_pos, elements+element_count, order_type_less());
      break;
      case MergeAdaptONsqrtN:
         //Both halves must be non-empty, and no external buffer is given, so
         //this measures the group rotations with no additional memory at all
         if(split_pos && split_pos != element_count){
            boost::movelib::merge_adaptive_ONsqrtN
               ( elements, elements+split_pos, elements+element_count, order_type_less()
               , elements, 0u);
         }
      break;
      case StdLkSqrtHAdaptMerge:
         std_like_adaptive_merge_buffered( elements, elements+split_pos, elements+element_count, order_type_less()
                            , boost::movelib::detail_adaptive::ceil_sqrt_multiple(element_count)/2+1);
      break;
      case StdLkSqrtAdaptMerge:
         std_like_adaptive_merge_buffered( elements, elements+split_pos, elements+element_count, order_type_less()
                            , boost::movelib::detail_adaptive::ceil_sqrt_multiple(element_count));
      break;
      case StdLkSqrt2AdaptMerge:
         std_like_adaptive_merge_buffered( elements, elements+split_pos, elements+element_count, order_type_less()
                            , 2*boost::movelib::detail_adaptive::ceil_sqrt_multiple(element_count));
      break;
      case StdLkQuartAdaptMerge:
         std_like_adaptive_merge_buffered( elements, elements+split_pos, elements+element_count, order_type_less()
                            , (element_count)/4+1);
      break;
   }
}

//Restores the input and runs one algorithm on it, so that the timing loop can
//repeat the measurement
template<class T>
struct merge_runner
{
   merge_runner(std::size_t n, std::size_t sp, std::size_t a)
      : element_count(n), split_pos(sp), alg(a)
      , num_compare(0), num_copy(0), num_elements(0)
   {}

   void reset()
   {
      order_perf_type::num_compare = 0;
      order_perf_type::num_copy = 0;
      order_perf_type::num_elements = element_count;
   }

   //Only the first run is measured for counters, the rest add to them
   void record()
   {
      num_compare  = order_perf_type::num_compare;
      num_copy     = order_perf_type::num_copy;
      num_elements = order_perf_type::num_elements;
   }

   void operator()(T *elements) const
   {  run_merge_algo(elements, element_count, split_pos, alg);  }

   std::size_t element_count;
   std::size_t split_pos;
   std::size_t alg;

   boost::ulong_long_type num_compare;
   boost::ulong_long_type num_copy;
   boost::ulong_long_type num_elements;
};

template<class Vector>
bool measure_algo( Vector &elements, const Vector &original, std::size_t element_count
                 , std::size_t split_pos, std::size_t alg, nanosecond_type &prev_clock)
{
   typedef typename Vector::value_type T;
   std::printf("%s ", AlgoNames[alg]);

   merge_runner<T> runner(element_count, split_pos, alg);
   bench_util::bench_result const r = bench_util::measure_best_of(elements, original, runner);

   if(runner.num_elements == element_count){
      std::printf(" Tmp Ok ");
   } else{
      std::printf(" Tmp KO ");
   }

   //The counters are read before the order check, which compares elements too
   std::printf( "Cmp:%8.04f Cpy:%9.04f "
              , double(runner.num_compare)/double(element_count)
              , double(runner.num_copy)/double(element_count) );
   bench_util::print_time(r.best);
   //"n" is the number of runs the best time comes from, and the last value is
   //how far the median is above it, that is, how noisy the measurement was
   std::printf( " (%6.02f) n=%-2u +%.01f%%\n"
              , prev_clock ? double(r.best)/double(prev_clock) : 1.0
              , r.runs
              , r.noise());
   prev_clock = r.best;
   return is_order_type_ordered(elements.data(), element_count, true);
}

template<class T>
bool measure_all(std::size_t L, std::size_t NK, std::size_t split = 0)
{
   boost::container::vector<T> original_elements, elements;
   std::size_t split_pos = generate_elements(original_elements, L, NK, order_type_less(), split);
   std::printf("\n - - N: %u, NK: %u, Len1: %u - -\n", (unsigned)L, (unsigned)NK, (unsigned)split_pos);

   nanosecond_type prev_clock = 0;
   nanosecond_type back_clock;
   bool res = true;

   res = res && measure_algo(elements, original_elements, L, split_pos, StdMerge, prev_clock);
   back_clock = prev_clock;
   //

   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L, split_pos, QuartAdaptMerge, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L, split_pos, QuartAdaptMergeNoStk, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L, split_pos, StdLkQuartAdaptMerge, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L, split_pos, Sqrt2AdaptMerge, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L, split_pos, Sqrt2AdaptMergeNoStk, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L, split_pos, StdLkSqrt2AdaptMerge, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L, split_pos, SqrtAdaptMerge, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L, split_pos, SqrtAdaptMergeNoStk, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L, split_pos, StdLkSqrtAdaptMerge, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L, split_pos, SqrtHAdaptMerge, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L, split_pos, SqrtHAdaptMergeNoStk, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L, split_pos, StdLkSqrtHAdaptMerge, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L, split_pos, AdaptMerge, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L, split_pos, AdaptMergeNoStk, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L, split_pos,StdInplaceMerge, prev_clock);
   //
   //These have quadratic complexity, so they are measured when the min half is short
   std::size_t const l_min = split_pos < (L - split_pos) ? split_pos : (L - split_pos);
   if(l_min && l_min <= 4u*boost::movelib::ceil_sqrt(L)){
      prev_clock = back_clock;
      res = res && measure_algo(elements, original_elements, L, split_pos, MergeBuflessON2, prev_clock);
      //
      prev_clock = back_clock;
      res = res && measure_algo(elements, original_elements, L, split_pos, MergeAdaptONsqrtN, prev_clock);
   }
   //
   if (!res)
      std::abort();
   return res;
}

//Undef it to run the long test
#ifndef LONG_BENCH
#define BENCH_MERGE_SHORT
#endif

#define BENCH_SORT_UNIQUE_VALUES

//Benchmarks around the threshold at which adaptive_merge gives up
//on block merging and rotates instead --> is min(len1,len2) <= 2*ceil_sqrt(len).
template<class T>
bool measure_all_lopsided(std::size_t L, std::size_t NK)
{
   const std::size_t csqrt = boost::movelib::ceil_sqrt(L);
   bool res = true;
   //A quarter of the threshold, the threshold itself, and just past it, so that a
   //change of the rotation-based merge shows up on both sides of the switch.
   const std::size_t len1[] = { csqrt, 2u*csqrt };
   for (std::size_t i = 0; i != sizeof(len1)/sizeof(*len1); ++i) {
      if (!len1[i] || len1[i] >= L/2u) {
         continue;
      }
      res = res && measure_all<T>(L, NK, len1[i]);
   }
   return res;
}

int main()
{
   #ifndef BENCH_SORT_UNIQUE_VALUES
   measure_all<order_perf_type>(101,1);
   measure_all<order_perf_type>(101,5);
   measure_all<order_perf_type>(101,7);
   measure_all<order_perf_type>(101,31);
   #endif
   measure_all<order_perf_type>(101,0);

   //
   #ifndef BENCH_SORT_UNIQUE_VALUES
   measure_all<order_perf_type>(1101,1);
   measure_all<order_perf_type>(1001,7);
   measure_all<order_perf_type>(1001,31);
   measure_all<order_perf_type>(1001,127);
   measure_all<order_perf_type>(1001,511);
   #endif
   measure_all<order_perf_type>(1001,0);

   //
   #ifndef BENCH_SORT_UNIQUE_VALUES
   measure_all<order_perf_type>(10001,65);
   measure_all<order_perf_type>(10001,255);
   measure_all<order_perf_type>(10001,1023);
   measure_all<order_perf_type>(10001,4095);
   #endif
   measure_all<order_perf_type>(10001,0);
   measure_all_lopsided<order_perf_type>(10001,0);

   //
   #if defined(NDEBUG) && !defined(BENCH_MERGE_SHORT)
   #ifndef BENCH_SORT_UNIQUE_VALUES
   measure_all<order_perf_type>(100001,511);
   measure_all<order_perf_type>(100001,2047);
   measure_all<order_perf_type>(100001,8191);
   measure_all<order_perf_type>(100001,32767);
   #endif
   measure_all<order_perf_type>(100001,0);
   measure_all_lopsided<order_perf_type>(100001,0);

   //
   #ifndef BENCH_SORT_UNIQUE_VALUES
   measure_all<order_perf_type>(1000001, 8192);
   measure_all<order_perf_type>(1000001, 32768);
   measure_all<order_perf_type>(1000001, 131072);
   measure_all<order_perf_type>(1000001, 524288);
   #endif
   measure_all<order_perf_type>(1000001,0);
   measure_all_lopsided<order_perf_type>(1000001,0);

   #ifndef BENCH_SORT_UNIQUE_VALUES
   measure_all<order_perf_type>(10000001, 65536);
   measure_all<order_perf_type>(10000001, 262144);
   measure_all<order_perf_type>(10000001, 1048576);
   measure_all<order_perf_type>(10000001, 4194304);
   #endif
   measure_all<order_perf_type>(10000001,0);
   #endif   //#ifdef NDEBUG && !BENCH_MERGE_SHORT

   return 0;
}

