/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_CONTAINERS_DEQUE_H
#define V_FRAMEWORK_CORE_STD_CONTAINERS_DEQUE_H

#include <vcore/std/allocator.h>
#include <vcore/std/allocator_traits.h>
#include <vcore/std/algorithm.h>
#include <vcore/std/createdestroy.h>
#include <vcore/std/typetraits/aligned_storage.h>
#include <vcore/std/typetraits/alignment_of.h>
#include <vcore/std/typetraits/is_integral.h>

namespace VStd {
    /**
     * Computes default number of elements per allocation block, based on element size.
     */
    template<VStd::size_t ElementSize>
    struct deque_block {
        enum {
            num_elements = ElementSize <= 1 ? 16 : ElementSize <= 2 ? 8 : ElementSize <= 4 ? 4 : ElementSize <= 8 ? 2 : 1
        };
    };

    /**
     * The deque is complaint with \ref CStd (23.2.1). In addition we introduce the following \ref DequeExtensions "extensions".
     * \par
     * As you know Deque allocate memory in blocks. Each block has NumElementsPerBlock * sizeof(T) size. As extension you are allowed
     * to control the number of elements per block or the minimal map size. This way you can get the optimal balance between memory
     * usage and number of allocation. Unless speed is critical you should not worry about that too much and use the default
     * settings.
     * If you want to pool or just know the node size you can use deque::block_node_type.
     *
     * Check the deque \ref VStdExamples.
     * \attention If you customize the block and map sizes make sure that NumElementsPerBlock and MinMapSize are >= 1.
     */
    template <class T, class Allocator = VStd::allocator, VStd::size_t NumElementsPerBlock = VStd::deque_block<sizeof(T)>::num_elements, VStd::size_t MinMapSize = 8 >
    class deque
#ifdef VSTD_HAS_CHECKED_ITERATORS
        : public Debug::checked_container_base
#endif
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        typedef deque<T, Allocator, NumElementsPerBlock, MinMapSize>   this_type;
        // use this only for size reference
        struct block_node
        {
            //T m_array[NumElementsPerBlock];
            typename aligned_storage<NumElementsPerBlock* sizeof(T), alignment_of<T>::value >::type   m_data;     ///< pointer with all the data
        };
    public:
        typedef T*                                      pointer;
        typedef const T*                                const_pointer;

        typedef T&                                      reference;
        typedef const T&                                const_reference;
        typedef typename Allocator::difference_type     difference_type;
        typedef typename Allocator::size_type           size_type;
        typedef Allocator                               allocator_type;

        // VSTD extension.
        typedef T                                       value_type;
        typedef block_node                              block_node_type;

        typedef pointer                                 map_node_type;
        typedef map_node_type*                          map_node_ptr_type;

        class const_iterator_impl
        {
            enum
            {
                ITERATOR_VERSION = 1
            };

            friend class deque;
            typedef const_iterator_impl                 this_type;
            typedef deque<T, Allocator, NumElementsPerBlock, MinMapSize> container_type;
        public:
            typedef T                                   value_type;
            typedef VStd::ptrdiff_t                    difference_type;
            typedef const T*                            pointer;
            typedef const T&                            reference;
            typedef VStd::random_access_iterator_tag   iterator_category;

            V_FORCE_INLINE const_iterator_impl()
                : m_offset(0) {}
            V_FORCE_INLINE const_iterator_impl(size_type offset, const container_type* container)
                : m_offset(offset)
                , m_container(container)
            {}

            V_FORCE_INLINE reference operator*() const
            {
                size_type block = m_offset / NumElementsPerBlock;
                size_type offset = m_offset & (NumElementsPerBlock - 1);
                if (m_container->m_mapSize <= block)
                {
                    block -= m_container->m_mapSize;
                }
                return m_container->m_map[block][offset];
            }

            V_FORCE_INLINE pointer operator->() const
            {
                size_type block = m_offset / NumElementsPerBlock;
                size_type offset = m_offset & (NumElementsPerBlock - 1);
                if (m_container->m_mapSize <= block)
                {
                    block -= m_container->m_mapSize;
                }
                return &m_container->m_map[block][offset];
            }

            V_FORCE_INLINE this_type& operator++()     { ++m_offset;   return *this;   }
            V_FORCE_INLINE this_type  operator++(int)  { this_type tmp = *this; ++m_offset; return tmp; }
            V_FORCE_INLINE this_type& operator--()     { --m_offset;   return *this;   }
            V_FORCE_INLINE this_type  operator--(int)  { this_type tmp = *this; --m_offset; return tmp; }
            V_FORCE_INLINE this_type& operator+=(difference_type offset)   { m_offset += offset; return *this; }
            V_FORCE_INLINE this_type  operator+(difference_type offset)        { this_type tmp = *this; tmp += offset; return tmp; }
            V_FORCE_INLINE this_type& operator-=(difference_type offset)   { m_offset -= offset; return *this; }
            V_FORCE_INLINE this_type  operator-(difference_type offset)        { this_type tmp = *this; tmp -= offset; return tmp; }
            /// ???
            V_FORCE_INLINE difference_type operator-(const this_type& rhs) const
            {
                return rhs.m_offset <= m_offset ? m_offset - rhs.m_offset : -(difference_type)(rhs.m_offset - m_offset);
            }
            V_FORCE_INLINE reference operator[](difference_type offset) const { this_type tmp = *this; tmp += offset; return *tmp; }
            V_FORCE_INLINE bool operator==(const this_type& rhs) const { return m_offset == rhs.m_offset;  }
            V_FORCE_INLINE bool operator!=(const this_type& rhs) const { return m_offset != rhs.m_offset;  }
            V_FORCE_INLINE bool operator<(const this_type& rhs) const  { return m_offset < rhs.m_offset; }
            V_FORCE_INLINE bool operator>(const this_type& rhs) const  { return m_offset > rhs.m_offset; }
            V_FORCE_INLINE bool operator<=(const this_type& rhs) const { return m_offset <= rhs.m_offset; }
            V_FORCE_INLINE bool operator>=(const this_type& rhs) const { return m_offset >= rhs.m_offset; }

