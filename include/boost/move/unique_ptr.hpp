//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2014-2014. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_MOVE_UNIQUE_PTR_HPP_INCLUDED
#define BOOST_MOVE_UNIQUE_PTR_HPP_INCLUDED

#include <boost/move/detail/config_begin.hpp>
#include <boost/move/detail/workaround.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/move/detail/meta_utils.hpp>
#include <boost/static_assert.hpp>
#include <boost/assert.hpp>

#if !defined(BOOST_NO_CXX11_NULLPTR)
#include <cstddef>   //For std::nullptr_t
#endif

//!\file
//! Describes the smart pointer unique_ptr, a drop-in replacement for std::unique_ptr,
//! usable also from C++03 compilers.
//!
//! Main differences from std::unique_ptr to avoid heavy dependencies,
//! specially in C++03 compilers:
//!   - <tt>operator < </tt> uses pointer <tt>operator < </tt>instead of <tt>std::less<common_type></tt>. 
//!      This avoids dependencies on <tt>std::common_type</tt> and <tt>std::less</tt>
//!      (<tt><type_traits>/<functional></tt> headers. In C++03 this avoid pulling Boost.Typeof and other
//!      cascading dependencies. As in all Boost platforms <tt>operator <</tt> on raw pointers and
//!      other smart pointers provides strict weak ordering in practice this should not be a problem for users.
//!   - assignable from literal 0 for compilers without nullptr
//!   - <tt>unique_ptr<T[]></tt> is constructible and assignable from <tt>unique_ptr<U[]></tt> if
//!      cv-less T and cv-less U are the same type and T is more CV qualified than U.

namespace boost{

namespace move_detail {

template <class D>
struct deleter_types
{
   typedef typename if_c
      < is_lvalue_reference<D>::value
      , D
      , typename add_lvalue_reference<typename add_const<D>::type>::type
      >::type deleter_arg_type1;

   #ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
   typedef typename remove_reference<D>::type && deleter_arg_type2;
   #else
   typedef ::boost::rv<D> &deleter_arg_type2;
   #endif
   typedef typename add_lvalue_reference<D>::type  deleter_lvalue_reference;
   typedef typename add_lvalue_reference
      <typename add_const<D>::type>::type          deleter_const_lvalue_reference;
};

template <class P, class D, bool = is_unary_or_binary_function<D>::value || is_reference<D>::value >
struct unique_ptr_data
{
   typedef typename deleter_types<D>::deleter_arg_type1 deleter_arg_type1;
   typedef typename deleter_types<D>::deleter_arg_type2 deleter_arg_type2;
   typedef typename deleter_types<D>::deleter_lvalue_reference       deleter_lvalue_reference;
   typedef typename deleter_types<D>::deleter_const_lvalue_reference deleter_const_lvalue_reference;

   unique_ptr_data() BOOST_NOEXCEPT
      : m_p(), d()
   {}

   explicit unique_ptr_data(P p) BOOST_NOEXCEPT
      : m_p(p), d()
   {}

   unique_ptr_data(P p, deleter_arg_type1 d1) BOOST_NOEXCEPT
      : m_p(p), d(d1)
   {}

   unique_ptr_data(P p, deleter_arg_type2 d2) BOOST_NOEXCEPT
      : m_p(p), d(::boost::move(d2))
   {}

   template <class U>
   unique_ptr_data(P p, BOOST_FWD_REF(U) d) BOOST_NOEXCEPT
      : m_p(::boost::forward<U>(p)), d(::boost::forward<U>(d))
   {}

   deleter_lvalue_reference deleter()
   { return d; }

   deleter_const_lvalue_reference deleter() const
   { return d; }

   P m_p;

   private:
   unique_ptr_data(const unique_ptr_data&);
   unique_ptr_data &operator=(const unique_ptr_data&);
   D d;
};

template <class P, class D>
struct unique_ptr_data<P, D, false>
   : private D
{
   typedef typename deleter_types<D>::deleter_arg_type1 deleter_arg_type1;
   typedef typename deleter_types<D>::deleter_arg_type2 deleter_arg_type2;
   typedef typename deleter_types<D>::deleter_lvalue_reference       deleter_lvalue_reference;
   typedef typename deleter_types<D>::deleter_const_lvalue_reference deleter_const_lvalue_reference;

   unique_ptr_data() BOOST_NOEXCEPT
      : D(), m_p()
   {}

   unique_ptr_data(P p) BOOST_NOEXCEPT
      : D(), m_p(p)
   {}

   unique_ptr_data(P p, deleter_arg_type1 d1) BOOST_NOEXCEPT
      : D(d1), m_p(p)
   {}

   unique_ptr_data(P p, deleter_arg_type2 d2) BOOST_NOEXCEPT
      : D(::boost::move(d2)), m_p(p)
   {}

   template <class U>
   unique_ptr_data(P p, BOOST_FWD_REF(U) d) BOOST_NOEXCEPT
      : D(::boost::forward<U>(d)), m_p(p)
   {}

   deleter_lvalue_reference deleter()
   { return static_cast<deleter_lvalue_reference>(*this); }

   deleter_const_lvalue_reference deleter() const
   {  return static_cast<deleter_const_lvalue_reference>(*this);  }

   P m_p;
   private:
   unique_ptr_data(const unique_ptr_data&);
   unique_ptr_data &operator=(const unique_ptr_data&);
};

//Although non-standard, we avoid using pointer_traits
//to avoid heavy dependencies
template <typename T>
struct get_element_type
{
   template <typename X>
   static char test(int, typename X::element_type*);

   template <typename X>
   static int test(...);

   struct DefaultWrap { typedef natify<T> element_type; };

   static const bool value = (1 == sizeof(test<T>(0, 0)));

