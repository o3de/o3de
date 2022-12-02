/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/time.h>
#include <AzCore/std/limits.h>
#include <AzCore/std/typetraits/is_integral.h>
#include <AzCore/std/typetraits/is_signed.h>

#include "UserTypes.h"

namespace UnitTest
{
    //////////////////////////////////////////////////////////////////////////
    // Fixtures

    // Fixture for non-typed tests
    class DurationTest
        : public LeakDetectionFixture
    {
    };

    /*
     * Helper Type Trait structure for adding expected value to typed
     */
    template<typename TestType, size_t RequiredBits, typename ExpectedPeriodType>
    struct DurationExpectation
    {
        using test_type = TestType;
        using rep = typename TestType::rep;
        using period = typename TestType::period;
        using expected_period = ExpectedPeriodType;
        static constexpr bool is_signed = AZStd::is_signed<int64_t>::value;
        static constexpr bool is_integral = AZStd::is_integral<int64_t>::value;
        static constexpr size_t required_bits = RequiredBits;
    };

    // Fixture for typed tests
    template<typename ExpectedResultTraits>
    class DurationTypedTest : public LeakDetectionFixture
    {
    };
    using ChronoTestTypes = ::testing::Types<
        DurationExpectation<AZStd::chrono::nanoseconds, 63, AZStd::nano>,
        DurationExpectation<AZStd::chrono::microseconds, 54, AZStd::micro>,
        DurationExpectation<AZStd::chrono::milliseconds, 44, AZStd::milli>,
        DurationExpectation<AZStd::chrono::seconds, 44, AZStd::ratio<1>>,
        DurationExpectation<AZStd::chrono::minutes, 28, AZStd::ratio<60>>,
        DurationExpectation<AZStd::chrono::hours, 22, AZStd::ratio<3600>>>;
    TYPED_TEST_CASE(DurationTypedTest, ChronoTestTypes);

    //////////////////////////////////////////////////////////////////////////
    // Tests for std::duration  compile time requirements
    namespace CompileTimeRequirements
    {
        TYPED_TEST(DurationTypedTest, TraitRequirementsSuccess)
        {
            static_assert(
                AZStd::is_signed<typename TypeParam::rep>::value,
                "built in helper types for AZStd::chrono::duration requires representation type to be signed");
            static_assert(
                AZStd::is_integral<typename TypeParam::rep>::value,
                "built in helper types for AZStd::chrono::duration requires representation type to an integral type");
            static_assert(
                AZStd::numeric_limits<typename TypeParam::rep>::digits >= TypeParam::required_bits,
                "representation type does not have the minimum number of required bits");
            static_assert(
                AZStd::is_same_v<typename TypeParam::period, typename TypeParam::expected_period>,
                "duration period type does not match expected period type");
        }
    }
    namespace Comparisons
    {
        TEST_F(DurationTest, Comparisons_SameType)
        {
            constexpr AZStd::chrono::milliseconds threeMillis(3);
            constexpr AZStd::chrono::milliseconds oneMillis(1);
            constexpr AZStd::chrono::milliseconds oneMillisAgain(1);

            static_assert(oneMillis == oneMillisAgain);
            static_assert(threeMillis > oneMillis);
            static_assert(oneMillis < threeMillis);
            static_assert(threeMillis >= oneMillis);
            static_assert(oneMillis <= threeMillis);
            static_assert(threeMillis >= threeMillis);
            static_assert(threeMillis <= threeMillis);
        }
        TEST_F(DurationTest, Comparisons_DifferentType)
        {
            constexpr AZStd::chrono::milliseconds threeMillis(3);
            constexpr AZStd::chrono::milliseconds oneMillis(1);
            // different types:
            constexpr AZStd::chrono::microseconds threeMillisButInMicroseconds(3000);
            constexpr AZStd::chrono::microseconds oneMilliButInMicroseconds(1000);

            static_assert(threeMillisButInMicroseconds > oneMillis);
            static_assert(oneMilliButInMicroseconds < threeMillis);
            static_assert(threeMillis == threeMillisButInMicroseconds);
            static_assert(oneMilliButInMicroseconds == oneMillis);
            static_assert(threeMillisButInMicroseconds >= threeMillis);
            static_assert(threeMillisButInMicroseconds <= threeMillis);
            static_assert(threeMillisButInMicroseconds >= oneMillis);
            static_assert(oneMilliButInMicroseconds <= threeMillis);
        }
    }

