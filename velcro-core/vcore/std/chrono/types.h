/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_CHRONO_TYPES_H
#define V_FRAMEWORK_CORE_STD_CHRONO_TYPES_H

#include <vcore/std/base.h>
#include <vcore/std/ratio.h>
#include <vcore/std/typetraits/common_type.h>
#include <vcore/std/typetraits/is_convertible.h>
#include <vcore/std/typetraits/is_floating_point.h>

#include <limits>

// Fix for windows without NO_MIN_MAX define, or any other abuse of such basic keywords.
#if defined(V_COMPILER_MSVC)
#   ifdef min
#       pragma push_macro("min")
#       undef min
#       define V_WINDOWS_MIN_PUSHED
#   endif
#   ifdef max
#       pragma push_macro("max")
#       undef max
#       define V_WINDOWS_MAX_PUSHED
#   endif
#endif


namespace VStd {

    namespace chrono {
        class system_clock;
        /*
        * Based.
        * A duration type measures time between two points in time (time_points). A duration has a representation
        * which holds a count of ticks and a tick period. The tick period is the amount of time which occurs from one
        * tick to the next, in units of seconds. It is expressed as a rational constant using the template ratio.
        *
        * Examples: duration<VStd::time_t, ratio<60>> d0; // holds a count of minutes using a time_t
        * duration<VStd::time_t, milli> d1; // holds a count of milliseconds using a time_t
        * duration<float, ratio<1, 30>> d2; // holds a count with a tick period of 1/30 of a second (30 Hz) using a float
        */
        template<class Rep, class Period = ratio<1> >
        class duration;

        template <class Clock = system_clock, class Duration = typename Clock::duration>
        class time_point;

        namespace Internal {
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            //
            template<class Rep>
            struct duration_value {
                static constexpr Rep zero() {
                    return {};
                }
                static constexpr Rep min() {
                    return std::numeric_limits<Rep>::lowest();
                }
                static constexpr Rep max() {
                    return std::numeric_limits<Rep>::max();
                }
            };
            //////////////////////////////////////////////////////////////////////////

            template <class T>
            struct is_duration
                : VStd::false_type {};

            template <class Rep, class Period>
            struct is_duration<duration<Rep, Period> >
                : VStd::true_type {};

            // duration_cast
            // duration_cast is the heart of this whole prototype.  It can convert any
            //   duration to any other.  It is also (implicitly) used in converting
            //   time_points.  The conversion is always exact if possible.  And it is
            //   always as efficient as hand written code.  If different representations
            //   are involved, care is taken to never require implicit conversions.
            //   Instead static_cast is used explicitly for every required conversion.
            //   If there are a mixture of integral and floating point representations,
            //   the use of common_type ensures that the most logical "intermediate"
            //   representation is used.
            template <class FromDuration, class ToDuration,
            class Period = typename ratio_divide<typename FromDuration::period, typename ToDuration::period>::type, bool = (Period::num == 1), bool = (Period::den == 1)>
            struct duration_cast;

            // When the two periods are the same, all that is left to do is static_cast from
            //   the source representation to the target representation (which may be a no-op).
            //   This conversion is always exact as long as the static_cast from the source
            //   representation to the destination representation is exact.
            template <class FromDuration, class ToDuration, class Period>
            struct duration_cast<FromDuration, ToDuration, Period, true, true> {
                static constexpr ToDuration cast(const FromDuration& fd) {
                    return ToDuration(static_cast<typename ToDuration::rep>(fd.count()));
                }
            };

            // When the numerator of FromPeriod / ToPeriod is 1, then all we need to do is
            //   divide by the denominator of FromPeriod / ToPeriod.  The common_type of
            //   the two representations is used for the intermediate computation before
            //   static_cast'ing to the destination.
            //   This conversion is generally not exact because of the division (but could be
            //   if you get lucky on the run time value of fd.count()).
            template <class FromDuration, class ToDuration, class Period>
            struct duration_cast<FromDuration, ToDuration, Period, true, false> {
                static constexpr ToDuration cast(const FromDuration& fd) {
                    using C = common_type_t<typename ToDuration::rep, typename FromDuration::rep, VStd::sys_time_t>;
                    return ToDuration(static_cast<typename ToDuration::rep>(static_cast<C>(fd.count()) / static_cast<C>(Period::den)));
                }
            };

