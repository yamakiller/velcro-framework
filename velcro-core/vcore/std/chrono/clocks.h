/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_CHRONO_CLOCKS_H
#define V_FRAMEWORK_CORE_STD_CHRONO_CLOCKS_H

#include <vcore/std/chrono/types.h>
#include <vcore/std/time.h>


namespace VStd {
    namespace chrono {
        //----------------------------------------------------------------------------//
        //                                                                            //
        //              Clocks [time.clock]                                           //
        //                                                                            //
        //----------------------------------------------------------------------------//

        // If you're porting, clocks are the system-specific (non-portable) part.
        // You'll need to know how to get the current time and implement that under now().
        // You'll need to know what units (tick period) and representation makes the most
        // sense for your clock and set those accordingly.
        // If you know how to map this clock to time_t (perhaps your clock is std::time, which
        // makes that trivial), then you can fill out system_clock's to_time_t() and from_time_t().

        //----------------------------------------------------------------------------//
        //              Class system_clock [time.clock.system]                        //
        //----------------------------------------------------------------------------//
        class system_clock {
        public:
            typedef microseconds                 duration;
            typedef duration::rep                rep;
            typedef duration::period             period;
            typedef chrono::time_point<system_clock>     time_point;
            static const bool is_monotonic =     true;

            // Get the current time
            static time_point  now()        { return time_point(duration(VStd::GetTimeNowMicroSecond())); }

            //          static std::time_t to_time_t(const time_point& t);
            //          static time_point  from_time_t(std::time_t t);
        };

        //----------------------------------------------------------------------------//
        //               Class monotonic_clock [time.clock.monotonic]                 //
        //----------------------------------------------------------------------------//
        typedef system_clock    monotonic_clock;        // as permitted by [time.clock.monotonic]
        typedef monotonic_clock high_resolution_clock;  // as permitted by [time.clock.hires]
    }
}

#endif // V_FRAMEWORK_CORE_STD_CHRONO_CLOCKS_H