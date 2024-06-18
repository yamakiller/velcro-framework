#ifndef V_FRAMEWORK_CORE_MEMORY_MEMORY_DETECTOR_BUS_H
#define V_FRAMEWORK_CORE_MEMORY_MEMORY_DETECTOR_BUS_H


#include <core/detector/detector_bus.h>

namespace V {
    class IAllocator;
    namespace Debug
    {
        //class AllocationRecords;
        struct AllocationInfo;

        /**
         * Memory allocations detector message.
         *
         * We use a detector bus so all messages are sending in exclusive matter no other detector messages
         * can be triggered at that moment, so we already preserve the calling order. You can assume
         * all access code in the detector framework in guarded. You can manually lock the detector mutex are you
         * use by using \ref V::Debug::DetectorEventBusMutex.
         */
        class MemoryDetectorMessages : public V::Debug::DetectorEventBusTraits
        {
        public:
            virtual ~MemoryDetectorMessages() {}

            /// Register allocation (with customizable tracking settings - TODO: we should centralize this settings and remove them from here)
            virtual void RegisterAllocator(IAllocator* allocator) = 0;
            virtual void UnregisterAllocator(IAllocator* allocator) = 0;

            virtual void RegisterAllocation(IAllocator* allocator, void* address, size_t byteSize, size_t alignment, const char* name, const char* fileName, int lineNum, unsigned int stackSuppressCount) = 0;
            virtual void UnregisterAllocation(IAllocator* allocator, void* address, size_t byteSize, size_t alignment, AllocationInfo* info) = 0;
            virtual void ReallocateAllocation(IAllocator* allocator, void* prevAddress, void* newAddress, size_t newByteSize, size_t newAlignment) = 0;
            virtual void ResizeAllocation(IAllocator* allocator, void* address, size_t newSize) = 0;

            virtual void DumpAllAllocations() = 0;
        };

        typedef V::EventBus<MemoryDetectorMessages> MemoryDetectorBus;
    } // namespace Debug
} // namespace V


#endif // V_FRAMEWORK_CORE_MEMORY_MEMORY_DETECTOR_BUS_H