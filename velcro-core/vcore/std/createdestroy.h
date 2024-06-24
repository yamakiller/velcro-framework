/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef V_FRAMEWORK_CORE_STD_CREATEDESTROY_H
#define V_FRAMEWORK_CORE_STD_CREATEDESTROY_H

#include <vcore/std/iterator.h>
#include <vcore/std/typetraits/integral_constant.h>
#include <vcore/std/typetraits/is_array.h>
#include <vcore/std/typetraits/is_assignable.h>
#include <vcore/std/typetraits/is_constructible.h>
#include <vcore/std/typetraits/is_destructible.h>
#include <vcore/std/typetraits/is_function.h>
#include <vcore/std/typetraits/is_trivially_copyable.h>
#include <vcore/std/typetraits/is_void.h>
#include <vcore/std/utils.h>

namespace VStd {
    // alias std::pointer_traits into the VStd::namespace
    using std::pointer_traits;

    //! Bring the names of uninitialized_default_construct and
    //! uninitialized_default_construct_n into the VStd namespace
    using std::uninitialized_default_construct;
    using std::uninitialized_default_construct_n;

    //! uninitialized_value_construct and uninitialized_value_construct_n
    //! are now brought into scope of the VStd namespace
    using std::uninitialized_value_construct;
    using std::uninitialized_value_construct_n;
}

namespace VStd::Internal {
    template <typename T, typename = void>
    constexpr bool pointer_traits_has_to_address_v = false;
    template <typename T>
    constexpr bool pointer_traits_has_to_address_v<T, VStd::void_t<decltype(VStd::pointer_traits<T>::to_address(declval<const T&>()))>> = true;
} // namespace VStd::Internal

namespace VStd {
    //! Implements the C++20 to_address function
    //! This obtains the address represented by ptr without forming a reference
    //! to the pointee type
    template <typename  T>
    constexpr T* to_address(T* ptr) noexcept {
        static_assert(!VStd::is_function_v<T>, "Invoking to address on a function pointer is not allowed");
        return ptr;
    }
    //! Fancy pointer overload which delegates to using a specialization of pointer_traits<T>::to_address
    //! if that is a well-formed expression, otherwise it returns ptr->operator->()
    //! For example invoking `to_address(VStd::reverse_iterator<const char*>(char_ptr))`
    //! Returns an element of type const char*
    template <typename  T>
    constexpr auto to_address(const T& ptr) noexcept {
        if constexpr (VStd::Internal::pointer_traits_has_to_address_v<T>) {
            return pointer_traits<T>::to_address(ptr);
        }
 
        return VStd::to_address(ptr.operator->());
    }
}

namespace VStd::Internal
{
    /**
     * Internal code should be used only by AZSTD. These are as flat as possible (less function calls) implementation, that
     * should be fast and easy in debug. You can call this functions, but it's better to use the specialized one in the
     * algorithms.
     */
     //////////////////////////////////////////////////////////////////////////
     /**
      * Type has trivial destructor. We don't call it.
      */
    template <class InputIterator, class ValueType = typename iterator_traits<InputIterator>::value_type, bool = is_trivially_destructible_v<ValueType>>
    struct destroy
    {
        static constexpr void range(InputIterator first, InputIterator last) { (void)first; (void)last; }
        static constexpr void single(InputIterator iter) { (void)iter; }
    };
    /**
        * Calls the destructor on a range of objects, defined by start and end forward iterators.
        */
    template<class InputIterator, class ValueType>
    struct destroy<InputIterator, ValueType, false>
    {
        static constexpr void range(InputIterator first, InputIterator last)
        {
            for (; first != last; ++first)
            {
                first->~ValueType();
            }
        }

        static constexpr void single(InputIterator iter)
        {
            (void)iter;
            iter->~ValueType();
        }
    };
}

