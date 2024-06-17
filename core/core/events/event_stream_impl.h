#ifndef V_FRAMEWORK_CORE_EVENTS_EVENT_STREAM_IMPL_H
#define V_FRAMEWORK_CORE_EVENTS_EVENT_STREAM_IMPL_H


#include <core/events/modes.h>
#include <core/events/storeage.h>

#include <core/std/parallel/scoped_lock.h>
#include <core/std/parallel/shared_mutex.h>
#include <core/std/parallel/lock.h>
#include <core/std/typetraits/is_same.h>
#include <core/std/typetraits/conditional.h>
#include <core/std/typetraits/function_traits.h>
#include <core/std/typetraits/is_reference.h>
#include <core/std/utils.h>

namespace V {
    struct NullMutex {
        void lock() {}
        bool try_lock() { return true; }
        void unlock() {}
    };

    struct NullStreamId {
        NullStreamId() {};
        NullStreamId(int) {};
    };

    inline bool operator==(const NullStreamId&, const NullStreamId&) { return true; }
    inline bool operator!=(const NullStreamId&, const NullStreamId&) { return false; }

    // 流ID比较
    struct NullStreamIdCompare;

    namespace Internal {
        template<class Lock>
        struct NullLockGuard {
            explicit NullLockGuard(Lock&) {}
            NullLockGuard(Lock&, VStd::adopt_lock_t) {}

            void lock() {}
            bool try_lock() { return true; }
            void unlock() {}
        };
    }
}

#endif // V_FRAMEWORK_CORE_EVENTS_EVENT_STREAM_IMPL_H