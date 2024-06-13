/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_SMART_PTR_UNIAUE_PTR_H
#define V_FRAMEWORK_CORE_STD_SMART_PTR_UNIAUE_PTR_H

#include <core/std/smart_ptr/sp_convertible.h>
#include <core/std/typetraits/is_array.h>
#include <core/std/typetraits/is_pointer.h>
#include <core/std/typetraits/is_reference.h>
#include <core/std/typetraits/extent.h>
#include <core/std/typetraits/remove_extent.h>
#include <core/std/utils.h>
#include <core/memory/memory.h>
#include <memory>


namespace VStd {
    /// unique_ptr for single objects.
    template <typename T, typename Deleter = std::default_delete<T> >
    using unique_ptr = std::unique_ptr<T, Deleter>;

    template<class T>
    struct hash;

    template <typename T, typename Deleter>
    struct hash<unique_ptr<T, Deleter>>
    {
        typedef unique_ptr<T, Deleter> argument_type;
        typedef VStd::size_t result_type;
        inline result_type operator()(const argument_type& value) const
        {
            return std::hash<argument_type>()(value);
        }
    };

    template<typename T, typename... Args>
    VStd::enable_if_t<!VStd::is_array<T>::value && V::HasVClassAllocator<T>::value, unique_ptr<T>> make_unique(Args&&... args)
    {
        return VStd::unique_ptr<T>(aznew T(VStd::forward<Args>(args)...));
    }

    template<typename T, typename... Args>
    VStd::enable_if_t<!VStd::is_array<T>::value && !V::HasVClassAllocator<T>::value, unique_ptr<T>> make_unique(Args&&... args)
    {
        return VStd::unique_ptr<T>(new T(VStd::forward<Args>(args)...));
    }

    // The reason that there is not an array aznew version version of make_unique is because VClassAllocator does not support array new
    template<typename T>
    VStd::enable_if_t<VStd::is_array<T>::value && VStd::extent<T>::value == 0, unique_ptr<T>> make_unique(std::size_t size)
    {
        return VStd::unique_ptr<T>(new typename VStd::remove_extent<T>::type[size]());
    }

    template<typename T, typename... Args>
    VStd::enable_if_t<VStd::is_array<T>::value && VStd::extent<T>::value != 0, unique_ptr<T>> make_unique(Args&&... args) = delete;

} // namespace VStd


#endif // V_FRAMEWORK_CORE_STD_SMART_PTR_UNIAUE_PTR_H