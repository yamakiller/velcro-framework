#include <vcore/memory/memory_detector.h>
#include <vcore/math/crc.h>

#include <vcore/memory/allocator_manager.h>
#include <vcore/memory/allocator_records.h>

#include <vcore/io/generic_streams.h>
#include <vcore/debug/stack_tracer.h>

namespace V
{
    namespace Debug
    {
        //=========================================================================
        // MemoryDetector
        //=========================================================================
        MemoryDetector::MemoryDetector(const Descriptor& desc)
        {
            (void)desc;
            BusConnect();

            AllocatorManager::Instance().EnterProfilingMode();

            {
                // Register all allocators that were created before the driller existed
                auto allocatorLock = AllocatorManager::Instance().LockAllocators();

                for (int i = 0; i < AllocatorManager::Instance().GetNumAllocators(); ++i)
                {
                    IAllocator* allocator = AllocatorManager::Instance().GetAllocator(i);
                    RegisterAllocator(allocator);
                }
            }
        }

        //=========================================================================
        // ~MemoryDetector
        //=========================================================================
        MemoryDetector::~MemoryDetector()
        {
            BusDisconnect();
            AllocatorManager::Instance().ExitProfilingMode();
        }

        //=========================================================================
        // Start
        //=========================================================================
        void MemoryDetector::Start(const Param* params, int numParams)
        {
            (void)params;
            (void)numParams;

            // dump current allocations for all allocators with tracking
            auto allocatorLock = AllocatorManager::Instance().LockAllocators();
            for (int i = 0; i < AllocatorManager::Instance().GetNumAllocators(); ++i)
            {
                IAllocator* allocator = AllocatorManager::Instance().GetAllocator(i);
                if (auto records = allocator->GetRecords())
                {
                    RegisterAllocatorOutput(allocator);
                    const AllocationRecordsType& allocMap = records->GetMap();
                    for (AllocationRecordsType::const_iterator allocIt = allocMap.begin(); allocIt != allocMap.end(); ++allocIt)
                    {
                        RegisterAllocationOutput(allocator, allocIt->first, &allocIt->second);
                    }
                }
            }
        }

        //=========================================================================
        // Stop
        //=========================================================================
        void MemoryDetector::Stop()
        {
        }

        //=========================================================================
        // RegisterAllocator
        //=========================================================================
        void MemoryDetector::RegisterAllocator(IAllocator* allocator)
        {
            // Ignore if our allocator is already registered
            if (allocator->GetRecords() != nullptr)
            {
                return;
            }

            auto debugConfig = allocator->GetDebugConfig();

            if (!debugConfig.ExcludeFromDebugging)
            {
                allocator->SetRecords(vnew Debug::AllocationRecords((unsigned char)debugConfig.StackRecordLevels, debugConfig.UsesMemoryGuards, debugConfig.MarksUnallocatedMemory, allocator->GetName()));

                m_allAllocatorRecords.push_back(allocator->GetRecords());

                if (m_output == nullptr)
                {
                    return;                    // we have no active output
                }
                RegisterAllocatorOutput(allocator);
            }
        }
        //=========================================================================
        // RegisterAllocatorOutput
        //=========================================================================
        void MemoryDetector::RegisterAllocatorOutput(IAllocator* allocator)
        {
            auto records = allocator->GetRecords();
            m_output->BeginTag(V_CRC("MemoryDetector", 0x1b31269d));
            m_output->BeginTag(V_CRC("RegisterAllocator", 0x19f08114));
            m_output->Write(V_CRC("Name", 0x5e237e06), allocator->GetName());
            m_output->Write(V_CRC("Id", 0xbf396750), allocator);
            m_output->Write(V_CRC("Capacity", 0xb5e8b174), allocator->GetAllocationSource()->Capacity());
            m_output->Write(V_CRC("RecordsId", 0x7caaca88), records);
            if (records)
            {
                m_output->Write(V_CRC("RecordsMode", 0x764c147a), (char)records->GetMode());
                m_output->Write(V_CRC("NumStackLevels", 0xad9cff15), records->GetNumStackLevels());
            }
            m_output->EndTag(V_CRC("RegisterAllocator", 0x19f08114));
            m_output->EndTag(V_CRC("MemoryDetector", 0x1b31269d));
        }

        //=========================================================================
        // UnregisterAllocator
        //=========================================================================
        void MemoryDetector::UnregisterAllocator(IAllocator* allocator)
        {
            auto allocatorRecords = allocator->GetRecords();
            V_Assert(allocatorRecords, "This allocator is not registered with the memory driller!");
            for (auto records : m_allAllocatorRecords)
            {
                if (records == allocatorRecords)
                {
                    m_allAllocatorRecords.remove(records);
                    break;
                }
            }
            delete allocatorRecords;
            allocator->SetRecords(nullptr);

            if (m_output == nullptr)
            {
                return;                    // we have no active output
            }
            m_output->BeginTag(V_CRC("MemoryDetector", 0x1b31269d));
            m_output->Write(V_CRC("UnregisterAllocator", 0xb2b54f93), allocator);
            m_output->EndTag(V_CRC("MemoryDetector", 0x1b31269d));
        }

