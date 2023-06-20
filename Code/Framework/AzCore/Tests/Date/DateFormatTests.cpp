/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Date/DateFormat.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    //////////////////////////////////////////////////////////////////////////
    // Fixtures

    // Fixture for non-typed tests
    class DateFormatTest : public LeakDetectionFixture
    {
    };

    namespace Iso8601Timestamp
    {
        TEST_F(DateFormatTest, DateFormat_ExtendedFormatDate_Succeeds)
        {
            using AZStd::chrono_literals::operator""_d;
            using AZStd::chrono_literals::operator""_y;
            using AZStd::chrono::March;

            constexpr AZStd::chrono::year_month_day testDate{ 2025_y / March / 18_d };
            constexpr AZStd::chrono::hh_mm_ss testTime{ AZStd::chrono::hours(11) + AZStd::chrono::minutes(5) + AZStd::chrono::seconds(37) +
                                                        AZStd::chrono::microseconds(71041) };

            const auto utcTestDateTime{ AZStd::chrono::utc_clock::from_sys(AZStd::chrono::sys_days(testDate) + testTime.to_duration()) };

            AZ::Date::Iso8601TimestampString iso8601Timestamp;
            EXPECT_TRUE(AZ::Date::GetIso8601ExtendedFormatDate(iso8601Timestamp, utcTestDateTime));
            EXPECT_STREQ("2025-03-18", iso8601Timestamp.c_str());
        }

        TEST_F(DateFormatTest, DateFormat_BasicFormatDate_Succeeds)
        {
            using AZStd::chrono_literals::operator""_d;
            using AZStd::chrono_literals::operator""_y;
            using AZStd::chrono::March;

            constexpr AZStd::chrono::year_month_day testDate{ 2025_y / March / 18_d };
            constexpr AZStd::chrono::hh_mm_ss testTime{ AZStd::chrono::hours(11) + AZStd::chrono::minutes(5) + AZStd::chrono::seconds(37) +
                                                        AZStd::chrono::microseconds(71041) };

            const auto utcTestDateTime{ AZStd::chrono::utc_clock::from_sys(AZStd::chrono::sys_days(testDate) + testTime.to_duration()) };

            AZ::Date::Iso8601TimestampString iso8601Timestamp;
            EXPECT_TRUE(AZ::Date::GetIso8601BasicFormatDate(iso8601Timestamp, utcTestDateTime));
            EXPECT_STREQ("20250318", iso8601Timestamp.c_str());
        }

        TEST_F(DateFormatTest, DateFormat_ExtendedFormatTime_Succeeds)
        {
            using AZStd::chrono_literals::operator""_d;
            using AZStd::chrono_literals::operator""_y;
            using AZStd::chrono::March;

            constexpr AZStd::chrono::year_month_day testDate{ 2025_y / March / 18_d };
            constexpr AZStd::chrono::hh_mm_ss testTime{ AZStd::chrono::hours(11) + AZStd::chrono::minutes(5) + AZStd::chrono::seconds(37) +
                                                        AZStd::chrono::microseconds(71041) };

            const auto utcTestDateTime{ AZStd::chrono::utc_clock::from_sys(AZStd::chrono::sys_days(testDate) + testTime.to_duration()) };

            AZ::Date::Iso8601TimestampString iso8601Timestamp;
            EXPECT_TRUE(AZ::Date::GetIso8601ExtendedFormatTime(iso8601Timestamp, utcTestDateTime));
            EXPECT_STREQ("T11:05:37Z", iso8601Timestamp.c_str());
        }

        TEST_F(DateFormatTest, DateFormat_BasicFormatTime_Succeeds)
        {
            using AZStd::chrono_literals::operator""_d;
            using AZStd::chrono_literals::operator""_y;
            using AZStd::chrono::March;

            constexpr AZStd::chrono::year_month_day testDate{ 2025_y / March / 18_d };
            constexpr AZStd::chrono::hh_mm_ss testTime{ AZStd::chrono::hours(11) + AZStd::chrono::minutes(5) + AZStd::chrono::seconds(37) +
                                                        AZStd::chrono::microseconds(71041) };

            const auto utcTestDateTime{ AZStd::chrono::utc_clock::from_sys(AZStd::chrono::sys_days(testDate) + testTime.to_duration()) };

            AZ::Date::Iso8601TimestampString iso8601Timestamp;
            EXPECT_TRUE(AZ::Date::GetIso8601BasicFormatTime(iso8601Timestamp, utcTestDateTime));
            EXPECT_STREQ("T110537Z", iso8601Timestamp.c_str());
        }

        TEST_F(DateFormatTest, DateFormat_ExtendedFormatTimeWithMilliseconds_Succeeds)
        {
            using AZStd::chrono_literals::operator""_d;
            using AZStd::chrono_literals::operator""_y;
            using AZStd::chrono::March;

            constexpr AZStd::chrono::year_month_day testDate{ 2025_y / March / 18_d };
            constexpr AZStd::chrono::hh_mm_ss testTime{ AZStd::chrono::hours(11) + AZStd::chrono::minutes(5) + AZStd::chrono::seconds(37) +
                                                        AZStd::chrono::microseconds(71041) };

            const auto utcTestDateTime{ AZStd::chrono::utc_clock::from_sys(AZStd::chrono::sys_days(testDate) + testTime.to_duration()) };

            AZ::Date::Iso8601TimestampString iso8601Timestamp;
            EXPECT_TRUE(AZ::Date::GetIso8601ExtendedFormatTimeWithMilliseconds(iso8601Timestamp, utcTestDateTime));
            EXPECT_STREQ("T11:05:37.071Z", iso8601Timestamp.c_str());
        }

        TEST_F(DateFormatTest, DateFormat_BasicFormatTimeWithMiliseconds_Succeeds)
        {
            using AZStd::chrono_literals::operator""_d;
            using AZStd::chrono_literals::operator""_y;
            using AZStd::chrono::March;

            constexpr AZStd::chrono::year_month_day testDate{ 2025_y / March / 18_d };
            constexpr AZStd::chrono::hh_mm_ss testTime{ AZStd::chrono::hours(11) + AZStd::chrono::minutes(5) + AZStd::chrono::seconds(37) +
                                                        AZStd::chrono::microseconds(71041) };

            const auto utcTestDateTime{ AZStd::chrono::utc_clock::from_sys(AZStd::chrono::sys_days(testDate) + testTime.to_duration()) };

            AZ::Date::Iso8601TimestampString iso8601Timestamp;
            EXPECT_TRUE(AZ::Date::GetIso8601BasicFormatTimeWithMilliseconds(iso8601Timestamp, utcTestDateTime));
            EXPECT_STREQ("T110537.071Z", iso8601Timestamp.c_str());
        }

        TEST_F(DateFormatTest, DateFormat_ExtendedFormatTimeWithMicroseconds_Succeeds)
        {
            using AZStd::chrono_literals::operator""_d;
            using AZStd::chrono_literals::operator""_y;
            using AZStd::chrono::March;

            constexpr AZStd::chrono::year_month_day testDate{ 2025_y / March / 18_d };
            constexpr AZStd::chrono::hh_mm_ss testTime{ AZStd::chrono::hours(11) + AZStd::chrono::minutes(5) + AZStd::chrono::seconds(37) +
                                                        AZStd::chrono::microseconds(71041) };

            const auto utcTestDateTime{ AZStd::chrono::utc_clock::from_sys(AZStd::chrono::sys_days(testDate) + testTime.to_duration()) };

            AZ::Date::Iso8601TimestampString iso8601Timestamp;
            EXPECT_TRUE(AZ::Date::GetIso8601ExtendedFormatTimeWithMicroseconds(iso8601Timestamp, utcTestDateTime));
            EXPECT_STREQ("T11:05:37.071041Z", iso8601Timestamp.c_str());
        }

        TEST_F(DateFormatTest, DateFormat_BasicFormatTimeWithMicroseconds_Succeeds)
        {
            using AZStd::chrono_literals::operator""_d;
            using AZStd::chrono_literals::operator""_y;
            using AZStd::chrono::March;

            constexpr AZStd::chrono::year_month_day testDate{ 2025_y / March / 18_d };
            constexpr AZStd::chrono::hh_mm_ss testTime{ AZStd::chrono::hours(11) + AZStd::chrono::minutes(5) + AZStd::chrono::seconds(37) +
                                                        AZStd::chrono::microseconds(71041) };

            const auto utcTestDateTime{ AZStd::chrono::utc_clock::from_sys(AZStd::chrono::sys_days(testDate) + testTime.to_duration()) };

            AZ::Date::Iso8601TimestampString iso8601Timestamp;
            EXPECT_TRUE(AZ::Date::GetIso8601BasicFormatTimeWithMicroseconds(iso8601Timestamp, utcTestDateTime));
            EXPECT_STREQ("T110537.071041Z", iso8601Timestamp.c_str());
        }

        TEST_F(DateFormatTest, DateFormat_OutputsExtendedIso8601TimestampUTC_ToSeconds)
        {
            using AZStd::chrono_literals::operator""_d;
            using AZStd::chrono_literals::operator""_y;
            using AZStd::chrono::March;

            constexpr AZStd::chrono::year_month_day testDate{ 2025_y / March / 18_d };
            constexpr AZStd::chrono::hh_mm_ss testTime{ AZStd::chrono::hours(11) + AZStd::chrono::minutes(5) + AZStd::chrono::seconds(37) +
                                                        AZStd::chrono::microseconds(71041) };

            const auto utcTestDateTime{ AZStd::chrono::utc_clock::from_sys(AZStd::chrono::sys_days(testDate) + testTime.to_duration()) };

            AZ::Date::Iso8601TimestampString iso8601Timestamp;
            EXPECT_TRUE(AZ::Date::GetIso8601ExtendedFormat(iso8601Timestamp, utcTestDateTime));
            EXPECT_STREQ("2025-03-18T11:05:37Z", iso8601Timestamp.c_str());
        }

        TEST_F(DateFormatTest, DateFormat_OutputsExtendedIso8601TimestampUTC_ToMilliseconds)
        {
            using AZStd::chrono_literals::operator""_d;
            using AZStd::chrono_literals::operator""_y;
            using AZStd::chrono::March;

            constexpr AZStd::chrono::year_month_day testDate{ 2025_y / March / 18_d };
            constexpr AZStd::chrono::hh_mm_ss testTime{ AZStd::chrono::hours(11) + AZStd::chrono::minutes(5) + AZStd::chrono::seconds(37) +
                                                        AZStd::chrono::microseconds(71041) };

            const auto utcTestDateTime{ AZStd::chrono::utc_clock::from_sys(AZStd::chrono::sys_days(testDate) + testTime.to_duration()) };

            AZ::Date::Iso8601TimestampString iso8601Timestamp;
            EXPECT_TRUE(AZ::Date::GetIso8601ExtendedFormatWithMilliseconds(iso8601Timestamp, utcTestDateTime));
            EXPECT_STREQ("2025-03-18T11:05:37.071Z", iso8601Timestamp.c_str());
        }

        TEST_F(DateFormatTest, DateFormat_OutputsExtendedIso8601TimestampUTC_ToMicroseconds)
        {
            using AZStd::chrono_literals::operator""_d;
            using AZStd::chrono_literals::operator""_y;
            using AZStd::chrono::March;

            constexpr AZStd::chrono::year_month_day testDate{ 2025_y / March / 18_d };
            constexpr AZStd::chrono::hh_mm_ss testTime{ AZStd::chrono::hours(11) + AZStd::chrono::minutes(5) + AZStd::chrono::seconds(37) +
                                                        AZStd::chrono::microseconds(71041) };

            const auto utcTestDateTime{ AZStd::chrono::utc_clock::from_sys(AZStd::chrono::sys_days(testDate) + testTime.to_duration()) };

            AZ::Date::Iso8601TimestampString iso8601Timestamp;
            EXPECT_TRUE(AZ::Date::GetIso8601ExtendedFormatWithMicroseconds(iso8601Timestamp, utcTestDateTime));
            EXPECT_STREQ("2025-03-18T11:05:37.071041Z", iso8601Timestamp.c_str());
        }

        TEST_F(DateFormatTest, DateFormat_OutputsBasicIso8601TimestampUTC_ToSeconds)
        {
            using AZStd::chrono_literals::operator""_d;
            using AZStd::chrono_literals::operator""_y;
            using AZStd::chrono::March;

            constexpr AZStd::chrono::year_month_day testDate{ 2025_y / March / 18_d };
            constexpr AZStd::chrono::hh_mm_ss testTime{ AZStd::chrono::hours(11) + AZStd::chrono::minutes(5) + AZStd::chrono::seconds(37) +
                                                        AZStd::chrono::microseconds(71041) };

            const auto utcTestDateTime{ AZStd::chrono::utc_clock::from_sys(AZStd::chrono::sys_days(testDate) + testTime.to_duration()) };

            AZ::Date::Iso8601TimestampString iso8601Timestamp;
            EXPECT_TRUE(AZ::Date::GetIso8601BasicFormat(iso8601Timestamp, utcTestDateTime));
            EXPECT_STREQ("20250318T110537Z", iso8601Timestamp.c_str());
        }

        TEST_F(DateFormatTest, DateFormat_OutputsBasicIso8601TimestampUTC_ToMilliseconds)
        {
            using AZStd::chrono_literals::operator""_d;
            using AZStd::chrono_literals::operator""_y;
            using AZStd::chrono::March;

            constexpr AZStd::chrono::year_month_day testDate{ 2025_y / March / 18_d };
            constexpr AZStd::chrono::hh_mm_ss testTime{ AZStd::chrono::hours(11) + AZStd::chrono::minutes(5) + AZStd::chrono::seconds(37) +
                                                        AZStd::chrono::microseconds(71041) };

            const auto utcTestDateTime{ AZStd::chrono::utc_clock::from_sys(AZStd::chrono::sys_days(testDate) + testTime.to_duration()) };

            AZ::Date::Iso8601TimestampString iso8601Timestamp;
            EXPECT_TRUE(AZ::Date::GetIso8601BasicFormatWithMilliseconds(iso8601Timestamp, utcTestDateTime));
            EXPECT_STREQ("20250318T110537.071Z", iso8601Timestamp.c_str());
        }

        TEST_F(DateFormatTest, DateFormat_OutputsBasicIso8601TimestampUTC_ToMicroseconds)
        {
            using AZStd::chrono_literals::operator""_d;
            using AZStd::chrono_literals::operator""_y;
            using AZStd::chrono::March;

            constexpr AZStd::chrono::year_month_day testDate{ 2025_y / March / 18_d };
            constexpr AZStd::chrono::hh_mm_ss testTime{ AZStd::chrono::hours(11) + AZStd::chrono::minutes(5) + AZStd::chrono::seconds(37) +
                                                        AZStd::chrono::microseconds(71041) };

            const auto utcTestDateTime{ AZStd::chrono::utc_clock::from_sys(AZStd::chrono::sys_days(testDate) + testTime.to_duration()) };

            AZ::Date::Iso8601TimestampString iso8601Timestamp;
            EXPECT_TRUE(AZ::Date::GetIso8601BasicFormatWithMicroseconds(iso8601Timestamp, utcTestDateTime));
            EXPECT_STREQ("20250318T110537.071041Z", iso8601Timestamp.c_str());
        }

        TEST_F(DateFormatTest, DateFormat_FilenameCompatibleTimestamp_WithSeconds_Succeeds)
        {
            using AZStd::chrono_literals::operator""_d;
            using AZStd::chrono_literals::operator""_y;
            using AZStd::chrono::March;

            constexpr AZStd::chrono::year_month_day testDate{ 2025_y / March / 18_d };
            constexpr AZStd::chrono::hh_mm_ss testTime{ AZStd::chrono::hours(11) + AZStd::chrono::minutes(5) + AZStd::chrono::seconds(37) +
                                                        AZStd::chrono::microseconds(71041) };

            const auto utcTestDateTime{ AZStd::chrono::utc_clock::from_sys(AZStd::chrono::sys_days(testDate) + testTime.to_duration()) };

            AZ::Date::Iso8601TimestampString iso8601Timestamp;
            EXPECT_TRUE(AZ::Date::GetFilenameCompatibleFormat(iso8601Timestamp, utcTestDateTime));
            EXPECT_STREQ("2025-03-18T110537Z", iso8601Timestamp.c_str());
        }

        TEST_F(DateFormatTest, DateFormat_FilenameCompatibleTimestamp_WithMilliseconds_Succeeds)
        {
            using AZStd::chrono_literals::operator""_d;
            using AZStd::chrono_literals::operator""_y;
            using AZStd::chrono::March;

            constexpr AZStd::chrono::year_month_day testDate{ 2025_y / March / 18_d };
            constexpr AZStd::chrono::hh_mm_ss testTime{ AZStd::chrono::hours(11) + AZStd::chrono::minutes(5) + AZStd::chrono::seconds(37) +
                                                        AZStd::chrono::microseconds(71041) };

            const auto utcTestDateTime{ AZStd::chrono::utc_clock::from_sys(AZStd::chrono::sys_days(testDate) + testTime.to_duration()) };

            AZ::Date::Iso8601TimestampString iso8601Timestamp;
            EXPECT_TRUE(AZ::Date::GetFilenameCompatibleFormatWithMilliseconds(iso8601Timestamp, utcTestDateTime));
            EXPECT_STREQ("2025-03-18T110537.071Z", iso8601Timestamp.c_str());
        }

        TEST_F(DateFormatTest, DateFormat_FilenameCompatibleTimestamp_WithMicroseconds_Succeeds)
        {
            using AZStd::chrono_literals::operator""_d;
            using AZStd::chrono_literals::operator""_y;
            using AZStd::chrono::March;

            constexpr AZStd::chrono::year_month_day testDate{ 2025_y / March / 18_d };
            constexpr AZStd::chrono::hh_mm_ss testTime{ AZStd::chrono::hours(11) + AZStd::chrono::minutes(5) + AZStd::chrono::seconds(37) +
                                                        AZStd::chrono::microseconds(71041) };

            const auto utcTestDateTime{ AZStd::chrono::utc_clock::from_sys(AZStd::chrono::sys_days(testDate) + testTime.to_duration()) };

            AZ::Date::Iso8601TimestampString iso8601Timestamp;
            EXPECT_TRUE(AZ::Date::GetFilenameCompatibleFormatWithMicroseconds(iso8601Timestamp, utcTestDateTime));
            EXPECT_STREQ("2025-03-18T110537.071041Z", iso8601Timestamp.c_str());
        }

        TEST_F(DateFormatTest, DateFormat_NowFunctions_CompileAndLink)
        {
            AZ::Date::Iso8601TimestampString iso8601Timestamp;
            EXPECT_TRUE(AZ::Date::GetIso8601ExtendedFormatNow(iso8601Timestamp));
            EXPECT_TRUE(AZ::Date::GetIso8601ExtendedFormatNowWithMilliseconds(iso8601Timestamp));
            EXPECT_TRUE(AZ::Date::GetIso8601ExtendedFormatNowWithMicroseconds(iso8601Timestamp));
            EXPECT_TRUE(AZ::Date::GetIso8601BasicFormatNow(iso8601Timestamp));
            EXPECT_TRUE(AZ::Date::GetIso8601BasicFormatNowWithMilliseconds(iso8601Timestamp));
            EXPECT_TRUE(AZ::Date::GetIso8601BasicFormatNowWithMicroseconds(iso8601Timestamp));
            EXPECT_TRUE(AZ::Date::GetFilenameCompatibleFormatNow(iso8601Timestamp));
            EXPECT_TRUE(AZ::Date::GetFilenameCompatibleFormatNowWithMilliseconds(iso8601Timestamp));
            EXPECT_TRUE(AZ::Date::GetFilenameCompatibleFormatNowWithMicroseconds(iso8601Timestamp));
        }
    } // namespace Iso8601Timestamp
} // namespace UnitTest
