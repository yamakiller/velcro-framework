
#include <vcore/io/istreamer_types.h>
#include <vcore/memory/system_allocator.h>

namespace V::IO::IStreamerTypes
{
    V::u64 Recommendations::CalculateRecommendedMemorySize(u64 readSize, u64 readOffset)
    {
        u64 offsetAdjustment = readOffset - V_SIZE_ALIGN_DOWN(readOffset, m_sizeAlignment);
        return V_SIZE_ALIGN_UP((readSize + offsetAdjustment), m_sizeAlignment);
    }

    DefaultRequestMemoryAllocator::DefaultRequestMemoryAllocator()
        : m_allocator(V::AllocatorInstance<V::SystemAllocator>::Get())
    {}

    DefaultRequestMemoryAllocator::DefaultRequestMemoryAllocator(V::IAllocatorAllocate& allocator)
        : m_allocator(allocator)
    {}

    DefaultRequestMemoryAllocator::~DefaultRequestMemoryAllocator()
    {
        V_Assert(m_lockCounter == 0, "There are still %i file requests using this allocator.", GetNumLocks());
        V_Assert(m_allocationCounter == 0, "There are still %i allocations from this allocator.", static_cast<int>(m_allocationCounter));
    }

    void DefaultRequestMemoryAllocator::LockAllocator()
    {
        m_lockCounter++;
    }

    void DefaultRequestMemoryAllocator::UnlockAllocator()
    {
        m_lockCounter--;
    }

    RequestMemoryAllocatorResult DefaultRequestMemoryAllocator::Allocate([[maybe_unused]] u64 minimalSize, u64 recommendedSize, size_t alignment)
    {
        RequestMemoryAllocatorResult result;
        result.Size = recommendedSize;
        result.Type = MemoryType::ReadWrite;
        result.Address = (recommendedSize > 0) ?
            m_allocator.Allocate(recommendedSize, alignment, 0, "DefaultRequestMemoryAllocator", __FILE__, __LINE__) :
            nullptr;

#ifdef V_ENABLE_TRACING
        if (result.Address)
        {
            m_allocationCounter++;
        }
#endif

        return result;
    }

    void DefaultRequestMemoryAllocator::Release(void* address)
    {
        if (address)
        {
#ifdef V_ENABLE_TRACING
            m_allocationCounter--;
#endif

            m_allocator.DeAllocate(address);
        }
    }

    int DefaultRequestMemoryAllocator::GetNumLocks() const
    {
        return m_lockCounter;
    }
} // namespace V::IO::IStreamer