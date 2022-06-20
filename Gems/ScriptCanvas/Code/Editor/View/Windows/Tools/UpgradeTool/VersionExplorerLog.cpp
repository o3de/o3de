/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/View/Windows/Tools/UpgradeTool/VersionExplorerLog.h>

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {
        void Log::Activate()
        {
            AZ::Debug::TraceMessageBus::Handler::BusConnect();
            LogBus::Handler::BusConnect();
        }

        void Log::Clear()
        {
            m_logs.clear();
        }

        bool Log::CaptureFromTraceBus(const char* window, const char* message)
        {
            if (m_isExclusiveReportingEnabled && window != ScriptCanvas::k_VersionExplorerWindow)
            {
                return true;
            }

            AZStd::string msg = message;
            if (msg.ends_with("\n"))
            {
                msg = msg.substr(0, msg.size() - 1);
            }

            m_logs.push_back(msg);
            return m_isExclusiveReportingEnabled;
        }

        void Log::Deactivate()
        {
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        }

        void Log::Entry(const char* format, ...)
        {
            if (m_isVerbose)
            {
                char sBuffer[2048];
                va_list ArgList;
                va_start(ArgList, format);
                azvsnprintf(sBuffer, sizeof(sBuffer), format, ArgList);
                sBuffer[sizeof(sBuffer) - 1] = '\0';
                va_end(ArgList);
                AZ_TracePrintf(ScriptCanvas::k_VersionExplorerWindow.data(), "%s\n", sBuffer);
            }
        }

        const AZStd::vector<AZStd::string>* Log::GetEntries() const
        {
            return &m_logs;
        }

        void Log::SetVersionExporerExclusivity(bool enabled)
        {
            m_isExclusiveReportingEnabled = enabled;
        }

        void Log::SetVerbose(bool verbosity)
        {
            m_isVerbose = verbosity;
        }

        bool Log::OnPreError(const char* window, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message)
        {
            AZStd::string msg = AZStd::string::format("(Error): %s", message);
            return CaptureFromTraceBus(window, msg.c_str());
        }

        bool Log::OnPreWarning(const char* window, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message)
        {
            AZStd::string msg = AZStd::string::format("(Warning): %s", message);
            return CaptureFromTraceBus(window, msg.c_str());
        }

        bool Log::OnException(const char* message)
        {
            AZStd::string msg = AZStd::string::format("(Exception): %s", message);
            return CaptureFromTraceBus("Script Canvas", msg.c_str());
        }

        bool Log::OnPrintf(const char* window, const char* message)
        {
            return CaptureFromTraceBus(window, message);
        }
    }
}
