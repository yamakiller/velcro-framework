/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_CASTING_LOSSY_CAST_H
#define V_FRAMEWORK_CORE_CASTING_LOSSY_CAST_H

#include <core/std/typetraits/conditional.h>
#include <core/std/typetraits/is_arithmetic.h>
#include <core/std/typetraits/is_enum.h>

/*
// Lossy casts are just a wrapper around static_cast, but indicate the *intent* that numeric data loss
// has been accounted for. This is only meant for lossy numeric casting, so expect compile errors if
// used with other types.
*/
template <typename ToType, typename FromType>
inline constexpr VStd::enable_if_t<
    (VStd::is_arithmetic<FromType>::value || VStd::is_enum<FromType>::value)
    && (VStd::is_arithmetic<ToType>::value || VStd::is_enum<ToType>::value)
    , ToType > vlossy_cast(FromType value)
{
    return static_cast<ToType>(value);
}

// This is a helper class that lets us induce the destination type of a lossy numeric cast.
// It should never be directly used by anything other than vlossy_caster.
namespace V {
    template <typename FromType>
    class LossyCasted
    {
    public:
        explicit constexpr LossyCasted(FromType value)
            : m_value(value) { }
        template <typename ToType>
        constexpr operator ToType() const { return vlossy_cast<ToType>(m_value); }

    private:
        LossyCasted() = delete;
        void operator=(LossyCasted const&) = delete;

        FromType m_value;
    };
}

// This is the primary function we should use when lossy casting, since it induces the type we need
// to cast to from the code rather than requiring an explicit coupling in the source.
template <typename FromType>
inline constexpr V::LossyCasted<FromType> vlossy_caster(FromType value)
{
    return V::LossyCasted<FromType>(value);
}


#endif // V_FRAMEWORK_CORE_CASTING_LOSSY_CAST_H