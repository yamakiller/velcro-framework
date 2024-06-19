#include <vcore/math/sfmt.h>

#include <vcore/math/random.h>
#include <vcore/std/parallel/lock.h>
#include <vcore/module/environment.h>

#include <string.h> // for memset


namespace V {
    namespace SfmtInternal {
        static const int N32    = N * 4;
        static const int N64    = N * 2;
        static const int POS1   = 122;
        static const int SL1    = 18;
        static const int SR1    = 11;
        static const int SL2    = 1;
        static const int SR2    = 1;
        static const unsigned int MSK1  = 0xdfffffefU;
        static const unsigned int MSK2  = 0xddfecb7fU;
        static const unsigned int MSK3  = 0xbffaffffU;
        static const unsigned int MSK4  = 0xbffffff6U;
        static const unsigned int PARITY1   = 0x00000001U;
        static const unsigned int PARITY2   = 0x00000000U;
        static const unsigned int PARITY3   = 0x00000000U;
        static const unsigned int PARITY4   = 0x13c9e684U;

        /** a parity check vector which certificate the period of 2^{MEXP} */
        static unsigned int parity[4] = {PARITY1, PARITY2, PARITY3, PARITY4};

#ifdef ONLY64
#   define idxof(_i) (_i ^ 1)
#else
#   define idxof(_i) _i
#endif // ONLY64


#if V_TRAIT_USE_PLATFORM_SIMD_SSE
        /**
         * This function represents the recursion formula.
         * @param a a 128-bit part of the internal state array
         * @param b a 128-bit part of the internal state array
         * @param c a 128-bit part of the internal state array
         * @param d a 128-bit part of the internal state array
         * @param mask 128-bit mask
         * @return output
         */
        V_FORCE_INLINE static Simd::Int32Type simd_recursion(Simd::Int32Type* a, Simd::Int32Type* b, Simd::Int32Type c, Simd::Int32Type d, Simd::Int32Type mask)
        {
            Simd::Int32Type v, x, y, z;
            x = *a;
            y = _mm_srli_epi32(*b, SR1);
            z = _mm_srli_si128(c, SR2);
            v = _mm_slli_epi32(d, SL1);
            z = Simd::Xor(z, x);
            z = Simd::Xor(z, v);
            x = _mm_slli_si128(x, SL2);
            y = Simd::And(y, mask);
            z = Simd::Xor(z, x);
            z = Simd::Xor(z, y);
            return z;
        }

        /**
         * This function fills the internal state array with pseudorandom
         * integers.
         */
        inline void gen_rand_all(Sfmt& g)
        {
            int i;
            Simd::Int32Type r, r1, r2, mask;
            mask = Simd::LoadImmediate((int32_t)MSK4, (int32_t)MSK3, (int32_t)MSK2, (int32_t)MSK1);

            r1 = Simd::LoadAligned((const int32_t*)&g.m_sfmt[N - 2].si);
            r2 = Simd::LoadAligned((const int32_t*)&g.m_sfmt[N - 1].si);
            for (i = 0; i < N - POS1; i++)
            {
                r = simd_recursion(&g.m_sfmt[i].si, &g.m_sfmt[i + POS1].si, r1, r2, mask);
                Simd::StoreAligned((int32_t*)&g.m_sfmt[i].si, r);
                r1 = r2;
                r2 = r;
            }
            for (; i < N; i++)
            {
                r = simd_recursion(&g.m_sfmt[i].si, &g.m_sfmt[i + POS1 - N].si, r1, r2, mask);
                Simd::StoreAligned((int32_t*)&g.m_sfmt[i].si, r);
                r1 = r2;
                r2 = r;
            }
        }

