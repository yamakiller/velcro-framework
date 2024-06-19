#ifndef V_FRAMEWORK_CORE_DEBUG_TRACE_MESSAGE_DETECTOR_BUS_H
#define V_FRAMEWORK_CORE_DEBUG_TRACE_MESSAGE_DETECTOR_BUS_H

#include <vcore/detector/detector_bus.h>

namespace V
{
    namespace Debug
    {
        class TraceMessageDetectorEvents : public DetectorEventBusTraits {
            virtual ~TraceMessageDetectorEvents() {}

            /// Triggered when a V_Assert failed. This is terminating event! (the code will break, crash).
            virtual void OnException(const char* /*message*/) {}
            virtual void OnPreError(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* /*message*/) {}
            virtual void OnError(const char* /*window*/, const char* /*message*/) {}
            virtual void OnPreWarning(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* /*message*/) {}
            virtual void OnWarning(const char* /*window*/, const char* /*message*/) {}
            virtual void OnPrintf(const char* /*window*/, const char* /*message*/) {}

            virtual void OnOutput(const char* /*window*/, const char* /*message*/) {}
        };

        typedef V::EventBus<TraceMessageDetectorEvents> TraceMessageDetectorBus;
    }
}
#endif //V_FRAMEWORK_CORE_DEBUG_TRACE_MESSAGE_DETECTOR_BUS_H

