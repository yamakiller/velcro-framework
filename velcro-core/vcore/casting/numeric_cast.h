/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_CASTING_NUMERIC_CAST_H
#define V_FRAMEWORK_CORE_CASTING_NUMERIC_CAST_H

// This is disabled by default because it puts in costly runtime checking of casted values.
// You can either change it here to enable it across the engine, or use push/pop_macro to enable per file/feature.
// Note that if using push/pop_macro, you may get some of the functions not inline and the definition coming from
// another compilation unit, in such case, you will have to push/pop_macro on that compilation unit as well.
// #define V_NUMERICCAST_ENABLED 1

#if !V_NUMERICCAST_ENABLED

#define vnumeric_cast static_cast

#else

#include <vcore/casting/numeric_cast_internal.h>
#include <vcore/std/typetraits/is_arithmetic.h>
#include <vcore/std/typetraits/is_class.h>
#include <vcore/std/typetraits/is_enum.h>
#include <vcore/std/typetraits/is_floating_point.h>
#include <vcore/std/typetraits/is_integral.h>
#include <vcore/std/typetraits/is_signed.h>
#include <vcore/std/typetraits/remove_cvref.h>
#include <vcore/std/typetraits/underlying_type.h>
#include <vcore/std/utils.h>
#include <limits>

/*
// Numeric casts add range checking when casting from one numeric type to another. It adds run-time validation (if enabled for the
// particular build configuration) to ensure that no actual data loss happens. Assigning long long(17) to an unsigned char is allowed,
// but assigning char(-1) to unsigned long long variable is not, and will result in an assert or error if the validation has been
// enabled.
//
// Because we can't do partial function specialization, I'm using enable_if to chop up the implementation into one of these
// implementations. If none of these fit, then we will get a compile error because it is an unknown conversion.
//
//--------------------------------------------
//              TYPE <- TYPE         DigitLoss
// (A)     Integer      Unsigned         N
// (A)      Signed        Signed         N
// (B)    Unsigned        Signed         N
// (C)     Integer      Unsigned         Y
// (D)     Integer        Signed         Y
//
// (E)     Integer          Enum         -
// (F)     Integer      Floating         -
//
// (G)        Enum       Integer         -
//
// (H)    Floating       Integer         -
//
// (I)        Enum          Enum         -
//
// (J)    Floating      Floating         N
// (K)    Floating      Floating         Y
*/

#define V_NUMERIC_ASSERT(expr, ...) V_Assert(expr, __VA_ARGS__)

// INTEGER -> INTEGER
// (A) Not losing digits or risking sign loss
template <typename ToType, typename FromType>
inline constexpr typename VStd::enable_if<
    VStd::is_integral<FromType>::value&& VStd::is_integral<ToType>::value
    && std::numeric_limits<FromType>::digits <= std::numeric_limits<ToType>::digits
    && (!std::numeric_limits<FromType>::is_signed || std::numeric_limits<ToType>::is_signed)
    , ToType > ::type vnumeric_cast(FromType value)
{
    return static_cast<ToType>(value);
}

// (B) Not losing digits, but we are losing sign, so make sure we aren't dealing with a negative number
template <typename ToType, typename FromType>
inline constexpr typename VStd::enable_if<
    VStd::is_integral<FromType>::value&& VStd::is_integral<ToType>::value
    && std::numeric_limits<FromType>::digits <= std::numeric_limits<ToType>::digits
    && std::numeric_limits<FromType>::is_signed&& !std::numeric_limits<ToType>::is_signed
    , ToType > ::type vnumeric_cast(FromType value)
{
    V_NUMERIC_ASSERT(
        NumericCastInternal::FitsInToType<ToType>(value),
        "Attempted cast causes loss of signed value.");
    return static_cast<ToType>(value);
}


// (C) Maybe losing digits from an unsigned type, so make sure we don't exceed the destination max value. No check against zero is necessary.
template <typename ToType, typename FromType>
inline constexpr typename VStd::enable_if<
    VStd::is_integral<FromType>::value&& VStd::is_integral<ToType>::value
    && !(std::numeric_limits<FromType>::digits <= std::numeric_limits<ToType>::digits)
    && !std::numeric_limits<FromType>::is_signed
    , ToType > ::type vnumeric_cast(FromType value)
{
    V_NUMERIC_ASSERT(
        NumericCastInternal::FitsInToType<ToType>(value),
        "Attempted downcast of unsigned integer causes loss of high bits and type narrowing.");
    return static_cast<ToType>(value);
}

// (D) Maybe losing digits within signed types, we need to check both the min and max values.
template <typename ToType, typename FromType>
inline constexpr typename VStd::enable_if<
    VStd::is_integral<FromType>::value&& VStd::is_integral<ToType>::value
    && !(std::numeric_limits<FromType>::digits <= std::numeric_limits<ToType>::digits)
    && std::numeric_limits<FromType>::is_signed
    , ToType > ::type vnumeric_cast(FromType value)
{
    V_NUMERIC_ASSERT(
        NumericCastInternal::FitsInToType<ToType>(value),
        "Attempted downcast of signed integer causes loss of high bits and type narrowing.");
    return static_cast<ToType>(value);
}