        /**
         * This function fills the user-specified array with pseudorandom
         * integers.
         *
         * @param array an 128-bit array to be filled by pseudorandom numbers.
         * @param size number of 128-bit pesudorandom numbers to be generated.
         */
        inline void gen_rand_array(Sfmt& g, w128_t* array, int size)
        {
            int i, j;
            Simd::Int32Type r, r1, r2, mask;
            mask = Simd::LoadImmediate((int32_t)MSK4, (int32_t)MSK3, (int32_t)MSK2, (int32_t)MSK1);

            r1 = Simd::LoadAligned((const int32_t*)&g.m_sfmt[N - 2].si);
            r2 = Simd::LoadAligned((const int32_t*)&g.m_sfmt[N - 1].si);
            for (i = 0; i < N - POS1; i++)
            {
                r = simd_recursion(&g.m_sfmt[i].si, &g.m_sfmt[i + POS1].si, r1, r2, mask);
                Simd::StoreAligned((int32_t*)&array[i].si, r);
                r1 = r2;
                r2 = r;
            }
            for (; i < N; i++)
            {
                r = simd_recursion(&g.m_sfmt[i].si, &array[i + POS1 - N].si, r1, r2, mask);
                Simd::StoreAligned((int32_t*)&array[i].si, r);
                r1 = r2;
                r2 = r;
            }
            /* main loop */
            for (; i < size - N; i++)
            {
                r = simd_recursion(&array[i - N].si, &array[i + POS1 - N].si, r1, r2, mask);
                Simd::StoreAligned((int32_t*)&array[i].si, r);
                r1 = r2;
                r2 = r;
            }
            for (j = 0; j < 2 * N - size; j++)
            {
                r = Simd::LoadAligned((const int32_t*)&array[j + size - N].si);
                Simd::StoreAligned((int32_t*)&g.m_sfmt[j].si, r);
            }
            for (; i < size; i++)
            {
                r = simd_recursion(&array[i - N].si, &array[i + POS1 - N].si, r1, r2, mask);
                Simd::StoreAligned((int32_t*)&array[i].si, r);
                Simd::StoreAligned((int32_t*)&g.m_sfmt[j++].si, r);
                r1 = r2;
                r2 = r;
            }
        }
#else
        inline void rshift128(w128_t* out, w128_t const* in, int shift)
        {
            V::u64 th, tl, oh, ol;
     #ifdef ONLY64
            th = ((V::u64)in->u[2] << 32) | ((V::u64)in->u[3]);
            tl = ((V::u64)in->u[0] << 32) | ((V::u64)in->u[1]);

            oh = th >> (shift * 8);
            ol = tl >> (shift * 8);
            ol |= th << (64 - shift * 8);
            out->u[0] = (V::u32)(ol >> 32);
            out->u[1] = (V::u32)ol;
            out->u[2] = (V::u32)(oh >> 32);
            out->u[3] = (V::u32)oh;
    #else
            th = ((V::u64)in->u[3] << 32) | ((V::u64)in->u[2]);
            tl = ((V::u64)in->u[1] << 32) | ((V::u64)in->u[0]);

            oh = th >> (shift * 8);
            ol = tl >> (shift * 8);
            ol |= th << (64 - shift * 8);
            out->u[1] = (V::u32)(ol >> 32);
            out->u[0] = (V::u32)ol;
            out->u[3] = (V::u32)(oh >> 32);
            out->u[2] = (V::u32)oh;
    #endif
        }

        inline void lshift128(w128_t* out, w128_t const* in, int shift)
        {
            V::u64 th, tl, oh, ol;
    #ifdef ONLY64
            th = ((V::u64)in->u[2] << 32) | ((V::u64)in->u[3]);
            tl = ((V::u64)in->u[0] << 32) | ((V::u64)in->u[1]);

            oh = th << (shift * 8);
            ol = tl << (shift * 8);
            oh |= tl >> (64 - shift * 8);
            out->u[0] = (V::u32)(ol >> 32);
            out->u[1] = (V::u32)ol;
            out->u[2] = (V::u32)(oh >> 32);
            out->u[3] = (V::u32)oh;
    #else
            th = ((V::u64)in->u[3] << 32) | ((V::u64)in->u[2]);
            tl = ((V::u64)in->u[1] << 32) | ((V::u64)in->u[0]);

            oh = th << (shift * 8);
            ol = tl << (shift * 8);
            oh |= tl >> (64 - shift * 8);
            out->u[1] = (V::u32)(ol >> 32);
            out->u[0] = (V::u32)ol;
            out->u[3] = (V::u32)(oh >> 32);
            out->u[2] = (V::u32)oh;
    #endif
        }

