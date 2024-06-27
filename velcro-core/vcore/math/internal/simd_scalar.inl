   

namespace V
{
    namespace Simd
    {
        V_MATH_INLINE float32x4_t LoadImmediate(float x, float y, float z, float w)
        {
            FloatType result = {{ x, y, z, w }};
            return result;
        }

        V_MATH_INLINE Int32Type LoadImmediate(int32_t x, int32_t y, int32_t z, int32_t w)
        {
            Int32Type result = {{ x, y, z, w }};
            return result;
        }

        V_MATH_INLINE FloatType LoadAligned(const float* __restrict addr)
        {
            return LoadUnaligned(addr);
        }


        V_MATH_INLINE Int32Type LoadAligned(const int32_t* __restrict addr)
        {
            return LoadUnaligned(addr);
        }


        V_MATH_INLINE FloatType LoadUnaligned(const float* __restrict addr)
        {
            FloatType result = {{ addr[0], addr[1], addr[2], addr[3] }};
            return result;
        }


        V_MATH_INLINE Int32Type LoadUnaligned(const int32_t* __restrict addr)
        {
            Int32Type result = {{ addr[0], addr[1], addr[2], addr[3] }};
            return result;
        }

        V_MATH_INLINE void StoreAligned(float* __restrict addr, FloatArgType value)
        {
            return StoreUnaligned(addr, value);
        }


        V_MATH_INLINE void StoreAligned(int32_t* __restrict addr, Int32ArgType value)
        {
            return StoreUnaligned(addr, value);
        }


        V_MATH_INLINE void StoreUnaligned(float* __restrict addr, FloatArgType value)
        {
            addr[0] = value.v[0];
            addr[1] = value.v[1];
            addr[2] = value.v[2];
            addr[3] = value.v[3];
        }


        V_MATH_INLINE void StoreUnaligned(int32_t* __restrict addr, Int32ArgType value)
        {
            addr[0] = value.v[0];
            addr[1] = value.v[1];
            addr[2] = value.v[2];
            addr[3] = value.v[3];
        }


        V_MATH_INLINE Int32Type Not(Int32ArgType value)
        {
            return {{ ~value.v[0], ~value.v[1], ~value.v[2], ~value.v[3] }};
        }

        V_MATH_INLINE Int32Type And(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ arg1.v[0] & arg2.v[0], arg1.v[1] & arg2.v[1], arg1.v[2] & arg2.v[2], arg1.v[3] & arg2.v[3] }};
        }


        V_MATH_INLINE Int32Type AndNot(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ ~arg1.v[0] & arg2.v[0], ~arg1.v[1] & arg2.v[1], ~arg1.v[2] & arg2.v[2], ~arg1.v[3] & arg2.v[3] }};
        }


        V_MATH_INLINE Int32Type Or(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ arg1.v[0] | arg2.v[0], arg1.v[1] | arg2.v[1], arg1.v[2] | arg2.v[2], arg1.v[3] | arg2.v[3] }};
        }


        V_MATH_INLINE Int32Type Xor(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ arg1.v[0] ^ arg2.v[0], arg1.v[1] ^ arg2.v[1], arg1.v[2] ^ arg2.v[2], arg1.v[3] ^ arg2.v[3] }};
        }
    }
}