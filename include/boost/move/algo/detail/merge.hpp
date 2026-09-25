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
#ifndef BOOST_MOVE_MERGE_HPP
#define BOOST_MOVE_MERGE_HPP

#include <boost/move/detail/config_begin.hpp>
#include <boost/move/adl_move_swap.hpp>
#include <boost/move/algo/detail/basic_op.hpp>
#include <boost/move/detail/iterator_traits.hpp>
#include <boost/move/detail/destruct_n.hpp>
#include <boost/move/algo/predicate.hpp>
#include <boost/move/algo/detail/search.hpp>
#include <boost/move/detail/iterator_to_raw_pointer.hpp>
#include <boost/move/detail/reverse_iterator.hpp>
#include <cassert>
#include <climits>
#include <cstddef>

#if defined(BOOST_CLANG) || (defined(BOOST_GCC) && (BOOST_GCC >= 40600))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

namespace boost {
namespace movelib {

template<class T, class RandRawIt = T*, class SizeType = typename iter_size<RandRawIt>::type>
class adaptive_xbuf
{
   adaptive_xbuf(const adaptive_xbuf &);
   adaptive_xbuf & operator=(const adaptive_xbuf &);

   #if !defined(UINTPTR_MAX)
   typedef std::size_t uintptr_t;
   #endif

   public:
   typedef RandRawIt iterator;
   typedef SizeType  size_type;

   inline adaptive_xbuf()
      : m_ptr(), m_size(0), m_capacity(0)
   {}

   inline adaptive_xbuf(RandRawIt raw_memory, size_type cap)
      : m_ptr(raw_memory), m_size(0), m_capacity(cap)
   {}

   template<class RandIt>
   void move_assign(RandIt first, size_type n)
   {
      typedef typename iterator_traits<RandIt>::difference_type rand_diff_t;
      if(n <= m_size){
         boost::move(first, first+rand_diff_t(n), m_ptr);
         size_type sz = m_size;
         while(sz-- != n){
            m_ptr[sz].~T();
         }
         m_size = n;
      }
      else{
         RandRawIt result = boost::move(first, first+rand_diff_t(m_size), m_ptr);
         boost::uninitialized_move(first+rand_diff_t(m_size), first+rand_diff_t(n), result);
         m_size = n;
      }
   }

   template<class RandIt>
   void push_back(RandIt first, size_type n)
   {
      assert(m_capacity - m_size >= n);
      boost::uninitialized_move(first, first+n, m_ptr+m_size);
      m_size += n;
   }

   template<class RandIt>
   iterator add(RandIt it)
   {
      assert(m_size < m_capacity);
      RandRawIt p_ret = m_ptr + m_size;
      ::new(&*p_ret) T(::boost::move(*it));
      ++m_size;
      return p_ret;
   }

   template<class RandIt>
   void insert(iterator pos, RandIt it)
   {
      if(pos == (m_ptr + m_size)){
         this->add(it);
      }
      else{
         this->add(m_ptr+m_size-1);
         //m_size updated
         boost::move_backward(pos, m_ptr+m_size-2, m_ptr+m_size-1);
         *pos = boost::move(*it);
      }
   }

   inline void set_size(size_type sz)
   {
      m_size = sz;
   }

   void shrink_to_fit(size_type const sz)
   {
      if(m_size > sz){
         for(size_type szt_i = sz; szt_i != m_size; ++szt_i){
            m_ptr[szt_i].~T();
         }
         m_size = sz;
      }
   }

   void initialize_until(size_type const sz, T &t)
   {
      assert(m_size < m_capacity);
      if(m_size < sz){
         BOOST_MOVE_TRY
         {
            ::new((void*)&m_ptr[m_size]) T(::boost::move(t));
            ++m_size;
            for(; m_size != sz; ++m_size){
               ::new((void*)&m_ptr[m_size]) T(::boost::move(m_ptr[m_size-1]));
            }
            t = ::boost::move(m_ptr[m_size-1]);
         }
         BOOST_MOVE_CATCH(...)
         {
            while(m_size)
            {
               --m_size;
               m_ptr[m_size].~T();
            }
            BOOST_MOVE_RETHROW
         }
         BOOST_MOVE_CATCH_END
      }
   }

   private:
   template<class RIt>
   inline static bool is_raw_ptr(RIt)
   {
      return false;
   }

   inline static bool is_raw_ptr(T*)
   {
      return true;
   }

   public:
   template<class U>
   bool supports_aligned_trailing(size_type sz, size_type trail_count) const
   {
      if(this->is_raw_ptr(this->data()) && m_capacity){
         uintptr_t u_addr_sz = uintptr_t(&*(this->data()+sz));
         uintptr_t u_addr_cp = uintptr_t(&*(this->data()+this->capacity()));
         u_addr_sz = ((u_addr_sz + sizeof(U)-1)/sizeof(U))*sizeof(U);
         return (u_addr_cp >= u_addr_sz) && ((u_addr_cp - u_addr_sz)/sizeof(U) >= trail_count);
      }
      return false;
   }

   template<class U>
   inline U *aligned_trailing() const
   {
      return this->aligned_trailing<U>(this->size());
   }

