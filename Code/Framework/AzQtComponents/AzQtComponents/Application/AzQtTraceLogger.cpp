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

#include <AzQtComponents/Application/AzQtTraceLogger.h>

#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Logging/LogFile.h>


namespace AzQtComponents
{
    class AzQtTraceLogger::Impl : public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        void WriteStartupLog(char name[]);

        Impl()
        {
            AZ::Debug::TraceMessageBus::Handler::BusConnect();
        }
        ~Impl()
        {
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        }

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

    AzQtTraceLogger::AzQtTraceLogger()
        : m_impl(new Impl)
    {
    }

    AzQtTraceLogger::~AzQtTraceLogger()
    {
    }

    void AzQtTraceLogger::WriteStartupLog(char name[])
    {
        m_impl->WriteStartupLog(name);
    }

    bool AzQtTraceLogger::Impl::OnOutput(const char* window, const char* message)
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

    void AzQtTraceLogger::Impl::WriteStartupLog(char name[])
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
        StringFunc::Path::Join(logDirectory.c_str(), name, logPath);

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
} // namespace AzQtComponents
