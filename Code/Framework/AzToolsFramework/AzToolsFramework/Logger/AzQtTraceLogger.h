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

#pragma once

#include <AzCore/Debug/TraceMessageBus.h>
#include <AzFramework/Logging/LogFile.h>

namespace AzToolsFramework
{
    class AzQtTraceLogger : public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        AzQtTraceLogger();
        ~AzQtTraceLogger();
        void WriteStartupLog(char name[]);

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
