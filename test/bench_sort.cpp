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

#include <cstdlib>   //std::srand
#include <algorithm> //std::stable_sort, std::make|sort_heap, std::random_shuffle
#include <cstdio>    //std::printf
#include <iostream>  //std::cout
#include <boost/container/vector.hpp>  //boost::container::vector

#include <boost/config.hpp>
#include <boost/move/unique_ptr.hpp>
#include <boost/move/detail/nsec_clock.hpp>
#include <boost/move/detail/force_ptr.hpp>
#include <cstdlib>

using boost::move_detail::cpu_timer;
using boost::move_detail::nanosecond_type;

#include "order_type.hpp"
#include "random_shuffle.hpp"
#include "bench_util.hpp"

//#define BOOST_MOVE_ADAPTIVE_SORT_STATS
//#define BOOST_MOVE_ADAPTIVE_SORT_INVARIANTS
void print_stats(const char *str, boost::ulong_long_type element_count)
{
   std::printf( "%sCmp:%7.03f Cpy:%8.03f\n", str
              , double(order_perf_type::num_compare)/double(element_count)
              , double(order_perf_type::num_copy)/double(element_count) );
}


#include <boost/move/algo/adaptive_sort.hpp>
#include <boost/move/algo/detail/merge_sort.hpp>
#include <boost/move/algo/detail/pdqsort.hpp>
#include <boost/move/algo/detail/heap_sort.hpp>
#include <boost/move/core.hpp>

template<class T>
void generate_elements(boost::container::vector<T> &elements, std::size_t L, std::size_t NK)
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
}

template<class T, class Compare>
void adaptive_sort_buffered(T *elements, std::size_t element_count, Compare comp, std::size_t BufLen)
{
   boost::movelib::unique_ptr<char[]> mem(new char[sizeof(T)*BufLen]);
   boost::movelib::adaptive_sort(elements, elements + element_count, comp, boost::move_detail::force_ptr<T*>(mem.get()), BufLen);
}

//Same as above but with no stack buffer, to measure what the stack buffer is worth
template<class T, class Compare>
void adaptive_sort_buffered_nostack(T *elements, std::size_t element_count, Compare comp, std::size_t BufLen)
{
   boost::movelib::unique_ptr<char[]> mem(new char[sizeof(T)*BufLen]);
   boost::movelib::adaptive_sort<0>(elements, elements + element_count, comp, boost::move_detail::force_ptr<T*>(mem.get()), BufLen);
}

template<class T, class Compare>
void std_like_adaptive_stable_sort_buffered(T *elements, std::size_t element_count, Compare comp, std::size_t BufLen)
{
   boost::movelib::unique_ptr<char[]> mem(new char[sizeof(T)*BufLen]);
   boost::movelib::stable_sort_adaptive_ONlogN2(elements, elements + element_count, comp, boost::move_detail::force_ptr<T*>(mem.get()), BufLen);
}

template<class T, class Compare>
void merge_sort_buffered(T *elements, std::size_t element_count, Compare comp)
{
   boost::movelib::unique_ptr<char[]> mem(new char[sizeof(T)*((element_count+1)/2)]);
   boost::movelib::merge_sort(elements, elements + element_count, comp, boost::move_detail::force_ptr<T*>(mem.get()));
}

enum AlgoType
{
   MergeSort,
   StdStableSort,
   PdQsort,
   StdSort,
   AdaptiveSort,
   AdaptiveSortNoStk,
   SqrtHAdaptiveSort,
   SqrtHAdaptiveSortNoStk,
   SqrtAdaptiveSort,
   SqrtAdaptiveSortNoStk,
   Sqrt2AdaptiveSort,
   Sqrt2AdaptiveSortNoStk,
   QuartAdaptiveSort,
   QuartAdaptiveSortNoStk,
   InplaceStableSort,
   StdLkSqrtHAdpSort,
   StdLkSqrtAdpSort,
   StdLkSqrt2AdpSort,
   StdLkQuartAdpSort,
   SlowStableSort,
   HeapSort,
   MaxSort
};

