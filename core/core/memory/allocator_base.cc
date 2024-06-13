#include <core/memory/allocator_base.h>

using namespace V;

AllocatorBase::AllocatorBase(IAllocatorAllocate* allocationSource, const char* name, const char* desc) :
    IAllocator(allocationSource),
    m_name(name),
    m_desc(desc) {}