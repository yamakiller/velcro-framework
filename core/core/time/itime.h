#ifndef V_FRAMEWORK_CORE_TIME_ITIME_H
#define V_FRAMEWORK_CORE_TIME_ITIME_H

#include <core/std/time.h>
#include <core/std/chrono/chrono.h>

namespace V {
    typedef  int64_t TimeMs;
    typedef  int64_t TimeUs;

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
}

#endif // V_FRAMEWORK_CORE_TIME_ITIME_H