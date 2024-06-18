#include <core/debug/budget_tracker.h>

#include <core/base.h>
#include <core/debug/budget.h>
#include <core/interface/interface.h>
#include <core/memory/memory.h>
#include <core/std/containers/unordered_map.h>
#include <core/std/parallel/scoped_lock.h>


namespace V::Debug
{
    struct BudgetTracker::BudgetTrackerImpl
    {
        VStd::unordered_map<const char*, Budget> m_budgets;
    };

    Budget* BudgetTracker::GetBudgetFromEnvironment(const char* budgetName, uint32_t crc)
    {
        BudgetTracker* tracker = Interface<BudgetTracker>::Get("BudgetTracker");
        if (tracker)
        {
            return &tracker->GetBudget(budgetName, crc);
        }
        return nullptr;
    }

    BudgetTracker::~BudgetTracker()
    {
        Reset();
    }

    bool BudgetTracker::Init()
    {
        if (Interface<BudgetTracker>::Get("BudgetTracker"))
        {
            return false;
        }

        Interface<BudgetTracker>::Register("BudgetTracker", this);
        m_impl = new BudgetTrackerImpl;
        return true;
    }

    void BudgetTracker::Reset()
    {
        if (m_impl)
        {
            Interface<BudgetTracker>::Unregister("BudgetTracker");
            delete m_impl;
            m_impl = nullptr;
        }
    }

    Budget& BudgetTracker::GetBudget(const char* budgetName, uint32_t crc)
    {
        VStd::scoped_lock lock{ m_mutex };

        auto it = m_impl->m_budgets.try_emplace(budgetName, budgetName, crc).first;

        return it->second;
    }
} // namespace V::Debug
