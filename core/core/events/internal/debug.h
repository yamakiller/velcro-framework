#ifndef V_FRAMEWORK_CORE_EVENTS_INTERNAL_DEBUG_H
#define V_FRAMEWORK_CORE_EVENTS_INTERNAL_DEBUG_H

// Set to 1 to enable internal EventStream asserts if debugging an EventStream bug
#define EVENT_STREAM_DEBUG 0

#if EVENT_STREAM_DEBUG
#include <core/debug/trace.h>
#define EVENT_STREAM_ASSERT(expr, msg) V_Assert(expr, msg)
#else
#define EVENT_STREAM_ASSERT(...)
#endif

#endif //V_FRAMEWORK_CORE_EVENTS_INTERNAL_DEBUG_H