
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/array.h>
#include <AzCore/std/math.h>
#include <AzCore/std/metaprogramming/ratio.h>
#include <AzCore/std/typetraits/is_integral.h>
#include <chrono>

namespace AZStd::chrono
{
    // [time.duration](http://eel.is/c++draft/time#duration), class template duration
    using std::chrono::duration;
    // [time.point](http://eel.is/c++draft/time#point), class template time_­point
    using std::chrono::time_point;

    // [time.traits](http://eel.is/c++draft/time#traits), customization traits
    using std::chrono::duration_values;
    using std::chrono::treat_as_floating_point;
    using std::chrono::treat_as_floating_point_v;
#if __cpp_lib_chrono >= 201907L
    using std::chrono::is_clock;
    using std::chrono::is_clock_v;
#endif

    // [time.duration.cast](http://eel.is/c++draft/time#duration.cast), conversions
    // [time.point.cast](http://eel.is/c++draft/time#point.cast), conversions
    using std::chrono::ceil;
    using std::chrono::duration_cast;
    using std::chrono::time_point_cast;
    using std::chrono::floor;
    using std::chrono::round;

    // convenience type aliases
    using std::chrono::hours;
    using std::chrono::microseconds;
    using std::chrono::milliseconds;
    using std::chrono::minutes;
    using std::chrono::nanoseconds;
    using std::chrono::seconds;

#if __cpp_lib_chrono >= 201907L
    using std::chrono::days;
    using std::chrono::months;
    using std::chrono::weeks;
    using std::chrono::years;
#else
    // This type aliases are used for implementing the GetUTCTimestamp functions in AzCore
    // so they are needed in c++17
    // Using the hours::rep type alias for the underlying type for all calendar representation types
    using days = duration<typename hours::rep, ratio_multiply<ratio<24>, typename hours::period>>;
    using weeks = duration<typename hours::rep, ratio_multiply<ratio<7>, typename days::period>>;
    // Why 146097?
    // Well the date time utilities of C++20 uses the Gregorian calendar https://en.wikipedia.org/wiki/Gregorian_calendar
    // The Gregorian calendar length of year is 365.2425(not 365.25 as commonly believed)
    // Using that calendar there is a leap year every year that is a multiple of 4 or 400 except ...
    // for years that are also multiples of 100
    // i.e the year 1600 and 2000 were leap years
    // But the year 1700, 1800, 1900 *were NOT* leap years
    // Also the year 2100 will not be a leap year
    // What this means is that the Gregorian calender "repeats" every 400 years
    // i.e Any weekday, month combination in 2022 was the same as 1622
    // The number 400 is the lowest value that an be multipled by 365.2425 to provide natural number of days
    // 365.2425 * 400 = 146097
    // Now the Julian calendar has a 1 leap year of 366 days for every 3 normal year of 365 days
    // Now if the Julian calendar was use to calculate the number of days that passed over 400 it would be
    // 365.25 * 400 = 146100
    // 146100 is 3 days more than the Gregorian calendar after 400 years
    // Those 3 days are accounted for by years that are multiples of 100, but not 400 being normal years(1700, 1800, 1900)
    using years = duration<typename hours::rep, ratio_multiply<ratio<146097, 400>, typename days::period>>;

    // Months are defined in terms of a year. So a month is 1/12 of a year, which if 365.2425 were divided by 12
    // the average month length is 30.436875 days
    using months = duration<typename hours::rep, ratio_divide<typename years::period, ratio<12>>>;
#endif

    // [time.duration.alg](http://eel.is/c++draft/time#duration.alg), specialized algorithms
    using std::chrono::abs;

    // [time.clock.system](http://eel.is/c++draft/time#clock.system), class system_­clock
    using std::chrono::system_clock;
#if __cpp_lib_chrono >= 201907L
    using std::chrono::sys_days;
    using std::chrono::sys_seconds;
    using std::chrono::sys_time;
#else
    template<class Duration>
    using sys_time = time_point<system_clock, Duration>;
    using sys_seconds = sys_time<std::chrono::seconds>;
    using sys_days = sys_time<days>;
#endif

    // [time.clock.utc](http://eel.is/c++draft/time#clock.utc), class utc_­clock
#if __cpp_lib_chrono >= 201907L
    using std::chrono::utc_clock;
    using std::chrono::utc_seconds;
    using std::chrono::utc_time;

    using std::chrono::get_leap_second_info;
    using std::chrono::leap_second;
    using std::chrono::leap_second_info;
#else

    class leap_second
    {
    public:
        leap_second(const leap_second&) = default;
        leap_second& operator=(const leap_second&) = default;

        constexpr sys_seconds date() const noexcept
        {
            return m_leap_second_date;
        }
        constexpr std::chrono::seconds value() const noexcept
        {
            return m_value;
        }

        constexpr leap_second(sys_seconds leap_second_date, std::chrono::seconds value)
            : m_leap_second_date(leap_second_date)
            , m_value(value)
        {
        }

    private:
        sys_seconds m_leap_second_date;
        std::chrono::seconds m_value;
    };

    constexpr bool operator==(const leap_second& x, const leap_second& y) noexcept
    {
        return x.date() == y.date();
    }
    constexpr bool operator!=(const leap_second& x, const leap_second& y) noexcept
    {
        return !operator==(x, y);
    }
    constexpr bool operator<(const leap_second& x, const leap_second& y) noexcept
    {
        return x.date() < y.date();
    }
    constexpr bool operator>(const leap_second& x, const leap_second& y) noexcept
    {
        return operator<(y, x);
    }
    constexpr bool operator<=(const leap_second& x, const leap_second& y) noexcept
    {
        return !operator<(y, x);
    }
    constexpr bool operator>=(const leap_second& x, const leap_second& y) noexcept
    {
        return !operator<(x, y);
    }

    // comparison with sys_time alias
    template<class Duration>
    constexpr bool operator==(const leap_second& x, const sys_time<Duration>& y) noexcept
    {
        return x.date() == y;
    }
    template<class Duration>
    constexpr bool operator==(const sys_time<Duration>& x, const leap_second& y) noexcept
    {
        return x == y.date();
    }
    template<class Duration>
    constexpr bool operator!=(const leap_second& x, const sys_time<Duration>& y) noexcept
    {
        return !operator==(x, y);
    }
    template<class Duration>
    constexpr bool operator!=(const sys_time<Duration>& x, const leap_second& y) noexcept
    {
        return !operator==(x, y);
    }
    template<class Duration>
    constexpr bool operator<(const leap_second& x, const sys_time<Duration>& y) noexcept
    {
        return x.date() < y;
    }
    template<class Duration>
    constexpr bool operator<(const sys_time<Duration>& x, const leap_second& y) noexcept
    {
        return x < y.date();
    }
    template<class Duration>
    constexpr bool operator>(const leap_second& x, const sys_time<Duration>& y) noexcept
    {
        return operator<(y, x);
    }
    template<class Duration>
    constexpr bool operator>(const sys_time<Duration>& x, const leap_second& y) noexcept
    {
        return operator<(y, x);
    }
    template<class Duration>
    constexpr bool operator<=(const leap_second& x, const sys_time<Duration>& y) noexcept
    {
        return !operator<(y, x);
    }
    template<class Duration>
    constexpr bool operator<=(const sys_time<Duration>& x, const leap_second& y) noexcept
    {
        return !operator<(y, x);
    }
    template<class Duration>
    constexpr bool operator>=(const leap_second& x, const sys_time<Duration>& y) noexcept
    {
        return operator<(x, y);
    }
    template<class Duration>
    constexpr bool operator>=(const sys_time<Duration>& x, const leap_second& y) noexcept
    {
        return !operator<(x, y);
    }

    namespace Internal
    {
        // Leap Second table since the system clock epoch of 1970-01-01 00:00:00 UTC
        // http://eel.is/c++draft/time#zone.leap
        // https://www.ietf.org/timezones/data/leap-seconds.list
        static constexpr auto LeapSecondTable = to_array<leap_second>({
            { sys_seconds{ std::chrono::seconds{ 78796800 } }, std::chrono::seconds{ 1 } },   { sys_seconds{ std::chrono::seconds{ 94694400 } }, std::chrono::seconds{ 1 } },
            { sys_seconds{ std::chrono::seconds{ 126230400 } }, std::chrono::seconds{ 1 } },  { sys_seconds{ std::chrono::seconds{ 157766400 } }, std::chrono::seconds{ 1 } },
            { sys_seconds{ std::chrono::seconds{ 189302400 } }, std::chrono::seconds{ 1 } },  { sys_seconds{ std::chrono::seconds{ 220924800 } }, std::chrono::seconds{ 1 } },
            { sys_seconds{ std::chrono::seconds{ 252460800 } }, std::chrono::seconds{ 1 } },  { sys_seconds{ std::chrono::seconds{ 283996800 } }, std::chrono::seconds{ 1 } },
            { sys_seconds{ std::chrono::seconds{ 315532800 } }, std::chrono::seconds{ 1 } },  { sys_seconds{ std::chrono::seconds{ 362793600 } }, std::chrono::seconds{ 1 } },
            { sys_seconds{ std::chrono::seconds{ 394329600 } }, std::chrono::seconds{ 1 } },  { sys_seconds{ std::chrono::seconds{ 425865600 } }, std::chrono::seconds{ 1 } },
            { sys_seconds{ std::chrono::seconds{ 489024000 } }, std::chrono::seconds{ 1 } },  { sys_seconds{ std::chrono::seconds{ 567993600 } }, std::chrono::seconds{ 1 } },
            { sys_seconds{ std::chrono::seconds{ 631152000 } }, std::chrono::seconds{ 1 } },  { sys_seconds{ std::chrono::seconds{ 662688000 } }, std::chrono::seconds{ 1 } },
            { sys_seconds{ std::chrono::seconds{ 709948800 } }, std::chrono::seconds{ 1 } },  { sys_seconds{ std::chrono::seconds{ 741484800 } }, std::chrono::seconds{ 1 } },
            { sys_seconds{ std::chrono::seconds{ 773020800 } }, std::chrono::seconds{ 1 } },  { sys_seconds{ std::chrono::seconds{ 820454400 } }, std::chrono::seconds{ 1 } },
            { sys_seconds{ std::chrono::seconds{ 867715200 } }, std::chrono::seconds{ 1 } },  { sys_seconds{ std::chrono::seconds{ 915148800 } }, std::chrono::seconds{ 1 } },
            { sys_seconds{ std::chrono::seconds{ 1136073600 } }, std::chrono::seconds{ 1 } }, { sys_seconds{ std::chrono::seconds{ 1230768000 } }, std::chrono::seconds{ 1 } },
            { sys_seconds{ std::chrono::seconds{ 1341100800 } }, std::chrono::seconds{ 1 } }, { sys_seconds{ std::chrono::seconds{ 1435708800 } }, std::chrono::seconds{ 1 } },
            { sys_seconds{ std::chrono::seconds{ 1483228800 } }, std::chrono::seconds{ 1 } },
        });
    } // namespace Internal