namespace VStd {
    //! Implements the C++20 version of destroy_at
    //! If the element type T is not an array type it invokes the destructor on that object
    //! Otherwise it recursively destructs the elements of the array as if by calling
    //! VStd::destroy(VStd::begin(*ptr), VStd::end(*ptr))
    template <typename T>
    constexpr void destroy_at(T* ptr) {
        if constexpr (VStd::is_array_v<T>) {
            for (auto& element : *ptr) {
                VStd::destroy_at(VStd::addressof(element));
            }
        } else {
            ptr->~T();
        }
    }
    //! Implements the C++17 destroy function which works on a range of elements
    //! The functions accepts a first and last iterator
    //! and invokes the destructor on all element in the iterator range
    template <typename ForwardIt>
    constexpr void destroy(ForwardIt first, ForwardIt last) {
        for (; first != last; ++first) {
            VStd::destroy_at(VStd::addressof(*first));
        }
    }

    //! Implements the C++17 destroy_n function
    //! The function accepts a forward iterator and a number of elements
    //! and invokes the destructor on all the elements in the range
    //! Returns an iterator past the last element destructed
    template <typename ForwardIt, size_t N>
    constexpr ForwardIt destroy_n(ForwardIt first, size_t numElements) {
        for (; numElements > 0; ++first, --numElements) {
            VStd::destroy_at(VStd::addressof(*first));
        }
        return first;
    }
}

namespace VStd::Internal {
    /**
     * Default object construction.
     */
    // placement new isn't a core constant expression therefore it cannot be used in a constexpr function
    template<class InputIterator, class ValueType = typename iterator_traits<InputIterator>::value_type,
        bool = is_trivially_constructible_v<ValueType>>
        struct construct
    {
        static constexpr void range(InputIterator first, InputIterator last) { (void)first; (void)last; }
        static constexpr void range(InputIterator first, InputIterator last, const ValueType& value)
        {
            for (; first != last; ++first)
            {
                *first = value;
            }
        }
        static constexpr void single(InputIterator iter) { (void)iter; }
        static constexpr void single(InputIterator iter, const ValueType& value)
        {
            *iter = value;
        }

        template<class ... InputArguments>
        static void single(InputIterator iter, InputArguments&& ... inputArguments)
        {
            ::new (static_cast<void*>(&*iter))ValueType(VStd::forward<InputArguments>(inputArguments) ...);
        }
    };

    template<class InputIterator, class ValueType>
    struct construct<InputIterator, ValueType, false>
    {
        static void range(InputIterator first, InputIterator last)
        {
            for (; first != last; ++first)
            {
                ::new (&*first) ValueType();
            }
        }

        static void range(InputIterator first, InputIterator last, const ValueType& value)
        {
            for (; first != last; ++first)
            {
                ::new (&*first) ValueType(value);
            }
        }

        static void single(InputIterator iter)
        {
            ::new (&*iter) ValueType();
        }

        template<class ... InputArguments>
        static void single(InputIterator iter, InputArguments&& ... inputArguments)
        {
            ::new (&*iter) ValueType(VStd::forward<InputArguments>(inputArguments) ...);
        }
    };
    //////////////////////////////////////////////////////////////////////////
}

namespace VStd
{
    //! C++20 implementation of construct_at
    //! Invokes placement new on the supplied address
    //! Constraints: Only available when the expression
    //! `new (declval<void*>()) T(declval<Args>()...)` is well-formed
    template <typename T, typename... Args>
    constexpr auto construct_at(T* ptr, Args&&... args)
        -> enable_if_t<is_void_v<void_t<decltype(new (declval<void*>()) T(VStd::forward<Args>(args)...))>>, T*>
    {
        return ::new (ptr) T(VStd::forward<Args>(args)...);
    }
}


namespace VStd::Internal
{

