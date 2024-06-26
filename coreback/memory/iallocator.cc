#include <vcore/memory/iallocator.h>
namespace V {
    IAllocator::IAllocator(IAllocatorAllocate* allocationSource)
        : m_allocationSource(allocationSource)
        , m_originalAllocationSource(allocationSource) {
    }

    IAllocator::~IAllocator() {
    }

    void IAllocator::SetAllocationSource(IAllocatorAllocate* allocationSource) {
        m_allocationSource = allocationSource;
    }

    void IAllocator::ResetAllocationSource() {
        m_allocationSource = m_originalAllocationSource;
    }
}