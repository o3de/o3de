/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/string/osstring.h>

namespace AZ
{
    namespace Debug
    {
        /**
        * A handler for TraceMessage bus which collects Error and Assert messages, and stores them in a list
        * It is meant to capture messages before logging, file system, or other typical means of providing information to the user may not be available during startup sequences.
        * The implementation of how the collected messages are displayed must be determined by the deriving class.
        */
        class StartupLogSink :
            public TraceMessageBus::Handler
        {
            /*
            * Stores the assert message in m_errorStringsCollected, but does not consume the message.
            * Other handlers should be able act on the EBus message,
            * since this class is only intended to store the messages to display later
            */
            bool OnPreAssert(const char* fileName, int line, const char* func, const char* message) override;
            bool OnPreError(const char* window, const char* fileName, int line, const char* func, const char* message) override;

            /**
            * Creates a user facing display of the collected messages in some way - such as a message box or writing to a log or console).
            * This function is intended to be called when the class goes out of scope.
            */
            virtual void DisplayCollectedErrorStrings() const = 0;

        protected:
            AZStd::list<AZ::OSString, AZ::OSStdAllocator> m_errorStringsCollected;
        };

        /**
        * A scoped helper class for use with derived classes from StartupLogSink.
        * This class manages the StartupLogSink's connection to the bus, and requests to display the collected error and assert messages when it falls out of scope.
        */
        template <typename StartupLogSinkType>
        class StartupLogSinkReporter
        {
        public:
            StartupLogSinkReporter()
            {
                m_myBusListener.BusConnect();
            }

            ~StartupLogSinkReporter()
            {
                m_myBusListener.BusDisconnect();
                m_myBusListener.DisplayCollectedErrorStrings();
            }

            StartupLogSinkType& GetContainedLogSink() { return m_myBusListener; }

        protected:
            StartupLogSinkType m_myBusListener;
        };
    } // namespace Debug
} // namespace AZ