        protected:
            size_type               m_offset;
            const container_type*   m_container;
        };

        class iterator_impl
            : public const_iterator_impl
        {
            typedef iterator_impl               this_type;
            typedef const_iterator_impl         base_type;
            typedef deque<T, Allocator, NumElementsPerBlock, MinMapSize> container_type;
        public:
            typedef T*                          pointer;
            typedef T&                          reference;
            typedef VStd::ptrdiff_t            difference_type;

            V_FORCE_INLINE iterator_impl() {}
            V_FORCE_INLINE iterator_impl(size_type offset, const container_type* container)
                : base_type(offset, container) {}
            V_FORCE_INLINE reference operator*() const
            {
                size_type block = base_type::m_offset / NumElementsPerBlock;
                size_type offset = base_type::m_offset & (NumElementsPerBlock - 1);
                if (base_type::m_container->m_mapSize <= block)
                {
                    block -= base_type::m_container->m_mapSize;
                }
                return base_type::m_container->m_map[block][offset];
            }

            V_FORCE_INLINE pointer operator->() const
            {
                size_type block = base_type::m_offset / NumElementsPerBlock;
                size_type offset = base_type::m_offset & (NumElementsPerBlock - 1);
                if (base_type::m_container->m_mapSize <= block)
                {
                    block -= base_type::m_container->m_mapSize;
                }
                return &base_type::m_container->m_map[block][offset];
            }

            V_FORCE_INLINE this_type& operator++()     { ++base_type::m_offset;    return *this;   }
            V_FORCE_INLINE this_type  operator++(int)  { this_type tmp = *this; ++base_type::m_offset; return tmp; }
            V_FORCE_INLINE this_type& operator--()     { --base_type::m_offset;    return *this;   }
            V_FORCE_INLINE this_type  operator--(int)  { this_type tmp = *this; --base_type::m_offset; return tmp; }
            V_FORCE_INLINE this_type& operator+=(difference_type offset)   { base_type::m_offset += offset; return *this; }
            V_FORCE_INLINE this_type  operator+(difference_type offset)        { this_type tmp = *this; tmp += offset; return tmp; }
            V_FORCE_INLINE this_type& operator-=(difference_type offset)   { base_type::m_offset -= offset; return *this; }
            V_FORCE_INLINE this_type  operator-(difference_type offset)        { this_type tmp = *this; tmp -= offset; return tmp; }
            V_FORCE_INLINE difference_type operator-(const this_type& rhs) const
            {
                return rhs.m_offset <= base_type::m_offset ? base_type::m_offset - rhs.m_offset : -(difference_type)(rhs.m_offset - base_type::m_offset);
            }
            V_FORCE_INLINE reference operator[](difference_type offset) const { this_type tmp = *this; tmp += offset; return *tmp; }
        };

#ifdef VSTD_HAS_CHECKED_ITERATORS
        typedef Debug::checked_randomaccess_iterator<iterator_impl, this_type>           iterator;
        typedef Debug::checked_randomaccess_iterator<const_iterator_impl, this_type> const_iterator;
#else
        typedef iterator_impl                           iterator;
        typedef const_iterator_impl                     const_iterator;
#endif

        typedef VStd::reverse_iterator<iterator>       reverse_iterator;
        typedef VStd::reverse_iterator<const_iterator> const_reverse_iterator;

        V_FORCE_INLINE deque()
            : m_map(0)
            , m_mapSize(0)
            , m_firstOffset(0)
            , m_size(0) {}
        V_FORCE_INLINE explicit deque(const Allocator& allocator)
            : m_map(0)
            , m_mapSize(0)
            , m_firstOffset(0)
            , m_size(0)
            , m_allocator(allocator) {}
        V_FORCE_INLINE explicit deque(size_type numElements)
            : m_map(0)
            , m_mapSize(0)
            , m_firstOffset(0)
            , m_size(0)
        {
            insert(iterator(VSTD_CHECKED_ITERATOR_2(iterator_impl, m_firstOffset, this)), numElements, value_type());
        }
        V_FORCE_INLINE deque(size_type numElements, const value_type& value)
            : m_map(0)
            , m_mapSize(0)
            , m_firstOffset(0)
            , m_size(0)
        {
            insert(iterator(VSTD_CHECKED_ITERATOR_2(iterator_impl, m_firstOffset, this)), numElements, value);
        }
        V_FORCE_INLINE deque(size_type numElements, const value_type& value, const Allocator& allocator)
            : m_map(0)
            , m_mapSize(0)
            , m_firstOffset(0)
            , m_size(0)
            , m_allocator(allocator)
        {
            insert(iterator(VSTD_CHECKED_ITERATOR_2(iterator_impl, m_firstOffset, this)), numElements, value);
        }

        V_FORCE_INLINE deque(const this_type& rhs)
            : m_map(0)
            , m_mapSize(0)
            , m_firstOffset(0)
            , m_size(0)
            , m_allocator(rhs.m_allocator)
        {
            if (!rhs.empty())
            {
                insert(iterator(VSTD_CHECKED_ITERATOR_2(iterator_impl, m_firstOffset, this)), rhs.begin(), rhs.end());
            }
        }

        template<class InputIterator>
        V_FORCE_INLINE deque(InputIterator first, InputIterator last)
            : m_map(0)
            , m_mapSize(0)
            , m_firstOffset(0)
            , m_size(0)
        {
            construct_iter(first, last, is_integral<InputIterator>());
        }

        template<class InputIterator>
        V_FORCE_INLINE deque(InputIterator first, InputIterator last, const Allocator& allocator)
            : m_map(0)
            , m_mapSize(0)
            , m_firstOffset(0)
            , m_size(0)
            , m_allocator(allocator)
        {
            construct_iter(first, last, is_integral<InputIterator>());
        }

        deque(std::initializer_list<T> ilist, const Allocator& alloc = Allocator())
            : deque(ilist.begin(), ilist.end(), alloc)
        {}

        V_FORCE_INLINE ~deque()
        {
            clear();
        }

