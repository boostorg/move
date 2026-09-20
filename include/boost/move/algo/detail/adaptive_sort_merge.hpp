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
//
// Stable sorting that works in O(N*log(N)) worst time
// and uses O(1) extra memory
//
//////////////////////////////////////////////////////////////////////////////
//
// The main idea of the adaptive_sort algorithm was developed by Andrey Astrelin
// and explained in the article from the russian collaborative blog
// Habrahabr (http://habrahabr.ru/post/205290/). The algorithm is based on
// ideas from B-C. Huang and M. A. Langston explained in their article
// "Fast Stable Merging and Sorting in Constant Extra Space (1989-1992)"
// (http://comjnl.oxfordjournals.org/content/35/6/643.full.pdf).
//
// This implementation by Ion Gaztanaga uses previous ideas with additional changes:
// 
// - Use of GCD-based rotation.
// - Non power of two buffer-sizes.
// - Tries to find sqrt(len)*2 unique keys, so that the merge sort
//   phase can form up to sqrt(len)*4 segments if enough keys are found.
// - The merge-sort phase can take advantage of external memory to
//   save some additional combination steps.
// - Combination phase: Blocks are selection sorted and merged in parallel.
// - The combination phase is performed alternating merge to left and merge
//   to right phases minimizing swaps due to internal buffer repositioning.
// - When merging blocks special optimizations are made to avoid moving some
//   elements twice.
//
// The adaptive_merge algorithm was developed by Ion Gaztanaga reusing some parts
// from the sorting algorithm and implementing an additional block merge algorithm
// without moving elements to left or right.
//////////////////////////////////////////////////////////////////////////////
#ifndef BOOST_MOVE_ADAPTIVE_SORT_MERGE_HPP
#define BOOST_MOVE_ADAPTIVE_SORT_MERGE_HPP

#include <boost/move/detail/config_begin.hpp>

#include <boost/move/detail/reverse_iterator.hpp>
#include <boost/move/algo/move.hpp>
#include <boost/move/algo/detail/merge.hpp>
#include <boost/move/adl_move_swap.hpp>
#include <boost/move/algo/detail/insertion_sort.hpp>
#include <boost/move/algo/detail/merge_sort.hpp>
#include <boost/move/algo/detail/heap_sort.hpp>
#include <boost/move/algo/detail/merge.hpp>
#include <boost/move/algo/detail/is_sorted.hpp>
#include <cassert>
#include <boost/cstdint.hpp>
#include <limits.h>

#if defined(BOOST_CLANG) || (defined(BOOST_GCC) && (BOOST_GCC >= 40600))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#endif

#ifndef BOOST_MOVE_ADAPTIVE_SORT_STATS_LEVEL
   #define BOOST_MOVE_ADAPTIVE_SORT_STATS_LEVEL 1
#endif

#ifdef BOOST_MOVE_ADAPTIVE_SORT_STATS
   #if BOOST_MOVE_ADAPTIVE_SORT_STATS_LEVEL == 2
      #define BOOST_MOVE_ADAPTIVE_SORT_PRINT_L1(STR, L) \
         print_stats(STR, L)\
      //

      #define BOOST_MOVE_ADAPTIVE_SORT_PRINT_L2(STR, L) \
         print_stats(STR, L)\
      //
   #else
      #define BOOST_MOVE_ADAPTIVE_SORT_PRINT_L1(STR, L) \
         print_stats(STR, L)\
      //

      #define BOOST_MOVE_ADAPTIVE_SORT_PRINT_L2(STR, L)
   #endif
#else
   #define BOOST_MOVE_ADAPTIVE_SORT_PRINT_L1(STR, L)
   #define BOOST_MOVE_ADAPTIVE_SORT_PRINT_L2(STR, L)
#endif

#ifdef BOOST_MOVE_ADAPTIVE_SORT_INVARIANTS
   #define BOOST_MOVE_ADAPTIVE_SORT_INVARIANT  assert
#else
   #define BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(L)
#endif

#if defined(BOOST_MOVE_ADAPTIVE_SORT_INVARIANTS)
#include <boost/move/algo/detail/is_sorted.hpp>
#endif

