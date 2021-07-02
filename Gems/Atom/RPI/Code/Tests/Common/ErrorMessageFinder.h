/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

namespace UnitTest
{
    //! This utility for unit tests listens for expected error messages. Just fill it with a list of messages
    //! and the number of times each message should occur. If a message is expected, it will be counted.
    //! If a message is not expected, it will be passed through the normal error handling process and
    //! your test will fail. When the object is destructed or when you call CheckExpectedErrorsFound(), 
    //! if any expected error messages were not received the expected number of times, it will report the
    //! discrepancy and your test will fail.
    //! Note the expected error messages are sub-strings and not case sensitive, so an exact match isn't required.
    class ErrorMessageFinder : public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        ErrorMessageFinder();
        ~ErrorMessageFinder();

        explicit ErrorMessageFinder(AZStd::string_view message, uint32_t expectedCount = 1);

        void AddExpectedErrorMessage(AZStd::string_view message, uint32_t expectedCount = 1);

        //! Use this function to prevent specific error messages from causing your test to fail
        //! but also don't require the message to occur. This can be a good way to deal with
        //! errors that occur outside the code you are testing, but are potential side-effects.
        //! Use caution when setting matchSubstring=true because you could accidentally ignore
        //! too many messages if the 'message' string is something too short.
        void AddIgnoredErrorMessage(AZStd::string_view message, bool matchSubstring = false);

        //! Reset the ErrorMessageFinder, clearing all expected errors messages, and all collected data.
        void Reset();

        //! Reset the received message counts for your expected error messages. The expected
        //! messages and expected counts will remain the same.
        void ResetCounts();

        //! Check whether all expected messages were received and fail if not. This will be called
        //! automatically in the destructor if you don't call this yourself.
        void CheckExpectedErrorsFound();

        //! Call this function to stop intercepting errors
        void Disable();

    private:
        struct ExpectedError
        {
            AZStd::string m_message;
            uint32_t m_expectedCount = 0;
            uint32_t m_gotCount = 0;
        };

        struct OptionalError
        {
            AZStd::string m_message;
            bool m_allowSubstring = false;
        };

        bool OnPreAssert(const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message) override;
        bool OnPreError(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message) override;
        bool OnPreWarning(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message) override;

        // Using a callback allows us to unit test the ErrorMessageFinder class itself
        friend class ErrorMessageFinderTests;
        using FailureCallback = AZStd::function<void(const AZStd::string&)>;
        static void ReportFailure(const AZStd::string&);
        FailureCallback m_reportFailure;

        /// Common implementation for various bus functions that can report error messages
        bool OnTrace(AZStd::string_view message);

        AZStd::vector<ExpectedError> m_expectedErrors;
        AZStd::vector<OptionalError> m_optionalErrors;
        bool m_checked = false;
        bool m_disabled = false;
    };

} // namespace UnitTest