        inline this_type& operator=(const this_type& rhs)
        {
            if (this == &rhs)
            {
                return *this;
            }

            if (rhs.m_size == 0)
            {
                clear();
            }

            else if (rhs.m_size <= m_size)
            {
                iterator mid = VStd::Internal::copy(rhs.begin(), rhs.end(), begin(), Internal::is_fast_copy<iterator, iterator>());
                erase(mid, end());
            }
            else
            {
                const_iterator mid = rhs.begin() + m_size;
                Internal::copy(rhs.begin(), mid, begin(), Internal::is_fast_copy<iterator, iterator>());
                insert(end(), mid, rhs.end());
            }

            return *this;
        }

        V_FORCE_INLINE iterator        begin()         { return iterator(VSTD_CHECKED_ITERATOR_2(iterator_impl, m_firstOffset, this)); }
        V_FORCE_INLINE const_iterator  begin() const   { return const_iterator(VSTD_CHECKED_ITERATOR_2(const_iterator_impl, m_firstOffset, this)); }
        V_FORCE_INLINE iterator        end()           { return iterator(VSTD_CHECKED_ITERATOR_2(iterator_impl, m_firstOffset + m_size, this)); }
        V_FORCE_INLINE const_iterator  end() const     { return const_iterator(VSTD_CHECKED_ITERATOR_2(const_iterator_impl, m_firstOffset + m_size, this)); }

        V_FORCE_INLINE reverse_iterator        rbegin()        { return reverse_iterator(iterator(VSTD_CHECKED_ITERATOR_2(iterator_impl, m_firstOffset + m_size, this))); }
        V_FORCE_INLINE const_reverse_iterator  rbegin() const  { return const_reverse_iterator(const_iterator(VSTD_CHECKED_ITERATOR_2(const_iterator_impl, m_firstOffset + m_size, this))); }
        V_FORCE_INLINE reverse_iterator        rend()          { return reverse_iterator(iterator(VSTD_CHECKED_ITERATOR_2(iterator_impl, m_firstOffset, this))); }
        V_FORCE_INLINE const_reverse_iterator  rend() const    { return const_reverse_iterator(const_iterator(VSTD_CHECKED_ITERATOR_2(const_iterator_impl, m_firstOffset, this))); }

        V_FORCE_INLINE void resize(size_type newSize)              { resize(newSize, value_type()); }
        V_FORCE_INLINE void resize(size_type newSize, const value_type& value) {
            if (m_size < newSize) {
                insert(end(), newSize - m_size, value);
            } else if (newSize < m_size) {
                erase(begin() + newSize, end());
            }
        }

        V_FORCE_INLINE size_type size() const      { return m_size; }
        V_FORCE_INLINE size_type max_size() const  { return VStd::allocator_traits<allocator_type>::max_size(m_allocator) / sizeof(block_node_type); }
        V_FORCE_INLINE bool empty() const          { return m_size == 0; }

        V_FORCE_INLINE const_reference at(size_type offset) const { return *const_iterator(VSTD_CHECKED_ITERATOR_2(const_iterator_impl, m_firstOffset + offset, this)); }
        V_FORCE_INLINE reference       at(size_type offset)       { return *iterator(VSTD_CHECKED_ITERATOR_2(iterator_impl, m_firstOffset + offset, this)); }
        V_FORCE_INLINE const_reference operator[](size_type offset) const { return *const_iterator(VSTD_CHECKED_ITERATOR_2(const_iterator_impl, m_firstOffset + offset, this)); }
        V_FORCE_INLINE reference       operator[](size_type offset)       { return *iterator(VSTD_CHECKED_ITERATOR_2(iterator_impl, m_firstOffset + offset, this)); }

        V_FORCE_INLINE const_reference front() const   { return *const_iterator(VSTD_CHECKED_ITERATOR_2(const_iterator_impl, m_firstOffset, this)); }
        V_FORCE_INLINE reference       front()         { return *iterator(VSTD_CHECKED_ITERATOR_2(iterator_impl, m_firstOffset, this)); }

        V_FORCE_INLINE const_reference back() const    { return *const_iterator(VSTD_CHECKED_ITERATOR_2(const_iterator_impl, m_firstOffset + m_size - 1, this)); }
        V_FORCE_INLINE reference       back()          { return *iterator(VSTD_CHECKED_ITERATOR_2(iterator_impl, m_firstOffset + m_size - 1, this)); }

        inline void push_front(const value_type& value)
        {
#ifdef VSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
            if (m_firstOffset % NumElementsPerBlock == 0 && m_mapSize <= (m_size + NumElementsPerBlock) / NumElementsPerBlock)
            {
                grow_map(1);
            }
            size_type newOffset = m_firstOffset != 0 ? m_firstOffset : m_mapSize * NumElementsPerBlock;
            size_type block = --newOffset / NumElementsPerBlock;
            if (m_map[block] == 0)
            {
                m_map[block] = reinterpret_cast<pointer>(m_allocator.allocate(sizeof(block_node_type), alignment_of<block_node_type>::value));
            }

            // copy construct
            map_node_type ptr = m_map[block] + newOffset % NumElementsPerBlock;
            Internal::construct<map_node_type>::single(ptr, value);

            m_firstOffset = newOffset;
            ++m_size;
        }
        inline void pop_front()
        {
            VSTD_CONTAINER_ASSERT(!empty(), "VStd::deque::pop_front - there are no element to pop!");
#ifdef VSTD_HAS_CHECKED_ITERATORS
            orphan_offset(m_firstOffset);
#endif

            size_type block = m_firstOffset / NumElementsPerBlock;
            pointer toDestroy = m_map[block] + m_firstOffset % NumElementsPerBlock;
            Internal::destroy<pointer>::single(toDestroy);

            if (m_mapSize * NumElementsPerBlock <= ++m_firstOffset)
            {
                m_firstOffset = 0;
            }
            if (--m_size == 0)
            {
                m_firstOffset = 0;
            }
        }

