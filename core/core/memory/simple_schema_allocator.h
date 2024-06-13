#ifndef V_FRAMEWORK_CORE_MEMORY_SIMPLE_SCHEMA_ALLOCATOR_H
#define V_FRAMEWORK_CORE_MEMORY_SIMPLE_SCHEMA_ALLOCATOR_H

#include <core/base.h>
#include <core/std/base.h>
#include <core/std/typetraits/aligned_storage.h>
#include <core/std/typetraits/alignment_of.h>
#include <core/memory/allocator_base.h>
#include <core/debug/memory_profiler.h>

namespace V {
    template <class Schema, class DescriptorType=typename Schema::Descriptor, bool ProfileAllocations=true, bool ReportOutOfMemory=true>
    class SimpleSchemaAllocator : public AllocatorBase, public IAllocatorAllocate {
        public:
            using Descriptor = DescriptorType;
            using pointer_type = typename IAllocatorAllocate::pointer_type;
            using size_type = typename IAllocatorAllocate::size_type;
            using difference_type = typename IAllocatorAllocate::difference_type;
          SimpleSchemaAllocator(const char* name, const char* desc)
            : AllocatorBase(this, name, desc)
            , m_schema(nullptr) {
        }

        ~SimpleSchemaAllocator() override {
            if (m_schema) {
                reinterpret_cast<Schema*>(&m_schemaStorage)->~Schema();
            }
            m_schema = nullptr;
        }

        bool Create(const Descriptor& desc = Descriptor()) {
            m_schema = new (&m_schemaStorage) Schema(desc);
            return m_schema != nullptr;
        }

        //---------------------------------------------------------------------
        // IAllocator
        //---------------------------------------------------------------------
        void Destroy() override
        {
            reinterpret_cast<Schema*>(&m_schemaStorage)->~Schema();
            m_schema = nullptr;
        }

        AllocatorDebugConfig GetDebugConfig() override
        {
            return AllocatorDebugConfig();
        }

        IAllocatorAllocate* GetSchema() override
        {
            return m_schema;
        }

        //---------------------------------------------------------------------
        // IAllocatorAllocate
        //---------------------------------------------------------------------
        pointer_type Allocate(size_type byteSize, size_type alignment, int flags = 0, const char* name = nullptr, const char* fileName = nullptr, int lineNum = 0, unsigned int suppressStackRecord = 0) override {
            byteSize = MemorySizeAdjustedUp(byteSize);
            pointer_type ptr = m_schema->Allocate(byteSize, alignment, flags, name, fileName, lineNum, suppressStackRecord);

            if (ProfileAllocations) {
                V_PROFILE_MEMORY_ALLOC_EX(MemoryReserved, fileName, lineNum, ptr, byteSize, name ? name : GetName());
                V_MEMORY_PROFILE(ProfileAllocation(ptr, byteSize, alignment, name, fileName, lineNum, suppressStackRecord));
            }

            V_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option") // conditional expression is constant
            if (ReportOutOfMemory && !ptr)
            V_POP_DISABLE_WARNING {
                OnOutOfMemory(byteSize, alignment, flags, name, fileName, lineNum);
            }

            return ptr;
        }

        void DeAllocate(pointer_type ptr, size_type byteSize = 0, size_type alignment = 0) override {
            byteSize = MemorySizeAdjustedUp(byteSize);

            if (ProfileAllocations) {
                V_PROFILE_MEMORY_FREE(MemoryReserved, ptr);
                V_MEMORY_PROFILE(ProfileDeallocation(ptr, byteSize, alignment, nullptr));
            }

            m_schema->DeAllocate(ptr, byteSize, alignment);
        }

        size_type Resize(pointer_type ptr, size_type newSize) override {
            newSize = MemorySizeAdjustedUp(newSize);
            size_t result = m_schema->Resize(ptr, newSize);

            if (ProfileAllocations)
            {
                V_MEMORY_PROFILE(ProfileResize(ptr, result));
            }

            // Failure to resize an existing pointer does not indicate out-of-memory, so we do not check for it here

            return result;
        }

        pointer_type ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) override {
            if (ProfileAllocations) {
                V_PROFILE_MEMORY_FREE(MemoryReserved, ptr);
            }

            newSize = MemorySizeAdjustedUp(newSize);

            if (ProfileAllocations) {
                V_MEMORY_PROFILE(ProfileReallocationBegin(ptr, newSize));
            }

            pointer_type newPtr = m_schema->ReAllocate(ptr, newSize, newAlignment);

            if (ProfileAllocations) {
                V_PROFILE_MEMORY_ALLOC(MemoryReserved, newPtr, newSize, GetName());
                V_MEMORY_PROFILE(ProfileReallocationEnd(ptr, newPtr, newSize, newAlignment));
            }

            V_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option") // conditional expression is constant
            if (ReportOutOfMemory && newSize && !newPtr)
            V_POP_DISABLE_WARNING {
                OnOutOfMemory(newSize, newAlignment, 0, nullptr, nullptr, 0);
            }

            return newPtr;
        }
                
        size_type AllocationSize(pointer_type ptr) override {
            return MemorySizeAdjustedDown(m_schema->AllocationSize(ptr));
        }
        
        void GarbageCollect() override {
            m_schema->GarbageCollect();
        }

        size_type NumAllocatedBytes() const override {
            return m_schema->NumAllocatedBytes();
        }

        size_type Capacity() const override {
            return m_schema->Capacity();
        }
        
        size_type GetMaxAllocationSize() const override { 
            return m_schema->GetMaxAllocationSize();
        }

        size_type GetMaxContiguousAllocationSize() const override {
            return m_schema->GetMaxContiguousAllocationSize();
        }

        size_type GetUnAllocatedMemory(bool isPrint = false) const override { 
            return m_schema->GetUnAllocatedMemory(isPrint);
        }
        
        IAllocatorAllocate* GetSubAllocator() override {
            return m_schema->GetSubAllocator();
        }

    protected:
        IAllocatorAllocate* m_schema;

    private:
        typename VStd::aligned_storage<sizeof(Schema), VStd::alignment_of<Schema>::value>::type m_schemaStorage;
    };
}

#endif // V_FRAMEWORK_CORE_MEMORY_SIMPLE_SCHEMA_ALLOCATOR_H