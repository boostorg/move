//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2012-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////

//! \file

#ifndef BOOST_MOVE_ITERATOR_HPP
#define BOOST_MOVE_ITERATOR_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/move/detail/config_begin.hpp>
#include <boost/move/detail/workaround.hpp>  //forceinline
#include <boost/move/detail/iterator_traits.hpp>
#include <boost/move/utility_core.hpp>

namespace boost {

//////////////////////////////////////////////////////////////////////////////
//
//                            move_iterator
//
//////////////////////////////////////////////////////////////////////////////

//! Class template move_iterator is an iterator adaptor with the same behavior
//! as the underlying iterator except that its dereference operator implicitly
//! converts the value returned by the underlying iterator's dereference operator
//! to an rvalue reference. Some generic algorithms can be called with move
//! iterators to replace copying with moving.
template <class It>
class move_iterator
{
   public:
   typedef It                                                              iterator_type;
   typedef typename boost::movelib::iterator_traits<iterator_type>::value_type        value_type;
   private:
   typedef typename boost::movelib::iterator_traits<iterator_type>::reference         base_reference;
   typedef typename ::boost::move_detail::remove_reference<base_reference>::type      base_referenced;
   public:
   #if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) || defined(BOOST_MOVE_DOXYGEN_INVOKED)
   //Reference: rvalue reference to the same (possibly const) type.
   //Proxy returned by value: the proxy itself (LWG 2106)
   typedef typename ::boost::move_detail::if_c
      < !::boost::move_detail::is_same<base_reference, base_referenced>::value
      , base_referenced &&
      , base_reference >::type                                             reference;
   #else
   //Non-const lvalue reference to a type with move emulation: rv<T>&
   //Otherwise (const lvalue, proxy or no move emulation): base reference, copies
   typedef typename ::boost::move_detail::if_c
      < ::boost::move_detail::is_lvalue_reference<base_reference>::value &&
        ::boost::move_detail::is_same<base_referenced, value_type>::value &&
        ::boost::has_move_emulation_enabled<value_type>::value
      , ::boost::rv<value_type>&
      , base_reference >::type                                             reference;
   #endif
   typedef It                                                              pointer;
   typedef typename boost::movelib::iterator_traits<iterator_type>::difference_type   difference_type;
   typedef typename boost::movelib::iterator_traits<iterator_type>::iterator_category iterator_category;

   private:
   //The dereferenced value is cast through a function parameter because MSVC
   //creates a temporary for static_cast<T&&>(p[n]) and returns a dangling reference.
   #if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
   template<class R>
   BOOST_MOVE_FORCEINLINE static reference to_reference(R&& r)
   {  return static_cast<reference>(r);  }
   #else
   template<class R>
   BOOST_MOVE_FORCEINLINE static reference to_reference(R& r)
   {  return static_cast<reference>(r);  }

   template<class R>
   BOOST_MOVE_FORCEINLINE static reference to_reference(const R& r)
   {  return static_cast<reference>(r);  }
   #endif
   public:

   inline move_iterator()
      : m_it()
   {}

   inline explicit move_iterator(const It &i)
      :  m_it(i)
   {}

   template <class U>
   inline move_iterator(const move_iterator<U>& u
      BOOST_MOVE_DOCIGN(BOOST_MOVE_I typename ::boost::move_detail::enable_if_convertible<U BOOST_MOVE_I It>::type* = 0))
      :  m_it(u.base())
   {}

   inline const iterator_type& base() const
   {  return m_it;   }

   inline reference operator*() const
   {  return move_iterator::to_reference(*m_it);  }

   inline pointer   operator->() const
   {  return m_it;   }

   inline move_iterator& operator++()
   {  ++m_it; return *this;   }

   inline move_iterator<iterator_type>  operator++(int)
   {  move_iterator<iterator_type> tmp(*this); ++(*this); return tmp;   }

   inline move_iterator& operator--()
   {  --m_it; return *this;   }