    // Test for std::duration arithmatic operations
    namespace ArithmaticOperators
    {
        TEST_F(DurationTest, MillisecondsSubtractionResultsNegativeSuccess)
        {
            constexpr AZStd::chrono::milliseconds milliSeconds(3);
            constexpr AZStd::chrono::milliseconds inverseMilliSeconds = -milliSeconds;
            static_assert(milliSeconds.count() == -inverseMilliSeconds.count(), "inverseMilliseconds should be inverse of milliseconds");
            static_assert(inverseMilliSeconds.count() == -3, "inverseMilliseconds value should be negative");
            constexpr auto subtractResultMilliSeconds = inverseMilliSeconds - milliSeconds;
            static_assert(subtractResultMilliSeconds.count() == -6, "subtract result is incorrect");
            AZStd::chrono::milliseconds compoundSubtractMilliSeconds(5);
            compoundSubtractMilliSeconds -= AZStd::chrono::milliseconds(4);
            EXPECT_EQ(compoundSubtractMilliSeconds.count(), 1);
        }

        TEST_F(DurationTest, MillisecondsAdditionWithNegativeSuccess)
        {
            constexpr AZStd::chrono::milliseconds milliSeconds(3);
            constexpr AZStd::chrono::milliseconds negativeMilliSeconds(-4);
            constexpr auto addResultMilliSeconds = negativeMilliSeconds + milliSeconds;
            static_assert(addResultMilliSeconds.count() == -1, "add result is incorrect");
            AZStd::chrono::milliseconds compoundAddMilliSeconds(5);
            compoundAddMilliSeconds += AZStd::chrono::milliseconds(2);
            EXPECT_EQ(compoundAddMilliSeconds.count(), 7);
        }

        TEST_F(DurationTest, NanosecondsMultiplicationWithNegativeSuccess)
        {
            constexpr AZStd::chrono::nanoseconds negativeNanoSeconds(-16);
            constexpr AZStd::chrono::nanoseconds multiplyResultSeconds = negativeNanoSeconds * 3;
            static_assert(multiplyResultSeconds.count() == -48, "multiply result is incorrect");
            AZStd::chrono::nanoseconds compoundMultiplyNanoSeconds(9);
            compoundMultiplyNanoSeconds *= -2;
            EXPECT_EQ(compoundMultiplyNanoSeconds.count(), -18);
        }

        TEST_F(DurationTest, SecondsDivideWithNegativeSuccess)
        {
            constexpr AZStd::chrono::seconds testSeconds(17);
            constexpr AZStd::chrono::seconds negativeTestSeconds(-3);
            constexpr auto divideResultSeconds = testSeconds / negativeTestSeconds;
            static_assert(divideResultSeconds == -5, "divide result is incorrect");
            AZStd::chrono::seconds compoundDivideSeconds(-42);
            compoundDivideSeconds /= -2;
            EXPECT_EQ(compoundDivideSeconds.count(), 21);
        }

        TEST_F(DurationTest, MicrosecondsModOperatorSuccess)
        {
            constexpr AZStd::chrono::microseconds microSeconds(23);
            constexpr AZStd::chrono::microseconds microSecondsDivisor(2);
            constexpr auto modResultMicroSeconds = microSeconds % microSecondsDivisor;
            static_assert(modResultMicroSeconds.count() == 1, "mod result is incorrect");
            AZStd::chrono::microseconds compoundModMicroSeconds(30);
            compoundModMicroSeconds %= 7;
            EXPECT_EQ(compoundModMicroSeconds.count(), 2);
        }
    }

    namespace Date
    {
        // Contains test for the date utility functions added in C++20
        TEST_F(DurationTest, DayDuration_ConvertibleToOtherDurations_Succeeds)
        {
            constexpr AZStd::chrono::days testDays(2);
            static_assert(AZStd::chrono::hours(48) == testDays);
            static_assert(AZStd::chrono::minutes(2880) == testDays);

            constexpr AZStd::chrono::seconds convertedSeconds = testDays;
            static_assert(convertedSeconds == AZStd::chrono::seconds(2880 * 60));
        }

