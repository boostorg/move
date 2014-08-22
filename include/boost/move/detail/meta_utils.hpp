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

#ifndef BOOST_MOVE_DETAIL_META_UTILS_HPP
#define BOOST_MOVE_DETAIL_META_UTILS_HPP

#include <boost/move/detail/config_begin.hpp>

//Small meta-typetraits to support move

namespace boost {

#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
//Forward declare boost::rv
template <class T> class rv;
#endif

namespace move_detail {

//////////////////////////////////////
//              empty
//////////////////////////////////////
struct empty{};

//////////////////////////////////////
//              nat
//////////////////////////////////////
struct nat{};

//////////////////////////////////////
//            natify
//////////////////////////////////////
template <class T> struct natify{};

//////////////////////////////////////
//             if_c
//////////////////////////////////////
template<bool C, typename T1, typename T2>
struct if_c
{
   typedef T1 type;
};

template<typename T1, typename T2>
struct if_c<false,T1,T2>
{
   typedef T2 type;
};

//////////////////////////////////////
//             if_
//////////////////////////////////////
template<typename T1, typename T2, typename T3>
struct if_
{
   typedef typename if_c<0 != T1::value, T2, T3>::type type;
};

//enable_if_
template <bool B, class T = nat>
struct enable_if_c
{
   typedef T type;
};

//////////////////////////////////////
//          enable_if_c
//////////////////////////////////////
template <class T>
struct enable_if_c<false, T> {};

//////////////////////////////////////
//           enable_if
//////////////////////////////////////
template <class Cond, class T = nat>
struct enable_if : public enable_if_c<Cond::value, T> {};

//////////////////////////////////////
//          disable_if
//////////////////////////////////////
template <class Cond, class T = nat>
struct disable_if : public enable_if_c<!Cond::value, T> {};

//////////////////////////////////////
//          integral_constant
//////////////////////////////////////
template<class T, T v>
struct integral_constant
{
   static const T value = v;
   typedef T value_type;
   typedef integral_constant<T, v> type;
};

typedef integral_constant<bool, true >  true_type;
typedef integral_constant<bool, false > false_type;

//////////////////////////////////////
//             identity
//////////////////////////////////////
template <class T>
struct identity
{
   typedef T type;
};

//////////////////////////////////////
//                and_
//////////////////////////////////////
template <typename Condition1, typename Condition2, typename Condition3 = integral_constant<bool, true> >
struct and_
   : public integral_constant<bool, Condition1::value && Condition2::value && Condition3::value>
{};

//////////////////////////////////////
//                not_
//////////////////////////////////////
template <typename Boolean>
struct not_
   : public integral_constant<bool, !Boolean::value>
{};

//////////////////////////////////////
//          remove_reference
//////////////////////////////////////
template<class T>
struct remove_reference
{
   typedef T type;
};

template<class T>
struct remove_reference<T&>
{
   typedef T type;
};

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template<class T>
struct remove_reference<T&&>
{
   typedef T type;
};

#else

template<class T>
struct remove_reference< rv<T> >
{
   typedef T type;
};

template<class T>
struct remove_reference< rv<T> &>
{
   typedef T type;
};

template<class T>
struct remove_reference< const rv<T> &>
{
   typedef T type;
};


#endif


//////////////////////////////////////
//             remove_const
//////////////////////////////////////
template< class T >
struct remove_const
{
   typedef T type;
};

template< class T >
struct remove_const<const T>
{
   typedef T type;
};

//////////////////////////////////////
//             remove_volatile
//////////////////////////////////////
template< class T >
struct remove_volatile
{
   typedef T type;
};

template< class T >
struct remove_volatile<volatile T>
{
   typedef T type;
};

//////////////////////////////////////
//             remove_cv
//////////////////////////////////////
template< class T >
struct remove_cv
{
    typedef typename remove_volatile
      <typename remove_const<T>::type>::type type;
};

//////////////////////////////////////
//             add_const
//////////////////////////////////////
template<class T>
struct add_const
{
   typedef const T type;
};

template<class T>
struct add_const<T&>
{
   typedef const T& type;
};

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template<class T>
struct add_const<T&&>
{
   typedef T&& type;
};

#endif

//////////////////////////////////////
//          remove_extent
//////////////////////////////////////
template<class T>
struct remove_extent
{
   typedef T type;
};
 
template<class T>
struct remove_extent<T[]>
{
   typedef T type;
};
 
template<class T, std::size_t N>
struct remove_extent<T[N]>
{
   typedef T type;
};

//////////////////////////////////////
//          element_pointer
//////////////////////////////////////
template<class T>
struct element_pointer
{
   typedef typename remove_extent<T>::type element_type;
   typedef element_type* type;
};

//////////////////////////////////////
//      add_lvalue_reference
//////////////////////////////////////
template<class T>
struct add_lvalue_reference
{
   typedef T& type;
};

template<class T>
struct add_lvalue_reference<T&>
{
   typedef T& type;
};

template<>
struct add_lvalue_reference<void>
{
   typedef void type;
};

template<>
struct add_lvalue_reference<const void>
{
   typedef const void type;
};

template<>
struct add_lvalue_reference<volatile void>
{
   typedef volatile void type;
};

template<>
struct add_lvalue_reference<const volatile void>
{
   typedef const volatile void type;
};


//////////////////////////////////////
//             is_same
//////////////////////////////////////
template<class T, class U>
struct is_same
{
   static const bool value = false;
};
 
template<class T>
struct is_same<T, T>
{
   static const bool value = true;
};

//////////////////////////////////////
//             is_pointer
//////////////////////////////////////
template< class T >
struct is_pointer
{
    static const bool value = false;
};

template< class T >
struct is_pointer<T*>
{
    static const bool value = true;
};

//////////////////////////////////////
//             is_reference
//////////////////////////////////////
template< class T >
struct is_reference
{
    static const bool value = false;
};

template< class T >
struct is_reference<T&>
{
    static const bool value = true;
};

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template< class T >
struct is_reference<T&&>
{
    static const bool value = true;
};

#endif

//////////////////////////////////////
//             is_lvalue_reference
//////////////////////////////////////
template<class T>
struct is_lvalue_reference
{
    static const bool value = false;
};

template<class T>
struct is_lvalue_reference<T&>
{
    static const bool value = true;
};

//////////////////////////////////////
//          is_array
//////////////////////////////////////
template<class T>
struct is_array
{
   static const bool value = false;
};
 
template<class T>
struct is_array<T[]>
{
   static const bool value = true;
};
 
template<class T, std::size_t N>
struct is_array<T[N]>
{
   static const bool value = true;
};

//////////////////////////////////////
//          is_class_or_union
//////////////////////////////////////
template<class T>
struct is_class_or_union
{
   struct twochar { char _[2]; };
   template <class U>
   static char is_class_or_union_tester(void(U::*)(void));
   template <class U>
   static twochar is_class_or_union_tester(...);
   static const bool value = sizeof(is_class_or_union_tester<T>(0)) == sizeof(char);
};

//////////////////////////////////////
//             addressof
//////////////////////////////////////
template<class T>
struct addr_impl_ref
{
   T & v_;
   inline addr_impl_ref( T & v ): v_( v ) {}
   inline operator T& () const { return v_; }

