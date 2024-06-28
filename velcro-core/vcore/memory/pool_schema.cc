#include <vcore/platform_incl.h>
#include <vcore/memory/pool_schema.h>

#include <vcore/std/containers/vector.h>
#include <vcore/std/containers/intrusive_slist.h>
#include <vcore/std/containers/intrusive_list.h>
#include <vcore/std/parallel/mutex.h>
#include <vcore/std/parallel/lock.h>
#include <vcore/std/parallel/thread.h>

#include <vcore/memory/allocator_records.h>
#include <vcore/math/math_utils.h>

#include <vcore/std/parallel/containers/lock_free_intrusive_stamped_stack.h>


namespace V
{
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // Pool Allocation algorithm
    /**
    * Pool Allocation algorithm implementation. Used in both PoolAllocator and ThreadPoolAllocator.
    */
    template<class Allocator>
    class PoolAllocation
    {
    public:
        V_CLASS_ALLOCATOR(PoolAllocation<Allocator>, SystemAllocator, 0)

        typedef typename Allocator::Page    PageType;
        typedef typename Allocator::Bucket  BucketType;

        PoolAllocation(Allocator* alloc, size_t pageSize, size_t minAllocationSize, size_t maxAllocationSize);
        virtual ~PoolAllocation();

        void*   Allocate(size_t byteSize, size_t alignment);
        void    DeAllocate(void* ptr);
        size_t  AllocationSize(void* ptr);
        // if isForceFreeAllPages is true we will free all pages even if they have allocations in them.
        void    GarbageCollect(bool isForceFreeAllPages = false);

        Allocator*              AllocatorHandle;
        size_t                  PageSize;
        size_t                  MinAllocationShift;
        size_t                  MinAllocationSize;
        size_t                  MaxAllocationSize;
        size_t                  NumBuckets;
        BucketType*             Buckets;
        size_t                  NumBytesAllocated;
    };

    /**
     * PoolSchema Implementation... to keep the header clean.
     */
    class PoolSchemaImpl
    {
    public:
        V_CLASS_ALLOCATOR(PoolSchemaImpl, SystemAllocator, 0)

        PoolSchemaImpl(const PoolSchema::Descriptor& desc);
        ~PoolSchemaImpl();

        PoolSchema::pointer_type    Allocate(PoolSchema::size_type byteSize, PoolSchema::size_type alignment, int flags = 0);
        void                        DeAllocate(PoolSchema::pointer_type ptr);
        PoolSchema::size_type       AllocationSize(PoolSchema::pointer_type ptr);

        /**
        * We allocate memory for pools in pages. Page is a information struct
        * located at the end of the allocated page. When it's in the at the end
        * we can usually hide it's size in the free bytes left from the pagesize/poolsize.
        * \note IMPORTANT pages are aligned on the page size, this way can find quickly which
        * pool the pointer belongs to.
        */
        struct Page
            : public VStd::intrusive_list_node<Page>
        {
            struct FakeNode
                : public VStd::intrusive_slist_node<FakeNode>
            {};

            void SetupFreeList(size_t elementSize, size_t pageDataBlockSize);

            /// We just use a free list of nodes which we cast to the pool type.
            typedef VStd::intrusive_slist<FakeNode, VStd::slist_base_hook<FakeNode> > FreeListType;

            FreeListType FreeList;
            u32 Bin;
            Debug::Magic32 Magic;
            u32 ElementSize;
            u32 MaxNumElements;
        };

        /**
        * A bucket has a list of pages used with the specific pool size.
        */
        struct Bucket
        {
            typedef VStd::intrusive_list<Page, VStd::list_base_hook<Page> > PageListType;
            PageListType            Pages;
        };

        // Functions used by PoolAllocation template
        V_INLINE Page* PopFreePage();
        V_INLINE void  PushFreePage(Page* page);
        void            GarbageCollect();
        inline bool     IsInStaticBlock(Page* page)
        {
            const char* staticBlockStart = reinterpret_cast<const char*>(m_staticDataBlock);
            const char* staticBlockEnd = staticBlockStart + m_numStaticPages*PageSize;
            const char* pageAddress = reinterpret_cast<const char*>(page);
            // all pages are the same size so we either in or out, no need to check the pageAddressEnd
            if (pageAddress >= staticBlockStart && pageAddress < staticBlockEnd)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        inline Page*    ConstructPage(size_t elementSize)
        {
            V_Assert(m_isDynamic, "We run out of static pages (%d) and this is a static allocator!", m_numStaticPages);
            // We store the page struct at the end of the block
            char* memBlock;
            memBlock = reinterpret_cast<char*>(m_pageAllocator->Allocate(PageSize, PageSize, 0, "AZSystem::PoolSchemaImpl::ConstructPage", __FILE__, __LINE__));
            size_t pageDataSize = PageSize - sizeof(Page);
            Page* page = new(memBlock+pageDataSize)Page();
            page->SetupFreeList(elementSize, pageDataSize);
            page->ElementSize = static_cast<u32>(elementSize);
            page->MaxNumElements = static_cast<u32>(pageDataSize / elementSize);
            return page;
        }

        inline void     FreePage(Page* page)
        {
            // TODO: It's optional if we want to check the guard value for corruption, since we are not going
            // to use this memory. Yet it might be useful to catch bugs.
            // We store the page struct at the end of the block
            char* memBlock = reinterpret_cast<char*>(page) - PageSize + sizeof(Page);
            page->~Page(); // destroy the page
            m_pageAllocator->DeAllocate(memBlock);
        }

        inline Page*    PageFromAddress(void* address)
        {
            char* memBlock = reinterpret_cast<char*>(reinterpret_cast<size_t>(address) & ~(PageSize-1));
            memBlock += PageSize - sizeof(Page);
            Page* page = reinterpret_cast<Page*>(memBlock);
            if (!page->Magic.Validate())
            {
                return nullptr;
            }
            return page;
        }

        typedef PoolAllocation<PoolSchemaImpl> AllocatorType;
        IAllocatorAllocate*                 m_pageAllocator;
        AllocatorType                       AllocatorHandle;
        void*                               m_staticDataBlock;
        unsigned int                        m_numStaticPages;
        bool                                m_isDynamic;
        size_t                              PageSize;
        Bucket::PageListType                m_freePages;
    };

