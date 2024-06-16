#ifndef V_FRAMEWORK_CORE_EVENTS_EVENT_BUS_H
#define V_FRAMEWORK_CORE_EVENTS_EVENT_BUS_H

#include <core/std/typetraits/is_same.h>
#include <core/std/utils.h>
#include <core/std/parallel/scoped_lock.h>
#include <core/std/parallel/shared_mutex.h>

#include <core/events/policies.h>


namespace V {
    class EventStreamTraits {
        protected:
            ~EventStreamTraits() = default;
        public:
            using AllocatorType = VStd::allocator;

            static constexpr EventStreamHandlerPolicy HandlerPolicy = EventStreamHandlerPolicy::Multiple;
            static constexpr EventStreamAddressPolicy AddressPolicy = EventStreamAddressPolicy::Single;
    };

}


#endif // V_FRAMEWORK_CORE_EVENTS_EVENT_BUS_H