   typedef typename
      if_c<value, T, DefaultWrap>::type::element_type type;
};

template<class T>
struct get_element_type<T*>
{
   typedef T type;
};

template<class T>
struct get_cvelement
{
   typedef typename remove_cv
      <typename get_element_type<T>::type
      >::type type;
};

template <class P1, class P2>
struct is_same_cvelement_and_convertible
{
   typedef typename remove_reference<P1>::type arg1;
   typedef typename remove_reference<P2>::type arg2;
   static const bool same_cvless = is_same<typename get_cvelement<arg1>::type
                                          ,typename get_cvelement<arg2>::type
                                          >::value;
   static const bool value = same_cvless && is_convertible<arg1, arg2>::value;
};

template <class P1, class P2, class Type = nat>
struct enable_same_cvelement_and_convertible
   : public enable_if_c
      <is_same_cvelement_and_convertible<P1, P2>::value, Type>
{};


template<bool OtherIsArray, bool IsArray, class FromPointer, class ThisPointer>
struct is_unique_acceptable
{
   static const bool value = OtherIsArray &&
           is_same_cvelement_and_convertible
               <FromPointer, ThisPointer>::value;
};

template<bool OtherIsArray, class FromPointer, class ThisPointer>
struct is_unique_acceptable<OtherIsArray, false, FromPointer, ThisPointer>
{
   static const bool value = !OtherIsArray &&
            is_convertible<FromPointer, ThisPointer>::value;
};


template<class U, class T, class Type = nat>
struct enable_default_delete
   : enable_if_c
      < is_unique_acceptable < is_array<U>::value
                             , is_array<T>::value
                             , typename element_pointer<U>::type
                             , typename element_pointer<T>::type
                             >::value
      , Type>
{};

template <class T, class U>
class is_rvalue_convertible
{
   #ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
   typedef typename remove_reference<T>::type&& t_from;
   #else
   typedef typename if_c
      < has_move_emulation_enabled<T>::value && !is_reference<T>::value
      , boost::rv<T>&
      , typename add_lvalue_reference<T>::type
      >::type t_from;
   #endif

   typedef char true_t;
   class false_t { char dummy[2]; };
   static false_t dispatch(...);
   static true_t  dispatch(U);
   static t_from trigger();
   public:
   static const bool value = sizeof(dispatch(trigger())) == sizeof(true_t);
};

template<class D, class E, bool IsReference = is_reference<D>::value>
struct unique_deleter_is_initializable
{
   static const bool value = is_same<D, E>::value;
};

template<class D, class E>
struct unique_deleter_is_initializable<D, E, false>
{
   #if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
   //Clang has some problems with is_rvalue_convertible with non-copyable types
   //so use intrinsic if available
   #if defined(BOOST_CLANG)
      #if __has_feature(is_convertible_to)
      static const bool value = __is_convertible_to(E, D);
      #else
      static const bool value = is_rvalue_convertible<E, D>::value;
      #endif
   #else
   static const bool value = is_rvalue_convertible<E, D>::value;
   #endif

   #else //!defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
   //No hope for compilers with move emulation for now. In several compilers is_convertible
   // leads to errors, so just move the Deleter and see if the conversion works
   static const bool value = true;  /*is_rvalue_convertible<E, D>::value*/
   #endif
};

template <class U, class E, class T, class D>
struct is_ptr_convertible
{
   typedef typename ::boost::move_detail::pointer_type<U, E>::type from_ptr;
   typedef typename ::boost::move_detail::pointer_type<T, D>::type to_ptr;
   static const bool value = is_convertible<from_ptr, to_ptr>::value;
};

template<class T, class D, class U, class E>
struct unique_moveconvert_assignable
{
   static const bool ptrconvert  = is_ptr_convertible<U, E, T, D>::value;
   static const bool notarray    = !is_array<U>::value;
   static const bool value = ptrconvert && notarray;
};

template<class T, class D, class U, class E>
struct unique_moveconvert_constructible
{
   static const bool massignable = unique_moveconvert_assignable<T, D, U, E>::value;
   static const bool value = massignable && unique_deleter_is_initializable<D, E>::value;
};

template<class T, class D, class U, class E, class Type = nat>
struct enable_unique_moveconvert_constructible
   : enable_if_c
      < unique_moveconvert_constructible<T, D, U, E>::value, Type>
{};

template<class T, class D, class U, class E, class Type = nat>
struct enable_unique_moveconvert_assignable
   : enable_if_c
      < unique_moveconvert_assignable<T, D, U, E>::value, Type>
{};

template<class T, class D, class U, class E>
struct uniquearray_moveconvert_assignable
{
   typedef typename ::boost::move_detail::pointer_type<U, E>::type from_ptr;
   typedef typename ::boost::move_detail::pointer_type<T, D>::type to_ptr;
   static const bool ptrconvert  = is_same_cvelement_and_convertible<from_ptr, to_ptr>::value;
   static const bool yesarray    = is_array<U>::value;
   static const bool value = ptrconvert && yesarray;
};

template<class T, class D, class U, class E>
struct uniquearray_moveconvert_constructible
{
   static const bool massignable = uniquearray_moveconvert_assignable<T, D, U, E>::value;
   static const bool value = massignable && unique_deleter_is_initializable<D, E>::value;
};

template<class T, class D, class U, class E, class Type = nat>
struct enable_uniquearray_moveconvert_constructible
   : enable_if_c
      < uniquearray_moveconvert_constructible<T, D, U, E>::value, Type>
{};

template<class T, class D, class U, class E, class Type = nat>
struct enable_uniquearray_moveconvert_assignable
   : enable_if_c
      < uniquearray_moveconvert_assignable<T, D, U, E>::value, Type>
{};

template<class Deleter, class Pointer>
void unique_ptr_call_deleter(Deleter &d, const Pointer &p, boost::move_detail::true_type)  //default_deleter
{  d(p);  }

template<class Deleter, class Pointer>
void unique_ptr_call_deleter(Deleter &d, const Pointer &p, boost::move_detail::false_type)
{  if(p) d(p);  }

}  //namespace move_detail {

namespace movelib {

//!The class template <tt>default_delete</tt> serves as the default deleter
//!(destruction policy) for the class template <tt>unique_ptr</tt>.
//!
//!A specialization to delete array types is provided
//!with a slightly altered interface.
template <class T>
struct default_delete
{
   //! Default constructor.
   //!
   BOOST_CONSTEXPR default_delete()
   //Avoid "defaulted on its first declaration must not have an exception-specification" error for GCC 4.6
   #if !defined(BOOST_GCC) || (BOOST_GCC < 40600 && BOOST_GCC >= 40700) || defined(BOOST_MOVE_DOXYGEN_INVOKED)
   BOOST_NOEXCEPT
   #endif
   #if !defined(BOOST_NO_CXX11_DEFAULTED_FUNCTIONS) || defined(BOOST_MOVE_DOXYGEN_INVOKED)
   = default;
   #else
   {};
   #endif

   #if defined(BOOST_MOVE_DOXYGEN_INVOKED)
   default_delete(const default_delete&) BOOST_NOEXCEPT = default;
   default_delete &operator=(const default_delete&) BOOST_NOEXCEPT = default;
   #else
   typedef T element_type;
   #endif

   //! <b>Effects</b>: Constructs a default_delete object from another <tt>default_delete<U></tt> object.
   //!
   //! <b>Remarks</b>: This constructor shall not participate in overload resolution unless U* is
   //!   implicitly convertible to T*.
   template <class U>
   default_delete(const default_delete<U>&
      #if !defined(BOOST_MOVE_DOXYGEN_INVOKED)
      ,  typename ::boost::move_detail::enable_default_delete<U, T>::type* = 0
      #endif
      ) BOOST_NOEXCEPT
   {}

