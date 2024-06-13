/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_SMART_PTR_CHECKED_DELETE_H
#define V_FRAMEWORK_CORE_STD_SMART_PTR_CHECKED_DELETE_H

#include <core/std/base.h>

namespace VStd {
    //////////////////////////////////////////////////////////////////////////
    // verify that types are complete for increased safety
    template<class T>
    inline void checked_delete(T* x) {
        // intentionally complex - simplification causes regressions
        typedef char type_must_be_complete[ sizeof(T) ? 1 : -1 ];
        (void) sizeof(type_must_be_complete);
        delete x;
    }

    template<class T>
    inline void checked_array_delete(T* x)
    {
        typedef char type_must_be_complete[ sizeof(T) ? 1 : -1 ];
        (void) sizeof(type_must_be_complete);
        delete [] x;
    }

    template<class T>
    struct checked_deleter
    {
        typedef void result_type;
        typedef T* argument_type;

        void operator()(T* x) const
        {
            VStd::checked_delete(x);
        }
    };

    template<class T>
    struct checked_array_deleter
    {
        typedef void result_type;
        typedef T* argument_type;

        void operator()(T* x) const
        {
            VStd::checked_array_delete(x);
        }
    };
} // namespace VStd

#endif // V_FRAMEWORK_CORE_STD_SMART_PTR_CHECKED_DELETE_H