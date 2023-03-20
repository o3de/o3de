/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ErrorMessageFinder.h"
#include <gtest/gtest.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace UnitTest
{
    ErrorMessageFinder::ErrorMessageFinder()
    {
        m_reportFailure = &ErrorMessageFinder::ReportFailure;
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
        UnitTest::TestRunner::Instance().StartAssertTests();
    }
    
    ErrorMessageFinder::ErrorMessageFinder(AZStd::string_view message, uint32_t expectedCount)
        : ErrorMessageFinder()
    {
        AddExpectedErrorMessage(message, expectedCount);
    }

    ErrorMessageFinder::~ErrorMessageFinder()
    {
        if (!m_disabled)
        {
            Disable();
        }
    }

    void ErrorMessageFinder::Disable()
    {
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        UnitTest::TestRunner::Instance().StopAssertTests();
        if (!m_checked)
        {
            CheckExpectedErrorsFound();
        }
        m_disabled = true;
    }

    void ErrorMessageFinder::AddExpectedErrorMessage(AZStd::string_view message, uint32_t expectedCount)
    {
        EXPECT_FALSE(m_disabled);

        if (expectedCount > 0)
        {
            ExpectedError expectedError;
            expectedError.m_message = message;
            expectedError.m_expectedCount = expectedCount;
            m_expectedErrors.push_back(expectedError);

            m_checked = false;
        }

    }

    void ErrorMessageFinder::AddIgnoredErrorMessage(AZStd::string_view message, bool matchSubstring)
    {
        EXPECT_FALSE(m_disabled);

        OptionalError optionalError;
        optionalError.m_message = message;
        optionalError.m_allowSubstring = matchSubstring;
        m_optionalErrors.push_back(optionalError);
    }

    void ErrorMessageFinder::CheckExpectedErrorsFound()
    {
        EXPECT_FALSE(m_disabled);

        for (auto expectedError : m_expectedErrors)
        {
            if (expectedError.m_expectedCount != expectedError.m_gotCount)
            {
                AZStd::string message = AZStd::string::format("Expected error %d time(s) but got %d time(s): '%s'", expectedError.m_expectedCount, expectedError.m_gotCount, expectedError.m_message.c_str());
                m_reportFailure(message);
            }
        }

        m_checked = true;
    }

    void ErrorMessageFinder::ReportFailure(const AZStd::string& failureMessage)
    {
        FAIL() << failureMessage.c_str();
    }

    void ErrorMessageFinder::Reset()
    {
        EXPECT_FALSE(m_disabled);

        m_expectedErrors.clear();
        m_optionalErrors.clear();
        m_checked = false;
    }

    void ErrorMessageFinder::ResetCounts()
    {
        EXPECT_FALSE(m_disabled);

        AZStd::for_each(m_expectedErrors.begin(), m_expectedErrors.end(), [](ExpectedError& expectedError) { expectedError.m_gotCount = 0; });

        m_checked = false;
    }

    bool ErrorMessageFinder::OnPreAssert(const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message)
    {
        return OnTrace(message);
    }

    bool ErrorMessageFinder::OnPreError(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message)
    {
        return OnTrace(message);
    }

    bool ErrorMessageFinder::OnPreWarning(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message)
    {
        return OnTrace(message);
    }

    bool ErrorMessageFinder::OnTrace(AZStd::string_view message)
    {
        bool found = false;

        for (auto expectedErrorIter = m_expectedErrors.begin(); expectedErrorIter != m_expectedErrors.end(); ++expectedErrorIter)
        {
            if (AzFramework::StringFunc::Find(message, expectedErrorIter->m_message) != AZStd::string::npos)
            {
                found = true;
                ++expectedErrorIter->m_gotCount;
            }
        }

        if (!found)
        {
            for (auto& optionalError : m_optionalErrors)
            {
                if (optionalError.m_message == message)
                {
                    found = true;
                    break;
                }
                else if (optionalError.m_allowSubstring && message.find(optionalError.m_message) != AZStd::string::npos)
                {
                    found = true;
                    break;
                }
            }
        }

        if (!found)
        {
            // We will return false below, but there are been times in the past when that did not trigger gtest failures
            // as expected, so we report the failure directly here. It might get reported again by another trace hander.
            AZStd::string fullMessage = AZStd::string::format("Detected unexpected error message: '%.*s'", AZ_STRING_ARG(message));
            m_reportFailure(fullMessage);
        }

        m_checked = false;

        return found;
    }
}