   //! <b>Effects</b>: Constructs a default_delete object from another <tt>default_delete<U></tt> object.
   //!
   //! <b>Remarks</b>: This constructor shall not participate in overload resolution unless U* is
   //!   implicitly convertible to T*.
   //!
   //! <b>Note</b>: Non-standard extension
   template <class U>
   #if defined(BOOST_MOVE_DOXYGEN_INVOKED)
   default_delete &
   #else
   typename ::boost::move_detail::enable_default_delete<U, T, default_delete &>::type
   #endif
      operator=(const default_delete<U>&) BOOST_NOEXCEPT
   {  return *this;  }

   //! <b>Effects</b>: calls <tt>delete</tt> on ptr.
   //!
   //! <b>Remarks</b>: If T is an incomplete type, the program is ill-formed.
   void operator()(T* ptr) const
   {
      //T must be a complete type
      BOOST_STATIC_ASSERT(sizeof(T) > 0);
      delete ptr;
   }
};

//!The specialization to delete array types (<tt>array_of_T = T[]</tt>)
//!with a slightly altered interface.
//!
//! \tparam array_of_T is an alias for types of form <tt>T[]</tt>
template <class T>
struct default_delete
#if defined(BOOST_MOVE_DOXYGEN_INVOKED)
<array_of_T>
#else
<T[]>
#endif
{
   //! Default constructor.
   //!
   BOOST_CONSTEXPR default_delete()
   //Avoid "defaulted on its first declaration must not have an exception-specification" error for GCC 4.6
      #if !defined(BOOST_GCC) || (BOOST_GCC < 40600 && BOOST_GCC >= 40700) || defined(BOOST_MOVE_DOXYGEN_INVOKED)
   BOOST_NOEXCEPT
   #endif
   #if !defined(BOOST_NO_CXX11_DEFAULTED_FUNCTIONS) || defined(BOOST_MOVE_DOXYGEN_INVOKED)
   = default;
   #else
   {};
   #endif

   #if defined(BOOST_MOVE_DOXYGEN_INVOKED)
   default_delete(const default_delete&) BOOST_NOEXCEPT = default;
   default_delete &operator=(const default_delete&) BOOST_NOEXCEPT = default;
   #else
   typedef T element_type;
   #endif

   //! <b>Effects</b>: Constructs a default_delete object from another <tt>default_delete<U></tt> object.
   //!
   //! <b>Remarks</b>: This constructor shall not participate in overload resolution unless U* is
   //!   a more CV cualified pointer to T.
   //!
   //! <b>Note</b>: Non-standard extension
   template <class U>
   default_delete(const default_delete<U>&
      #if !defined(BOOST_MOVE_DOXYGEN_INVOKED)
      ,  typename ::boost::move_detail::enable_default_delete<U, T[]>::type* = 0
      #endif
      ) BOOST_NOEXCEPT
   {}

   //! <b>Effects</b>: Constructs a default_delete object from another <tt>default_delete<U></tt> object.
   //!
   //! <b>Remarks</b>: This constructor shall not participate in overload resolution unless U* is
   //!   a more CV cualified pointer to T.
   //!
   //! <b>Note</b>: Non-standard extension
   template <class U>
   #if defined(BOOST_MOVE_DOXYGEN_INVOKED)
   default_delete &
   #else
   typename ::boost::move_detail::enable_default_delete<U, T[], default_delete &>::type
   #endif
      operator=(const default_delete<U>&) BOOST_NOEXCEPT
   {  return *this;  }

   #if !defined(BOOST_NO_CXX11_DELETED_FUNCTIONS) || defined(BOOST_MOVE_DOXYGEN_INVOKED)
   //! <b>Effects</b>: calls <tt>delete[]</tt> on ptr.
   //!
   //! <b>Remarks</b>: If T is an incomplete type, the program is ill-formed.
   void operator()(T* ptr) const
   {
      //T must be a complete type
      BOOST_STATIC_ASSERT(sizeof(T) > 0);
      delete [] ptr;
   }

   //! <b>Remarks</b>: This deleter can't delete pointers convertible to T unless U* is
   //!   a more CV cualified pointer to T.
   //!
   //! <b>Note</b>: Non-standard extension
   template <class U>
   void operator()(U*) const = delete;
   #else
   template <class U>
   void operator()(U* ptr
                  #if !defined(BOOST_MOVE_DOXYGEN_INVOKED)
                  , typename ::boost::move_detail::enable_default_delete<U[], T[]>::type * = 0
                  #endif
                  ) const BOOST_NOEXCEPT
   {
      //U must be a complete type
      BOOST_STATIC_ASSERT(sizeof(U) > 0);
      delete [] ptr;
   }
   #endif
};


//! A unique pointer is an object that owns another object and
//! manages that other object through a pointer.
//! 
//! More precisely, a unique pointer is an object u that stores a pointer to a second object p and will dispose
//! of p when u is itself destroyed (e.g., when leaving block scope). In this context, u is said to own p.
//! 
//! The mechanism by which u disposes of p is known as p's associated deleter, a function object whose correct
//! invocation results in p's appropriate disposition (typically its deletion).
//! 
//! Let the notation u.p denote the pointer stored by u, and let u.d denote the associated deleter. Upon request,
//! u can reset (replace) u.p and u.d with another pointer and deleter, but must properly dispose of its owned
//! object via the associated deleter before such replacement is considered completed.
//! 
//! Additionally, u can, upon request, transfer ownership to another unique pointer u2. Upon completion of
//! such a transfer, the following postconditions hold:
//!   - u2.p is equal to the pre-transfer u.p,
//!   - u.p is equal to nullptr, and
//!   - if the pre-transfer u.d maintained state, such state has been transferred to u2.d.
//! 
//! As in the case of a reset, u2 must properly dispose of its pre-transfer owned object via the pre-transfer
//! associated deleter before the ownership transfer is considered complete.
//! 
//! Each object of a type U instantiated from the unique_ptr template specified in this subclause has the strict
//! ownership semantics, specified above, of a unique pointer. In partial satisfaction of these semantics, each
//! such U is MoveConstructible and MoveAssignable, but is not CopyConstructible nor CopyAssignable.
//! The template parameter T of unique_ptr may be an incomplete type.
//! 
//! The uses of unique_ptr include providing exception safety for dynamically allocated memory, passing
//! ownership of dynamically allocated memory to a function, and returning dynamically allocated memory from
//! a function.
//!
//! \tparam T Provides the type of the stored pointer.
//! \tparam D The deleter type:
//!   -  The default type for the template parameter D is default_delete. A client-supplied template argument
//!      D shall be a function object type, lvalue-reference to function, or lvalue-reference to function object type
//!      for which, given a value d of type D and a value ptr of type unique_ptr<T, D>::pointer, the expression
//!      d(ptr) is valid and has the effect of disposing of the pointer as appropriate for that deleter.
//!   -  If the deleter's type D is not a reference type, D shall satisfy the requirements of Destructible.
//!   -  If the type <tt>remove_reference<D>::type::pointer</tt> exists, it shall satisfy the requirements of NullablePointer.
template <class T, class D = default_delete<T> >
class unique_ptr
{
   #if defined(BOOST_MOVE_DOXYGEN_INVOKED)
   public:
   unique_ptr(const unique_ptr&) = delete;
   unique_ptr& operator=(const unique_ptr&) = delete;
   private:
   #else
   BOOST_MOVABLE_BUT_NOT_COPYABLE(unique_ptr)
   typedef move_detail::pointer_type<T, D> pointer_type_obtainer;
   typedef ::boost::move_detail::unique_ptr_data
      <typename pointer_type_obtainer::type, D> data_type;
   typedef typename data_type::deleter_arg_type1 deleter_arg_type1;
   typedef typename data_type::deleter_arg_type2 deleter_arg_type2;
   typedef typename data_type::deleter_lvalue_reference       deleter_lvalue_reference;
   typedef typename data_type::deleter_const_lvalue_reference deleter_const_lvalue_reference;
   struct nat {int for_bool;};
   typedef ::boost::move_detail::integral_constant<bool, move_detail::is_same< D, default_delete<T> >::value > is_default_deleter_t;
   #endif

