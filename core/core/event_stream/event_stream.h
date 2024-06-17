#ifndef V_FRAMEWORK_CORE_EVENT_STREAM_EVENT_STREAM_H
#define V_FRAMEWORK_CORE_EVENT_STREAM_EVENT_STREAM_H

#include <core/event_stream/policies.h>

#include <core/std/typetraits/is_same.h>

#include <core/std/utils.h>
#include <core/std/parallel/scoped_lock.h>
#include <core/std/parallel/shared_mutex.h>

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
    

    /// @cond
    inline bool operator==(const NullStreamId&, const NullStreamId&) { return true; }
    inline bool operator!=(const NullStreamId&, const NullStreamId&) { return false; }
    /// @endcond

    //struct NullStreamIdCompare;

    struct StreamHandlerCompareDefault;

    struct EventStream {
        protected:
            ~EventStream() = default;
        public:
            using AllocatorType = VStd::allocator;
            // 事件流ID数据类型
            using StreamIdType = NullStreamId;
            // 事件流处理器策略
            static constexpr EventStreamHandlerPolicy HandlerPolicy = EventStreamHandlerPolicy::Multiple;
            // 流处理器排序比较方法
            using StreamHandlerOrderCompare = StreamHandlerCompareDefault;
            // 同步锁类型
            using MutexType = NullMutex;
    };
}

#endif // V_FRAMEWORK_CORE_EVENT_STREAM_EVENT_STREAM_H