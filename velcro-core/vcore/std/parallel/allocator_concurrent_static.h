#ifndef V_FRAMEWORK_CORE_STD_PARALLEL_ALLOCATOR_CONCURRENT_STATIC_H
#define V_FRAMEWORK_CORE_STD_PARALLEL_ALLOCATOR_CONCURRENT_STATIC_H

#include <vcore/std/base.h>
#include <vcore/std/parallel/atomic.h>
#include <vcore/std/typetraits/integral_constant.h>
#include <vcore/std/typetraits/aligned_storage.h>
#include <vcore/std/typetraits/alignment_of.h>

namespace VStd
{
    /**
     *  Declares a static buffer of Node[NumNodes], and them pools them. It provides concurrent
     *  safe access. This is a perfect allocator for pooling lists or hash table nodes.
     *  Internally the buffer is allocated using aligned_storage.
     *  \note only allocate/deallocate are thread safe. 
     *  reset, leak_before_destroy and comparison operators are not thread safe.
     *  get_allocated_size is thread safe but the returned value is not perfectly in 
     *  sync on the actual number of allocations (the number of allocations is incremented before the
     *  allocation happens and decremented after the allocation happens, trying to give a conservative
     *  number)
     *  \note be careful if you use this on the stack, since many platforms
     *  do NOT support alignment more than 16 bytes. In such cases you will need to do it manually.
     */
    template< class Node, VStd::size_t NumNodes >
    class static_pool_concurrent_allocator
    {
        typedef static_pool_concurrent_allocator<Node, NumNodes> this_type;

        typedef int32_t            index_type;

        union  pool_node
        {
            typename aligned_storage<sizeof(Node), VStd::alignment_of<Node>::value >::type m_node;
            index_type  m_index;
        };

        static constexpr index_type invalid_index = index_type(-1);

    public:
        typedef Node                value_type;
        typedef Node*               pointer_type;
        typedef VStd::size_t       size_type;
        typedef VStd::ptrdiff_t    difference_type;
        typedef VStd::false_type   allow_memory_leaks;

        V_FORCE_INLINE static_pool_concurrent_allocator(const char* name = "VStd::static_pool_concurrent_allocator")
            : m_name(name)
        {
            reset();
        }

        V_FORCE_INLINE ~static_pool_concurrent_allocator()
        {
            AZ_Assert(m_numOfAllocatedNodes == 0, "We still have allocated nodes. Call leak_before_destroy() before you destroy the container, to indicate this is ok.");
        }

        // When we copy the allocator we don't copy the allocated memory since it's the user responsibility.
        V_FORCE_INLINE static_pool_concurrent_allocator(const this_type& rhs)
            : m_name(rhs.m_name)    {  reset(); }
        V_FORCE_INLINE static_pool_concurrent_allocator(const this_type& rhs, const char* name)
            : m_name(name) { (void)rhs; reset(); }
        V_FORCE_INLINE this_type& operator=(const this_type& rhs)      { m_name = rhs.m_name; return *this; }

        V_FORCE_INLINE const char*  get_name() const           { return m_name; }
        V_FORCE_INLINE void         set_name(const char* name) { m_name = name; }
        constexpr size_type          max_size() const           { return NumNodes * sizeof(Node); }
        V_FORCE_INLINE size_type   get_allocated_size() const  { return m_numOfAllocatedNodes.load(VStd::memory_order_relaxed) * sizeof(Node); }

        inline Node* allocate()
        {
            index_type firstFreeNode = m_firstFreeNode.load(VStd::memory_order_relaxed);
            if (firstFreeNode != invalid_index)
            {
                ++m_numOfAllocatedNodes;

                // weak compare_exchange is used here since firstFreeNode could change invalidating the value being passed to compare_exchange (2nd parameter)
                // which requires the loop to execute. Therefore, since we need to execute the loop anyway, a spurious failure will yield to better results.
                while (!m_firstFreeNode.compare_exchange_weak(
                    firstFreeNode,
                    (reinterpret_cast<pool_node*>(&m_data) + firstFreeNode)->m_index,
                    VStd::memory_order_release,
                    VStd::memory_order_relaxed))
                {
                    if (firstFreeNode == invalid_index)
                    {
                        AZ_Assert(false, "VStd::static_pool_concurrent_allocator - No more free nodes!");
                        return nullptr;
                    }
                }
                return reinterpret_cast<Node*>(reinterpret_cast<pool_node*>(&m_data) + firstFreeNode);
            }
            else
            {
                AZ_Assert(false, "VStd::static_pool_concurrent_allocator - No more free nodes!");
                return nullptr;
            }
        }