   public:
   //! If the type <tt>remove_reference<D>::type::pointer</tt> exists, then it shall be a
   //! synonym for <tt>remove_reference<D>::type::pointer</tt>. Otherwise it shall be a
   //! synonym for T*.
   typedef typename BOOST_MOVE_SEEDOC(pointer_type_obtainer::type) pointer;
   typedef T element_type;
   typedef D deleter_type;

   //! <b>Requires</b>: D shall satisfy the requirements of DefaultConstructible, and
   //!   that construction shall not throw an exception.
   //!
   //! <b>Effects</b>: Constructs a unique_ptr object that owns nothing, value-initializing the
   //!   stored pointer and the stored deleter.
   //!
   //! <b>Postconditions</b>: <tt>get() == nullptr</tt>. <tt>get_deleter()</tt> returns a reference to the stored deleter.
   //!
   //! <b>Remarks</b>: If this constructor is instantiated with a pointer type or reference type
   //!   for the template argument D, the program is ill-formed.   
   BOOST_CONSTEXPR unique_ptr() BOOST_NOEXCEPT
      : m_data()
   {
      //If this constructor is instantiated with a pointer type or reference type
      //for the template argument D, the program is ill-formed.
      BOOST_STATIC_ASSERT(!::boost::move_detail::is_pointer<D>::value);
      BOOST_STATIC_ASSERT(!::boost::move_detail::is_reference<D>::value);
   }

   //! <b>Requires</b>: D shall satisfy the requirements of DefaultConstructible, and
   //!   that construction shall not throw an exception.
   //!
   //! <b>Effects</b>: Constructs a unique_ptr which owns p, initializing the stored pointer 
   //!   with p and value initializing the stored deleter.
   //!
   //! <b>Postconditions</b>: <tt>get() == p</tt>. <tt>get_deleter()</tt> returns a reference to the stored deleter.
   //!
   //! <b>Remarks</b>: If this constructor is instantiated with a pointer type or reference type
   //!   for the template argument D, the program is ill-formed. 
   explicit unique_ptr(pointer p) BOOST_NOEXCEPT
      : m_data(p)
   {
      //If this constructor is instantiated with a pointer type or reference type
      //for the template argument D, the program is ill-formed.
      BOOST_STATIC_ASSERT(!::boost::move_detail::is_pointer<D>::value);
      BOOST_STATIC_ASSERT(!::boost::move_detail::is_reference<D>::value);
   }

   //!The signature of this constructor depends upon whether D is a reference type.
   //!   - If D is non-reference type A, then the signature is <tt>unique_ptr(pointer p, const A& d)</tt>.
   //!   - If D is an lvalue-reference type A&, then the signature is <tt>unique_ptr(pointer p, A& d)</tt>.
   //!   - If D is an lvalue-reference type const A&, then the signature is <tt>unique_ptr(pointer p, const A& d)</tt>.
   //!
   //! <b>Effects</b>: Constructs a unique_ptr object which owns p, initializing the stored pointer with p and
   //!   initializing the deleter as described above.
   //! 
   //! <b>Postconditions</b>: <tt>get() == p.get_deleter()</tt> returns a reference to the stored deleter. If D is a
   //!   reference type then <tt>get_deleter()</tt> returns a reference to the lvalue d.
   unique_ptr(pointer p, BOOST_MOVE_SEEDOC(deleter_arg_type1) d1) BOOST_NOEXCEPT
      : m_data(p, d1)
   {}

   //! The signature of this constructor depends upon whether D is a reference type.
   //!   - If D is non-reference type A, then the signature is <tt>unique_ptr(pointer p, A&& d)</tt>.
   //!   - If D is an lvalue-reference type A&, then the signature is <tt>unique_ptr(pointer p, A&& d)</tt>.
   //!   - If D is an lvalue-reference type const A&, then the signature is <tt>unique_ptr(pointer p, const A&& d)</tt>.
   //!
   //! <b>Effects</b>: Constructs a unique_ptr object which owns p, initializing the stored pointer with p and
   //!   initializing the deleter as described above.
   //! 
   //! <b>Postconditions</b>: <tt>get() == p.get_deleter()</tt> returns a reference to the stored deleter. If D is a
   //!   reference type then <tt>get_deleter()</tt> returns a reference to the lvalue d.
   unique_ptr(pointer p, BOOST_MOVE_SEEDOC(deleter_arg_type2) d2) BOOST_NOEXCEPT
      : m_data(p, ::boost::move(d2))
   {}

   //! <b>Requires</b>: If D is not a reference type, D shall satisfy the requirements of MoveConstructible.
   //! Construction of the deleter from an rvalue of type D shall not throw an exception.
   //! 
   //! <b>Effects</b>: Constructs a unique_ptr by transferring ownership from u to *this. If D is a reference type,
   //! this deleter is copy constructed from u's deleter; otherwise, this deleter is move constructed from u's
   //! deleter.
   //! 
   //! <b>Postconditions</b>: <tt>get()</tt> yields the value u.get() yielded before the construction. <tt>get_deleter()</tt>
   //! returns a reference to the stored deleter that was constructed from u.get_deleter(). If D is a
   //! reference type then <tt>get_deleter()</tt> and <tt>u.get_deleter()</tt> both reference the same lvalue deleter.
   unique_ptr(BOOST_RV_REF(unique_ptr) u) BOOST_NOEXCEPT
      : m_data(u.release(), ::boost::move_if_not_lvalue_reference<D>(u.get_deleter()))
   {}

