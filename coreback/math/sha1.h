#ifndef V_FRAMEWORK_CORE_MATH_SHA1_H
#define V_FRAMEWORK_CORE_MATH_SHA1_H

#include <vcore/base.h>

namespace V {
    class Sha1 {
        public:
            typedef V::u32(&DigestType)[5];
        public:
            Sha1();
            void Reset();

            void ProcessByte(unsigned char byte);
            void ProcessBlock(void const* bytesBegin, void const* bytesEnd);
            void ProcessBytes(void const* buffer, size_t byteCount);

            void GetDigest(DigestType digest);
        private:
            void processBlock();

            V_FORCE_INLINE V::u32 leftRotate(V::u32 x, size_t n) {
                return (x << n) ^ (x >> (32 - n));
            }
        private:
            V::u32 m_h[5];
            unsigned char m_block[64];
            size_t m_blockByteIndex;
            size_t m_byteCount;
    };

    inline Sha1::Sha1() {
        Reset();
    }

    inline void Sha1::Reset() {
        m_h[0] = 0x67452301;
        m_h[1] = 0xEFCDAB89;
        m_h[2] = 0x98BADCFE;
        m_h[3] = 0x10325476;
        m_h[4] = 0xC3D2E1F0;

        m_blockByteIndex = 0;
        m_byteCount = 0;
    }

    inline void Sha1::ProcessByte(unsigned char byte) {
        m_block[m_blockByteIndex++] = byte;
        ++m_byteCount;
        if (m_blockByteIndex == 64) {
            m_blockByteIndex = 0;
            processBlock();
        }
    }

    inline void Sha1::ProcessBlock(void const* bytesBegin, void const* bytesEnd) {
        unsigned char const* begin = static_cast<unsigned char const*>(bytesBegin);
        unsigned char const* end = static_cast<unsigned char const*>(bytesEnd);
        for (; begin != end; ++begin)
        {
            ProcessByte(*begin);
        }
    }

    inline void Sha1::ProcessBytes(void const* buffer, size_t byteCount)
    {
        unsigned char const* b = static_cast<unsigned char const*>(buffer);
        ProcessBlock(b, b + byteCount);
    }

    inline void Sha1::processBlock()
    {
        V::u32 w[80];
        for (size_t i = 0; i < 16; ++i) {
            w[i] = (m_block[i * 4 + 0] << 24);
            w[i] |= (m_block[i * 4 + 1] << 16);
            w[i] |= (m_block[i * 4 + 2] << 8);
            w[i] |= (m_block[i * 4 + 3]);
        }
        for (size_t i = 16; i < 80; ++i) {
            w[i] = leftRotate((w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16]), 1);
        }

        V::u32 a = m_h[0];
        V::u32 b = m_h[1];
        V::u32 c = m_h[2];
        V::u32 d = m_h[3];
        V::u32 e = m_h[4];

        for (size_t i = 0; i < 80; ++i) {
            V::u32 f;
            V::u32 k;

            if (i < 20) {
                f = (b & c) | (~b & d);
                k = 0x5A827999;
            } else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }

            V::u32 temp = leftRotate(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = leftRotate(b, 30);
            b = a;
            a = temp;
        }

        m_h[0] += a;
        m_h[1] += b;
        m_h[2] += c;
        m_h[3] += d;
        m_h[4] += e;
    }

    inline void Sha1::GetDigest(DigestType digest) {
        size_t bit_count = m_byteCount * 8;

        // append the bit '1' to the message
        ProcessByte(0x80);

        // append k bits '0', where k is the minimum number >= 0
        // such that the resulting message length is congruent to 56 (mod 64)
        // check if there is enough space for padding and bit_count
        if (m_blockByteIndex > 56) {
            // finish this block
            while (m_blockByteIndex != 0) {
                ProcessByte(0);
            }

            // one more block
            while (m_blockByteIndex < 56) {
                ProcessByte(0);
            }
        } else {
            while (m_blockByteIndex < 56) {
                ProcessByte(0);
            }
        }

        // append length of message (before pre-processing) 
        // as a 64-bit big-endian integer
        ProcessByte(0);
        ProcessByte(0);
        ProcessByte(0);
        ProcessByte(0);
        ProcessByte(static_cast<unsigned char>((bit_count >> 24) & 0xFF));
        ProcessByte(static_cast<unsigned char>((bit_count >> 16) & 0xFF));
        ProcessByte(static_cast<unsigned char>((bit_count >> 8) & 0xFF));
        ProcessByte(static_cast<unsigned char>((bit_count) & 0xFF));

        // get final digest
        digest[0] = m_h[0];
        digest[1] = m_h[1];
        digest[2] = m_h[2];
        digest[3] = m_h[3];
        digest[4] = m_h[4];
    }
}

#endif // V_FRAMEWORK_CORE_MATH_SHA1_H