/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_CASTING_NUMERIC_CAST_INTERNAL_H
#define V_FRAMEWORK_CORE_CASTING_NUMERIC_CAST_INTERNAL_H

#include <vcore/std/typetraits/conditional.h>
#include <vcore/std/typetraits/is_floating_point.h>
#include <vcore/std/typetraits/is_integral.h>
#include <vcore/std/typetraits/is_signed.h>
#include <vcore/std/typetraits/is_unsigned.h>
#include <limits>

#if defined(max)
    #undef max
#endif 

namespace NumericCastInternal {
    template<typename ToType, typename FromType>
    inline constexpr typename VStd::enable_if<!VStd::is_integral<FromType>::value || !VStd::is_floating_point<ToType>::value, bool>::type
    UnderflowsToType(const FromType& value) {
        return (value < static_cast<FromType>(std::numeric_limits<ToType>::lowest()));
    }

    template<typename ToType, typename FromType>
    inline constexpr typename VStd::enable_if<VStd::is_integral<FromType>::value && VStd::is_floating_point<ToType>::value, bool>::type
    UnderflowsToType(const FromType& value) {
        return (static_cast<ToType>(value) < std::numeric_limits<ToType>::lowest());
    }

    template<typename ToType, typename FromType>
    inline constexpr typename VStd::enable_if<!VStd::is_integral<FromType>::value || !VStd::is_floating_point<ToType>::value, bool>::type
    OverflowsToType(const FromType& value) {
        return (value > static_cast<FromType>(std::numeric_limits<ToType>::max()));
    }

    template<typename ToType, typename FromType>
    inline constexpr typename VStd::enable_if<VStd::is_integral<FromType>::value && VStd::is_floating_point<ToType>::value, bool>::type
    OverflowsToType(const FromType& value) {
        return (static_cast<ToType>(value) > std::numeric_limits<ToType>::max());
    }

    template<typename ToType, typename FromType>
    inline constexpr typename VStd::enable_if<
        VStd::is_integral<FromType>::value && VStd::is_integral<ToType>::value &&
            std::numeric_limits<FromType>::digits <= std::numeric_limits<ToType>::digits && VStd::is_signed<FromType>::value &&
            VStd::is_unsigned<ToType>::value,
        bool>::type
    FitsInToType(const FromType& value) {
        return !NumericCastInternal::UnderflowsToType<ToType>(value);
    }

    template<typename ToType, typename FromType>
    inline constexpr typename VStd::enable_if<
        VStd::is_integral<FromType>::value && VStd::is_integral<ToType>::value &&
            (std::numeric_limits<FromType>::digits > std::numeric_limits<ToType>::digits) && VStd::is_unsigned<FromType>::value,
        bool>::type
    FitsInToType(const FromType& value) {
        return !NumericCastInternal::OverflowsToType<ToType>(value);
    }

    template<typename ToType, typename FromType>
    inline constexpr typename VStd::enable_if<
        (!VStd::is_integral<FromType>::value || !VStd::is_integral<ToType>::value) ||
            ((std::numeric_limits<FromType>::digits <= std::numeric_limits<ToType>::digits) &&
             (VStd::is_unsigned<FromType>::value || VStd::is_signed<ToType>::value)) ||
            ((std::numeric_limits<FromType>::digits > std::numeric_limits<ToType>::digits) && VStd::is_signed<FromType>::value),
        bool>::type
    FitsInToType(const FromType& value) {
        return !NumericCastInternal::OverflowsToType<ToType>(value) && !NumericCastInternal::UnderflowsToType<ToType>(value);
    }

}

#endif // V_FRAMEWORK_CORE_CASTING_NUMERIC_CAST_INTERNAL_H