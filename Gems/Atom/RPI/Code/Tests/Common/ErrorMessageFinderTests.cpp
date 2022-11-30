/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <Common/ErrorMessageFinder.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    // In order to have confidence in our other unit tests, we need to ensure ErrorMessageFinder correctly detects error messages

    class ErrorMessageFinderTests
        : public LeakDetectionFixture
    {
    protected:
        AZStd::vector<AZStd::string> m_reportedFailures;

        void TearDown() override
        {
            m_reportedFailures.clear();
        }

        void HandleFailure(const AZStd::string& failureMessage)
        {
            m_reportedFailures.push_back(failureMessage);
        }

        void RedirectGtestFailures(ErrorMessageFinder& finder)
        {
            finder.m_reportFailure = std::bind(&ErrorMessageFinderTests::HandleFailure, this, std::placeholders::_1);
        }

        bool FailureWasReported(const AZStd::string& forMessage)
        {
            for (const AZStd::string& failureMessage : m_reportedFailures)
            {
                if (AzFramework::StringFunc::Find(failureMessage, forMessage) != AZStd::string::npos)
                {
                    return true;
                }
            }

            return false;
        }
    };

    TEST_F(ErrorMessageFinderTests, ConsumeExpectedMessage)
    {
        ErrorMessageFinder finder;
        RedirectGtestFailures(finder);

        finder.AddExpectedErrorMessage("this is a test");
        AZ_Assert(false, "...this is a test...");
        finder.CheckExpectedErrorsFound();

        EXPECT_FALSE(FailureWasReported("this is a test"));
    }

    TEST_F(ErrorMessageFinderTests, ReportExpectedMessageNotReceived)
    {
        ErrorMessageFinder finder;
        RedirectGtestFailures(finder);

        finder.AddExpectedErrorMessage("this is a test");
        finder.CheckExpectedErrorsFound();

        EXPECT_TRUE(FailureWasReported("Expected error 1 time(s) but got 0 time(s): 'this is a test'"));
    }

    TEST_F(ErrorMessageFinderTests, ReportExpectedMessageFoundTooManyTimes)
    {
        ErrorMessageFinder finder;
        RedirectGtestFailures(finder);

        finder.AddExpectedErrorMessage("this is a test", 1);
        AZ_Assert(false, "this is a test.");
        AZ_Assert(false, "this is a test.");
        finder.CheckExpectedErrorsFound();

        EXPECT_TRUE(FailureWasReported("Expected error 1 time(s) but got 2 time(s): 'this is a test'"));
    }

    TEST_F(ErrorMessageFinderTests, ReportUnexpectedMessage)
    {
        ErrorMessageFinder finder;
        RedirectGtestFailures(finder);

        finder.AddExpectedErrorMessage("this is a test");
        AZ_Assert(false, "...this is a test...");
        AZ_Assert(false, "This message is not expected.");
        finder.CheckExpectedErrorsFound();

        EXPECT_FALSE(FailureWasReported("this is a test"));
        EXPECT_TRUE(FailureWasReported("This message is not expected"));
    }
}

