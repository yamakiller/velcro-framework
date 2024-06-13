/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_TYPETRAITS_TYPE_IDENTITY_H
#define V_FRAMEWORK_CORE_STD_TYPETRAITS_TYPE_IDENTITY_H 1


namespace VStd {
    /// Implements the C++20 std::type_identity type trait
    /// Note type_identity can be used to block template argument deduction
    template <typename T>
    struct type_identity
    {
        using type = T;
    };

    template <typename T>
    using type_identity_t = typename type_identity<T>::type;
}

#endif // V_FRAMEWORK_CORE_STD_TYPETRAITS_TYPE_IDENTITY_H