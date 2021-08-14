/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Debug/TraceMessageBus.h>
#include <AzFramework/Logging/LogFile.h>
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>
#include <AzFramework/Asset/AssetSystemBus.h>

namespace AzToolsFramework
{
    //! Connects and disconnects TraceMessageBus and allows for logging for O3DE Tools Applications
    class TraceLogger
        : public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        TraceLogger();
        ~TraceLogger();

        //! Open log file and dump log sink into it
        void PrepareLogFile(const AZStd::string& logFileName);

        //! Ignore messages sent to windowd with names matching filter
        void AddWindowFilter(const AZStd::string& filter);

        //! Ignore messages with text matching filter
        void AddMessageFilter(const AZStd::string& filter);

    protected:
        //////////////////////////////////////////////////////////////////////////
        // AZ::Debug::TraceMessageBus::Handler overrides...
        bool OnOutput(const char* window, const char* message) override;
        //////////////////////////////////////////////////////////////////////////

        struct LogMessage
        {
        public:
            AZStd::string window;
            AZStd::string message;
        };
        AZStd::vector<LogMessage> m_startupLogSink;
        AZStd::unordered_set<AZStd::string> m_windowFilters;
        AZStd::unordered_set<AZStd::string> m_messageFilters;
        AZStd::unique_ptr<AzFramework::LogFile> m_logFile;
    };
} // namespace AzToolsFramework