    //////////////////////////////////////////////////////////////////////////
    // Sequence copy. If we use optimized version we use memcpy.
    /**
    * Helper class to determine if we have apply fast copy. There are 2 conditions
    * - trivial copy ctor.
    * - all iterators satisfy the C++20 are contiguous iterator concept: pointers or iterator classes with
    *   the iterator_concept typedef set to contiguous_iterator_tag
    */
    template<class InputIterator, class ResultIterator>
    struct is_fast_copy_helper
    {
        using value_type = typename iterator_traits<ResultIterator>::value_type;
        static constexpr bool value = VStd::is_trivially_copyable_v<value_type>
            && Internal::satisfies_contiguous_iterator_concept_v<InputIterator>
            && Internal::satisfies_contiguous_iterator_concept_v<ResultIterator>;
    };

    // Use this trait to to determine copy mode, based on the iterator category and object copy properties,
    // Use it when when you call  uninitialized_copy, Internal::copy, Internal::move, etc.
    template< typename InputIterator, typename ResultIterator >
    struct is_fast_copy
        : public ::VStd::integral_constant<bool, ::VStd::Internal::is_fast_copy_helper<InputIterator, ResultIterator>::value> {};

    template <class InputIterator, class ForwardIterator>
    constexpr ForwardIterator copy(const InputIterator& first, const InputIterator& last, ForwardIterator result, const false_type& /* is_fast_copy<InputIterator,ForwardIterator>() */)
    {
        InputIterator iter(first);
        for (; iter != last; ++result, ++iter)
        {
            *result = *iter;
        }

        return result;
    }

    // Specialized copy for contiguous iterators (pointers) and trivial copy type.
    // This overload cannot be constexpr until builtin_memcpy is added to MSVC compilers
    template <class InputIterator, class ForwardIterator>
    inline ForwardIterator copy(const InputIterator& first, const InputIterator& last, ForwardIterator result, const true_type& /* is_fast_copy<InputIterator,ForwardIterator>() */)
    {
        // \todo Make sure memory ranges don't overlap, otherwise people should use move and move_backward.
        static_assert(sizeof(typename iterator_traits<InputIterator>::value_type) == sizeof(typename iterator_traits<ForwardIterator>::value_type), "Size of value types must match for a trivial copy");
        VStd::size_t numElements = last - first;
        if (numElements > 0)
        {
            V_Assert((static_cast<const void*>(&*result) < static_cast<const void*>(&*first)) || (static_cast<const void*>(&*result) >= static_cast<const void*>(&*first + numElements)), "VStd::copy memory overlaps use VStd::copy_backward!");
            V_Assert((static_cast<const void*>(&*result + numElements) <= static_cast<const void*>(&*first)) || (static_cast<const void*>(&*result + numElements) > static_cast<const void*>(&*first + numElements)), "VStd::copy memory overlaps use VStd::copy_backward!");
            /*AZSTD_STL::*/ memcpy(&*result, &*first, numElements * sizeof(typename iterator_traits<InputIterator>::value_type));
        }
        return result + numElements;
    }

    // Copy backward.
    template <class BidirectionalIterator1, class BidirectionalIterator2>
    constexpr BidirectionalIterator2 copy_backward(const BidirectionalIterator1& first, const BidirectionalIterator1& last, BidirectionalIterator2 result, const false_type& /* is_fast_copy<BidirectionalIterator1,BidirectionalIterator2>() */)
    {
        BidirectionalIterator1 iter(last);
        while (first != iter)
        {
            *--result = *--iter;
        }

        return result;
    }