        inline void push_back(const value_type& value)
        {
#ifdef VSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
            if ((m_firstOffset + m_size) % NumElementsPerBlock == 0 && m_mapSize <= (m_size + NumElementsPerBlock) / NumElementsPerBlock)
            {
                grow_map(1);
            }
            size_type newOffset = m_firstOffset + m_size;
            size_type block = newOffset / NumElementsPerBlock;
            if (m_mapSize <= block)
            {
                block  -= m_mapSize;
            }
            if (m_map[block] == 0)
            {
                m_map[block] =  reinterpret_cast<pointer>(m_allocator.allocate(sizeof(block_node_type), alignment_of<block_node_type>::value));
            }

            // copy construct
            map_node_type ptr = m_map[block] + newOffset % NumElementsPerBlock;
            Internal::construct<map_node_type>::single(ptr, value);
            ++m_size;
        }

        inline void pop_back()
        {
            VSTD_CONTAINER_ASSERT(!empty(), "VStd::deque::pop_back - there are no element to pop!");
#ifdef VSTD_HAS_CHECKED_ITERATORS
            orphan_offset(m_firstOffset + m_size - 1);
#endif
            size_type newOffset = m_firstOffset + m_size - 1;
            size_type block = newOffset / NumElementsPerBlock;
            if (m_mapSize <= block)
            {
                block -= m_mapSize;
            }
            pointer toDestroy = m_map[block] + newOffset % NumElementsPerBlock;
            Internal::destroy<pointer>::single(toDestroy);
            if (--m_size == 0)
            {
                m_firstOffset = 0;
            }
        }

        V_FORCE_INLINE void assign(size_type numElements, const value_type& value)
        {
            value_type valueCopy = value;   // in case value is in sequence
            clear();
            insert(begin(), numElements, valueCopy);
        }

        template<class InputIterator>
        V_FORCE_INLINE void assign(InputIterator first, InputIterator last)
        {
            assign_iter(first, last, is_integral<InputIterator>());
        }

        iterator insert(const_iterator insertPos, const value_type& value)
        {
            iterator last, first = begin();
            if (insertPos == first)
            {
                push_front(value);
                return begin();
            }
            else if (insertPos == (last = end()))
            {
                push_back(value);
                return end() - 1;
            }
            else
            {
                iterator mid;
                size_type offset = insertPos - first;
                value_type valueCopy = value; // in case value is in sequence
                if (offset < m_size / 2)
                {
                    push_front(front());
                    first = begin();
                    mid = first + offset;
                    Internal::copy(first + 2, mid + 1, first + 1, Internal::is_fast_copy<iterator, iterator>());
                }
                else
                {
                    push_back(back());
                    mid = begin() + offset;
                    last = end();
                    Internal::copy_backward(mid, last - 2, last - 1, Internal::is_fast_copy<iterator, iterator>());
                }

                *mid = valueCopy;
                return mid;
            }
        }

        void insert(const_iterator insertPos, size_type numElements, const value_type& value)
        {
            iterator  mid;
            size_type offset = insertPos - begin();
            size_type rem = m_size - offset;
            //          size_type size = m_size;
            size_type i;

            if (offset < rem)
            {
                // Closer to the front of the block.
                if (offset < numElements)
                {
                    for (i = numElements - offset; i > 0; --i)
                    {
                        push_front(value);
                    }
                    for (i = offset; i > 0; --i)
                    {
                        push_front(at(numElements - 1));//push_front(begin()[numElements - 1]); // push prefix
                    }
                    mid = begin() + numElements;
                    Internal::fill(mid, mid + offset, value, Internal::is_fast_fill<iterator>());
                }
                else
                {
                    for (i = numElements; i > 0; --i)
                    {
                        push_front(at(numElements - 1));//push_fron(begin()[numElements -1]);
                    }
                    mid = begin() + numElements;
                    value_type valueCopy = value; // if it is in the sequence
                    Internal::copy(mid + numElements, mid + offset, mid, Internal::is_fast_copy<iterator, iterator>());
                    Internal::fill(begin() + offset, mid + offset, valueCopy, Internal::is_fast_fill<iterator>());
                }
            }
            else
            {
                if (rem < numElements)
                {
                    for (i = numElements - rem; i > 0; --i)
                    {
                        push_back(value);
                    }
                    for (i = 0; i < rem; ++i)
                    {
                        push_back(at(offset + i));//push_back(begin()[offset + i]); // push prefix
                    }
                    mid = begin() + offset;
                    Internal::fill(mid, mid + rem, value, Internal::is_fast_fill<iterator>());
                }
                else
                {
                    for (i = 0; i < numElements; ++i)
                    {
                        push_back(at(offset + rem - numElements + i)); // push_back(begin()[offset + rem - numElements + i]);
                    }
                    mid = begin() + offset;
                    value_type valueCopy = value;
                    Internal::copy_backward(mid, mid + rem - numElements, mid + rem, Internal::is_fast_copy<iterator, iterator>());
                    Internal::fill(mid, mid + numElements, valueCopy, Internal::is_fast_fill<iterator>());
                }
            }
        }

        V_FORCE_INLINE void insert(const_iterator insertPos, std::initializer_list<value_type> list)
        {
            insert(insertPos, list.begin(), list.end());
        }

        template<class InputIterator>
        V_FORCE_INLINE void insert(const_iterator insertPos, InputIterator first, InputIterator last)
        {
            insert_iter(insertPos, first, last, is_integral<InputIterator>());
        }

        V_FORCE_INLINE iterator erase(iterator erasePos)
        {
            return erase(erasePos, erasePos + 1);
        }