        inline void do_recursion(w128_t* r, w128_t* a, w128_t* b, w128_t* c,    w128_t* d)
        {
            w128_t x;
            w128_t y;
            lshift128(&x, a, SL2);
            rshift128(&y, c, SR2);
    #ifdef ONLY64
            r->u[0] = a->u[0] ^ x.u[0] ^ ((b->u[0] >> SR1) & MSK2) ^ y.u[0] ^ (d->u[0] << SL1);
            r->u[1] = a->u[1] ^ x.u[1] ^ ((b->u[1] >> SR1) & MSK1) ^ y.u[1] ^ (d->u[1] << SL1);
            r->u[2] = a->u[2] ^ x.u[2] ^ ((b->u[2] >> SR1) & MSK4) ^ y.u[2] ^ (d->u[2] << SL1);
            r->u[3] = a->u[3] ^ x.u[3] ^ ((b->u[3] >> SR1) & MSK3) ^ y.u[3] ^ (d->u[3] << SL1);
    #else
            r->u[0] = a->u[0] ^ x.u[0] ^ ((b->u[0] >> SR1) & MSK1) ^ y.u[0] ^ (d->u[0] << SL1);
            r->u[1] = a->u[1] ^ x.u[1] ^ ((b->u[1] >> SR1) & MSK2) ^ y.u[1] ^ (d->u[1] << SL1);
            r->u[2] = a->u[2] ^ x.u[2] ^ ((b->u[2] >> SR1) & MSK3) ^ y.u[2] ^ (d->u[2] << SL1);
            r->u[3] = a->u[3] ^ x.u[3] ^ ((b->u[3] >> SR1) & MSK4) ^ y.u[3] ^ (d->u[3] << SL1);
    #endif
        }
        /**
         * This function fills the internal state array with pseudorandom
         * integers.
         */
        inline void gen_rand_all(Sfmt& g)
        {
            int i;
            w128_t* r1, * r2;

            r1 = &g.m_sfmt[N - 2];
            r2 = &g.m_sfmt[N - 1];
            for (i = 0; i < N - POS1; i++)
            {
                do_recursion(&g.m_sfmt[i], &g.m_sfmt[i], &g.m_sfmt[i + POS1], r1, r2);
                r1 = r2;
                r2 = &g.m_sfmt[i];
            }
            for (; i < N; i++)
            {
                do_recursion(&g.m_sfmt[i], &g.m_sfmt[i], &g.m_sfmt[i + POS1 - N], r1, r2);
                r1 = r2;
                r2 = &g.m_sfmt[i];
            }
        }

        /**
         * This function fills the user-specified array with pseudorandom
         * integers.
         *
         * @param array an 128-bit array to be filled by pseudorandom numbers.
         * @param size number of 128-bit pseudorandom numbers to be generated.
         */
        inline void gen_rand_array(Sfmt& g, w128_t* array, int size)
        {
            int i, j;
            w128_t* r1, * r2;

            r1 = &g.m_sfmt[N - 2];
            r2 = &g.m_sfmt[N - 1];
            for (i = 0; i < N - POS1; i++)
            {
                do_recursion(&array[i], &g.m_sfmt[i], &g.m_sfmt[i + POS1], r1, r2);
                r1 = r2;
                r2 = &array[i];
            }
            for (; i < N; i++)
            {
                do_recursion(&array[i], &g.m_sfmt[i], &array[i + POS1 - N], r1, r2);
                r1 = r2;
                r2 = &array[i];
            }
            for (; i < size - N; i++)
            {
                do_recursion(&array[i], &array[i - N], &array[i + POS1 - N], r1, r2);
                r1 = r2;
                r2 = &array[i];
            }
            for (j = 0; j < 2 * N - size; j++)
            {
                g.m_sfmt[j] = array[j + size - N];
            }
            for (; i < size; i++, j++)
            {
                do_recursion(&array[i], &array[i - N], &array[i + POS1 - N], r1, r2);
                r1 = r2;
                r2 = &array[i];
                g.m_sfmt[j] = array[i];
            }
        }

#endif
    } // SmftInternal
} // V


using namespace V;

//////////////////////////////////////////////////////////////////////////
// Statics
//////////////////////////////////////////////////////////////////////////


static EnvironmentVariable<V::Sfmt> _sfmt;
static const char* _globalSfmtName = "GlobalSfmt";

