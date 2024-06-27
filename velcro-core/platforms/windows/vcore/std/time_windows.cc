#include <vcore/std/time.h>
#include <vcore/platform_incl.h>

namespace VStd {
    VStd::sys_time_t GetTimeTicksPerSecond()
    {
        static VStd::sys_time_t freq = 0;
        if (freq == 0)
        {
            QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
        }
        return freq;
    }

    VStd::sys_time_t GetTimeNowTicks()
    {
        VStd::sys_time_t timeNow;
        QueryPerformanceCounter((LARGE_INTEGER*)&timeNow);
        return timeNow;
    }

    VStd::sys_time_t GetTimeNowMicroSecond()
    {
        VStd::sys_time_t timeNowMicroSecond;
        // NOTE: The default line below was not working on systems with smaller TicksPerSecond() values (like in Windows7, for example)
        // So we are now spreading the division between the numerator and denominator to maintain better precision
        timeNowMicroSecond = (GetTimeNowTicks() * 1000) / (GetTimeTicksPerSecond() / 1000);
        return timeNowMicroSecond;
    }

    VStd::sys_time_t GetTimeNowSecond()
    {
        VStd::sys_time_t timeNowSecond;
        // Using get tick count, since it's more stable for longer time measurements.
        timeNowSecond = GetTickCount() / 1000;
        return timeNowSecond;
    }

    V::u64 GetTimeUTCMilliSecond()
    {
        V::u64 utc;
        FILETIME UTCFileTime;
        GetSystemTimeAsFileTime(&UTCFileTime);
        // store time in 100 of nanoseconds since January 1, 1601 UTC
        utc = (V::u64)UTCFileTime.dwHighDateTime << 32 | UTCFileTime.dwLowDateTime;
        // convert to since 1970/01/01 00:00:00 UTC
        utc -= 116444736000000000;
        // convert to millisecond
        utc /= 10000;
        return utc;
    }
}
