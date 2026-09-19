//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2015-2026.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////

//Utilities shared by bench_sort and bench_merge to make the measurements
//repeatable: barriers that keep the optimizer out of the timed region, a
//best-of-N timing loop.

#ifndef BOOST_MOVE_TEST_BENCH_UTIL_HPP
#define BOOST_MOVE_TEST_BENCH_UTIL_HPP

#include <boost/config.hpp>
#include <boost/move/detail/nsec_clock.hpp>
#include <cstddef>
#include <cstdio>
#include <algorithm>

#if defined(BOOST_MSVC)
#include <intrin.h>
#endif

namespace bench_util {

using ::boost::move_detail::cpu_timer;
using ::boost::move_detail::nanosecond_type;

///////////////////////////////////////////////////////////////////////////////
//
//  Optimization barriers
//
//  "escape" makes the memory that a pointer refers to reachable by code the
//  compiler can not see, so the work that fills it can not be removed nor
//  delayed. "clobber" makes all pending writes to memory visible. Together
//  they keep the compiler from moving work out of the timed region.
//
///////////////////////////////////////////////////////////////////////////////

#if defined(BOOST_GCC) || defined(BOOST_CLANG)

inline void escape(void *p)
{  __asm__ __volatile__("" : : "g"(p) : "memory");  }

inline void clobber()
{  __asm__ __volatile__("" : : : "memory");  }

#elif defined(BOOST_MSVC)

//Microsoft x64 has no inline assembly. An unoptimized function is never
//inlined, so the argument is really passed and the call is really made.
#pragma optimize("", off)
inline void escape(void *p)
{  (void)p;  }
#pragma optimize("", on)

inline void clobber()
{  _ReadWriteBarrier();  }

#else

//Portable fallback: the pointer is stored in a volatile object, so the
//compiler must assume that the memory it refers to is reachable from
//elsewhere.
void * volatile bench_escape_sink = 0;

inline void escape(void *p)
{  bench_escape_sink = p;  }

inline void clobber()
{  void *const p = bench_escape_sink; bench_escape_sink = p;  }

#endif

///////////////////////////////////////////////////////////////////////////////
//
//  Timing loop
//
///////////////////////////////////////////////////////////////////////////////

//A measurement is repeated until this much time is spent on it, so that short
//cases are repeated many times and long ones only once.
#ifndef BENCH_TIME_BUDGET_NS
#define BENCH_TIME_BUDGET_NS 200000000u    //200 ms
#endif

//Upper limit on the repetitions, whatever the budget says.
#ifndef BENCH_MAX_RUNS
#define BENCH_MAX_RUNS 63u
#endif

//The clock has a resolution near 100 ns, so a measurement shorter than this is
//timed in batches. See "measure_best_of".
#ifndef BENCH_MIN_TIMED_NS
#define BENCH_MIN_TIMED_NS 2000000u    //2 ms
#endif

struct bench_result
{
   //Noise can only add time, so the minimum is the most repeatable estimator of
   //the cost of the algorithm. The median tells how noisy the measurement was:
   //when it is far from the minimum, the machine was busy with something else.
   nanosecond_type best;
   nanosecond_type median;
   unsigned        runs;

   //Percentage the median is above the minimum
   double noise() const
   {  return best ? 100.0*double(median-best)/double(best) : 0.0;  }
};

//Copies "original" over "elements" the given number of times, which is what a
//batch of measurements has to do to give each run the same input
template<class Vector>
nanosecond_type time_restores(Vector &elements, const Vector &original, unsigned batch)
{
   cpu_timer timer;
   timer.resume();
   for(unsigned i = 0u; i != batch; ++i){
      elements = original;
      escape(elements.data());
   }
   timer.stop();
   clobber();
   return timer.elapsed().wall;
}

//Runs "runner" several times on a fresh copy of "original" and keeps the best
//time. "runner" must provide:
//   reset()               called before the first run
//   operator()(pointer)   runs the algorithm
//   record()              called after the first run, to keep the counters of a
//                         single run, because the following runs add to them
//
//A single run of a short case is too close to the resolution of the clock to
//mean anything, so short cases are timed in batches and the cost of restoring
//the input, measured apart, is subtracted.
template<class Vector, class Runner>
bench_result measure_best_of(Vector &elements, const Vector &original, Runner &runner)
{
   typedef typename Vector::value_type value_type;

   //A first run gives a rough estimate and leaves the counters the caller prints
   nanosecond_type estimate;
   {
      elements = original;
      value_type *const data = elements.data();
      runner.reset();
      escape(data);
      cpu_timer timer;
      timer.resume();
      runner(data);
      timer.stop();
      clobber();
      estimate = timer.elapsed().wall;
      runner.record();
   }

   unsigned batch = 1u;
   if(estimate < nanosecond_type(BENCH_MIN_TIMED_NS)){
      batch = unsigned(nanosecond_type(BENCH_MIN_TIMED_NS)/(estimate ? estimate : 1)) + 1u;
   }

   //Cost of the restores alone, so that it can be taken out of the batches
   nanosecond_type restore = 0;
   if(batch > 1u){
      restore = time_restores(elements, original, batch);
      for(unsigned i = 0u; i != 2u; ++i){
         nanosecond_type const t = time_restores(elements, original, batch);
         if(t < restore){
            restore = t;
         }
      }
   }

   nanosecond_type times[BENCH_MAX_RUNS];
   nanosecond_type total = 0;
   unsigned runs = 0u;

   do{
      cpu_timer timer;
      timer.resume();
      for(unsigned i = 0u; i != batch; ++i){
         elements = original;
         value_type *const data = elements.data();
         escape(data);
         runner(data);
         clobber();
      }
      timer.stop();
      nanosecond_type t = timer.elapsed().wall;
      total = nanosecond_type(total + t);
      t = t > restore ? nanosecond_type(t - restore) : nanosecond_type(0);
      times[runs++] = nanosecond_type(t/batch);
   }while(runs < BENCH_MAX_RUNS && total < nanosecond_type(BENCH_TIME_BUDGET_NS));

   std::sort(times, times + runs);
   bench_result res;
   res.best   = times[0];
   res.median = times[runs/2u];
   res.runs   = runs;
   return res;
}

//Prints a duration with the unit that keeps it readable
inline void print_time(nanosecond_type ns)
{
   double time = double(ns);
   const char *units = "ns";
   if(time >= 1000000000.0){
      time /= 1000000000.0;
      units = " s";
   }
   else if(time >= 1000000.0){
      time /= 1000000.0;
      units = "ms";
   }
   else if(time >= 1000.0){
      time /= 1000.0;
      units = "us";
   }
   std::printf("%6.02f%s", time, units);
}

}  //namespace bench_util {

#endif   //BOOST_MOVE_TEST_BENCH_UTIL_HPP