const char *AlgoNames [] = { "MergeSort           "
                           , "StdStableSort       "
                           , "PdQsort             "
                           , "StdSort             "
                           , "AdaptSort           "
                           , "AdaptSortNoStk      "
                           , "SqrtHAdaptSort      "
                           , "SqrtHAdaptSortNoStk "
                           , "SqrtAdaptSort       "
                           , "SqrtAdaptSortNoStk  "
                           , "Sqrt2AdaptSort      "
                           , "Sqrt2AdaptSortNoStk "
                           , "QuartAdaptSort      "
                           , "QuartAdaptSortNoStk "
                           , "InplStableSort      "
                           , "StdLkSqrtHAdpSort   "
                           , "StdLkSqrtAdpSort    "
                           , "StdLkSqrt2AdpSort   "
                           , "StdLkQuartAdpSort   "
                           , "SlowSort            "
                           , "HeapSort            "
                           };

BOOST_MOVE_STATIC_ASSERT((sizeof(AlgoNames)/sizeof(*AlgoNames)) == MaxSort);

template<class T>
void run_sort_algo(T *elements, std::size_t element_count, std::size_t alg)
{
   switch(alg)
   {
      case MergeSort:
         merge_sort_buffered(elements, element_count, order_type_less());
      break;
      case StdStableSort:
         std::stable_sort(elements,elements+element_count,order_type_less());
      break;
      case PdQsort:
         boost::movelib::pdqsort(elements,elements+element_count,order_type_less());
      break;
      case StdSort:
         std::sort(elements,elements+element_count,order_type_less());
      break;
      case AdaptiveSort:
         boost::movelib::adaptive_sort(elements, elements+element_count, order_type_less());
      break;
      case AdaptiveSortNoStk:
         boost::movelib::adaptive_sort<0>(elements, elements+element_count, order_type_less());
      break;
      case SqrtHAdaptiveSort:
         adaptive_sort_buffered( elements, element_count, order_type_less()
                            , boost::movelib::detail_adaptive::ceil_sqrt_multiple(element_count)/2+1);
      break;
      case SqrtHAdaptiveSortNoStk:
         adaptive_sort_buffered_nostack( elements, element_count, order_type_less()
                            , boost::movelib::detail_adaptive::ceil_sqrt_multiple(element_count)/2+1);
      break;
      case SqrtAdaptiveSort:
         adaptive_sort_buffered( elements, element_count, order_type_less()
                            , boost::movelib::detail_adaptive::ceil_sqrt_multiple(element_count));
      break;
      case SqrtAdaptiveSortNoStk:
         adaptive_sort_buffered_nostack( elements, element_count, order_type_less()
                            , boost::movelib::detail_adaptive::ceil_sqrt_multiple(element_count));
      break;
      case Sqrt2AdaptiveSort:
         adaptive_sort_buffered( elements, element_count, order_type_less()
                            , 2*boost::movelib::detail_adaptive::ceil_sqrt_multiple(element_count));
      break;
      case Sqrt2AdaptiveSortNoStk:
         adaptive_sort_buffered_nostack( elements, element_count, order_type_less()
                            , 2*boost::movelib::detail_adaptive::ceil_sqrt_multiple(element_count));
      break;
      case QuartAdaptiveSort:
         adaptive_sort_buffered( elements, element_count, order_type_less()
                            , (element_count-1)/4+1);
      break;
      case QuartAdaptiveSortNoStk:
         adaptive_sort_buffered_nostack( elements, element_count, order_type_less()
                            , (element_count-1)/4+1);
      break;
      case InplaceStableSort:
         boost::movelib::inplace_stable_sort(elements, elements+element_count, order_type_less());
      break;
      case StdLkSqrtHAdpSort:
         std_like_adaptive_stable_sort_buffered( elements, element_count, order_type_less()
                                              , boost::movelib::detail_adaptive::ceil_sqrt_multiple(element_count)/2+1);
      break;
      case StdLkSqrtAdpSort:
         std_like_adaptive_stable_sort_buffered( elements, element_count, order_type_less()
                                               , boost::movelib::detail_adaptive::ceil_sqrt_multiple(element_count));
      break;
      case StdLkSqrt2AdpSort:
         std_like_adaptive_stable_sort_buffered( elements, element_count, order_type_less()
                                               , 2*boost::movelib::detail_adaptive::ceil_sqrt_multiple(element_count));
      break;
      case StdLkQuartAdpSort:
         std_like_adaptive_stable_sort_buffered( elements, element_count, order_type_less()
                                               , (element_count-1)/4+1);
      break;
      case SlowStableSort:
         boost::movelib::detail_adaptive::slow_stable_sort(elements, elements+element_count, order_type_less());
      break;
      case HeapSort:
         boost::movelib::heap_sort(elements, elements+element_count, order_type_less());
         boost::movelib::heap_sort((order_move_type*)0, (order_move_type*)0, order_type_less());

      break;
   }
}

