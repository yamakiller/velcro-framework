#ifndef V_FRAMEWOKR_CORE_PLATFORM_COMMON_IO_STREAMER_CONTEXT_WINAPI_H
#define V_FRAMEWOKR_CORE_PLATFORM_COMMON_IO_STREAMER_CONTEXT_WINAPI_H

#include <vcore/platform_default.h>
#include <vcore/base.h>

namespace V::Platform {
    class StreamerContextThreadSync
    {
    public:
        static constexpr size_t MaxIoEvents = MAXIMUM_WAIT_OBJECTS - 1;

        StreamerContextThreadSync();
        ~StreamerContextThreadSync();

        void Suspend();
        void Resume();

        HANDLE CreateEventHandle();
        void DestroyEventHandle(HANDLE event);
        DWORD GetEventHandleCount() const;
        bool AreEventHandlesAvailable() const;

    private:
        // Note: The first event handle is reserved for the synchronization of the
        // scheduler thread with the rest of the engine. The remaining event handles
        // can be freely used by Streamer's internals.
        HANDLE m_events[MAXIMUM_WAIT_OBJECTS]{};
        DWORD m_handleCount{ 1 }; // The first event is for external wake up calls.
    };

} // namespace V::Platform


#endif // V_FRAMEWOKR_CORE_PLATFORM_COMMON_IO_STREAMER_CONTEXT_WINAPI_H