            // When the denomenator of FromPeriod / ToPeriod is 1, then all we need to do is
            //   multiply by the numerator of FromPeriod / ToPeriod.  The common_type of
            //   the two representations is used for the intermediate computation before
            //   static_cast'ing to the destination.
            //   This conversion is always exact as long as the static_cast's involved are exact.
            template <class FromDuration, class ToDuration, class Period>
            struct duration_cast<FromDuration, ToDuration, Period, false, true> {
                static constexpr ToDuration cast(const FromDuration& fd) {
                    typedef typename common_type<typename ToDuration::rep, typename FromDuration::rep, VStd::sys_time_t>::type C;
                    return ToDuration(static_cast<typename ToDuration::rep>(static_cast<C>(fd.count()) * static_cast<C>(Period::num)));
                }
            };

            // When neither the numerator or denominator of FromPeriod / ToPeriod is 1, then we need to
            //   multiply by the numerator and divide by the denominator of FromPeriod / ToPeriod.  The
            //   common_type of the two representations is used for the intermediate computation before
            //   static_cast'ing to the destination.
            //   This conversion is generally not exact because of the division (but could be
            //   if you get lucky on the run time value of fd.count()).
            template <class FromDuration, class ToDuration, class Period>
            struct duration_cast<FromDuration, ToDuration, Period, false, false> {
                static constexpr ToDuration cast(const FromDuration& fd) {
                    typedef typename common_type<typename ToDuration::rep, typename FromDuration::rep, VStd::sys_time_t>::type C;
                    return ToDuration(static_cast<typename ToDuration::rep>(static_cast<C>(fd.count()) * static_cast<C>(Period::num) / static_cast<C>(Period::den)));
                }
            };
        }

        //----------------------------------------------------------------------------//
        //      is_floating_point [time.traits.is_fp]                                 //
        //      Probably should have been treat_as_floating_point. Editor notifed.    //
        //----------------------------------------------------------------------------//
        // Support bidirectional (non-exact) conversions for floating point rep types
        //   (or user defined rep types which specialize treat_as_floating_point).
        template <class Rep>
        struct treat_as_floating_point
            : VStd::is_floating_point<Rep> {};

        //----------------------------------------------------------------------------//
        //      duration_cast [time.duration.cast]                                   //
        //----------------------------------------------------------------------------//
        // Compile-time select the most efficient algorithm for the conversion...
        template <class ToDuration, class Rep, class Period>
        inline constexpr ToDuration duration_cast(const duration<Rep, Period>& fd) {
            return Internal::duration_cast<duration<Rep, Period>, ToDuration>::cast(fd);
        }

        //////////////////////////////////////////////////////////////////////////
        // Duration
        template <class Rep, class Period >
        class duration {
        public:
            typedef Rep     rep;
            typedef Period  period;

        public:
            // construct/copy/destroy:
            constexpr duration() = default;
            template <class Rep2>
            explicit constexpr duration(const Rep2& r)
                : m_rep(r) {}
            template <class Rep2, class Period2>
            constexpr duration(const duration<Rep2, Period2>& d)
                : m_rep(duration_cast<duration>(d).count())
            {}

            // observer:
            constexpr rep count() const { return m_rep; }
            // arithmetic:
            constexpr duration operator+() const { return *this; }
            constexpr duration operator-() const { return duration(-m_rep); }
            constexpr duration& operator++() { ++m_rep; return *this; }
            constexpr duration operator++(int) { return duration(m_rep++); }
            constexpr duration& operator--() { --m_rep; return *this; }
            constexpr duration operator--(int) { return duration(m_rep--); }
            constexpr duration& operator+=(const duration& d) { m_rep += d.count(); return *this; }
            constexpr duration& operator-=(const duration& d) { m_rep -= d.count(); return *this; }
            constexpr duration& operator*=(const rep& rhs) { m_rep *= rhs; return *this; }
            constexpr duration  operator/(const rep& rhs) { return duration(m_rep / rhs); }
            constexpr duration& operator/=(const rep& rhs) { m_rep /= rhs; return *this; }
            constexpr duration& operator%=(const rep& rhs) { m_rep %= rhs; return *this; }
            constexpr duration& operator%=(const duration& rhs) { m_rep %= rhs.count(); return *this; }
            // special values:
            static constexpr duration zero() { return duration(Internal::duration_value<rep>::zero()); }
            static constexpr duration min() { return duration(Internal::duration_value<rep>::min()); }
            static constexpr duration max() { return duration(Internal::duration_value<rep>::max()); }
        private:
            rep         m_rep; // exposition only
        };
        //////////////////////////////////////////////////////////////////////////