        //=========================================================================
        // RegisterAllocation
        //=========================================================================
        void MemoryDetector::RegisterAllocation(IAllocator* allocator, void* address, size_t byteSize, size_t alignment, const char* name, const char* fileName, int lineNum, unsigned int stackSuppressCount)
        {
            auto records = allocator->GetRecords();
            if (records)
            {
                const AllocationInfo* info = records->RegisterAllocation(address, byteSize, alignment, name, fileName, lineNum, stackSuppressCount + 1);
                if (m_output == nullptr)
                {
                    return;                   // we have no active output
                }
                RegisterAllocationOutput(allocator, address, info);
            }
        }

        //=========================================================================
        // RegisterAllocationOutput
        //=========================================================================
        void MemoryDetector::RegisterAllocationOutput(IAllocator* allocator, void* address, const AllocationInfo* info)
        {
            auto records = allocator->GetRecords();
            m_output->BeginTag(V_CRC("MemoryDetector", 0x1b31269d));
            m_output->BeginTag(V_CRC("RegisterAllocation", 0x992a9780));
            m_output->Write(V_CRC("RecordsId", 0x7caaca88), records);
            m_output->Write(V_CRC("Address", 0x0d4e6f81), address);
            if (info)
            {
                if (info->Name)
                {
                    m_output->Write(V_CRC("Name", 0x5e237e06), info->Name);
                }
                m_output->Write(V_CRC("Alignment", 0x2cce1e5c), info->Alignment);
                m_output->Write(V_CRC("Size", 0xf7c0246a), info->ByteSize);
                if (info->FileName)
                {
                    m_output->Write(V_CRC("FileName", 0x3c0be965), info->FileName);
                    m_output->Write(V_CRC("FileLine", 0xb33c2395), info->LineNum);
                }
                // copy the stack frames directly, resolving the stack should happen later as this is a SLOW procedure.
                if (info->StackFrames)
                {
                    m_output->Write(V_CRC("Stack", 0x41a87b6a), info->StackFrames, info->StackFrames + records->GetNumStackLevels());
                }
            }
            m_output->EndTag(V_CRC("RegisterAllocation", 0x992a9780));
            m_output->EndTag(V_CRC("MemoryDetector", 0x1b31269d));
        }

        //=========================================================================
        // UnRegisterAllocation
        //=========================================================================
        void MemoryDetector::UnregisterAllocation(IAllocator* allocator, void* address, size_t byteSize, size_t alignment, AllocationInfo* info)
        {
            auto records = allocator->GetRecords();
            if (records)
            {
                records->UnregisterAllocation(address, byteSize, alignment, info);

                if (m_output == nullptr)
                {
                    return;                    // we have no active output
                }
                m_output->BeginTag(V_CRC("MemoryDetector", 0x1b31269d));
                m_output->BeginTag(V_CRC("UnRegisterAllocation", 0xea5dc4cd));
                m_output->Write(V_CRC("RecordsId", 0x7caaca88), records);
                m_output->Write(V_CRC("Address", 0x0d4e6f81), address);
                m_output->EndTag(V_CRC("UnRegisterAllocation", 0xea5dc4cd));
                m_output->EndTag(V_CRC("MemoryDetector", 0x1b31269d));
            }
        }

        //=========================================================================
        // ReallocateAllocation
        //=========================================================================
        void MemoryDetector::ReallocateAllocation(IAllocator* allocator, void* prevAddress, void* newAddress, size_t newByteSize, size_t newAlignment)
        {
            AllocationInfo info;
            UnregisterAllocation(allocator, prevAddress, 0, 0, &info);
            RegisterAllocation(allocator, newAddress, newByteSize, newAlignment, info.Name, info.FileName, info.LineNum, 0);
        }

        //=========================================================================
        // ResizeAllocation
        //=========================================================================
        void MemoryDetector::ResizeAllocation(IAllocator* allocator, void* address, size_t newSize)
        {
            auto records = allocator->GetRecords();
            if (records)
            {
                records->ResizeAllocation(address, newSize);

                if (m_output == nullptr)
                {
                    return;                    // we have no active output
                }
                m_output->BeginTag(V_CRC("MemoryDetector", 0x1b31269d));
                m_output->BeginTag(V_CRC("ResizeAllocation", 0x8a9c78dc));
                m_output->Write(V_CRC("RecordsId", 0x7caaca88), records);
                m_output->Write(V_CRC("Address", 0x0d4e6f81), address);
                m_output->Write(V_CRC("Size", 0xf7c0246a), newSize);
                m_output->EndTag(V_CRC("ResizeAllocation", 0x8a9c78dc));
                m_output->EndTag(V_CRC("MemoryDetector", 0x1b31269d));
            }
        }

        void MemoryDetector::DumpAllAllocations()
        {
            // Create a copy so allocations done during the printing dont end up affecting the container
            const VStd::list<Debug::AllocationRecords*, OSStdAllocator> allocationRecords = m_allAllocatorRecords;

            for (auto records : allocationRecords)
            {
                // Skip if we have had no allocations made
                if (records->RequestedAllocs())
                {
                    records->EnumerateAllocations(V::Debug::PrintAllocationsCB(true, true));
                }
            }
        }

    }// namespace Debug
} // namespace AZ