// ENUMS -> INTEGER
// (E) handled by changing the enum to its underlying type
template <typename ToType, typename FromType>
inline constexpr typename VStd::enable_if<
    VStd::is_enum<FromType>::value&& VStd::is_integral<ToType>::value
    , ToType > ::type vnumeric_cast(FromType value)
{
    using UnderlyingFromType = typename VStd::underlying_type<FromType>::type;
    return vnumeric_cast<ToType>(static_cast<UnderlyingFromType>(value));
}

// FLOATING -> INTEGER
// (E) We'll accept precision loss as long as it stays in range
template <typename ToType, typename FromType>
inline constexpr typename VStd::enable_if<
    VStd::is_floating_point<FromType>::value&& VStd::is_integral<ToType>::value
    , ToType > ::type vnumeric_cast(FromType value)
{
    V_NUMERIC_ASSERT(
        NumericCastInternal::FitsInToType<ToType>(value),
        "Attempted cast of floating point value does not fit in the supplied type.");
    return static_cast<ToType>(value);
}

// INTEGER -> ENUM
// (G) We must cast to an enum so go through the backing type
template <typename ToType, typename FromType>
inline constexpr typename VStd::enable_if<
    VStd::is_integral<FromType>::value&& VStd::is_enum<ToType>::value
    , ToType > ::type vnumeric_cast(FromType value)
{
    using UnderlyingToType = typename VStd::underlying_type<ToType>::type;
    return static_cast<ToType>(vnumeric_cast<UnderlyingToType>(value));
}

// INTEGER -> FLOATING POINT
// (H) Perhaps some faster code substitutions could be done here instead of the standard int->float calls
template <typename ToType, typename FromType>
inline constexpr typename VStd::enable_if<
    VStd::is_integral<FromType>::value&& VStd::is_floating_point<ToType>::value
    , ToType > ::type vnumeric_cast(FromType value)
{
    return static_cast<ToType>(value);
}

// ENUM -> ENUM
// (I) crossing enums using the underlying type as the transport
template <typename ToType, typename FromType>
inline constexpr typename VStd::enable_if<
    VStd::is_enum<FromType>::value&& VStd::is_enum<ToType>::value
    , ToType > ::type vnumeric_cast(FromType value)
{
    using UnderlyingFromType = typename VStd::underlying_type<FromType>::type;
    using UnderlyingToType = typename VStd::underlying_type<ToType>::type;
    return static_cast<ToType>(vnumeric_cast<UnderlyingToType>(static_cast<UnderlyingFromType>(value)));
}

// FLOATING POINT -> FLOATING POINT
// (J) crossing floats with no digit loss
template <typename ToType, typename FromType>
inline constexpr typename VStd::enable_if<
    VStd::is_floating_point<FromType>::value&& VStd::is_floating_point<ToType>::value
    && std::numeric_limits<FromType>::digits <= std::numeric_limits<ToType>::digits
    , ToType > ::type vnumeric_cast(FromType value)
{
    return static_cast<ToType>(value);
}

// (K) crossing floats with digit loss
template <typename ToType, typename FromType>
inline constexpr typename VStd::enable_if<
    VStd::is_floating_point<FromType>::value&& VStd::is_floating_point<ToType>::value
    && !(std::numeric_limits<FromType>::digits <= std::numeric_limits<ToType>::digits)
    , ToType > ::type vnumeric_cast(FromType value)
{
    V_NUMERIC_ASSERT(
        NumericCastInternal::FitsInToType<ToType>(value),
        "Attempted cast of floating point value does not fit in the supplied type.");
    return static_cast<ToType>(value);
}

// (L) Support for types that implement a specific conversion operator FromType()
// This is used to forward the numeric_cast from a class type to arithmetic type
template <typename ToType, typename FromType>
inline constexpr auto vnumeric_cast(FromType&& value) ->
    VStd::enable_if_t<VStd::is_class_v<VStd::remove_cvref_t<FromType>> && VStd::is_arithmetic_v<ToType> && VStd::is_convertible_v<VStd::remove_cvref_t<FromType>, ToType>, ToType>
{
    return static_cast<ToType>(value);
}

#endif

// This is a helper class that lets us induce the destination type of a numeric cast
// It should never be directly used by anything other than vnumeric_caster.
namespace V {
    template <typename FromType>
    class NumericCasted {
    public:
        explicit constexpr NumericCasted(FromType value)
            : m_value(value) { }
        template <typename ToType>
        constexpr operator ToType() const { return vnumeric_cast<ToType>(m_value); }

    private:
        NumericCasted() = delete;
        void operator=(NumericCasted const&) = delete;

        FromType m_value;
    };
} // namespace V

// This is the primary function we should use when doing numeric casting, since it induces the
// type we need to cast to from the code rather than requiring an explicit coupling in the source.
template <typename FromType>
inline constexpr V::NumericCasted<FromType> vnumeric_caster(FromType value) {
    return V::NumericCasted<FromType>(value);
}

#endif // V_FRAMEWORK_CORE_CASTING_NUMERIC_CAST_H