        V_INLINE iterator erase(const_iterator constFirst, const_iterator constLast)
        {
            VSTD_CONTAINER_ASSERT(constFirst <= constLast, "VStd::deque::erase - first iterator should be before last iterator!");

            iterator first = VStd::Internal::ConstIteratorCast<iterator>(constFirst);
            iterator last = VStd::Internal::ConstIteratorCast<iterator>(constLast);

            size_type offset = first - begin();
            size_type numElements = last - first;

#ifdef VSTD_HAS_CHECKED_ITERATORS
            bool isMoved = offset > 0 && (offset + numElements) < m_size;
#endif
            if (offset < size_type(end() - last))
            {
                // closer to front
                Internal::copy_backward(begin(), first, last, Internal::is_fast_copy<iterator, iterator>());
                for (; numElements > 0; --numElements)
                {
                    pop_front();
                }
            }
            else
            {
                // close to back
                Internal::copy(last, end(), first, Internal::is_fast_copy<iterator, iterator>());
                for (; numElements > 0; --numElements)
                {
                    pop_back();
                }
            }
#ifdef VSTD_HAS_CHECKED_ITERATORS
            if (isMoved)
            {
                orphan_all();
            }
#endif
            return begin() + offset;
        }

        inline void clear()
        {
            while (m_size)
            {
                pop_back();
            }

            for (size_type i = m_mapSize; i > 0; )
            {   // free storage for a block and destroy pointer
                if (*(m_map + --i) != 0)
                {
                    deallocate_memory(*(m_map + i), sizeof(block_node_type), alignment_of<block_node_type>::value, typename allocator_type::allow_memory_leaks());
                }
                map_node_ptr_type toDestroy = m_map + i;
                Internal::destroy<map_node_ptr_type>::single(toDestroy);
            }

            if (m_map)
            {
                deallocate_memory(m_map, sizeof(map_node_type) * m_mapSize, alignment_of<map_node_type>::value, typename allocator_type::allow_memory_leaks());
                m_map = 0;
            }

            m_mapSize = 0;
        }

        void swap(this_type& rhs)
        {
            VSTD_CONTAINER_ASSERT(this != &rhs, "VStd::deque::swap - no point to swap itself!");
            if (m_allocator == rhs.m_allocator)
            {
#ifdef VSTD_HAS_CHECKED_ITERATORS
                swap_all(rhs);
#endif
                VStd::swap(m_map, rhs.m_map);
                VStd::swap(m_mapSize, rhs.m_mapSize);
                VStd::swap(m_firstOffset, rhs.m_firstOffset);
                VStd::swap(m_size, rhs.m_size);
            }
            else
            {
                this_type tmp = *this;
                *this = rhs;
                rhs = tmp;
            }
        }

        deque(this_type&& rhs)
            : m_map(0)
            , m_mapSize(0)
            , m_firstOffset(0)
            , m_size(0)
            , m_allocator(rhs.m_allocator)
        {
            assign_rv(VStd::forward<this_type>(rhs));
        }
        this_type& operator=(this_type&& rhs)
        {
            assign_rv(VStd::forward<this_type>(rhs));
            return *this;
        }

        void assign_rv(this_type&& rhs)
        {
            if (this == &rhs)
            {
                return;
            }
            if (m_allocator != rhs.m_allocator)
            {
                clear();
                for (iterator iter = rhs.begin(); iter != rhs.end(); ++iter)
                {
                    push_back(VStd::forward<T>(*iter));
                }
            }
            else
            {
                clear();
#ifdef VSTD_HAS_CHECKED_ITERATORS
                swap_all(rhs);
#endif
                m_map = rhs.m_map;
                m_mapSize = rhs.m_mapSize;
                m_firstOffset = rhs.m_firstOffset;
                m_size = rhs.m_size;

                rhs.m_map = 0;
                rhs.m_mapSize = 0;
                rhs.m_firstOffset = 0;
                rhs.m_size = 0;
            }
        }

        void push_front(value_type&& value)
        {
#ifdef VSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
            if (m_firstOffset % NumElementsPerBlock == 0 && m_mapSize <= (m_size + NumElementsPerBlock) / NumElementsPerBlock)
            {
                grow_map(1);
            }
            size_type newOffset = m_firstOffset != 0 ? m_firstOffset : m_mapSize * NumElementsPerBlock;
            size_type block = --newOffset / NumElementsPerBlock;
            if (m_map[block] == 0)
            {
                m_map[block] = reinterpret_cast<pointer>(m_allocator.allocate(sizeof(block_node_type), alignment_of<block_node_type>::value));
            }

            map_node_type ptr = m_map[block] + newOffset % NumElementsPerBlock;
            Internal::construct<map_node_type>::single(ptr, VStd::forward<value_type>(value));

            m_firstOffset = newOffset;
            ++m_size;
        }

        void push_back(value_type&& value)
        {
#ifdef VSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
            if ((m_firstOffset + m_size) % NumElementsPerBlock == 0 && m_mapSize <= (m_size + NumElementsPerBlock) / NumElementsPerBlock)
            {
                grow_map(1);
            }
            size_type newOffset = m_firstOffset + m_size;
            size_type block = newOffset / NumElementsPerBlock;
            if (m_mapSize <= block)
            {
                block  -= m_mapSize;
            }
            if (m_map[block] == 0)
            {
                m_map[block] =  reinterpret_cast<pointer>(m_allocator.allocate(sizeof(block_node_type), alignment_of<block_node_type>::value));
            }

            map_node_type ptr = m_map[block] + newOffset % NumElementsPerBlock;
            Internal::construct<map_node_type>::single(ptr, VStd::forward<value_type>(value));
            ++m_size;
        }

        template<class... Args>
        void emplace_front(Args&&... args)
        {
#ifdef VSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
            if (m_firstOffset % NumElementsPerBlock == 0 && m_mapSize <= (m_size + NumElementsPerBlock) / NumElementsPerBlock)
            {
                grow_map(1);
            }
            size_type newOffset = m_firstOffset != 0 ? m_firstOffset : m_mapSize * NumElementsPerBlock;
            size_type block = --newOffset / NumElementsPerBlock;
            if (m_map[block] == 0)
            {
                m_map[block] = reinterpret_cast<pointer>(m_allocator.allocate(sizeof(block_node_type), alignment_of<block_node_type>::value));
            }

            map_node_type ptr = m_map[block] + newOffset % NumElementsPerBlock;
            Internal::construct<map_node_type>::single(ptr, VStd::forward<Args>(args)...);

            m_firstOffset = newOffset;
            ++m_size;
        }