   inline move_iterator<iterator_type>  operator--(int)
   {  move_iterator<iterator_type> tmp(*this); --(*this); return tmp;   }

   move_iterator<iterator_type>  operator+ (difference_type n) const
   {  return move_iterator<iterator_type>(m_it + n);  }

   inline move_iterator& operator+=(difference_type n)
   {  m_it += n; return *this;   }

   inline move_iterator<iterator_type>  operator- (difference_type n) const
   {  return move_iterator<iterator_type>(m_it - n);  }

   inline move_iterator& operator-=(difference_type n)
   {  m_it -= n; return *this;   }

   inline reference operator[](difference_type n) const
   {  return move_iterator::to_reference(m_it[n]);  }

   inline friend bool operator==(const move_iterator& x, const move_iterator& y)
   {  return x.m_it == y.m_it;  }

   inline friend bool operator!=(const move_iterator& x, const move_iterator& y)
   {  return x.m_it != y.m_it;  }

   inline friend bool operator< (const move_iterator& x, const move_iterator& y)
   {  return x.m_it < y.m_it;   }

   inline friend bool operator<=(const move_iterator& x, const move_iterator& y)
   {  return x.m_it <= y.m_it;  }

   inline friend bool operator> (const move_iterator& x, const move_iterator& y)
   {  return x.m_it > y.m_it;  }

   inline friend bool operator>=(const move_iterator& x, const move_iterator& y)
   {  return x.m_it >= y.m_it;  }

   inline friend difference_type operator-(const move_iterator& x, const move_iterator& y)
   {  return x.m_it - y.m_it;   }

   inline friend move_iterator operator+(difference_type n, const move_iterator& x)
   {  return move_iterator(x.m_it + n);   }

   private:
   It m_it;
};

//Comparison and difference between move iterators of different base iterator types
//(e.g. move_iterator<T*> and move_iterator<const T*>). Move iterators of the same
//type use the friend functions.
template <class It1, class It2>
inline bool operator==(const move_iterator<It1>& x, const move_iterator<It2>& y)
{  return x.base() == y.base();  }

template <class It1, class It2>
inline bool operator!=(const move_iterator<It1>& x, const move_iterator<It2>& y)
{  return x.base() != y.base();  }

template <class It1, class It2>
inline bool operator< (const move_iterator<It1>& x, const move_iterator<It2>& y)
{  return x.base() < y.base();   }

template <class It1, class It2>
inline bool operator<=(const move_iterator<It1>& x, const move_iterator<It2>& y)
{  return x.base() <= y.base();  }

template <class It1, class It2>
inline bool operator> (const move_iterator<It1>& x, const move_iterator<It2>& y)
{  return x.base() > y.base();   }

template <class It1, class It2>
inline bool operator>=(const move_iterator<It1>& x, const move_iterator<It2>& y)
{  return x.base() >= y.base();  }

template <class It1, class It2>
inline typename move_iterator<It1>::difference_type
   operator-(const move_iterator<It1>& x, const move_iterator<It2>& y)
{  return x.base() - y.base();   }

//is_move_iterator
namespace move_detail {

template <class I>
struct is_move_iterator
{
   static const bool value = false;
};

template <class I>
struct is_move_iterator< ::boost::move_iterator<I> >
{
   static const bool value = true;
};

}  //namespace move_detail {

//////////////////////////////////////////////////////////////////////////////
//
//                            move_iterator
//
//////////////////////////////////////////////////////////////////////////////

//!
//! <b>Returns</b>: move_iterator<It>(i).
template<class It>
inline move_iterator<It> make_move_iterator(const It &it)
{  return move_iterator<It>(it); }

//////////////////////////////////////////////////////////////////////////////
//
//                         back_move_insert_iterator
//
//////////////////////////////////////////////////////////////////////////////


//! A move insert iterator that move constructs elements at the
//! back of a container
template <typename C> // C models Container
class back_move_insert_iterator
{
   C* container_m;