    // Specialized copy for contiguous iterators (pointers) and trivial copy type.
    // This overload cannot be constexpr until builtin_memcpy is added to MSVC compilers
    template <class BidirectionalIterator1, class BidirectionalIterator2>
    inline BidirectionalIterator2 copy_backward(const BidirectionalIterator1& first, const BidirectionalIterator1& last, BidirectionalIterator2 result, const true_type& /* is_fast_copy<BidirectionalIterator1,BidirectionalIterator2>() */)
    {
        // \todo Make sure memory ranges don't overlap, otherwise people should use move and move_backward.
        static_assert(sizeof(typename iterator_traits<BidirectionalIterator1>::value_type) == sizeof(typename iterator_traits<BidirectionalIterator2>::value_type), "Size of value types must match for a trivial copy");
        VStd::size_t numElements = last - first;
        if (numElements > 0)
        {
            result -= numElements;
            V_Assert((&*result < &*first) || (&*result >= (&*first + numElements)), "VStd::copy_backward memory overlaps use VStd::copy!");
            V_Assert(((&*result + numElements) <= &*first) || ((&*result + numElements) > (&*first + numElements)), "VStd::copy_backward memory overlaps use VStd::copy!");
            /*VSTD_STL::*/ memcpy(&*result, &*first, numElements * sizeof(typename iterator_traits<BidirectionalIterator1>::value_type));
        }
        return result;
    }

    template <class BidirectionalIterator1, class ForwardIterator>
    constexpr ForwardIterator reverse_copy(const BidirectionalIterator1& first, const BidirectionalIterator1& last, ForwardIterator dest)
    {
        BidirectionalIterator1 iter(last);
        while (iter != first)
        {
            *(dest++) = *(--iter);
        }

        return dest;
    }
    //////////////////////////////////////////////////////////////////////////
}

namespace VStd
{
    /**
    * Specialized algorithms 20.4.4. We extend that by adding faster specialized versions when we have trivial assign type.
    */
    template <class InputIterator, class ForwardIterator>
    constexpr ForwardIterator uninitialized_copy(const InputIterator& first, const InputIterator& last, ForwardIterator result, const false_type& /* is_fast_copy<InputIterator,ForwardIterator>() */)
    {
        InputIterator iter(first);
        for (; iter != last; ++result, ++iter)
        {
            ::new (static_cast<void*>(&*result)) typename iterator_traits<ForwardIterator>::value_type(*iter);
        }

        return result;
    }

    // Specialized copy for contiguous iterators and trivial copy type.
    // This overload cannot be constexpr until builtin_memcpy is added to MSVC compilers
    template <class InputIterator, class ForwardIterator>
    inline ForwardIterator uninitialized_copy(const InputIterator& first, const InputIterator& last, ForwardIterator result, const true_type& /* is_fast_copy<InputIterator,ForwardIterator>() */)
    {
        static_assert(sizeof(typename iterator_traits<InputIterator>::value_type) == sizeof(typename iterator_traits<ForwardIterator>::value_type), "Value type sizes must match for a trivial copy");
        VStd::size_t numElements = last - first;
        if (numElements > 0)
        {
            /*AZSTD_STL::*/
            memcpy(&*result, &*first, numElements * sizeof(typename iterator_traits<InputIterator>::value_type));
        }
        return result + numElements;
    }

    template <class InputIterator, class ForwardIterator>
    constexpr ForwardIterator uninitialized_copy(const InputIterator& first, const InputIterator& last, ForwardIterator result)
    {
        return uninitialized_copy(first, last, result, Internal::is_fast_copy<InputIterator, ForwardIterator>());
    }

    // 25.3.1 Copy
    template<class InputIterator, class OutputIterator>
    constexpr OutputIterator copy(InputIterator first, InputIterator last, OutputIterator result)
    {
        return VStd::Internal::copy(first, last, result, VStd::Internal::is_fast_copy<InputIterator, OutputIterator>());
    }

    template <class BidirectionalIterator, class OutputIterator>
    constexpr OutputIterator reverse_copy(BidirectionalIterator first, BidirectionalIterator last, OutputIterator dest)
    {
        return VStd::Internal::reverse_copy(first, last, dest);
    }

    template<class BidirectionalIterator1, class BidirectionalIterator2>
    BidirectionalIterator2 copy_backward(BidirectionalIterator1 first, BidirectionalIterator1 last, BidirectionalIterator2 result)
    {
        return VStd::Internal::copy_backward(first, last, result, VStd::Internal::is_fast_copy<BidirectionalIterator1, BidirectionalIterator2>());
    }
}

