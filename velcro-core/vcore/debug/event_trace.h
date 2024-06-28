#ifndef V_FRAMEWORK_CORE_DEBUG_EVENT_TRACE_H
#define V_FRAMEWORK_CORE_DEBUG_EVENT_TRACE_H

#include <vcore/base.h>
#include <vcore/platform_default.h>
#include <vcore/debug/profiler.h>

namespace VStd
{
    struct thread_id;
}

namespace V
{
    namespace Debug
    {
        namespace EventTrace
        {
            class ScopedSlice
            {
            public:
                ScopedSlice(const char* name, const char* category);
                ~ScopedSlice();

            private:
                const char* m_Name;
                const char* m_Category;
                u64 m_Time;
            };
        }
    }
}

#define V_TRACE_METHOD_NAME_CATEGORY(name, category)
#define V_TRACE_METHOD_NAME(name) V_TRACE_METHOD_NAME_CATEGORY(name, "")
#define V_TRACE_METHOD() V_TRACE_METHOD_NAME(V_FUNCTION_SIGNATURE)

#endif // V_FRAMEWORK_CORE_DEBUG_EVENT_TRACE_H