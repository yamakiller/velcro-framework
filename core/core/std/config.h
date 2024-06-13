/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef VSTD_CONFIG_H
#define VSTD_CONFIG_H 1

#ifndef VSTD_CONTAINER_ASSERT
    #define VSTD_CONTAINER_ASSERT V_Assert
#endif

/// VSTD_CHECKED_ITERATORS
#ifndef VSTD_CHECKED_ITERATORS
    #define VSTD_CHECKED_ITERATORS 0
#endif

#if ((defined(V_DEBUG_BUILD) && (VSTD_CHECKED_ITERATORS == 1)) || (VSTD_CHECKED_ITERATORS == 2))
    #define VSTD_HAS_CHECKED_ITERATORS
#endif

/// VSTD_NO_DEFAULT_STD_INCLUDES
/// use <new> and <string.h>
/// new, memcopy, memset
#ifndef VSTD_NO_DEFAULT_STD_INCLUDES
    #include <new>
    #include <string.h>
    #include <stdlib.h>
#endif

/// VSTD_NO_CHECKED_ITERATORS_IN_MULTI_THREADS
/// 如果您不想在不同线程中使用容器并启用 VSTD_CHECKED_ITERATORS, 则必须定义此定义.
/// 默认情况下, 当启用 VSTD_HAS_CHECKED_ITERATORS 时 Windows 会启用此功能.这仅用于调试,您仍然需要自己进行容器访问同步或使用提供的同步类, 
/// 如 concurent_queue、concurent_prioriry_queue 等
#if !defined(VSTD_NO_CHECKED_ITERATORS_IN_MULTI_THREADS) && defined(VSTD_HAS_CHECKED_ITERATORS)
#define VSTD_CHECKED_ITERATORS_IN_MULTI_THREADS
#endif

#ifdef VSTD_DOXYGEN_INVOKED
// for documentation only
    #define VStd::allocator
    #define VSTD_NO_DEFAULT_STD_INCLUDES
    #define VSTD_NO_PARALLEL_SUPPORT
    #define VSTD_NO_CHECKED_ITERATORS_IN_MULTI_THREADS
#endif

#ifndef VSTD_STL
    #if defined(V_COMPILER_MSVC)
        #define VSTD_STL std
    #else
        #define VSTD_STL
    #endif
#endif

#if !defined(V_USE_AUTO_PTR)
#   define V_NO_AUTO_PTR
#endif

/// VSTD_ADL_FIX_FUNCTION_SPEC_1_2(iter_swap, VStd::vector<int>::iterator&);
#define VSTD_ADL_FIX_FUNCTION_SPEC_1(_Function, _Param) \
    V_FORCE_INLINE void _Function(_Param a)        { VStd::_Function(a); }
#define VSTD_ADL_FIX_FUNCTION_SPEC_1_RET(_Function, _Return, _Param) \
    V_FORCE_INLINE _Return _Function(_Param a)             { return VStd::_Function(a); }
#define VSTD_ADL_FIX_FUNCTION_SPEC_1_2(_Function, _Param) \
    V_FORCE_INLINE void _Function(_Param a, _Param b)       { VStd::_Function(a, b); }
#define VSTD_ADL_FIX_FUNCTION_SPEC_1_2_RET(_Function, _Return, _Param) \
    V_FORCE_INLINE _Return _Function(_Param a, _Param b)        { return VStd::_Function(a, b); }
#define VSTD_ADL_FIX_FUNCTION_SPEC_2(_Function, _Param1, _Param2) \
    V_FORCE_INLINE void _Function(_Param1 a, _Param2 b)     { VStd::_Function(a, b); }
#define VSTD_ADL_FIX_FUNCTION_SPEC_2_RET(_Function, _Return, _Param1, _Param2) \
    V_FORCE_INLINE _Return _Function(_Param1 a, _Param2 b)  { return VStd::_Function(a, b); }
#define VSTD_ADL_FIX_FUNCTION_SPEC_2_3(_Function, _Param1, _Param2) \
    V_FORCE_INLINE void _Function(_Param1 a, _Param1 b, _Param2 c)   { VStd::_Function(a, b, c); }
#define VSTD_ADL_FIX_FUNCTION_SPEC_2_3_RET(_Function, _Return, _Param1, _Param2) \
    V_FORCE_INLINE _Return _Function(_Param1 a, _Param1 b, _Param2 c)    { return VStd::_Function(a, b, c); }
#define VSTD_ADL_FIX_FUNCTION_SPEC_2_4(_Function, _Param1, _Param2) \
    V_FORCE_INLINE void _Function(_Param1 a, _Param1 b, _Param1 c, _Param2 d) { VStd::_Function(a, b, c, d); }
#define VSTD_ADL_FIX_FUNCTION_SPEC_2_4_RET(_Function, _Return, _Param1, _Param2) \
    V_FORCE_INLINE _Return _Function(_Param1 a, _Param1 b, _Param1 c, _Param2 d)  { return VStd::_Function(a, b, c, d); }

#endif // VSTD_CONFIG_H