        template<class... Args>
        void emplace_back(Args&&... args)
        {
#ifdef VSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
            if ((m_firstOffset + m_size) % NumElementsPerBlock == 0 && m_mapSize <= (m_size + NumElementsPerBlock) / NumElementsPerBlock)
            {
                grow_map(1);
            }
            size_type newOffset = m_firstOffset + m_size;
            size_type block = newOffset / NumElementsPerBlock;
            if (m_mapSize <= block)
            {
                block  -= m_mapSize;
            }
            if (m_map[block] == 0)
            {
                m_map[block] =  reinterpret_cast<pointer>(m_allocator.allocate(sizeof(block_node_type), alignment_of<block_node_type>::value));
            }

            map_node_type ptr = m_map[block] + newOffset % NumElementsPerBlock;
            Internal::construct<map_node_type>::single(ptr, VStd::forward<Args>(args)...);
            ++m_size;
        }

        template<class Args>
        iterator insert(const_iterator pos, Args&& args)
        {
            return emplace(pos, VStd::forward<Args>(args));
        }

        template<class... Args>
        iterator emplace(const_iterator pos, Args&&... args)
        {
            size_type offset = pos - begin();
            if (offset <= m_size / 2)
            {
                emplace_front(VStd::forward<Args>(args)...);
                VStd::rotate(begin(), begin() + 1, begin() + 1 + offset);
            }
            else
            {   // closer to back, push to back then rotate
                emplace_back(VStd::forward<Args>(args)...);
                VStd::rotate(begin() + offset, end() - 1, end());
            }
            return begin() + offset;
        }

        void swap(this_type&& rhs)
        {
            assign_rv(VStd::forward<this_type>(rhs));
        }

        /**
        * \anchor DequeExtensions
        * \name Extensions
        * @{
        */

        // The only difference from the standard is that we return the allocator instance, not a copy.
        V_FORCE_INLINE allocator_type&         get_allocator()         { return m_allocator; }
        V_FORCE_INLINE const allocator_type&   get_allocator() const   { return m_allocator; }
        /// Set the vector allocator. If different than then current all elements will be reallocated.
        void                                    set_allocator(const allocator_type& allocator)
        {
            if (m_allocator != allocator)
            {
                if (m_size > 0)
                {
                    // create temp deque with all elements relocated.
                    this_type templist(begin(), end(), allocator);
                    clear();
                    m_allocator = allocator;
                    swap(templist);
                }
                else
                {
                    m_allocator = allocator;
                }
            }
        }

        // Validate container status.
        inline bool     validate() const    { return true; }
        // Validate iterator.
        inline int      validate_iterator(const iterator& iter) const
        {
#ifdef VSTD_HAS_CHECKED_ITERATORS
            V_Assert(iter.m_container == this, "This iterator doesn't belong to this container");
            size_type iterOffset = iter.m_iter.m_offset;
            //          V_Assert(iter.m_iter.m_container==this,"VStd::deque::validate_iterator - this iter doesn't belong to this container!");
#else
            size_type iterOffset = iter.m_offset;
#endif
            if (iterOffset < m_firstOffset || iterOffset > (m_firstOffset + m_size))
            {
                return isf_none;
            }

            return isf_valid | isf_can_dereference;
        }
        inline int      validate_iterator(const const_iterator& iter) const
        {
#ifdef VSTD_HAS_CHECKED_ITERATORS
            V_Assert(iter.m_container == this, "This iterator doesn't belong to this container");
            size_type iterOffset = iter.m_iter.m_offset;
            //          V_Assert(iter.m_iter.m_container==this,"VStd::deque::validate_iterator - this iter doesn't belong to this container!");
#else
            size_type iterOffset = iter.m_offset;
#endif
            if (iterOffset < m_firstOffset || iterOffset > (m_firstOffset + m_size))
            {
                return isf_none;
            }

            return isf_valid | isf_can_dereference;
        }

        /**
        *  Pushes an element at the front of the deque without a provided instance. This can be used for value types
        *  with expensive constructors so we don't want to create temporary one.
        */
        inline void push_front()
        {
#ifdef VSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
            if (m_firstOffset % NumElementsPerBlock == 0 && m_mapSize <= (m_size + NumElementsPerBlock) / NumElementsPerBlock)
            {
                grow_map(1);
            }
            size_type newOffset = m_firstOffset != 0 ? m_firstOffset : m_mapSize * NumElementsPerBlock;
            size_type block = --newOffset / NumElementsPerBlock;
            if (m_map[block] == 0)
            {
                m_map[block] = reinterpret_cast<pointer>(m_allocator.allocate(sizeof(block_node_type), alignment_of<block_node_type>::value));
            }

            pointer toCreate = m_map[block] + newOffset % NumElementsPerBlock;
            Internal::construct<pointer>::single(toCreate);

            m_firstOffset = newOffset;
            ++m_size;
        }

        /**
        *  Pushes an element at the back of the deque without a provided instance. This can be used for value types
        *  with expensive constructors so we don't want to create temporary one.
        */
        inline void push_back()
        {
#ifdef VSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
            if ((m_firstOffset + m_size) % NumElementsPerBlock == 0 && m_mapSize <= (m_size + NumElementsPerBlock) / NumElementsPerBlock)
            {
                grow_map(1);
            }
            size_type newOffset = m_firstOffset + m_size;
            size_type block = newOffset / NumElementsPerBlock;
            if (m_mapSize <= block)
            {
                block  -= m_mapSize;
            }
            if (m_map[block] == 0)
            {
                m_map[block] = reinterpret_cast<pointer>(m_allocator.allocate(sizeof(block_node_type), alignment_of<block_node_type>::value));
            }

            pointer toCreate = m_map[block] + newOffset % NumElementsPerBlock;
            Internal::construct<pointer>::single(toCreate);
            ++m_size;
        }


