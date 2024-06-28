#ifndef V_FRAMEWORK_CORE_STD_PARALLEL_THREAD_BUS_H
#define V_FRAMEWORK_CORE_STD_PARALLEL_THREAD_BUS_H

#include <vcore/detector/detector_bus.h>
#include <vcore/std/parallel/thread.h>
#include <vcore/std/parallel/mutex.h>

namespace VStd
{
    //! ThreadEvents are used by user-code.
    //! You can connect to the ThreadEvents bus, and then be told whenever a new thread is spawned
    //! and when it dies.
    class ThreadEvents : public V::EventBusTraits
    {
    public:
        static const V::EventBusHandlerPolicy HandlerPolicy = V::EventBusHandlerPolicy::Multiple;
        static const V::EventBusAddressPolicy AddressPolicy = V::EventBusAddressPolicy::Single;
        using MutexType = VStd::recursive_mutex;

        virtual ~ThreadEvents() {}

        /// Called when we enter a thread, optional thread_desc is provided when the use provides one.
        virtual void OnThreadEnter(const VStd::thread::id& id, const VStd::thread_desc* desc) = 0;
        /// Called when we exit a thread.
        virtual void OnThreadExit(const VStd::thread::id& id) = 0;
    };

    //! Thread events driller bus - only "detectors" (profilers) should connect to this.
    //! A global mutex that includes a lock on the memory manager and other driller busses
    //! is held during dispatch, and listeners are expected to do no allocation
    //! or thread workloads or blocking or mutex operations of their own - only dump the data to
    //! network or file ASAP.
    //! DO NOT USE this bus unless you are a profiler capture system, use the ThreadEvents / ThreadBus instead
    class ThreadDetectorEvents
        : public V::Debug::DetectorEventBusTraits
    {
        public:
        /// Called when we enter a thread, optional thread_desc is provided when the use provides one.
        virtual void OnThreadEnter(const VStd::thread::id& id, const VStd::thread_desc* desc) = 0;
        /// Called when we exit a thread.
        virtual void OnThreadExit(const VStd::thread::id& id) = 0;
    };

    typedef V::EventBus<ThreadEvents> ThreadEventBus;
    typedef V::EventBus<ThreadDetectorEvents> ThreadDetectorEventBus;
}

#endif // V_FRAMEWORK_CORE_STD_PARALLEL_THREAD_BUS_H