#ifndef V_FRAMEWORK_CORE_DEBUG_TIME_H
#define V_FRAMEWORK_CORE_DEBUG_TIME_H

#include <vcore/base.h>
#include <vcore/std/time.h>

namespace V
{
    namespace Debug
    {
        /**
         * \todo use VStd::chrono
         */
        class Timer
        {
        public:
            /// Stores the current time in the timer.
            V_FORCE_INLINE void  Stamp()                                   { m_timeStamp = VStd::GetTimeNowTicks(); }
            /// Return delta time from the last MarkTime() in seconds.
            V_FORCE_INLINE float GetDeltaTimeInSeconds() const             { return (float)(VStd::GetTimeNowTicks() - m_timeStamp) /  (float)VStd::GetTimeTicksPerSecond(); }
            /// Return delta time from the last MarkTime() in ticks.
            V_FORCE_INLINE VStd::sys_time_t GetDeltaTimeInTicks() const   { return VStd::GetTimeNowTicks() - m_timeStamp; }
            /// Return delta time from the last MarkTime() in seconds and sets sets the current time.
            V_FORCE_INLINE float StampAndGetDeltaTimeInSeconds()
            {
                VStd::sys_time_t curTime = VStd::GetTimeNowTicks();
                float timeInSec = (float)(curTime - m_timeStamp) /  (float)VStd::GetTimeTicksPerSecond();
                m_timeStamp = curTime;
                return timeInSec;
            }
            /// Return delta time from the last MarkTime() in ticks and sets sets the current time.
            V_FORCE_INLINE VStd::sys_time_t StampAndGetDeltaTimeInTicks()
            {
                VStd::sys_time_t curTime = VStd::GetTimeNowTicks();
                VStd::sys_time_t timeInTicks = curTime - m_timeStamp;
                m_timeStamp = curTime;
                return timeInTicks;
            }
        private:
            VStd::sys_time_t m_timeStamp;
        };
    } // namespace Debug
} // namespace V

#endif // V_FRAMEWORK_CORE_DEBUG_TIME_H