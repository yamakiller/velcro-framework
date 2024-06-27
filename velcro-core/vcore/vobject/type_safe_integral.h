#ifndef V_FRAMEWORK_CORE_RTTI_TYPE_SAFE_INTEGRAL_H
#define V_FRAMEWORK_CORE_RTTI_TYPE_SAFE_INTEGRAL_H

#include <stdint.h>
#include <vcore/base.h>
#include <vcore/std/typetraits/is_integral.h>
#include <vcore/std/typetraits/underlying_type.h>

namespace VStd {
    template <typename TYPE>
    struct is_type_safe_integral {
        static constexpr bool value = false;
    };
}

//! This implements a type-safe integral class.
//! The usage of an enum class prevents any potential negative performance impact that might occur with a wrapping struct.
//! Usage: VOBJECT_SAFE_INTEGRAL(TypeSafeClassName, int8_t/uint8_t/int16_t/uint16_t/int32_t/uint32_t/int64_t/uint64_t);
#define VOBJECT_SAFE_INTEGRAL(TYPE_NAME, BASE_TYPE)                                                                                                 \
    static_assert(VStd::is_integral<BASE_TYPE>::value, "Underlying type must be an integral");                                                     \
    enum class TYPE_NAME : BASE_TYPE {};                                                                                                            \
    inline constexpr TYPE_NAME operator+(TYPE_NAME lhs, TYPE_NAME rhs) { return TYPE_NAME{ BASE_TYPE((BASE_TYPE)lhs + (BASE_TYPE)rhs) }; }          \
    inline constexpr TYPE_NAME operator-(TYPE_NAME lhs, TYPE_NAME rhs) { return TYPE_NAME{ BASE_TYPE((BASE_TYPE)lhs - (BASE_TYPE)rhs) }; }          \
    inline constexpr TYPE_NAME operator*(TYPE_NAME lhs, TYPE_NAME rhs) { return TYPE_NAME{ BASE_TYPE((BASE_TYPE)lhs * (BASE_TYPE)rhs) }; }          \
    inline constexpr TYPE_NAME operator/(TYPE_NAME lhs, TYPE_NAME rhs) { return TYPE_NAME{ BASE_TYPE((BASE_TYPE)lhs / (BASE_TYPE)rhs) }; }          \
    inline constexpr TYPE_NAME operator%(TYPE_NAME lhs, TYPE_NAME rhs) { return TYPE_NAME{ BASE_TYPE((BASE_TYPE)lhs % (BASE_TYPE)rhs) }; }          \
    inline constexpr TYPE_NAME operator>>(TYPE_NAME value, int32_t shift) { return TYPE_NAME{ BASE_TYPE((BASE_TYPE)value >> shift) }; }             \
    inline constexpr TYPE_NAME operator<<(TYPE_NAME value, int32_t shift) { return TYPE_NAME{ BASE_TYPE((BASE_TYPE)value << shift) }; }             \
    inline constexpr TYPE_NAME& operator+=(TYPE_NAME& lhs, TYPE_NAME rhs) { return lhs = TYPE_NAME{ BASE_TYPE((BASE_TYPE)lhs + (BASE_TYPE)rhs) }; } \
    inline constexpr TYPE_NAME& operator-=(TYPE_NAME& lhs, TYPE_NAME rhs) { return lhs = TYPE_NAME{ BASE_TYPE((BASE_TYPE)lhs - (BASE_TYPE)rhs) }; } \
    inline constexpr TYPE_NAME& operator*=(TYPE_NAME& lhs, TYPE_NAME rhs) { return lhs = TYPE_NAME{ BASE_TYPE((BASE_TYPE)lhs * (BASE_TYPE)rhs) }; } \
    inline constexpr TYPE_NAME& operator/=(TYPE_NAME& lhs, TYPE_NAME rhs) { return lhs = TYPE_NAME{ BASE_TYPE((BASE_TYPE)lhs / (BASE_TYPE)rhs) }; } \
    inline constexpr TYPE_NAME& operator%=(TYPE_NAME& lhs, TYPE_NAME rhs) { return lhs = TYPE_NAME{ BASE_TYPE((BASE_TYPE)lhs % (BASE_TYPE)rhs) }; } \
    inline constexpr TYPE_NAME& operator>>=(TYPE_NAME& value, int32_t shift) { return value = TYPE_NAME{ BASE_TYPE((BASE_TYPE)value >> shift) }; }  \
    inline constexpr TYPE_NAME& operator<<=(TYPE_NAME& value, int32_t shift) { return value = TYPE_NAME{ BASE_TYPE((BASE_TYPE)value << shift) }; }  \
    inline constexpr TYPE_NAME& operator++(TYPE_NAME& value) { return value = TYPE_NAME{ BASE_TYPE((BASE_TYPE)value + 1) }; }                       \
    inline constexpr TYPE_NAME& operator--(TYPE_NAME& value) { return value = TYPE_NAME{ BASE_TYPE((BASE_TYPE)value - 1) }; }                       \
    inline constexpr TYPE_NAME operator++(TYPE_NAME& value, int) { const TYPE_NAME result = value; ++value; return result; }                        \
    inline constexpr TYPE_NAME operator--(TYPE_NAME& value, int) { const TYPE_NAME result = value; --value; return result; }                        \
    inline constexpr bool operator>(TYPE_NAME lhs, TYPE_NAME rhs) { return (BASE_TYPE)lhs > (BASE_TYPE)rhs; }                                       \
    inline constexpr bool operator<(TYPE_NAME lhs, TYPE_NAME rhs) { return (BASE_TYPE)lhs < (BASE_TYPE)rhs; }                                       \
    inline constexpr bool operator>=(TYPE_NAME lhs, TYPE_NAME rhs) { return (BASE_TYPE)lhs >= (BASE_TYPE)rhs; }                                     \
    inline constexpr bool operator<=(TYPE_NAME lhs, TYPE_NAME rhs) { return (BASE_TYPE)lhs <= (BASE_TYPE)rhs; }                                     \
    inline constexpr bool operator==(TYPE_NAME lhs, TYPE_NAME rhs) { return (BASE_TYPE)lhs == (BASE_TYPE)rhs; }                                     \
    inline constexpr bool operator!=(TYPE_NAME lhs, TYPE_NAME rhs) { return (BASE_TYPE)lhs != (BASE_TYPE)rhs; }                                     \
    V_DEFINE_ENUM_BITWISE_OPERATORS(TYPE_NAME)