   template<class U>
   inline U *aligned_trailing(size_type pos) const
   {
      uintptr_t u_addr = uintptr_t(&*(this->data()+pos));
      u_addr = ((u_addr + sizeof(U)-1)/sizeof(U))*sizeof(U);
      return (U*)u_addr;
   }

   inline ~adaptive_xbuf()
   {
      this->clear();
   }

   inline size_type capacity() const
   {  return m_capacity;   }

   inline iterator data() const
   {  return m_ptr;   }

   inline iterator begin() const
   {  return m_ptr;   }

   inline iterator end() const
   {  return m_ptr+m_size;   }

   inline size_type size() const
   {  return m_size;   }

   inline bool empty() const
   {  return !m_size;   }

   inline void clear()
   {
      this->shrink_to_fit(0u);
   }

   private:
   RandRawIt m_ptr;
   size_type m_size;
   size_type m_capacity;
};

template<class Iterator, class SizeType, class Op>
class range_xbuf
{
   range_xbuf(const range_xbuf &);
   range_xbuf & operator=(const range_xbuf &);

   public:
   typedef SizeType size_type;
   typedef Iterator iterator;

   range_xbuf(Iterator first, Iterator last)
      : m_first(first), m_last(first), m_cap(last)
   {}

   template<class RandIt>
   void move_assign(RandIt first, size_type n)
   {
      assert(size_type(n) <= size_type(m_cap-m_first));
      typedef typename iter_difference<RandIt>::type d_type;
      m_last = Op()(forward_t(), first, first+d_type(n), m_first);
   }

   ~range_xbuf()
   {}

   size_type capacity() const
   {  return m_cap-m_first;   }

   Iterator data() const
   {  return m_first;   }

   Iterator end() const
   {  return m_last;   }

   size_type size() const
   {  return m_last-m_first;   }

   bool empty() const
   {  return m_first == m_last;   }

   void clear()
   {
      m_last = m_first;
   }

   template<class RandIt>
   iterator add(RandIt it)
   {
      Iterator pos(m_last);
      *pos = boost::move(*it);
      ++m_last;
      return pos;
   }

   void set_size(size_type sz)
   {
      m_last  = m_first;
      m_last += sz;
   }