namespace VStd::Internal
{
    //////////////////////////////////////////////////////////////////////////
    // Sequence move. If we use optimized version we use memmove.
    template <class InputIterator, class ForwardIterator>
    constexpr ForwardIterator move(const InputIterator& first, const InputIterator& last, ForwardIterator result, const false_type& /* is_fast_copy<InputIterator,ForwardIterator>() */)
    {
        InputIterator iter(first);
        for (; iter != last; ++result, ++iter)
        {
            *result = VStd::move(*iter);
        }
        return result;
    }

    // Specialized copy for contiguous iterators (pointers) and trivial copy type.
    // This overload cannot be constexpr until builtin_memmove is added to MSVC compilers
    template <class InputIterator, class ForwardIterator>
    inline ForwardIterator move(const InputIterator& first, const InputIterator& last, ForwardIterator result, const true_type& /* is_fast_copy<InputIterator,ForwardIterator>() */)
    {
        static_assert(sizeof(typename iterator_traits<InputIterator>::value_type) == sizeof(typename iterator_traits<ForwardIterator>::value_type), "Size of value types must match for a trivial copy");
        VStd::size_t numElements = last - first;
        if (numElements > 0)
        {
            /*AZSTD_STL::*/
            memmove(&*result, &*first, numElements * sizeof(typename iterator_traits<InputIterator>::value_type));
        }
        return result + numElements;
    }

    // For generic iterators, move is the same as copy.
    template <class BidirectionalIterator1, class BidirectionalIterator2>
    constexpr BidirectionalIterator2 move_backward(const BidirectionalIterator1& first, const BidirectionalIterator1& last, BidirectionalIterator2 result, const false_type& /* is_fast_copy<BidirectionalIterator1,BidirectionalIterator2>() */)
    {
        BidirectionalIterator1 iter(last);
        while (first != iter)
        {
            *--result = VStd::move(*--iter);
        }
        return result;
    }

    // Specialized copy for contiguous iterators (pointers) and trivial copy type.
    // This overload cannot be constexpr until builtin_memmove is added to MSVC compilers
    template <class BidirectionalIterator1, class BidirectionalIterator2>
    inline BidirectionalIterator2 move_backward(const BidirectionalIterator1& first, const BidirectionalIterator1& last, BidirectionalIterator2 result, const true_type& /* is_fast_copy<BidirectionalIterator1,BidirectionalIterator2>() */)
    {
        // \todo Make sure memory ranges don't overlap, otherwise people should use move and move_backward.
        static_assert(sizeof(typename iterator_traits<BidirectionalIterator1>::value_type) == sizeof(typename iterator_traits<BidirectionalIterator2>::value_type), "Size of value types must match for a trivial copy");
        VStd::size_t numElements = last - first;
        result -= numElements;
        if (numElements > 0)
        {
            /*AZSTD_STL::*/
            memmove(&*result, &*first, numElements * sizeof(typename iterator_traits<BidirectionalIterator1>::value_type));
        }
        return result;
    }

    template <class InputIterator, class ForwardIterator>
    constexpr ForwardIterator uninitialized_move(const InputIterator& first, const InputIterator& last, ForwardIterator result, const false_type& /* is_fast_copy<InputIterator,ForwardIterator>() */)
    {
        InputIterator iter(first);

        for (; iter != last; ++result, ++iter)
        {
            ::new (static_cast<void*>(&*result)) typename iterator_traits<ForwardIterator>::value_type(VStd::move(*iter));
        }
        return result;
    }
    // Specialized copy for contiguous iterators and trivial move type. (since the object is POD we will just perform a copy)
    // This overload cannot be constexpr until builtin_memcpy is added to MSVC compilers
    template <class InputIterator, class ForwardIterator>
    inline ForwardIterator uninitialized_move(const InputIterator& first, const InputIterator& last, ForwardIterator result, const true_type& /* is_fast_copy<InputIterator,ForwardIterator>() */)
    {
        static_assert(sizeof(typename iterator_traits<InputIterator>::value_type) == sizeof(typename iterator_traits<ForwardIterator>::value_type), "Value type sizes must match for a trivial copy");
        VStd::size_t numElements = last - first;
        if (numElements > 0)
        {
            /*AZSTD_STL::*/
            memcpy(&*result, &*first, numElements * sizeof(typename iterator_traits<InputIterator>::value_type));
        }
        return result + numElements;
    }