namespace boost {
namespace movelib {
namespace detail_adaptive {

//Default number of stack bytes that adaptive_sort and adaptive_merge may use
//as an internal buffer when the caller supplies a smaller one (or none).
static const std::size_t AdaptiveDefaultStackBytes = 1024u*sizeof(void*)/8u;

//Combination step can tag with integers instead of the collected keys
//making key comparison trivial. Tags are held in a array of unsigned char
//so the number of tags is limited to CHAR_BIT
static const std::size_t AdaptiveLocalKeyCount = std::size_t(1) << CHAR_BIT;

//Number of T elements that fit in a buffer of StackBytes bytes, clamped so
//that the count is representable in SizeType: some iterators use a tiny size
//type (e.g. a signed char difference type) in which the count would overflow.
template<std::size_t StackBytes, class SizeType, class T>
inline SizeType stack_buffer_capacity()
{
   const std::size_t n_elem  = StackBytes/sizeof(T);
   const std::size_t max_val = std::size_t(SizeType(-1));
   return static_cast<SizeType>(n_elem < max_val ? n_elem : max_val);
}
BOOST_MOVE_STATIC_ASSERT((MergeSortInsertionSortThreshold&(MergeSortInsertionSortThreshold-1)) == 0);
#if defined BOOST_HAS_INTPTR_T
   typedef ::boost::uintptr_t uintptr_t;
#else
   typedef std::size_t uintptr_t;
#endif

template<class T>
const T &min_value(const T &a, const T &b)
{
   return a < b ? a : b;
}

template<class T>
const T &max_value(const T &a, const T &b)
{
   return a > b ? a : b;
}

template<class ForwardIt, class Pred, class V>
typename iter_size<ForwardIt>::type
   count_if_with(ForwardIt first, ForwardIt last, Pred pred, const V &v)
{
   typedef typename iter_size<ForwardIt>::type size_type;
   size_type count = 0;
   while(first != last) {
      count = size_type(count + static_cast<size_type>(0 != pred(*first, v)));
      ++first;
   }
   return count;
}


// Tells where the free elements a merge may rotate a hole through are:
//   - free_run_t: they are [d_first, first1), in front of range 1, which is what
//     the rotating hole needs.
//   - no_free_run_t: range 1 is a detached buffer and the output chases range 2,
//     so the element after the output is live and no hole can be opened.
// Only the caller knows which one applies, so it says so with these tags.
struct free_run_t{};
struct no_free_run_t{};

// True if both iterators denote the same position. Iterators of different types
// walk different storage, which can not overlap, so they never do. Comparing the
// iterators and not the addresses of their elements means the position one past
// the last one may be tested without dereferencing it.
template<class It1, class It2>
inline bool is_same_position(It1, It2)
{  return false;  }

template<class It>
inline bool is_same_position(It l, It r)
{  return l == r;  }

template<class RandIt, class Compare>
RandIt skip_until_merge
   ( RandIt first1, RandIt const last1
   , const typename iterator_traits<RandIt>::value_type &next_key, Compare comp)
{
   while(first1 != last1 && !comp(next_key, *first1)){
      ++first1;
   }
   return first1;
}


template<class RandItKeys>
void update_key
(RandItKeys const key_next
   , RandItKeys const key_range2
   , RandItKeys &key_mid)
{
   if (key_next != key_range2) {
      ::boost::adl_move_swap(*key_next, *key_range2);
      if (key_next == key_mid) {
         key_mid = key_range2;
      }
      else if (key_mid == key_range2) {
         key_mid = key_next;
      }
   }
}

template<class RandItKeys, class RandIt>
void swap_and_update_key
   ( RandItKeys const key_next
   , RandItKeys const key_range2
   , RandItKeys &key_mid
   , RandIt const begin
   , RandIt const end
   , RandIt const with)
{
   if(begin != with){
      ::boost::adl_move_swap_ranges(begin, end, with);
      update_key(key_next, key_range2, key_mid);
   }
}

template<class RandItKeys, class RandIt, class RandIt2, class Op>
RandIt2 buffer_and_update_key
(RandItKeys const key_next
   , RandItKeys const key_range2
   , RandItKeys &key_mid
   , RandIt begin
   , RandIt end
   , RandIt with
   , RandIt2 buffer
   , Op op)
{
   if (begin != with) {
      while(begin != end) {
         op(three_way_t(), begin++, with++, buffer++);
      }
      if (key_next != key_range2)   //Avoid potential self-swapping
         ::boost::adl_move_swap(*key_next, *key_range2);
      if (key_next == key_mid) {
         key_mid = key_range2;
      }
      else if (key_mid == key_range2) {
         key_mid = key_next;
      }
   }
   return buffer;
}

///////////////////////////////////////////////////////////////////////////////
//
//                         MERGE BUFFERLESS
//
///////////////////////////////////////////////////////////////////////////////

// [first1, last1) merge [last1,last2) -> [first1,last2)
//
// The second range is never empty: the only caller (merge_blocks_bufferless)
// always passes a whole block, whose length is one or more elements.
template<class RandIt, class Compare>
RandIt partial_merge_bufferless_impl
   (RandIt first1, RandIt last1, RandIt const last2, bool *const pis_range1_A, Compare comp)
{
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(last1 != last2);
   bool const is_range1_A = *pis_range1_A;
   if(first1 != last1 && comp(*last1, last1[-1])){
      do{
         RandIt const old_last1 = last1;
         last1  = boost::movelib::lower_bound(last1, last2, *first1, comp);
         first1 = rotate_gcd(first1, old_last1, last1);//old_last1 == last1 supported
         if(last1 == last2){
            return first1;
         }
         do{
            ++first1;
         } while(last1 != first1 && !comp(*last1, *first1) );
      } while(first1 != last1);
   }
   *pis_range1_A = !is_range1_A;
   return last1;
}

// [first1, last1) merge [last1,last2) -> [first1,last2)
template<class RandIt, class Compare>
RandIt partial_merge_bufferless
   (RandIt first1, RandIt last1, RandIt const last2, bool *const pis_range1_A, Compare comp)
{
   return *pis_range1_A ? partial_merge_bufferless_impl(first1, last1, last2, pis_range1_A, comp)
                        : partial_merge_bufferless_impl(first1, last1, last2, pis_range1_A, antistable<Compare>(comp));
}

template<class SizeType>
static SizeType needed_keys_count(SizeType n_block_a, SizeType n_block_b)
{
   return SizeType(n_block_a + n_block_b);
}

template<class RandItKeys, class KeyCompare, class RandIt, class Compare>
typename iter_size<RandIt>::type
   find_next_block
      ( RandItKeys const key_first
      , KeyCompare key_comp
      , RandIt const first
      , typename iter_size<RandIt>::type const l_block
      , typename iter_size<RandIt>::type const ix_first_block
      , typename iter_size<RandIt>::type const ix_last_block
      , Compare comp)
{
   typedef typename iter_size<RandIt>::type      size_type;
   typedef typename iterator_traits<RandIt>::value_type     value_type;
   typedef typename iterator_traits<RandItKeys>::value_type key_type;
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(ix_first_block <= ix_last_block);
   size_type ix_min_block = 0u;
   for (size_type szt_i = ix_first_block; szt_i < ix_last_block; ++szt_i) {
      const value_type &min_val = first[size_type(ix_min_block*l_block)];
      const value_type &cur_val = first[size_type(szt_i*l_block)];
      const key_type   &min_key = key_first[ix_min_block];
      const key_type   &cur_key = key_first[szt_i];

      bool const less_than_minimum = comp(cur_val, min_val) ||
         (!comp(min_val, cur_val) && key_comp(cur_key, min_key));

      if (less_than_minimum) {
         ix_min_block = szt_i;
      }
   }
   return ix_min_block;
}

template<class RandItKeys, class KeyCompare, class RandIt, class Compare>
void merge_blocks_bufferless
   ( RandItKeys const key_first
   , KeyCompare key_comp
   , RandIt const first
   , typename iter_size<RandIt>::type const l_block
   , typename iter_size<RandIt>::type const l_irreg1
   , typename iter_size<RandIt>::type const n_block_a
   , typename iter_size<RandIt>::type const n_block_b
   , typename iter_size<RandIt>::type const l_irreg2
   , Compare comp)
{
   typedef typename iter_size<RandIt>::type size_type;
   size_type const key_count = needed_keys_count(n_block_a, n_block_b);
   ::boost::movelib::ignore(key_count);
   //assert(n_block_a || n_block_b);
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(boost::movelib::is_sorted_and_unique(key_first, key_first + key_count, key_comp));
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(!n_block_b || n_block_a == count_if_with(key_first, key_first + key_count, key_comp, key_first[n_block_a]));

   size_type n_bef_irreg2 = 0;
   bool l_irreg_pos_count = true;
   RandItKeys key_mid(key_first + n_block_a);
   RandIt const first_irr2 = first + size_type(l_irreg1 + (n_block_a+n_block_b)*l_block);
   RandIt const last_irr2  = first_irr2 + l_irreg2;

   {  //Selection sort blocks
      size_type n_block_left = size_type(n_block_b + n_block_a);
      RandItKeys key_range2(key_first);

      size_type min_check = n_block_a == n_block_left ? 0u : n_block_a;
      size_type max_check = min_value<size_type>(size_type(min_check+1), n_block_left);
      for ( RandIt f = first+l_irreg1; n_block_left; --n_block_left) {
         size_type const next_key_idx = find_next_block(key_range2, key_comp, f, l_block, min_check, max_check, comp);
         RandItKeys const key_next(key_range2 + next_key_idx);
         max_check = min_value<size_type>(max_value<size_type>(max_check, size_type(next_key_idx+2)), n_block_left);

         RandIt const first_min = f + size_type(next_key_idx*l_block);

         //Check if irregular b block should go here.
         //If so, break to the special code handling the irregular block
         if (l_irreg_pos_count && l_irreg2 && comp(*first_irr2, *first_min)){
            l_irreg_pos_count = false;
         }
         n_bef_irreg2 = size_type(n_bef_irreg2+l_irreg_pos_count);

         swap_and_update_key(key_next, key_range2, key_mid, f, f + l_block, first_min);
         BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(boost::movelib::is_sorted(f, f+l_block, comp));
         BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(boost::movelib::is_sorted(first_min, first_min + l_block, comp));
         BOOST_MOVE_ADAPTIVE_SORT_INVARIANT((f == (first+l_irreg1)) || !comp(*f, *(f-l_block)));
         //Update context
         ++key_range2;
         f += l_block;
         min_check = size_type(min_check - (min_check != 0));
         max_check = size_type(max_check - (max_check != 0));
      }
   }
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(boost::movelib::is_sorted(first+l_irreg1+n_bef_irreg2*l_block, first_irr2, comp));

   RandIt first1 = first;
   RandIt last1  = first+l_irreg1;
   RandItKeys const key_end (key_first+n_bef_irreg2);
   bool is_range1_A = true;

   for(RandItKeys key_next = key_first; key_next != key_end; ++key_next){
      bool is_range2_A = key_mid == (key_first+key_count) || key_comp(*key_next, *key_mid);
      first1 = is_range1_A == is_range2_A
         ? last1 : partial_merge_bufferless(first1, last1, last1 + l_block, &is_range1_A, comp);
      last1 += l_block;
      BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(boost::movelib::is_sorted(first, first1, comp));
   }

   merge_bufferless_ONlogN(is_range1_A ? first1 : last1, first_irr2, last_irr2, comp);
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(boost::movelib::is_sorted(first, last_irr2, comp));
}

//Tags telling collect_unique whether the input range is already ordered
struct collect_unsorted_t {};
struct collect_sorted_t   {};

//Position where "*u" would be inserted in the sorted key range [kfirst, klast).
//"*u" is a new distinct value if the result is klast or compares greater.
template<class RandItKeys, class RandIt, class Compare>
BOOST_MOVE_FORCEINLINE RandItKeys collect_unique_find
   (RandItKeys kfirst, RandItKeys klast, RandIt u, Compare comp, collect_unsorted_t)
{
   return boost::movelib::lower_bound(kfirst, klast, *u, comp);
}

//For an ordered range a new value can only be greater than the last one
template<class RandItKeys, class RandIt, class Compare>
BOOST_MOVE_FORCEINLINE RandItKeys collect_unique_find
   (RandItKeys, RandItKeys klast, RandIt u, Compare comp, collect_sorted_t)
{
   RandItKeys klast_bef = klast;
   --klast_bef;
   return comp(*klast_bef, *u) ? klast : klast_bef;
}

//Advance to the next candidate. Without order there is nothing to skip
template<class RandIt, class RandItKeys, class Compare>
BOOST_MOVE_FORCEINLINE void collect_unique_next
   (RandIt &u, RandIt, RandItKeys, Compare, collect_unsorted_t)
{
   ++u;
}

//In an ordered range every element up to the first one greater than the biggest
//collected key repeats an already collected value, so the whole run is skipped at
//once. That visits one element per distinct value instead of the whole range
template<class RandIt, class RandItKeys, class Compare>
BOOST_MOVE_FORCEINLINE void collect_unique_next
   (RandIt &u, RandIt last, RandItKeys klast, Compare comp, collect_sorted_t)
{
   //The element at "u" was already processed so start after it.
   ++u;
   u = boost::movelib::gallop_upper_bound(u, last, *(klast-1), comp);
}

// Complexity: 2*distance(first, last)+max_collected^2/2, or
//             max_collected*log(distance(first, last)) if the input is ordered
//
// Tries to collect at most n_keys unique elements from [first, last),
// in the begining of the range, and ordered according to comp
// 
// Returns the number of collected keys
//
// While the range is scanned it is seen as four consecutive pieces:
//
//   [first, h0)        non-key elements already scanned, in their original order
//   [h0, search_end)   the "h" keys collected so far, ordered according to comp
//   [search_end, u)    non-key elements scanned after the last collected key
//   [u, last)          not scanned yet
//
// so h0+h == search_end holds all the time. Collecting a key moves the key block
// past the non-key elements found since the previous one, which keeps those in
// their original relative order, and a final step brings the key block to the
// front of the range.
template<class RandIt, class Compare, class XBuf, class CollectTag>
typename iter_size<RandIt>::type
   collect_unique
      ( RandIt const first, RandIt const last
      , typename iter_size<RandIt>::type const max_collected, Compare comp
      , XBuf & xbuf, CollectTag tag)
{
   typedef typename iter_size<RandIt>::type       size_type;
   size_type h = 0;

   if(max_collected){
      ++h;                       //*first is always a key, no comparison is needed
      RandIt h0 = first;         //beginning of the key block
      RandIt u = first; ++u;     //candidate
      RandIt search_end = u;     //end of the key block

      //If the additional memory can hold every key, the key block is kept there.
      //The keys are then contiguous and adding one leaves a hole in the range
      if(xbuf.capacity() >= max_collected){
         typename XBuf::iterator const ph0 = xbuf.add(first);
         while(u != last && h < max_collected){
            //*u belongs in the key block. It is a new value if that position
            //is the end of the block or the key is greater
            typename XBuf::iterator const r = collect_unique_find(ph0, xbuf.end(), u, comp, tag);
            if(r == xbuf.end() || comp(*u, *r) ){
               //Slide the non-keys found since the previous key
               RandIt const new_h0 = boost::move(search_end, u, h0);
               search_end = u;
               ++search_end;
               ++h;
               xbuf.insert(r, u);   //Move *u to the buffer and leave a hole
               h0 = new_h0;
            }
            //Next candidate, skipping what can not be a new value
            collect_unique_next(u, last, xbuf.end(), comp, tag);
         }
         //Move front data to make room for keys
         boost::move_backward(first, h0, h0+h);
         boost::move(xbuf.data(), xbuf.end(), first);
      }
      else{
         //Not enough memory, put key block inside the range
         while(u != last && h < max_collected){
            //*u belongs in the key block. It is a new value if that position
            //is the end of the block or the key that sits there is greater
            RandIt const r = collect_unique_find(h0, search_end, u, comp, tag);
            if(r == search_end || comp(*u, *r) ){
               //Move the key block after the non-keys found since the previous key
               RandIt const new_h0 = rotate_gcd(h0, search_end, u);
               search_end = u;
               ++search_end;
               ++h;
               //*u is now the last element of the key block, rotate it to the
               //correct position. A no-op for an ordered input
               rotate_gcd(r+(new_h0-h0), u, search_end);
               h0 = new_h0;
            }
            //Next candidate, skipping what can not be a new value
            collect_unique_next(u, last, search_end, comp, tag);
         }
         //Move the key block to the front of the range
         rotate_gcd(first, h0, h0+h);
      }
   }
   return h;
}

template<class Unsigned>
Unsigned floor_sqrt(Unsigned n)
{
   Unsigned rem = 0, root = 0;
   const unsigned bits = sizeof(Unsigned)*CHAR_BIT;

   for (unsigned i = bits / 2; i > 0; i--) {
      root = Unsigned(root << 1u);
      rem = Unsigned(Unsigned(rem << 2u) | Unsigned(n >> (bits - 2u)));
      n = Unsigned(n << 2u);
      if (root < rem) {
         rem  = Unsigned(rem - Unsigned(root | 1u));
         root = Unsigned(root + 2u);
      }
   }
   return Unsigned(root >> 1u);
}

template<class Unsigned>
Unsigned ceil_sqrt(Unsigned const n)
{
   Unsigned r = floor_sqrt(n);
   return Unsigned(r + Unsigned((n%r) != 0));
}

template<class Unsigned>
Unsigned floor_merge_multiple(Unsigned const n, Unsigned &base, Unsigned &pow)
{
   Unsigned s = n;
   Unsigned p = 0;
   while(s > MergeSortInsertionSortThreshold){
      s /= 2;
      ++p;
   }
   base = s;
   pow = p;
   return Unsigned(s << p);
}

template<class Unsigned>
Unsigned ceil_merge_multiple(Unsigned const n, Unsigned &base, Unsigned &pow)
{
   Unsigned fm = floor_merge_multiple(n, base, pow);

   if(fm != n){
      if(base < MergeSortInsertionSortThreshold){
         ++base;
      }
      else{
         base = MergeSortInsertionSortThreshold/2 + 1;
         ++pow;
      }
   }
   return Unsigned(base << pow);
}

template<class Unsigned>
Unsigned ceil_sqrt_multiple(Unsigned const n, Unsigned *pbase = 0)
{
   Unsigned const r = ceil_sqrt(n);
   Unsigned pow = 0;
   Unsigned base = 0;
   Unsigned const res = ceil_merge_multiple(r, base, pow);
   if(pbase) *pbase = base;
   return res;
}

struct less
{
   template<class T>
   bool operator()(const T &l, const T &r)
   {  return l < r;  }
};

///////////////////////////////////////////////////////////////////////////////
//
//                            MERGE BLOCKS
//
///////////////////////////////////////////////////////////////////////////////

//Define it to fall back to the top-down form instead of the bottom-up one.
//Both are stable, need no additional memory and are O(N log^2 N), so this only
//chooses which of the two the adaptive algorithms use.
//#define ADAPTIVE_SORT_MERGE_SLOW_STABLE_SORT_IS_NLOGN

// The stable sort the adaptive algorithms fall back to when they could not get
// a buffer. It is only a name for one of the two bufferless stable sorts.
template<class RandIt, class Compare>
void slow_stable_sort
   ( RandIt const first, RandIt const last, Compare comp)
{
   #if defined ADAPTIVE_SORT_MERGE_SLOW_STABLE_SORT_IS_NLOGN
   boost::movelib::stable_sort_bufferless_ONlogN2_recursive(first, last, comp);
   #else
   stable_sort_bufferless_ONlogN2(first, last, comp);
   #endif
}

//Returns new l_block and updates use_buf
template<class Unsigned>
Unsigned lblock_for_combine
   (Unsigned const l_block, Unsigned const n_keys, Unsigned const l_data, bool &use_buf)
{
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(l_data > 1);

   //We need to guarantee lblock >= l_merged/(n_keys/2) keys for the combination.
   //We have at least 4 keys guaranteed (which are the minimum to merge 2 ranges)
   //If l_block != 0, then n_keys is already enough to merge all blocks in all
   //phases as we've found all needed keys for that buffer and length before.
   //If l_block == 0 then see if half keys can be used as buffer and the rest
   //as keys guaranteeing that n_keys >= (2*l_merged)/lblock = 
   if(!l_block){
      //If l_block == 0 then n_keys is power of two
      //(guaranteed by build_params(...))
      BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(n_keys >= 4);
      //assert(0 == (n_keys &(n_keys-1)));

      //See if half keys are at least 4 and if half keys fulfill
      Unsigned const new_buf  = n_keys/2;
      Unsigned const new_keys = Unsigned(n_keys-new_buf);
      use_buf = new_keys >= 4 && new_keys >= l_data/new_buf;
      if(use_buf){
         return new_buf;
      }
      else{
         return l_data/n_keys;
      }
   }
   else{
      use_buf = true;
      return l_block;
   }
}

// Stable sort tries to take advantages of any uninitialized memory in "xbuf"
// using different algorithms depending on the provided buffer size.
template<class RandIt, class Compare, class XBuf>
void stable_sort( RandIt first, RandIt last, Compare comp, XBuf & xbuf)
{
   typedef typename iter_size<RandIt>::type size_type;
   size_type const len = size_type(last - first);
   size_type const half_len = size_type(len/2u + (len&1u));
   size_type const cap = size_type(xbuf.capacity() - xbuf.size());
   if(cap >= half_len) {
      merge_sort(first, last, comp, xbuf.data()+xbuf.size());
   }
   else if(cap){
      //If merge sort is not possible use existing capacity for internal merges
      stable_sort_adaptive_ONlogN2(first, last, comp, xbuf.data()+xbuf.size(), cap);
   }
   else{
      slow_stable_sort(first, last, comp);
   }
}

template<class RandIt, class Comp, class XBuf>
void unstable_sort( RandIt first, RandIt last
                    , Comp comp
                    , XBuf & xbuf)
{
   heap_sort(first, last, comp);
   ::boost::movelib::ignore(xbuf);
}

///////////////////////////////////////////////////////////////////////////////
//
//                    MERGE SMALL RUN INTO LARGE RUN (GROUP ROTATIONS)
//
///////////////////////////////////////////////////////////////////////////////

// Stable in-place merge of a small sorted run [first, middle) (m elements)
// into a large sorted run [middle, last) (n elements) using O(1) extra memory.
//
// Keys are processed in groups of g ~ sqrt(m) elements, from the smallest to
// the largest. For each group:
//   - the data elements smaller than the largest key of the group are located
//     with a binary search (they must be interleaved with this group),
//   - the remaining keys are rotated past those data elements (one move per
//     element, so each data element is moved once here),
//   - the group is merged into that data segment with single-key rotations
//     (each data element of the segment is moved once more).
//
// Cost: ~2*n + m*sqrt(m) moves and O(m*log(n)) comparisons, instead of the
// ~n*log(m)/2 moves of the recursive rotation merge (merge_bufferless_ONlogN).
// Use only when m*sqrt(m) is small compared to n (see use_small_run_merge).
//
// If a (too small for a buffered merge) external buffer of constructed elements
// [buffer, buffer + buffer_size) is available, it is used to speed up the
// rotations and the group merges once the remaining keys fit in it.
template<class RandIt, class Compare, class RandItBuf>
void merge_small_run_rotations
   ( RandIt const first, RandIt const middle, RandIt const last, Compare comp
   , RandItBuf const buffer, typename iter_size<RandIt>::type const buffer_size)
{
   typedef typename iter_size<RandIt>::type size_type;

   //Both halves are never empty: the only caller (stable_merge) returns before
   //if one of them is empty and the searches that trim the range leave at least
   //one element in each half.
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(first != middle && middle != last);

   size_type n_keys_left = size_type(middle - first);
   size_type const l_group = ceil_sqrt(n_keys_left);

   RandIt keys = first;    //remaining keys: [keys, keys + n_keys_left)
   while(n_keys_left){
      size_type const l_cur = min_value<size_type>(l_group, n_keys_left);
      size_type const l_rest = size_type(n_keys_left - l_cur);
      RandIt const group_end = keys + l_cur;
      RandIt const keys_end  = keys + n_keys_left;
      RandIt group_last = group_end;
      --group_last;
      //Data elements that must be placed before the largest key of the group
      RandIt const data_end  = boost::movelib::lower_bound(keys_end, last, *group_last, comp);
      //Move the remaining keys after those data elements: [group][data][rest keys]
      RandIt const rest_keys = rotate_adaptive
         (group_end, keys_end, data_end, l_rest, size_type(data_end - keys_end), buffer, buffer_size);
      //Merge the group with its data segment
      if(l_cur <= buffer_size){
         range_xbuf<RandItBuf, size_type, move_op> rxbuf(buffer, buffer + buffer_size);
         buffered_merge(keys, group_end, rest_keys, comp, rxbuf);
      }
      else{
         //The group does not fit in the buffer. The input is unbalanced by
         //construction, about sqrt(n_keys) vs whole data segment, so
         //merge_bufferless_ON2 pays the squared term on the small group
         //A recursive rotation merge would move more elements
         merge_bufferless_ON2(keys, group_end, rest_keys, comp);
      }
      keys = rest_keys;
      n_keys_left = l_rest;
   }
}

// True if merging two consecutive runs of "len1" and "len2" elements with
// merge_small_run_rotations is cheaper than the recursive rotation merge.
//
// merge_small_run_rotations moves ~2*l_large + 2*l_small^1.5 elements, while
// the recursive rotation merge moves ~l_large*log2(l_small)/2. The group
// rotations win while l_small^1.5 stays small compared to l_large, that is
//
//    l_small^3 <= l_large^2 / 4
//
// Note that the exponents matter: comparing l_small^2 with l_large would place
// the limit at sqrt(l_large), which does not follow the crossover. The
// crossover was measured (MSVC and GCC, l_large from 1e5 to 1e7) at
// l_small ~ 1.0 to 3.9 * l_large^(2/3), always above the limit above, which
// keeps a safety factor of 1.5 for the smallest range and more for the rest.
//
// Dividing both sides by l_small^2 and writing "ratio" for l_large/l_small,
// the condition becomes l_small/ratio <= ratio/4, which needs divisions only:
// no intermediate value can overflow (both lengths may be close to the maximum
// of SizeType) and no cube or square has to be computed.
//
// Note that l_small/l_large may not be used as the left hand side, even though
// the real-arithmetic condition is also l_small/l_large <= l_large/l_small^2/4:
// l_small is the smaller length, so that quotient always truncates to zero and
// the test would degenerate into "always true".
template<class SizeType>
inline bool use_small_run_merge(SizeType const len1, SizeType const len2)
{
   //Neither length is zero: the only caller (stable_merge) returns before if one
   //of the halves is empty, so the divisions below are always safe.
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(len1 && len2);
   SizeType const l_small = min_value<SizeType>(len1, len2);
   SizeType const l_large = max_value<SizeType>(len1, len2);
   SizeType const ratio = SizeType(l_large/l_small);   //Always >= 1
   return SizeType(l_small/ratio) <= SizeType(ratio/4u);
}

template<class RandIt, class Compare, class XBuf>
void stable_merge
      ( RandIt first, RandIt const middle, RandIt last
      , Compare comp
      , XBuf &xbuf)
{
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(xbuf.empty());
   typedef typename iter_size<RandIt>::type   size_type;

   //Both ranges sorted. Skip the elements that are already in their final position.
   //
   //This speeds up merge when the ranges barely interleave.
   if(BOOST_UNLIKELY(first == middle || middle == last)){
      return;
   }
   {
      RandIt first_high(middle);
      --first_high;
      if(BOOST_UNLIKELY(!comp(*middle, *first_high))){   //Already sorted
         return;
      }
      //Both searches leave a non-empty range, as *middle < *first_high
      first = boost::movelib::upper_bound(first, middle, *middle, comp);
      last  = boost::movelib::lower_bound(middle, last, *first_high, comp);
   }


   size_type const len1  = size_type(middle-first);
   size_type const len2  = size_type(last-middle);
   size_type const l_min = min_value<size_type>(len1, len2);
   if(xbuf.capacity() >= l_min){
      buffered_merge(first, middle, last, comp, xbuf);
      xbuf.clear();
   }
   else if(use_small_run_merge(len1, len2)){
      //The external buffer (if any) is too small for a buffered merge, but it
      //can speed up rotations. Construct its elements so that they can be assigned.
      size_type const buffer_size = size_type(xbuf.capacity());
      if(buffer_size){
         xbuf.initialize_until(buffer_size, *first);
      }
      if(len1 <= len2){
         merge_small_run_rotations(first, middle, last, comp, xbuf.begin(), buffer_size);
      }
      else{
         //Mirror the problem: the small run is at the end. Merging the reversed
         //sequences with the inverse comparison yields the reversed stable merge.
         merge_small_run_rotations
            ( (make_reverse_iterator)(last), (make_reverse_iterator)(middle)
            , (make_reverse_iterator)(first), inverse<Compare>(comp), xbuf.begin(), buffer_size);
      }
      xbuf.clear();
   }
   else{
      //merge_bufferless_ONlogN(first, middle, last, comp);
      merge_adaptive_ONlogN(first, middle, last, comp, xbuf.begin(), xbuf.capacity());
   }
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(boost::movelib::is_sorted(first, last, boost::movelib::unantistable(comp)));
}

template<class RandIt, class Comp, class XBuf>
void initialize_keys( RandIt first, RandIt last
                    , Comp comp
                    , XBuf & xbuf)
{
   unstable_sort(first, last, comp, xbuf);
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(boost::movelib::is_sorted_and_unique(first, last, comp));
}

template<class RandIt, class U>
void initialize_keys( RandIt first, RandIt last
                    , less
                    , U &)
{
   typedef typename iterator_traits<RandIt>::value_type value_type;
   std::size_t count = std::size_t(last - first);
   for(std::size_t i = 0; i != count; ++i){
      *first = static_cast<value_type>(i);
      ++first;
   }
}

template <class Unsigned>
Unsigned calculate_total_combined(Unsigned const len, Unsigned const l_prev_merged, Unsigned *pl_irreg_combined = 0)
{
   typedef Unsigned size_type;

   size_type const l_combined = size_type(2*l_prev_merged);
   size_type l_irreg_combined = size_type(len%l_combined);
   size_type l_total_combined = len;
   if(l_irreg_combined <= l_prev_merged){
      l_total_combined = size_type(l_total_combined - l_irreg_combined);
      l_irreg_combined = 0;
   }
   if(pl_irreg_combined)
      *pl_irreg_combined = l_irreg_combined;
   return l_total_combined;
}

template<class RandItKeys, class KeyCompare, class SizeType, class XBuf>
void combine_params
   ( RandItKeys const keys
   , KeyCompare key_comp
   , SizeType l_combined
   , SizeType const l_prev_merged
   , SizeType const l_block
   , XBuf & xbuf
   //Output
   , SizeType &n_block_a
   , SizeType &n_block_b
   , SizeType &l_irreg1
   , SizeType &l_irreg2
   //Options
   , bool do_initialize_keys = true)
{
   typedef SizeType   size_type;

   //Initial parameters for selection sort blocks
   l_irreg1 = size_type(l_prev_merged%l_block);
   l_irreg2 = size_type((l_combined-l_irreg1)%l_block);
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(((l_combined-l_irreg1-l_irreg2)%l_block) == 0);
   size_type const n_reg_block = size_type((l_combined-l_irreg1-l_irreg2)/l_block);
   n_block_a = l_prev_merged/l_block;
   n_block_b = size_type(n_reg_block - n_block_a);
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(n_reg_block>=n_block_a);

   //Key initialization
   if (do_initialize_keys) {
      initialize_keys(keys, keys + needed_keys_count(n_block_a, n_block_b), key_comp, xbuf);
   }
}



//////////////////////////////////
//
//          partial_merge
//
//////////////////////////////////
template<class InputIt1, class InputIt2, class OutputIt, class Compare, class Op>
OutputIt op_partial_merge_impl
   (InputIt1 &r_first1, InputIt1 const last1, InputIt2 &r_first2, InputIt2 const last2, OutputIt d_first, Compare comp, Op op)
{
   InputIt1 first1(r_first1);
   InputIt2 first2(r_first2);
   if(first2 != last2 && last1 != first1)
   while(1){
      if(comp(*first2, *first1)) {
         op(first2++, d_first++);
         if(first2 == last2){
            break;
         }
      }
      else{
         op(first1++, d_first++);
         if(first1 == last1){
            break;
         }
      }
   }
   r_first1 = first1;
   r_first2 = first2;
   return d_first;
}

// Merges as op_partial_merge_impl does, but rotating a hole through the free
// elements in front of range 1, which needs one move less per element than a
// swap (Katajainen, Pasanen and Teuhola, "Practical in-place mergesort", Nordic
// Journal of Computing 3(1), 1996, Section 2).
//
// The hole is the output position: the selected element is moved into it and the
// free element that follows the output takes the slot just vacated, which puts
// the hole at the next output position. The free elements end up rotated by one
// place, which no caller depends on.
//
// Consuming from range 2 shortens the free run. Should it run out, the hole is
// closed and the rest is swapped as before, so how long it lasts need not be
// known. Only callers passing free_run_t reach this.
template<class InputIt1, class InputIt2, class OutputIt, class Compare>
OutputIt op_partial_merge_hole_impl
   (InputIt1 &r_first1, InputIt1 const last1, InputIt2 &r_first2, InputIt2 const last2, OutputIt d_first, Compare comp)
{
   typedef typename iterator_traits<OutputIt>::value_type value_type;
   InputIt1 first1(r_first1);
   InputIt2 first2(r_first2);

   if(first2 != last2 && last1 != first1){
      value_type tmp(boost::move(*d_first));      //Opens the hole
      bool hole_open = true;
      while(1){
         if(comp(*first2, *first1)) {
            if(hole_open){
               InputIt2 const src = first2++;
               *d_first = boost::move(*src);
               OutputIt const next = d_first+1;
               if(!is_same_position(next, first1)){
                  *src = boost::move(*next);      //A free element follows the output
               }
               else{                              //The free run is spent
                  *src = boost::move(tmp);
                  hole_open = false;
               }
               ++d_first;
            }
            else{
               swap_op()(first2++, d_first++);
            }
            if(first2 == last2){
               break;
            }
         }
         else{
            if(hole_open){
               //Taking from range 1 leaves the free run as long as it was
               InputIt1 const src = first1++;
               *d_first = boost::move(*src);
               OutputIt const next = d_first+1;
               if(!is_same_position(next, src)){  //Else the hole already is there
                  *src = boost::move(*next);
               }
               ++d_first;
            }
            else{
               swap_op()(first1++, d_first++);
            }
            if(first1 == last1){
               break;
            }
         }
      }
      if(hole_open){
         *d_first = boost::move(tmp);             //Closes the hole
      }
   }
   r_first1 = first1;
   r_first2 = first2;
   return d_first;
}

template<class InputIt1, class InputIt2, class OutputIt, class Compare, class Op>
OutputIt op_partial_merge
   (InputIt1 &r_first1, InputIt1 const last1, InputIt2 &r_first2, InputIt2 const last2, OutputIt d_first, Compare comp, Op op, bool is_stable)
{
   return is_stable ? op_partial_merge_impl(r_first1, last1, r_first2, last2, d_first, comp, op)
                    : op_partial_merge_impl(r_first1, last1, r_first2, last2, d_first, antistable<Compare>(comp), op);
}

//////////////////////////////////
//////////////////////////////////
//////////////////////////////////
//
//    op_partial_merge_and_save
//
//////////////////////////////////
//////////////////////////////////
//////////////////////////////////
template<class InputIt1, class InputIt2, class OutputIt, class Compare, class Op>
OutputIt op_partial_merge_and_swap_impl
   (InputIt1 &r_first1, InputIt1 const last1, InputIt2 &r_first2, InputIt2 const last2, InputIt2 &r_first_min, OutputIt d_first, Compare comp, Op op)
{
   InputIt1 first1(r_first1);
   InputIt2 first2(r_first2);
   
   if(first2 != last2 && last1 != first1) {
      InputIt2 first_min(r_first_min);
      bool non_empty_ranges = true;
      do{
         if(comp(*first_min, *first1)) {
            op(three_way_t(), first2++, first_min++, d_first++);
            non_empty_ranges = first2 != last2;
         }
         else{
            op(first1++, d_first++);
            non_empty_ranges = first1 != last1;
         }
      } while(non_empty_ranges);
      r_first_min = first_min;
      r_first1 = first1;
      r_first2 = first2;
   }
   return d_first;
}

// op_partial_merge_and_swap_impl with a rotating hole. Taking from range 2 is a
// three way rotation, range2 -> range_min -> output, so the hole travels to the
// range 2 slot and a free element refills it. See op_partial_merge_hole_impl.
template<class InputIt1, class InputIt2, class OutputIt, class Compare>
OutputIt op_partial_merge_and_swap_hole_impl
   (InputIt1 &r_first1, InputIt1 const last1, InputIt2 &r_first2, InputIt2 const last2, InputIt2 &r_first_min, OutputIt d_first, Compare comp)
{
   typedef typename iterator_traits<OutputIt>::value_type value_type;
   InputIt1 first1(r_first1);
   InputIt2 first2(r_first2);

   if(first2 != last2 && last1 != first1) {
      InputIt2 first_min(r_first_min);
      value_type tmp(boost::move(*d_first));      //Opens the hole
      bool hole_open = true;
      bool non_empty_ranges = true;
      do{
         if(comp(*first_min, *first1)) {
            if(hole_open){
               InputIt2 const src  = first2++;
               InputIt2 const srcm = first_min++;
               *d_first = boost::move(*srcm);
               *srcm    = boost::move(*src);
               OutputIt const next = d_first+1;
               if(!is_same_position(next, first1)){
                  *src = boost::move(*next);      //A free element follows the output
               }
               else{                              //The free run is spent
                  *src = boost::move(tmp);
                  hole_open = false;
               }
               ++d_first;
            }
            else{
               swap_op()(three_way_t(), first2++, first_min++, d_first++);
            }
            non_empty_ranges = first2 != last2;
         }
         else{
            if(hole_open){
               InputIt1 const src = first1++;
               *d_first = boost::move(*src);
               OutputIt const next = d_first+1;
               if(!is_same_position(next, src)){  //Else the hole already is there
                  *src = boost::move(*next);
               }
               ++d_first;
            }
            else{
               swap_op()(first1++, d_first++);
            }
            non_empty_ranges = first1 != last1;
         }
      } while(non_empty_ranges);
      if(hole_open){
         *d_first = boost::move(tmp);             //Closes the hole
      }
      r_first_min = first_min;
   }
   r_first1 = first1;
   r_first2 = first2;
   return d_first;
}

template<class RandIt, class InputIt2, class OutputIt, class Compare, class Op>
OutputIt op_partial_merge_and_swap
   (RandIt &r_first1, RandIt const last1, InputIt2 &r_first2, InputIt2 const last2, InputIt2 &r_first_min, OutputIt d_first, Compare comp, Op op, bool is_stable)
{
   return is_stable ? op_partial_merge_and_swap_impl(r_first1, last1, r_first2, last2, r_first_min, d_first, comp, op)
                    : op_partial_merge_and_swap_impl(r_first1, last1, r_first2, last2, r_first_min, d_first, antistable<Compare>(comp), op);
}

// Range 1 is never longer than range 2: range 1 is the unmerged remainder of a
// block and range 2 is a whole block. Range 2 can't be exhausted before range 1
// then, because every step takes one element from range 1 and at most one from
// range 2. The same bound is required by the buffer, which holds one block and
// receives one element per step.
template<class RandIt1, class RandIt2, class RandItB, class Compare, class Op>
RandItB op_buffered_partial_merge_and_swap_to_range1_and_buffer
   ( RandIt1 first1, RandIt1 const last1
   , RandIt2 &rfirst2, RandIt2 const last2, RandIt2 &rfirst_min
   , RandItB &rfirstb, Compare comp, Op op )
{
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT((last1-first1) <= (last2-rfirst2));
   RandItB firstb = rfirstb;
   RandItB lastb  = firstb;
   RandIt2 first2 = rfirst2;

   //Move to buffer while merging
   //Three way moves need less moves when op is swap_op so use it
   //when merging elements from range2 to the destination occupied by range1
   if(first1 != last1 && first2 != last2){
      RandIt2 first_min = rfirst_min;
      op(four_way_t(), first2++, first_min++, first1++, lastb++);

      while(first1 != last1){
         if(comp(*first_min, *firstb)){
            op( four_way_t(), first2++, first_min++, first1++, lastb++);
         }
         else{
            op(three_way_t(), firstb++, first1++, lastb++);
         }
      }
      rfirst2 = first2;
      rfirstb = firstb;
      rfirst_min = first_min;
   }

   return lastb;
}

// See op_buffered_partial_merge_and_swap_to_range1_and_buffer: range 2 is never
// exhausted before range 1, as range 1 is never the longer range.
template<class RandIt1, class RandIt2, class RandItB, class Compare, class Op>
RandItB op_buffered_partial_merge_to_range1_and_buffer
   ( RandIt1 first1, RandIt1 const last1
   , RandIt2 &rfirst2, RandIt2 const last2
   , RandItB &rfirstb, Compare comp, Op op )
{
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT((last1-first1) <= (last2-rfirst2));
   RandItB firstb = rfirstb;
   RandItB lastb  = firstb;
   RandIt2 first2 = rfirst2;

   //Move to buffer while merging
   //Three way moves need less moves when op is swap_op so use it
   //when merging elements from range2 to the destination occupied by range1
   if(first1 != last1 && first2 != last2){
      op(three_way_t(), first2++, first1++, lastb++);

      while(first1 != last1){
         if (comp(*first2, *firstb)) {
            op(three_way_t(), first2++, first1++, lastb++);
         }
         else {
            op(three_way_t(), firstb++, first1++, lastb++);
         }
      }
      rfirst2 = first2;
      rfirstb = firstb;
   }

   return lastb;
}

// The rotating hole is taken only when the caller says the free elements are in
// front of range 1, the operation is a swap, and the free run is not empty
// already. Every other combination goes through the general merge below.
template<class InputIt1, class InputIt2, class OutputIt, class Compare, class Op, class FreeRun>
inline OutputIt op_partial_merge_sel
   (InputIt1 &f1, InputIt1 const l1, InputIt2 &f2, InputIt2 const l2, OutputIt d, Compare comp, Op op, FreeRun)
{  return op_partial_merge_impl(f1, l1, f2, l2, d, comp, op);  }

template<class InputIt1, class InputIt2, class OutputIt, class Compare, class Op, class FreeRun>
inline OutputIt op_partial_merge_and_swap_sel
   (InputIt1 &f1, InputIt1 const l1, InputIt2 &f2, InputIt2 const l2, InputIt2 &fm, OutputIt d, Compare comp, Op op, FreeRun)
{  return op_partial_merge_and_swap_impl(f1, l1, f2, l2, fm, d, comp, op);  }

template<class InputIt1, class InputIt2, class OutputIt, class Compare>
inline OutputIt op_partial_merge_sel
   (InputIt1 &f1, InputIt1 const l1, InputIt2 &f2, InputIt2 const l2, OutputIt d, Compare comp, swap_op, free_run_t)
{
   return is_same_position(d, f1)   //An empty free run leaves no slot to open the hole in
      ? op_partial_merge_impl(f1, l1, f2, l2, d, comp, swap_op())
      : op_partial_merge_hole_impl(f1, l1, f2, l2, d, comp);
}

template<class InputIt1, class InputIt2, class OutputIt, class Compare>
inline OutputIt op_partial_merge_and_swap_sel
   (InputIt1 &f1, InputIt1 const l1, InputIt2 &f2, InputIt2 const l2, InputIt2 &fm, OutputIt d, Compare comp, swap_op, free_run_t)
{
   return is_same_position(d, f1)
      ? op_partial_merge_and_swap_impl(f1, l1, f2, l2, fm, d, comp, swap_op())
      : op_partial_merge_and_swap_hole_impl(f1, l1, f2, l2, fm, d, comp);
}

template<class RandIt, class RandItBuf, class Compare, class Op, class FreeRun>
RandIt op_partial_merge_and_save_impl
   ( RandIt first1, RandIt const last1, RandIt &rfirst2, RandIt last2, RandIt first_min
   , RandItBuf &buf_first1_in_out, RandItBuf &buf_last1_in_out
   , Compare comp, Op op, FreeRun free_run
   )
{
   RandItBuf buf_first1 = buf_first1_in_out;
   RandItBuf buf_last1  = buf_last1_in_out;
   RandIt first2(rfirst2);

   bool const do_swap = first2 != first_min;
   if(buf_first1 == buf_last1){
      //Skip any element that does not need to be moved
      RandIt new_first1 = skip_until_merge(first1, last1, *first_min, comp);
      buf_first1 += (new_first1-first1);
      first1 = new_first1;
      buf_last1  = do_swap ? op_buffered_partial_merge_and_swap_to_range1_and_buffer(first1, last1, first2, last2, first_min, buf_first1, comp, op)
                           : op_buffered_partial_merge_to_range1_and_buffer    (first1, last1, first2, last2, buf_first1, comp, op);
      first1 = last1;
   }
   else{
      BOOST_MOVE_ADAPTIVE_SORT_INVARIANT((last1-first1) == (buf_last1 - buf_first1));
   }

   //Now merge from buffer
   first1 = do_swap ? op_partial_merge_and_swap_sel(buf_first1, buf_last1, first2, last2, first_min, first1, comp, op, free_run)
                    : op_partial_merge_sel    (buf_first1, buf_last1, first2, last2, first1, comp, op, free_run);
   buf_first1_in_out = buf_first1;
   buf_last1_in_out  = buf_last1;
   rfirst2 = first2;
   return first1;
}

template<class RandIt, class RandItBuf, class Compare, class Op, class FreeRun>
RandIt op_partial_merge_and_save
   ( RandIt first1, RandIt const last1, RandIt &rfirst2, RandIt last2, RandIt first_min
   , RandItBuf &buf_first1_in_out
   , RandItBuf &buf_last1_in_out
   , Compare comp
   , Op op
   , bool is_stable
   , FreeRun free_run)
{
   return is_stable
      ? op_partial_merge_and_save_impl
         (first1, last1, rfirst2, last2, first_min, buf_first1_in_out, buf_last1_in_out, comp, op, free_run)
      : op_partial_merge_and_save_impl
         (first1, last1, rfirst2, last2, first_min, buf_first1_in_out, buf_last1_in_out, antistable<Compare>(comp), op, free_run)
      ;
}

//////////////////////////////////
//////////////////////////////////
//////////////////////////////////
//
//    op_merge_blocks_with_irreg
//
//////////////////////////////////
//////////////////////////////////
//////////////////////////////////

template<class RandItKeys, class KeyCompare, class RandIt, class RandIt2, class OutputIt, class Compare, class Op>
OutputIt op_merge_blocks_with_irreg
   ( RandItKeys key_first
   , RandItKeys key_mid
   , KeyCompare key_comp
   , RandIt first_reg
   , RandIt2 &first_irr
   , RandIt2 const last_irr
   , OutputIt dest
   , typename iter_size<RandIt>::type const l_block
   , typename iter_size<RandIt>::type n_block_left
   , typename iter_size<RandIt>::type min_check
   , typename iter_size<RandIt>::type max_check
   , Compare comp, bool const is_stable, Op op)
{
   typedef typename iter_size<RandIt>::type size_type;

   for(; n_block_left; --n_block_left){
      size_type next_key_idx = find_next_block(key_first, key_comp, first_reg, l_block, min_check, max_check, comp);  
      max_check = min_value(max_value(max_check, size_type(next_key_idx+2u)), n_block_left);
      RandIt const last_reg  = first_reg + l_block;
      RandIt first_min = first_reg + size_type(next_key_idx*l_block);
      RandIt const last_min  = first_min + l_block;
      boost::movelib::ignore(last_min);

      BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(boost::movelib::is_sorted(first_reg, last_reg, comp));
      BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(!next_key_idx || boost::movelib::is_sorted(first_min, last_min, comp));
      BOOST_MOVE_ADAPTIVE_SORT_INVARIANT((!next_key_idx || !comp(*first_reg, *first_min )));

      OutputIt orig_dest = dest;
      boost::movelib::ignore(orig_dest);
      dest = next_key_idx ? op_partial_merge_and_swap(first_irr, last_irr, first_reg, last_reg, first_min, dest, comp, op, is_stable)
                          : op_partial_merge         (first_irr, last_irr, first_reg, last_reg, dest, comp, op, is_stable);
      BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(boost::movelib::is_sorted(orig_dest, dest, comp));

      if(first_reg == dest){
         dest = next_key_idx ? ::boost::adl_move_swap_ranges(first_min, last_min, first_reg)
                             : last_reg;
      }
      else{
         dest = next_key_idx ? op(three_way_forward_t(), first_reg, last_reg, first_min, dest)
                             : op(forward_t(), first_reg, last_reg, dest);
      }

      //The block of "first_min" now holds the block of "first_reg", so their keys must
      //be exchanged.
      RandItKeys const key_next(key_first + next_key_idx);
      update_key(key_next, key_first, key_mid);

      BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(boost::movelib::is_sorted(orig_dest, dest, comp));
      first_reg = last_reg;
      ++key_first;
      min_check = size_type(min_check - (min_check != 0));
      max_check = size_type(max_check - (max_check != 0));
   }
   return dest;
}

//////////////////////////////////
//////////////////////////////////
//////////////////////////////////
//
//    op_merge_blocks_left/right
//
//////////////////////////////////
//////////////////////////////////
//////////////////////////////////

template<class RandItKeys, class KeyCompare, class RandIt, class Compare, class Op>
void op_merge_blocks_left
   ( RandItKeys const key_first
   , KeyCompare key_comp
   , RandIt const first
   , typename iter_size<RandIt>::type const l_block
   , typename iter_size<RandIt>::type const l_irreg1
   , typename iter_size<RandIt>::type const n_block_a
   , typename iter_size<RandIt>::type const n_block_b
   , typename iter_size<RandIt>::type const l_irreg2
   , Compare comp, Op op)
{
   typedef typename iter_size<RandIt>::type       size_type;

   size_type const key_count = needed_keys_count(n_block_a, n_block_b);
   boost::movelib::ignore(key_count);

//   assert(n_block_a || n_block_b);
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(boost::movelib::is_sorted_and_unique(key_first, key_first + key_count, key_comp));
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(!n_block_b || n_block_a == count_if_with(key_first, key_first + key_count, key_comp, key_first[n_block_a]));

   size_type n_block_b_left = n_block_b;
   size_type n_block_a_left = n_block_a;
   size_type n_block_left = size_type(n_block_b + n_block_a);
   RandItKeys key_mid(key_first + n_block_a);

   RandIt buffer = first - l_block;
   RandIt first1 = first;
   RandIt last1  = first1 + l_irreg1;
   RandIt first2 = last1;
   RandIt const irreg2 = first2 + size_type(n_block_left*l_block);
   bool is_range1_A = true;

   RandItKeys key_range2(key_first);

   ////////////////////////////////////////////////////////////////////////////
   //Process all regular blocks before the irregular B block
   ////////////////////////////////////////////////////////////////////////////
   size_type min_check = n_block_a == n_block_left ? 0u : n_block_a;
   size_type max_check = min_value<size_type>(size_type(min_check+1u), n_block_left);
   for (; n_block_left; --n_block_left) {
      size_type const next_key_idx = find_next_block(key_range2, key_comp, first2, l_block, min_check, max_check, comp);
      max_check = min_value<size_type>(max_value<size_type>(max_check, size_type(next_key_idx+2u)), n_block_left);
      RandIt const first_min = first2 + size_type(next_key_idx*l_block);
      RandIt const last_min  = first_min + l_block;

      boost::movelib::ignore(last_min);
      RandIt const last2  = first2 + l_block;

      BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(boost::movelib::is_sorted(first1, last1, comp));
      BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(boost::movelib::is_sorted(first2, last2, comp));
      BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(!n_block_left || boost::movelib::is_sorted(first_min, last_min, comp));

      //Check if irregular b block should go here.
      //If so, break to the special code handling the irregular block
      if (!n_block_b_left &&
            ( (l_irreg2 && comp(*irreg2, *first_min)) || (!l_irreg2 && is_range1_A)) ){
         break;
      }

      RandItKeys const key_next(key_range2 + next_key_idx);
      bool const is_range2_A = key_mid == (key_first+key_count) || key_comp(*key_next, *key_mid);

      bool const is_buffer_middle = last1 == buffer;
      BOOST_MOVE_ADAPTIVE_SORT_INVARIANT( ( is_buffer_middle && size_type(first2-buffer) == l_block && buffer == last1) ||
                                          (!is_buffer_middle && size_type(first1-buffer) == l_block && first2 == last1));

      if(is_range1_A == is_range2_A){
         BOOST_MOVE_ADAPTIVE_SORT_INVARIANT((first1 == last1) || !comp(*first_min, last1[typename iterator_traits<RandIt>::difference_type(-1)]));
         if(!is_buffer_middle){
            buffer = op(forward_t(), first1, last1, buffer);
         }
         swap_and_update_key(key_next, key_range2, key_mid, first2, last2, first_min);
         first1 = first2;
         last1  = last2;
      }
      else {
         RandIt unmerged;
         RandIt buf_beg;
         RandIt buf_end;
         if(is_buffer_middle){
            buf_end = buf_beg = first2 - (last1-first1);
            unmerged = op_partial_merge_and_save( first1, last1, first2, last2, first_min
                                                , buf_beg, buf_end, comp, op, is_range1_A, free_run_t());
         }  
         else{
            buf_beg = first1;
            buf_end = last1;
            unmerged = op_partial_merge_and_save
               (buffer, buffer+(last1-first1), first2, last2, first_min, buf_beg, buf_end, comp, op, is_range1_A, free_run_t());
         }

         boost::movelib::ignore(unmerged);
         BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(boost::movelib::is_sorted(first-l_block, unmerged, comp));

         swap_and_update_key( key_next, key_range2, key_mid, first2, last2
                            , last_min - size_type(last2 - first2));

         if(buf_beg != buf_end){  //range2 exhausted: is_buffer_middle for the next iteration
            first1 = buf_beg;
            last1  = buf_end;
            BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(buf_end == (last2-l_block));
            buffer = last1;
         }
         else{ //range1 exhausted: !is_buffer_middle for the next iteration
            first1 = first2;
            last1  = last2;
            buffer = first2 - l_block;
            is_range1_A = is_range2_A;
         }
      }
      BOOST_MOVE_ADAPTIVE_SORT_INVARIANT( (is_range2_A && n_block_a_left) || (!is_range2_A && n_block_b_left));
      is_range2_A ? --n_block_a_left : --n_block_b_left;
      first2 = last2;
      //Update context
      ++key_range2;
      min_check = size_type(min_check - (min_check != 0));
      max_check = size_type(max_check - (max_check != 0));
   }

   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(!n_block_b || n_block_a == count_if_with(key_first, key_range2 + n_block_left, key_comp, *key_mid));
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(!n_block_b_left);

   ////////////////////////////////////////////////////////////////////////////
   //Process remaining range 1 left before the irregular B block
   ////////////////////////////////////////////////////////////////////////////
   bool const is_buffer_middle = last1 == buffer;
   RandIt first_irr2 = irreg2;
   RandIt const last_irr2  = first_irr2 + l_irreg2;
   if(l_irreg2 && is_range1_A){
      if(is_buffer_middle){
         first1 = skip_until_merge(first1, last1, *first_irr2, comp);
         //Even if we copy backward, no overlapping occurs so use forward copy
         //that can be faster specially with trivial types
         RandIt const new_first1 = first2 - (last1 - first1);
         op(forward_t(), first1, last1, new_first1);
         first1 = new_first1;
         last1 = first2;
         buffer = first1 - l_block;
      }
      buffer = op_partial_merge_impl(first1, last1, first_irr2, last_irr2, buffer, comp, op);
      buffer = op(forward_t(), first1, last1, buffer);
   }
   else if(!is_buffer_middle){
      buffer = op(forward_t(), first1, last1, buffer);
   }
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(boost::movelib::is_sorted(first-l_block, buffer, comp));

   ////////////////////////////////////////////////////////////////////////////
   //Process irregular B block and remaining A blocks
   ////////////////////////////////////////////////////////////////////////////
   buffer = op_merge_blocks_with_irreg
      ( key_range2, key_mid, key_comp, first2, first_irr2, last_irr2
      , buffer, l_block, n_block_left, min_check, max_check, comp, false, op);
   buffer = op(forward_t(), first_irr2, last_irr2, buffer);
   boost::movelib::ignore(buffer);
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(boost::movelib::is_sorted(first-l_block, buffer, comp));
}

// first - first element to merge.
// first[-l_block, 0) - buffer (if use_buf == true)
// l_block - length of regular blocks. First nblocks are stable sorted by 1st elements and key-coded
// keys - sequence of keys, in same order as blocks. key<midkey means stream A
// n_bef_irreg2/n_aft_irreg2 are regular blocks
// l_irreg2 is a irregular block, that is to be combined after n_bef_irreg2 blocks and before n_aft_irreg2 blocks
// If l_irreg2==0 then n_aft_irreg2==0 (no irregular blocks).
template<class RandItKeys, class KeyCompare, class RandIt, class Compare>
void merge_blocks_left
   ( RandItKeys const key_first
   , KeyCompare key_comp
   , RandIt const first
   , typename iter_size<RandIt>::type const l_block
   , typename iter_size<RandIt>::type const l_irreg1
   , typename iter_size<RandIt>::type const n_block_a
   , typename iter_size<RandIt>::type const n_block_b
   , typename iter_size<RandIt>::type const l_irreg2
   , Compare comp
   , bool const xbuf_used)
{
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(!n_block_b || n_block_a == count_if_with(key_first, key_first + needed_keys_count(n_block_a, n_block_b), key_comp, key_first[n_block_a]));
   if(xbuf_used){
      op_merge_blocks_left
         (key_first, key_comp, first, l_block, l_irreg1, n_block_a, n_block_b, l_irreg2, comp, move_op());
   }
   else{
      op_merge_blocks_left
         (key_first, key_comp, first, l_block, l_irreg1, n_block_a, n_block_b, l_irreg2, comp, swap_op());
   }
}

// first - first element to merge.
// [first+l_block*(n_bef_irreg2+n_aft_irreg2)+l_irreg2, first+l_block*(n_bef_irreg2+n_aft_irreg2+1)+l_irreg2) - buffer
// l_block - length of regular blocks. First nblocks are stable sorted by 1st elements and key-coded
// keys - sequence of keys, in same order as blocks. key<midkey means stream A
// n_bef_irreg2/n_aft_irreg2 are regular blocks
// l_irreg2 is a irregular block, that is to be combined after n_bef_irreg2 blocks and before n_aft_irreg2 blocks
// If l_irreg2==0 then n_aft_irreg2==0 (no irregular blocks).
template<class RandItKeys, class KeyCompare, class RandIt, class Compare>
void merge_blocks_right
   ( RandItKeys const key_first
   , KeyCompare key_comp
   , RandIt const first
   , typename iter_size<RandIt>::type const l_block
   , typename iter_size<RandIt>::type const n_block_a
   , typename iter_size<RandIt>::type const n_block_b
   , typename iter_size<RandIt>::type const l_irreg2
   , Compare comp
   , bool const xbuf_used)
{
   typedef typename iter_size<RandIt>::type size_type;
   merge_blocks_left
      ( (make_reverse_iterator)(key_first + needed_keys_count(n_block_a, n_block_b))
      , inverse<KeyCompare>(key_comp)
      , (make_reverse_iterator)(first + size_type((n_block_a+n_block_b)*l_block+l_irreg2))
      , l_block
      , l_irreg2
      , n_block_b
      , n_block_a
      , 0
      , inverse<Compare>(comp), xbuf_used);
}

//////////////////////////////////
//////////////////////////////////
//////////////////////////////////
//
//    op_merge_blocks_with_buf
//
//////////////////////////////////
//////////////////////////////////
//////////////////////////////////
template<class RandItKeys, class KeyCompare, class RandIt, class Compare, class Op, class RandItBuf>
void op_merge_blocks_with_buf
   ( RandItKeys key_first
   , KeyCompare key_comp
   , RandIt const first
   , typename iter_size<RandIt>::type const l_block
   , typename iter_size<RandIt>::type const l_irreg1
   , typename iter_size<RandIt>::type const n_block_a
   , typename iter_size<RandIt>::type const n_block_b
   , typename iter_size<RandIt>::type const l_irreg2
   , Compare comp
   , Op op
   , RandItBuf const buf_first)
{
   typedef typename iter_size<RandIt>::type size_type;
   size_type const key_count = needed_keys_count(n_block_a, n_block_b);
   boost::movelib::ignore(key_count);
   //assert(n_block_a || n_block_b);
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(boost::movelib::is_sorted_and_unique(key_first, key_first + key_count, key_comp));
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(!n_block_b || n_block_a == count_if_with(key_first, key_first + key_count, key_comp, key_first[n_block_a]));

   size_type n_block_b_left = n_block_b;
   size_type n_block_a_left = n_block_a;
   size_type n_block_left = size_type(n_block_b + n_block_a);
   RandItKeys key_mid(key_first + n_block_a);

   RandItBuf buffer = buf_first;
   RandItBuf buffer_end = buffer;
   RandIt first1 = first;
   RandIt last1  = first1 + l_irreg1;
   RandIt first2 = last1;
   RandIt const first_irr2 = first2 + size_type(n_block_left*l_block);
   bool is_range1_A = true;
   const size_type len = size_type(l_block * n_block_a + l_block * n_block_b + l_irreg1 + l_irreg2);
   boost::movelib::ignore(len);

   RandItKeys key_range2(key_first);

   ////////////////////////////////////////////////////////////////////////////
   //Process all regular blocks before the irregular B block
   ////////////////////////////////////////////////////////////////////////////
   size_type min_check = n_block_a == n_block_left ? 0u : n_block_a;
   size_type max_check = min_value(size_type(min_check+1), n_block_left);
   for (; n_block_left; --n_block_left) {
      size_type const next_key_idx = find_next_block(key_range2, key_comp, first2, l_block, min_check, max_check, comp);
      max_check = min_value(max_value(max_check, size_type(next_key_idx+2)), n_block_left);
      RandIt       first_min = first2 + size_type(next_key_idx*l_block);
      RandIt const last_min  = first_min + l_block;
      boost::movelib::ignore(last_min);
      RandIt const last2  = first2 + l_block;

      bool const buffer_empty = buffer == buffer_end;
      boost::movelib::ignore(buffer_empty);
      BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(buffer_empty ? boost::movelib::is_sorted(first1, last1, comp) : boost::movelib::is_sorted(buffer, buffer_end, comp));
      BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(boost::movelib::is_sorted(first2, last2, comp));
      BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(!n_block_left || boost::movelib::is_sorted(first_min, last_min, comp));

      //Check if irregular b block should go here.
      //If so, break to the special code handling the irregular block
      if (!n_block_b_left &&
            ( (l_irreg2 && comp(*first_irr2, *first_min)) || (!l_irreg2 && is_range1_A)) ){
         break;
      }

      RandItKeys const key_next(key_range2 + next_key_idx);
      bool const is_range2_A = key_mid == (key_first+key_count) || key_comp(*key_next, *key_mid);

      if(is_range1_A == is_range2_A){
         BOOST_MOVE_ADAPTIVE_SORT_INVARIANT((first1 == last1) || (buffer_empty ? !comp(*first_min, last1[-1]) : !comp(*first_min, buffer_end[-1])));
         //If buffered, put those elements in place
         RandIt res = op(forward_t(), buffer, buffer_end, first1);
         BOOST_MOVE_ADAPTIVE_SORT_PRINT_L2("   merge_blocks_w_fwd: ", len);
         buffer    = buffer_end = buf_first;
         BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(buffer_empty || res == last1);
         boost::movelib::ignore(res);
         //swap_and_update_key(key_next, key_range2, key_mid, first2, last2, first_min);
         buffer_end = buffer_and_update_key(key_next, key_range2, key_mid, first2, last2, first_min, buffer = buf_first, op);
         BOOST_MOVE_ADAPTIVE_SORT_PRINT_L2("   merge_blocks_w_swp: ", len);
         BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(boost::movelib::is_sorted(first_min, last_min, comp));
         first1 = first2;
         BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(boost::movelib::is_sorted(first, first1, comp));
      }
      else {
         //The save area is behind the output here, not in front of range 1, so
         //there is no free run to rotate a hole through
         RandIt const unmerged = op_partial_merge_and_save(first1, last1, first2, last2, first_min, buffer, buffer_end, comp, op, is_range1_A, no_free_run_t());
         BOOST_MOVE_ADAPTIVE_SORT_PRINT_L2("   merge_blocks_w_mrs: ", len);
         bool const is_range_1_empty = buffer == buffer_end;
         BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(is_range_1_empty || (buffer_end-buffer) == (last1+l_block-unmerged));
         if(is_range_1_empty){
            buffer    = buffer_end = buf_first;
            first_min = last_min - (last2 - first2);
            //swap_and_update_key(key_next, key_range2, key_mid, first2, last2, first_min);
            buffer_end = buffer_and_update_key(key_next, key_range2, key_mid, first2, last2, first_min, buf_first, op);
         }
         else{
            first_min = last_min;
            //swap_and_update_key(key_next, key_range2, key_mid, first2, last2, first_min);
            update_key(key_next, key_range2, key_mid);
         }
         BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(!is_range_1_empty || (last_min-first_min) == (last2-unmerged));
         BOOST_MOVE_ADAPTIVE_SORT_PRINT_L2("   merge_blocks_w_swp: ", len);
         BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(boost::movelib::is_sorted(first_min, last_min, comp));
         is_range1_A ^= is_range_1_empty;
         first1 = unmerged;
         BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(boost::movelib::is_sorted(first, unmerged, comp));
      }
      BOOST_MOVE_ADAPTIVE_SORT_INVARIANT( (is_range2_A && n_block_a_left) || (!is_range2_A && n_block_b_left));
      is_range2_A ? --n_block_a_left : --n_block_b_left;
      last1 += l_block;
      first2 = last2;
      //Update context
      ++key_range2;
      min_check = size_type(min_check - (min_check != 0));
      max_check = size_type(max_check - (max_check != 0));
   }
   RandIt res = op(forward_t(), buffer, buffer_end, first1);
   boost::movelib::ignore(res);
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(boost::movelib::is_sorted(first, res, comp));
   BOOST_MOVE_ADAPTIVE_SORT_PRINT_L2("   merge_blocks_w_fwd: ", len);

   ////////////////////////////////////////////////////////////////////////////
   //Process irregular B block and remaining A blocks
   ////////////////////////////////////////////////////////////////////////////
   RandIt const last_irr2 = first_irr2 + l_irreg2;
   op(forward_t(), first_irr2, first_irr2+l_irreg2, buf_first);
   BOOST_MOVE_ADAPTIVE_SORT_PRINT_L2("   merge_blocks_w_fwir:", len);
   buffer = buf_first;
   buffer_end = buffer+l_irreg2;

   reverse_iterator<RandItBuf> rbuf_beg(buffer_end);
   RandIt dest = op_merge_blocks_with_irreg
      ((make_reverse_iterator)(key_first + n_block_b + n_block_a), (make_reverse_iterator)(key_mid), inverse<KeyCompare>(key_comp)
      , (make_reverse_iterator)(first_irr2), rbuf_beg, (make_reverse_iterator)(buffer), (make_reverse_iterator)(last_irr2)
      , l_block, n_block_left, 0, n_block_left
      , inverse<Compare>(comp), true, op).base();
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(boost::movelib::is_sorted(dest, last_irr2, comp));
   BOOST_MOVE_ADAPTIVE_SORT_PRINT_L2("   merge_blocks_w_irg: ", len);

   buffer_end = rbuf_beg.base();
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT((dest-last1) == (buffer_end-buffer));
   op_merge_with_left_placed(is_range1_A ? first1 : last1, last1, dest, buffer, buffer_end, comp, op);
   BOOST_MOVE_ADAPTIVE_SORT_PRINT_L2("   merge_with_left_plc:", len);
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(boost::movelib::is_sorted(first, last_irr2, comp));
}

//////////////////////////////////
//////////////////////////////////
//////////////////////////////////
//
//  op_insertion_sort_step_left/right
//
//////////////////////////////////
//////////////////////////////////
//////////////////////////////////

template<class RandIt, class Compare, class Op>
typename iter_size<RandIt>::type
   op_insertion_sort_step_left
      ( RandIt const first
      , typename iter_size<RandIt>::type const length
      , typename iter_size<RandIt>::type const step
      , Compare comp, Op op)
{
   typedef typename iter_size<RandIt>::type       size_type;

   size_type const s = min_value<size_type>(step, MergeSortInsertionSortThreshold);
   size_type m = 0;

   while(size_type(length - m) > s){
      insertion_sort_op(first+m, first+m+s, first+m-s, comp, op);
      m = size_type(m + s);
   }
   insertion_sort_op(first+m, first+length, first+m-s, comp, op);
   return s;
}

template<class RandIt, class Compare, class Op>
void op_merge_right_step_once
      ( RandIt first_block
      , typename iter_size<RandIt>::type const elements_in_blocks
      , typename iter_size<RandIt>::type const l_build_buf
      , Compare comp
      , Op op)
{
   typedef typename iter_size<RandIt>::type size_type;
   size_type restk = size_type(elements_in_blocks%(2*l_build_buf));
   size_type p = size_type(elements_in_blocks - restk);
   BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(0 == (p%(2*l_build_buf)));

   if(restk <= l_build_buf){
      op(backward_t(),first_block+p, first_block+p+restk, first_block+p+restk+l_build_buf);
   }
   else{
      op_merge_right(first_block+p, first_block+p+l_build_buf, first_block+p+restk, first_block+p+restk+l_build_buf, comp, op);
   }
   while(p>0){
      p = size_type(p - 2u*l_build_buf);
      op_merge_right( first_block+p, first_block+size_type(p+l_build_buf)
                    , first_block+size_type(p+2*l_build_buf)
                    , first_block+size_type(p+3*l_build_buf), comp, op);
   }
}


//////////////////////////////////
//////////////////////////////////
//////////////////////////////////
//
//    op_merge_left_step_multiple
//
//////////////////////////////////
//////////////////////////////////
//////////////////////////////////
template<class RandIt, class Compare, class Op>
typename iter_size<RandIt>::type  
   op_merge_left_step_multiple
      ( RandIt first_block
      , typename iter_size<RandIt>::type const elements_in_blocks
      , typename iter_size<RandIt>::type l_merged
      , typename iter_size<RandIt>::type const l_build_buf
      , typename iter_size<RandIt>::type l_left_space
      , Compare comp
      , Op op)
{
   typedef typename iter_size<RandIt>::type size_type;
   for(; l_merged < l_build_buf && l_left_space >= l_merged; l_merged = size_type(l_merged*2u)){
      size_type p0=0;
      RandIt pos = first_block;
      while((elements_in_blocks - p0) > 2*l_merged) {
         op_merge_left(pos-l_merged, pos, pos+l_merged, pos+size_type(2*l_merged), comp, op);
         BOOST_MOVE_ADAPTIVE_SORT_INVARIANT(boost::movelib::is_sorted(pos-l_merged, pos+l_merged, comp));
         p0 = size_type(p0 + 2u*l_merged);
         pos = first_block+p0;
      }
      if((elements_in_blocks-p0) > l_merged) {
         op_merge_left(pos-l_merged, pos, pos+l_merged, first_block+elements_in_blocks, comp, op);
         BOOST_MOVE_ADAPTIVE_SORT_INVARIANT
            (boost::movelib::is_sorted
               (pos-l_merged, pos+size_type((first_block+elements_in_blocks-pos))-l_merged, comp));
      }
      else {
         op(forward_t(), pos, first_block+elements_in_blocks, pos-l_merged);
         BOOST_MOVE_ADAPTIVE_SORT_INVARIANT
            (boost::movelib::is_sorted
               (pos-l_merged, first_block+size_type(elements_in_blocks-l_merged), comp));
      }
      first_block  -= l_merged;
      l_left_space = size_type(l_left_space - l_merged);
   }
   return l_merged;
}


}  //namespace detail_adaptive {
}  //namespace movelib {
}  //namespace boost {

#if defined(BOOST_CLANG) || (defined(BOOST_GCC) && (BOOST_GCC >= 40600))
#pragma GCC diagnostic pop
#endif

#include <boost/move/detail/config_end.hpp>

#endif   //#define BOOST_MOVE_ADAPTIVE_SORT_MERGE_HPP