//Restores the input and runs one algorithm on it, so that the timing loop can
//repeat the measurement
template<class T>
struct sort_runner
{
   sort_runner(std::size_t n, std::size_t a)
      : element_count(n), alg(a)
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
   {  run_sort_algo(elements, element_count, alg);  }

   std::size_t element_count;
   std::size_t alg;

   boost::ulong_long_type num_compare;
   boost::ulong_long_type num_copy;
   boost::ulong_long_type num_elements;
};

template<class Vector>
bool measure_algo( Vector &elements, const Vector &original
                 , std::size_t element_count, std::size_t alg, nanosecond_type &prev_clock)
{
   typedef typename Vector::value_type T;
   std::printf("%s ", AlgoNames[alg]);

   sort_runner<T> runner(element_count, alg);
   bench_util::bench_result const r = bench_util::measure_best_of(elements, original, runner);

   if(runner.num_elements == element_count){
      std::printf(" Tmp Ok ");
   } else{
      std::printf(" Tmp KO ");
   }

   //The counters are read before the order check, which compares elements too
   std::printf( "Cmp:%7.03f Cpy:%8.03f "
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
   return is_order_type_ordered(elements.data(), element_count, alg != HeapSort && alg != PdQsort && alg != StdSort);
}

template<class T>
bool measure_all(std::size_t L, std::size_t NK)
{
   boost::container::vector<T> original_elements, elements;
   generate_elements(original_elements, L, NK);
   std::printf("\n - - N: %u, NK: %u - -\n", (unsigned)L, (unsigned)NK);

   nanosecond_type prev_clock = 0;
   nanosecond_type back_clock;
   bool res = true;
   res = res && measure_algo(elements, original_elements, L,MergeSort, prev_clock);
   back_clock = prev_clock;
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L,StdStableSort, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L,PdQsort, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L,StdSort, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L,HeapSort, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L,QuartAdaptiveSort, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L,QuartAdaptiveSortNoStk, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L, StdLkQuartAdpSort, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L,Sqrt2AdaptiveSort, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L,Sqrt2AdaptiveSortNoStk, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L, StdLkSqrt2AdpSort, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L,SqrtAdaptiveSort, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L,SqrtAdaptiveSortNoStk, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L, StdLkSqrtAdpSort, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L,SqrtHAdaptiveSort, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L,SqrtHAdaptiveSortNoStk, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L, StdLkSqrtHAdpSort, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L,AdaptiveSort, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L,AdaptiveSortNoStk, prev_clock);
   //
   prev_clock = back_clock;
   res = res && measure_algo(elements, original_elements, L,InplaceStableSort, prev_clock);
   //
   //prev_clock = back_clock;
   //res = res && measure_algo(elements, original_elements, L,SlowStableSort, prev_clock);

   if(!res)
      std::abort();
   return res;
}

//Undef it to run the long test
#ifndef LONG_BENCH
#define BENCH_SORT_SHORT
#endif
#define BENCH_SORT_UNIQUE_VALUES

int main()
{
   #ifndef BENCH_SORT_UNIQUE_VALUES
   measure_all<order_perf_type>(101,1);
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

   //
   #ifdef NDEBUG
   #ifndef BENCH_SORT_UNIQUE_VALUES
   measure_all<order_perf_type>(100001,511);
   measure_all<order_perf_type>(100001,2047);
   measure_all<order_perf_type>(100001,8191);
   measure_all<order_perf_type>(100001,32767);
   #endif
   measure_all<order_perf_type>(100001,0);

   //
   #ifndef BENCH_SORT_SHORT
   #ifndef BENCH_SORT_UNIQUE_VALUES
   measure_all<order_perf_type>(1000001, 8192);
   measure_all<order_perf_type>(1000001, 32768);
   measure_all<order_perf_type>(1000001, 131072);
   measure_all<order_perf_type>(1000001, 524288);
   #endif
   measure_all<order_perf_type>(1000001,0);

   #ifndef BENCH_SORT_UNIQUE_VALUES
   measure_all<order_perf_type>(10000001, 65536);
   measure_all<order_perf_type>(10000001, 262144);
   measure_all<order_perf_type>(10000001, 1048576);
   measure_all<order_perf_type>(10000001, 4194304);
   #endif
   measure_all<order_perf_type>(10000001,0);
   #endif   //#ifndef BENCH_SORT_SHORT
   #endif   //NDEBUG

   //measure_all<order_perf_type>(100000001,0);

   return 0;
}