        /**
        * Resets the container without deallocating any memory or calling any destructor.
        * This function should be used when we need very quick tear down. Generally it's used for temporary vectors and we can just nuke them that way.
        * In addition the provided \ref Allocators, has leak and reset flag which will enable automatically this behavior. So this function should be used in
        * special cases \ref VStdExamples.
        * \note This function is added to the vector for consistency. In the vector case we have only one allocation, and if the allocator allows memory leaks
        * it can just leave deallocate function empty, which performance wise will be the same. For more complex containers this will make big difference.
        */
        inline void             leak_and_reset()
        {
            m_map = 0;
            m_size = 0;
            m_mapSize = 0;
            m_firstOffset = 0;

#ifdef VSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
        }
        /// @}
    private:
        template <class InputIterator>
        V_FORCE_INLINE void    construct_iter(const InputIterator& first, const InputIterator& last, const true_type& /* is_integral<InputIterator> */)
        {
            insert(iterator(VSTD_CHECKED_ITERATOR_2(iterator_impl, m_firstOffset, this)), (size_type)first, (value_type)last);
        }

        template <class InputIterator>
        V_FORCE_INLINE void    construct_iter(const InputIterator& first, const InputIterator& last, const false_type& /* !is_integral<InputIterator> */)
        {
            insert(iterator(VSTD_CHECKED_ITERATOR_2(iterator_impl, m_firstOffset, this)), first, last);
        }

        V_FORCE_INLINE void    deallocate_memory(void* ptr, size_type size, size_type alignment, const true_type& /* allocator::allow_memory_leaks */)
        {
            (void)ptr;
            (void)size;
            (void)alignment;
        }
        V_FORCE_INLINE void    deallocate_memory(void* ptr, size_type size, size_type alignment, const false_type& /* !allocator::allow_memory_leaks */)
        {
            m_allocator.deallocate(ptr, size, alignment);
        }

        void grow_map(size_type numElements)
        {
            size_type inc = m_mapSize / 2; // grow by 50%, the same as the vector (may be we add grow policy for both of them ?)
            if (inc < MinMapSize)
            {
                inc = MinMapSize;
            }
            if (numElements < inc)
            {
                numElements = inc;
            }
            size_type offset = m_firstOffset / NumElementsPerBlock;
            map_node_ptr_type newMap = reinterpret_cast<map_node_ptr_type>(m_allocator.allocate(sizeof(map_node_type) * (m_mapSize + numElements), alignment_of<map_node_type>::value));
            map_node_ptr_type mapPtr = newMap + offset;

            mapPtr = VStd::uninitialized_copy(m_map + offset, m_map + m_mapSize, mapPtr, Internal::is_fast_copy<map_node_ptr_type, map_node_ptr_type>());
            if (offset <= numElements)
            {
                mapPtr = VStd::uninitialized_copy(m_map, m_map + offset, mapPtr, Internal::is_fast_copy<map_node_ptr_type, map_node_ptr_type>());
                //VStd::uninitialized_fill_n(mapPtr, numElements - offset,0,VStd::true_type());
                //VStd::uninitialized_fill_n(newMap, offset,0,VStd::true_type());
                ::memset(mapPtr, 0, (numElements - offset) * sizeof(map_node_type));
                ::memset(newMap, 0, (offset) * sizeof(map_node_type));
            }
            else
            {
                VStd::uninitialized_copy(m_map, m_map + numElements, mapPtr, Internal::is_fast_copy<map_node_ptr_type, map_node_ptr_type>());
                mapPtr = VStd::uninitialized_copy(m_map + numElements, m_map + offset, newMap, Internal::is_fast_copy<map_node_ptr_type, map_node_ptr_type>());
                //VStd::uninitialized_fill_n(mapPtr, numElements,0,VStd::true_type());
                ::memset(mapPtr, 0, (numElements) * sizeof(map_node_type));
            }

            if (m_map)
            {
                map_node_ptr_type toDestroyStart = m_map + offset;
                map_node_ptr_type toDestroyEnd = m_map + m_mapSize;
                Internal::destroy<map_node_ptr_type>::range(toDestroyStart, toDestroyEnd);
                deallocate_memory(m_map, sizeof(map_node_type) * m_mapSize, alignment_of<map_node_type>::value, typename allocator_type::allow_memory_leaks());
            }

            m_map = newMap;
            m_mapSize += numElements;
        }


        template<class InputIterator>
        V_FORCE_INLINE void assign_iter(const InputIterator& first, const InputIterator& last, const true_type& /* is_intergral<InputIterator>()*/)
        {
            assign((size_type)first, (value_type)last);
        }
        template<class InputIterator>
        V_FORCE_INLINE void assign_iter(const InputIterator& first, const InputIterator& last, const false_type& /* is_intergral<InputIterator>()*/)
        {
            clear();
            insert(begin(), first, last);
        }
        template<class InputIterator>
        V_FORCE_INLINE void insert_iter(const_iterator insertPos, const InputIterator& first, const InputIterator& last, const true_type& /* is_intergral<InputIterator>()*/)
        {
            insert(insertPos, (size_type)first, (value_type)last);
        }

        template<class InputIterator>
        void insert_iter(const_iterator insertPos, const InputIterator& first, const InputIterator& last, const false_type& /* !is_intergral<InputIterator>()*/)
        {
            VSTD_CONTAINER_ASSERT(first != last, "VStd::deque::insert_iter - first and last iterator are the same!");
            size_type offset = insertPos - begin();
            size_type rem = m_size - offset;
            size_type size = m_size;
            if (offset < rem)
            {
                // Closer to the front of the block.
                InputIterator iter(first);
                for (; iter != last; ++iter)
                {
                    push_front(*iter);
                }

                size_type num = m_size - size;
                if (offset > 0)
                {
                    reverse(num, num + offset);
                    reverse(0, num + offset);
                }
                else
                {
                    reverse(0, num);
                }
            }
            else
            {
                // Closer to the back of the block.
                InputIterator iter(first);
                for (; iter != last; ++iter)
                {
                    push_back(*iter);
                }

                if (offset < size)
                {
                    reverse(offset, size);
                    reverse(size, m_size);
                    reverse(offset, m_size);
                }
            }
        }

