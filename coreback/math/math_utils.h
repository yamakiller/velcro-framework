#ifndef V_FRAMEWORK_CORE_MATH_MATH_UTILS_H
#define V_FRAMEWORK_CORE_MATH_MATH_UTILS_H

#include <float.h>
#include <limits>
#include <math.h>
#include <utility>

#define V_MATH_INLINE V_FORCE_INLINE

// We have a separate macro for asserts within the math functions.
// Profile leaves asserts enabled, but in many cases we need highly performant profile builds (editor).
// This allows us to disable math asserts independently of asserts throughout the rest of the engine.
#ifdef V_DEBUG_BUILD
#   define V_MATH_ASSERT V_Assert
#else
#   define V_MATH_ASSERT(...)
#endif

namespace V {

    constexpr bool IsPowerOfTwo(uint32_t testValue) {
        return (testValue & (testValue - 1)) == 0;
    }

    template <uint32_t Alignment>
    V_MATH_INLINE bool IsAligned(const void* addr) {
        static_assert(IsPowerOfTwo(Alignment), "Alignment must be a power of two");
        return (reinterpret_cast<uintptr_t>(addr) & (Alignment - 1)) == 0;
    }

    constexpr uint32_t Log2(uint64_t maxValue) {
        uint32_t bits = 0;
        do
        {
            maxValue >>= 1;
            ++bits;
        } while (maxValue > 0);
        return bits;
    }
}


#endif // V_FRAMEWORK_CORE_MATH_MATH_UTILS_H