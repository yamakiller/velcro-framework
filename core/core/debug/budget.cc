#include <core/debug/budget.h>

#include <core/module/environment.h>
#include <core/math/crc.h>
#include <core/memory/system_allocator.h>
#include <core/statistics/statistical_profiler_proxy.h>

V_DEFINE_BUDGET(Core);
V_DEFINE_BUDGET(Editor);
V_DEFINE_BUDGET(System);

namespace V::Debug
{
    struct BudgetImpl
    {
        V_CLASS_ALLOCATOR(BudgetImpl, V::SystemAllocator, 0);
        // TODO: Budget implementation for tracking budget wall time per-core, memory, etc.
    };

    Budget::Budget(const char* name)
        : Budget( name, Crc32(name) )
    {
    }

    Budget::Budget(const char* name, uint32_t crc)
        : m_name{ name }
        , m_crc{ crc }
    {
        m_impl = vnew BudgetImpl;
        if (auto statsProfiler = Interface<Statistics::StatisticalProfilerProxy>::Get("StatisticalProfilerProxy"); statsProfiler)
        {
            statsProfiler->RegisterProfilerId(m_crc);
        }
    }

    Budget::~Budget()
    {
        if (m_impl)
        {
            delete m_impl;
        }
    }

    // TODO:Budgets Methods below are stubbed pending future work to both update budget data and visualize it

    void Budget::PerFrameReset()
    {
    }

    void Budget::BeginProfileRegion()
    {
    }

    void Budget::EndProfileRegion()
    {
    }

    void Budget::TrackAllocation(uint64_t)
    {
    }

    void Budget::UntrackAllocation(uint64_t)
    {
    }
} // namespace V::Debug
