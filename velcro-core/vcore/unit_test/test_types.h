#ifndef V_FRAMEWORK_CORE_UNITTEST_USERTYPES_H
#define V_FRAMEWORK_CORE_UNITTEST_USERTYPES_H

#include <vcore/base.h>
#include <vcore/unit_test/unit_test.h>

#include <vcore/debug/budget_tracker.h>
#include <vcore/memory/system_allocator.h>
#include <vcore/detector/detector.h>
#include <vcore/memory/memory_detector.h>
#include <vcore/memory/allocator_records.h>

#if defined(HAVE_BENCHMARK)

#if defined(V_COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif // clang

//#include <benchmark/benchmark.h>

#if defined(V_COMPILER_CLANG)
#pragma clang diagnostic pop
#endif // clang

#endif // HAVE_BENCHMARK

namespace UnitTest
{
    /**
    * Base class to share common allocator code between fixture types.
    */
    class AllocatorsBase
    {
        public:
            virtual ~AllocatorsBase() = default;

            void SetupAllocator(const V::SystemAllocator::Descriptor& allocatorDesc = {})
            {
                m_detectorManager = V::Debug::DetectorManager::Create();
                m_detectorManager->Register(vnew V::Debug::MemoryDetector);
                V::AllocatorManager::Instance().SetDefaultTrackingMode(V::Debug::AllocationRecords::RECORD_FULL);

                // Only create the SystemAllocator if it s not ready
                if (!V::AllocatorInstance<V::SystemAllocator>::IsReady())
                {
                    V::AllocatorInstance<V::SystemAllocator>::Create(allocatorDesc);
                    m_ownsAllocator = true;
                }
            }

            void TeardownAllocator()
            {
                // Don't destroy the SystemAllocator if it is not ready aand was not created by
                // the AllocatorsBase
                if (m_ownsAllocator && V::AllocatorInstance<V::SystemAllocator>::IsReady())
                {
                    V::AllocatorInstance<V::SystemAllocator>::Destroy();
                }
                m_ownsAllocator = false;
                V::Debug::DetectorManager::Destroy(m_detectorManager);

                V::AllocatorManager::Instance().SetDefaultTrackingMode(V::Debug::AllocationRecords::RECORD_NO_RECORDS);
            }
        private:
            V::Debug::DetectorManager* m_detectorManager;
            bool m_ownsAllocator{};
    };

    /**
    * RAII wrapper of AllocatorBase.
    * The benefit of using this wrapper instead of AllocatorsTestFixture is that SetUp/TearDown of the allocator is managed
    * on construction/destruction, allowing member variables of derived classes to exist as value (and do heap allocation).
    */
    class ScopedAllocatorSetupFixture  : public ::testing::Test
                                , AllocatorsBase
    {
    public:
        ScopedAllocatorSetupFixture() { SetupAllocator(); }
        explicit ScopedAllocatorSetupFixture(const V::SystemAllocator::Descriptor& allocatorDesc) { SetupAllocator(allocatorDesc); }
        ~ScopedAllocatorSetupFixture() { TeardownAllocator(); }
    };

     /**
    * Helper class to handle the boiler plate of setting up a test fixture that uses the system allocators
    * If you wish to do additional setup and tear down be sure to call the base class SetUp first and TearDown
    * last.
    * By default memory tracking through detector is enabled.
    * Defaults to a heap size of 15 MB
    */

    class AllocatorsTestFixture
        : public ::testing::Test
        , public AllocatorsBase
    {
    public:
        //GTest interface
        void SetUp() override
        {
            SetupAllocator();
        }

        void TearDown() override
        {
            TeardownAllocator();
        }
    };

    using AllocatorsFixture = AllocatorsTestFixture;
    #if defined(HAVE_BENCHMARK)
          /**
    * Helper class to handle the boiler plate of setting up a benchmark fixture that uses the system allocators
    * If you wish to do additional setup and tear down be sure to call the base class SetUp first and TearDown
    * last.
    * By default memory tracking through driller is disabled.
    * Defaults to a heap size of 15 MB
    */
    class AllocatorsBenchmarkFixture
        : public ::benchmark::Fixture
        , public AllocatorsBase
    {
    public:
        //Benchmark interface
        void SetUp(const ::benchmark::State& st) override
        {
            V_UNUSED(st);
            SetupAllocator();
        }
        void SetUp(::benchmark::State& st) override
        {
            V_UNUSED(st);
            SetupAllocator();
        }

        void TearDown(const ::benchmark::State& st) override
        {
            V_UNUSED(st);
            TeardownAllocator();
        }
        void TearDown(::benchmark::State& st) override
        {
            V_UNUSED(st);
            TeardownAllocator();
        }
    };
    #endif // HAVE_BENCHMARK

    class DLLTestVirtualClass
    {
    public:
        DLLTestVirtualClass()
            : m_data(1) {}
        virtual ~DLLTestVirtualClass() {}

        int m_data;
    };

    template <V::u32 size, V::u8 instance, size_t alignment = 16>
    struct CreationCounter
    {
        alignas(alignment) int test[size / sizeof(int)];

        static int _count;
        static int _copied;
        static int _moved;
        CreationCounter(int def = 0)
        {
            ++_count;
            test[0] = def;
        }

        CreationCounter(VStd::initializer_list<int> il)
        {
            ++_count;
            if (il.size() > 0)
            {
                test[0] = *il.begin();
            }
        }
        CreationCounter(const CreationCounter& rhs)
            : CreationCounter()
        {
            memcpy(test, rhs.test, V_ARRAY_SIZE(test));
            ++_copied;
        }
        CreationCounter(CreationCounter&& rhs)
            : CreationCounter()
        {
            memmove(test, rhs.test, V_ARRAY_SIZE(test));
            ++_moved;
        }

        ~CreationCounter()
        {
            --_count;
        }

        const int& val() const { return test[0]; }
        int& val() { return test[0]; }

        static void Reset()
        {
            _count = 0;
            _copied = 0;
            _moved = 0;
        }
    private:
        // The test member variable has configurable alignment and this class has configurable size
        // In some cases (when the size < alignment) padding is required. To avoid the compiler warning
        // us about the padding being added, we added it here for the cases that is required.
        // '% alignment' in the padding is redundant, but it fixes an 'array is too large' clang error.
        static constexpr bool sHasPadding = size < alignment;
        VStd::enable_if<sHasPadding, char[(alignment - size) % alignment]> mPadding;
    };
    
    template <V::u32 size, V::u8 instance, size_t alignment>
    int CreationCounter<size, instance, alignment>::_count = 0;
    template <V::u32 size, V::u8 instance, size_t alignment>
    int CreationCounter<size, instance, alignment>::_copied = 0;
    template <V::u32 size, V::u8 instance, size_t alignment>
    int CreationCounter<size, instance, alignment>::_moved = 0;
}
#endif //V_FRAMEWORK_CORE_UNITTEST_USERTYPES_H