   #if !defined(BOOST_NO_CXX11_NULLPTR) || defined(BOOST_MOVE_DOXYGEN_INVOKED)
   //! <b>Effects</b>: Same as <tt>unique_ptr()</tt> (default constructor).
   //! 
   BOOST_CONSTEXPR unique_ptr(std::nullptr_t) BOOST_NOEXCEPT
      : m_data()
   {
      //If this constructor is instantiated with a pointer type or reference type
      //for the template argument D, the program is ill-formed.
      BOOST_STATIC_ASSERT(!::boost::move_detail::is_pointer<D>::value);
      BOOST_STATIC_ASSERT(!::boost::move_detail::is_reference<D>::value);
   }
   #endif

   //! <b>Requires</b>: If E is not a reference type, construction of the deleter from an rvalue of type E shall be
   //!   well formed and shall not throw an exception. Otherwise, E is a reference type and construction of the
   //!   deleter from an lvalue of type E shall be well formed and shall not throw an exception.
   //!
   //! <b>Remarks</b>: This constructor shall not participate in overload resolution unless:
   //!   - <tt>unique_ptr<U, E>::pointer</tt> is implicitly convertible to pointer,
   //!   - U is not an array type, and
   //!   - either D is a reference type and E is the same type as D, or D is not a reference type and E is
   //!      implicitly convertible to D.
   //!
   //! <b>Effects</b>: Constructs a unique_ptr by transferring ownership from u to *this. If E is a reference type,
   //!   this deleter is copy constructed from u's deleter; otherwise, this deleter is move constructed from u's deleter.
   //!
   //! <b>Postconditions</b>: <tt>get()</tt> yields the value <tt>u.get()</tt> yielded before the construction. <tt>get_deleter()</tt>
   //!   returns a reference to the stored deleter that was constructed from <tt>u.get_deleter()</tt>.
   template <class U, class E>
   unique_ptr( BOOST_RV_REF_BEG unique_ptr<U, E> BOOST_RV_REF_END u
      #if !defined(BOOST_MOVE_DOXYGEN_INVOKED)
             , typename ::boost::move_detail::enable_unique_moveconvert_constructible<T, D, U, E>::type * = 0
      #endif
      ) BOOST_NOEXCEPT
      : m_data(u.release(), ::boost::move_if_not_lvalue_reference<E>(u.get_deleter()))
   {}

   //! <b>Requires</b>: The expression <tt>get_deleter()(get())</tt> shall be well formed, shall have well-defined behavior,
   //!   and shall not throw exceptions.
   //!
   //! <b>Effects</b>: If <tt>get() == nullpt1r</tt> there are no effects. Otherwise <tt>get_deleter()(get())</tt>.
   //!
   //! <b>Note</b>: The use of default_delete requires T to be a complete type
   ~unique_ptr()
   {  unique_ptr_call_deleter(m_data.deleter(), m_data.m_p, is_default_deleter_t());   }

   //! <b>Requires</b>: If D is not a reference type, D shall satisfy the requirements of MoveAssignable
   //!   and assignment of the deleter from an rvalue of type D shall not throw an exception. Otherwise, D
   //!   is a reference type; <tt>remove_reference<D>::type</tt> shall satisfy the CopyAssignable requirements and
   //!   assignment of the deleter from an lvalue of type D shall not throw an exception.
   //!
   //! <b>Effects</b>: Transfers ownership from u to *this as if by calling <tt>reset(u.release())</tt> followed
   //!   by <tt>get_deleter() = std::forward<D>(u.get_deleter())</tt>.
   //!
   //! <b>Returns</b>: *this.
   unique_ptr& operator=(BOOST_RV_REF(unique_ptr) u) BOOST_NOEXCEPT
   {
      this->reset(u.release());
      m_data.deleter() = ::boost::move_if_not_lvalue_reference<D>(u.get_deleter());
      return *this;
   }

   //! <b>Requires</b>: If E is not a reference type, assignment of the deleter from an rvalue of type E shall be
   //!   well-formed and shall not throw an exception. Otherwise, E is a reference type and assignment of the
   //!   deleter from an lvalue of type E shall be well-formed and shall not throw an exception.
   //!
   //! <b>Remarks</b>: This operator shall not participate in overload resolution unless:
   //!   - <tt>unique_ptr<U, E>::pointer</tt> is implicitly convertible to pointer and
   //!   - U is not an array type.
   //!
   //! <b>Effects</b>: Transfers ownership from u to *this as if by calling <tt>reset(u.release())</tt> followed by
   //!   <tt>get_deleter() = std::forward<E>(u.get_deleter())</tt>.
   //!
   //! <b>Returns</b>: *this.
   template <class U, class E>
   #if defined(BOOST_MOVE_DOXYGEN_INVOKED)
   unique_ptr&
   #else
   typename ::boost::move_detail::enable_unique_moveconvert_assignable<T, D, U, E, unique_ptr &>::type
   #endif
      operator=(BOOST_RV_REF_BEG unique_ptr<U, E> BOOST_RV_REF_END u) BOOST_NOEXCEPT
   {
      this->reset(u.release());
      m_data.deleter() = ::boost::move_if_not_lvalue_reference<E>(u.get_deleter());
      return *this;
   }

   #if !defined(BOOST_NO_CXX11_NULLPTR) || defined(BOOST_MOVE_DOXYGEN_INVOKED)
   //! <b>Effects</b>: <tt>reset()</tt>.
   //!
   //! <b>Postcondition</b>: <tt>get() == nullptr</tt>
   //!
   //! <b>Returns</b>: *this.
   unique_ptr& operator=(std::nullptr_t) BOOST_NOEXCEPT
   {
      this->reset();
      return *this;
   }
   #else
   //! <b>Effects</b>: <tt>reset()</tt>.
   //!
   //! <b>Postcondition</b>: <tt>get() == nullptr</tt>
   //!
   //! 
   //! <b>Note</b>: Non-standard extension to use 0 as nullptr
   unique_ptr& operator=(int nat::*) BOOST_NOEXCEPT
   {
      this->reset();
      return *this;
   }
   #endif


   //! <b>Requires</b>: <tt>get() != nullptr</tt>.
   //!
   //! <b>Returns</b>: <tt>*get()</tt>.
   #if defined(BOOST_MOVE_DOXYGEN_INVOKED)
   add_lvalue_reference_t
   #else
   typename ::boost::move_detail::add_lvalue_reference<T>::type 
   #endif
      operator*() const
   {  return *get();  }