    struct leap_second_info
    {
        bool is_leap_second{};
        std::chrono::seconds elapsed{};
    };

    //! utc_clock and its associated time_point utc_time utc_time,
    //! count time, including leap seconds, since 1970-01-01 00:00:00 UTC
    //! Note: The UTC time actually standard began on 1972-01-01 00:00:10 TAI
    class utc_clock;

    template<class Duration>
    using utc_time = time_point<utc_clock, Duration>;
    using utc_seconds = utc_time<std::chrono::seconds>;

    class utc_clock
    {
    public:
        // Re-use the system_clock representation and tick period type
        using rep = system_clock::rep;
        using period = system_clock::period;
        using duration = chrono::duration<rep, period>;
        using time_point = chrono::time_point<utc_clock>;
        static constexpr bool is_steady = system_clock::is_steady;

        static time_point now()
        {
            return from_sys(system_clock::now());
        }

        template<class Duration>
        static sys_time<common_type_t<Duration, std::chrono::seconds>> to_sys(const utc_time<Duration>& t)
        {
            using common_type = common_type_t<Duration, std::chrono::seconds>;
            leap_second_info lsi = get_leap_second_info(t);
            common_type ticks;
            if (lsi.is_leap_second)
            {
                // the leap second that is being inserted is not included in the sys_time
                // and last representable value before the insertion of the leap second is returned
                auto elapsedSeconds = floor<std::chrono::seconds>(t.time_since_epoch()) - lsi.elapsed;
                if constexpr (is_integral_v<typename duration::rep>)
                {
                    // increment the last representable value for the common_type that is less than a second
                    ticks = elapsedSeconds + std::chrono::seconds(1) - common_type(1);
                }
                else
                {
                    // Find the next floating point value in the direction of 0.0
                    // This returns a value the last representable value for the common type that is
                    // round_to_next_lowest_representable_float_value(elapsedSeconds + 1)
                    ticks = common_type{ nextafter(ceil<common_type>(elapsedSeconds + std::chrono::seconds(1)).count(), typename common_type::rep{ 0 }) };
                }
            }
            else
            {
                ticks = t.time_since_epoch() - lsi.elapsed;
            }
            return sys_time<common_type>{ ticks };
        }
        template<class Duration>
        static utc_time<common_type_t<Duration, std::chrono::seconds>> from_sys(const sys_time<Duration>& t)
        {
            // count the number of elapsed leap seconds
            std::chrono::seconds elapsed{};
            for (size_t i = 0; i < Internal::LeapSecondTable.size(); ++i, ++elapsed)
            {
                const leap_second& leapSecond = Internal::LeapSecondTable[i];
                if (leapSecond.date() > t)
                {
                    break;
                }
            }
            return utc_time<common_type_t<Duration, std::chrono::seconds>>{ t.time_since_epoch() + elapsed };
        }
    };

    template<class Duration>
    constexpr leap_second_info get_leap_second_info(const utc_time<Duration>& ut)
    {
        leap_second_info result;
        for (size_t i = 0; i < Internal::LeapSecondTable.size(); ++i)
        {
            const leap_second& leapSecond = Internal::LeapSecondTable[i];
            if (leapSecond.date().time_since_epoch() > ut.time_since_epoch())
            {
                break;
            }

            result.elapsed += leapSecond.value();
            // special case, if the current utc time happens to fall on a
            // a leap second, then add mark the current second as leap second
            if (leapSecond.date().time_since_epoch() == ut.time_since_epoch())
            {
                result.is_leap_second = true;
            }
        }

        return result;
    }
#endif

    // [time.clock.tai](http://eel.is/c++draft/time#clock.tai), class tai_­clock
#if __cpp_lib_chrono >= 201907L
    // Not needed for calculating UTC timestamp so a c++17 impl is not provided
    using std::chrono::tai_clock;
    using std::chrono::tai_seconds;
    using std::chrono::tai_time;
#endif

    // [time.clock.gps](http://eel.is/c++draft/time#clock.gps), class gps_­clock
#if __cpp_lib_chrono >= 201907L
    // Not needed for calculating UTC timestamp so a c++17 impl is not provided
    using std::chrono::gps_clock;
    using std::chrono::gps_seconds;
    using std::chrono::gps_time;
#endif

    // [time.clock.file](http://eel.is/c++draft/time#clock.file), type file_­clock
#if __cpp_lib_chrono >= 201907L
    // Not needed for calculating UTC timestamp so a c++17 impl is not provided
    using std::chrono::file_clock;
    using std::chrono::file_time;
#endif

    // [time.clock.steady](http://eel.is/c++draft/time#clock.steady), class steady_­clock
    using std::chrono::steady_clock;

    // [time.clock.hires](http://eel.is/c++draft/time#clock.hires), class high_­resolution_­clock
    using std::chrono::high_resolution_clock;

    // [time.clock.local](http://eel.is/c++draft/time#clock.local), local time
#if __cpp_lib_chrono >= 201907L
    using std::chrono::local_days;
    using std::chrono::local_seconds;
    using std::chrono::local_t;
#else
    struct local_t
    {
    };
    template<class Duration>
    using local_time = time_point<local_t, Duration>;
    using local_seconds = local_time<std::chrono::seconds>;
    using local_days = local_time<days>;
#endif
    // [time.clock.cast](http://eel.is/c++draft/time#clock.cast), time_­point conversions
#if __cpp_lib_chrono >= 201907L
    // Not needed for calculating UTC timestamp so a c++17 impl is not provided
    using std::chrono::clock_cast;
    using std::chrono::clock_time_conversion;
#endif

    // The [time.cal] types describe a Gregorian calendar
    // http://eel.is/c++draft/time#cal

    // Modulo functions which rounds towards negative infinity
    // and always return a remainder in the range of [0, pos]
    namespace Internal
    {
        // For euclidean division, given two integers a and b, there exist
        // a quotient q and a remainder are such that `a = bq + r`
        // and the remainder r is 0 <= r < |b|
        // https://en.wikipedia.org/wiki/Euclidean_division#Division_theorem
        static constexpr long long euclidean_divide(long long dividend, long long divisor)
        {
            // C++11 implements integer division which truncates to 0
            // subtract the abs(divisor - 1) from the dividend when the dividend is negative
            // This allows the remainder to be positive and fulfill the theorem
            // This does not check for underflow
            return dividend >= 0 ? dividend / divisor : (dividend - (divisor >= 0 ? divisor - 1 : -divisor - 1)) / divisor;
        }
        static constexpr long long euclidean_modulo(long long dividend, long long divisor)
        {
            const long long rem = dividend % divisor;
            // C++11 implements integer division which truncates to 0, therefore negative remainders are possible
            // add the divisor to make the remainder positive
            return rem >= 0 ? rem : rem + (divisor >= 0 ? divisor : -divisor);
        }

        // as the dividend is positive the remainder should be 2
        // a = 5, b = 3, therefore 5 = 3 * 1 + 2
        static_assert(euclidean_divide(5, 3) == 1);
        static_assert(euclidean_modulo(5, 3) == 2);
        // a = 5, b = -3, therefore 5 = 3 * (-1) + 2
        static_assert(euclidean_divide(5, -3) == -1);
        static_assert(euclidean_modulo(5, -3) == 2);

        // for a negative dividend the remainder should be 1
        // a = -5, b = -3, therefore -5 = 3 * (-2) + 1
        static_assert(euclidean_divide(-5, -3) == 2);
        static_assert(euclidean_modulo(-5, -3) == 1);
        // a = -5, b = 3, therefore -5 = (-3) * 2 + 1
        static_assert(euclidean_divide(-5, 3) == -2);
        static_assert(euclidean_modulo(-5, 3) == 1);

        // validate when the division is exact
        static_assert(euclidean_divide(5, 5) == 1);
        static_assert(euclidean_modulo(5, 5) == 0);

        static_assert(euclidean_divide(5, -5) == -1);
        static_assert(euclidean_modulo(5, -5) == 0);

        static_assert(euclidean_divide(-5, -5) == 1);
        static_assert(euclidean_modulo(-5, -5) == 0);

        static_assert(euclidean_divide(-5, 5) == -1);
        static_assert(euclidean_modulo(-5, 5) == 0);

        // when a negative divisor has a smaller absolute value than the dividend
        // the result should 1 or -1 with a positive remainder
        static_assert(euclidean_divide(-3, 5) == -1);
        static_assert(euclidean_modulo(-3, 5) == 2);

        static_assert(euclidean_divide(-3, -5) == 1);
        static_assert(euclidean_modulo(-3, -5) == 2);
    } // namespace Internal

