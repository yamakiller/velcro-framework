/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef V_FRAMEWORK_CORE_STD_ITERATOR_H
#define V_FRAMEWORK_CORE_STD_ITERATOR_H

#include <vcore/std/base.h>
#include <vcore/std/typetraits/integral_constant.h>
#include <vcore/std/typetraits/void_t.h>
#include <vcore/std/typetraits/is_convertible.h>

#include <vcore/std/typetraits/is_base_of.h> // use by ConstIteratorCast
#include <vcore/std/typetraits/remove_cv.h>
#include <vcore/std/typetraits/is_reference.h>

#include <iterator>

namespace VStd {
    // Everything unless specified is based on C++ standard 24 (lib.iterators).

    /// Identifying tag for input iterators.
    using input_iterator_tag = std::input_iterator_tag;
    /// Identifying tag for output iterators.
    using output_iterator_tag = std::output_iterator_tag;
    /// Identifying tag for forward iterators.
    using forward_iterator_tag = std::forward_iterator_tag;
    /// Identifying tag for bidirectional iterators.
    using bidirectional_iterator_tag = std::bidirectional_iterator_tag;
    /// Identifying tag for random-access iterators.
    using random_access_iterator_tag = std::random_access_iterator_tag;
    /// Identifying tag for contagious iterators
    struct contiguous_iterator_tag
        : public random_access_iterator_tag {};
}

namespace VStd::Internal
{
    template <typename Iterator, typename = void>
    inline constexpr bool has_iterator_type_aliases_v = false;

    template <typename Iterator>
    inline constexpr bool has_iterator_type_aliases_v<Iterator, void_t<
        typename Iterator::iterator_category,
        typename Iterator::value_type,
        typename Iterator::difference_type,
        typename Iterator::pointer,
        typename Iterator::reference>
    > = true;


    template <typename Iterator, typename = void>
    inline constexpr bool has_iterator_category_v = false;
    template <typename Iterator>
    inline constexpr bool has_iterator_category_v<Iterator, VStd::void_t<typename Iterator::iterator_category>> = true;
    template <typename Iterator, typename = void>
    inline constexpr bool has_iterator_concept_v = false;
    template <typename Iterator>
    inline constexpr bool has_iterator_concept_v<Iterator, VStd::void_t<typename Iterator::iterator_concept>> = true;

    // Iterator iterator_category alias must be one of the iterator category tags
    template <typename Iterator, bool>
    struct iterator_traits_category_tags
    {};

    template <typename Iterator>
    struct iterator_traits_category_tags<Iterator, true> {
        using difference_type = typename Iterator::difference_type;
        using value_type = typename Iterator::value_type;
        using pointer = typename Iterator::pointer;
        using reference = typename Iterator::reference;
        using iterator_category = typename Iterator::iterator_category;
    };

    template <typename Iterator, bool>
    struct iterator_traits_type_aliases
    {};

    // Iterator must have all five type aliases
    template <typename Iterator>
    struct iterator_traits_type_aliases<Iterator, true>
        : iterator_traits_category_tags<Iterator,
        is_convertible_v<typename Iterator::iterator_category, input_iterator_tag>
        || is_convertible_v<typename Iterator::iterator_category, output_iterator_tag>>
    {};
}

namespace VStd {
    /**
     * Default iterator traits struct.
    */
    template <class Iterator>
    struct iterator_traits
        : Internal::iterator_traits_type_aliases<Iterator, Internal::has_iterator_type_aliases_v<Iterator>> {
    };

    /**
     * Default STL pointer specializations use random_access_iterator as a category.
     * We do refine this as being a continuous iterator.
     */
    template <class T>
    struct iterator_traits<T*> {
        using difference_type = ptrdiff_t;
        using value_type = remove_cv_t<T>;
        using pointer = T*;
        using reference = T&;
        using iterator_category = random_access_iterator_tag;
        using iterator_concept = contiguous_iterator_tag;
    };

} // namespace VStd

namespace VStd::Internal {
    // iterator_category tag testers
    template <typename Iterator, typename Category, bool = has_iterator_category_v<iterator_traits<Iterator>>>
    inline constexpr bool has_iterator_category_convertible_to_v = false;
    template <typename Iterator, typename Category>
    inline constexpr bool has_iterator_category_convertible_to_v<Iterator, Category, true> = is_convertible_v<typename iterator_traits<Iterator>::iterator_category, Category>;

    template <typename Iterator>
    inline constexpr bool is_input_iterator_v = has_iterator_category_convertible_to_v<Iterator, input_iterator_tag>;

    template <typename Iterator>
    inline constexpr bool is_forward_iterator_v = has_iterator_category_convertible_to_v<Iterator, forward_iterator_tag>;