   private:
   Iterator const m_first;
   Iterator m_last;
   Iterator const m_cap;
};

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
   //r*r can not overflow as r <= 2^(bits/2)-1.
   //It also avoids a division (slow) that can also cause divice by zero
   return Unsigned(r + Unsigned(Unsigned(r*r) != n));
}

//Modified version from "An Optimal In-Place Array Rotation Algorithm", Ching-Kuang Shene
template<typename Unsigned>
Unsigned gcd(Unsigned x, Unsigned y)
{
   if(0 == ((x &(x-1)) | (y & (y-1)))){
      return x < y ? x : y;
   }
   else{
      Unsigned z = 1;
      while((!(x&1)) & (!(y&1))){
         z = Unsigned(z << 1);
         x = Unsigned(x >> 1);
         y = Unsigned(y >> 1);
      }
      while(x && y){
         if(!(x&1))
            x = Unsigned(x >> 1);
         else if(!(y&1))
            y = Unsigned (y >> 1);
         else if(x >=y)
            x = Unsigned((x-y) >> 1u);
         else
            y = Unsigned((y-x) >> 1);
      }
      return Unsigned(z*(x+y));
   }
}

template<typename RandIt>
RandIt rotate_gcd(RandIt first, RandIt middle, RandIt last)
{
   typedef typename iter_size<RandIt>::type size_type;
   typedef typename iterator_traits<RandIt>::value_type value_type;

   if(first == middle)
      return last;
   if(middle == last)
      return first;
   const size_type middle_pos = size_type(middle - first);
   RandIt ret = last - middle_pos;
   if (middle == ret){
      boost::adl_move_swap_ranges(first, middle, middle);
   }
   else{
      const size_type length = size_type(last - first);
      for( RandIt it_i(first), it_gcd(it_i + gcd(length, middle_pos))
         ; it_i != it_gcd
         ; ++it_i){
         value_type temp(boost::move(*it_i));
         RandIt it_j = it_i;
         RandIt it_k = it_j+middle_pos;
         do{
            *it_j = boost::move(*it_k);
            it_j = it_k;
            size_type const left = size_type(last - it_j);
            it_k = left > middle_pos ? it_j + middle_pos : first + middle_pos - left;
         } while(it_k != it_i);
         *it_j = boost::move(temp);
      }
   }
   return ret;
}

///////////////////////////////////////////////////////////////////////////////
//
//                        MERGE LEFT / MERGE RIGHT
//
///////////////////////////////////////////////////////////////////////////////

// op_merge_left and op_merge_right merge the two adjacent sorted ranges
//
//    range 1 = [first1, last1)   and   range 2 = [last1, last2)
//
// into a run of free elements next to them, which the adaptive algorithms call
// the buffer: in front of range 1 for op_merge_left and behind range 2 for
// op_merge_right. Those are the only elements the merge writes outside the two
// ranges, and it does not care what they hold.
//
//    op_merge_left
//       buf_first   first1     last1      last2
//       [  buffer  ][ range 1 ][ range 2 ]
//    -> [       merged       ][  buffer  ]
//
//    op_merge_right
//       first1     last1      last2       buf_last
//       [ range 1 ][ range 2 ][  buffer  ]
//    -> [  buffer  ][       merged       ]
//
// So the merged range comes out where the buffer used to start (op_merge_left)
// or where it used to end (op_merge_right), and the buffer ends up on the other
// side of it. The merge is stable: of two equivalent elements the one of
// range 1 comes first.
//
// "op" says what to do with each element that is placed. move_op moves it, so
// the buffer is overwritten and what is left of it is a run of moved-from
// elements. swap_op swaps it, so no value is lost: the elements of the buffer
// are carried to the other side of the merged range, in an order the caller
// must not rely on.
//
// Both need the buffer to be at least as long as the range that has to travel
// across it, that is
//
//    op_merge_left    (first1 - buf_first) >= (last2 - last1)
//    op_merge_right   (buf_last - last2)   >= (last1 - first1)
//
// because the output only gains on that range when one of its elements is
// placed. With a shorter buffer the output would catch up with the other range
// and overwrite elements that are not consumed yet.

///////////////////////////////////////////////////////////////////////////////
//
//                       MERGE LEFT WITH A ROTATING HOLE
//
///////////////////////////////////////////////////////////////////////////////

// Moves [first, last) to [hole, hole+(last-first)), refilling each slot it
// vacates with the element next to the hole. [hole, first) must hold free
// elements, so that the hole ends up just before the elements moved there.
// Returns the new position of the hole.
template<class RandIt>
RandIt op_shift_hole(RandIt hole, RandIt first, RandIt const last)
{
   for(; first != last; ++first){
      *hole = boost::move(*first);
      RandIt const next_hole = hole+1;
      if(next_hole != first){    //A free element follows the hole, put it in the slot just vacated
         *first = boost::move(*next_hole);
      }
      hole = next_hole;
   }
   return hole;
}

// Same result as op_merge_left with swap_op, and chosen for it because this
// overload is the more specialized one, but each element costs two moves
// instead of the three a swap needs (Katajainen, Pasanen and Teuhola, "Practical
// in-place mergesort", Nordic Journal of Computing 3(1), 1996, Section 2).
//
// One buffer element is extracted, leaving a hole at the output position. The
// selected element is moved into the hole and the buffer element next to the
// hole is moved into the slot just vacated, which recreates the hole at the next
// output position. Buffer elements end up rotated by one place with respect to
// the swapping version, which is harmless because the buffer holds the collected
// unique elements and the caller does not preserve their order either
// (adaptive_sort sorts them in the final merge).
//
// Needs the same buffer length as the general version, and for the same reason:
// the buffer in front of range 1 must only run out once range 2 is exhausted.

template<class RandIt, class Compare>
inline void op_merge_left( RandIt buf_first
                         , RandIt first1
                         , RandIt const last1
                         , RandIt const last2
                         , Compare comp
                         , swap_op)
{
   typedef typename iterator_traits<RandIt>::value_type value_type;
   RandIt first2 = last1;
   assert(buf_first != first1);
   assert(std::size_t(first1 - buf_first) >= std::size_t(last2 - last1));

   RandIt hole = buf_first;
   value_type tmp(boost::move(*hole));   //Opens the hole
   bool buf_exhausted = false;

   while(first1 != last1 && first2 != last2){
      RandIt const src = comp(*first2, *first1) ? first2++ : first1++;
      *hole = boost::move(*src);
      RandIt const next_hole = hole+1;
      if(next_hole == src){            //The hole already moved with the consumed element
         hole = src;
      }
      else if(next_hole != first1){    //A buffer element follows the hole
         *src = boost::move(*next_hole);
         hole = next_hole;
      }
      else{
         //The buffer in front of the first range is exhausted. As the buffer is
         //at least as long as the second range, that can only happen just as the
         //second range empties, and the rest of the first range is then already
         //in its final place.
         hole = src;
         assert(first2 == last2);
         buf_exhausted = true;
         break;
      }
   }
   //One range is left. Its elements just shift over the free ones before them.
   if(!buf_exhausted){
      hole = first1 == last1 ? op_shift_hole(hole, first2, last2)
                             : op_shift_hole(hole, first1, last1);
   }
   *hole = boost::move(tmp);           //Closes the hole
}

// Merges range 1 and range 2 forward into the buffer that precedes them, as
// described above. Each placed element costs whatever "op" costs: one move for
// move_op, one swap for swap_op.
template<class RandIt, class Compare, class Op>
void op_merge_left( RandIt buf_first
                    , RandIt first1
                    , RandIt const last1
                    , RandIt const last2
                    , Compare comp
                    , Op op)
{
   RandIt first2 = last1;
   bool is_range_1_left = first1 != last1;
   for( ; is_range_1_left && first2 != last2; ++buf_first){
      if(comp(*first2, *first1)){
         op(first2, buf_first);
         ++first2;
      }
      else{
         op(first1, buf_first);
         ++first1;
         is_range_1_left = first1 != last1;
      }
   }
   if(!is_range_1_left){
      op(forward_t(), first2, last2, buf_first);
      return;
   }
   if(buf_first != first1){//In case all remaining elements are in the same place
                           //(e.g. buffer is exactly the size of the second half
                           //and all elements from the second half are less)
      op(forward_t(), first1, last1, buf_first);
   }
}

// Merges range 1 and range 2 backward into the buffer that follows them, as
// described above.
//
// It is the mirror image of op_merge_left: reversing the three ranges puts the
// buffer in front of the merged output again, and the inverse comparison makes
// the reversed merge the stable one (the reversal swaps the roles of the two
// ranges, so the tie rule of op_merge_left still puts range 1 first here).
template<class RandIt, class Compare, class Op>
inline void op_merge_right
   (RandIt const first1, RandIt last1, RandIt last2, RandIt buf_last, Compare comp, Op op)
{
   op_merge_left( (make_reverse_iterator)(buf_last)
                , (make_reverse_iterator)(last2)
                , (make_reverse_iterator)(last1)
                , (make_reverse_iterator)(first1)
                , inverse<Compare>(comp), op);
}

///////////////////////////////////////////////////////////////////////////////
//
//                            BUFFERED MERGE
//
///////////////////////////////////////////////////////////////////////////////

// Merges the two adjacent sorted ranges
//
//    range 1 = [first, middle)   and   range 2 = [middle, last)
//
// in place, with the help of "xbuf", which must hold at least
// min(len1, len2) elements. Nothing outside [first, last) is written and the
// merge is stable.
//
// The leading and trailing elements that are already in their final position
// are skipped first. Then the shorter half is moved into "xbuf", which leaves
// a free run inside [first, last) as long as that half, and the half that
// stayed in place already sits at the end of the output it belongs to. So the
// merge is finished by op_merge_with_right_placed when the first half was the
// one buffered, and by op_merge_with_left_placed when it was the second one:
//
//    len1 <= len2   [ range 1 ][ range 2 ]      xbuf = range 1
//                   [  free   ][ range 2 ]
//                -> [       merged       ]
//
//    len1 >  len2   [ range 1 ][ range 2 ]      xbuf = range 2
//                   [ range 1 ][  free   ]
//                -> [       merged       ]
//
// The half that goes to "xbuf" is always moved there, so its previous contents
// are lost whatever "op" is, and on return it holds that many moved-from
// elements. "op" only places the elements of the merge itself.
//
// "xbuf" is any of the buffer types of this header: adaptive_xbuf, which owns
// raw storage and constructs the elements it is given, or range_xbuf, which is
// a range of elements that already exist.
template<class RandIt, class Compare, class Op, class Buf>
void op_buffered_merge
      ( RandIt first, RandIt const middle, RandIt last
      , Compare comp, Op op
      , Buf &xbuf)
{
   if(first != middle && middle != last && comp(*middle, middle[-1])){
      typedef typename iter_size<RandIt>::type   size_type;
      size_type const len1 = size_type(middle-first);
      size_type const len2 = size_type(last-middle);
      if(len1 <= len2){
         first = boost::movelib::upper_bound(first, middle, *middle, comp);
         xbuf.move_assign(first, size_type(middle-first));
         op_merge_with_right_placed
            (xbuf.data(), xbuf.end(), first, middle, last, comp, op);
      }
      else{
         last = boost::movelib::lower_bound(middle, last, middle[-1], comp);
         xbuf.move_assign(middle, size_type(last-middle));
         op_merge_with_left_placed
            (first, middle, last, xbuf.data(), xbuf.end(), comp, op);
      }
   }
}

// op_buffered_merge that moves the elements it places
template<class RandIt, class Compare, class XBuf>
void buffered_merge
      ( RandIt first, RandIt const middle, RandIt last
      , Compare comp
      , XBuf &xbuf)
{
   op_buffered_merge(first, middle, last, comp, move_op(), xbuf);
}

///////////////////////////////////////////////////////////////////////////////
//
//                            BUFFERLESS MERGE
//
///////////////////////////////////////////////////////////////////////////////

// Merges the two adjacent sorted ranges
//
//    range 1 = [first, middle)   and   range 2 = [middle, last)
//
// in place, into [first, last), with no additional memory at all: elements are
// carried to their place by rotations. Stable, like every merge of this header.
//
// This one walks the shorter range element by element. Taking range 1 as the
// shorter one, each step binary searches in range 2 the elements that must
// precede the leading element of range 1, rotates them in front of it, and then
// skips the elements of range 1 that the rotation already left in place:
//
//       first      middle
//       [ x rest1 ][ b1 b2 ][ rest2 ] last         b1, b2 < x
//    -> [ b1 b2 ][ x rest1 ][ rest2 ]
//                ^the next step starts here
//
// When range 2 is the shorter one the same is done from the other end.
//
// Only one binary search and one rotation are paid per element of the shorter
// range, so the comparisons are about min*log(max), which is very few when one
// range is much shorter than the other. The quadratic term is paid on the
// shorter range alone, so this beats the recursive merge below while that range
// stays near sqrt(len1+len2).
//
//Complexity: min(len1,len2)^2 + max(len1,len2)
template<class RandIt, class Compare>
void merge_bufferless_ON2(RandIt first, RandIt middle, RandIt last, Compare comp)
{
   if((middle - first) < (last - middle)){
      while(first != middle){
         RandIt const old_last1 = middle;
         middle = boost::movelib::lower_bound(middle, last, *first, comp);
         first = rotate_gcd(first, old_last1, middle);
         if(middle == last){
            break;
         }
         do{
            ++first;
         } while(first != middle && !comp(*middle, *first));
      }
   }
   else{
      while(middle != last){
         RandIt p = boost::movelib::upper_bound(first, middle, last[-1], comp);
         last = rotate_gcd(p, middle, last);
         middle = p;
         if(middle == first){
            break;
         }
         --p;
         do{
            --last;
         } while(middle != last && !comp(last[-1], *p));
      }
   }
}

static const std::size_t MergeBufferlessONLogNRotationThreshold = 16u;

// Merges the same two adjacent ranges as merge_bufferless_ON2, in place and
// with no additional memory, but by halving instead of by walking: "len1" and
// "len2" are their lengths, which the caller already knows.
//
// The longer range is cut in half, the matching cut of the other one is found
// with a binary search, and one rotation puts the two inner pieces in the right
// order. That leaves two independent merges that touch no common element, one
// on each side of the new middle:
//
//       first     first_cut middle    second_cut
//       [   A1   ][   A2   ][   B1   ][   B2   ] last    B1 goes before A2
//    -> [   A1   ][   B1   ][   A2   ][   B2   ]
//                           ^new_middle
//
// A1 is then merged with B1 and A2 with B2. The bigger of the two is continued
// by the loop rather than by a recursive call, so the recursion only ever
// descends into the smaller half.
//
// Ranges short enough to make the halving not worth it go to
// merge_bufferless_ON2.
template <class RandIt, class Compare>
void merge_bufferless_ONlogN_recursive
   ( RandIt first, RandIt middle, RandIt last
   , typename iter_size<RandIt>::type len1
   , typename iter_size<RandIt>::type len2
   , Compare comp)
{
   typedef typename iter_size<RandIt>::type size_type;

   while(1) {
      //trivial cases
      if (!len2) {
         return;
      }
      else if (!len1) {
         return;
      }
      else if (size_type(len1 | len2) == 1u) {
         if (comp(*middle, *first))
            adl_move_swap(*first, *middle);  
         return;
      }
      else if(size_type(len1+len2) < MergeBufferlessONLogNRotationThreshold){
         //Base case: below this size the binary searches and the rotation
         //of the halving cost more than walking the shorter range
         merge_bufferless_ON2(first, middle, last, comp);
         return;
      }

      RandIt first_cut = first;
      RandIt second_cut = middle;
      size_type len11 = 0;
      size_type len22 = 0;
      if (len1 > len2) {
         len11 = len1 / 2;
         first_cut +=  len11;
         second_cut = boost::movelib::lower_bound(middle, last, *first_cut, comp);
         len22 = size_type(second_cut - middle);
      }
      else {
         len22 = len2 / 2;
         second_cut += len22;
         first_cut = boost::movelib::upper_bound(first, middle, *second_cut, comp);
         len11 = size_type(first_cut - first);
      }
      RandIt new_middle = rotate_gcd(first_cut, middle, second_cut);

      //Avoid one recursive call doing a manual tail call elimination on the biggest range
      const size_type len_internal = size_type(len11+len22);
      if( len_internal < (len1 + len2 - len_internal) ) {
         merge_bufferless_ONlogN_recursive(first, first_cut,  new_middle, len11, len22, comp);
         first = new_middle;
         middle = second_cut;
         len1 = size_type(len1-len11);
         len2 = size_type(len2-len22);
      }
      else {
         merge_bufferless_ONlogN_recursive
            (new_middle, second_cut, last, size_type(len1 - len11), size_type(len2 - len22), comp);
         middle = first_cut;
         last = new_middle;
         len1 = len11;
         len2 = len22;
      }
   }
}


//Complexity: NlogN
template<class RandIt, class Compare>
void merge_bufferless_ONlogN(RandIt first, RandIt middle, RandIt last, Compare comp)
{
   typedef typename iter_size<RandIt>::type size_type;
   merge_bufferless_ONlogN_recursive
      (first, middle, last, size_type(middle - first), size_type(last - middle), comp);
}

///////////////////////////////////////////////////////////////////////////////
//
//                        MERGE WITH ONE RANGE PLACED
//
///////////////////////////////////////////////////////////////////////////////

// op_merge_with_right_placed and op_merge_with_left_placed also merge two
// sorted ranges, but unlike the merges above the two ranges are not adjacent:
// one of them already sits inside the destination, at the end it belongs to,
// and the other one is somewhere else, usually a buffer the caller filled.
// The destination is the range that one covers plus the free run next to it,
// and that free run must be exactly as long as the range that comes from
// outside:
//
//    op_merge_with_right_placed      range 2 is the one already placed
//
//       first      last                            (anywhere else)
//       [ range 1 ]
//
//       dest_first r_first    r_last
//       [   free  ][ range 2 ]
//    -> [       merged       ]
//
//    op_merge_with_left_placed       range 1 is the one already placed
//
//                  r_first    r_last                (anywhere else)
//                  [ range 2 ]
//
//       first      last       dest_last
//       [ range 1 ][   free  ]
//    -> [       merged       ]
//
// op_merge_with_right_placed fills the destination forward from dest_first and
// op_merge_with_left_placed backward from dest_last, so in both the output runs
// towards the range that is already in place. Neither ever overtakes it: the
// free run always holds exactly as many slots as elements are left outside, so
// when those run out the output has reached the placed range and whatever is
// left of it needs no move at all. That is why they take no buffer of their own
// and write nothing outside the destination.
//
// The merge is stable, range 1 first on equivalent elements, and "op" places
// every element. move_op leaves the outside range moved-from, so its storage is
// free afterwards. swap_op loses no value: what the destination held is carried
// out into the outside range instead, which is what the callers that keep an
// internal buffer there want, and it comes back in an order they must not rely
// on.

// [r_first, r_last) are already in the right part of the destination range.
template <class Compare, class InputIterator, class InputOutIterator, class Op>
void op_merge_with_right_placed
   ( InputIterator first, InputIterator last
   , InputOutIterator dest_first, InputOutIterator r_first, InputOutIterator r_last
   , Compare comp, Op op)
{
   assert((last - first) == (r_first - dest_first));
   while ( first != last ) {
      if (r_first == r_last) {
         InputOutIterator end = op(forward_t(), first, last, dest_first);
         assert(end == r_last);
         boost::movelib::ignore(end);
         return;
      }
      else if (comp(*r_first, *first)) {
         op(r_first, dest_first);
         ++r_first;
      }
      else {
         op(first, dest_first);
         ++first;
      }
      ++dest_first;
   }
   // Remaining [r_first, r_last) already in the correct place
}

// [first, last) are already in the left part of the destination range.
//
// Mirror image of op_merge_with_right_placed: reversing the destination turns
// the range that is already in its left part into a range in the right part,
// and the inverse comparison makes the reversed merge the stable one.
template <class Compare, class Op, class BidirIterator, class BidirOutIterator>
inline void op_merge_with_left_placed
   ( BidirOutIterator const first, BidirOutIterator last, BidirOutIterator dest_last
   , BidirIterator const r_first, BidirIterator r_last
   , Compare comp, Op op)
{
   op_merge_with_right_placed
      ( (make_reverse_iterator)(r_last), (make_reverse_iterator)(r_first)
      , (make_reverse_iterator)(dest_last)
      , (make_reverse_iterator)(last), (make_reverse_iterator)(first)
      , inverse<Compare>(comp), op);
}

// @endcond

// [r_first, r_last) are already in the right part of the destination range.
template <class Compare, class InputIterator, class InputOutIterator>
void merge_with_right_placed
   ( InputIterator first, InputIterator last
   , InputOutIterator dest_first, InputOutIterator r_first, InputOutIterator r_last
   , Compare comp)
{
   op_merge_with_right_placed(first, last, dest_first, r_first, r_last, comp, move_op());
}

// [r_first, r_last) are already in the right part of the destination range.
// [dest_first, r_first) is uninitialized memory
template <class Compare, class InputIterator, class InputOutIterator>
void uninitialized_merge_with_right_placed
   ( InputIterator first, InputIterator last
   , InputOutIterator dest_first, InputOutIterator r_first, InputOutIterator r_last
   , Compare comp)
{
   assert((last - first) == (r_first - dest_first));
   typedef typename iterator_traits<InputOutIterator>::value_type value_type;
   InputOutIterator const original_r_first = r_first;

   destruct_n<value_type, InputOutIterator> d(dest_first);

   while ( first != last && dest_first != original_r_first ) {
      if (r_first == r_last) {
         for(; dest_first != original_r_first; ++dest_first, ++first){
            ::new((iterator_to_raw_pointer)(dest_first)) value_type(::boost::move(*first));
            d.incr();
         }
         d.release();
         InputOutIterator end = ::boost::move(first, last, original_r_first);
         assert(end == r_last);
         boost::movelib::ignore(end);
         return;
      }
      else if (comp(*r_first, *first)) {
         ::new((iterator_to_raw_pointer)(dest_first)) value_type(::boost::move(*r_first));
         d.incr();
         ++r_first;
      }
      else {
         ::new((iterator_to_raw_pointer)(dest_first)) value_type(::boost::move(*first));
         d.incr();
         ++first;
      }
      ++dest_first;
   }
   merge_with_right_placed(first, last, original_r_first, r_first, r_last, comp);
   d.release();
}

/// This is a helper function for the merge routines.
template<typename BidirectionalIterator1, typename BidirectionalIterator2>
   BidirectionalIterator1
   rotate_adaptive(BidirectionalIterator1 first,
      BidirectionalIterator1 middle,
      BidirectionalIterator1 last,
      typename iter_size<BidirectionalIterator1>::type len1,
      typename iter_size<BidirectionalIterator1>::type len2,
      BidirectionalIterator2 buffer,
      typename iter_size<BidirectionalIterator1>::type buffer_size)
{
   if (len1 > len2 && len2 <= buffer_size)
   {
      if(len2) //Protect against self-move ranges
      {
         BidirectionalIterator2 buffer_end = boost::move(middle, last, buffer);
         boost::move_backward(first, middle, last);
         return boost::move(buffer, buffer_end, first);
      }
      else
         return first;
   }
   else if (len1 <= buffer_size)
   {
      if(len1) //Protect against self-move ranges
      {
         BidirectionalIterator2 buffer_end = boost::move(first, middle, buffer);
         BidirectionalIterator1 ret = boost::move(middle, last, first);
         boost::move(buffer, buffer_end, ret);
         return ret;
      }
      else
         return last;
   }
   else
      return rotate_gcd(first, middle, last);
}

template<typename BidirectionalIterator,
   typename Pointer, typename Compare>
   void merge_adaptive_ONlogN_recursive
   (BidirectionalIterator first,
      BidirectionalIterator middle,
      BidirectionalIterator last,
      typename  iter_size<BidirectionalIterator>::type len1,
      typename  iter_size<BidirectionalIterator>::type len2,
      Pointer buffer,
      typename  iter_size<BidirectionalIterator>::type buffer_size,
      Compare comp)
{
   typedef typename  iter_size<BidirectionalIterator>::type size_type;
   //trivial cases
   if (!len2 || !len1) {
      // no-op
   }
   else if (len1 <= buffer_size || len2 <= buffer_size) {
      range_xbuf<Pointer, size_type, move_op> rxbuf(buffer, buffer + buffer_size);
      buffered_merge(first, middle, last, comp, rxbuf);
   }
   else if (size_type(len1 + len2) == 2u) {
      if (comp(*middle, *first))
         adl_move_swap(*first, *middle);
   }
   else if (size_type(len1 + len2) < MergeBufferlessONLogNRotationThreshold) {
      //Base case: below this size the binary searches and the rotation
      //of the halving cost more than walking the shorter range
      merge_bufferless_ON2(first, middle, last, comp);
   }
   else {
      BidirectionalIterator first_cut = first;
      BidirectionalIterator second_cut = middle;
      size_type len11 = 0;
      size_type len22 = 0;
      if (len1 > len2)  //(len1 < len2)
      {
         len11 = len1 / 2;
         first_cut += len11;
         second_cut = boost::movelib::lower_bound(middle, last, *first_cut, comp);
         len22 = size_type(second_cut - middle);
      }
      else
      {
         len22 = len2 / 2;
         second_cut += len22;
         first_cut = boost::movelib::upper_bound(first, middle, *second_cut, comp);
         len11 = size_type(first_cut - first);
      }

      BidirectionalIterator new_middle
         = rotate_adaptive(first_cut, middle, second_cut,
            size_type(len1 - len11), len22, buffer,
            buffer_size);
      merge_adaptive_ONlogN_recursive(first, first_cut, new_middle, len11,
         len22, buffer, buffer_size, comp);
      merge_adaptive_ONlogN_recursive(new_middle, second_cut, last,
         size_type(len1 - len11), size_type(len2 - len22), buffer, buffer_size, comp);
   }
}


template<typename BidirectionalIterator, typename Compare, typename RandRawIt>
void merge_adaptive_ONlogN(BidirectionalIterator first,
		                     BidirectionalIterator middle,
		                     BidirectionalIterator last,
		                     Compare comp,
                           RandRawIt uninitialized,
                           typename  iter_size<BidirectionalIterator>::type uninitialized_len)
{
   typedef typename iterator_traits<BidirectionalIterator>::value_type  value_type;
   typedef typename  iter_size<BidirectionalIterator>::type   size_type;

   if (first == middle || middle == last)
      return;

   if(uninitialized_len)
   {
      const size_type len1 = size_type(middle - first);
      const size_type len2 = size_type(last - middle);

      ::boost::movelib::adaptive_xbuf<value_type, RandRawIt> xbuf(uninitialized, uninitialized_len);
      xbuf.initialize_until(uninitialized_len, *first);
	   merge_adaptive_ONlogN_recursive(first, middle, last, len1, len2, xbuf.begin(), uninitialized_len, comp);
   }
   else
   {
      merge_bufferless_ONlogN(first, middle, last, comp);
   }
}

///////////////////////////////////////////////////////////////////////////////
//
//                 MERGE ADAPTIVE ONsqrtN (GROUP ROTATIONS)
//
///////////////////////////////////////////////////////////////////////////////

// Merges (stable) the two adjacent sorted ranges
//
//    range 1 = [first, middle)   and   range 2 = [middle, last)
//
// in place, into [first, last). It is correct for any pair of lengths, but it
// is meant for a range 1 much shorter than range 2, see the complexity below.
//
// The elements of range 1 are taken in groups of ceil_sqrt(r1) elements
// A step starts with:
//    - The elements of range 1 that are still unmerged at [r1_cur, r1_end)
//    - The elements of range 2 that are still unmerged at [r1_end, last)
//    - And everything before r1_cur already in its final place.
// 
// The step then:
//   - takes the first ceil_sqrt(r1) elements of range 1 as the group, which is
//     [r1_cur, group_end),
//   - locates with a binary search the elements of range 2 that must go before
//     the last element of the group, which are [r1_end, r2_cut),
//   - rotates the rest of range 1, [group_end, r1_end), past them, which leaves
//     the group next to the part of range 2 it interleaves with, and the rest
//     of range 1 at [r1_rest, r2_cut),
//   - merges the group with that part of range 2, so [r1_cur, r1_rest) is done
//     and r1_rest is where the next step starts.
//
//       r1_cur   group_end      r1_end        r2_cut
//       [ group ][ rest of r1 ][ part of r2 ][ rest of r2 ]  last
//    -> [ group ][ part of r2 ][ rest of r1 ][ rest of r2 ]
//       |-----merged here-----|^r1_rest, the r1_cur of the next step
//
// So each element of range 2 is moved once by the rotation and once
// more by the merge step.
//
// If an external buffer of constructed elements [buffer, buffer + buffer_size)
// is available it is used to speed up the rotations, and to merge a group with
// its part of range 2 once the group fits in it. The buffer is optional: a size
// of zero is valid and makes this an O(1) additional memory merge.
//
//Precondition: both ranges are non-empty, so that the binary searches below
//  always have somewhere to look.
//
//Complexity: with r1 = middle - first, r2 = last - middle and g = ceil_sqrt(r1),
//  - moves:       ~2*r2 + r1*sqrt(r1)
//  - comparisons: ~g*log2(r2) for the binary searches, plus the group merges:
//      - buffer_size >= g: each group is merged with buffered_merge, a linear
//        merge, so the total is ~r1 + r2. A larger buffer does not reduce it.
//      - buffer_size < g (or no buffer): the groups are merged with
//        merge_bufferless_ON2, so the total is O(r1*log(r2)). Only a last
//        group shorter than g can use the buffer, and its merge is linear.
//
// Without a buffer the comparison count is the same order as any bufferless
// merge, so the moves are what makes this algorithm worth choosing:
// merge_bufferless_ONlogN moves ~r2*log2(r1)/2 elements, which is more as soon
// as r1*sqrt(r1) is small compared to r2. With a buffer of g or more elements
// the comparisons are ~r2, many more than r1*log2(r2) when r1 is much shorter
// than r2.
//
// Measured with r1 = 2*ceil_sqrt(r2) and r2 from 1e4 to 1e6, the moves per
// element stay at 2.07 to 2.15 while merge_bufferless_ONlogN grows from 4.04 to
// 5.68, which makes this merge 2.0 to 2.7 times faster. With r1 = ceil_sqrt(r2)
// the shorter range is already short enough for merge_bufferless_ON2, which
// moves 1.50 per element there and is the one adaptive_merge picks.
template<class RandIt, class Compare, class RandItBuf>
void merge_adaptive_ONsqrtN
   ( RandIt const first, RandIt const middle, RandIt const last, Compare comp
   , RandItBuf const buffer, typename iter_size<RandIt>::type const buffer_size)
{
   typedef typename iter_size<RandIt>::type size_type;

   assert(first != middle && middle != last);

   size_type r1_left = size_type(middle - first);
   size_type const l_group = ceil_sqrt(r1_left);

   RandIt r1_cur = first;    //range 1 elements left to merge: [r1_cur, r1_cur + r1_left)
   while(r1_left){
      size_type const l_cur  = min_value<size_type>(l_group, r1_left);
      size_type const l_rest = size_type(r1_left - l_cur);
      RandIt const group_end = r1_cur + l_cur;
      RandIt const r1_end    = r1_cur + r1_left;
      RandIt group_last = group_end;
      --group_last;
      //Range 2 elements that must be placed before the last element of the group
      RandIt const r2_cut = boost::movelib::lower_bound(r1_end, last, *group_last, comp);
      //Move the rest of range 1 after them: [group][part of r2][rest of r1]
      RandIt const r1_rest = rotate_adaptive
         (group_end, r1_end, r2_cut, l_rest, size_type(r2_cut - r1_end), buffer, buffer_size);
      //Merge the group with the part of range 2 it interleaves with
      if(l_cur <= buffer_size){
         range_xbuf<RandItBuf, size_type, move_op> rxbuf(buffer, buffer + buffer_size);
         buffered_merge(r1_cur, group_end, r1_rest, comp, rxbuf);
      }
      else{
         //The group does not fit in the buffer. The two ranges merged here are
         //unbalanced by construction, about sqrt(r1) elements against a part of
         //range 2, so merge_bufferless_ON2 pays the squared term on the group
         //alone. A recursive rotation merge would move more elements
         merge_bufferless_ON2(r1_cur, group_end, r1_rest, comp);
      }
      r1_cur  = r1_rest;
      r1_left = l_rest;
   }
}

}  //namespace movelib {
}  //namespace boost {

#if defined(BOOST_CLANG) || (defined(BOOST_GCC) && (BOOST_GCC >= 40600))
#pragma GCC diagnostic pop
#endif

#include <boost/move/detail/config_end.hpp>

#endif   //#define BOOST_MOVE_MERGE_HPP
