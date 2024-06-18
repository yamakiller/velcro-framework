#ifndef V_FRAMEWORK_CORE_DEBUG_BUDGET_TRACKER_H
#define V_FRAMEWORK_CORE_DEBUG_BUDGET_TRACKER_H


#include <core/module/environment.h>
#include <core/std/parallel/mutex.h>

namespace V::Debug {
    class Budget;

    class BudgetTracker {
        struct BudgetTrackerImpl;

    public:
        static Budget* GetBudgetFromEnvironment(const char* budgetName, uint32_t crc);

        ~BudgetTracker();

        // Returns false if the budget tracker was already present in the environment (initialized already elsewhere)
        bool Init();
        void Reset();

        Budget& GetBudget(const char* budgetName, uint32_t crc);

    private:
        VStd::mutex m_mutex;

        // The BudgetTracker is likely included in proportionally high number of files throughout the
        // engine, so indirection is used here to avoid imposing excessive recompilation in periods
        // while the budget system is iterated on.
        BudgetTrackerImpl* m_impl = nullptr;
    };
}

#endif // V_FRAMEWORK_CORE_DEBUG_BUDGET_TRACKER_H