    /**
         * Thread safe pool allocator.
         */
    class ThreadPoolSchemaImpl
    {
    public:
        V_CLASS_ALLOCATOR(ThreadPoolSchemaImpl, SystemAllocator, 0)

        /**
         * Specialized \ref PoolAllocator::Page page for lock free allocator.
         */
        struct Page : public VStd::intrusive_list_node<Page>
        {
            Page(ThreadPoolData* threadData)
                : ThreadData(threadData) {}

            struct FakeNode
                : public VStd::intrusive_slist_node<FakeNode>
            {};
            // Fake Lock Free node used when we delete an element from another thread.
            struct FakeNodeLF
                : public VStd::lock_free_intrusive_stack_node<FakeNodeLF>{};

            void SetupFreeList(size_t elementSize, size_t pageDataBlockSize);

            /// We just use a free list of nodes which we cast to the pool type.
            typedef VStd::intrusive_slist<FakeNode, VStd::slist_base_hook<FakeNode> > FreeListType;

            FreeListType FreeList;
            VStd::lock_free_intrusive_stack_node<Page> LFStack;        ///< Lock Free stack node
            struct ThreadPoolData* ThreadData;                        ///< The thread data that own's the page.
            u32 Bin;
            Debug::Magic32 Magic;
            u32 ElementSize;
            u32 MaxNumElements;
        };

        /**
        * A bucket has a list of pages used with the specific pool size.
        */
        struct Bucket
        {
            typedef VStd::intrusive_list<Page, VStd::list_base_hook<Page> > PageListType;
            PageListType            Pages;
        };

        ThreadPoolSchemaImpl(const ThreadPoolSchema::Descriptor& desc, ThreadPoolSchema::GetThreadPoolData threadPoolGetter, ThreadPoolSchema::SetThreadPoolData threadPoolSetter);
        ~ThreadPoolSchemaImpl();

        ThreadPoolSchema::pointer_type  Allocate(ThreadPoolSchema::size_type byteSize, ThreadPoolSchema::size_type alignment, int flags = 0);
        void                            DeAllocate(ThreadPoolSchema::pointer_type ptr);
        ThreadPoolSchema::size_type     AllocationSize(ThreadPoolSchema::pointer_type ptr);
        /// Return unused memory to the OS. Don't call this too often because you will force unnecessary allocations.
        void            GarbageCollect();
        //////////////////////////////////////////////////////////////////////////

        // Functions used by PoolAllocation template
        V_INLINE Page* PopFreePage();
        V_INLINE void  PushFreePage(Page* page);
        inline bool     IsInStaticBlock(Page* page)
        {
            const char* staticBlockStart = reinterpret_cast<const char*>(m_staticDataBlock);
            const char* staticBlockEnd = staticBlockStart + m_numStaticPages*PageSize;
            const char* pageAddress = reinterpret_cast<const char*>(page);
            // all pages are the same size so we either in or out, no need to check the pageAddressEnd
            if (pageAddress > staticBlockStart && pageAddress < staticBlockEnd)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        inline Page*    ConstructPage(size_t elementSize)
        {
            V_Assert(m_isDynamic, "We run out of static pages (%d) and this is a static allocator!", m_numStaticPages);
            // We store the page struct at the end of the block
            char* memBlock;
            memBlock = reinterpret_cast<char*>(m_pageAllocator->Allocate(PageSize, PageSize, 0, "AZSystem::ThreadPoolSchema::ConstructPage", __FILE__, __LINE__));
            size_t pageDataSize = PageSize - sizeof(Page);
            Page* page = new(memBlock+pageDataSize)Page(m_threadPoolGetter());
            page->SetupFreeList(elementSize, pageDataSize);
            page->ElementSize = static_cast<u32>(elementSize);
            page->MaxNumElements = static_cast<u32>(pageDataSize / elementSize);
            return page;
        }

        inline void     FreePage(Page* page)
        {
            // TODO: It's optional if we want to check the guard value for corruption, since we are not going
            // to use this memory. Yet it might be useful to catch bugs.
            // We store the page struct at the end of the block
            char* memBlock = reinterpret_cast<char*>(page) - PageSize + sizeof(Page);
            page->~Page();     // destroy the page
            m_pageAllocator->DeAllocate(memBlock);
        }

        inline Page*    PageFromAddress(void* address)
        {
            char* memBlock = reinterpret_cast<char*>(reinterpret_cast<size_t>(address) & ~static_cast<size_t>(PageSize-1));
            memBlock += PageSize - sizeof(Page);
            Page* page = reinterpret_cast<Page*>(memBlock);
            if (!page->Magic.Validate())
            {
                return nullptr;
            }
            return page;
        }

        // NOTE we allow only one instance of a allocator, you need to subclass it for different versions.
        // so for now it's safe to use static. We use TLS on a static because on some platforms set thread key is available
        // only for pthread lib and we don't use it. I can't find other way to it, otherwise please switch this to
        // use TlsAlloc/TlsFree etc.
        ThreadPoolSchema::GetThreadPoolData m_threadPoolGetter;
        ThreadPoolSchema::SetThreadPoolData m_threadPoolSetter;

        // Fox X64 we push/pop pages using the m_mutex to sync. Pages are
        typedef Bucket::PageListType FreePagesType;
        FreePagesType               m_freePages;
        VStd::vector<ThreadPoolData*> m_threads;           ///< Array with all separate thread data. Used to traverse end free elements.

        IAllocatorAllocate*         m_pageAllocator;
        void*                       m_staticDataBlock;
        size_t                      m_numStaticPages;
        size_t                      PageSize;
        size_t                      MinAllocationSize;
        size_t                      MaxAllocationSize;
        bool                        m_isDynamic;
        // TODO rbbaklov Changed to recursive_mutex from mutex for Linux support.
        VStd::recursive_mutex      m_mutex;
    };

