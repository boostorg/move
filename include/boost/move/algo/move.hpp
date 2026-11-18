//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2012-2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////

//! \file

#ifndef BOOST_MOVE_ALGO_MOVE_HPP
#define BOOST_MOVE_ALGO_MOVE_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/move/detail/config_begin.hpp>

#include <boost/move/utility_core.hpp>
#include <boost/move/detail/iterator_traits.hpp>
#include <boost/move/detail/iterator_to_raw_pointer.hpp>
#include <boost/move/detail/addressof.hpp>
#include <boost/move/detail/placement_new.hpp>
#if defined(BOOST_MOVE_USE_STANDARD_LIBRARY_MOVE)
#include <algorithm>
#endif

namespace boost {

//////////////////////////////////////////////////////////////////////////////
//
//                               move
//
//////////////////////////////////////////////////////////////////////////////

#if !defined(BOOST_MOVE_USE_STANDARD_LIBRARY_MOVE)

   //! <b>Effects</b>: Moves elements in the range [f,l) into the range [result,result + (l -
   //!   f)) starting from f and proceeding to l. For each non-negative integer n < (l-f),
   //!   performs *(result + n) = ::boost::move (*(f + n)).
   //!
   //! <b>Returns</b>: result + (l - f).
   //!
   //! <b>Requires</b>: result shall not be in the range [f,l).
   //!
   //! <b>Complexity</b>: Exactly l - f move assignments.
   template <typename I, // I models InputIterator
            typename O> // O models OutputIterator
   O move(I f, I l, O result)
   {
      while (f != l) {
         *result = ::boost::move(*f);
         ++f; ++result;
      }
      return result;
   }

   //////////////////////////////////////////////////////////////////////////////
   //
   //                               move_backward
   //
   //////////////////////////////////////////////////////////////////////////////

   //! <b>Effects</b>: Moves elements in the range [f,l) into the range
   //!   [result - (l-f),result) starting from l - 1 and proceeding to
   //!   f. For each positive integer n <= (l - f),
   //!   performs *(result - n) = ::boost::move(*(l - n)).
   //!
   //! <b>Requires</b>: result shall not be in the range (f,l].
   //!
   //! <b>Returns</b>: result - (l - f).
   //!
   //! <b>Complexity</b>: Exactly l - f move assignments.
   template <typename I, // I models BidirectionalIterator
   typename O> // O models BidirectionalIterator
   O move_backward(I f, I l, O result)
   {
      while (f != l) {
         --l; --result;
         *result = ::boost::move(*l);
      }
      return result;
   }

#else

   using ::std::move_backward;

#endif   // !defined(BOOST_MOVE_USE_STANDARD_LIBRARY_MOVE)

//////////////////////////////////////////////////////////////////////////////
//
//                               uninitialized_move
//
//////////////////////////////////////////////////////////////////////////////

//! <b>Effects</b>:
//!   \code
//!   for (; f != l; ++r, ++f)
//!      new (static_cast<void*>(&*r))
//!         typename iterator_traits<F>::value_type(boost::move(*f));
//!   \endcode
//!
//! <b>Returns</b>: r
template
   <typename I, // I models InputIterator
    typename F> // F models ForwardIterator
F uninitialized_move(I f, I l, F r
   /// @cond
//   ,typename ::boost::move_detail::enable_if<has_move_emulation_enabled<typename boost::movelib::iterator_traits<I>::value_type> >::type* = 0
   /// @endcond
   )
{
   typedef typename boost::movelib::iterator_traits<F>::value_type value_type;

   F back = r;
   BOOST_MOVE_TRY{
      while (f != l) {
         void * const addr = static_cast<void*>(::boost::move_detail::addressof(*r));
         ::new(addr, boost_move_new_t()) value_type(::boost::move(*f));
         ++f; ++r;
      }
   }
   BOOST_MOVE_CATCH(...){
      for (; back != r; ++back){
         ::boost::move_detail::addressof(*back)->~value_type();
      }
      BOOST_MOVE_RETHROW;
   }
   BOOST_MOVE_CATCH_END
   return r;
}

/// @cond
/*
template
   <typename I,   // I models InputIterator
    typename F>   // F models ForwardIterator
F uninitialized_move(I f, I l, F r,
   typename ::boost::move_detail::disable_if<has_move_emulation_enabled<typename boost::movelib::iterator_traits<I>::value_type> >::type* = BOOST_NULLPTR)
{
   return std::uninitialized_copy(f, l, r);
}
*/

/// @endcond

}  //namespace boost {

#include <boost/move/detail/config_end.hpp>

#endif //#ifndef BOOST_MOVE_ALGO_MOVE_HPP
