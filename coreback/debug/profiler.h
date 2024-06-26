#ifndef V_FRAMEWORK_CORE_DEBUG_PROFILER_H
#define V_FRAMEWORK_CORE_DEBUG_PROFILER_H

#include <vcore/debug/budget.h>
#include <vcore/statistics/statistical_profiler_proxy.h>

#ifdef USE_PIX
#include <vcore/platform_incl.h>
#include <WinPixEventRuntime/pix3.h>
#endif

#if defined(V_PROFILER_MACRO_DISABLE) // by default we never disable the profiler registers as their overhead should be minimal, you can
                                       // still do that for your code though.
    #define V_PROFILE_SCOPE(...)
    #define V_PROFILE_FUNCTION(...)
    #define V_PROFILE_BEGIN(...)
    #define V_PROFILE_END(...)
#else
    /**
     * Macro to declare a profile section for the current scope { }.
     * format is: V_PROFILE_SCOPE(categoryName, const char* formatStr, ...)
     */
    #define V_PROFILE_SCOPE(budget, ...)                                                                                                \
    ::V::Debug::ProfileScope V_JOIN(vProfileScope, __LINE__)                                                                            \
    {                                                                                                                                   \
        V_BUDGET_GETTER(budget)(), __VA_ARGS__                                                                                          \
    }

    #define V_PROFILE_FUNCTION(category) V_PROFILE_SCOPE(category, V_FUNCTION_SIGNATURE)

    // Prefer using the scoped macros which automatically end the event (V_PROFILE_SCOPE/V_PROFILE_FUNCTION)
    #define V_PROFILE_BEGIN(budget, ...) ::V::Debug::ProfileScope::BeginRegion(V_BUDGET_GETTER(budget)(), __VA_ARGS__)
    #define V_PROFILE_END(budget) ::V::Debug::ProfileScope::EndRegion(V_BUDGET_GETTER(budget)())

#endif // V_PROFILER_MACRO_DISABLE

#ifndef V_PROFILE_INTERVAL_START
#define V_PROFILE_INTERVAL_START(...)
#define V_PROFILE_INTERVAL_START_COLORED(...)
#define V_PROFILE_INTERVAL_END(...)
#define V_PROFILE_INTERVAL_SCOPED(budget, scopeNameId, ...) \
    static constexpr V::Crc32 V_JOIN(blockId, __LINE__)(scopeNameId); \
    V::Statistics::StatisticalProfilerProxy::TimedScope V_JOIN(scope, __LINE__)(V_CRC_CE(#budget), V_JOIN(blockId, __LINE__));
#endif

#ifndef V_PROFILE_DATAPOINT
#define V_PROFILE_DATAPOINT(...)
#define V_PROFILE_DATAPOINT_PERCENT(...)
#endif

namespace VStd {
    struct thread_id; // forward declare. This is the same type as VStd::thread::id
}


namespace V::Debug {
    class Profiler {
    public:
        VOBJECT(Profiler, "{d582324b-4d54-4316-af39-a5e00ce5e9c6}");

        Profiler() = default;
        virtual ~Profiler() = default;

        // support for the extra macro args (e.g. format strings) will come in a later PR
        virtual void BeginRegion(const Budget* budget, const char* eventName) = 0;
        virtual void EndRegion(const Budget* budget) = 0;
    };

    class ProfileScope {
    public:
        template<typename... T>
        static void BeginRegion([[maybe_unused]] Budget* budget, [[maybe_unused]] const char* eventName, [[maybe_unused]] T const&... args);

        static void EndRegion([[maybe_unused]] Budget* budget);

        template<typename... T>
        ProfileScope(Budget* budget, char const* eventName, T const&... args);

        ~ProfileScope();

    private:
        Budget* m_budget;
    };
} // V::Debug

#include <vcore/debug/profiler.inl>

#endif // V_FRAMEWORK_CORE_DEBUG_PROFILER_H