#//////////////////////////////////////////////////////////////////////////////
#//
#// (C) Copyright Ion Gaztanaga 2015-2015.
#// Distributed under the Boost Software License, Version 1.0.
#// (See accompanying file LICENSE_1_0.txt or copy at
#// http://www.boost.org/LICENSE_1_0.txt)
#//
#// See http://www.boost.org/libs/move for documentation.
#//
#//////////////////////////////////////////////////////////////////////////////
//Both can be pushed (e.g. clang-cl with libc++), so pop them independently
#ifdef BOOST_MOVE_STD_NS_GCC_DIAGNOSTIC_PUSH
   #pragma GCC diagnostic pop
   #undef BOOST_MOVE_STD_NS_GCC_DIAGNOSTIC_PUSH
#endif   //BOOST_MOVE_STD_NS_GCC_DIAGNOSTIC_PUSH

#ifdef BOOST_MOVE_STD_NS_MSVC_WARNING_PUSH
   #pragma warning (pop)
   #undef BOOST_MOVE_STD_NS_MSVC_WARNING_PUSH
#endif   //BOOST_MOVE_STD_NS_MSVC_WARNING_PUSH

#undef BOOST_MOVE_STD_NS_BEG
#undef BOOST_MOVE_STD_NS_END
