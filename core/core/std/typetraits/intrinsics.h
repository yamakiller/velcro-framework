/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_TYPETRAITS_IS_INTRINSICS_H
#define V_FRAMEWORK_CORE_STD_TYPETRAITS_IS_INTRINSICS_H

#include <core/std/typetraits/config.h>

#if defined(V_COMPILER_MSVC)
#   include <core/std/typetraits/is_same.h>

#   define VSTD_IS_UNION(T) __is_union(T)
#   define VSTD_IS_POD(T) (__is_pod(T) && __has_trivial_constructor(T))
#   define VSTD_IS_EMPTY(T) __is_empty(T)
#   define VSTD_HAS_TRIVIAL_CONSTRUCTOR(T) __has_trivial_constructor(T)
#   define VSTD_HAS_TRIVIAL_COPY(T) __has_trivial_copy(T)
#   define VSTD_HAS_TRIVIAL_ASSIGN(T) __has_trivial_assign(T)
#   define VSTD_HAS_TRIVIAL_DESTRUCTOR(T) __has_trivial_destructor(T)
#   define VSTD_HAS_NOTHROW_CONSTRUCTOR(T) __has_nothrow_constructor(T)
#   define VSTD_HAS_NOTHROW_COPY(T) __has_nothrow_copy(T)
#   define VSTD_HAS_NOTHROW_ASSIGN(T) __has_nothrow_assign(T)
#   define VSTD_HAS_VIRTUAL_DESTRUCTOR(T) __has_virtual_destructor(T)

#   define VSTD_IS_ABSTRACT(T) __is_abstract(T)
#   define VSTD_IS_BASE_OF(T, U) (__is_base_of(T, U) && !is_same<T, U>::value)
#   define VSTD_IS_CLASS(T) __is_class(T)
//  This one doesn't quite always do the right thing:
#   define VSTD_IS_CONVERTIBLE(_From, _To) __is_convertible_to(_From, _To)
#   define VSTD_IS_ENUM(T) __is_enum(T)
//  This one doesn't quite always do the right thing:
//  #   define VSTD_IS_POLYMORPHIC(T) __is_polymorphic(T)
#endif

#if defined(V_COMPILER_CLANG)
#   include <AzCore/std/typetraits/is_same.h>
#   include <AzCore/std/typetraits/is_reference.h>
#   include <AzCore/std/typetraits/is_volatile.h>

#   define VSTD_IS_UNION(T) __is_union(T)
#   define VSTD_IS_POD(T) __is_pod(T)
#   define VSTD_IS_EMPTY(T) __is_empty(T)
#   define VSTD_HAS_TRIVIAL_CONSTRUCTOR(T) __has_trivial_constructor(T)
//  This is claiming that unique_ptr is trivially copyable, which is incorrect.
//  #   define VSTD_HAS_TRIVIAL_COPY(T) (__has_trivial_copy(T) && !is_reference<T>::value)
#   define VSTD_HAS_TRIVIAL_ASSIGN(T) __has_trivial_assign(T)
#   define VSTD_HAS_TRIVIAL_DESTRUCTOR(T) __has_trivial_destructor(T)
#   define VSTD_HAS_NOTHROW_CONSTRUCTOR(T) __has_nothrow_constructor(T)
#   define VSTD_HAS_NOTHROW_COPY(T) (__has_nothrow_copy(T) && !is_volatile<T>::value && !is_reference<T>::value)
#   define VSTD_HAS_NOTHROW_ASSIGN(T) (__has_nothrow_assign(T) && !is_volatile<T>::value)
#   define VSTD_HAS_VIRTUAL_DESTRUCTOR(T) __has_virtual_destructor(T)

#   define VSTD_IS_ABSTRACT(T) __is_abstract(T)
#   define VSTD_IS_BASE_OF(T, U) (__is_base_of(T, U) && !is_same<T, U>::value)
#   define VSTD_IS_CLASS(T) __is_class(T)
#   define VSTD_IS_CONVERTIBLE(_From, _To) __is_convertible_to(_From, _To)
#   define VSTD_IS_ENUM(T) __is_enum(T)
#   define VSTD_IS_POLYMORPHIC(T) __is_polymorphic(T)
#endif

#ifndef VSTD_IS_UNION
#   define VSTD_IS_UNION(T) false
#endif

#ifndef VSTD_IS_POD
#   define VSTD_IS_POD(T) false
#endif

#ifndef VSTD_IS_EMPTY
#   define VSTD_IS_EMPTY(T) false
#endif

#ifndef VSTD_HAS_TRIVIAL_CONSTRUCTOR
#   define VSTD_HAS_TRIVIAL_CONSTRUCTOR(T) false
#endif

#ifndef VSTD_HAS_TRIVIAL_COPY
#   define VSTD_HAS_TRIVIAL_COPY(T) false
#endif

#ifndef VSTD_HAS_TRIVIAL_ASSIGN
#   define VSTD_HAS_TRIVIAL_ASSIGN(T) false
#endif

#ifndef VSTD_HAS_TRIVIAL_DESTRUCTOR
#   define VSTD_HAS_TRIVIAL_DESTRUCTOR(T) false
#endif

#ifndef VSTD_HAS_NOTHROW_CONSTRUCTOR
#   define VSTD_HAS_NOTHROW_CONSTRUCTOR(T) false
#endif

#ifndef VSTD_HAS_NOTHROW_COPY
#   define VSTD_HAS_NOTHROW_COPY(T) false
#endif

#ifndef VSTD_HAS_NOTHROW_ASSIGN
#   define VSTD_HAS_NOTHROW_ASSIGN(T) false
#endif

#ifndef VSTD_HAS_VIRTUAL_DESTRUCTOR
#   define VSTD_HAS_VIRTUAL_DESTRUCTOR(T) false
#endif

#endif // V_FRAMEWORK_CORE_STD_TYPETRAITS_IS_INTRINSICS_H