    // end of sequence move.
    //////////////////////////////////////////////////////////////////////////
}

namespace VStd
{
    //! Implements the C++17 uninitialized_move function
    //! The functions accepts two input iterators and an output iterator
    //! It performs an VStd::move on each in in the range of the input iterator 
    //! and stores the result in location pointed by the output iterator
    template <typename InputIt, typename ForwardIt>
    ForwardIt uninitialized_move(InputIt first, InputIt last, ForwardIt result)
    {
        return VStd::Internal::uninitialized_move(first, last, result, VStd::Internal::is_fast_copy<InputIt, InputIt>{});
    }
    // 25.3.2 Move
    template<class InputIterator, class OutputIterator>
    OutputIterator move(InputIterator first, InputIterator last, OutputIterator result)
    {
        return VStd::Internal::move(first, last, result, VStd::Internal::is_fast_copy<InputIterator, OutputIterator>());
    }

    template<class BidirectionalIterator1, class BidirectionalIterator2>
    BidirectionalIterator2 move_backward(BidirectionalIterator1 first, BidirectionalIterator1 last, BidirectionalIterator2 result)
    {
        return VStd::Internal::move_backward(first, last, result, VStd::Internal::is_fast_copy<BidirectionalIterator1, BidirectionalIterator2>());
    }
}

namespace VStd::Internal
{
    //////////////////////////////////////////////////////////////////////////
    // Fill
    /**
    * Helper class to determine if we have apply fast fill. There are 3 conditions
    * - trivial assign
    * - size of type == 1 (chars) to use memset
    * - contiguous iterators (pointers)
    */
    template<class Iterator>
    struct is_fast_fill_helper
    {
        using value_type = typename iterator_traits<Iterator>::value_type;
        constexpr static bool value = is_trivially_copy_assignable_v<value_type> && sizeof(value_type) == 1
            && Internal::satisfies_contiguous_iterator_concept_v<Iterator>;
    };

    // Use this trait to to determine fill mode, based on the iterator, value size, etc.
    // Use it when you call uninitialized_fill, uninitialized_fill_n, fill and fill_n.
    template< typename Iterator >
    struct is_fast_fill
        : public ::VStd::integral_constant<bool, ::VStd::Internal::is_fast_fill_helper<Iterator>::value>
    {};

    template <class ForwardIterator, class T>
    constexpr void fill(const ForwardIterator& first, const ForwardIterator& last, const T& value, const false_type& /* is_fast_fill<ForwardIterator>() */)
    {
        ForwardIterator iter(first);
        for (; iter != last; ++iter)
        {
            *iter = value;
        }
    }
    // Specialized version for character types where memset can be used
    // This overload cannot be constexpr until builtin_memset is added to MSVC compilers
    template <class ForwardIterator, class T>
    inline void fill(const ForwardIterator& first, const ForwardIterator& last, const T& value, const true_type& /* is_fast_fill<ForwardIterator>() */)
    {
        VStd::size_t numElements = last - first;
        if (numElements > 0)
        {
            /*AZSTD_STL::*/
            memset((void*)&*first, *reinterpret_cast<const unsigned char*>(&value), numElements);
        }
    }

    template <class ForwardIterator, class Size, class T>
    constexpr void fill_n(ForwardIterator first, Size numElements, const T& value, const false_type& /* is_fast_fill<ForwardIterator>() */)
    {
        for (; numElements--; ++first)
        {
            *first = value;
        }
    }

