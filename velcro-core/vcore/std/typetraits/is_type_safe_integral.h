#ifndef V_FRAMEWORK_CORE_STD_TYPETRAITS_IS_TYPE_SAFE_INTEGRAL_H
#define V_FRAMEWORK_CORE_STD_TYPETRAITS_IS_TYPE_SAFE_INTEGRAL_H

#include <vcore/std/typetraits/is_integral.h>
#include <vcore/std/typetraits/underlying_type.h>

namespace VStd
{
    template <typename TYPE>
    struct is_type_safe_integral
    {
        static constexpr bool value = false;
    };
}

#define V_TYPE_SAFE_INTEGRAL(TYPE_NAME, BASE_TYPE)                                                                                                   \
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

#define V_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(TYPE_NAME) \
    namespace VStd \
    { \
        template <> \
        struct is_type_safe_integral<TYPE_NAME> \
        { \
            static constexpr bool value = true; \
        }; \
    }

#endif // V_FRAMEWORK_CORE_STD_TYPETRAITS_IS_TYPE_SAFE_INTEGRAL_H