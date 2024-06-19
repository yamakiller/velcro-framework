#ifndef V_FRAMEWORK_CORE_MATH_INTERNAL_SIMD_H
#define V_FRAMEWORK_CORE_MATH_INTERNAL_SIMD_H

#include <vcore/math/internal/math_type.h>

namespace V {
    namespace Simd {
#if   V_TRAIT_USE_PLATFORM_SIMD_SSE
            using FloatType = __m128;
            using Int32Type = __m128i;
            using FloatArgType = FloatType;
            using Int32ArgType = Int32Type;
#elif V_TRAIT_USE_PLATFORM_SIMD_NEON
            using FloatType = float32x4_t;
            using Int32Type = int32x4_t;
            using FloatArgType = FloatType;
            using Int32ArgType = Int32Type;
#elif V_TRAIT_USE_PLATFORM_SIMD_SCALAR
            using FloatType = struct { float   v[ElementCount]; };
            using Int32Type = struct { int32_t v[ElementCount]; };
            using FloatArgType = const FloatType&;
            using Int32ArgType = const Int32Type&;
#endif
    
        static Int32Type Not(Int32ArgType value);
        static Int32Type And(Int32ArgType arg1, Int32ArgType arg2);
        static Int32Type AndNot(Int32ArgType arg1, Int32ArgType arg2);
        static Int32Type Or(Int32ArgType arg1, Int32ArgType arg2);
        static Int32Type Xor(Int32ArgType arg1, Int32ArgType arg2);
    } // namespace Simd

} // namespace V

#if   V_TRAIT_USE_PLATFORM_SIMD_SSE
#   include <core/math/internal/simd_see.inl>
#elif V_TRAIT_USE_PLATFORM_SIMD_NEON
#   include <core/math/internal/simd_neon.inl>
#elif V_TRAIT_USE_PLATFORM_SIMD_SCALAR
#   include <core/math/internal/simd_scalar.inl>
#endif


#endif // V_FRAMEWORK_CORE_MATH_INTERNAL_SIMD_H