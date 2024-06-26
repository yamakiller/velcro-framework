#ifndef V_FRAMEWORK_CORE_DEBUG_PROFILER_BUS_H
#define V_FRAMEWORK_CORE_DEBUG_PROFILER_BUS_H

#include <vcore/event_bus/event_bus.h>

namespace V
{
    namespace Debug
    {
        /**
         * ProfilerNotifications provides a profiler event interface that can be used to update listeners on profiler status
         */
        class ProfilerNotifications
            : public V::EventBusTraits
        {
        public:
            virtual ~ProfilerNotifications() = default;

            virtual void OnProfileSystemInitialized() = 0;
        };
        using ProfilerNotificationBus = V::EventBus<ProfilerNotifications>;

        enum class ProfileFrameAdvanceType
        {
            Game,
            Render,
            Default = Game
        };

        /**
        * ProfilerRequests provides an interface for making profiling system requests
        */
        class ProfilerRequests
            : public V::EventBusTraits
        {
        public:
            // Allow multiple threads to concurrently make requests
            using MutexType = VStd::mutex;

            virtual ~ProfilerRequests() = default;

            virtual bool IsActive() = 0;
            virtual void FrameAdvance(ProfileFrameAdvanceType type) = 0;
        };
        using ProfilerRequestBus = V::EventBus<ProfilerRequests>;
    }
}


#endif // V_FRAMEWORK_CORE_DEBUG_PROFILER_BUS_H