        TEST_F(DurationTest, WeeksDuration_ConvertibleToOtherDurations_Succeeds)
        {
            constexpr AZStd::chrono::weeks testWeeks(2);
            static_assert(AZStd::chrono::days(14) == testWeeks);
            static_assert(AZStd::chrono::hours(336) == testWeeks);

            constexpr AZStd::chrono::minutes convertedMinutes = testWeeks;
            static_assert(convertedMinutes == AZStd::chrono::minutes(336 * 60));
        }

        TEST_F(DurationTest, YearsDuration_ConvertibleToOtherDurations_Succeeds)
        {
            constexpr AZStd::chrono::years testYears(2);
            static_assert(AZStd::chrono::months(24) == testYears);
            static_assert(AZStd::chrono::days(365 * 2) < testYears);
            static_assert(AZStd::chrono::days(366 * 2) > testYears);

            // A year is 365.2425 days in the gregorian calendar
            // It is not convertible a duration of chrono::days
            // without a cast due to precisions loss

            // duration_cast will round toward 0
            constexpr auto convertedDays = AZStd::chrono::duration_cast<AZStd::chrono::days>(testYears);
            static_assert(convertedDays == AZStd::chrono::days(730));

            // There should be at least 52 weeks in every year
            constexpr auto convertedWeeks = AZStd::chrono::duration_cast<AZStd::chrono::weeks>(testYears);
            static_assert(convertedWeeks == AZStd::chrono::weeks(104));
        }

        TEST_F(DurationTest, MonthsDuration_ConvertibleToOtherDurations_Succeeds)
        {
            constexpr AZStd::chrono::months testMonths(24);
            static_assert(AZStd::chrono::years(2) == testMonths);

            // Every month is at least 4 weeks
            static_assert(AZStd::chrono::weeks(8) <= testMonths);
            // Every month is at least 28 days
            static_assert(AZStd::chrono::days(56) <= testMonths);

            // A month is defined as a year in the gregorian calendar divided by 12
            // A year is 365.2425 days so a month is defined as 365.2425 / 12 = 30.436875

            // duration_cast will round toward 0
            // 24 months are equivalent to 30.436875 * 24 days = 730.485 days
            constexpr auto convertedDays = AZStd::chrono::duration_cast<AZStd::chrono::days>(testMonths);
            static_assert(convertedDays >= AZStd::chrono::days(730));
            static_assert(convertedDays < AZStd::chrono::days(731));

            // 24 months are equivalent to (30.436875 / 7) * 24 weeks = 104.355... weeks
            constexpr auto convertedWeeks = AZStd::chrono::duration_cast<AZStd::chrono::weeks>(testMonths);
            static_assert(convertedWeeks >= AZStd::chrono::weeks(104));
            static_assert(convertedWeeks < AZStd::chrono::weeks(105));
        }

        TEST_F(DurationTest, UtcClock_IsConvertibleToSystemClock)
        {
            auto utcTime = AZStd::chrono::utc_clock::now();
            auto sysTime = AZStd::chrono::utc_clock::to_sys(utcTime);

            // sysTime should not be equal to utc time due to the accumulation of leap seconds
            // https://www.ietf.org/timezones/data/leap-seconds.list
            EXPECT_NE(utcTime.time_since_epoch(), sysTime.time_since_epoch());
        }

        TEST_F(DurationTest, UtcClock_IsConvertibleFromSystemClock)
        {
            auto sysTime = AZStd::chrono::system_clock::now();
            auto utcTime = AZStd::chrono::utc_clock::from_sys(sysTime);

            // sysTime should not be equal to utc time due to the accumulation of leap seconds
            // https://www.ietf.org/timezones/data/leap-seconds.list
            EXPECT_NE(sysTime.time_since_epoch(), utcTime.time_since_epoch());
        }

        TEST_F(DurationTest, UtcClock_ToSys_RoundTrips_WithFromSys)
        {
            const auto utcTime = AZStd::chrono::utc_clock::now();
            const auto sysTime = AZStd::chrono::utc_clock::to_sys(utcTime);
            const auto utcTimeRoundTrip = AZStd::chrono::utc_clock::from_sys(sysTime);

            EXPECT_EQ(utcTime, utcTimeRoundTrip);
        }

