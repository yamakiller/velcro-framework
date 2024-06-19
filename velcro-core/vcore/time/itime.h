#ifndef V_FRAMEWORK_CORE_TIME_ITIME_H
#define V_FRAMEWORK_CORE_TIME_ITIME_H

#include <vcore/std/time.h>
#include <vcore/std/chrono/chrono.h>
#include <vcore/std/typetraits/is_type_safe_integral.h>


namespace V {
    V_TYPE_SAFE_INTEGRAL(TimeMs, int64_t);
    V_TYPE_SAFE_INTEGRAL(TimeUs, int64_t);

    class ITime {
        public:
            ITime() = default;
            virtual ~ITime() = default;

            /// @brief 返回应用程序从开始到现在的时间间隔,单位毫秒
            /// @return 应用程序从开始到现在的时间间隔
            virtual TimeMs GetElapsedTimeMs() const = 0;
            /// @brief 返回应用程序从开始到现在的时间间隔,单位微妙
            /// @return 应用程序从开始到现在的时间间隔
            virtual TimeUs GetElapsedTimeUs() const = 0;

            V_DISABLE_COPY_MOVE(ITime);
    };

    inline TimeMs GetElapsedTimeMs() {
        return V::Interface<ITime>::Get("ITime")->GetElapsedTimeMs();
    }

     //! This is a simple convenience wrapper
    inline TimeUs GetElapsedTimeUs()
    {
        return V::Interface<ITime>::Get("ITime")->GetElapsedTimeUs();
    }

    //! Converts from milliseconds to microseconds
    inline TimeUs TimeMsToUs(TimeMs value)
    {
        return static_cast<TimeUs>(value * static_cast<TimeMs>(1000));
    }

    //! Converts from microseconds to milliseconds
    inline TimeMs TimeUsToMs(TimeUs value)
    {
        return static_cast<TimeMs>(value / static_cast<TimeUs>(1000));
    }

    //! Converts from milliseconds to seconds
    inline float TimeMsToSeconds(TimeMs value)
    {
        return static_cast<float>(value) / 1000.0f;
    }

    //! Converts from microseconds to seconds
    inline float TimeUsToSeconds(TimeUs value)
    {
        return static_cast<float>(value) / 1000000.0f;
    }

    //! Converts from milliseconds to VStd::chrono::time_point
    inline auto TimeMsToChrono(TimeMs value)
    {
        auto epoch = VStd::chrono::time_point<VStd::chrono::high_resolution_clock>();
        auto chronoValue = VStd::chrono::milliseconds(vnumeric_cast<int64_t>(value));
        return epoch + chronoValue;
    }

    //! Converts from microseconds to VStd::chrono::time_point
    inline auto TimeUsToChrono(TimeUs value)
    {
        auto epoch = VStd::chrono::time_point<VStd::chrono::high_resolution_clock>();
        auto chronoValue = VStd::chrono::microseconds(vnumeric_cast<int64_t>(value));
        return epoch + chronoValue;
    }
}

V_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(V::TimeMs);
V_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(V::TimeUs);

#endif // V_FRAMEWORK_CORE_TIME_ITIME_H