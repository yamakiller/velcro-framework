/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef V_FRAMEWORK_CORE_STD_TYPEINFO_H
#define V_FRAMEWORK_CORE_STD_TYPEINFO_H

#include <core/std/typetraits/static_storage.h>
#include <core/std/typetraits/is_pointer.h>
#include <core/std/typetraits/is_const.h>
#include <core/std/typetraits/is_enum.h>
#include <core/std/typetraits/is_base_of.h>
#include <core/std/typetraits/remove_pointer.h>
#include <core/std/typetraits/remove_const.h>
#include <core/std/typetraits/remove_reference.h>
#include <core/std/typetraits/has_member_function.h>
#include <core/std/typetraits/void_t.h>
#include <core/std/function/invoke.h>

#include <cstdio> // for snprintf

namespace VStd {
    template <class T>
    struct less;
    template <class T>
    struct less_equal;
    template <class T>
    struct greater;
    template <class T>
    struct greater_equal;
    template <class T>
    struct equal_to;
    template <class T>
    struct hash;
    template< class T1, class T2>
    struct pair;
    template< class T, class Allocator/* = VStd::allocator*/ >
    class vector;
    template< class T, VStd::size_t N >
    class array;
    template< class T, class Allocator/* = VStd::allocator*/ >
    class list;
    template< class T, class Allocator/* = VStd::allocator*/ >
    class forward_list;
    template<class Key, class MappedType, class Compare /*= VStd::less<Key>*/, class Allocator /*= VStd::allocator*/>
    class map;
    template<class Key, class MappedType, class Hasher /*= VStd::hash<Key>*/, class EqualKey /*= VStd::equal_to<Key>*/, class Allocator /*= VStd::allocator*/ >
    class unordered_map;
    template<class Key, class MappedType, class Hasher /* = VStd::hash<Key>*/, class EqualKey /* = VStd::equal_to<Key>*/, class Allocator /*= VStd::allocator*/ >
    class unordered_multimap;
    template <class Key, class Compare, class Allocator>
    class set;
    template<class Key, class Hasher /*= VStd::hash<Key>*/, class EqualKey /*= VStd::equal_to<Key>*/, class Allocator /*= VStd::allocator*/>
    class unordered_set;
    template<class Key, class Hasher /*= VStd::hash<Key>*/, class EqualKey /*= VStd::equal_to<Key>*/, class Allocator /*= VStd::allocator*/>
    class unordered_multiset;
    template<class T, size_t Capacity>
    class fixed_vector;
    template< class T, size_t NumberOfNodes>
    class fixed_list;
    template< class T, size_t NumberOfNodes>
    class fixed_forward_list;
    template<VStd::size_t NumBits>
    class bitset;

    template<class T>
    class intrusive_ptr;
    template<class T>
    class shared_ptr;

    template<class Element, class Traits, class Allocator>
    class basic_string;

    template<class Element>
    struct char_traits;

    template <class Element, class Traits>
    class basic_string_view;

    template <class Element, size_t MaxElementCount, class Traits>
    class basic_fixed_string;

    template<class Signature>
    class function;

    template<class T>
    class optional;

    struct monostate;

    template<class... Types>
    class variant;
}

#endif // V_FRAMEWORK_CORE_STD_TYPEINFO_H