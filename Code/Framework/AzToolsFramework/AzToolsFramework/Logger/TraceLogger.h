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

        //! Intalize logging for O3DEToolsApplications
        void WriteStartupLog(const AZStd::string& logFileName);

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
        AZStd::unique_ptr<AzFramework::LogFile> m_logFile;
    };
} // namespace AzToolsFramework
