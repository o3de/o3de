/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        : public ScopedAllocatorSetupFixture
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

