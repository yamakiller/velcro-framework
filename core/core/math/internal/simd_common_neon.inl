
namespace V
{
    namespace Simd
    {
            V_MATH_INLINE float32x4_t LoadImmediate(float x, float y, float z, float w)
            {
                const float values[4] = { x, y, z, w };
                return vld1q_f32(values);
            }

            V_MATH_INLINE int32x4_t LoadImmediate(int32_t x, int32_t y, int32_t z, int32_t w)
            {
                const int32_t values[4] = { x, y, z, w };
                return vld1q_s32(values);
            }

            V_MATH_INLINE float32x4_t LoadAligned(const float* __restrict addr)
            {
                V_MATH_ASSERT(IsAligned<16>(addr), "Alignment failure");
                return vld1q_f32(addr);
            }

            V_MATH_INLINE int32x4_t LoadAligned(const int32_t* __restrict addr)
            {
                V_MATH_ASSERT(IsAligned<16>(addr), "Alignment failure");
                return vld1q_s32(addr);
            }

            V_MATH_INLINE void StoreAligned(float* __restrict addr, float32x4_t value)
            {
                V_MATH_ASSERT(IsAligned<16>(addr), "Alignment failure");
                vst1q_f32(addr, value);
            }

            V_MATH_INLINE void StoreAligned(int32_t* __restrict addr, int32x4_t value)
            {
                V_MATH_ASSERT(IsAligned<16>(addr), "Alignment failure");
                vst1q_s32(addr, value);
            }

            V_MATH_INLINE int32x4_t Not(int32x4_t value)
            {
                return vmvnq_s32(value);
            }

            V_MATH_INLINE int32x4_t And(int32x4_t arg1, int32x4_t arg2)
            {
                return vandq_s32(arg1, arg2);
            }

            V_MATH_INLINE int32x4_t AndNot(int32x4_t arg1, int32x4_t arg2)
            {
                return And(Not(arg1), arg2);
            }

            V_MATH_INLINE int32x4_t Or(int32x4_t arg1, int32x4_t arg2)
            {
                return vorrq_s32(arg1, arg2);
            }

            V_MATH_INLINE int32x4_t Xor(int32x4_t arg1, int32x4_t arg2)
            {
                return veorq_s32(arg1, arg2);
            }

    }
}