    namespace Internal
    {
        // computes if the supplied year is a leap year
        constexpr bool is_leap(int y) noexcept
        {
            return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0);
        }
        // difference between common year and leap year is the number of days
        // in february(28 vs 29)
        constexpr unsigned last_day_of_month_common_year(unsigned m) noexcept
        {
            constexpr unsigned char MonthLastDayTable[]{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
            // Months values are between [1, 12] so subtract 1 from the value
            return MonthLastDayTable[m - 1];
        }
        constexpr unsigned last_day_of_month_leap_year(unsigned m) noexcept
        {
            constexpr unsigned char MonthLastDayTable[]{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
            // Months values are between [1, 12] so subtract 1 from the value
            return MonthLastDayTable[m - 1];
        }

        // computes the last day of the month given a year value
        // leap years are accounted for
        constexpr unsigned last_day_of_month(int y, unsigned m) noexcept
        {
            // If the month isn't Feburary of a leap year then call
            // last_day_of_month_common_year, otherwise the last day is 29th of Febrary
            return m != 2 || !is_leap(y) ? last_day_of_month_common_year(m) : 29u;
        }
    } // namespace Internal

    // [time.cal.last](http://eel.is/c++draft/time#cal.last), class last_­spec
#if __cpp_lib_chrono >= 201907L
    using std::chrono::last_spec using std::chrono::last;
#else
    struct last_spec
    {
        explicit last_spec() = default;
    };

    inline constexpr last_spec last{};
#endif

    // [time.cal.day](http://eel.is/c++draft/time#cal.day), class day
#if __cpp_lib_chrono >= 201907L
    using std::chrono::day;
#else
    class day
    {
    public:
        day() = default;
        constexpr explicit day(unsigned d) noexcept
            : m_day(static_cast<unsigned char>(d))
        {
        }
        constexpr day& operator++() noexcept
        {
            ++m_day;
            return *this;
        }
        constexpr day operator++(int) noexcept
        {
            auto tmp = *this;
            ++m_day;
            return tmp;
        }
        constexpr day& operator--() noexcept
        {
            --m_day;
            return *this;
        }
        constexpr day operator--(int) noexcept
        {
            auto tmp = *this;
            --m_day;
            return tmp;
        }

        constexpr day& operator+=(const days& d) noexcept
        {
            *this = *this + d;
            return *this;
        }
        constexpr day& operator-=(const days& d) noexcept
        {
            *this = *this - d;
            return *this;
        }

        constexpr explicit operator unsigned() const noexcept
        {
            return m_day;
        }

        //! A day is valid if it represents a day of a month(1-31)
        constexpr bool ok() const noexcept
        {
            return m_day >= 1 && m_day <= 31;
        }

        friend constexpr day operator+(const day& x, const days& y) noexcept;
        friend constexpr day operator-(const day& x, const days& y) noexcept;

    private:
        unsigned char m_day{};
    };

    constexpr bool operator==(const day& x, const day& y) noexcept
    {
        return unsigned{ x } == unsigned{ y };
    }
    constexpr bool operator!=(const day& x, const day& y) noexcept
    {
        return !operator==(x, y);
    }

    constexpr bool operator<(const day& x, const day& y) noexcept
    {
        return unsigned{ x } < unsigned{ y };
    }
    constexpr bool operator>(const day& x, const day& y) noexcept
    {
        return operator<(y, x);
    }
    constexpr bool operator<=(const day& x, const day& y) noexcept
    {
        return !operator<(y, x);
    }
    constexpr bool operator>=(const day& x, const day& y) noexcept
    {
        return !operator<(x, y);
    }

    constexpr day operator+(const day& x, const days& y) noexcept
    {
        return day{ static_cast<unsigned>(static_cast<int>(unsigned{ x }) + y.count()) };
    }
    constexpr day operator+(const days& x, const day& y) noexcept
    {
        return y + x;
    }
    constexpr day operator-(const day& x, const days& y) noexcept
    {
        return x + -y;
    }
    constexpr days operator-(const day& x, const day& y) noexcept
    {
        return days{ int(unsigned{ x }) - int(unsigned{ y }) };
    }
#endif

    // [time.cal.month](http://eel.is/c++draft/time#cal.month), class month
#if __cpp_lib_chrono >= 201907L
    using std::chrono::month;
#else
    class month
    {
    public:
        month() = default;
        constexpr explicit month(unsigned m) noexcept
            : m_month(static_cast<unsigned char>(m))
        {
        }
        constexpr month& operator++() noexcept
        {
            *this += months{ 1 };
            return *this;
        }
        constexpr month operator++(int) noexcept
        {
            auto tmp = *this;
            *this += months{ 1 };
            return tmp;
        }
        constexpr month& operator--() noexcept
        {
            *this -= months{ 1 };
            return *this;
        }
        constexpr month operator--(int) noexcept
        {
            auto tmp = *this;
            *this -= months{ 1 };
            return tmp;
        }

        constexpr month& operator+=(const months& m) noexcept
        {
            *this = *this + m;
            return *this;
        }
        constexpr month& operator-=(const months& m) noexcept
        {
            *this = *this - m;
            return *this;
        }

        constexpr explicit operator unsigned() const noexcept
        {
            return m_month;
        }

        //! A month is valid if it represents a month of the year(1-12)
        constexpr bool ok() const noexcept
        {
            return m_month >= 1 && m_month <= 12;
        }

        friend constexpr month operator+(const month& x, const months& y) noexcept;
        friend constexpr month operator-(const month& x, const months& y) noexcept;

    private:
        unsigned char m_month{};
    };

    constexpr bool operator==(const month& x, const month& y) noexcept
    {
        return unsigned{ x } == unsigned{ y };
    }
    constexpr bool operator!=(const month& x, const month& y) noexcept
    {
        return !operator==(x, y);
    }

    constexpr bool operator<(const month& x, const month& y) noexcept
    {
        return unsigned{ x } < unsigned{ y };
    }
    constexpr bool operator>(const month& x, const month& y) noexcept
    {
        return operator<(y, x);
    }
    constexpr bool operator<=(const month& x, const month& y) noexcept
    {
        return !operator<(y, x);
    }
    constexpr bool operator>=(const month& x, const month& y) noexcept
    {
        return !operator<(x, y);
    }

    constexpr month operator+(const month& x, const months& y) noexcept
    {
        return month{ unsigned(Internal::euclidean_modulo(static_cast<long long>(unsigned{ x }) + (y.count() - 1), 12)) + 1 };
    }
    constexpr month operator+(const months& x, const month& y) noexcept
    {
        return y + x;
    }
    constexpr month operator-(const month& x, const months& y) noexcept
    {
        return x + -y;
    }
    constexpr months operator-(const month& x, const month& y) noexcept
    {
        return months{ Internal::euclidean_modulo(int(unsigned{ x }) - int(unsigned{ y }), 12) };
    }
#endif

    // [time.cal.year](http://eel.is/c++draft/time#cal.year), class year
#if __cpp_lib_chrono >= 201907L
    using std::chrono::year;
#else
    class year
    {
    public:
        year() = default;
        constexpr explicit year(int y) noexcept
            : m_year(static_cast<short>(y))
        {
        }
        constexpr year& operator++() noexcept
        {
            *this += years{ 1 };
            return *this;
        }
        constexpr year operator++(int) noexcept
        {
            auto tmp = *this;
            *this += years{ 1 };
            return tmp;
        }
        constexpr year& operator--() noexcept
        {
            *this -= years{ 1 };
            return *this;
        }
        constexpr year operator--(int) noexcept
        {
            auto tmp = *this;
            *this -= years{ 1 };
            return tmp;
        }

        constexpr year& operator+=(const years& y) noexcept
        {
            *this = *this + y;
            return *this;
        }
        constexpr year& operator-=(const years& y) noexcept
        {
            *this = *this - y;
            return *this;
        }

        constexpr bool is_leap() const noexcept
        {
            return Internal::is_leap(int{ m_year });
        }

        constexpr explicit operator int() const noexcept
        {
            return m_year;
        }

        //! A year is valid if it is between the min and max values
        constexpr bool ok() const noexcept
        {
            return min().m_year <= m_year && m_year <= max().m_year;
        }

        static constexpr year min() noexcept
        {
            return year{ -32767 };
        }
        static constexpr year max() noexcept
        {
            return year{ 32767 };
        }

        friend constexpr year operator+(const year& x, const years& y) noexcept;
        friend constexpr year operator-(const year& x, const years& y) noexcept;

    private:
        short m_year{};
    };

    constexpr bool operator==(const year& x, const year& y) noexcept
    {
        return int{ x } == int{ y };
    }
    constexpr bool operator!=(const year& x, const year& y) noexcept
    {
        return !operator==(x, y);
    }

    constexpr bool operator<(const year& x, const year& y) noexcept
    {
        return int{ x } < int{ y };
    }
    constexpr bool operator>(const year& x, const year& y) noexcept
    {
        return operator<(y, x);
    }
    constexpr bool operator<=(const year& x, const year& y) noexcept
    {
        return !operator<(y, x);
    }
    constexpr bool operator>=(const year& x, const year& y) noexcept
    {
        return !operator<(x, y);
    }

    constexpr year operator+(const year& x, const years& y) noexcept
    {
        return year{ int{ x } + static_cast<int>(y.count()) };
    }
    constexpr year operator+(const years& x, const year& y) noexcept
    {
        return y + x;
    }
    constexpr year operator-(const year& x, const years& y) noexcept
    {
        return x + -y;
    }
    constexpr years operator-(const year& x, const year& y) noexcept
    {
        return years{ int{ x } - int{ y } };
    }
#endif
    // [time.cal.wd](http://eel.is/c++draft/time#cal.wd), class weekday
    namespace Internal
    {
        // The epoch of 1970-01-01 00:00:00 UTC was Thursday which is weekday 4
        // indexed from 0(0 = Sunday, 1 = Monday, 2 = Tuesday, 3 = Wednesday, 4 = Thursday,
        // 5 = Friday, 6 = Saturday)
        // A modulo operation based on a euclidean divison with sys_day(0) = Thursday
        // is detailed at https://howardhinnant.github.io/date_algorithms.html#weekday_from_days
        static constexpr unsigned epoch_weekday = 4;

        // computes the day of the week from a serial date
        static constexpr unsigned weekday_from_days(const sys_days& dp) noexcept
        {
            // uses euclidean modulo to map the serial date into weekday
            // since the epoch weekday was a Thursday(4th day of the week) it is added to the serial date
            return unsigned(
                Internal::euclidean_modulo(dp.time_since_epoch().count() + Internal::epoch_weekday, 7));
        }

        // Determines how many days are needed to get from weekday y to weekday x
        // precondition: x, y <= 6 (in the range of [0, 6])
        // returns: days in the range of [0, 6] if the preconditions are met
        static constexpr unsigned weekday_difference(unsigned x, unsigned y) noexcept
        {
            x -= y;
            return x <= 6 ? x : x + 7;
        }

        // Returns the next weekday in the circular range of [Sun, Sat] which
        // is equivalently [0, 6]
        // precondtion: wd <= 6;
        // returns the next weekday in the range of [0, 6]
        static constexpr unsigned next_weekday(unsigned wd) noexcept
        {
            return wd < 6 ? wd + 1 : 0;
        }
        // Returns the previous weekday in the circular range of [Sun, Sat] which
        // is equivalently [0, 6]
        // precondtion: wd <= 6;
        // returns the previous weekday in the range of [0, 6]
        static constexpr unsigned prev_weekday(unsigned wd) noexcept
        {
            return wd > 0 ? wd - 1 : 6;
        }

    } // namespace Internal

#if __cpp_lib_chrono >= 201907L
    using std::chrono::weekday;
    using std::chrono::weekday_indexed;
    using std::chrono::weekday_last;
#else
    class weekday_indexed; // forward declare weekday_indexed and weekday_last
    class weekday_last;

    //! represents a day of the week
    //! weekdays aren't ordered since there is not a consensus
    //! on which day a week starts
    class weekday
    {
    public:
        weekday() = default;
        constexpr explicit weekday(unsigned wd) noexcept
            : m_weekday(wd == 7 ? 0u : static_cast<unsigned char>(wd))
        {
        }
        constexpr weekday(const sys_days& dp) noexcept
            : m_weekday(static_cast<unsigned char>(Internal::weekday_from_days(dp)))
        {
        }
        constexpr explicit weekday(const local_days& dp) noexcept
            : weekday{ sys_days{ dp.time_since_epoch() } }
        {
        }

        constexpr weekday& operator++() noexcept
        {
            *this += days{ 1 };
            return *this;
        }
        constexpr weekday operator++(int) noexcept
        {
            auto tmp = *this;
            *this += days{ 1 };
            return tmp;
        }
        constexpr weekday& operator--() noexcept
        {
            *this -= days{ 1 };
            return *this;
        }
        constexpr weekday operator--(int) noexcept
        {
            auto tmp = *this;
            *this -= days{ 1 };
            return tmp;
        }

        constexpr weekday& operator+=(const days& d) noexcept
        {
            *this = *this + d;
            return *this;
        }
        constexpr weekday& operator-=(const days& d) noexcept
        {
            *this = *this - d;
            return *this;
        }

        constexpr explicit operator unsigned() const noexcept
        {
            return m_weekday;
        }

        constexpr unsigned c_encoding() const noexcept
        {
            return m_weekday;
        }
        constexpr unsigned iso_encoding() const noexcept
        {
            return m_weekday == 0u ? 7u : m_weekday;
        }

        //! A weekday is valid if it represents a day of the week(0-6)
        constexpr bool ok() const noexcept
        {
            return m_weekday <= 6;
        }

        //! Needs to be implemented after weekday_indexed is complete
        constexpr weekday_indexed operator[](unsigned index) const noexcept;

        //! Needs to be implemented after weekday_last is complete
        constexpr weekday_last operator[](last_spec) const noexcept;

        friend constexpr bool operator==(const weekday& x, const weekday& y) noexcept;
        friend constexpr weekday operator+(const weekday& x, const days& y) noexcept;
        friend constexpr weekday operator-(const weekday& x, const days& y) noexcept;
        friend constexpr days operator-(const weekday& x, const weekday& y) noexcept;

    private:
        unsigned char m_weekday{};
    };

    constexpr bool operator==(const weekday& x, const weekday& y) noexcept
    {
        return x.m_weekday == y.m_weekday;
    }
    constexpr bool operator!=(const weekday& x, const weekday& y) noexcept
    {
        return !operator==(x, y);
    }

    constexpr weekday operator+(const weekday& x, const days& y) noexcept
    {
        return weekday{ unsigned(Internal::euclidean_modulo(static_cast<long long>(x.m_weekday) + y.count(), 7)) };
    }
    constexpr weekday operator+(const days& x, const weekday& y) noexcept
    {
        return y + x;
    }
    constexpr weekday operator-(const weekday& x, const days& y) noexcept
    {
        return x + -y;
    }
    constexpr days operator-(const weekday& x, const weekday& y) noexcept
    {
        return days{ Internal::euclidean_modulo(
            static_cast<long long>(unsigned{ x.m_weekday }) - static_cast<long long>(unsigned{ y.m_weekday }), 7) };
    }
#endif

    // [time.cal.wdidx](http://eel.is/c++draft/time#cal.wdidx), class weekday_­indexed
#if __cpp_lib_chrono >= 201907L
    using std::chrono::weekday_indexed;
#else
    // Represents the first, second, third, fourth or fifth weekday of a month
    class weekday_indexed
    {
    public:
        weekday_indexed() = default;
        constexpr weekday_indexed(const chrono::weekday& wd, unsigned index) noexcept
            : m_weekday{ wd }
            , m_index{ static_cast<unsigned char>(index) }
        {
        }

        constexpr chrono::weekday weekday() const noexcept
        {
            return m_weekday;
        }
        constexpr unsigned index() const noexcept
        {
            return m_index;
        }
        constexpr bool ok() const noexcept
        {
            // since this class doesn't take a month into account
            // the only validation that can b done is that the
            // weekday is between [0, 6] and the week index is between [1, 5]
            return m_weekday.ok() && 1 <= m_index && m_index <= 5;
        }

    private:
        chrono::weekday m_weekday;
        unsigned char m_index{};
    };

    constexpr bool operator==(const weekday_indexed& x, const weekday_indexed& y) noexcept
    {
        return x.weekday() == y.weekday() && x.index() == y.index();
    }
    constexpr bool operator!=(const weekday_indexed& x, const weekday_indexed& y) noexcept
    {
        return !operator==(x, y);
    }

    //! implementing weekday operator[] after the weekday_indexed definition
    constexpr weekday_indexed weekday::operator[](unsigned index) const noexcept
    {
        return { *this, index };
    }
#endif

    // [time.cal.wdlast](http://eel.is/c++draft/time#cal.wdlast), class weekday_­last
#if __cpp_lib_chrono >= 201907L
    using std::chrono::weekday_last;
#else
    class weekday_last
    {
    public:
        constexpr explicit weekday_last(const chrono::weekday& wd) noexcept
            : m_weekday{ wd }
        {
        }

        constexpr chrono::weekday weekday() const noexcept
        {
            return m_weekday;
        }
        constexpr bool ok() const noexcept
        {
            return m_weekday.ok();
        }

    private:
        chrono::weekday m_weekday;
    };

    constexpr bool operator==(const weekday_last& x, const weekday_last& y) noexcept
    {
        return x.weekday() == y.weekday();
    }
    constexpr bool operator!=(const weekday_last& x, const weekday_last& y) noexcept
    {
        return !operator==(x, y);
    }

    //! implementing the weekday::operator[] after the weekday_last definition
    constexpr weekday_last weekday::operator[](last_spec) const noexcept
    {
        return weekday_last{ *this };
    }
#endif

    // [time.cal.md](http://eel.is/c++draft/time#cal.md), class month_­day
#if __cpp_lib_chrono >= 201907L
    using std::chrono::month_day;
#else
    class month_day
    {
    public:
        month_day() = default;
        constexpr month_day(const chrono::month& m, const chrono::day& d) noexcept
            : m_month{ m }
            , m_day{ d }
        {
        }

        constexpr chrono::month month() const noexcept
        {
            return m_month;
        }
        constexpr chrono::day day() const noexcept
        {
            return m_day;
        }
        constexpr bool ok() const noexcept
        {
            // When month is February, the number of days considered is 29
            // So Febuary 29th always passes the validity check
            // since this function doesn't take year into account
            return m_month.ok() && chrono::day{ 1 } < m_day &&
                m_day < chrono::day{ Internal::last_day_of_month_leap_year(unsigned{ m_month }) };
        }

    private:
        chrono::month m_month;
        chrono::day m_day;
    };

    constexpr bool operator==(const month_day& x, const month_day& y) noexcept
    {
        return x.month() == y.month() && x.day() == y.day();
    }
    constexpr bool operator!=(const month_day& x, const month_day& y) noexcept
    {
        return !operator==(x, y);
    }
#endif

    // [time.cal.mdlast](http://eel.is/c++draft/time#cal.mdlast), class month_­day_­last
#if __cpp_lib_chrono >= 201907L
    using std::chrono::month_day_last;
#else
    class month_day_last
    {
    public:
        constexpr explicit month_day_last(const chrono::month& wd) noexcept
            : m_month{ wd }
        {
        }

        constexpr chrono::month month() const noexcept
        {
            return m_month;
        }
        constexpr bool ok() const noexcept
        {
            return m_month.ok();
        }

    private:
        chrono::month m_month;
    };

    constexpr bool operator==(const month_day_last& x, const month_day_last& y) noexcept
    {
        return x.month() == y.month();
    }
    constexpr bool operator!=(const month_day_last& x, const month_day_last& y) noexcept
    {
        return !operator==(x, y);
    }
#endif

    // [time.cal.mwd](http://eel.is/c++draft/time#cal.mwd), class month_­weekday
#if __cpp_lib_chrono >= 201907L
    using std::chrono::month_weekday;
#else
    class month_weekday
    {
    public:
        month_weekday() = default;
        constexpr month_weekday(const chrono::month& m, const chrono::weekday_indexed& wdi) noexcept
            : m_month{ m }
            , m_weekday_indexed{ wdi }
        {
        }

        constexpr chrono::month month() const noexcept
        {
            return m_month;
        }
        constexpr chrono::weekday_indexed weekday_indexed() const noexcept
        {
            return m_weekday_indexed;
        }
        constexpr bool ok() const noexcept
        {
            // When month is February, the number of days considered is 29
            // So Febuary 29th always passes the validity check(since this function doesn't take into account year)
            return m_month.ok() && m_weekday_indexed.ok();
        }

    private:
        chrono::month m_month;
        chrono::weekday_indexed m_weekday_indexed;
    };

    constexpr bool operator==(const month_weekday& x, const month_weekday& y) noexcept
    {
        return x.month() == y.month() && x.weekday_indexed() == y.weekday_indexed();
    }
    constexpr bool operator!=(const month_weekday& x, const month_weekday& y) noexcept
    {
        return !operator==(x, y);
    }
#endif

    // [time.cal.mwdlast](http://eel.is/c++draft/time#cal.mwdlast), class month_­weekday_­last
#if __cpp_lib_chrono >= 201907L
    using std::chrono::month_weekday_last;
#else
    class month_weekday_last
    {
    public:
        constexpr month_weekday_last(const chrono::month& m, const chrono::weekday_last& wdl) noexcept
            : m_month{ m }
            , m_weekday_last{ wdl }
        {
        }

        constexpr chrono::month month() const noexcept
        {
            return m_month;
        }
        constexpr chrono::weekday_last weekday_last() const noexcept
        {
            return m_weekday_last;
        }
        constexpr bool ok() const noexcept
        {
            // When month is February, the number of days considered is 29
            // So Febuary 29th always passes the validity check(since this function doesn't take into account year)
            return m_month.ok() && m_weekday_last.ok();
        }

    private:
        chrono::month m_month;
        chrono::weekday_last m_weekday_last;
    };

    constexpr bool operator==(const month_weekday_last& x, const month_weekday_last& y) noexcept
    {
        return x.month() == y.month() && x.weekday_last() == y.weekday_last();
    }
    constexpr bool operator!=(const month_weekday_last& x, const month_weekday_last& y) noexcept
    {
        return !operator==(x, y);
    }
#endif


    // [time.cal.ym](http://eel.is/c++draft/time#cal.ym), class year_­month
#if __cpp_lib_chrono >= 201907L
    using std::chrono::year_month;
#else
    class year_month
    {
    public:
        year_month() = default;
        constexpr year_month(const chrono::year& y, const chrono::month& m)
            : m_year{ y }
            , m_month{ m }
        {
        }

        constexpr chrono::year year() const noexcept
        {
            return m_year;
        }
        constexpr chrono::month month() const noexcept
        {
            return m_month;
        }

        constexpr year_month& operator+=(const months& dm) noexcept
        {
            *this = *this + dm;
            return *this;
        }
        constexpr year_month& operator-=(const months& dm) noexcept
        {
            *this = *this - dm;
            return *this;
        }

        constexpr year_month& operator+=(const years& dy) noexcept
        {
            *this = *this + dy;
            return *this;
        }
        constexpr year_month& operator-=(const years& dy) noexcept
        {
            *this = *this - dy;
            return *this;
        }

        constexpr bool ok() const noexcept
        {
            return m_year.ok() && m_month.ok();
        }

        friend constexpr year_month operator+(const year_month& ym, const months& dm) noexcept;
        friend constexpr year_month operator-(const year_month& ym, const months& dm) noexcept;
        friend constexpr year_month operator+(const year_month& ym, const years& dy) noexcept;
        friend constexpr year_month operator-(const year_month& ym, const years& dy) noexcept;

    private:
        chrono::year m_year;
        chrono::month m_month;
    };

    // forward declare operator/ functions which are used in friend functions for year_month
    constexpr year_month operator/(const year& y, const month& m) noexcept;

    constexpr bool operator==(const year_month& x, const year_month& y) noexcept
    {
        return x.year() == y.year() && x.month() == y.month();
    }
    constexpr bool operator!=(const year_month& x, const year_month& y) noexcept
    {
        return !operator==(x, y);
    }

    constexpr year_month operator+(const year_month& ym, const months& dm) noexcept
    {
        // chrono::month is 1-based while chrono::months is 0-based
        // so adding a month such as September(9th month) to 4 months should result
        // in overflowing to the next year with a chrono::month value of January(1st month)
        // however substracting 9 months from september should result
        // in underflowing to the previous year with a chrono::month value of December(12th month)
        // euclidean division takes care of updating the year value
        // i.e (9 + (4 - 1)) / 12 = 1 (with rem = 0), while (9 + (-9 - 1)) / 12 = -1 /12 = -1 (with rem = 11)
        // The remainder is the 0-based month [0, 11] and is normalized back into the range of [1, 12]
        const auto serial_months = static_cast<long long>(unsigned{ ym.month() }) + (dm.count() - 1);

        return year_month{ ym.year() + years{ Internal::euclidean_divide(serial_months, 12) },
                           month{ unsigned(Internal::euclidean_modulo(serial_months, 12) + 1) } };
    }
    constexpr year_month operator+(const months& dm, const year_month& ym) noexcept
    {
        return ym + dm;
    }
    constexpr year_month operator-(const year_month& ym, const months& dm) noexcept
    {
        return ym + -dm;
    }

    constexpr months operator-(const year_month& x, const year_month& y) noexcept
    {
        return x.year() - y.year() + months{ static_cast<int>(unsigned{ x.month() }) - static_cast<int>(unsigned{ y.month() }) };
    }

    constexpr year_month operator+(const year_month& ym, const years& dy) noexcept
    {
        return (ym.year() + dy) / ym.month();
    }
    constexpr year_month operator+(const years& dy, const year_month& ym) noexcept
    {
        return ym + dy;
    }
    constexpr year_month operator-(const year_month& ym, const years& dy) noexcept
    {
        return ym + -dy;
    }
#endif

    // [time.cal.ymd](http://eel.is/c++draft/time#cal.ymd), class year_­month_­day
#if __cpp_lib_chrono >= 201907L
    using std::chrono::year_month_day;
    using std::chrono::year_month_last;
#else
    namespace Internal
    {
        // Implement the days_from_civil
        // function to caluclate the sys_days since the epoch
        // The algorithm is listed at https://howardhinnant.github.io/date_algorithms.html#days_from_civil
        //! Calculates the day of the year given a month
        //! Convert the year to day in the range [0, 365]
        //! March 1 -> day 0
        //! April 1 -> day 31
        //! May 1 -> day 61
        //! June 1 -> day 92
        //! July 1 -> day 122
        //! August 1 -> day 153
        //! September 1 -> day 184
        //! October 1 -> day 214
        //! November 1 -> day 245
        //! December 1 -> day 275
        //! January 1 -> day 306
        //! February 1 -> day 337
        static constexpr int day_from_month(int month)
        {
            return (153 * month + 2) / 5;
        }

        static_assert(day_from_month(0) == 0, "March 1 should be day 0");
        static_assert(day_from_month(1) == 31, "April 1 should be day 31");
        static_assert(day_from_month(2) == 61, "May 1 should be day 61");
        static_assert(day_from_month(3) == 92, "June 1 should be day 92");
        static_assert(day_from_month(4) == 122, "July 1 should be day 122");
        static_assert(day_from_month(5) == 153, "August 1 should be day 153");
        static_assert(day_from_month(6) == 184, "September 1 should be day 184");
        static_assert(day_from_month(7) == 214, "October 1 should be day 214");
        static_assert(day_from_month(8) == 245, "November 1 should be day 245");
        static_assert(day_from_month(9) == 275, "December 1 should be day 275");
        static_assert(day_from_month(10) == 306, "January 1 should be day 306");
        static_assert(day_from_month(11) == 337, "February 1 should be day 337");

        //! This caluclates a month given the day of the year
        //! This is the reverse of day_from_month
        static constexpr int month_from_day(int day)
        {
            return (5 * day + 2) / 153;
        }

        static_assert(month_from_day(0) == 0, "day 0 should be March 1");
        static_assert(month_from_day(31) == 1, "day 31 should be April 1");
        static_assert(month_from_day(61) == 2, "day 61 should be May 1");
        static_assert(month_from_day(92) == 3, "day 92 be June 1");
        static_assert(month_from_day(122) == 4, "day 122 should be July 1");
        static_assert(month_from_day(153) == 5, "day 153 should be August 1");
        static_assert(month_from_day(184) == 6, "day 184 should be September 1");
        static_assert(month_from_day(214) == 7, "day 214 should be October 1");
        static_assert(month_from_day(245) == 8, "day 245 should be November 1");
        static_assert(month_from_day(275) == 9, " day 276 should be December 1");
        static_assert(month_from_day(306) == 10, "day 306 should be January 1");
        static_assert(month_from_day(337) == 11, "day 337 should be February 1");

        // The civil_from_days function is used to calculate a triple of
        // year, month, day from a serial date
        // https://howardhinnant.github.io/date_algorithms.html#civil_from_days
        static constexpr void civil_from_days(chrono::year& y, chrono::month& m, chrono::day& d, const sys_days& dp)
        {
            // adding 719468 days normalizes the system clock epoch of 1970-01-01 to beginning of an "era 0"
            // which is march first of a year 0: 0000-03-01
            // March first is chosen as the first day of the year because it moves the leap day of February 29th
            // the last day of the year
            long long era_normalization = dp.time_since_epoch().count() + 719468;
            // retrieve the era the day is in within by dividing by the number of days within an era
            // which is the length of 400 Gregorian years (365.2425days/yr * 400yr = 146097 days)
            const int era = int(Internal::euclidean_divide(era_normalization, 146097));
            // determine the day within the era by subtracting the out the beginning day of the era
            const unsigned day_of_era = static_cast<unsigned>(era_normalization - era * 146097); // [0, 146096]
            // calculate the year within the era
            // this is done by taking the day within era,
            // then subtracting out any leap days that occur ever 4 years (365.2425 * 4 = 1460.99) which floors to 1460
            // but years which are multiples of 100 aren't leap years,
            // therefore add added back in (365.2425 * 100 = 36524.25) which floors to 36524
            // Finally if the day within era happens to be the last day, such as Feburary 29th 400 (0400-02-29)
            // then that day is a leap day and is removed
            // Finally dividing by 365 provides the year within the era
            const unsigned year_of_era = (day_of_era - day_of_era / 1460 + day_of_era / 36524 - day_of_era / 146096) / 365; // [0, 399]
            // get the actual year by converting and era -> years and adding the year of the era to it
            const int year = static_cast<int>(year_of_era) + era * 400;
            // locate the serial day which begins the the year within the era and subtract that from the day with the era
            // to find the day of the year
            const unsigned day_of_year = day_of_era - (365 * year_of_era + year_of_era / 4 - year_of_era / 100); // [0, 365]
            // calculate the month from the day_of_year is in
            // this is normalized so that month 0 is March and month 11 is February
            const unsigned month_of_year_normalized = month_from_day(day_of_year); // [0, 11]
            // Find the first day of the month and subtract it out from the day of the year
            // to find the day of the month
            const unsigned day_of_month = day_of_year - day_from_month(month_of_year_normalized) + 1; // [1, 31]
            // undo the normalization of the first month of the year
            // i.e map March (0 -> 3), April (1 -> 4), ... January (10 -> 1), February (11 -> 2)
            const unsigned month = month_of_year_normalized < 10 ? month_of_year_normalized + 3 : month_of_year_normalized - 9; // [1, 12]
            // a month of <=2 is really January and February of the next year, since it was normalized
            y = chrono::year{ year + (month <= 2) };
            m = chrono::month{ month };
            d = chrono::day{ day_of_month };
        }

        // The days_from_civil function computes the serial date from a triple of year, month, day
        // of the Gregorian calendar
        // https://howardhinnant.github.io/date_algorithms.html#days_from_civil
        static constexpr sys_days days_from_civil(const chrono::year& y, const chrono::month& m, const chrono::day& d)
        {
            auto year = int{ y };
            auto month = unsigned{ m };
            auto day = unsigned{ d };
            // remap the year so that it starts in March
            year -= (month <= 2);
            // determine which era the year is in
            // era -1 start date is -0400-03-01, end date is 0000-02-29
            // era 0 start date is 0000-03-01, end date is 0400-02-29
            // era 1 start date is 0400-03-01, end date is 0800-02-29
            // era 2 start date is 0800-03-01, end date is 1200-02-29
            // era 3 start date is 1200-03-01, end date is 1600-02-29
            // era 4 start date is 1600-03-01, end date is 2000-02-29
            // era 5 start date is 2000-03-01, end date is 2400-02-29
            // ...
            const int era = int(Internal::euclidean_divide(year, 400));
            // Retrieve the year of within the era
            const unsigned year_of_era = static_cast<unsigned>(year - era * 400); // [0, 399]
            // Retrieve the day of the year
            const unsigned day_of_year = day_from_month(month > 2 ? month - 3 : month + 9) + day - 1; // [0, 365]
            // Retrieve the day within the era
            // This does this by first calculating the 365 days * the year of the era to form a 365 day year
            // Then it adds the leap years which occur ever 4 years,
            // but also subtracts out the years which are multiples of 100 which aren't not leap years
            // Finally it appends the current day of the year
            const unsigned day_of_era = year_of_era * 365 + year_of_era / 4 - year_of_era / 100 + day_of_year; // [0, 146096]
            // Multiply the era by the number of days within it (365.2425 days/yr * 400yr) = 146097 days
            // Then add the current day of the era
            // Finally substract 719468 days which is the number of days from the 0000-03-01 to 1970-01-01 which is the epoch
            // This makes it so that the serial date of 0 is equal to the system_clock epoch
            return sys_days{ days{ era * 146097 + static_cast<int>(day_of_era) - 719468 } };
        }
    } // namespace Internal

    // forward declare year_month_last for constructor
    class year_month_day_last;

    // forward declare operator/ functions which are used in year_month_day
    class year_month_day;
    constexpr year_month_day operator/(const year_month& ym, const day& d) noexcept;

    class year_month_day
    {
    public:
        year_month_day() = default;
        constexpr year_month_day(const chrono::year& y, const chrono::month& m, const chrono::day& d) noexcept
            : m_year{ y }
            , m_month{ m }
            , m_day{ d }
        {
        }

        //! Must be implemented after the year_month_day_last class definition
        constexpr year_month_day(const year_month_day_last& ymdl) noexcept;

        constexpr year_month_day(const sys_days& dp) noexcept
        {
            Internal::civil_from_days(m_year, m_month, m_day, dp);
        }
        constexpr explicit year_month_day(const local_days& dp) noexcept
            : year_month_day{ sys_days{ dp.time_since_epoch() } }
        {
        }

        constexpr year_month_day& operator+=(const months& dm) noexcept
        {
            *this = *this + dm;
            return *this;
        }
        constexpr year_month_day& operator-=(const months& dm) noexcept
        {
            *this = *this - dm;
            return *this;
        }

        constexpr year_month_day& operator+=(const years& dy) noexcept
        {
            *this = *this + dy;
            return *this;
        }
        constexpr year_month_day& operator-=(const years& dy) noexcept
        {
            *this = *this - dy;
            return *this;
        }

        constexpr chrono::year year() const noexcept
        {
            return m_year;
        }
        constexpr chrono::month month() const noexcept
        {
            return m_month;
        }
        constexpr chrono::day day() const noexcept
        {
            return m_day;
        }

        constexpr operator sys_days() const noexcept
        {
            // The algorithm makes use of an "era" which is a 400 year period.
            // As the Gregorian Calendar repeats every 400 years, tha algorithm
            // will work for any era
            // March 1 will be the first day of the year, which makes February 28 or February 29th
            // the last day of the previous year.
            // This makes it easy to add the leap day when needed
            if (ok())
            {
                return Internal::days_from_civil(year(), month(), day());
            }
            else if (m_year.ok() && m_month.ok())
            {
                return sys_days{ m_year / m_month / chrono::day{ 1 } } + (m_day - chrono::day{ 1 });
            }
            else
            {
                return Internal::days_from_civil(year(), month(), day());
            }
        }
        constexpr explicit operator local_days() const noexcept
        {
            return local_days{ sys_days{ *this }.time_since_epoch() };
        }

        constexpr bool ok() const noexcept
        {
            return m_year.ok() && m_month.ok() && m_day.ok()
                && unsigned{m_day} <= Internal::last_day_of_month(int{ m_year }, unsigned{ m_month });
        }

        friend constexpr year_month_day operator+(const year_month_day& ymd, const months& dm) noexcept;
        friend constexpr year_month_day operator-(const year_month_day& ymd, const months& dm) noexcept;
        friend constexpr year_month_day operator+(const year_month_day& ymd, const years& dy) noexcept;
        friend constexpr year_month_day operator-(const year_month_day& ymd, const years& dy) noexcept;

    private:
        chrono::year m_year;
        chrono::month m_month;
        chrono::day m_day;
    };

    constexpr bool operator==(const year_month_day& x, const year_month_day& y) noexcept
    {
        return x.month() == y.month() && x.day() == y.day();
    }
    constexpr bool operator!=(const year_month_day& x, const year_month_day& y) noexcept
    {
        return !operator==(x, y);
    }

    constexpr year_month_day operator+(const year_month_day& ymd, const months& dm) noexcept
    {
        // This can result in year_month_day where ok is false
        // if the day is outside the range of the new month value. A day value of [1, 28] is safe
        return (ymd.year() / ymd.month() + dm) / ymd.day();
    }
    constexpr year_month_day operator+(const months& dm, const year_month_day& ymd) noexcept
    {
        return ymd + dm;
    }
    constexpr year_month_day operator+(const year_month_day& ymd, const years& dy) noexcept
    {
        // This can result in year_month_day where ok is false if
        // the month is February and the is 29
        return (ymd.year() + dy) / ymd.month() / ymd.day();
    }
    constexpr year_month_day operator+(const years& dy, const year_month_day& ymd) noexcept
    {
        return ymd + dy;
    }
    constexpr year_month_day operator-(const year_month_day& ymd, const months& dm) noexcept
    {
        return ymd + -dm;
    }
    constexpr year_month_day operator-(const year_month_day& ymd, const years& dy) noexcept
    {
        return ymd + -dy;
    }
#endif

    // [time.cal.ymdlast](http://eel.is/c++draft/time#cal.ymdlast), class year_­month_­day_­last
#if __cpp_lib_chrono >= 201907L
    using std::chrono::year_month_day_last;
#else
    class year_month_day_last
    {
    public:
        constexpr year_month_day_last(const chrono::year& y, const chrono::month_day_last& mdl) noexcept
            : m_year{ y }
            , m_month_day_last{ mdl }
        {
        }

        constexpr chrono::year year() const noexcept
        {
            return m_year;
        }
        constexpr chrono::month month() const noexcept
        {
            return m_month_day_last.month();
        }
        constexpr chrono::month_day_last month_day_last() const noexcept
        {
            return m_month_day_last;
        }
        constexpr chrono::day day() const noexcept
        {
            return chrono::day{ Internal::last_day_of_month(int{ m_year }, unsigned{ month() }) };
        }

        constexpr operator sys_days() const noexcept
        {
            return sys_days{ year() / month() / day() };
        }
        constexpr explicit operator local_days() const noexcept
        {
            return local_days{ sys_days{ *this }.time_since_epoch() };
        }

        constexpr bool ok() const noexcept
        {
            return m_year.ok() && m_month_day_last.ok();
        }

    private:
        chrono::year m_year;
        chrono::month_day_last m_month_day_last;
    };

    //! implementation of year_month_day constructor which accepts a year_month_day_last
    constexpr year_month_day::year_month_day(const year_month_day_last& ymdl) noexcept
        : m_year{ ymdl.year() }
        , m_month{ ymdl.month() }
        , m_day{ ymdl.day() }
    {
    }

    //! forward declare operator/ for year_month_day friend functions
    constexpr year_month_day_last operator/(const year_month& ym, last_spec) noexcept;

    constexpr bool operator==(const year_month_day_last& x, const year_month_day_last& y) noexcept
    {
        return x.year() == y.year() && x.month_day_last() == y.month_day_last();
    }
    constexpr bool operator!=(const year_month_day_last& x, const year_month_day_last& y) noexcept
    {
        return !operator==(x, y);
    }

    constexpr year_month_day_last operator+(const year_month_day_last& ymdl, const months& dm) noexcept
    {
        return (ymdl.year() / ymdl.month() + dm) / last;
    }
    constexpr year_month_day_last operator+(const months& dm, const year_month_day_last& ymdl) noexcept
    {
        return ymdl + dm;
    }
    constexpr year_month_day_last operator+(const year_month_day_last& ymdl, const years& dy) noexcept
    {
        return { ymdl.year() + dy, ymdl.month_day_last() };
    }
    constexpr year_month_day_last operator+(const years& dy, const year_month_day_last& ymdl) noexcept
    {
        return ymdl + dy;
    }
    constexpr year_month_day_last operator-(const year_month_day_last& ymdl, const months& dm) noexcept
    {
        return ymdl + -dm;
    }
    constexpr year_month_day_last operator-(const year_month_day_last& ymdl, const years& dy) noexcept
    {
        return ymdl + -dy;
    }
#endif

    // [time.cal.ymwd](http://eel.is/c++draft/time#cal.ymwd), class year_­month_­weekday
#if __cpp_lib_chrono >= 201907L
    using std::chrono::year_month_weekday;
#else
    class year_month_weekday
    {
    public:
        year_month_weekday() = default;
        constexpr year_month_weekday(const chrono::year& y, const chrono::month& m, const chrono::weekday_indexed& wdi) noexcept
            : m_year{ y }
            , m_month{ m }
            , m_weekday_indexed{ wdi }
        {
        }
        constexpr year_month_weekday(const sys_days& dp) noexcept
        {
            chrono::day day;
            Internal::civil_from_days(m_year, m_month, day, dp);

            // compute weekday from serial day
            const chrono::weekday weekday{ dp };
            // compute the weekday_index using the day of the month and the weekday
            // chrono::day is 1-based [1, 31], so normalize it to 0-based [0, 30]
            // before dividing by 7 to compute the week of the month the day is in.
            // finally add 1 to the calucation since the weekday_indexed index()
            // is 1-based
            m_weekday_indexed = weekday[(unsigned{ day } - 1) / 7 + 1];
        }

        constexpr explicit year_month_weekday(const local_days& dp) noexcept
            : year_month_weekday{ sys_days{ dp.time_since_epoch() } }
        {
        }

        constexpr year_month_weekday& operator+=(const months& m) noexcept
        {
            *this = *this + m;
            return *this;
        }
        constexpr year_month_weekday& operator-=(const months& m) noexcept
        {
            *this = *this - m;
            return *this;
        }
        constexpr year_month_weekday& operator+=(const years& y) noexcept
        {
            *this = *this + y;
            return *this;
        }
        constexpr year_month_weekday& operator-=(const years& y) noexcept
        {
            *this = *this - y;
            return *this;
        }

        constexpr chrono::year year() const noexcept
        {
            return m_year;
        }
        constexpr chrono::month month() const noexcept
        {
            return m_month;
        }
        constexpr chrono::weekday weekday() const noexcept
        {
            return m_weekday_indexed.weekday();
        }
        constexpr unsigned index() const noexcept
        {
            return m_weekday_indexed.index();
        }
        constexpr chrono::weekday_indexed weekday_indexed() const noexcept
        {
            return m_weekday_indexed;
        }

        constexpr operator sys_days() const noexcept
        {
            if (m_year.ok() && m_month.ok() && m_weekday_indexed.ok())
            {
                // compute the serial date using the Get the first day of the year and month
                // and append the weekday_index to it
                const sys_days first_day_month_serial = Internal::days_from_civil(m_year, m_month, chrono::day{ 1 });
                const chrono::weekday first_weekday_month{ first_day_month_serial };
                return first_day_month_serial +
                    chrono::days{ Internal::weekday_difference(unsigned{ weekday() }, unsigned{ first_weekday_month }) +
                                  (index() - 1) * 7 };
            }
            else
            {
                AZ_Assert(
                    false,
                    "year month weekday combination of year=%i/month=%u/(weekday=%u,index=%u) is not valid",
                    int{ year() },
                    unsigned{ month() },
                    unsigned{ weekday() },
                    index());

                // Just return the first day of the month which happens to fall on the weekday in this case
                const sys_days first_day_month_serial = Internal::days_from_civil(m_year, m_month, chrono::day{ 1 });
                return first_day_month_serial +
                    chrono::days{ Internal::weekday_difference(
                        unsigned{ weekday() }, unsigned{ chrono::weekday{ first_day_month_serial } }) };
            }
        }
        constexpr explicit operator local_days() const noexcept
        {
            return local_days{ { sys_days{ *this }.time_since_epoch() } };
        }
        constexpr bool ok() const noexcept
        {
            if (!m_year.ok() || !m_month.ok() || !m_weekday_indexed.ok())
            {
                return false;
            }

            // Since every month as at least 28 days, any index that is in the range of [1, 4]
            // is valid
            if (index() <= 4)
            {
                return true;
            }

            // index() = 5 is valid but depends only if the day of the first occurrence of the weekday
            // in a given month + 4 weeks(28 days) falls with the last day of the month
            const sys_days first_day_month_serial = Internal::days_from_civil(m_year, m_month, chrono::day{ 1 });
            // The chrono::day class is 1-based, so add 1 to normalize the day between [1, 31]
            const unsigned weekday_of_month_first =
                Internal::weekday_difference(unsigned{ weekday() }, unsigned{ chrono::weekday{ first_day_month_serial } }) + 1;
            const unsigned weekday_of_month_last = weekday_of_month_first + 28;
            return weekday_of_month_last <= Internal::last_day_of_month(int{ year() }, unsigned{ month() });
        }

        friend constexpr year_month_weekday operator+(const year_month_weekday& ymwd, const months& dm) noexcept;
        friend constexpr year_month_weekday operator-(const year_month_weekday& ymwd, const months& dm) noexcept;
        friend constexpr year_month_weekday operator+(const year_month_weekday& ymwd, const years& dy) noexcept;
        friend constexpr year_month_weekday operator-(const year_month_weekday& ymwd, const years& dy) noexcept;

    private:
        chrono::year m_year;
        chrono::month m_month;
        chrono::weekday_indexed m_weekday_indexed;
    };

    //! forward declare operator/ for year_month_weekday friend functions
    constexpr year_month_weekday operator/(const year_month& ym, const weekday_indexed& wdi) noexcept;

    constexpr bool operator==(const year_month_weekday& x, const year_month_weekday& y) noexcept
    {
        return x.year() == y.year() && x.month() == y.month() && x.weekday_indexed() == y.weekday_indexed();
    }
    constexpr bool operator!=(const year_month_weekday& x, const year_month_weekday& y) noexcept
    {
        return !operator==(x, y);
    }

    constexpr year_month_weekday operator+(const year_month_weekday& ymwd, const months& dm) noexcept
    {
        return (ymwd.year() / ymwd.month() + dm) / ymwd.weekday_indexed();
    }
    constexpr year_month_weekday operator+(const months& dm, const year_month_weekday& ymwd) noexcept
    {
        return ymwd + dm;
    }
    constexpr year_month_weekday operator+(const year_month_weekday& ymwd, const years& dy) noexcept
    {
        return { ymwd.year() + dy, ymwd.month(), ymwd.weekday_indexed() };
    }
    constexpr year_month_weekday operator+(const years& dy, const year_month_weekday& ymwd) noexcept
    {
        return ymwd + dy;
    }
    constexpr year_month_weekday operator-(const year_month_weekday& ymwd, const months& dm) noexcept
    {
        return ymwd + -dm;
    }
    constexpr year_month_weekday operator-(const year_month_weekday& ymwd, const years& dy) noexcept
    {
        return ymwd + -dy;
    }
#endif

    // [time.cal.ymwdlast](http://eel.is/c++draft/time#cal.ymwdlast), class year_­month_­weekday_­last
#if __cpp_lib_chrono >= 201907L
    using std::chrono::year_month_weekday_last;
#else
    class year_month_weekday_last
    {
    public:
        constexpr year_month_weekday_last(const chrono::year& y, const chrono::month& m, const chrono::weekday_last& wdl) noexcept
            : m_year{ y }
            , m_month{ m }
            , m_weekday_last{ wdl }
        {
        }

        constexpr year_month_weekday_last& operator+=(const months& m) noexcept
        {
            *this = *this + m;
            return *this;
        }
        constexpr year_month_weekday_last& operator-=(const months& m) noexcept
        {
            *this = *this - m;
            return *this;
        }
        constexpr year_month_weekday_last& operator+=(const years& y) noexcept
        {
            *this = *this + y;
            return *this;
        }
        constexpr year_month_weekday_last& operator-=(const years& y) noexcept
        {
            *this = *this - y;
            return *this;
        }

        constexpr chrono::year year() const noexcept
        {
            return m_year;
        }
        constexpr chrono::month month() const noexcept
        {
            return m_month;
        }
        constexpr chrono::weekday weekday() const noexcept
        {
            return m_weekday_last.weekday();
        }
        constexpr chrono::weekday_last weekday_last() const noexcept
        {
            return m_weekday_last;
        }

        constexpr operator sys_days() const noexcept
        {
            if (ok())
            {
                // compute the serial data using the first day of the year, month
                const sys_days first_day_month_serial = Internal::days_from_civil(year(), month(), chrono::day{ 1 });
                // Compute the first day of the month the weekday falls on
                unsigned weekday_of_month_first =
                    Internal::weekday_difference(unsigned{ weekday() }, unsigned{ chrono::weekday{ first_day_month_serial } }) + 1;
                // append 4 weeks(28 days) to the day of first occurrence of the weekday
                // and if it is below the number days within the month return the result
                // otherwise append 3 weeks(21 days) which is guaranteed to fall within a month(since every month has a least 28 days)
                return first_day_month_serial +
                    chrono::days{ weekday_of_month_first +
                                  (weekday_of_month_first + 28 <= Internal::last_day_of_month(int{ year() }, unsigned{ month() }) ? 28
                                                                                                                                  : 21) };
            }
            else
            {
                AZ_Assert(
                    false,
                    "year month weekday_last combination of year=%i/month=%u/(weekday=%u) is not valid",
                    int{ year() },
                    unsigned{ month() },
                    unsigned{ weekday() });

                // Fallback to returning the last day of the month
                return Internal::days_from_civil(year(), month(), chrono::day{ Internal::last_day_of_month(int{ year() }, unsigned{ month() }) });
            }
        }
        constexpr explicit operator local_days() const noexcept
        {
            return local_days{ { sys_days{ *this }.time_since_epoch() } };
        }
        constexpr bool ok() const noexcept
        {
            return m_year.ok() && m_month.ok() && m_weekday_last.ok();
        }

        friend constexpr year_month_weekday_last operator+(const year_month_weekday_last& ymwdl, const months& dm) noexcept;
        friend constexpr year_month_weekday_last operator-(const year_month_weekday_last& ymwdl, const months& dm) noexcept;
        friend constexpr year_month_weekday_last operator+(const year_month_weekday_last& ymwdl, const years& dy) noexcept;
        friend constexpr year_month_weekday_last operator-(const year_month_weekday_last& ymwdl, const years& dy) noexcept;

    private:
        chrono::year m_year;
        chrono::month m_month;
        chrono::weekday_last m_weekday_last;
    };

    //! forward declare operator/ for friend functions of year_month_weekday_last
    constexpr year_month_weekday_last operator/(const year_month& ym, const weekday_last& wdl) noexcept;

    constexpr bool operator==(const year_month_weekday_last& x, const year_month_weekday_last& y) noexcept
    {
        return x.year() == y.year() && x.month() == y.month() && x.weekday_last() == y.weekday_last();
    }
    constexpr bool operator!=(const year_month_weekday_last& x, const year_month_weekday_last& y) noexcept
    {
        return !operator==(x, y);
    }

    constexpr year_month_weekday_last operator+(const year_month_weekday_last& ymwdl, const months& dm) noexcept
    {
        return (ymwdl.year() / ymwdl.month() + dm) / ymwdl.weekday_last();
    }
    constexpr year_month_weekday_last operator+(const months& dm, const year_month_weekday_last& ymwdl) noexcept
    {
        return ymwdl + dm;
    }
    constexpr year_month_weekday_last operator+(const year_month_weekday_last& ymwdl, const years& dy) noexcept
    {
        return { ymwdl.year() + dy, ymwdl.month(), ymwdl.weekday_last() };
    }
    constexpr year_month_weekday_last operator+(const years& dy, const year_month_weekday_last& ymwdl) noexcept
    {
        return ymwdl + dy;
    }
    constexpr year_month_weekday_last operator-(const year_month_weekday_last& ymwdl, const months& dm) noexcept
    {
        return ymwdl + -dm;
    }
    constexpr year_month_weekday_last operator-(const year_month_weekday_last& ymwdl, const years& dy) noexcept
    {
        return ymwdl + -dy;
    }
#endif

    // [time.cal.operators](http://eel.is/c++draft/time#cal.operators), civil calendar conventional syntax operators
#if __cpp_lib_chrono < 201907L
    // Implement the operator/ for combining dates
    constexpr year_month operator/(const year& y, const month& m) noexcept
    {
        return { y, m };
    }
    constexpr year_month operator/(const year& y, int m) noexcept
    {
        return y / month{ static_cast<unsigned>(m) };
    }
    constexpr month_day operator/(const month& m, const day& d) noexcept
    {
        return { m, d };
    }
    constexpr month_day operator/(const month& m, int d) noexcept
    {
        return m / day{ static_cast<unsigned>(d) };
    }
    constexpr month_day operator/(int m, const day& d) noexcept
    {
        return month{ static_cast<unsigned>(m) } / d;
    }
    constexpr month_day operator/(const day& d, const month& m) noexcept
    {
        return m / d;
    }
    constexpr month_day operator/(const day& d, int m) noexcept
    {
        return month{ static_cast<unsigned>(m) } / d;
    }
    constexpr month_day_last operator/(const month& m, last_spec) noexcept
    {
        return month_day_last{ m };
    }
    constexpr month_day_last operator/(int m, last_spec) noexcept
    {
        return month{ static_cast<unsigned>(m) } / last;
    }
    constexpr month_day_last operator/(last_spec, const month& m) noexcept
    {
        return month_day_last{ m };
    }
    constexpr month_day_last operator/(last_spec, int m) noexcept
    {
        return month{ static_cast<unsigned>(m) } / last;
    }
    constexpr month_weekday operator/(const month& m, const weekday_indexed& wdi) noexcept
    {
        return { m, wdi };
    }
    constexpr month_weekday operator/(int m, const weekday_indexed& wdi) noexcept
    {
        return month{ static_cast<unsigned>(m) } / wdi;
    }
    constexpr month_weekday operator/(const weekday_indexed& wdi, const month& m) noexcept
    {
        return m / wdi;
    }
    constexpr month_weekday operator/(const weekday_indexed& wdi, int m) noexcept
    {
        return month{ static_cast<unsigned>(m) } / wdi;
    }
    constexpr month_weekday_last operator/(const month& m, const weekday_last& wdl) noexcept
    {
        return { m, wdl };
    }
    constexpr month_weekday_last operator/(int m, const weekday_last& wdl) noexcept
    {
        return month{ static_cast<unsigned>(m) } / wdl;
    }
    constexpr month_weekday_last operator/(const weekday_last& wdl, const month& m) noexcept
    {
        return m / wdl;
    }
    constexpr month_weekday_last operator/(const weekday_last& wdl, int m) noexcept
    {
        return month{ static_cast<unsigned>(m) } / wdl;
    }
    constexpr year_month_day operator/(const year_month& ym, const day& d) noexcept
    {
        return { ym.year(), ym.month(), d };
    }
    constexpr year_month_day operator/(const year_month& ym, int d) noexcept
    {
        return ym / day{ static_cast<unsigned>(d) };
    }
    constexpr year_month_day operator/(const year& y, const month_day& md) noexcept
    {
        return y / md.month() / md.day();
    }
    constexpr year_month_day operator/(int y, const month_day& md) noexcept
    {
        return year{ y } / md;
    }
    constexpr year_month_day operator/(const month_day& md, const year& y) noexcept
    {
        return y / md;
    }
    constexpr year_month_day operator/(const month_day& md, int y) noexcept
    {
        return year{ y } / md;
    }
    constexpr year_month_day_last operator/(const year_month& ym, last_spec) noexcept
    {
        return { ym.year(), month_day_last{ ym.month() } };
    }
    constexpr year_month_day_last operator/(const year& y, const month_day_last& mdl) noexcept
    {
        return { y, mdl };
    }
    constexpr year_month_day_last operator/(int y, const month_day_last& mdl) noexcept
    {
        return year{ y } / mdl;
    }
    constexpr year_month_day_last operator/(const month_day_last& mdl, const year& y) noexcept
    {
        return y / mdl;
    }
    constexpr year_month_day_last operator/(const month_day_last& mdl, int y) noexcept
    {
        return year{ y } / mdl;
    }
    constexpr year_month_weekday operator/(const year_month& ym, const weekday_indexed& wdi) noexcept
    {
        return { ym.year(), ym.month(), wdi };
    }
    constexpr year_month_weekday operator/(const year& y, const month_weekday& mwd) noexcept
    {
        return { y, mwd.month(), mwd.weekday_indexed() };
    }
    constexpr year_month_weekday operator/(int y, const month_weekday& mwd) noexcept
    {
        return year{ y } / mwd;
    }
    constexpr year_month_weekday operator/(const month_weekday& mwd, const year& y) noexcept
    {
        return y / mwd;
    }
    constexpr year_month_weekday operator/(const month_weekday& mwd, int y) noexcept
    {
        return year{ y } / mwd;
    }
    constexpr year_month_weekday_last operator/(const year_month& ym, const weekday_last& wdl) noexcept
    {
        return { ym.year(), ym.month(), wdl };
    }
    constexpr year_month_weekday_last operator/(const year& y, const month_weekday_last& mwdl) noexcept
    {
        return { y, mwdl.month(), mwdl.weekday_last() };
    }
    constexpr year_month_weekday_last operator/(int y, const month_weekday_last& mwdl) noexcept
    {
        return year{ y } / mwdl;
    }
    constexpr year_month_weekday_last operator/(const month_weekday_last& mwdl, const year& y) noexcept
    {
        return y / mwdl;
    }
    constexpr year_month_weekday_last operator/(const month_weekday_last& mwdl, int y) noexcept
    {
        return year{ y } / mwdl;
    }
#endif

    // [time.hms](http://eel.is/c++draft/time#hms), class template hh_­mm_­ss
#if __cpp_lib_chrono >= 201907L
    using std::chrono::hh_mm_ss;
#else
    template<class Duration>
    class hh_mm_ss
    {
        // Calculates a power of 10 at compile time
        static constexpr unsigned long long pow_10_exponentiate(unsigned exp)
        {
            unsigned long long value = 1;
            for (unsigned i = 0; i < exp; ++i)
            {
                value *= 10;
            }

            return value;
        }
    public:
        // The fractional width must be a power of 10
        // between [0, 18]
        // If the there is fractional_width in the range,
        // that can exactly represent all values of the duration
        // then it defaults to 6
        // http://eel.is/c++draft/time#hms.members
        static constexpr unsigned fractional_width = []()
        {
            constexpr unsigned MaxExp = 19;
            unsigned decimalExp = 0;
            // If the numerator can divide the denominator without remainder
            // then their is ratio of 1 / 10^(num) that can exactly store
            // all values of the duration
            // Multiplying the remainder by 10 will shift the decimal point over to the left
            // until either the numeator can exactly divide the denominator
            // or the algorithm gives up once it reaches 19 attempts
            // (64-bit number can only represent a decimal value exactly on the order of 10^18)
            for (auto num = Duration::period::num, den = Duration::period::den; num % den != 0 && decimalExp < MaxExp;
                 num = (num % den) * 10, ++decimalExp)
            {
            }

            return decimalExp == MaxExp ? 6 : decimalExp;
        }();

        using precision = duration<common_type_t<typename Duration::rep, typename std::chrono::seconds::rep>, ratio<1, pow_10_exponentiate(fractional_width)>>;

        hh_mm_ss() noexcept
            : hh_mm_ss(Duration::zero())
        {
        }

        constexpr hh_mm_ss(Duration d) noexcept
            : m_is_negative{ d < Duration::zero() }
            , m_hours{ duration_cast<chrono::hours>(abs(d)) }
            , m_minutes{ duration_cast<chrono::minutes>(abs(d) - m_hours) }
            , m_seconds{ duration_cast<std::chrono::seconds>(abs(d) - m_hours - m_minutes) }
            , m_subseconds{ precision::zero() }
        {
            if constexpr (treat_as_floating_point_v<typename precision::rep>)
            {
                m_subseconds = abs(d) - hours() - minutes() - seconds();
            }
            else
            {
                m_subseconds = duration_cast<precision>(abs(d) - hours() - minutes() - seconds());
            }
        }

        constexpr bool is_negative() const noexcept
        {
            return m_is_negative;
        }

        constexpr chrono::hours hours() const noexcept
        {
            return m_hours;
        }
        constexpr chrono::minutes minutes() const noexcept
        {
            return m_minutes;
        }
        constexpr std::chrono::seconds seconds() const noexcept
        {
            return m_seconds;
        }

        constexpr precision subseconds() const noexcept
        {
            return m_subseconds;
        }

        constexpr precision to_duration() const noexcept
        {
            return m_is_negative ? -(hours() + minutes() + seconds() + subseconds()) : hours() + minutes() + seconds() + subseconds();
        }

        constexpr explicit operator precision() const noexcept
        {
            return to_duration();
        }

    private:
        bool m_is_negative{};
        chrono::hours m_hours;
        chrono::minutes m_minutes;
        std::chrono::seconds m_seconds;
        precision m_subseconds;
    };
#endif

    // [time.12](http://eel.is/c++draft/time#12), 12/24 hour functions
#if __cpp_lib_chrono >= 201907L
    using std::chrono::is_am;
    using std::chrono::is_pm;
    using std::chrono::make12;
    using std::chrono::make24;
#else
    constexpr bool is_am(const hours& h) noexcept
    {
        return hours{ 0 } <= h && h <= hours{ 11 };
    }
    constexpr bool is_pm(const hours& h) noexcept
    {
        return hours{ 12 } <= h && h <= hours{ 23 };
    }

    constexpr hours make12(const hours& h) noexcept
    {
        return hours{ Internal::euclidean_modulo(h.count() - 1, 12) + 1 };
    }
    constexpr hours make24(const hours& h, bool is_pm) noexcept
    {
        return hours{ Internal::euclidean_modulo(h.count(), 12) + (is_pm ? 12 : 0) };
    }
#endif

    // calendrical constants
#if __cpp_lib_chrono >= 201907L
    using std::chrono::Friday;
    using std::chrono::last;
    using std::chrono::Monday;
    using std::chrono::Saturday;
    using std::chrono::Sunday;
    using std::chrono::Thursday;
    using std::chrono::Tuesday;
    using std::chrono::Wednesday;

    using std::chrono::April;
    using std::chrono::August;
    using std::chrono::December;
    using std::chrono::February;
    using std::chrono::January;
    using std::chrono::July;
    using std::chrono::June;
    using std::chrono::March;
    using std::chrono::May;
    using std::chrono::November;
    using std::chrono::October;
    using std::chrono::September;
#else
    inline constexpr weekday Sunday{ 0 };
    inline constexpr weekday Monday{ 1 };
    inline constexpr weekday Tuesday{ 2 };
    inline constexpr weekday Wednesday{ 3 };
    inline constexpr weekday Thursday{ 4 };
    inline constexpr weekday Friday{ 5 };
    inline constexpr weekday Saturday{ 6 };

    inline constexpr month January{ 1 };
    inline constexpr month February{ 2 };
    inline constexpr month March{ 3 };
    inline constexpr month April{ 4 };
    inline constexpr month May{ 5 };
    inline constexpr month June{ 6 };
    inline constexpr month July{ 7 };
    inline constexpr month August{ 8 };
    inline constexpr month September{ 9 };
    inline constexpr month October{ 10 };
    inline constexpr month November{ 11 };
    inline constexpr month December{ 12 };
#endif

    // time zone functions are not needed yet, so no bringing their
    // names into AZStd scope has not been done
} // namespace AZStd::chrono

namespace AZStd
{
    inline namespace literals
    {
        inline namespace chrono_literals
        {
            using namespace std::literals::chrono_literals;

            // [time.cal.day.nonmembers](http://eel.is/c++draft/time#cal.day.nonmembers), non-member functions
    #if __cpp_lib_chrono < 201907L
            constexpr chrono::day operator""_d(unsigned long long d) noexcept
            {
                return chrono::day{ static_cast<unsigned int>(d) };
            }
    #endif

            // [time.cal.year.nonmembers](http://eel.is/c++draft/time#cal.year.nonmembers), non-member functions
    #if __cpp_lib_chrono < 201907L
            constexpr chrono::year operator""_y(unsigned long long y) noexcept
            {
                return chrono::year{ static_cast<int>(y) };
            }
    #endif
        }
    } // namespace literals

    namespace chrono
    {
        using namespace literals::chrono_literals;
    }
} // namespace AZStd
