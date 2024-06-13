/*
 * Copyright (c) Contributors to the VelcroFramework Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef V_FRAMEWORK_CORE_STD_FUNCTION_IDENTITY_H
#define V_FRAMEWORK_CORE_STD_FUNCTION_IDENTITY_H

#include <utility>

namespace VStd {
    // Bring std::forward into the VStd namespace
    using std::forward;

    /// Implements the C++20 std::identity type trait
    /// Note: The member type is_transparent indicates to the caller that this function object is a transparent function object:
    /// it accepts arguments of arbitrary types and uses perfect forwarding, which avoids unnecessary copying and conversion
    /// when the function object is used in heterogeneous context, or with rvalue arguments.
    /// In particular, template functions such as std::set::find and std::set::lower_bound make use of this member type on their Compare types.
    struct identity {
        using is_transparent = void;

        template <typename T>
        constexpr T&& operator()(T&& t) const {
            return VStd::forward<T>(t);
        }
    };
}

#endif // V_FRAMEWORK_CORE_STD_FUNCTION_IDENTITY_H