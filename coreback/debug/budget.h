#ifndef V_FRAMEWORK_CORE_DEBUG_BUGET_H
#define V_FRAMEWORK_CORE_DEBUG_BUGET_H

#include <vcore/debug/budget_tracker.h>
#include <vcore/math/crc.h>

namespace V::Debug {
     // A budget collates per-frame resource utilization and memory for a particular category
    class Budget final
    {
    public:
        explicit Budget(const char* name);
        Budget(const char* name, uint32_t crc);
        ~Budget();

        void PerFrameReset();
        void BeginProfileRegion();
        void EndProfileRegion();
        void TrackAllocation(uint64_t bytes);
        void UntrackAllocation(uint64_t bytes);

        const char* Name() const
        {
            return m_name;
        }

        uint32_t Crc() const
        {
            return m_crc;
        }

    private:
        const char* m_name;
        const uint32_t m_crc;
        struct BudgetImpl* m_impl = nullptr;
    };
} // namespace V::Debug


// The budget is usable in the same file it was defined without needing an additional declaration.
// If you encounter a linker error complaining that this function is not defined, you have likely forgotten to either
// define or declare the budget used in a profile or memory marker. See V_DEFINE_BUDGET and V_DECLARE_BUDGET below
// for usage.
#define V_BUDGET_GETTER(name) GetAzBudget##name

#if defined(_RELEASE)
#define V_DEFINE_BUDGET(name)                                                                                                              \
    ::V::Debug::Budget* V_BUDGET_GETTER(name)()                                                                                            \
    {                                                                                                                                      \
        return nullptr;                                                                                                                    \
    }
#else
// Usage example:
// In a single C++ source file:
// V_DEFINE_BUDGET(core);
//
// Anywhere the budget is used, the budget must be declared (either in a header or in the source file itself)
// V_DECLARE_BUDGET(core);
#define V_DEFINE_BUDGET(name)                                                                                                             \
    ::V::Debug::Budget* V_BUDGET_GETTER(name)()                                                                                           \
    {                                                                                                                                     \
        constexpr static uint32_t crc = V_CRC_CE(#name);                                                                                  \
        static ::V::Debug::Budget* budget = ::V::Debug::BudgetTracker::GetBudgetFromEnvironment(#name, crc);                              \
        return budget;                                                                                                                    \
    }
#endif

// If using a budget defined in a different C++ source file, add V_DECLARE_BUDGET(yourBudget); somewhere in your source file at namespace
// scope Alternatively, V_DECLARE_BUDGET can be used in a header to declare the budget for use across any users of the header
#define V_DECLARE_BUDGET(name) ::V::Debug::Budget* V_BUDGET_GETTER(name)()

V_DECLARE_BUDGET(Core);
V_DECLARE_BUDGET(Editor);
V_DECLARE_BUDGET(System);


#endif // V_FRAMEWORK_CORE_DEBUG_BUGET_H