    // Specialized version for character types where memset can be used to perform the fill
    // This overload cannot be constexpr until builtin_memset is added to MSVC compilers
    template <class ForwardIterator, class Size, class T>
    inline void fill_n(ForwardIterator first, Size numElements, const T& value, const true_type& /* is_fast_fill<ForwardIterator>() */)
    {
        if (numElements > 0)
        {
            /*AZSTD_STL::*/
            memset(&*first, *reinterpret_cast<const unsigned char*>(&value), numElements);
        }
    }
}

namespace VStd
{
    template <class ForwardIterator, class T>
    constexpr void fill(const ForwardIterator& first, const ForwardIterator& last, const T& value)
    {
        Internal::fill(first, last, value, Internal::is_fast_fill<ForwardIterator>());
    }


    template <class ForwardIterator, class Size, class T>
    constexpr void fill_n(ForwardIterator first, Size numElements, const T& value)
    {
        Internal::fill_n(first, numElements, value, Internal::is_fast_fill<ForwardIterator>());
    }

    template <class ForwardIterator, class T>
    constexpr void uninitialized_fill(const ForwardIterator& first, const ForwardIterator& last, const T& value, const false_type& /* is_fast_fill<ForwardIterator>() */)
    {
        ForwardIterator iter(first);
        for (; iter != last; ++iter)
        {
            ::new (static_cast<void*>(&*iter)) typename iterator_traits<ForwardIterator>::value_type(value);
        }
    }

    // Specialized overload for types which meet the following criteria.
    // 1. Has it's iterator_traits<T>::iterator_concept type set to to contiguous_iterator_tag
    // 2. Is trivially assignable
    // 3. Has a sizeof(T) == 1
    // In such a case memset can be used to fill in the data
    // This overload cannot be constexpr until builtin_memset is added to MSVC compilers
    template <class ForwardIterator, class T>
    inline void uninitialized_fill(const ForwardIterator& first, const ForwardIterator& last, const T& value, const true_type& /* is_fast_fill<ForwardIterator>() */)
    {
        VStd::size_t numElements = last - first;
        if (numElements > 0)
        {
            /*AZSTD_STL::*/
            memset(&*first, *reinterpret_cast<const unsigned char*>(&value), numElements);
        }
    }

    template <class ForwardIterator, class Size, class T>
    constexpr void uninitialized_fill(ForwardIterator first, Size numElements, const T& value)
    {
        return uninitialized_fill(first, numElements, value, Internal::is_fast_fill<ForwardIterator>());
    }

    template <class ForwardIterator, class Size, class T>
    constexpr void uninitialized_fill_n(ForwardIterator first, Size numElements, const T& value, const false_type& /* is_fast_fill<ForwardIterator>() */)
    {
        for (; numElements--; ++first)
        {
            ::new (static_cast<void*>(&*first)) typename iterator_traits<ForwardIterator>::value_type(value);
        }
    }

    // Specialized overload for types which meet the following criteria.
    // 1. Has it's iterator_traits<T>::iterator_concept type set to to contiguous_iterator_tag
    // 2. Is trivially assignable
    // 3. Has a sizeof(T) == 1
    // In such a case memset can be used to fill in the data
    template <class ForwardIterator, class Size, class T>
    inline void uninitialized_fill_n(ForwardIterator first, Size numElements, const T& value, const true_type& /* is_fast_fill<ForwardIterator>() */)
    {
        if (numElements)
        {
            /*AZSTD_STL::*/
            memset(&*first, *reinterpret_cast<const unsigned char*>(&value), numElements);
        }
    }

    template <class ForwardIterator, class Size, class T>
    constexpr void uninitialized_fill_n(ForwardIterator first, Size numElements, const T& value)
    {
        return uninitialized_fill_n(first, numElements, value, Internal::is_fast_fill<ForwardIterator>());
    }
}

#endif // V_FRAMEWORK_CORE_STD_CREATEDESTROY_H