Sfmt& Sfmt::GetInstance()
{
    if (!_sfmt)
    {
        _sfmt = V::Environment::FindVariable<Sfmt>(_globalSfmtName);
        if (!_sfmt)
        {
            Sfmt::Create();
        }
    }

    return _sfmt.Get();
}

void Sfmt::Create()
{
    if (!_sfmt)
    {
        _sfmt = V::Environment::CreateVariable<V::Sfmt>(_globalSfmtName);
    }
}

void Sfmt::Destroy()
{
    _sfmt.Reset();
}

//=========================================================================
// Sfmt
//=========================================================================
Sfmt::Sfmt()
{
    m_psfmt32 = &m_sfmt[0].u[0];
    m_psfmt64 = reinterpret_cast<V::u64*>(m_psfmt32);

    Seed();
}

//=========================================================================
// Seed
//=========================================================================
Sfmt::Sfmt(V::u32* keys, int numKeys)
{
    m_psfmt32 = &m_sfmt[0].u[0];
    m_psfmt64 = reinterpret_cast<V::u64*>(m_psfmt32);

    Seed(keys, numKeys);
}

//=========================================================================
// Seed
//=========================================================================
void Sfmt::Seed()
{
    // buffer with random values
    V::u32 buffer[32];
    BetterPseudoRandom rnd;
    bool result = rnd.GetRandom(buffer, sizeof(buffer));
    (void)result;
    V_Warning("System", result, "Failed to seed properly the Smft generator!");
    Seed(buffer, V_ARRAY_SIZE(buffer));
}

/**
 * This function represents a function used in the initialization
 * by init_by_array
 * @param x 32-bit integer
 * @return 32-bit integer
 */
#define vsfmt_func1(x) ((x ^ (x >> 27)) * (V::u32)1664525UL)

/**
 * This function represents a function used in the initialization
 * by init_by_array
 * @param x 32-bit integer
 * @return 32-bit integer
 */
#define vsfmt_func2(x) ((x ^ (x >> 27)) * (V::u32)1566083941UL)

//=========================================================================
// Seed
//=========================================================================
void Sfmt::Seed(V::u32* keys, int numKeys)
{
    using SfmtInternal::N;
    using SfmtInternal::N32;
    int i, j, count;
    V::u32 r;
    int lag;
    int mid;
    int size = N * 4;

    if (size >= 623)
    {
        lag = 11;
    }
    else if (size >= 68)
    {
        lag = 7;
    }
    else if (size >= 39)
    {
        lag = 5;
    }
    else
    {
        lag = 3;
    }
    mid = (size - lag) / 2;

    memset(m_sfmt, 0x8b, sizeof(m_sfmt));
    if (numKeys + 1 > SfmtInternal::N32)
    {
        count = numKeys + 1;
    }
    else
    {
        count = N32;
    }
    r = vsfmt_func1((m_psfmt32[idxof(0)] ^ m_psfmt32[idxof(mid)] ^ m_psfmt32[idxof(N32 - 1)]));
    m_psfmt32[idxof(mid)] += r;
    r += numKeys;
    m_psfmt32[idxof(mid + lag)] += r;
    m_psfmt32[idxof(0)] = r;

    count--;
    for (i = 1, j = 0; (j < count) && (j < numKeys); j++)
    {
        r = vsfmt_func1((m_psfmt32[idxof(i)] ^ m_psfmt32[idxof((i + mid) % N32)] ^ m_psfmt32[idxof((i + N32 - 1) % N32)]));
        m_psfmt32[idxof((i + mid) % N32)] += r;
        r += keys[j] + i;
        m_psfmt32[idxof((i + mid + lag) % N32)] += r;
        m_psfmt32[idxof(i)] = r;
        i = (i + 1) % N32;
    }
    for (; j < count; j++)
    {
        r = vsfmt_func1((m_psfmt32[idxof(i)] ^ m_psfmt32[idxof((i + mid) % N32)] ^ m_psfmt32[idxof((i + N32 - 1) % N32)]));
        m_psfmt32[idxof((i + mid) % N32)] += r;
        r += i;
        m_psfmt32[idxof((i + mid + lag) % N32)] += r;
        m_psfmt32[idxof(i)] = r;
        i = (i + 1) % N32;
    }
    for (j = 0; j < N32; j++)
    {
        r = vsfmt_func2((m_psfmt32[idxof(i)] + m_psfmt32[idxof((i + mid) % N32)] + m_psfmt32[idxof((i + N32 - 1) % N32)]));
        m_psfmt32[idxof((i + mid) % N32)] ^= r;
        r -= i;
        m_psfmt32[idxof((i + mid + lag) % N32)] ^= r;
        m_psfmt32[idxof(i)] = r;
        i = (i + 1) % N32;
    }

    m_index = N32;
    PeriodCertification();
}