        // convenience typedefs
        typedef duration<VStd::sys_time_t, nano> nanoseconds;    // at least 64 bits needed
        typedef duration<VStd::sys_time_t, micro> microseconds;  // at least 55 bits needed
        typedef duration<VStd::sys_time_t, milli> milliseconds;  // at least 45 bits needed
        typedef duration<VStd::sys_time_t> seconds;              // at least 35 bits needed
        typedef duration<VStd::sys_time_t, ratio< 60> > minutes; // at least 29 bits needed
        typedef duration<VStd::sys_time_t, ratio<3600> > hours;  // at least 23 bits needed

        //----------------------------------------------------------------------------//
        //         duration non-member arithmetic [time.duration.nonmember]           //
        //----------------------------------------------------------------------------//
        // Duration +
        template <class Rep1, class Period1, class Rep2, class Period2>
        inline constexpr common_type_t< duration<Rep1, Period1>, duration<Rep2, Period2> >
            operator+(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
            using CD = common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2> >;
            return CD(CD(lhs).count() + CD(rhs).count());
        }

        // Duration -
        template <class Rep1, class Period1, class Rep2, class Period2>
        inline constexpr common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2> >
            operator-(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
            using CD = common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2> >;
            return CD(CD(lhs).count() - CD(rhs).count());
        }

        // Duration *
        template <class Rep1, class Period, class Rep2>
        inline constexpr
            duration<common_type_t<Rep1, Rep2>, Period>
            operator*(const duration<Rep1, Period>& d, const Rep2& s) {
            using CR = common_type_t<Rep1, Rep2>;
            using CD = duration<CR, Period>;
            return CD(CD(d).count() * static_cast<CR>(s));
        }

        template <class Rep1, class Period, class Rep2>
        inline constexpr
            duration<common_type_t<Rep1, Rep2>, Period>
            operator*(const Rep1& s, const duration<Rep2, Period>& d) {
            return d * s;
        }

        // Duration /
        namespace Internal {
            template <class Duration, class Rep, bool = is_duration<Rep>::value>
            struct duration_divide_result {};

            template <class Duration, class Rep2,
            bool = (VStd::is_convertible<typename Duration::rep, common_type_t<typename Duration::rep, Rep2>>::value
                && VStd::is_convertible<Rep2, common_type_t<typename Duration::rep, Rep2>>::value) >
            struct duration_divide_imp {};

            template <class Rep1, class Period, class Rep2>
            struct duration_divide_imp<duration<Rep1, Period>, Rep2, true> {
                typedef duration<common_type_t<Rep1, Rep2>, Period> type;
            };

            template <class Rep1, class Period, class Rep2>
            struct duration_divide_result<duration<Rep1, Period>, Rep2, false>
                : duration_divide_imp<duration<Rep1, Period>, Rep2> {};
        }

        template <class Rep1, class Period, class Rep2>
        inline constexpr typename Internal::duration_divide_result<duration<Rep1, Period>, Rep2>::type
            operator/(const duration<Rep1, Period>& d, const Rep2& s) {
            using CR = common_type_t<Rep1, Rep2>;
            using CD = typename Internal::duration_divide_result<duration<Rep1, Period>, Rep2>::type;
            return CD(CD(d).count() / static_cast<CR>(s));
        }

        template <class Rep1, class Period1, class Rep2, class Period2>
        inline constexpr common_type_t<Rep1, Rep2>
            operator/(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
            using CD = common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2> >;
            return CD(lhs).count() / CD(rhs).count();
        }

        template <class Rep1, class Period, class Rep2>
        inline constexpr typename Internal::duration_divide_result<duration<Rep1, Period>, Rep2>::type
            operator%(const duration<Rep1, Period>& d, const Rep2& s) {
            using CR = common_type_t<Rep1, Rep2>;
            using CD = typename Internal::duration_divide_result<duration<Rep1, Period>, Rep2>::type;
            return CD(CD(d).count() % static_cast<CR>(s));
        }

        template <class Rep1, class Period1, class Rep2, class Period2>
        inline constexpr auto
            operator%(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) -> common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2>> {
            using CD = common_type_t<duration<Rep1, Period1>, duration<Rep2, Period2> >;
            return CD(CD(lhs).count() % CD(rhs).count());
        }