   //! <b>Requires</b>: <tt>get() != nullptr</tt>.
   //!
   //! <b>Returns</b>: <tt>get()</tt>.
   //!
   //! <b>Note</b>: use typically requires that T be a complete type.
   pointer operator->() const BOOST_NOEXCEPT
   {  return get();  }

   //! <b>Returns</b>: The stored pointer.
   //!
   pointer get() const BOOST_NOEXCEPT
   {  return m_data.m_p;  }

   //! <b>Returns</b>: A reference to the stored deleter.
   //!
   deleter_lvalue_reference get_deleter() BOOST_NOEXCEPT
   {  return m_data.deleter();  }

   //! <b>Returns</b>: A reference to the stored deleter.
   //!
   deleter_const_lvalue_reference get_deleter() const BOOST_NOEXCEPT
   {  return m_data.deleter();  }

   #ifdef BOOST_MOVE_DOXYGEN_INVOKED
   //! <b>Returns</b>: Returns: get() != nullptr.
   //!
   explicit operator bool() const BOOST_NOEXCEPT;
   #else
   operator int nat::*() const BOOST_NOEXCEPT
   { return m_data.m_p ? &nat::for_bool : (int nat::*)0; }
   #endif

   //! <b>Postcondition</b>: <tt>get() == nullptr</tt>.
   //!
   //! <b>Returns</b>: The value <tt>get()</tt> had at the start of the call to release.   
   pointer release() BOOST_NOEXCEPT
   {
      const pointer tmp = m_data.m_p;
      m_data.m_p = pointer();
      return tmp;
   }

   //! <b>Requires</b>: The expression <tt>get_deleter()(get())</tt> shall be well formed, shall have well-defined behavior,
   //!   and shall not throw exceptions.
   //!
   //! <b>Effects</b>: assigns p to the stored pointer, and then if the old value of the stored pointer, old_p, was not
   //!   equal to nullptr, calls <tt>get_deleter()(old_p)</tt>. Note: The order of these operations is significant
   //!   because the call to <tt>get_deleter()</tt> may destroy *this.
   //!
   //! <b>Postconditions</b>: <tt>get() == p</tt>. Note: The postcondition does not hold if the call to <tt>get_deleter()</tt>
   //!   destroys *this since <tt>this->get()</tt> is no longer a valid expression.
   void reset(pointer p = pointer()) BOOST_NOEXCEPT
   {
      pointer tmp = m_data.m_p;
      m_data.m_p = p;
      unique_ptr_call_deleter(m_data.deleter(), tmp, is_default_deleter_t());
   }

   //! <b>Requires</b>: <tt>get_deleter()</tt> shall be swappable and shall not throw an exception under swap.
   //!
   //! <b>Effects</b>: Invokes swap on the stored pointers and on the stored deleters of *this and u.
   void swap(unique_ptr& u) BOOST_NOEXCEPT
   {
      using boost::move_detail::swap;
      swap(m_data.m_p, u.m_data.m_p);
      swap(m_data.deleter(), u.m_data.deleter());
   }

