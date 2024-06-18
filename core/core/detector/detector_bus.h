#ifndef V_FRAMEWORK_CORE_DETECTOR_DETECTOR_BUS_H
#define V_FRAMEWORK_CORE_DETECTOR_DETECTOR_BUS_H

#include <core/base.h>
#include <core/event_bus/event_bus.h>
#include <core/memory/osallocator.h>

namespace VStd
{
    class mutex;
}

namespace V
{
    namespace Debug
    {
        class DetectorEventBusMutex
        {
        public:
            typedef VStd::recursive_mutex MutexType;

            static MutexType& GetMutex();
            void lock();
            bool try_lock();
            void unlock();
        };

        /**
         * Specialization of the EBusTraits for a driller bus. We make sure
         * all allocation are made using DebugAllocation (so no engine systems are involved).
         * In addition we make sure all driller buses use the same Mutex to synchronize data across
         * threads (so all events came in order all the time), they are still executed in the context of
         * the thread.
         */
        struct DetectorEventBusTraits : public V::EventBusTraits
        {
            typedef DetectorEventBusMutex    MutexType;
            typedef OSStdAllocator           AllocatorType;
        };
    }
}


#endif // V_FRAMEWORK_CORE_DETECTOR_DETECTOR_BUS_H