        //----------------------------------------------------------------------------//
        //             duration comparisons [time.duration.comparisons]               //
        //----------------------------------------------------------------------------//
        namespace Internal {
            template <class LhsDuration, class RhsDuration>
            struct duration_eq {
                constexpr bool operator()(const LhsDuration& lhs, const RhsDuration& rhs) const {
                    using CD = common_type_t<LhsDuration, RhsDuration>;
                    return CD(lhs).count() == CD(rhs).count();
                }
            };

            template <class LhsDuration>
            struct duration_eq<LhsDuration, LhsDuration> {
                constexpr bool operator()(const LhsDuration& lhs, const LhsDuration& rhs) const {
                    return lhs.count() == rhs.count();
                }
            };

            template <class LhsDuration, class RhsDuration>
            struct duration_lt {
                constexpr bool operator()(const LhsDuration& lhs, const RhsDuration& rhs) const {
                    using CD = common_type_t<LhsDuration, RhsDuration>;
                    return CD(lhs).count() < CD(rhs).count();
                }
            };

            template <class LhsDuration>
            struct duration_lt<LhsDuration, LhsDuration> {
                constexpr bool operator()(const LhsDuration& lhs, const LhsDuration& rhs) const {
                    return lhs.count() < rhs.count();
                }
            };
        }

        // Duration ==
        template <class Rep1, class Period1, class Rep2, class Period2>
        inline constexpr bool
            operator==(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
            return Internal::duration_eq<duration<Rep1, Period1>, duration<Rep2, Period2> >()(lhs, rhs);
        }

        // Duration !=
        template <class Rep1, class Period1, class Rep2, class Period2>
        inline constexpr bool
            operator!=(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) { return !(lhs == rhs); }

        // Duration <
        template <class Rep1, class Period1, class Rep2, class Period2>
        inline constexpr bool
            operator< (const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) {
            return Internal::duration_lt<duration<Rep1, Period1>, duration<Rep2, Period2> >()(lhs, rhs);
        }

        // Duration >
        template <class Rep1, class Period1, class Rep2, class Period2>
        inline constexpr bool
            operator> (const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) { return rhs < lhs; }

        // Duration <=
        template <class Rep1, class Period1, class Rep2, class Period2>
        inline constexpr bool
            operator<=(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) { return !(rhs < lhs); }

        // Duration >=
        template <class Rep1, class Period1, class Rep2, class Period2>
        inline constexpr bool
            operator>=(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs) { return !(lhs < rhs); }

        //////////////////////////////////////////////////////////////////////////
        // Time point
        /**
         *  Class template time_point [time.point]
         */
        template<class Clock, class Duration >
        class time_point {
        public:
            typedef Clock clock;
            typedef Duration duration;
            typedef typename duration::rep rep;
            typedef typename duration::period period;

        public:
            // 20.9.4.1, construct
            constexpr time_point()
                : m_d(duration::zero()) {}          // has value epoch
            constexpr explicit time_point(const duration& d)
                : m_d(d) {}
            template <class Duration2>
            constexpr time_point(const time_point<clock, Duration2>& t)
                : m_d(t.time_since_epoch()) {}
            // 20.9.4.2, observer:
            constexpr duration time_since_epoch() const { return m_d; }
            // 20.9.4.3, arithmetic:
            constexpr time_point& operator+=(const duration& d) { m_d += d; return *this; }
            constexpr time_point& operator-=(const duration& d) { m_d -= d; return *this; }
            // 20.9.4.4, special values:
            static constexpr time_point min() { return time_point(duration::min()); }
            static constexpr time_point max() { return time_point(duration::max()); }
        private:
            duration m_d;
        };
        //////////////////////////////////////////////////////////////////////////
    } // namespace chrono
} // VStd

namespace std {
    //----------------------------------------------------------------------------//
    //          Specializations of common_type [time.traits.specializations]      //
    //----------------------------------------------------------------------------//
    // Must be done in the std:: namespace since the "std::common_type" is aliased into the the VStd:: namespace
    template <class Rep1, class Period1, class Rep2, class Period2>
    struct common_type<VStd::chrono::duration<Rep1, Period1>, VStd::chrono::duration<Rep2, Period2> >
    {
        using type = AZStd::chrono::duration<common_type_t<Rep1, Rep2>, typename VStd::Internal::ratio_gcd<Period1, Period2>::type>;
    };

    template <class Clock, class Duration1, class Duration2>
    struct common_type<VStd::chrono::time_point<Clock, Duration1>, VStd::chrono::time_point<Clock, Duration2> >
    {
        using type = VStd::chrono::time_point<Clock, common_type_t<Duration1, Duration2>>;
    };
}