        // BIG NOTE: chrono::days with an s is a chrono::duration counting days
        // chrono::day without an s is a abstraction of a day
        TEST_F(DurationTest, Day_CanRepresent_DayOfArbitrary_Month)
        {
            AZStd::chrono::day testDay{};
            // A day is only valid between 1 and 31 inclusive
            // default constructs to 0
            EXPECT_FALSE(testDay.ok());

            // pre-increment / pre-decrement
            auto updatedDay = ++testDay;
            EXPECT_EQ(testDay, updatedDay);

            updatedDay = --testDay;
            EXPECT_EQ(testDay, updatedDay);

            // post-increment / post-decrement
            auto oldDay = testDay++;
            EXPECT_NE(oldDay, testDay);

            oldDay = testDay--;
            EXPECT_NE(oldDay, testDay);

            // Day 1 - ok
            testDay = AZStd::chrono::day{ 1 };
            EXPECT_TRUE(testDay.ok());

            // Day 31 - ok
            testDay = AZStd::chrono::day{ 31 };
            EXPECT_TRUE(testDay.ok());

            // Day 32 - not ok
            testDay = AZStd::chrono::day{ 32 };
            EXPECT_FALSE(testDay.ok());
        }

        TEST_F(DurationTest, Day_Comparison_Succeeds)
        {
            constexpr AZStd::chrono::day testDay1{ 13 };
            constexpr AZStd::chrono::day testDay2{ 27 };
            static_assert(testDay1 == testDay1);
            static_assert(testDay1 != testDay2);
            static_assert(testDay1 < testDay2);
            static_assert(testDay2 > testDay1);
            static_assert(testDay1 <= testDay2);
            static_assert(testDay2 >= testDay1);
        }

        TEST_F(DurationTest, Day_Arithmetic_Succeeds)
        {
            constexpr AZStd::chrono::day testDay1{ 20 };
            constexpr AZStd::chrono::day testDay2{ 25 };
            constexpr AZStd::chrono::days days{ 15 };
            static_assert(testDay1 + days == AZStd::chrono::day{ 35 });
            static_assert(testDay2 - days == AZStd::chrono::day{ 10 });
            static_assert(testDay2 - testDay1 == AZStd::chrono::days{ 5 });
            static_assert(testDay1 - testDay2 == AZStd::chrono::days{ -5 });
        }

        // BIG NOTE: chrono::months with an s is a chrono::duration counting months
        // chrono::month without an s is a abstraction of a month
        TEST_F(DurationTest, Month_CanRepresent_MonthOfYear)
        {
            AZStd::chrono::month testMonth{};
            // A month is only valid between 1 and 12 inclusive
            // default constructs to 0
            EXPECT_FALSE(testMonth.ok());

            // pre-increment / pre-decrement
            auto updatedMonth = ++testMonth;
            EXPECT_EQ(testMonth, updatedMonth);

            updatedMonth = --testMonth;
            EXPECT_EQ(testMonth, updatedMonth);

            // post-increment / post-decrement
            auto oldMonth = testMonth++;
            EXPECT_NE(oldMonth, testMonth);

            oldMonth = testMonth--;
            EXPECT_NE(oldMonth, testMonth);

            // Month 1 - ok
            testMonth = AZStd::chrono::month{ 1 };
            EXPECT_TRUE(testMonth.ok());

            // Month 12 - ok
            testMonth = AZStd::chrono::month{ 12 };
            EXPECT_TRUE(testMonth.ok());

            // Month 13 - not ok
            testMonth = AZStd::chrono::month{ 13 };
            EXPECT_FALSE(testMonth.ok());
        }

        TEST_F(DurationTest, Month_Comparison_Succeeds)
        {
            constexpr AZStd::chrono::day testMonth1{ 13 };
            constexpr AZStd::chrono::day testMonth2{ 27 };
            static_assert(testMonth1 == testMonth1);
            static_assert(testMonth1 != testMonth2);
            static_assert(testMonth1 < testMonth2);
            static_assert(testMonth2 > testMonth1);
            static_assert(testMonth1 <= testMonth2);
            static_assert(testMonth2 >= testMonth1);
        }

        TEST_F(DurationTest, Month_Arithmetic_Succeeds)
        {
            constexpr AZStd::chrono::month testMonth1{ 3 };
            constexpr AZStd::chrono::month testMonth2{ 10 };
            constexpr AZStd::chrono::months months{ 6 };
            static_assert(testMonth1 + months == AZStd::chrono::month{ 9 });
            static_assert(testMonth2 - months == AZStd::chrono::month{ 4 });
            static_assert(testMonth2 - testMonth1 == AZStd::chrono::months{ 7 });
            static_assert(testMonth1 - testMonth2 == AZStd::chrono::months{ 5 });
        }