    template <typename Iterator>
    inline constexpr bool is_bidirectional_iterator_v = has_iterator_category_convertible_to_v<Iterator, bidirectional_iterator_tag>;

    template <typename Iterator>
    inline constexpr bool is_random_access_iterator_v = has_iterator_category_convertible_to_v<Iterator, random_access_iterator_tag>;

    template <typename Iterator>
    inline constexpr bool is_contiguous_iterator_v = has_iterator_category_convertible_to_v<Iterator, contiguous_iterator_tag>;

    template <typename Iterator>
    inline constexpr bool is_exactly_input_iterator_v = has_iterator_category_convertible_to_v<Iterator, input_iterator_tag> && !has_iterator_category_convertible_to_v<Iterator, forward_iterator_tag>;

    // iterator concept testers
    template <typename Derived, typename Base>
    inline constexpr bool derived_from = is_base_of_v<Base, Derived> && is_convertible_v<const volatile Derived*, const volatile Base*>;

    template <typename Iterator, typename Concept, bool = has_iterator_concept_v<iterator_traits<Iterator>>>
    inline constexpr bool satisfies_iterator_concept = false;
    template <typename Iterator, typename Concept>
    inline constexpr bool satisfies_iterator_concept<Iterator, Concept, true> = derived_from<typename iterator_traits<Iterator>::iterator_concept, Concept>;
    template <typename Iterator>
    inline constexpr bool satisfies_contiguous_iterator_concept_v = satisfies_iterator_concept<Iterator, contiguous_iterator_tag>;
} // namespace VStd::Internal

namespace VStd {
    // Alias the C++ standard reverse_iterator into the VStd:: namespace
    using std::reverse_iterator;
    using std::make_reverse_iterator;

    // Alias the C++ standard move_iterator into the VStd:: namespace
    using std::move_iterator;
    using std::make_move_iterator;

    // Alias the C++ standard insert iterators into the VStd:: namespace
    using std::back_insert_iterator;
    using std::back_inserter;
    using std::front_insert_iterator;
    using std::front_inserter;
    using std::insert_iterator;
    using std::inserter;
  
    enum iterator_status_flag
    {
        isf_none = 0x00,     ///< Iterator is invalid.
        isf_valid = 0x01,     ///< Iterator point to a valid element in container, we can't dereference of all them. For instance the end() is a valid iterator, but you can't access it.
        isf_can_dereference = 0x02,     ///< We can dereference this iterator (it points to a valid element).
        isf_current = 0x04
    };

    //////////////////////////////////////////////////////////////////////////
    // Advance
     // Alias the std::advance function into the VStd namespace now that C++17 has made it constexpr
    using std::advance;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Distance functions.
    // Alias the std::distance function into the VStd namespace
    // It is constexpr as of C++17
    using std::distance;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // next prev utils
    // Alias the std::next and std::prev functions into the VStd namespace
    // Both functions are constexpr as of C++17
    using std::next;
    using std::prev;
    
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    //  (c)begin (c)end, (cr)begin, (cr)end utils
    // Alias all the begin and end iterator functions into the VStd namespacew
    // They are constexpr as of C++17
    using std::begin;
    using std::cbegin;
    using std::end;
    using std::cend;

    using std::rbegin;
    using std::crbegin;
    using std::rend;
    using std::crend;

    // Alias the C++17 size, empty and data function into the VStd namespace
    // All of the functions are constexpr
    using std::size;
    using std::empty;
    using std::data;
   
    namespace Debug {
        // Keep macros around for backwards compatibility
        #define VSTD_POINTER_ITERATOR_PARAMS(_Pointer)         _Pointer
        #define VSTD_CHECKED_ITERATOR(_IteratorImpl, _Param)    _Param
        #define VSTD_CHECKED_ITERATOR_2(_IteratorImpl, _Param1, _Param2) _Param1, _Param2
        #define VSTD_GET_ITER(_Iterator)                       _Iterator
    } // namespace Debug

    namespace Internal {
        // VStd const and non-const iterators are written to be binary compatible, i.e. iterator inherits from const_iterator and only add methods. This we "const casting"
        // between them is a matter of reinterpretation. In general avoid this cast as much as possible, as it's a bad practice, however it's true for all iterators.
        template<class Iterator, class ConstIterator>
        inline Iterator ConstIteratorCast(ConstIterator& iter)
        {
            static_assert((VStd::is_base_of<ConstIterator, Iterator>::value), "For this cast to work Iterator should derive from ConstIterator");
            static_assert(sizeof(ConstIterator) == sizeof(Iterator), "For this cast to work ConstIterator and Iterator should be binarily identical");
            return *reinterpret_cast<Iterator*>(&iter);
        }
    }

    using std::size;
    using std::empty;
    using std::data;
} // namespace VStd

#endif // V_FRAMEWORK_CORE_STD_ITERATOR_H