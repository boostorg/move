//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2026-2026.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////

//Include the trait before any standard header
#include <boost/move/detail/iterator_traits.hpp>

//An iterator that defines no member types (only valid as a non-contiguous iterator)
struct no_member_types_iterator
{};

//A C++20 style iterator without "pointer" and "reference"
struct cxx20_style_iterator
{
   typedef int                               value_type;
   typedef std::ptrdiff_t                    difference_type;
   typedef std::random_access_iterator_tag   iterator_category;
   int &operator*() const;
};

#include <boost/move/detail/workaround.hpp>
#include <boost/move/iterator.hpp>
#include <vector>
#include <deque>
#include <list>
#include <string>
#include <iterator>

//Iterators that use boost::movelib::contiguous_iterator_tag (detected in any standard mode)
struct boost_tag_concept_iterator
{
   typedef int                                     value_type;
   typedef std::ptrdiff_t                          difference_type;
   typedef int*                                    pointer;
   typedef int&                                    reference;
   typedef std::random_access_iterator_tag         iterator_category;
   typedef boost::movelib::contiguous_iterator_tag iterator_concept;
};

struct derived_boost_tag : boost::movelib::contiguous_iterator_tag {};

struct derived_boost_tag_concept_iterator
{
   typedef derived_boost_tag iterator_concept;
};

struct boost_tag_category_iterator
{
   typedef boost::movelib::contiguous_iterator_tag iterator_category;
};

#if defined(BOOST_MOVE_CONTIGUOUS_ITERATOR_TAG)

struct contiguous_concept_iterator
{
   typedef int                               value_type;
   typedef std::ptrdiff_t                    difference_type;
   typedef int*                              pointer;
   typedef int&                              reference;
   typedef std::random_access_iterator_tag   iterator_category;
   typedef std::contiguous_iterator_tag      iterator_concept;
};

struct derived_contiguous_tag : std::contiguous_iterator_tag {};

struct derived_concept_iterator
{
   typedef derived_contiguous_tag iterator_concept;
};

struct contiguous_category_iterator
{
   typedef std::contiguous_iterator_tag iterator_category;
};

static const bool is_cxx20 = true;

#else

static const bool is_cxx20 = false;

#endif   //BOOST_MOVE_CONTIGUOUS_ITERATOR_TAG

template<class T, class U>
struct is_same
{  static const bool value = false;  };

template<class T>
struct is_same<T, T>
{  static const bool value = true;  };

//Checks the expected value and, if available, compares it with std::contiguous_iterator
template<class It, bool Expected>
void test_contiguous()
{
   BOOST_MOVE_STATIC_ASSERT(boost::movelib::is_contiguous_iterator<It>::value == Expected);
   #if defined(__cpp_lib_concepts)
   BOOST_MOVE_STATIC_ASSERT(boost::movelib::is_contiguous_iterator<It>::value == std::contiguous_iterator<It>);
   #endif
}

int main()
{
   using boost::movelib::iterator_traits;

   //Pointers
   test_contiguous<int*, true>();
   test_contiguous<const int*, true>();
   test_contiguous<volatile int*, true>();
   test_contiguous<const volatile int*, true>();

   //Iterators without a tag
   BOOST_MOVE_STATIC_ASSERT(!boost::movelib::is_contiguous_iterator<no_member_types_iterator>::value);
   BOOST_MOVE_STATIC_ASSERT(!boost::movelib::is_contiguous_iterator<cxx20_style_iterator>::value);

   //Fallbacks for C++20 style iterators
   BOOST_MOVE_STATIC_ASSERT((is_same<iterator_traits<cxx20_style_iterator>::pointer, void>::value));
   #if !defined(BOOST_NO_CXX11_DECLTYPE)
   BOOST_MOVE_STATIC_ASSERT((is_same<iterator_traits<cxx20_style_iterator>::reference, int&>::value));
   #endif
   BOOST_MOVE_STATIC_ASSERT((is_same<iterator_traits<int*>::reference, int&>::value));
   BOOST_MOVE_STATIC_ASSERT((is_same<iterator_traits<std::vector<int>::iterator>::reference, int&>::value));
   BOOST_MOVE_STATIC_ASSERT((is_same<iterator_traits<std::vector<int>::const_iterator>::pointer, const int*>::value));

   #if defined(__cpp_lib_concepts)
   //No member types: obtained from the std::iterator_traits specialization
   typedef std::common_iterator<std::counted_iterator<std::vector<int>::iterator>, std::default_sentinel_t> common_it;
   BOOST_MOVE_STATIC_ASSERT((is_same<iterator_traits<common_it>::value_type, int>::value));
   BOOST_MOVE_STATIC_ASSERT((is_same<iterator_traits<common_it>::difference_type, std::ptrdiff_t>::value));
   BOOST_MOVE_STATIC_ASSERT((is_same<boost::movelib::iter_value<common_it>::type, int>::value));
   BOOST_MOVE_STATIC_ASSERT((is_same<boost::movelib::iter_category<common_it>::type, std::iterator_traits<common_it>::iterator_category>::value));
   test_contiguous<common_it, false>();
   #endif

   //Standard iterators
   test_contiguous<std::vector<int>::iterator, is_cxx20>();
   test_contiguous<std::vector<int>::const_iterator, is_cxx20>();
   test_contiguous<std::string::iterator, is_cxx20>();
   test_contiguous<std::string::const_iterator, is_cxx20>();
   test_contiguous<std::vector<bool>::iterator, false>();
   test_contiguous<std::deque<int>::iterator, false>();
   test_contiguous<std::list<int>::iterator, false>();
   test_contiguous<std::reverse_iterator<int*>, false>();
   test_contiguous<std::istream_iterator<int>, false>();
   test_contiguous<boost::move_iterator<int*>, false>();

   //boost::movelib::contiguous_iterator_tag
   BOOST_MOVE_STATIC_ASSERT(boost::movelib::is_contiguous_iterator<boost_tag_concept_iterator>::value);
   BOOST_MOVE_STATIC_ASSERT(boost::movelib::is_contiguous_iterator<derived_boost_tag_concept_iterator>::value);
   BOOST_MOVE_STATIC_ASSERT(boost::movelib::is_contiguous_iterator<boost_tag_category_iterator>::value);
   BOOST_MOVE_STATIC_ASSERT((is_same<iterator_traits<int*>::iterator_concept, boost::movelib::contiguous_iterator_tag>::value));

   //std::contiguous_iterator_tag
   #if defined(BOOST_MOVE_CONTIGUOUS_ITERATOR_TAG)
   BOOST_MOVE_STATIC_ASSERT((is_same<boost::movelib::contiguous_iterator_tag, std::contiguous_iterator_tag>::value));
   BOOST_MOVE_STATIC_ASSERT(boost::movelib::is_contiguous_iterator<contiguous_concept_iterator>::value);
   BOOST_MOVE_STATIC_ASSERT(boost::movelib::is_contiguous_iterator<derived_concept_iterator>::value);
   BOOST_MOVE_STATIC_ASSERT(boost::movelib::is_contiguous_iterator<contiguous_category_iterator>::value);
   #endif

   return 0;
}
