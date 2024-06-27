namespace V
{
    namespace Simd
    {
        namespace Sse
        {
            V_MATH_INLINE __m128 Splat(float value)
            {
                return _mm_set1_ps(value);
            }

            V_MATH_INLINE __m128i Splat(int32_t value)
            {
                return _mm_set1_epi32(value);
            }

            V_MATH_INLINE __m128 CastToFloat(__m128i value)
            {
                return _mm_castsi128_ps(value);
            }

            V_MATH_INLINE __m128i CastToInt(__m128 value)
            {
                return _mm_castps_si128(value);
            }

            V_MATH_INLINE __m128 LoadImmediate(float x, float y, float z, float w)
            {
                return _mm_set_ps(w, z, y, x);
            }

            V_MATH_INLINE __m128i LoadImmediate(int32_t x, int32_t y, int32_t z, int32_t w)
            {
                return _mm_set_epi32(w, z, y, x);
            }

            V_MATH_INLINE __m128 LoadAligned(const float* __restrict addr)
            {
                V_MATH_ASSERT(IsAligned<16>(addr), "Alignment failure");
                return _mm_load_ps(addr);
            }

            V_MATH_INLINE __m128i LoadAligned(const int32_t* __restrict addr)
            {
                V_MATH_ASSERT(IsAligned<16>(addr), "Alignment failure");
                return _mm_load_si128((const __m128i *)addr);
            }

            V_MATH_INLINE void StoreAligned(float* __restrict addr, __m128 value)
            {
                V_MATH_ASSERT(IsAligned<16>(addr), "Alignment failure");
                _mm_store_ps(addr, value);
            }


            V_MATH_INLINE void StoreAligned(int32_t* __restrict addr, __m128i value)
            {
                V_MATH_ASSERT(IsAligned<16>(addr), "Alignment failure");
                _mm_store_si128((__m128i *)addr, value);
            }

            V_MATH_INLINE __m128 Not(__m128 value)
            {
                const __m128i invert = Splat(static_cast<int32_t>(0xFFFFFFFF));
                return _mm_andnot_ps(value, CastToFloat(invert));
            }

            V_MATH_INLINE __m128i Not(__m128i value)
            {
                return CastToInt(Not(CastToFloat(value)));
            }


            V_MATH_INLINE __m128i And(__m128i arg1, __m128i arg2)
            {
                return _mm_and_si128(arg1, arg2);
            }


            V_MATH_INLINE __m128i AndNot(__m128i arg1, __m128i arg2)
            {
                return _mm_andnot_si128(arg1, arg2);
            }


            V_MATH_INLINE __m128i Or(__m128i arg1, __m128i arg2)
            {
                return _mm_or_si128(arg1, arg2);
            }


            V_MATH_INLINE __m128i Xor(__m128i arg1, __m128i arg2)
            {
                return _mm_xor_si128(arg1, arg2);
            }
        } // namespace Sse
    } // namespace Simd
} // namespace V