   #if !defined(BOOST_MOVE_DOXYGEN_INVOKED)
   data_type m_data;
   #endif
};

//!A specialization for array types (<tt>array_of_T = T[]</tt>) is provided with a slightly altered interface.
//!   - Conversions between different types of <tt>unique_ptr<T[], D></tt> or to or from the non-array forms of
//!      unique_ptr produce an ill-formed program.
//!   - Pointers to types derived from T are rejected by the constructors, and by reset.
//!   - The observers <tt>operator*</tt> and <tt>operator-></tt> are not provided.
//!   - The indexing observer <tt>operator[]</tt> is provided.
//!   - The default deleter will call <tt>delete[]</tt>.
//!
//! Descriptions are provided below only for member functions that have behavior different from the primary template.
//!
//! \tparam array_of_T is an alias for types of form T[]. The template argument T shall be a complete type.
template <class T, class D>
class unique_ptr
#if defined(BOOST_MOVE_DOXYGEN_INVOKED)
< array_of_T
#else
< T[]
#endif
, D>
{
   #if defined(BOOST_MOVE_DOXYGEN_INVOKED)
   public:
   unique_ptr(const unique_ptr&) = delete;
   unique_ptr& operator=(const unique_ptr&) = delete;
   private:
   #else
   BOOST_MOVABLE_BUT_NOT_COPYABLE(unique_ptr)
   typedef ::boost::move_detail::pointer_type<T, D> pointer_type_obtainer;
   typedef ::boost::move_detail::unique_ptr_data
      <typename pointer_type_obtainer::type, D> data_type;
   typedef typename data_type::deleter_arg_type1 deleter_arg_type1;
   typedef typename data_type::deleter_arg_type2 deleter_arg_type2;
   typedef typename data_type::deleter_lvalue_reference       deleter_lvalue_reference;
   typedef typename data_type::deleter_const_lvalue_reference deleter_const_lvalue_reference;
   struct nat {int for_bool;};
   typedef ::boost::move_detail::integral_constant<bool, ::boost::move_detail::is_same< D, default_delete<T[]> >::value > is_default_deleter_t;
   #endif

   public:
   typedef typename BOOST_MOVE_SEEDOC(pointer_type_obtainer::type) pointer;
   typedef T element_type;
   typedef D deleter_type;

   BOOST_CONSTEXPR unique_ptr() BOOST_NOEXCEPT
      : m_data()
   {
      //If this constructor is instantiated with a pointer type or reference type
      //for the template argument D, the program is ill-formed.
      BOOST_STATIC_ASSERT(!::boost::move_detail::is_pointer<D>::value);
      BOOST_STATIC_ASSERT(!::boost::move_detail::is_reference<D>::value);
   }

   //! This constructor behave the same as in the primary template except that it does
   //! not accept pointer types which are convertible to pointer.
   #if defined(BOOST_MOVE_DOXYGEN_INVOKED)
   unique_ptr(pointer p);
   #else
   template<class P>
   explicit unique_ptr(P p
      ,typename ::boost::move_detail::enable_same_cvelement_and_convertible<P, pointer>::type* = 0
                      ) BOOST_NOEXCEPT
      : m_data(p)
   {
      BOOST_STATIC_ASSERT(!::boost::move_detail::is_pointer<D>::value);
      BOOST_STATIC_ASSERT(!::boost::move_detail::is_reference<D>::value);
   }
   #endif

   //! This constructor behave the same as in the primary template except that it does
   //! not accept pointer types which are convertible to pointer.
   #if defined(BOOST_MOVE_DOXYGEN_INVOKED)
   unique_ptr(pointer p, BOOST_MOVE_SEEDOC(deleter_arg_type1) d1);
   #else
   template<class P>
   unique_ptr(P p, BOOST_MOVE_SEEDOC(deleter_arg_type1) d1
      ,typename ::boost::move_detail::enable_same_cvelement_and_convertible<P, pointer>::type* = 0
             ) BOOST_NOEXCEPT
      : m_data(p, d1)
   {}
   #endif

   //! This assignement behave the same as in the primary template except that it does
   //! only accept pointer types which are equal or less cv qualified than pointer
   #if defined(BOOST_MOVE_DOXYGEN_INVOKED)
   unique_ptr(pointer p, BOOST_MOVE_SEEDOC(deleter_arg_type2) d2);
   #else
   template<class P>
   unique_ptr(P p, BOOST_MOVE_SEEDOC(deleter_arg_type2) d2
      ,typename ::boost::move_detail::enable_same_cvelement_and_convertible<P, pointer>::type* = 0
             ) BOOST_NOEXCEPT
      : m_data(p, ::boost::move(d2))
   {}
   #endif

   unique_ptr(BOOST_RV_REF(unique_ptr) u) BOOST_NOEXCEPT
      : m_data(u.release(), ::boost::move_if_not_lvalue_reference<D>(u.get_deleter()))
   {}

   //! This assignement behave the same as in the primary template except that it does
   //! only accept pointer types which are equal or less cv qualified than pointer.
   //!
   //! <b>Note</b>: Non-standard extension
   template <class U, class E>
   unique_ptr( BOOST_RV_REF_BEG unique_ptr<U, E> BOOST_RV_REF_END u
      #if !defined(BOOST_MOVE_DOXYGEN_INVOKED)
             , typename ::boost::move_detail::enable_uniquearray_moveconvert_constructible<T, D, U, E>::type * = 0
      #endif
             ) BOOST_NOEXCEPT
      : m_data(u.release(), ::boost::move_if_not_lvalue_reference<E>(u.get_deleter()))
   {}

   #if !defined(BOOST_NO_CXX11_NULLPTR) || defined(BOOST_MOVE_DOXYGEN_INVOKED)
   BOOST_CONSTEXPR unique_ptr(std::nullptr_t) BOOST_NOEXCEPT
      : m_data()
   {
      //If this constructor is instantiated with a pointer type or reference type
      //for the template argument D, the program is ill-formed.
      BOOST_STATIC_ASSERT(!::boost::move_detail::is_pointer<D>::value);
      BOOST_STATIC_ASSERT(!::boost::move_detail::is_reference<D>::value);
   }
   #endif

   ~unique_ptr()
   {
      unique_ptr_call_deleter(m_data.deleter(), m_data.m_p, is_default_deleter_t());
   }

   unique_ptr& operator=(BOOST_RV_REF(unique_ptr) u) BOOST_NOEXCEPT
   {
      this->reset(u.release());
      m_data.deleter() = ::boost::move_if_not_lvalue_reference<D>(u.get_deleter());
      return *this;
   }

   //! This assignement behave the same as in the primary template except that it does
   //! only accept pointer types which are equal or less cv qualified than pointer
   //!
   //! <b>Note</b>: Non-standard extension
   template <class U, class E>
   #if defined(BOOST_MOVE_DOXYGEN_INVOKED)
   unique_ptr&
   #else
   typename ::boost::move_detail::enable_uniquearray_moveconvert_assignable<T, D, U, E, unique_ptr &>::type
   #endif
      operator=(BOOST_RV_REF_BEG unique_ptr<U, E> BOOST_RV_REF_END u) BOOST_NOEXCEPT
   {
      this->reset(u.release());
      m_data.deleter() = ::boost::move_if_not_lvalue_reference<E>(u.get_deleter());
      return *this;
   }

   #if !defined(BOOST_NO_CXX11_NULLPTR) || defined(BOOST_MOVE_DOXYGEN_INVOKED)
   unique_ptr& operator=(std::nullptr_t) BOOST_NOEXCEPT
   {
      this->reset();
      return *this;
   }
   #else
   unique_ptr& operator=(int nat::*) BOOST_NOEXCEPT
   {
      this->reset();
      return *this;
   }
   #endif

   //! <b>Requires</b>: i < the number of elements in the array to which the stored pointer points.
   //! <b>Returns</b>: <tt>get()[i]</tt>.
   T& operator[](size_t i) const
   {
      const pointer p = this->get();
      BOOST_ASSERT(p);
      return p[i];
   }

   pointer get() const BOOST_NOEXCEPT
   {  return m_data.m_p;  }

   deleter_lvalue_reference get_deleter() BOOST_NOEXCEPT
   {  return m_data.deleter();  }

   deleter_const_lvalue_reference get_deleter() const BOOST_NOEXCEPT
   {  return m_data.deleter();  }

   #ifdef BOOST_MOVE_DOXYGEN_INVOKED
   explicit operator bool() const BOOST_NOEXCEPT;
   #else
   operator int nat::*() const BOOST_NOEXCEPT
   { return m_data.m_p ? &nat::for_bool : (int nat::*)0; }
   #endif

   pointer release() BOOST_NOEXCEPT
   {
      const pointer tmp = m_data.m_p;
      m_data.m_p = pointer();
      return tmp;
   }

   #if defined(BOOST_MOVE_DOXYGEN_INVOKED)
   void reset(pointer p = pointer());
   #else
   template<class P>
   void reset(P p
      ,typename ::boost::move_detail::enable_same_cvelement_and_convertible<P, pointer>::type* = 0
             ) BOOST_NOEXCEPT
   {
      pointer tmp = m_data.m_p;
      m_data.m_p = p;
      unique_ptr_call_deleter(m_data.deleter(), tmp, is_default_deleter_t());
   }

   void reset() BOOST_NOEXCEPT
   {
      pointer tmp = m_data.m_p;
      m_data.m_p = pointer();
      unique_ptr_call_deleter(m_data.deleter(), tmp, is_default_deleter_t());
   }
   #endif

   #if !defined(BOOST_NO_CXX11_NULLPTR) || defined(BOOST_MOVE_DOXYGEN_INVOKED)
   void reset(std::nullptr_t) BOOST_NOEXCEPT
   {  this->reset(); }
   #endif

   void swap(unique_ptr& u) BOOST_NOEXCEPT
   {
      using boost::move_detail::swap;
      swap(m_data.m_p, u.m_data.m_p);
      swap(m_data.deleter(), u.m_data.deleter());
   }

   #if defined(BOOST_MOVE_DOXYGEN_INVOKED)
   template <class U> void reset(U) = delete;
   #else
   data_type m_data;
   #endif
};

//! <b>Effects</b>: Calls <tt>x.swap(y)</tt>.
//!
template <class T, class D>
inline void swap(unique_ptr<T, D> &x, unique_ptr<T, D> &y) BOOST_NOEXCEPT
{  x.swap(y); }

//! <b>Returns</b>: <tt>x.get() == y.get()</tt>.
//!
template <class T1, class D1, class T2, class D2>
inline bool operator==(const unique_ptr<T1, D1> &x, const unique_ptr<T2, D2> &y)
{  return x.get() == y.get(); }

//! <b>Returns</b>: <tt>x.get() != y.get()</tt>.
//!
template <class T1, class D1, class T2, class D2>
inline bool operator!=(const unique_ptr<T1, D1> &x, const unique_ptr<T2, D2> &y)
{  return x.get() != y.get(); }

#if 0
//! <b>Requires</b>: Let CT be <tt>common_type<unique_ptr<T1, D1>::pointer, unique_ptr<T2, D2>::pointer>::
//!   type</tt>. Then the specialization <tt>less<CT></tt> shall be a function object type that induces a
//!   strict weak ordering on the pointer values.
//!
//! <b>Returns</b>: less<CT>()(x.get(), y.get()).
//!
//! <b>Remarks</b>: If <tt>unique_ptr<T1, D1>::pointer</tt> is not implicitly convertible to CT or 
//!   <tt>unique_ptr<T2, D2>::pointer</tt> is not implicitly convertible to CT, the program is ill-formed.
#else
//! <b>Returns</b>: x.get() < y.get().
//!
//! <b>Remarks</b>: This comparison shall induces a
//!   strict weak ordering on the pointer values.
#endif
template <class T1, class D1, class T2, class D2>
inline bool operator<(const unique_ptr<T1, D1> &x, const unique_ptr<T2, D2> &y)
{
   #if 0
    typedef typename common_type
      < typename unique_ptr<T1, D1>::pointer
      , typename unique_ptr<T2, D2>::pointer>::type CT;
    return std::less<CT>()(x.get(), y.get());*/
   #endif
   return x.get() < y.get();
}

//! <b>Returns</b>: !(y < x).
//!
template <class T1, class D1, class T2, class D2>
inline bool operator<=(const unique_ptr<T1, D1> &x, const unique_ptr<T2, D2> &y)
{  return !(y < x);  }

//! <b>Returns</b>: y < x.
//!
template <class T1, class D1, class T2, class D2>
inline bool operator>(const unique_ptr<T1, D1> &x, const unique_ptr<T2, D2> &y)
{  return y < x;  }

//! <b>Returns</b>:!(x < y).
//!
template <class T1, class D1, class T2, class D2>
inline bool operator>=(const unique_ptr<T1, D1> &x, const unique_ptr<T2, D2> &y)
{  return !(x < y);  }

#if !defined(BOOST_NO_CXX11_NULLPTR) || defined(BOOST_MOVE_DOXYGEN_INVOKED)

//! <b>Returns</b>:!x.
//!
template <class T1, class D1>
inline bool operator==(const unique_ptr<T1, D1> &x, std::nullptr_t) BOOST_NOEXCEPT
{  return !x;  }

//! <b>Returns</b>:!x.
//!
template <class T1, class D1>
inline bool operator==(std::nullptr_t, const unique_ptr<T1, D1> &x) BOOST_NOEXCEPT
{  return !x;  }

//! <b>Returns</b>: (bool)x.
//!
template <class T1, class D1>
inline bool operator!=(const unique_ptr<T1, D1> &x, std::nullptr_t) BOOST_NOEXCEPT
{  return !!x;  }

//! <b>Returns</b>: (bool)x.
//!
template <class T1, class D1>
inline bool operator!=(std::nullptr_t, const unique_ptr<T1, D1> &x) BOOST_NOEXCEPT
{  return !!x;  }

//! <b>Requires</b>: The specialization <tt>less<unique_ptr<T, D>::pointer></tt> shall be a function object type
//!   that induces a strict weak ordering on the pointer values.
//!
//! <b>Returns</b>: <tt>less<unique_ptr<T, D>::pointer>()(x.get(), nullptr)</tt>.
template <class T1, class D1>
inline bool operator<(const unique_ptr<T1, D1> &x, std::nullptr_t)
{
   typedef typename unique_ptr<T1, D1>::pointer pointer;
   return x.get() < pointer();
}

//! <b>Requires</b>: The specialization <tt>less<unique_ptr<T, D>::pointer></tt> shall be a function object type
//!   that induces a strict weak ordering on the pointer values.
//!
//! <b>Returns</b>: The second function template returns <tt>less<unique_ptr<T, D>::pointer>()(nullptr, x.get())</tt>.
template <class T1, class D1>
inline bool operator<(std::nullptr_t, const unique_ptr<T1, D1> &x)
{
   typedef typename unique_ptr<T1, D1>::pointer pointer;
   return pointer() < x.get();
}

//! <b>Returns</b>: <tt>nullptr < x</tt>.
//!
template <class T1, class D1>
inline bool operator>(const unique_ptr<T1, D1> &x, std::nullptr_t)
{
   typedef typename unique_ptr<T1, D1>::pointer pointer;
   return x.get() > pointer();
}

//! <b>Returns</b>: <tt>x < nullptr</tt>.
//!
template <class T1, class D1>
inline bool operator>(std::nullptr_t, const unique_ptr<T1, D1> &x)
{
   typedef typename unique_ptr<T1, D1>::pointer pointer;
   return pointer() > x.get();
}

//! <b>Returns</b>: <tt>!(nullptr < x)</tt>.
//!
template <class T1, class D1>
inline bool operator<=(const unique_ptr<T1, D1> &x, std::nullptr_t)
{  return !(nullptr < x);  }

//! <b>Returns</b>: <tt>!(x < nullptr)</tt>.
//!
template <class T1, class D1>
inline bool operator<=(std::nullptr_t, const unique_ptr<T1, D1> &x)
{  return !(x < nullptr);  }

//! <b>Returns</b>: <tt>!(x < nullptr)</tt>.
//!
template <class T1, class D1>
inline bool operator>=(const unique_ptr<T1, D1> &x, std::nullptr_t)
{  return !(x < nullptr);  }

//! <b>Returns</b>: <tt>!(nullptr < x)</tt>.
//!
template <class T1, class D1>
inline bool operator>=(std::nullptr_t, const unique_ptr<T1, D1> &x)
{  return !(nullptr < x);  }

#endif   //   #if !defined(BOOST_NO_CXX11_NULLPTR) || defined(BOOST_MOVE_DOXYGEN_INVOKED)

}  //namespace movelib {
}  //namespace boost{

#include <boost/move/detail/config_end.hpp>

#endif   //#ifndef BOOST_MOVE_UNIQUE_PTR_HPP_INCLUDED