    struct ThreadPoolData
    {
        V_CLASS_ALLOCATOR(ThreadPoolData, SystemAllocator, 0)

        ThreadPoolData(ThreadPoolSchemaImpl* alloc, size_t pageSize, size_t minAllocationSize, size_t maxAllocationSize);
        ~ThreadPoolData();

        typedef PoolAllocation<ThreadPoolSchemaImpl>  AllocatorType;
        /**
        * Stack with freed elements from other threads. We don't need stamped stack since the ABA problem can not
        * happen here. We push from many threads and pop from only one (we don't push from it).
        */
        typedef VStd::lock_free_intrusive_stack<ThreadPoolSchemaImpl::Page::FakeNodeLF, VStd::lock_free_intrusive_stack_base_hook<ThreadPoolSchemaImpl::Page::FakeNodeLF> > FreedElementsStack;

        AllocatorType           AllocatorHandle;
        FreedElementsStack      m_freedElements;
    };
}

using namespace V;

//=========================================================================
// PoolAllocation
//=========================================================================
template<class Allocator>
PoolAllocation<Allocator>::PoolAllocation(Allocator* alloc, size_t pageSize, size_t minAllocationSize, size_t maxAllocationSize)
    : AllocatorHandle(alloc)
    , PageSize(pageSize)
    , NumBytesAllocated(0)
{
    V_Assert(alloc->m_pageAllocator, "We need the page allocator setup!");
    V_Assert(pageSize >= maxAllocationSize * 4, "We need to fit at least 4 objects in a pool! Increase your page size! Page %d MaxAllocationSize %d",pageSize,maxAllocationSize);
    V_Assert(minAllocationSize == maxAllocationSize || ((minAllocationSize)&(minAllocationSize - 1)) == 0, "Min allocation should be either equal to max allocation size or power of two");

    MinAllocationSize = V::GetMax(minAllocationSize, size_t(8));
    MaxAllocationSize = V::GetMax(maxAllocationSize, minAllocationSize);

    MinAllocationShift = 0;
    for (size_t i = 1; i < sizeof(unsigned int)*8; i++)
    {
        if (MinAllocationSize >> i == 0)
        {
            MinAllocationShift = i-1;
            break;
        }
    }

    V_Assert(MaxAllocationSize % MinAllocationSize == 0, "You need to be able to divide MaxAllocationSize (%d) / MinAllocationSize (%d) without fraction!", MaxAllocationSize, MinAllocationSize);
    NumBuckets = MaxAllocationSize / MinAllocationSize;
    V_Assert(NumBuckets <= 0xffff, "You can't have more than 65535 number of buckets! We need to increase the index size!");
    Buckets = reinterpret_cast<BucketType*>(alloc->m_pageAllocator->Allocate(sizeof(BucketType)*NumBuckets, VStd::alignment_of<BucketType>::value));
    for (size_t i = 0; i < NumBuckets; ++i)
    {
        new(Buckets + i)BucketType();
    }
}

//=========================================================================
// ~PoolAllocation
//=========================================================================
template<class Allocator>
PoolAllocation<Allocator>::~PoolAllocation()
{
    GarbageCollect(true);

    for (size_t i = 0; i < NumBuckets; ++i)
    {
        Buckets[i].~BucketType();
    }
    AllocatorHandle->m_pageAllocator->DeAllocate(Buckets, sizeof(BucketType) * NumBuckets);
}

//=========================================================================
// Allocate
//=========================================================================
template<class Allocator>
V_INLINE void*
PoolAllocation<Allocator>::Allocate(size_t byteSize, size_t alignment)
{
    V_Assert(byteSize>0, "You can not allocate 0 bytes!");
    V_Assert(alignment>0&&(alignment&(alignment-1))==0, "Alignment must be >0 and power of 2!");

    // pad the size to the min allocation size.
    byteSize = V::SizeAlignUp(byteSize, MinAllocationSize);
    byteSize = V::SizeAlignUp(byteSize, alignment);

    if (byteSize > MaxAllocationSize)
    {
        V_Assert(false, "Allocation size (%d) is too big (max: %d) for pools!", byteSize, MaxAllocationSize);
        return nullptr;
    }

    u32 bucketIndex = static_cast<u32>((byteSize >> MinAllocationShift)-1);
    BucketType& bucket = Buckets[bucketIndex];
    PageType* page = nullptr;
    if (!bucket.Pages.empty())
    {
        page = &bucket.Pages.front();

        // check if we have free slot in the page
        if (page->FreeList.empty())
        {
            page = nullptr;
        }
        else if (page->FreeList.size()==1)
        {
            // if we have only 1 free slot this allocation will
            // fill the page, so put in on the back
            bucket.Pages.pop_front();
            bucket.Pages.push_back(*page);
        }
    }
    if (!page)
    {
        page = AllocatorHandle->PopFreePage();
        if (page)
        {
            // We have any pages available on free page stack.
            if (page->Bin != bucketIndex)  // if this page was used the same bucket we are ready to roll.
            {
                size_t elementSize = byteSize;
                size_t pageDataSize = PageSize - sizeof(PageType);
                page->SetupFreeList(elementSize, pageDataSize);
                page->Bin = bucketIndex;
                page->ElementSize = static_cast<u32>(elementSize);
                page->MaxNumElements = static_cast<u32>(pageDataSize / elementSize);
            }
        }
        else
        {
            // We need to align each page on it's size, this way we can quickly find which page the pointer belongs to.
            page = AllocatorHandle->ConstructPage(byteSize);
            page->Bin = bucketIndex;
        }
        bucket.Pages.push_front(*page);
    }

    // The data address and the fake node address are shared.
    void* address = &page->FreeList.front();
    page->FreeList.pop_front();

    NumBytesAllocated += byteSize;

    return address;
}

//=========================================================================
// DeAllocate
//=========================================================================
template<class Allocator>
V_INLINE void
PoolAllocation<Allocator>::DeAllocate(void* ptr)
{
    PageType* page = AllocatorHandle->PageFromAddress(ptr);
    if (page==nullptr)
    {
        V_Error("Memory", false, "Address 0x%08x is not in the ThreadPool!", ptr);
        return;
    }

    // (pageSize - info struct at the end) / (element size)
    size_t maxElementsPerBucket = page->MaxNumElements;

    size_t numFreeNodes = page->FreeList.size();
    typename PageType::FakeNode* node = new(ptr) typename PageType::FakeNode();
    page->FreeList.push_front(*node);

    if (numFreeNodes==0)
    {
        // if the page was full before sort at the front
        BucketType& bucket = Buckets[page->Bin];
        bucket.Pages.erase(*page);
        bucket.Pages.push_front(*page);
    }
    else if (numFreeNodes == maxElementsPerBucket-1)
    {
        // push to the list of free pages
        BucketType& bucket = Buckets[page->Bin];
        PageType* frontPage = &bucket.Pages.front();
        if (frontPage != page)
        {
            bucket.Pages.erase(*page);
            // check if the front page is full if so push the free page to the front otherwise push
            // push it on the free pages list so it can be reused by other bins.
            if (frontPage->FreeList.empty())
            {
                bucket.Pages.push_front(*page);
            }
            else
            {
                AllocatorHandle->PushFreePage(page);
            }
        }
        else if (frontPage->m_next != nullptr)
        {
            // if the next page has free slots free the current page
            if (frontPage->m_next->FreeList.size() < maxElementsPerBucket)
            {
                bucket.Pages.erase(*page);
                AllocatorHandle->PushFreePage(page);
            }
        }
    }

    NumBytesAllocated -= page->ElementSize;
}

//=========================================================================
// AllocationSize
//=========================================================================
template<class Allocator>
V_INLINE size_t
PoolAllocation<Allocator>::AllocationSize(void* ptr)
{
    PageType* page = AllocatorHandle->PageFromAddress(ptr);
    size_t elementSize;
    if (page)
    {
        elementSize = page->ElementSize;
    }
    else
    {
        elementSize = 0;
    }

    return elementSize;
}

//=========================================================================
// GarbageCollect
//=========================================================================
template<class Allocator>
V_INLINE void
PoolAllocation<Allocator>::GarbageCollect(bool isForceFreeAllPages)
{
    // Free empty pages in the buckets (or better be empty)
    for (unsigned int i = 0; i < (unsigned int)NumBuckets; ++i)
    {
        // (pageSize - info struct at the end) / (element size)
        size_t maxElementsPerBucket = (PageSize - sizeof(PageType)) / ((i+1) << MinAllocationShift);

        typename BucketType::PageListType& pages = Buckets[i].Pages;
        while (!pages.empty())
        {
            PageType& page = pages.front();
            pages.pop_front();
            if (page.FreeList.size()==maxElementsPerBucket || isForceFreeAllPages)
            {
                if (!AllocatorHandle->IsInStaticBlock(&page))
                {
                    AllocatorHandle->FreePage(&page);
                }
                else
                {
                    AllocatorHandle->PushFreePage(&page);
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// PollAllocator
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//=========================================================================
// PoolSchema
//=========================================================================
PoolSchema::PoolSchema(const Descriptor& desc)
    : m_impl(nullptr)
{
    (void)desc;  // ignored here, applied in Create()
}

//=========================================================================
// ~PoolSchema
//=========================================================================
PoolSchema::~PoolSchema()
{
    V_Assert(m_impl==nullptr, "You did not destroy the pool schema!");
    delete m_impl;
}

//=========================================================================
// Create
//=========================================================================
bool PoolSchema::Create(const Descriptor& desc)
{
    V_Assert(m_impl==nullptr, "PoolSchema already created!");
    if (m_impl == nullptr)
    {
        m_impl = vnew PoolSchemaImpl(desc);
    }
    return (m_impl!=nullptr);
}

//=========================================================================
// ~Destroy
//=========================================================================
bool PoolSchema::Destroy()
{
    delete m_impl;
    m_impl = nullptr;
    return true;
}

//=========================================================================
// Allocate
//=========================================================================
PoolSchema::pointer_type
PoolSchema::Allocate(size_type byteSize, size_type alignment, int flags, const char* name, const char* fileName, int lineNum, unsigned int suppressStackRecord)
{
    (void)flags;
    (void)name;
    (void)fileName;
    (void)lineNum;
    (void)suppressStackRecord;
    return m_impl->Allocate(byteSize, alignment);
}

//=========================================================================
// DeAllocate
//=========================================================================
void
PoolSchema::DeAllocate(pointer_type ptr, size_type byteSize, size_type alignment)
{
    (void)byteSize;
    (void)alignment;
    m_impl->DeAllocate(ptr);
}

//=========================================================================
// Resize
//=========================================================================
PoolSchema::size_type
PoolSchema::Resize(pointer_type ptr, size_type newSize)
{
    (void)ptr;
    (void)newSize;
    return 0;  // unsupported
}

//=========================================================================
// ReAllocate
//=========================================================================
PoolSchema::pointer_type 
PoolSchema::ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment)
{
    (void)ptr;
    (void)newSize;
    (void)newAlignment;
    V_Assert(false, "unsupported");

    return ptr;
}

//=========================================================================
// AllocationSize
//=========================================================================
PoolSchema::size_type
PoolSchema::AllocationSize(pointer_type ptr)
{
    return m_impl->AllocationSize(ptr);
}

//=========================================================================
// DeAllocate
//=========================================================================
void
PoolSchema::GarbageCollect()
{
    // External requests for garbage collection may come from any thread, and the
    // garbage collection operation isn't threadsafe, which can lead to crashes.
    //
    // Due to the low memory consumption of this allocator in practice on Dragonfly
    // (~3kb) it makes sense to not bother with garbage collection and leave it to
    // occur exclusively in the destruction of the allocator.
    //
    // TODO: A better solution needs to be found for integrating back into mainline 
    // Open 3D Engine.
    //m_impl->GarbageCollect();
}

auto PoolSchema::GetMaxContiguousAllocationSize() const -> size_type
{
    return m_impl->AllocatorHandle.MaxAllocationSize;
}

//=========================================================================
// NumAllocatedBytes
//=========================================================================
PoolSchema::size_type
PoolSchema::NumAllocatedBytes() const
{
    return m_impl->AllocatorHandle.NumBytesAllocated;
}

//=========================================================================
// Capacity
//=========================================================================
PoolSchema::size_type
PoolSchema::Capacity() const
{
    return m_impl->m_numStaticPages * m_impl->PageSize;
}

//=========================================================================
// GetPageAllocator
//=========================================================================
IAllocatorAllocate*
PoolSchema::GetSubAllocator()
{
    return m_impl->m_pageAllocator;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// PollAllocator Implementation
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//=========================================================================
// PoolSchemaImpl
//=========================================================================
PoolSchemaImpl::PoolSchemaImpl(const PoolSchema::Descriptor& desc)
    : m_pageAllocator(desc.PageAllocator ? desc.PageAllocator : &AllocatorInstance<SystemAllocator>::Get())
    , AllocatorHandle(this, desc.PageSize, desc.MinAllocationSize, desc.MaxAllocationSize)
    , m_staticDataBlock(nullptr)
    , m_numStaticPages(desc.NumStaticPages)
    , m_isDynamic(desc.IsDynamic)
    , PageSize(desc.PageSize)
{
    if (m_numStaticPages)
    {
        // We store the page struct at the end of the block
        char* memBlock = reinterpret_cast<char*>(m_pageAllocator->Allocate(PageSize*m_numStaticPages, PageSize, 0, "AZSystem::PoolAllocation::Page static array", __FILE__, __LINE__));
        m_staticDataBlock = memBlock;
        size_t pageDataSize = PageSize - sizeof(Page);
        for (unsigned int i = 0; i < m_numStaticPages; ++i)
        {
            Page* page = new(memBlock+pageDataSize)Page();
            page->Bin = 0xffffffff;
            page->ElementSize = 0;
            page->MaxNumElements = 0;
            PushFreePage(page);
            memBlock += PageSize;
        }
    }
}

//=========================================================================
// ~PoolSchemaImpl
//=========================================================================
PoolSchemaImpl::~PoolSchemaImpl()
{
    // Force free all pages
    AllocatorHandle.GarbageCollect(true);

    // Free all unused memory
    GarbageCollect();

    if (m_staticDataBlock)
    {
        while (!m_freePages.empty())
        {
            Page* page = &m_freePages.front();
            (void)page;
            m_freePages.pop_front();
            V_Assert(IsInStaticBlock(page), "All dynamic pages should be deleted by now!");
        }
        ;

        char* memBlock = reinterpret_cast<char*>(m_staticDataBlock);
        size_t pageDataSize = PageSize - sizeof(Page);
        for (unsigned int i = 0; i < m_numStaticPages; ++i)
        {
            Page* page = reinterpret_cast<Page*>(memBlock+pageDataSize);
            page->~Page();
            memBlock += PageSize;
        }
        m_pageAllocator->DeAllocate(m_staticDataBlock);
    }
}

//=========================================================================
// Allocate
//=========================================================================
PoolSchema::pointer_type
PoolSchemaImpl::Allocate(PoolSchema::size_type byteSize, PoolSchema::size_type alignment, int flags)
{
    //V_Warning("Memory",m_ownerThread==VStd::this_thread::get_id(),"You can't allocation from a different context/thread, use ThreadPoolAllocator!");
    (void)flags;
    void* address = AllocatorHandle.Allocate(byteSize, alignment);
    return address;
}

//=========================================================================
// DeAllocate
//=========================================================================
void
PoolSchemaImpl::DeAllocate(PoolSchema::pointer_type ptr)
{
    //V_Warning("Memory",m_ownerThread==VStd::this_thread::get_id(),"You can't deallocate from a different context/thread, use ThreadPoolAllocator!");
    AllocatorHandle.DeAllocate(ptr);
}

//=========================================================================
// AllocationSize
//=========================================================================
PoolSchema::size_type
PoolSchemaImpl::AllocationSize(PoolSchema::pointer_type ptr)
{
    //V_Warning("Memory",m_ownerThread==VStd::this_thread::get_id(),"You can't use PoolAllocator from a different context/thread, use ThreadPoolAllocator!");
    return AllocatorHandle.AllocationSize(ptr);
}

//=========================================================================
// Pop
//=========================================================================
V_FORCE_INLINE PoolSchemaImpl::Page*
PoolSchemaImpl::PopFreePage()
{
    Page* page = nullptr;
    if (!m_freePages.empty())
    {
        page = &m_freePages.front();
        m_freePages.pop_front();
    }
    return page;
}

//=========================================================================
// Push
//=========================================================================
V_INLINE void
PoolSchemaImpl::PushFreePage(Page* page)
{
    m_freePages.push_front(*page);
}

//=========================================================================
// PurgePages
//=========================================================================
void
PoolSchemaImpl::GarbageCollect()
{
    //if( m_ownerThread == VStd::this_thread::get_id() )
    {
        if (m_isDynamic)
        {
            AllocatorHandle.GarbageCollect();

            Bucket::PageListType staticPages;
            while (!m_freePages.empty())
            {
                Page* page = &m_freePages.front();
                m_freePages.pop_front();
                if (IsInStaticBlock(page))
                {
                    staticPages.push_front(*page);
                }
                else
                {
                    FreePage(page);
                }
            }

            while (!staticPages.empty())
            {
                Page* page = &staticPages.front();
                staticPages.pop_front();
                m_freePages.push_front(*page);
            }
        }
    }
}

//=========================================================================
// SetupFreeList
//=========================================================================
V_FORCE_INLINE void
PoolSchemaImpl::Page::SetupFreeList(size_t elementSize, size_t pageDataBlockSize)
{
    char* pageData = reinterpret_cast<char*>(this) - pageDataBlockSize;
    FreeList.clear();
    // setup free list
    size_t numElements = pageDataBlockSize / elementSize;
    for (unsigned int i = 0; i < numElements; ++i)
    {
        char* address = pageData+i*elementSize;
        Page::FakeNode* node = new(address) Page::FakeNode();
        FreeList.push_back(*node);
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// ThreadPoolSchema
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//=========================================================================
// ThreadPoolSchema
//=========================================================================
ThreadPoolSchema::ThreadPoolSchema(GetThreadPoolData getThreadPoolData, SetThreadPoolData setThreadPoolData)
    : m_impl(nullptr)
    , m_threadPoolGetter(getThreadPoolData)
    , m_threadPoolSetter(setThreadPoolData)
{
}

//=========================================================================
// ~ThreadPoolSchema
//=========================================================================
ThreadPoolSchema::~ThreadPoolSchema()
{
    V_Assert(m_impl==nullptr, "You did not destroy the thread pool schema!");
    delete m_impl;
}

//=========================================================================
// Create
//=========================================================================
bool ThreadPoolSchema::Create(const Descriptor& desc)
{
    V_Assert(m_impl==nullptr, "PoolSchema already created!");
    if (m_impl == nullptr)
    {
        m_impl = vnew ThreadPoolSchemaImpl(desc, m_threadPoolGetter, m_threadPoolSetter);
    }
    return (m_impl!=nullptr);
}

//=========================================================================
// Destroy
// [9/15/2009]
//=========================================================================
bool ThreadPoolSchema::Destroy()
{
    delete m_impl;
    m_impl = nullptr;
    return true;
}
//=========================================================================
// Allocate
// [9/15/2009]
//=========================================================================
ThreadPoolSchema::pointer_type
ThreadPoolSchema::Allocate(size_type byteSize, size_type alignment, int flags, const char* name, const char* fileName, int lineNum, unsigned int suppressStackRecord)
{
    (void)flags;
    (void)name;
    (void)fileName;
    (void)lineNum;
    (void)suppressStackRecord;
    return m_impl->Allocate(byteSize, alignment);
}

//=========================================================================
// DeAllocate
//=========================================================================
void
ThreadPoolSchema::DeAllocate(pointer_type ptr, size_type byteSize, size_type alignment)
{
    (void)byteSize;
    (void)alignment;
    m_impl->DeAllocate(ptr);
}

//=========================================================================
// Resize
//=========================================================================
ThreadPoolSchema::size_type
ThreadPoolSchema::Resize(pointer_type ptr, size_type newSize)
{
    (void)ptr;
    (void)newSize;
    return 0;  // unsupported
}

//=========================================================================
// ReAllocate
//=========================================================================
ThreadPoolSchema::pointer_type
ThreadPoolSchema::ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment)
{
    (void)ptr;
    (void)newSize;
    (void)newAlignment;
    V_Assert(false, "unsupported");
    
    return ptr;
}

//=========================================================================
// AllocationSize
//=========================================================================
ThreadPoolSchema::size_type
ThreadPoolSchema::AllocationSize(pointer_type ptr)
{
    return m_impl->AllocationSize(ptr);
}

//=========================================================================
// DeAllocate
//=========================================================================
void
ThreadPoolSchema::GarbageCollect()
{
    m_impl->GarbageCollect();
}

auto ThreadPoolSchema::GetMaxContiguousAllocationSize() const -> size_type
{
    return m_impl->MaxAllocationSize;
}

//=========================================================================
// NumAllocatedBytes
// [11/1/2010]
//=========================================================================
ThreadPoolSchema::size_type
ThreadPoolSchema::NumAllocatedBytes() const
{
    size_type bytesAllocated = 0;
    {
        VStd::lock_guard<VStd::recursive_mutex> lock(m_impl->m_mutex);
        for (size_t i = 0; i < m_impl->m_threads.size(); ++i)
        {
            bytesAllocated += m_impl->m_threads[i]->AllocatorHandle.NumBytesAllocated;
        }
    }
    return bytesAllocated;
}

//=========================================================================
// Capacity
//=========================================================================
ThreadPoolSchema::size_type
ThreadPoolSchema::Capacity() const
{
    return m_impl->m_numStaticPages * m_impl->PageSize;
}

//=========================================================================
// GetPageAllocator
// [11/17/2010]
//=========================================================================
IAllocatorAllocate*
ThreadPoolSchema::GetSubAllocator()
{
    return m_impl->m_pageAllocator;
}


//=========================================================================
// ThreadPoolSchemaImpl
//=========================================================================
ThreadPoolSchemaImpl::ThreadPoolSchemaImpl(const ThreadPoolSchema::Descriptor& desc, ThreadPoolSchema::GetThreadPoolData threadPoolGetter, ThreadPoolSchema::SetThreadPoolData threadPoolSetter)
    : m_threadPoolGetter(threadPoolGetter)
    , m_threadPoolSetter(threadPoolSetter)
    , m_pageAllocator(desc.PageAllocator)
    , m_staticDataBlock(nullptr)
    , m_numStaticPages(desc.NumStaticPages)
    , PageSize(desc.PageSize)
    , MinAllocationSize(desc.MinAllocationSize)
    , MaxAllocationSize(desc.MaxAllocationSize)
    , m_isDynamic(desc.IsDynamic)
{
#   if V_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT
    // In memory allocation case (usually tools) we might have high contention,
    // using spin lock will improve performance.
    SetCriticalSectionSpinCount(m_mutex.native_handle(), 4000);
#   endif

    if (m_pageAllocator == nullptr)
    {
        m_pageAllocator = &AllocatorInstance<SystemAllocator>::Get();  // use the SystemAllocator if no page allocator is provided
    }
    if (m_numStaticPages)
    {
        // We store the page struct at the end of the block
        char* memBlock = reinterpret_cast<char*>(m_pageAllocator->Allocate(PageSize*m_numStaticPages, PageSize, 0, "AZSystem::ThreadPoolSchemaImpl::Page static array", __FILE__, __LINE__));
        m_staticDataBlock = memBlock;
        size_t pageDataSize = PageSize - sizeof(Page);
        for (unsigned int i = 0; i < m_numStaticPages; ++i)
        {
            Page* page = new(memBlock+pageDataSize)Page(m_threadPoolGetter());
            page->Bin = 0xffffffff;
            PushFreePage(page);
            memBlock += PageSize;
        }
    }
}

//=========================================================================
// ~ThreadPoolSchemaImpl
//=========================================================================
ThreadPoolSchemaImpl::~ThreadPoolSchemaImpl()
{
    // clean up all the thread data.
    // IMPORTANT: We assume/rely that all threads (except the calling one) are or will
    // destroyed before you create another instance of the pool allocation.
    // This should generally be ok since the all allocators are singletons.
    {
        VStd::lock_guard<VStd::recursive_mutex> lock(m_mutex);
        if (!m_threads.empty())
        {
            for (size_t i = 0; i < m_threads.size(); ++i)
            {
                if (m_threads[i])
                {
                    // Force free all pages
                    delete m_threads[i];
                }
            }

            /// reset the variable for the owner thread.
            m_threadPoolSetter(nullptr);
        }
    }

    GarbageCollect();

    if (m_staticDataBlock)
    {
        Page* page;
        {
            VStd::lock_guard<VStd::recursive_mutex> lock(m_mutex);
            while (!m_freePages.empty())
            {
                page = &m_freePages.front();
                m_freePages.pop_front();
                V_Assert(IsInStaticBlock(page), "All dynamic pages should be free by now!");
            }
        }

        char* memBlock = reinterpret_cast<char*>(m_staticDataBlock);
        size_t pageDataSize = PageSize - sizeof(Page);
        for (unsigned int i = 0; i < m_numStaticPages; ++i)
        {
            page = reinterpret_cast<Page*>(memBlock+pageDataSize);
            page->~Page();
            memBlock += PageSize;
        }
        m_pageAllocator->DeAllocate(m_staticDataBlock);
    }
}

//=========================================================================
// Allocate
//=========================================================================
ThreadPoolSchema::pointer_type
ThreadPoolSchemaImpl::Allocate(ThreadPoolSchema::size_type byteSize, ThreadPoolSchema::size_type alignment, int flags)
{
    (void)flags;

    ThreadPoolData* threadData = m_threadPoolGetter();

    if (threadData == nullptr)
    {
        threadData = vnew ThreadPoolData(this, PageSize, MinAllocationSize, MaxAllocationSize);
        m_threadPoolSetter(threadData);
        {
            VStd::lock_guard<VStd::recursive_mutex> lock(m_mutex);
            m_threads.push_back(threadData);
        }
    }
    else
    {
        // deallocate elements if they were freed from other threads
        Page::FakeNodeLF* fakeLFNode;
        while ((fakeLFNode = threadData->m_freedElements.pop())!=nullptr)
        {
            threadData->AllocatorHandle.DeAllocate(fakeLFNode);
        }
    }

    return threadData->AllocatorHandle.Allocate(byteSize, alignment);
}

//=========================================================================
// DeAllocate
//=========================================================================
void
ThreadPoolSchemaImpl::DeAllocate(ThreadPoolSchema::pointer_type ptr)
{
    Page* page = PageFromAddress(ptr);
    if (page==nullptr)
    {
        V_Error("Memory", false, "Address 0x%08x is not in the ThreadPool!", ptr);
        return;
    }
    V_Assert(page->ThreadData!=nullptr, ("We must have valid page thread data for the page!"));
    ThreadPoolData* threadData = m_threadPoolGetter();
    if (threadData == page->ThreadData)
    {
        // we can free here
        threadData->AllocatorHandle.DeAllocate(ptr);
    }
    else
    {
        // push this element to be deleted from it's own thread!
        // cast the pointer to a fake lock free node
        Page::FakeNodeLF*   fakeLFNode = reinterpret_cast<Page::FakeNodeLF*>(ptr);
#ifdef V_DEBUG_BUILD
        // we need to reset the fakeLFNode because we share the memory.
        // otherwise we will assert the node is in the list
        fakeLFNode->m_next = 0;
#endif
        page->ThreadData->m_freedElements.push(*fakeLFNode);
    }
}

//=========================================================================
// AllocationSize
//=========================================================================
ThreadPoolSchema::size_type
ThreadPoolSchemaImpl::AllocationSize(ThreadPoolSchema::pointer_type ptr)
{
    Page* page = PageFromAddress(ptr);
    if (page==nullptr)
    {
        return 0;
    }
    V_Assert(page->ThreadData!=nullptr, ("We must have valid page thread data for the page!"));
    return page->ThreadData->AllocatorHandle.AllocationSize(ptr);
}

//=========================================================================
// PopFreePage
//=========================================================================
V_INLINE ThreadPoolSchemaImpl::Page*
ThreadPoolSchemaImpl::PopFreePage()
{
    Page* page;
    {
        VStd::lock_guard<VStd::recursive_mutex> lock(m_mutex);
        if (m_freePages.empty())
        {
            page = nullptr;
        }
        else
        {
            page = &m_freePages.front();
            m_freePages.pop_front();
        }
    }
    if (page)
    {
#   ifdef V_DEBUG_BUILD
        V_Assert(page->ThreadData == 0, "If we stored the free page properly we should have null here!");
#   endif
        // store the current thread data, used when we free elements
        page->ThreadData = m_threadPoolGetter();
    }
    return page;
}

//=========================================================================
// PushFreePage
//=========================================================================
V_INLINE void
ThreadPoolSchemaImpl::PushFreePage(Page* page)
{
#ifdef V_DEBUG_BUILD
    page->ThreadData = 0;
#endif
    {
        VStd::lock_guard<VStd::recursive_mutex> lock(m_mutex);
        m_freePages.push_front(*page);
    }
}

//=========================================================================
// GarbageCollect
//=========================================================================
void
ThreadPoolSchemaImpl::GarbageCollect()
{
    if (!m_isDynamic)
    {
        return;                // we have the memory statically allocated, can't collect garbage.
    }

    FreePagesType staticPages;
    VStd::lock_guard<VStd::recursive_mutex> lock(m_mutex);
    while (!m_freePages.empty())
    {
        Page* page = &m_freePages.front();
        m_freePages.pop_front();
        if (IsInStaticBlock(page))
        {
            staticPages.push_front(*page);
        }
        else
        {
            FreePage(page);
        }
    }
    while (!staticPages.empty())
    {
        Page* page = &staticPages.front();
        staticPages.pop_front();
        m_freePages.push_front(*page);
    }
}

//=========================================================================
// SetupFreeList
//=========================================================================
inline void
ThreadPoolSchemaImpl::Page::SetupFreeList(size_t elementSize, size_t pageDataBlockSize)
{
    char* pageData = reinterpret_cast<char*>(this) - pageDataBlockSize;
    FreeList.clear();
    // setup free list
    size_t numElements = pageDataBlockSize / elementSize;
    for (size_t i = 0; i < numElements; ++i)
    {
        char* address = pageData+i*elementSize;
        Page::FakeNode* node = new(address) Page::FakeNode();
        FreeList.push_back(*node);
    }
}

//=========================================================================
// ThreadPoolData::ThreadPoolData
//=========================================================================
ThreadPoolData::ThreadPoolData(ThreadPoolSchemaImpl* alloc, size_t pageSize, size_t minAllocationSize, size_t maxAllocationSize)
    : AllocatorHandle(alloc, pageSize, minAllocationSize, maxAllocationSize)
{}

//=========================================================================
// ThreadPoolData::~ThreadPoolData
//=========================================================================
ThreadPoolData::~ThreadPoolData()
{
    // deallocate elements if they were freed from other threads
    ThreadPoolSchemaImpl::Page::FakeNodeLF* fakeLFNode;
    while ((fakeLFNode = m_freedElements.pop())!=nullptr)
    {
        AllocatorHandle.DeAllocate(fakeLFNode);
    }
}