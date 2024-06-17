#ifndef V_FRAMEWORK_CORE_EVENT_STREAM_EVENT_H
#define V_FRAMEWORK_CORE_EVENT_STREAM_EVENT_H

namespace V {

    enum class EventStreamHandlerPolicy {
        // 单播
        Single,
        // 多播
        Multiple,
        // 多播排序
        MultipleAndOrdered
    };
}

#endif // V_FRAMEWORK_CORE_EVENT_STREAM_EVENT_H