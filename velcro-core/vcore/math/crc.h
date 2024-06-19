#ifndef V_FRAMEWORK_CORE_MATH_CRC_H
#define V_FRAMEWORK_CORE_MATH_CRC_H

#include <vcore/base.h>
#include <vcore/std/hash.h>
#include <vcore/std/string/string_view.h>

//////////////////////////////////////////////////////////////////////////
// Macros for pre-processor Crc32 conversion
//
// When V_CRC("My string") is used by default it will map to V::Crc32("My string").
// We do have a pro-processor program which will precompute the crc for you and
// transform that macro to V_CRC("My string",0xabcdef00) this will expand to just 0xabcdef00.
// This will remove completely the "My string" from your executable, it will add it to a database and so on.
// WHen you want to update the string, just change the string.
// If you don't run the precompile step the code should still run fine, except it will be slower,
// the strings will be in the exe and converted on the fly and you can't use the result where you need
// a constant expression.
// For example
// switch(id) {
//  case V_CRC("My string",0xabcdef00): {} break; // this will compile fine
//  case V_CRC("My string"): {} break; // this will cause "error C2051: case expression not constant"
// }
// So it's you choice what you do, depending on your needs.
//
/// Implementation when we have only 1 param (by default it should be string.
#define V_CRC_1(_1)    V::Crc32(_1)
/// Implementation when we have 2 params (we use the 2 only)
#define V_CRC_2(_1, _2) V::Crc32(V::u32(_2))

#define V_CRC(...)     V_MACRO_SPECIALIZE(V_CRC_, V_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

//! V_CRC_CE macro is for enforcing a compile time evaluation of a Crc32 value
//! "CE" by the way stands for consteval, which is the C++20 term used to mark immediate functions
//! It can be used for any paramater that is convertible to either the uint32_t or string_view Crc constructor
//! It works by converting using the Crc32 constructor to create a temp Crc32 and then use the operator uint32_t
//! To convert the parameter into uint32_t. Since this code isn't using  C++20 yet only integral types can be used
//! as non-type-template parameters
//! An example of the use is as below
//! V::Crc32 testValue1 = V_CRC_CE("Test1");
//! V::Crc32 testValue2 = V_CRC_CE(0x43b3afd1);
#define V_CRC_CE(literal_) V::Internal::CompileTimeCrc32<static_cast<V::u32>(V::Crc32{ literal_ })>
//////////////////////////////////////////////////////////////////////////

namespace V {
    /**
     * Class for all of our crc32 types, better than just using ints everywhere.
     */
    class Crc32 {
    public:
        /**
         * Initializes to 0.
         */
        constexpr Crc32()
            : m_value(0)    {  }

        /**
         * Initializes from an int.
         */
        constexpr Crc32(V::u32 value) : m_value{ value } {}

        /**
         * Calculates the value from a string.
         */
        explicit constexpr Crc32(VStd::string_view view);

        /**
         * Calculates the value from a block of raw data.
         */
        Crc32(const void* data, size_t size, bool forceLowerCase = false);
        constexpr Crc32(const uint8_t* data, size_t size, bool forceLowerCase = false);
        constexpr Crc32(const char* data, size_t size, bool forceLowerCase = false);

        constexpr void Add(VStd::string_view view);
        void Add(const void* data, size_t size, bool forceLowerCase = false);
        constexpr void Add(const uint8_t* data, size_t size, bool forceLowerCase = false);
        constexpr void Add(const char* data, size_t size, bool forceLowerCase = false);

        constexpr operator u32() const               { return m_value; }

        constexpr bool operator==(Crc32 rhs) const   { return (m_value == rhs.m_value); }
        constexpr bool operator!=(Crc32 rhs) const   { return (m_value != rhs.m_value); }

        constexpr bool operator!() const             { return (m_value == 0); }


    protected:
        // A constant expression cannot contain a conversion from const-volatile void to any pointer to object type
        // nor can it contain a reinterpret_cast, therefore overloads for const char* and uint8_t are added
        void Set(const void* data, size_t size, bool forceLowerCase = false);
        constexpr void Set(const uint8_t* data, size_t size, bool forceLowerCase = false);
        constexpr void Set(const char* data, size_t size, bool forceLowerCase = false);
        constexpr void Combine(u32 crc, size_t len);
    protected:
        u32 m_value;
    };
}

namespace VStd {
    template<>
    struct hash<V::Crc32> {
        size_t operator()(const V::Crc32& id) const
        {
            VStd::hash<V::u32> hasher;
            return hasher(static_cast<V::u32>(id));
        }
    };
}

#include <vcore/math/crc.inl>

#endif // V_FRAMEWORK_CORE_MATH_CRC_H