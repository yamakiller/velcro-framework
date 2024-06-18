#include <core/math/internal/simd_common_sse.inl>

namespace V
{
    namespace Simd
    {
        V_MATH_INLINE FloatType LoadImmediate(float x, float y, float z, float w)
        {
            return Sse::LoadImmediate(x, y, z, w);
        }


        V_MATH_INLINE Int32Type LoadImmediate(int32_t x, int32_t y, int32_t z, int32_t w)
        {
            return Sse::LoadImmediate(x, y, z, w);
        }

        V_MATH_INLINE Int32Type LoadAligned(const int32_t* __restrict addr)
        {
            return Sse::LoadAligned(addr);
        }

        V_MATH_INLINE void StoreAligned(float* __restrict addr, FloatArgType value)
        {
            return Sse::StoreAligned(addr, value);
        }


        V_MATH_INLINE void StoreAligned(int32_t* __restrict addr, Int32ArgType value)
        {
            return Sse::StoreAligned(addr, value);
        }

        V_MATH_INLINE Int32Type Not(Int32ArgType value)
        {
            return Sse::Not(value);
        }

         V_MATH_INLINE Int32Type And(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::And(arg1, arg2);
        }


        V_MATH_INLINE Int32Type AndNot(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::AndNot(arg1, arg2);
        }


        V_MATH_INLINE Int32Type Or(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Or(arg1, arg2);
        }


        V_MATH_INLINE Int32Type Xor(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Xor(arg1, arg2);
        }
    }
}