//! This implements cvar binding methods for a type safe integral alias.
//! This must be placed in global scope (not in a namespace) and the TYPE_NAME should be fully qualified.
#define VOBJECT_SAFE_INTEGRAL_CVARBINDING(TYPE_NAME)                                                                                                \
    namespace V::ConsoleTypeHelpers                                                                                                                \
    {                                                                                                                                               \
        template <>                                                                                                                                 \
        inline CVarFixedString ValueToString<TYPE_NAME>(const TYPE_NAME& value)                                                                     \
        {                                                                                                                                           \
            return ConvertString(VStd::to_string(static_cast<VStd::underlying_type<TYPE_NAME>::type>(value)));                                    \
        }                                                                                                                                           \
                                                                                                                                                    \
        template <>                                                                                                                                 \
        inline bool StringSetToValue<TYPE_NAME>(TYPE_NAME& outValue, const AZ::ConsoleCommandContainer& arguments)                                  \
        {                                                                                                                                           \
            VStd::underlying_type<TYPE_NAME>::type underlyingValue;                                                                                \
            const bool result = StringSetToValue(underlyingValue, arguments);                                                                       \
            if (result)                                                                                                                             \
            {                                                                                                                                       \
                outValue = TYPE_NAME(underlyingValue);                                                                                              \
            }                                                                                                                                       \
            return result;                                                                                                                          \
        }                                                                                                                                           \
    }

#define VOBJECT_SAFE_INTEGRAL_SERIALIZEBINDING(TYPE_NAME) \
    namespace VStd \
    { \
        template <> \
        struct is_type_safe_integral<TYPE_NAME> \
        { \
            static constexpr bool value = true; \
        }; \
    }

#endif // V_FRAMEWORK_CORE_RTTI_TYPE_SAFE_INTEGRAL_H