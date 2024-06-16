/*
 * Copyright (c) Contributors to the VelcroFramework Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef V_FRAMEWORK_CORE_STD_BITSET_H
#define V_FRAMEWORK_CORE_STD_BITSET_H

#include <core/std/string/string.h>

namespace VStd
{
    /**
     * Bitset template
     */
    template<VStd::size_t NumBits>
    class bitset
    {
        typedef bitset<NumBits> this_type;
    public:
        // store fixed-length sequence of Boolean elements
        typedef unsigned int            word_t;         // base type for a storage word
        typedef word_t*                 pointer;
        typedef const word_t*           const_pointer;
        typedef VStd::size_t           size_type;

        // bit reference
        class reference
        {
            friend class bitset<NumBits>;
        public:
            V_FORCE_INLINE reference& operator=(bool value)
            {
                m_bitSet.set(m_pos, value);
                return *this;
            }

            V_FORCE_INLINE reference& operator=(const reference& bitRef)
            {
                m_bitSet.set(m_pos, bool(bitRef));
                return *this;
            }

            V_FORCE_INLINE reference& flip()
            {
                m_bitSet.flip(m_pos);
                return *this;
            }

            V_FORCE_INLINE bool operator~() const  { return !m_bitSet.test(m_pos); }

            V_FORCE_INLINE operator bool() const   { return m_bitSet.test(m_pos);  }

        private:
            V_FORCE_INLINE reference(bitset<NumBits>& bitSet, VStd::size_t pos)
                : m_bitSet(bitSet)
                , m_pos(pos)
            {}

            bitset<NumBits>& m_bitSet;  ///< pointer to the bitset
            VStd::size_t    m_pos;     ///< position of element in bitset
        };

        /*V_FORCE_INLINE bool at(VStd::size_t pos) const  { return test(pos); }
        V_FORCE_INLINE reference at(size_t pos)                { return reference(*this, pos); }*/

        V_FORCE_INLINE bool operator[](VStd::size_t pos) const    { return (test(pos)); }
        V_FORCE_INLINE reference operator[](VStd::size_t pos) { return (reference(*this, pos)); }

        V_FORCE_INLINE bitset()
        {   // construct with all false values
            set_word();
        }

        inline bitset(unsigned long long value)
        {   // construct from bits in unsigned long long
            set_word();
            for (int wpos = 0;; )
            {   // store to one or more words
                m_bits[wpos] = static_cast<word_t>(value);
                if ((int)(sizeof (unsigned long long) / sizeof (word_t)) <= ++wpos || NumWords < wpos)
                {
                    break;
                }

                // to avoid warnings, this will not really be called very often
                value >>= BitsPerWord - 1;
                value >>= 1;
            }
            trim();
        }

        template<class Element, class Traits, class Allocator>
        inline explicit bitset(const basic_string<Element, Traits, Allocator>& str, VStd::size_t pos = 0, VStd::size_t count = /*basic_string<Element, Traits, Allocator>::npos*/ 0xffffffff, Element zero = (Element)'0')
        {
            // initialize from [pos, pos + count) elements in string
            VStd::size_t num;
            VSTD_CONTAINER_ASSERT(str.size() >= pos, "Invalid position %d (%d)", pos, str.size());

            if (str.size() - pos < count)
            {
                count = str.size() - pos;   // trim count to size
            }
            if (NumBits < count)
            {
                count = NumBits;    // trim count to length of bitset
            }
            set_word();

            for (pos += count, num = 0; num < count; ++num)
            {
                if (str[--pos] == zero + 1)
                {
                    set(num);
                }
            }
        }

        V_FORCE_INLINE this_type& operator&=(const this_type& rhs)
        {   // AND in rhs
            for (int pos = NumWords; 0 <= pos; --pos)
            {
                m_bits[pos] &= rhs.m_bits[pos];
            }
            return *this;
        }

        V_FORCE_INLINE this_type& operator|=(const this_type& rhs)
        {   // OR in rhs
            for (int pos = NumWords; 0 <= pos; --pos)
            {
                m_bits[pos] |= rhs.m_bits[pos];
            }
            return *this;
        }

        V_FORCE_INLINE this_type& operator^=(const this_type& rhs)
        {   // XOR in rhs
            for (int pos = NumWords; 0 <= pos; --pos)
            {
                m_bits[pos] ^= rhs.m_bits[pos];
            }
            return *this;
        }

        inline this_type& operator<<=(VStd::size_t pos)
        {   // shift left by pos
            const int shift = (int)(pos / BitsPerWord);
            if (shift != 0)
            {
                for (int wpos = NumWords; 0 <= wpos; --wpos)    // shift by words
                {
                    m_bits[wpos] = shift <= wpos ? m_bits[wpos - shift] : (word_t)0;
                }
            }

            pos %= BitsPerWord;
            if (pos)
            {   // 0 < pos < BitsPerWord, shift by bits
                for (int wpos = NumWords; 0 < wpos; --wpos)
                {
                    m_bits[wpos] = (word_t)((m_bits[wpos] << pos)
                                            | (m_bits[wpos - 1] >> (BitsPerWord - pos)));
                }
                m_bits[0] <<= pos;
            }
            trim();
            return *this;
        }

        inline this_type& operator>>=(VStd::size_t pos)
        {   // shift right by pos
            const int shift = (int)(pos / BitsPerWord);
            if (shift != 0)
            {
                for (int wpos = 0; wpos <= NumWords; ++wpos)    // shift by words
                {
                    m_bits[wpos] = shift <= NumWords - wpos
                        ? m_bits[wpos + shift] : (word_t)0;
                }
            }

            pos %= BitsPerWord;
            if (pos)
            {   // 0 < pos < BitsPerWord, shift by bits
                for (int wpos = 0; wpos < NumWords; ++wpos)
                {
                    m_bits[wpos] = (word_t)((m_bits[wpos] >> pos)
                                            | (m_bits[wpos + 1] << (BitsPerWord - pos)));
                }
                m_bits[NumWords] >>= pos;
            }
            return *this;
        }

        V_FORCE_INLINE this_type& set()
        {   // set all bits true
            set_word((word_t) ~0);
            return (*this);
        }

        V_FORCE_INLINE this_type& set(VStd::size_t pos, bool value = true)
        {   // set bit at pos to value
            VSTD_CONTAINER_ASSERT(NumBits > pos, "Invalid position %d (%d)", pos, NumBits);
            if (value)
            {
                m_bits[pos / BitsPerWord] |= (word_t)1 << pos % BitsPerWord;
            }
            else
            {
                m_bits[pos / BitsPerWord] &= ~((word_t)1 << pos % BitsPerWord);
            }
            return *this;
        }

        V_FORCE_INLINE this_type& reset()
        {   // set all bits false
            set_word();
            return *this;
        }

        V_FORCE_INLINE this_type& reset(VStd::size_t pos) { return set(pos, false); }

        V_FORCE_INLINE this_type operator~() const         { return this_type(*this).flip(); }

        V_FORCE_INLINE bitset<NumBits>& flip()
        {
            for (int pos = NumWords; 0 <= pos; --pos)
            {
                m_bits[pos] = (word_t) ~m_bits[pos];
            }

            trim();
            return *this;
        }

        V_FORCE_INLINE this_type& flip(VStd::size_t pos)
        {   // flip bit at pos
            VSTD_CONTAINER_ASSERT(NumBits > pos, "Invalid position %d (%d)", pos, NumBits);
            m_bits[pos / BitsPerWord] ^= (word_t)1 << pos % BitsPerWord;
            return *this;
        }

        inline unsigned long to_ulong() const
        {   // convert bitset to unsigned long
            static_assert((sizeof (unsigned long) % sizeof (word_t) == 0), "unsigned long and word_t have uncompatible sizes");

            int wpos = NumWords;
            for (; (int)(sizeof (unsigned long) / sizeof (word_t)) <= wpos; --wpos)
            {
                VSTD_CONTAINER_ASSERT(m_bits[wpos] == 0, "high-order words should be zero");    // fail if any high-order words are nonzero
            }
            unsigned long value = m_bits[wpos];
            for (; 0 <= --wpos; )
            {
                value = ((value << (BitsPerWord - 1)) << 1) | m_bits[wpos];
            }
            return value;
        }

        inline V::u64 to_ullong() const
        {   // convert bitset to V::u64
            static_assert((sizeof(V::u64) % sizeof(word_t) == 0), "V::u64 and word_t have incompatible sizes");

            int wpos = NumWords;
            for (; static_cast<int>(sizeof(V::u64) / sizeof(word_t)) <= wpos; --wpos)
            {
                VSTD_CONTAINER_ASSERT(m_bits[wpos] == 0, "high-order words should be zero");    // fail if any high-order words are nonzero
            }
            V::u64 value = m_bits[wpos];
            while (0 <= --wpos)
            {
                value = ((value << (BitsPerWord - 1)) << 1) | m_bits[wpos];
            }
            return value;
        }

        template<class Element, class Traits, class Allocator>
        inline basic_string<Element, Traits, Allocator> to_string(Element zero = (Element)'0') const
        {
            basic_string<Element, Traits, Allocator> str;
            VStd::size_t pos;
            str.reserve(NumBits);
            for (pos = NumBits; 0 < pos; )
            {
                str += (Element)(zero + (int)test(--pos));
            }
            return str;
        }

        template<class Element, class Traits>
        inline basic_string<Element, Traits, VStd::allocator> to_string(Element zero = (Element)'0') const
        {
            basic_string<Element, Traits, VStd::allocator> str;
            VStd::size_t pos;
            str.reserve(NumBits);
            for (pos = NumBits; 0 < pos; )
            {
                str += (Element)(zero + (int)test(--pos));
            }
            return str;
        }

        template<class Element>
        inline basic_string<Element, VStd::char_traits<Element>, VStd::allocator> to_string(Element zero = (Element)'0') const
        {
            basic_string<Element, VStd::char_traits<Element>, VStd::allocator> str;
            VStd::size_t pos;
            str.reserve(NumBits);
            for (pos = NumBits; 0 < pos; )
            {
                str += (Element)(zero + (int)test(--pos));
            }
            return str;
        }

        inline VStd::size_t count() const
        {   // count number of set bits
            static char _Bitsperhex[] = "\0\1\1\2\1\2\2\3\1\2\2\3\2\3\3\4";
            VStd::size_t value = 0;
            for (int wpos = NumWords; 0 <= wpos; --wpos)
            {
                for (word_t wordVal = m_bits[wpos]; wordVal != 0; wordVal >>= 4)
                {
                    value += _Bitsperhex[wordVal & 0xF];
                }
            }
            return value;
        }

        constexpr VStd::size_t size() const
        {
            return NumBits;
        }

        V_FORCE_INLINE bool operator==(const this_type& rhs) const
        {
            for (int pos = NumWords; 0 <= pos; --pos)
            {
                if (m_bits[pos] != rhs.m_bits[pos])
                {
                    return false;
                }
            }

            return true;
        }

        V_FORCE_INLINE bool operator!=(const this_type& rhs) const
        {
            for (int pos = NumWords; 0 <= pos; --pos)
            {
                if (m_bits[pos] != rhs.m_bits[pos])
                {
                    return true;
                }
            }
            return false;
        }

        V_FORCE_INLINE bool test(VStd::size_t pos) const
        {   // test if bit at pos is set
            VSTD_CONTAINER_ASSERT(NumBits > pos, "Invalid position %d (%d)", pos, NumBits);
            return ((m_bits[pos / BitsPerWord] & ((word_t)1 << pos % BitsPerWord)) != 0);
        }

        V_FORCE_INLINE bool any() const
        {
            for (int pos = NumWords; 0 <= pos; --pos)
            {
                if (m_bits[pos] != 0)
                {
                    return true;
                }
            }
            return false;
        }

        V_FORCE_INLINE bool none() const
        {
            for (int pos = NumWords; 0 <= pos; --pos)
            {
                if (m_bits[pos] != 0)
                {
                    return false;
                }
            }
            return true;
        }

        V_FORCE_INLINE this_type operator<<(VStd::size_t pos) const   {   return (this_type(*this) <<= pos);  }

        V_FORCE_INLINE this_type operator>>(VStd::size_t pos) const   {   return (this_type(*this) >>= pos);  }

        /**
        * \anchor BitsetExtensions
        * \name Extensions
        * @{
        */
        /// VStd Extensions. Return pointer to the bitset data. The bitset data is guaranteed to be stored as an array.
        V_FORCE_INLINE pointer         data()              { return m_bits; }
        V_FORCE_INLINE const_pointer   data() const        { return m_bits; }
        V_FORCE_INLINE VStd::size_t   num_words() const   { return NumWords + 1; }
        // @}

    private:
        enum
        {   // parameters for packing bits into words
            BitsPerWord = (8 * sizeof (word_t)),    // bits in each word
            NumWords = (NumBits - 1) / BitsPerWord
        };

        V_FORCE_INLINE void set_word(word_t value = 0)
        {   // set all words to _Wordval
            for (int pos = NumWords; 0 <= pos; --pos)
            {
                m_bits[pos] = value;
            }
            if (value != 0)
            {
                trim();
            }
        }

        V_FORCE_INLINE void trim()
        {
V_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option") // conditional expression is constant (for Traits::LocklessDispatch in asserts)
            // clear any trailing bits in last word
            if (NumBits % BitsPerWord != 0)
            {
                m_bits[NumWords] &= ((word_t)1 << NumBits % BitsPerWord) - 1;
            }
V_POP_DISABLE_WARNING
        }

        word_t m_bits[NumWords + 1];    // the set of bits
    };

    template<VStd::size_t NumBits>
    inline
    bitset<NumBits> operator&(const bitset<NumBits>& left,  const bitset<NumBits>& right)
    {   // return bitset left AND right
        bitset<NumBits> leftCopy = left;
        return (leftCopy &= right);
    }

    template<VStd::size_t NumBits>
    inline
    bitset<NumBits> operator|(const bitset<NumBits>& left,  const bitset<NumBits>& right)
    {   // return bitset left OR right
        bitset<NumBits> leftCopy = left;
        return (leftCopy |= right);
    }

    template<VStd::size_t NumBits>
    inline
    bitset<NumBits> operator^(const bitset<NumBits>& left, const bitset<NumBits>& right)
    {   // return bitset left XOR right
        bitset<NumBits> leftCopy = left;
        return (leftCopy ^= right);
    }
}


#endif // V_FRAMEWORK_CORE_STD_BITSET_H