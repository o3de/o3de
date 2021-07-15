/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Logger/TraceLogger.h>

#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzFramework/StringFunc/StringFunc.h>


namespace AzToolsFramework
{
    TraceLogger::TraceLogger()
    {
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
    }

    TraceLogger::~TraceLogger()
    {
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
    }

    bool TraceLogger::OnOutput(const char* window, const char* message)
    {
        if (m_logFile)
        {
            m_logFile->AppendLog(AzFramework::LogFile::SEV_NORMAL, window, message);
        }
        else
        {
            m_startupLogSink.push_back({ window, message });
        }
        return false;
    }

    void TraceLogger::WriteStartupLog(const AZStd::string& logFileName)
    {    
        using namespace AzFramework;
        
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO != nullptr, "FileIO should be running at this point");

        // There is no log system online so we have to create your own log file.
        char resolveBuffer[AZ_MAX_PATH_LEN] = { 0 };
        fileIO->ResolvePath("@user@", resolveBuffer, AZ_MAX_PATH_LEN);

        // Note: @log@ hasn't been set at this point
        AZStd::string logDirectory;
        StringFunc::Path::Join(resolveBuffer, "log", logDirectory);
        fileIO->SetAlias("@log@", logDirectory.c_str());

        fileIO->CreatePath("@root@");
        fileIO->CreatePath("@user@");
        fileIO->CreatePath("@log@");

        AZStd::string logPath;
        StringFunc::Path::Join(logDirectory.c_str(), logFileName.c_str(), logPath);

        m_logFile.reset(aznew LogFile(logPath.c_str()));
        if (m_logFile)
        {
            m_logFile->SetMachineReadable(false);
            for (const LogMessage& message : m_startupLogSink)
            {
                m_logFile->AppendLog(LogFile::SEV_NORMAL, message.window.c_str(), message.message.c_str());
            }
            m_startupLogSink = {};
            m_logFile->FlushLog();
        }
    }
} // namespace AzToolsFramework
