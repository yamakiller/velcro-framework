#include <vcore/math/math_intrinsics.h>

namespace V::IO::IStreamerTypes
{
    constexpr bool IsPowerOf2(V::u64 value)
    {
        return (value != 0) && ((value & (value - 1)) == 0);
    }

    constexpr bool IsAlignedTo(V::u64 value, V::u64 alignment)
    {
        V_Assert(IsPowerOf2(alignment), "Provided alignment is not a power of 2.");
        return (value & (alignment - 1)) == 0;
    }

    bool IsAlignedTo(void* address, V::u64 alignment)
    {
        return IsAlignedTo(reinterpret_cast<uintptr_t>(address), alignment);
    }

    V::u64 GetAlignment(V::u64 value)
    {
        return u64{ 1 } << v_ctz_u64(value);
    }

    V::u64 GetAlignment(void* address)
    {
        return GetAlignment(reinterpret_cast<uintptr_t>(address));
    }
}

constexpr V::u64 operator"" _kib(V::u64 value)
{
    return value * 1024;
}

constexpr V::u64 operator"" _mib(V::u64 value)
{
    return value * (1024 * 1024);
}

constexpr V::u64 operator"" _gib(V::u64 value)
{
    return value * (1024 * 1024 * 1024);
}