        TEST_F(DurationTest, Month_CanArithmetic_IsCircular)
        {
            // August + 5 months should be January
            constexpr auto monthOverflow = AZStd::chrono::August + AZStd::chrono::months{ 5 };
            static_assert(AZStd::chrono::January == monthOverflow);

            // August - 8 months should be December
            constexpr auto monthUnderflow = AZStd::chrono::August - AZStd::chrono::months{ 8 };
            static_assert(AZStd::chrono::December == monthUnderflow);

            // Subtraction between months is bounded in the range of [0, 11]
            // It represents a how many months to add to go form the right month to the left month
            // Because a month does not take year into account, the range is circular
            // September - August should be 1 month as adding 1 month to August results in September
            static_assert(AZStd::chrono::months{ 1 } == AZStd::chrono::September - AZStd::chrono::August);

            // August - September should be 11 months as adding 11 months to September results in Augustst
            static_assert(AZStd::chrono::months{ 11 } == AZStd::chrono::August - AZStd::chrono::September);
        }

        // BIG NOTE: chrono::years with an s is a chrono::duration counting years
        // chrono::year without an s is a abstraction of a year
        TEST_F(DurationTest, Year_Represents_YearInCivilCalendar)
        {
            AZStd::chrono::year testYear{};
            // A year is valid between -32767 and -32767 inclusive
            // default constructs to 0
            EXPECT_TRUE(testYear.ok());

            // pre-increment / pre-decrement
            auto updatedYear = ++testYear;
            EXPECT_EQ(testYear, updatedYear);

            updatedYear = --testYear;
            EXPECT_EQ(testYear, updatedYear);

            // post-increment / post-decrement
            auto oldYear = testYear++;
            EXPECT_NE(oldYear, testYear);

            oldYear = testYear--;
            EXPECT_NE(oldYear, testYear);

            // Year 2000 - ok
            testYear = AZStd::chrono::year{ 2000 };
            EXPECT_TRUE(testYear.ok());

            // Year -32768 - not ok
            testYear = AZStd::chrono::year{ -32768 };
            EXPECT_FALSE(testYear.ok());

            // Year 32768 - not ok
            testYear = AZStd::chrono::year{ 32768 };
            EXPECT_FALSE(testYear.ok());
        }

        TEST_F(DurationTest, Year_Comparison_Succeeds)
        {
            constexpr AZStd::chrono::year testYear1{ 2000 };
            constexpr AZStd::chrono::year testYear2{ 2025 };
            static_assert(testYear1 == testYear1);
            static_assert(testYear1 != testYear2);
            static_assert(testYear1 < testYear2);
            static_assert(testYear2 > testYear1);
            static_assert(testYear1 <= testYear2);
            static_assert(testYear2 >= testYear1);
        }

        TEST_F(DurationTest, Year_Arithmetic_Succeeds)
        {
            constexpr AZStd::chrono::year testYear1{ 2000 };
            constexpr AZStd::chrono::year testYear2{ 2025 };
            constexpr AZStd::chrono::years years{ 15 };
            static_assert(testYear1 + years == AZStd::chrono::year{ 2015 });
            static_assert(testYear2 - years == AZStd::chrono::year{ 2010 });
            static_assert(testYear2 - testYear1 == AZStd::chrono::years{ 25 });
            static_assert(testYear1 - testYear2 == AZStd::chrono::years{ -25 });
        }

        TEST_F(DurationTest, Weekday_Represents_DayOfWeek_BetweenSundayAndSaturday)
        {
            AZStd::chrono::weekday testWeekday{};
            // default constructs to 0
            // which corresponds to Sunbday
            EXPECT_TRUE(testWeekday.ok());

            // pre-increment / pre-decrement
            auto updatedWeekday = ++testWeekday;
            EXPECT_EQ(testWeekday, updatedWeekday);

            updatedWeekday = --testWeekday;
            EXPECT_EQ(testWeekday, updatedWeekday);

            // post-increment / post-decrement
            auto oldWeekday = testWeekday++;
            EXPECT_NE(oldWeekday, testWeekday);

            oldWeekday = testWeekday--;
            EXPECT_NE(oldWeekday, testWeekday);

            // Weekday 4 - ok
            testWeekday = AZStd::chrono::weekday{ 4 };
            EXPECT_TRUE(testWeekday.ok());

            // Weekday 7 - ok (converts to Sunday(0)
            testWeekday = AZStd::chrono::weekday{ 7 };
            EXPECT_TRUE(testWeekday.ok());

            // Weekday 8 - not ok
            testWeekday = AZStd::chrono::weekday{ 8 };
            EXPECT_FALSE(testWeekday.ok());
        }