namespace VStd {
    namespace chrono {
        //----------------------------------------------------------------------------//
        //         time_point non-member arithmetic [time.point.nonmember]            //
        //----------------------------------------------------------------------------//
        // time_point operator+(time_point x, duration y);

        template <class Clock, class Duration1, class Rep2, class Period2>
        inline constexpr time_point<Clock, common_type_t<Duration1, duration<Rep2, Period2> >>
        operator+(const time_point<Clock, Duration1>& lhs, const duration<Rep2, Period2>& rhs) {
            using TimeResult = time_point<Clock, common_type_t<Duration1, duration<Rep2, Period2> >>;
            return TimeResult(lhs.time_since_epoch() + rhs);
        }

        // time_point operator+(duration x, time_point y);

        template <class Rep1, class Period1, class Clock, class Duration2>
        inline constexpr time_point<Clock, common_type_t<duration<Rep1, Period1>, Duration2>>
        operator+(const duration<Rep1, Period1>& lhs, const time_point<Clock, Duration2>& rhs)  { return rhs + lhs; }

        // time_point operator-(time_point x, duration y);
        template <class Clock, class Duration1, class Rep2, class Period2>
        inline constexpr time_point<Clock, common_type_t<Duration1, duration<Rep2, Period2> >>
        operator-(const time_point<Clock, Duration1>& lhs, const duration<Rep2, Period2>& rhs)  { return lhs + (-rhs); }

        // duration operator-(time_point x, time_point y);
        template <class Clock, class Duration1, class Duration2>
        inline constexpr common_type_t<Duration1, Duration2>
        operator-(const time_point<Clock, Duration1>& lhs, const time_point<Clock, Duration2>& rhs) { return lhs.time_since_epoch() - rhs.time_since_epoch(); }

        //----------------------------------------------------------------------------//
        //            time_point comparisons [time.point.comparisons]                 //
        //----------------------------------------------------------------------------//
        // time_point ==
        template <class Clock, class Duration1, class Duration2>
        inline constexpr bool
        operator==(const time_point<Clock, Duration1>& lhs, const time_point<Clock, Duration2>& rhs) {
            return lhs.time_since_epoch() == rhs.time_since_epoch();
        }

        // time_point !=
        template <class Clock, class Duration1, class Duration2>
        inline constexpr bool
        operator!=(const time_point<Clock, Duration1>& lhs, const time_point<Clock, Duration2>& rhs)    { return !(lhs == rhs); }

        // time_point <
        template <class Clock, class Duration1, class Duration2>
        inline constexpr bool
        operator<(const time_point<Clock, Duration1>& lhs, const time_point<Clock, Duration2>& rhs) {
            return lhs.time_since_epoch() < rhs.time_since_epoch();
        }

        // time_point >
        template <class Clock, class Duration1, class Duration2>
        inline constexpr bool
        operator>(const time_point<Clock, Duration1>& lhs, const time_point<Clock, Duration2>& rhs)     { return rhs < lhs; }

        // time_point <=
        template <class Clock, class Duration1, class Duration2>
        inline constexpr bool
        operator<=(const time_point<Clock, Duration1>& lhs, const time_point<Clock, Duration2>& rhs)    { return !(rhs < lhs);  }

        // time_point >=
        template <class Clock, class Duration1, class Duration2>
        inline constexpr bool
        operator>=(const time_point<Clock, Duration1>& lhs, const time_point<Clock, Duration2>& rhs)    { return !(lhs < rhs);  }

        //----------------------------------------------------------------------------//
        //               time_point_cast [time.point.cast]                            //
        //----------------------------------------------------------------------------//
        template <class ToDuration, class Clock, class Duration>
        inline constexpr time_point<Clock, ToDuration> time_point_cast(const time_point<Clock, Duration>& t) {
            return time_point<Clock, ToDuration>(duration_cast<ToDuration>(t.time_since_epoch()));
        }
    } // namespace chrono
} // namespace VStd

#ifdef V_WINDOWS_MIN_PUSHED
#   pragma pop_macro("min")
#   undef V_WINDOWS_MIN_PUSHED
#endif
#ifdef V_WINDOWS_MAX_PUSHED
#   pragma pop_macro("max")
#   undef V_WINDOWS_MAX_PUSHED
#endif

#endif // V_FRAMEWORK_CORE_STD_CHRONO_TYPES_H