/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_SMART_PTR_SP_CONVERTIBLE_H
#define V_FRAMEWORK_CORE_STD_SMART_PTR_SP_CONVERTIBLE_H

#include <vcore/std/base.h>

namespace VStd {
    namespace Internal {
        // sp_convertible
        template< class Y, class T >
        struct sp_convertible {
            typedef char (& yes) [1];
            typedef char (& no)  [2];

            static yes f(T*);
            static no  f(...);

            enum _vt {
                value = sizeof((f)(static_cast<Y*>(0))) == sizeof(yes)
            };
        };
        struct sp_empty {};

        template< bool >
        struct sp_enable_if_convertible_impl;
        template<>
        struct sp_enable_if_convertible_impl<true> {
            typedef sp_empty type;
        };
        template<>
        struct sp_enable_if_convertible_impl<false>  {};

        template< class Y, class T >
        struct sp_enable_if_convertible
            : public sp_enable_if_convertible_impl< sp_convertible< Y, T >::value >
        {};
    }
} // namespace VStd

#endif // V_FRAMEWORK_CORE_STD_SMART_PTR_SP_CONVERTIBLE_H