#undef vsfmt_func1
#undef vsfmt_func2

//=========================================================================
// PeriodCertification
//=========================================================================
void Sfmt::PeriodCertification()
{
    int inner = 0;
    int i, j;
    V::u32 work;

    for (i = 0; i < 4; i++)
    {
        inner ^= m_psfmt32[idxof(i)] & SfmtInternal::parity[i];
    }
    for (i = 16; i > 0; i >>= 1)
    {
        inner ^= inner >> i;
    }
    inner &= 1;
    /* check OK */
    if (inner == 1)
    {
        return;
    }
    /* check NG, and modification */
    for (i = 0; i < 4; i++)
    {
        work = 1;
        for (j = 0; j < 32; j++)
        {
            if ((work & SfmtInternal::parity[i]) != 0)
            {
                m_psfmt32[idxof(i)] ^= work;
                return;
            }
            work = work << 1;
        }
    }
}

//=========================================================================
// Rand32
//=========================================================================
V::u32 Sfmt::Rand32()
{
    int index = m_index.fetch_add(1);
    if (index >= SfmtInternal::N32)
    {
        VStd::lock_guard<decltype(m_generationMutex)> lock(m_generationMutex);
        // if this thread is the one that sets m_index to 0, then this thread
        // does the generation
        index += 1; // compare against the result of fetch_add(1) above
        if (m_index.compare_exchange_strong(index, 0))
        {
            SfmtInternal::gen_rand_all(*this);
        }
        // try again, with the new table
        return Rand32();
    }
    return m_psfmt32[index];
}

//=========================================================================
// Rand64
//=========================================================================
V::u64 Sfmt::Rand64()
{
    int index = m_index.fetch_add(2);
    if (index >= (SfmtInternal::N32 - 1))
    {
        VStd::lock_guard<decltype(m_generationMutex)> lock(m_generationMutex);
        // if this thread is the one that sets m_index to 0, then this thread
        // does the generation
        index += 2; // compare against the result of fetch_add(2) above
        if (m_index.compare_exchange_strong(index, 0))
        {
            SfmtInternal::gen_rand_all(*this);
        }
        // try again, with the new table
        return Rand64();
    }

    V::u64 r;
    r = m_psfmt64[index / 2];
    return r;
}

//=========================================================================
// FillArray32
//=========================================================================
void Sfmt::FillArray32(V::u32* array, int size)
{
    V_MATH_ASSERT(m_index == SfmtInternal::N32, "Invalid m_index! Reinitialize!");
    V_MATH_ASSERT(size % 4 == 0, "Size must be multiple of 4!");
    V_MATH_ASSERT(size >= SfmtInternal::N32, "Size must be bigger than %d GetMinArray32Size()!", SfmtInternal::N32);

    SfmtInternal::gen_rand_array(*this, (SfmtInternal::w128_t*)array, size / 4);
    m_index = SfmtInternal::N32;
}

//=========================================================================
// FillArray64
//=========================================================================
void Sfmt::FillArray64(V::u64* array, int size)
{
    V_MATH_ASSERT(m_index == SfmtInternal::N32, "Invalid m_index! Reinitialize!");
    V_MATH_ASSERT(size % 4 == 0, "Size must be multiple of 4!");
    V_MATH_ASSERT(size >= SfmtInternal::N64, "Size must be bigger than %d GetMinArray64Size()!", SfmtInternal::N64);

    SfmtInternal::gen_rand_array(*this, (SfmtInternal::w128_t*)array, size / 2);
    m_index = SfmtInternal::N32;
}

//=========================================================================
// GetMinArray32Size
//=========================================================================
int Sfmt::GetMinArray32Size() const
{
    return SfmtInternal::N32;
}

//=========================================================================
// GetMinArray64Size
//=========================================================================
int Sfmt::GetMinArray64Size() const
{
    return SfmtInternal::N64;
}