        TEST_F(DurationTest, Weekday_Comparison_Succeeds)
        {
            // Weekdays can only compare for equality, not order
            // since their is no universal consensu on the start of the week
            // http://eel.is/c++draft/time#cal.wd.overview-note-1
            constexpr AZStd::chrono::weekday testWeekday1{ 0 };
            constexpr AZStd::chrono::weekday testWeekday2{ 3 };
            constexpr AZStd::chrono::weekday testWeekday3{ 6 };
            static_assert(testWeekday1 != testWeekday2);
            static_assert(testWeekday1 != testWeekday3);
            static_assert(testWeekday2 != testWeekday3);
            static_assert(testWeekday1 == AZStd::chrono::Sunday);
            static_assert(testWeekday2 == AZStd::chrono::Wednesday);
            static_assert(testWeekday3 == AZStd::chrono::Saturday);
        }

        TEST_F(DurationTest, Weekday_Arithmetic_Succeeds)
        {
            constexpr AZStd::chrono::weekday testWeekday1{ AZStd::chrono::Monday };
            constexpr AZStd::chrono::weekday testWeekday2{ AZStd::chrono::Thursday };
            constexpr AZStd::chrono::days days{ 15 };
            // adding two weeks and 1 day should transition Monday -> Tuesday
            static_assert(testWeekday1 + days == AZStd::chrono::Tuesday);
            // subtracting two weeks and 1 day should transition Thursday -> Wednesday
            static_assert(testWeekday2 - days == AZStd::chrono::Wednesday);
            // Weekday arithmetic is circular
            // Subtraction returns a value that represents that can be added to the right operand
            // to get the left operand.
            // i.e Monday(1) - Thursday(4) = 4 days as adding 4 days to Thursday result in Monday
            static_assert(testWeekday2 - testWeekday1 == AZStd::chrono::days{ 3 });
            static_assert(testWeekday1 - testWeekday2 == AZStd::chrono::days{ 4 });
        }

        TEST_F(DurationTest, YearMonthDay_RepresentsDateInCivilCalendar)
        {
            using namespace AZStd::chrono;
            constexpr AZStd::chrono::year_month_day ymd = 2024_y / February / 29_d;
            static_assert(ymd.ok());
            static_assert(ymd.year() == 2024_y);
            static_assert(ymd.month() == February);
            static_assert(ymd.day() == 29_d);
        }

        TEST_F(DurationTest, YearMonthDay_Comparison_Succeeds)
        {
            using namespace AZStd::chrono;
            constexpr AZStd::chrono::year_month_day ymd1 = 2024_y / February / 29_d;
            constexpr AZStd::chrono::year_month_day ymd2 = 2024_y / March / 1_d;
            static_assert(ymd1 == ymd1);
            static_assert(ymd1 != ymd2);
        }

        TEST_F(DurationTest, YearMonthDay_Arithmetic_Succeeds)
        {
            using namespace AZStd::chrono;
            constexpr AZStd::chrono::year_month_day ymd1{ 2024_y / February / 29_d };
            constexpr AZStd::chrono::years years{ 3 };

            // validate adding years to a date
            // February 2027 is not a leap year, so the 29th is not a valid day,
            // making the year-month-day not ok
            static_assert(!(ymd1 + years).ok());
            static_assert((ymd1 + years).year() == 2027_y);
            static_assert((ymd1 + years).month() == February);

            // check subtracting years from a date
            // February 2021 is also not a leap year
            static_assert(!(ymd1 - years).ok());
            static_assert((ymd1 - years).year() == 2021_y);
            static_assert((ymd1 - years).month() == February);

            // test adding months to a date
            constexpr AZStd::chrono::months months{ 2 };
            // April 29th is a valid date
            static_assert((ymd1 + months).ok());
            static_assert((ymd1 + months).year() == 2024_y);
            static_assert((ymd1 + months).month() == April);
            static_assert((ymd1 + months).day() == 29_d);

            // verify subtracting months from a date
            // December 29th is a valid date of the previous year
            static_assert((ymd1 - months).ok());
            static_assert((ymd1 - months).year() == 2023_y);
            static_assert((ymd1 - months).month() == December);
            static_assert((ymd1 - months).day() == 29_d);
        }

