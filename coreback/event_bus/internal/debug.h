#ifndef V_FRAMEWORK_CORE_EVENT_BUS_INTERNAL_DEBUG_H
#define V_FRAMEWORK_CORE_EVENT_BUS_INTERNAL_DEBUG_H

// Set to 1 to enable internal EBus asserts if debugging an EBus bug
#define EVENTBUS_DEBUG 0

#if EVENTBUS_DEBUG
#include <vcore/debug/trace.h>
#define EVENTBUS_ASSERT(expr, msg) V_Assert(expr, msg)
#else
#define EVENTBUS_ASSERT(...)
#endif


#endif // V_FRAMEWORK_CORE_EVENT_BUS_INTERNAL_DEBUG_H