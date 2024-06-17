#ifndef V_FRAMEWORK_CORE_EVENTS_EVENT_STREAM_H
#define V_FRAMEWORK_CORE_EVENTS_EVENT_STREAM_H

#include <core/events/event_stream_impl.h>

#include <core/std/typetraits/is_same.h>
#include <core/std/utils.h>
#include <core/std/parallel/scoped_lock.h>
#include <core/std/parallel/shared_mutex.h>

#include <core/events/modes.h>
#include <core/events/storeage.h>


namespace V {
    class EventStreamTraits {
        protected:
            ~EventStreamTraits() = default;
        public:
            using AllocatorType = VStd::allocator;

            static constexpr EventStreamHandlerMode HandlerMode = EventStreamHandlerMode::Multiple;
            static constexpr EventStreamAddressMode AddressMode = EventStreamAddressMode::Single;

            using StreamIdType = NullStreamId;
            using StreamIdOrderCompare = NullStreamIdCompare;
            using MutexType = NullMutex;
            // 指定EventStream是否支持队列,如果不支持就不能够延迟或堆积执行;
            // 默认情况下是不支持队列, 如果要执行队列中的事件必须调用: ExecuteQueuedEvents().
            static constexpr bool EnableEventQueue = false;
            // 指定EventStream是否默认接受排队消息, 如果设置为 false,则必须在接受事件前调用
            // 则必须在接受事件之前调用 AllowFunctionQueuing(true),且EnableEventQueue必须为ture.
            static constexpr bool EventQueueingActiveByDefault = true;
    };

}


#endif // V_FRAMEWORK_CORE_EVENTS_EVENT_STREAM_H