        TEST_F(DurationTest, HourMinuteSecond_RepresentsTimeOfDay)
        {
            // Contrived value of time past 12 hours in the day
            constexpr AZStd::chrono::microseconds time14Hours6Minutes23Seconds506293Microseconds{ AZStd::chrono::hours{ 14 } +
                                                                                                  AZStd::chrono::minutes{ 6 } +
                                                                                                  AZStd::chrono::seconds{ 23 } +
                                                                                                  AZStd::chrono::microseconds{ 506293 } };

            constexpr AZStd::chrono::hh_mm_ss timeOfDay(time14Hours6Minutes23Seconds506293Microseconds);
            static_assert(timeOfDay.hours() == AZStd::chrono::hours{ 14 });
            static_assert(timeOfDay.minutes() == AZStd::chrono::minutes{ 6 });
            static_assert(timeOfDay.seconds() == AZStd::chrono::seconds{ 23 });
            static_assert(timeOfDay.subseconds() == AZStd::chrono::microseconds{ 506293 });
        }

        TEST_F(DurationTest, HourMinuteSecond_CanBeNegative)
        {
            using namespace AZStd::chrono_literals;
            constexpr AZStd::chrono::hh_mm_ss timeOfDay(-5s);
            static_assert(timeOfDay.is_negative());
            // hours, minutes, seconds and subseconds are stored as absolute values
            // using the hh_mm_ss::to_duration will return a duration accounting for the sign
            static_assert(timeOfDay.hours() == AZStd::chrono::hours{ 0 });
            static_assert(timeOfDay.minutes() == AZStd::chrono::minutes{ 0 });
            static_assert(timeOfDay.seconds() == AZStd::chrono::seconds{ 5 });
            static_assert(timeOfDay.subseconds() == typename decltype(timeOfDay)::precision{ 0 });
            static_assert(timeOfDay.to_duration() == -5s);
        }
    }

    namespace Time_12_24_Functions
    {
        TEST_F(DurationTest, IsAm_Hours_Succeeds)
        {
            using namespace AZStd::chrono_literals;
            static_assert(AZStd::chrono::is_am(0h));
            static_assert(AZStd::chrono::is_am(11h));
            static_assert(!AZStd::chrono::is_am(12h));
            static_assert(!AZStd::chrono::is_am(23h));
        }
        TEST_F(DurationTest, IsPm_Hours_Succeeds)
        {
            using namespace AZStd::chrono_literals;
            static_assert(!AZStd::chrono::is_pm(0h));
            static_assert(!AZStd::chrono::is_pm(11h));
            static_assert(AZStd::chrono::is_pm(12h));
            static_assert(AZStd::chrono::is_pm(23h));
        }

        TEST_F(DurationTest, Make12_CanConvertHours24)
        {
            using namespace AZStd::chrono_literals;
            static_assert(AZStd::chrono::make12(0h) == 12h);
            static_assert(AZStd::chrono::make12(11h) == 11h);
            static_assert(AZStd::chrono::make12(12h) == 12h);
            static_assert(AZStd::chrono::make12(23h) == 11h);
        }

        TEST_F(DurationTest, Make24_CanConvertHours12_WithPMFalse_ToHoursBelow12)
        {
            using namespace AZStd::chrono_literals;
            static_assert(AZStd::chrono::make24(1h, false) == 1h);
            static_assert(AZStd::chrono::make24(11h, false) == 11h);
            static_assert(AZStd::chrono::make24(12h, false) == 0h);
        }

        TEST_F(DurationTest, Make24_CanConvertHours12_WithPMTrue_ToHoursBelow12)
        {
            using namespace AZStd::chrono_literals;
            static_assert(AZStd::chrono::make24(1h, true) == 13h);
            static_assert(AZStd::chrono::make24(11h, true) == 23h);
            static_assert(AZStd::chrono::make24(12h, true) == 12h);
        }
    }
}