   public:
   typedef C                           container_type;
   typedef typename C::value_type      value_type;
   typedef typename C::reference       reference;
   typedef typename C::pointer         pointer;
   typedef typename C::difference_type difference_type;
   typedef std::output_iterator_tag    iterator_category;

   explicit back_move_insert_iterator(C& x) : container_m(&x) { }

   back_move_insert_iterator& operator=(reference x)
   { container_m->push_back(boost::move(x)); return *this; }

   back_move_insert_iterator& operator=(BOOST_RV_REF(value_type) x)
   {  reference rx = x; return this->operator=(rx);  }

   back_move_insert_iterator& operator*()     { return *this; }
   back_move_insert_iterator& operator++()    { return *this; }
   back_move_insert_iterator& operator++(int) { return *this; }
};

//!
//! <b>Returns</b>: back_move_insert_iterator<C>(x).
template <typename C> // C models Container
inline back_move_insert_iterator<C> back_move_inserter(C& x)
{
   return back_move_insert_iterator<C>(x);
}

//////////////////////////////////////////////////////////////////////////////
//
//                         front_move_insert_iterator
//
//////////////////////////////////////////////////////////////////////////////

//! A move insert iterator that move constructs elements int the
//! front of a container
template <typename C> // C models Container
class front_move_insert_iterator
{
   C* container_m;

public:
   typedef C                           container_type;
   typedef typename C::value_type      value_type;
   typedef typename C::reference       reference;
   typedef typename C::pointer         pointer;
   typedef typename C::difference_type difference_type;
   typedef std::output_iterator_tag    iterator_category;

   explicit front_move_insert_iterator(C& x) : container_m(&x) { }

   front_move_insert_iterator& operator=(reference x)
   { container_m->push_front(boost::move(x)); return *this; }

   front_move_insert_iterator& operator=(BOOST_RV_REF(value_type) x)
   {  reference rx = x; return this->operator=(rx);  }

   front_move_insert_iterator& operator*()     { return *this; }
   front_move_insert_iterator& operator++()    { return *this; }
   front_move_insert_iterator& operator++(int) { return *this; }
};

//!
//! <b>Returns</b>: front_move_insert_iterator<C>(x).
template <typename C> // C models Container
inline front_move_insert_iterator<C> front_move_inserter(C& x)
{
   return front_move_insert_iterator<C>(x);
}

//////////////////////////////////////////////////////////////////////////////
//
//                         insert_move_iterator
//
//////////////////////////////////////////////////////////////////////////////
template <typename C> // C models Container
class move_insert_iterator
{
   C* container_m;
   typename C::iterator pos_;

   public:
   typedef C                           container_type;
   typedef typename C::value_type      value_type;
   typedef typename C::reference       reference;
   typedef typename C::pointer         pointer;
   typedef typename C::difference_type difference_type;
   typedef std::output_iterator_tag    iterator_category;

   explicit move_insert_iterator(C& x, typename C::iterator pos)
      : container_m(&x), pos_(pos)
   {}

   move_insert_iterator& operator=(reference x)
   {
      pos_ = container_m->insert(pos_, ::boost::move(x));
      ++pos_;
      return *this;
   }

   move_insert_iterator& operator=(BOOST_RV_REF(value_type) x)
   {  reference rx = x; return this->operator=(rx);  }

   move_insert_iterator& operator*()     { return *this; }
   move_insert_iterator& operator++()    { return *this; }
   move_insert_iterator& operator++(int) { return *this; }
};

//!
//! <b>Returns</b>: move_insert_iterator<C>(x, it).
template <typename C> // C models Container
inline move_insert_iterator<C> move_inserter(C& x, typename C::iterator it)
{
   return move_insert_iterator<C>(x, it);
}

}  //namespace boost {

#include <boost/move/detail/config_end.hpp>

#endif //#ifndef BOOST_MOVE_ITERATOR_HPP