        inline pointer_type allocate(size_type byteSize, size_type alignment, int flags = 0)
        {
            (void)alignment;
            (void)byteSize;
            (void)flags;

            AZ_Assert(alignment > 0 && (alignment & (alignment - 1)) == 0, "VStd::static_pool_concurrent_allocator::allocate - alignment must be > 0 and power of 2");
            AZ_Assert(byteSize == sizeof(Node), "VStd::static_pool_concurrent_allocator - We can allocate only node sizes from the pool!");
            AZ_Assert(alignment <= VStd::alignment_of<Node>::value, "VStd::static_pool_concurrent_allocator - Invalid data alignment!");

            return allocate();
        }

        void  deallocate(Node* ptr)
        {
            AZ_Assert(((char*)ptr >= (char*)&m_data) && ((char*)ptr < ((char*)&m_data + sizeof(Node) * NumNodes)), "VStd::static_pool_concurrent_allocator - Pointer is out of range!");

            pool_node* firstPoolNode = reinterpret_cast<pool_node*>(&m_data);
            pool_node* curPoolNode = reinterpret_cast<pool_node*>(ptr);
            do 
            {
                curPoolNode->m_index = m_firstFreeNode.load(VStd::memory_order_relaxed);
            } while (!m_firstFreeNode.compare_exchange_weak(
                curPoolNode->m_index,
                (index_type)(curPoolNode - firstPoolNode),
                VStd::memory_order_release,
                VStd::memory_order_relaxed));
            --m_numOfAllocatedNodes;
        }

        inline void  deallocate(pointer_type ptr, size_type byteSize, size_type alignment)
        { 
            (void)byteSize;
            (void)alignment;
            AZ_Assert(alignment > 0 && (alignment & (alignment - 1)) == 0, "VStd::static_pool_concurrent_allocator::deallocate - alignment must be > 0 and power of 2");
            AZ_Assert(byteSize <= sizeof(Node), "VStd::static_pool_concurrent_allocator - We can allocate only node sizes from the pool!");
            deallocate(reinterpret_cast<Node*>(ptr));
        }

        V_FORCE_INLINE size_type    resize(pointer_type ptr, size_type newSize)
        {
            (void)ptr;
            (void)newSize;
            return sizeof(pool_node); // this is the max size we can have.
        }

        void  reset()
        {
            m_numOfAllocatedNodes = 0;
            m_firstFreeNode = 0;
            pool_node* poolNode = reinterpret_cast<pool_node*>(&m_data);
            for (index_type iNode = 1; iNode < (index_type)NumNodes; ++iNode, ++poolNode)
            {
                poolNode->m_index = iNode;
            }

            poolNode->m_index = invalid_index;
        }

        void leak_before_destroy()
        {
#ifdef AZ_Assert  // used only to confirm that it is ok for use to have allocated nodes
            m_numOfAllocatedNodes = 0;
            m_firstFreeNode = invalid_index; // We are not allowed to allocate after we call leak_before_destroy.
#endif
        }

        V_FORCE_INLINE void*               data()      const { return &m_data; }
        V_FORCE_INLINE constexpr size_type data_size() const { return sizeof(Node) * NumNodes; }

    private:
        typename aligned_storage<sizeof(Node)* NumNodes, VStd::alignment_of<Node>::value>::type m_data;
        const char*    m_name;
        atomic<index_type>      m_firstFreeNode;
        atomic<size_type>       m_numOfAllocatedNodes;
    };

    template< class Node, VStd::size_t NumNodes >
    V_FORCE_INLINE bool operator==(const static_pool_concurrent_allocator<Node, NumNodes>& a, const static_pool_concurrent_allocator<Node, NumNodes>& b)
    {
        return &a == &b;
    }

    template< class Node, VStd::size_t NumNodes >
    V_FORCE_INLINE bool operator!=(const static_pool_concurrent_allocator<Node, NumNodes>& a, const static_pool_concurrent_allocator<Node, NumNodes>& b)
    {
        return &a != &b;
    }
}


#endif // V_FRAMEWORK_CORE_STD_PARALLEL_ALLOCATOR_CONCURRENT_STATIC_H