        inline void reverse(size_type first, size_type last)
        {
            for (; first != last && first != --last; ++first)
            {
                //iterator start = begin();
                value_type temp = at(first);

                at(first) = at(last);
                at(last) = temp;
            }
        }

#ifdef VSTD_HAS_CHECKED_ITERATORS
        void orphan_offset(size_type offset) const
        {   // orphan iterators with specified offset(s)
            (void)offset;
            V_Assert(m_size > 0, "VStd::deque::orphan_offset - deque is empty!");
            size_type offset_end = m_firstOffset + m_size <= offset + 1 ? (size_type)(-1) : offset;
            if (offset == m_firstOffset)
            {
                offset = 0;
            }

#ifdef VSTD_CHECKED_ITERATORS_IN_MULTI_THREADS
            V_GLOBAL_SCOPED_LOCK(get_global_section());
#endif
            Debug::checked_iterator_base* iter = m_iteratorList;
            while (iter != 0)
            {
                V_Assert(iter->m_container == static_cast<const checked_container_base*>(this), "VStd::deque::orphan_range - iterator was corrupted!");
                iterator_impl deqIter = static_cast<iterator*>(iter)->m_iter;
                if (deqIter.m_offset >= offset && deqIter.m_offset <= offset_end)
                {
                    // orphan the iterator
                    iter->m_container = 0;

                    if (iter->m_prevIterator)
                    {
                        iter->m_prevIterator->m_nextIterator = iter->m_nextIterator;
                    }
                    else
                    {
                        m_iteratorList = iter->m_nextIterator;
                    }

                    if (iter->m_nextIterator)
                    {
                        iter->m_nextIterator->m_prevIterator = iter->m_prevIterator;
                    }
                }

                iter = iter->m_nextIterator;
            }
        }
#endif // VSTD_HAS_CHECKED_ITERATORS

    protected:
        map_node_ptr_type   m_map;          ///< Pointer to array of pointers to blocks.
        size_type           m_mapSize;      ///< Size of map array.
        size_type           m_firstOffset;  ///< Offset of initial element.
        size_type           m_size;         ///< Number of elements in the deque.
        allocator_type      m_allocator;    ///< Instance of the allocator.
    };

    template <class T, class Allocator, VStd::size_t NumElementsPerBlock, VStd::size_t MinMapSize>
    V_FORCE_INLINE bool operator==(const deque<T, Allocator, NumElementsPerBlock, MinMapSize>& left, const deque<T, Allocator, NumElementsPerBlock, MinMapSize>& right)
    {
        return left.size() == right.size() && VStd::equal(left.begin(), left.end(), right.begin());
    }
    template <class T, class Allocator, VStd::size_t NumElementsPerBlock, VStd::size_t MinMapSize>
    V_FORCE_INLINE bool operator!=(const deque<T, Allocator, NumElementsPerBlock, MinMapSize>& left, const deque<T, Allocator, NumElementsPerBlock, MinMapSize>& right)
    {
        return !(left.size() == right.size() && VStd::equal(left.begin(), left.end(), right.begin()));
    }
    /*template <class T, class Allocator, VStd::size_t NumElementsPerBlock, VStd::size_t MinMapSize>
    V_FORCE_INLINE bool operator< (const deque<T,Allocator,NumElementsPerBlock,MinMapSize>& left,const deque<T,Allocator,NumElementsPerBlock,MinMapSize>& right)
    {

    }
    template <class T, class Allocator, VStd::size_t NumElementsPerBlock, VStd::size_t MinMapSize>
    V_FORCE_INLINE bool operator> (const deque<T,Allocator,NumElementsPerBlock,MinMapSize>& left,const deque<T,Allocator,NumElementsPerBlock,MinMapSize>& right)
    {

    }
    template <class T, class Allocator, VStd::size_t NumElementsPerBlock, VStd::size_t MinMapSize>
    V_FORCE_INLINE bool operator>=(const deque<T,Allocator,NumElementsPerBlock,MinMapSize>& left,const deque<T,Allocator,NumElementsPerBlock,MinMapSize>& right)
    {

    }
    template <class T, class Allocator, VStd::size_t NumElementsPerBlock, VStd::size_t MinMapSize>
    V_FORCE_INLINE bool operator<=(const deque<T,Allocator,NumElementsPerBlock,MinMapSize>& left,const deque<T,Allocator,NumElementsPerBlock,MinMapSize>& right)
    {

    }*/
    // specialized algorithms:
    template <class T, class Allocator, VStd::size_t NumElementsPerBlock, VStd::size_t MinMapSize>
    V_FORCE_INLINE void swap(deque<T, Allocator, NumElementsPerBlock, MinMapSize>& left, deque<T, Allocator, NumElementsPerBlock, MinMapSize>& right)
    {
        left.swap(right);
    }

    template <class T, class Allocator, VStd::size_t NumElementsPerBlock, VStd::size_t MinMapSize>
    inline void swap(deque<T, Allocator, NumElementsPerBlock, MinMapSize>& left, deque<T, Allocator, NumElementsPerBlock, MinMapSize>&& right)
    {
        typedef deque<T, Allocator, NumElementsPerBlock, MinMapSize> this_type;
        left.swap(VStd::forward<this_type>(right));
    }

    template <class T, class Allocator, VStd::size_t NumElementsPerBlock, VStd::size_t MinMapSize>
    inline void swap(deque<T, Allocator, NumElementsPerBlock, MinMapSize>&& left, deque<T, Allocator, NumElementsPerBlock, MinMapSize>& right)
    {
        typedef deque<T, Allocator, NumElementsPerBlock, MinMapSize> this_type;
        right.swap(VStd::forward<this_type>(left));
    }

    template<class T, class Allocator, VStd::size_t NumElementsPerBlock, VStd::size_t MinMapSize, class Predicate>
    decltype(auto) erase_if(deque<T, Allocator, NumElementsPerBlock, MinMapSize>& container, Predicate predicate)
    {
        auto iter = VStd::remove_if(container.begin(), container.end(), predicate);
        auto removedCount = VStd::distance(iter, container.end());
        container.erase(iter, container.end());
        return removedCount;
    }
}

#endif // V_FRAMEWORK_CORE_STD_CONTAINERS_DEQUE_H