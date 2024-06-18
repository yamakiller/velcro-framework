#ifndef V_FRAMEWORK_CORE_MEMORY_MEMORY_DETECTOR_H
#define V_FRAMEWORK_CORE_MEMORY_MEMORY_DETECTOR_H

#include <core/detector/detector.h>
#include <core/memory/memory_detector_bus.h>

namespace V
{
    namespace Debug
    {
        struct StackFrame;

        /**
         * Trace messages driller class
         */
        class MemoryDetector
            : public Detector
            , public MemoryDetectorBus::Handler
        {
        public:
           V_CLASS_ALLOCATOR(MemoryDetector, OSAllocator, 0)

            // TODO: Centralized settings for memory tracking.
            struct Descriptor {
            };

            MemoryDetector(const Descriptor& desc = Descriptor());
            ~MemoryDetector();

        protected:
            //////////////////////////////////////////////////////////////////////////
            // Detector
            const char*  GroupName() const override          { return "SystemDetectors"; }
            const char*  GetName() const override            { return "MemoryDetector"; }
            const char*  GetDescription() const override     { return "Reports all allocators and memory allocations."; }
            void         Start(const Param* params = NULL, int numParams = 0) override;
            void         Stop() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // MemoryDrillerBus
            void RegisterAllocator(IAllocator* allocator) override;
            void UnregisterAllocator(IAllocator* allocator) override;

            void RegisterAllocation(IAllocator* allocator, void* address, size_t byteSize, size_t alignment, const char* name, const char* fileName, int lineNum, unsigned int stackSuppressCount) override;
            void UnregisterAllocation(IAllocator* allocator, void* address, size_t byteSize, size_t alignment, AllocationInfo* info) override;
            void ReallocateAllocation(IAllocator* allocator, void* prevAddress, void* newAddress, size_t newByteSize, size_t newAlignment) override;
            void ResizeAllocation(IAllocator* allocator, void* address, size_t newSize) override;

            void DumpAllAllocations() override;
            //////////////////////////////////////////////////////////////////////////

            void RegisterAllocatorOutput(IAllocator* allocator);
            void RegisterAllocationOutput(IAllocator* allocator, void* address, const AllocationInfo* info);
        private:
            // Store a list of all of our allocator records so we can dump them all without having to know about the allocators
            VStd::list<Debug::AllocationRecords*, OSStdAllocator> m_allAllocatorRecords;
        };
    } // namespace Debug
} // namespace V


#endif // V_FRAMEWORK_CORE_MEMORY_MEMORY_DETECTOR_H