#ifndef V_FRAMEWORK_CORE_EVENTS_POLICIES_H
#define V_FRAMEWORK_CORE_EVENTS_POLICIES_H

#include <core/std/functional.h>
#include <core/std/function/invoke.h>
#include <core/std/containers/queue.h>
#include <core/std/containers/intrusive_set.h>
#include <core/std/parallel/scoped_lock.h>

namespace V {
    // 地址策略
    enum class EventStreamAddressPolicy {
        // (默认) EventStream 只有一个地址.
        Single, 
        // EventStream 有多个地址. 地址接收事件的顺序未定义.
        // 发送至某个 ID 的事件由连接到该 ID 的处理程序接收.
        // 没有 ID 的广播事件将由所有地址的处理程序接收.
        Id, 
        // EventStream 有多个地址. 定义地址接收事件的顺序.
        // 发送至某个 ID 的事件由连接到该 ID 的处理程序接收.
        // 没有 ID 的广播事件将由所有地址的处理程序接收. 
        // 地址接收事件的顺序由  V::EventStreamFeatures::BusIdOrderCompare定义
        IdAndOrdered,
    };

    /// @brief 定义有多少个处理程序可以连接到 EventStream 上的某个地址, 以及每个地址处的处理程序接收事件的顺序.
    enum class EventStreamHandlerPolicy {
        // EventStream 支持每个地址一个处理程序
        Single,
        // (默认) 允许每个地址有任意数量的处理程序;
        // 地址处的处理程序按照处理程序连接到 EventStream 的顺序接收事件.
        Multiple,
        // 允许每个地址有任意数量的处理程序;
        // 每个地址接收事件的顺序由 V::EventStreamFeatures::BusHandlerOrderCompare 定义.
        MultipleAndOrdered,
    };

    namespace Internal {
        struct NullEventStreamMessageCall {
            template<class Function>
            NullEventStreamMessageCall(Function) {}
            template<class Function, class Allocator>
            NullEventStreamMessageCall(Function, const Allocator&) {}
        };
    } // namespace Internal

    // TODO: 继续
    template <class EventStream>
    struct EventEventStreamConnectionPolicy {
    };

    template <class Context>
    struct EventStreamGlobalStoragePolicy {
        /// @brief 返回这个 EventStream Data.
        /// @return 指向 EventStream 数据的指针.
        static Context* Get() {
            // Because the context in this policy lives in static memory space, and
            // doesn't need to be allocated, there is no reason to defer creation.
            return &GetOrCreate();
        }

        /// @brief 返回这个 EventStream Data.
        /// @return 对 EventStream 数据的引用.
        static Context& GetOrCreate() {
            static Context _context;
            return _context;
        }
    };

    template <class Context>
    struct EventStreamThreadLocalStoragePolicy {
        static Context* Get() {
            return &GetOrCreate();
        }
        
        static Context& GetOrCreate() {
            thread_local static Context _context;
            return _context;
        }
    };
}

#endif // V_FRAMEWORK_CORE_EVENTS_POLICIES_H