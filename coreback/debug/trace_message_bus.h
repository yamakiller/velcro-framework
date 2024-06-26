#ifndef V_FRAMEWORK_CORE_DEBUG_TRACE_MESSAGE_BUS_H
#define V_FRAMEWORK_CORE_DEBUG_TRACE_MESSAGE_BUS_H

#include <vcore/event_bus/event_bus.h>
#include <vcore/memory/osallocator.h>

namespace V
{
    namespace Debug
    {
        /**
         * Trace messages event handle.
         * Whenever code invokes V_TracePrintf / V_Warning / V_Assert / V_Error and similar macros
         * this bus gets invoked and is an ideal place to put logging sinks / displays.
         * All messages are optional (they have default implementation) and you can handle only one at a time.
         * Most messages return a boolean, if false means all the default handling will be
         * executed (callstack, details, etc.) if returned true only a minimal amount of data
         * will be output (that's critical). For example: Asserts will always print a header,
         * so no matter what you return we will have a minimal indication an assert is triggered.
         */
        class TraceMessageEvents : public V::EventBusTraits
        {
        public:
            typedef VStd::recursive_mutex MutexType;
            typedef OSStdAllocator AllocatorType;

            virtual ~TraceMessageEvents() {}
            
            /** 
            * Exception handling is only invoked in the case of an actual OS-level exception.
            * If any handlers return true, the program will be allowed to continue running despite the exception and no callstack will be emitted.
            * If all handlers return false, the OS will be given the exception, (as well as any other exception handlers).
            */
            virtual bool OnException(const char* /*message*/) { return false; }
            virtual bool OnPreError(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* /*message*/) { return false; }
            virtual bool OnError(const char* /*window*/, const char* /*message*/) { return false; }
            virtual bool OnPreWarning(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* /*message*/) { return false; }
            virtual bool OnWarning(const char* /*window*/, const char* /*message*/) { return false; }
            virtual bool OnPrintf(const char* /*window*/, const char* /*message*/) { return false; }

            /**
            * All trace functions you output to anything. So if you want to handle all the output this is the place.
            * Do not return true and disable the system output if you listen at that level. Otherwise we can trigger
            * an assert without even one line of message send to the console/debugger.
            */
            virtual bool OnOutput(const char* /*window*/, const char* /*message*/) { return false; }
        };

        typedef V::EventBus<TraceMessageEvents> TraceMessageBus;
    } // namespace Debug
} // namespace V


#endif // V_FRAMEWORK_CORE_DEBUG_TRACE_MESSAGE_BUS_H