#include <vcore/math/internal/simd_common_neon.inl>

namespace V
{
    namespace Simd
    {
        V_MATH_INLINE FloatType LoadImmediate(float x, float y, float z, float w)
        {
            return NeonQuad::LoadImmediate(x, y, z, w);
        }

        V_MATH_INLINE Int32Type LoadImmediate(int32_t x, int32_t y, int32_t z, int32_t w)
        {
            return NeonQuad::LoadImmediate(x, y, z, w);
        }

        V_MATH_INLINE Vec4::FloatType Vec4::LoadAligned(const float* __restrict addr)
        {
            return NeonQuad::LoadAligned(addr);
        }

        V_MATH_INLINE Int32Type LoadAligned(const int32_t* __restrict addr)
        {
            return NeonQuad::LoadAligned(addr);
        }

        V_MATH_INLINE void Vec4::StoreAligned(float* __restrict addr, FloatArgType value)
        {
            NeonQuad::StoreAligned(addr, value);
        }

        V_MATH_INLINE void Vec4::StoreAligned(int32_t* __restrict addr, Int32ArgType value)
        {
            NeonQuad::StoreAligned(addr, value);
        }

        V_MATH_INLINE Int32Type Not(Int32ArgType value)
        {
            return NeonQuad::Not(value);
        }

        V_MATH_INLINE Int32Type And(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::And(arg1, arg2);
        }

        V_MATH_INLINE Int32Type AndNot(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::AndNot(arg1, arg2);
        }

        V_MATH_INLINE Int32Type Or(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::Or(arg1, arg2);
        }

        V_MATH_INLINE Int32Type Xor(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::Xor(arg1, arg2);
        }
    }
}