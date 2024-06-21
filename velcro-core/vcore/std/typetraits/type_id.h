/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_TYPETRAITS_TYPE_ID_H
#define V_FRAMEWORK_CORE_STD_TYPETRAITS_TYPE_ID_H 1

#include <vcore/std/base.h>

#if defined(V_COMPILER_MSVC)
// Some compilers have the type info even with rtti off. By default
// we don't expect people to run with rtti.
#define VSTD_TYPE_ID_SUPPORT
#endif

#ifdef VSTD_TYPE_ID_SUPPORT
//#include <typeinfo>

namespace VStd {
    #if defined(V_COMPILER_MSVC)
        typedef const ::type_info* type_id;
    #else 
        typedef const std::type_info* type_id;
    #endif
}

#define vtypeid(T) (&typeid(T))
#define vtypeid_cmp(A, B) (*A) == (*B)
#else // VSTD_TYPE_ID_SUPPORT

namespace VStd {
    typedef void*    type_id;
    template<class T>
    struct type_id_holder {
        static char v_;
    };
    template<class T>
    char   type_id_holder< T >::v_;
    template<class T>
    struct type_id_holder< T const >
        : type_id_holder< T >{};
    template<class T>
    struct    type_id_holder< T volatile >
        : type_id_holder< T >{};
    template<class T>
    struct    type_id_holder< T const volatile >
        : type_id_holder< T >{};
}

#define vtypeid(T) (&VStd::type_id_holder<T>::v_)
#define vtypeid_cmp(A, B) (A) == (B)

#endif //  VSTD_TYPE_ID_SUPPORT

#endif // V_FRAMEWORK_CORE_STD_TYPETRAITS_TYPE_ID_H