   private:
   addr_impl_ref & operator=(const addr_impl_ref &);
};

template<class T>
struct addressof_impl
{
   static inline T * f( T & v, long )
   {
      return reinterpret_cast<T*>(
         &const_cast<char&>(reinterpret_cast<const volatile char &>(v)));
   }

   static inline T * f( T * v, int )
   {  return v;  }
};

template<class T>
inline T * addressof( T & v )
{
   return ::boost::move_detail::addressof_impl<T>::f
      ( ::boost::move_detail::addr_impl_ref<T>( v ), 0 );
}

//////////////////////////////////////
//          has_pointer_type
//////////////////////////////////////
template <class T>
struct has_pointer_type
{
   struct two { char c[2]; };
   template <class U> static two test(...);
   template <class U> static char test(typename U::pointer* = 0);
   static const bool value = sizeof(test<T>(0)) == 1;
};

//////////////////////////////////////
//             pointer_type
//////////////////////////////////////
template <class T, class D, bool = has_pointer_type<D>::value>
struct pointer_type_imp
{
    typedef typename D::pointer type;
};

template <class T, class D>
struct pointer_type_imp<T, D, false>
{
    typedef typename remove_extent<T>::type* type;
};

template <class T, class D>
struct pointer_type
{
    typedef typename pointer_type_imp<T, typename remove_reference<D>::type>::type type;
};

//////////////////////////////////////
//           is_convertible
//////////////////////////////////////
#if defined(_MSC_VER) && (_MSC_VER >= 1400)

//use intrinsic since in MSVC
//overaligned types can't go through ellipsis
template <class T, class U>
struct is_convertible
{
   static const bool value = __is_convertible_to(T, U);
};

#else

template <class T, class U>
class is_convertible
{
   typedef typename add_lvalue_reference<T>::type t_reference;
   typedef char true_t;
   class false_t { char dummy[2]; };
   static false_t dispatch(...);
   static true_t  dispatch(U);
   static t_reference       trigger();
   public:
   static const bool value = sizeof(dispatch(trigger())) == sizeof(true_t);
};

#endif

//////////////////////////////////////
//    is_unary_or_binary_function
//////////////////////////////////////
#if defined(BOOST_MSVC) || defined(__BORLANDC_)
#define BOOST_MOVE_TT_DECL __cdecl
#else
#define BOOST_MOVE_TT_DECL
#endif

#if defined(_MSC_EXTENSIONS) && !defined(__BORLAND__) && !defined(_WIN64) && !defined(_M_ARM) && !defined(UNDER_CE)
#define BOOST_MOVE_TT_TEST_MSC_FUNC_SIGS
#endif

template <typename T>
struct is_unary_or_binary_function_impl
{  static const bool value = false; };

// avoid duplicate definitions of is_unary_or_binary_function_impl
#ifndef BOOST_MOVE_TT_TEST_MSC_FUNC_SIGS

template <typename R>
struct is_unary_or_binary_function_impl<R (*)()>
{  static const bool value = true;  };

template <typename R>
struct is_unary_or_binary_function_impl<R (*)(...)>
{  static const bool value = true;  };

#else // BOOST_MOVE_TT_TEST_MSC_FUNC_SIGS

template <typename R>
struct is_unary_or_binary_function_impl<R (__stdcall*)()>
{  static const bool value = true;  };

#ifndef _MANAGED

template <typename R>
struct is_unary_or_binary_function_impl<R (__fastcall*)()>
{  static const bool value = true;  };

#endif

template <typename R>
struct is_unary_or_binary_function_impl<R (__cdecl*)()>
{  static const bool value = true;  };

template <typename R>
struct is_unary_or_binary_function_impl<R (__cdecl*)(...)>
{  static const bool value = true;  };

#endif

// avoid duplicate definitions of is_unary_or_binary_function_impl
#ifndef BOOST_MOVE_TT_TEST_MSC_FUNC_SIGS

template <typename R, class T0>
struct is_unary_or_binary_function_impl<R (*)(T0)>
{  static const bool value = true;  };

template <typename R, class T0>
struct is_unary_or_binary_function_impl<R (*)(T0...)>
{  static const bool value = true;  };

#else // BOOST_MOVE_TT_TEST_MSC_FUNC_SIGS

template <typename R, class T0>
struct is_unary_or_binary_function_impl<R (__stdcall*)(T0)>
{  static const bool value = true;  };

#ifndef _MANAGED

template <typename R, class T0>
struct is_unary_or_binary_function_impl<R (__fastcall*)(T0)>
{  static const bool value = true;  };

#endif

template <typename R, class T0>
struct is_unary_or_binary_function_impl<R (__cdecl*)(T0)>
{  static const bool value = true;  };

template <typename R, class T0>
struct is_unary_or_binary_function_impl<R (__cdecl*)(T0...)>
{  static const bool value = true;  };

#endif

// avoid duplicate definitions of is_unary_or_binary_function_impl
#ifndef BOOST_MOVE_TT_TEST_MSC_FUNC_SIGS

template <typename R, class T0, class T1>
struct is_unary_or_binary_function_impl<R (*)(T0, T1)>
{  static const bool value = true;  };

template <typename R, class T0, class T1>
struct is_unary_or_binary_function_impl<R (*)(T0, T1...)>
{  static const bool value = true;  };

#else // BOOST_MOVE_TT_TEST_MSC_FUNC_SIGS

template <typename R, class T0, class T1>
struct is_unary_or_binary_function_impl<R (__stdcall*)(T0, T1)>
{  static const bool value = true;  };

#ifndef _MANAGED

template <typename R, class T0, class T1>
struct is_unary_or_binary_function_impl<R (__fastcall*)(T0, T1)>
{  static const bool value = true;  };

#endif

template <typename R, class T0, class T1>
struct is_unary_or_binary_function_impl<R (__cdecl*)(T0, T1)>
{  static const bool value = true;  };

template <typename R, class T0, class T1>
struct is_unary_or_binary_function_impl<R (__cdecl*)(T0, T1...)>
{  static const bool value = true;  };
#endif

template <typename T>
struct is_unary_or_binary_function_impl<T&>
{  static const bool value = false; };

template<typename T>
struct is_unary_or_binary_function
{  static const bool value = is_unary_or_binary_function_impl<T>::value;   };

}  //namespace move_detail {
}  //namespace boost {

#include <boost/move/detail/config_end.hpp>

#endif //#ifndef BOOST_MOVE_DETAIL_META_UTILS_HPP
