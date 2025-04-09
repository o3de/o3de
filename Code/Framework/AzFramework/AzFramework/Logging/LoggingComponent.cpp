/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LogFile.h"
#include "LoggingComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Memory/Memory.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AzFramework
{
    namespace
    {
        const char* loggingHeaderString =
            "/---------------------------------------------------------------------------\\\n";
        const char* loggingHelloWorld =
            "| ------------ AzFramework File Logging New Run----------------------------- |\n";
        const char* loggingFooterString =
            "\\---------------------------------------------------------------------------/\n\n";

        const AZ::u64 defaultFileSize = 4 * 1024 * 1024;
        const AZStd::string defaultPrefix = "AzFramework";
    };


    LogComponent::LogComponent()
        : m_logFileBaseName(defaultPrefix)
        , m_rolloverLength(defaultFileSize)
        , m_logFile(nullptr)
    {
    }

    LogComponent::~LogComponent()
    {
        DeactivateLogFile();
    }

    void LogComponent::Init()
    {
    }


    void LogComponent::Activate()
    {
        ActivateLogFile();
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
    }

    void LogComponent::Deactivate()
    {
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        DeactivateLogFile();
    }

    void LogComponent::ActivateLogFile()
    {
        // correction to faulty settings by restoring to defaults
        if (m_logFileBaseName.length() <= 0)
        {
            m_logFileBaseName = defaultPrefix;
        }
        if (m_rolloverLength <= 0)
        {
            m_rolloverLength = defaultFileSize;
        }

        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO)
        {
            return;
        }

        const char* logDirectory = fileIO->GetAlias("@log@");

        if (!logDirectory)
        {
            //We will not log anything if the log alias is not empty
            AZ_Warning("Log Component", logDirectory, "Please set the log alias first, before trying to log data");
            return;
        }

        m_logFile = aznew LogFile(logDirectory, m_logFileBaseName.c_str(), m_rolloverLength);
        if (m_logFile)
        {
            m_logFile->SetMachineReadable(m_machineReadable);
            m_logFile->AppendLog(LogFile::SEV_NORMAL, loggingHeaderString);
            m_logFile->AppendLog(LogFile::SEV_NORMAL, loggingHelloWorld);
            m_logFile->AppendLog(LogFile::SEV_NORMAL, loggingFooterString);
        }
    }
    void LogComponent::DeactivateLogFile()
    {
        if (m_logFile)
        {
            delete m_logFile;
            m_logFile = nullptr;
        }
    }

    bool LogComponent::OnPrintf(const char* window, const char* message)
    {
        if (azstrnicmp(window, "debug", 5) == 0)
        {
            //If the window name contains the word debug than write the message only to the log file
            //Our intention is to filter out anything sent to the "debug" window
            //By returning true here, we override the default behavior of the trace system which would otherwise output it to stdout
            OutputMessage(LogFile::SEV_DEBUG, window, message);
            return true;
        }

        OutputMessage(LogFile::SEV_NORMAL, window, message);

        return false;
    }


    bool LogComponent::OnAssert(const char* message)
    {
        OutputMessage(LogFile::SEV_ASSERT, "*ASSERT*", message);

        return false;
    }

    bool LogComponent::OnWarning(const char* window, const char* message)
    {
        OutputMessage(LogFile::SEV_WARNING, window, message);

        return false;
    }

    bool LogComponent::OnError(const char* window, const char* message)
    {
        OutputMessage(LogFile::SEV_ERROR, window, message);

        return false;
    }

    bool LogComponent::OnException(const char* message)
    {
        OutputMessage(LogFile::SEV_EXCEPTION, "*EXCEPTION*", message);

        return false;
    }

    void LogComponent::OutputMessage(LogFile::SeverityLevel severity, const char* window, const char* message)
    {
        if (m_logFile)
        {
            m_logFile->AppendLog(severity, window, message);
        }
    }

    void LogComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<LogComponent, AZ::Component>()
                ->Version(2)
                ->Field("Log FileBase Name", &LogComponent::m_logFileBaseName)
                ->Field("Log File Rollover Length", &LogComponent::m_rolloverLength)
                ->Field("MachineReadableMode", &LogComponent::m_machineReadable);
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<LogComponent>(
                    "File Logging", "Listens to AZ trace messages and forwards them to a log file")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Profiling")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &LogComponent::m_logFileBaseName, "Log file name", "The base name of the file to log to")
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &LogComponent::m_rolloverLength, "Rollover length", "Max size of a log file before saving and opening a new one")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &LogComponent::m_machineReadable, "Machine Readable", "")
                    ;
            }
        }
    }

    void LogComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("LoggingService"));
    }

    void LogComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("LoggingService"));
    }

    void LogComponent::SetMachineReadable(bool newValue)
    {
        m_machineReadable = newValue;
        bool isActive = ((this->GetEntity()) && (this->GetEntity()->GetState() == AZ::Entity::State::Active));
        if (isActive)
        {
            //deactivate and activate the log file again on basename change
            DeactivateLogFile();
            ActivateLogFile();
        }
    }

    void LogComponent::SetLogFileBaseName(const char* baseName)
    {
        if (AzFramework::StringFunc::Equal(m_logFileBaseName.c_str(), baseName, true))
        {
            return;
        }
        m_logFileBaseName = AZStd::string(baseName);
        //Check to see whether the entity is active
        bool isActive = ((this->GetEntity()) && (this->GetEntity()->GetState() == AZ::Entity::State::Active));
        if (isActive)
        {
            //deactivate and activate the log file again on basename change
            DeactivateLogFile();
            ActivateLogFile();
        }
    }

    void LogComponent::SetRollOverLength(AZ::u64 rolloverLength)
    {
        if (m_rolloverLength == rolloverLength)
        {
            return;
        }
        m_rolloverLength = rolloverLength;
        //Check to see whether the entity is active
        bool isActive = ((this->GetEntity()) && (this->GetEntity()->GetState() == AZ::Entity::State::Active));
        if (isActive)
        {
            //deactivate and activate the log file again on rollover length change
            DeactivateLogFile();
            ActivateLogFile();
        }
    }

    const char* LogComponent::GetLogFileBaseName()
    {
        return m_logFileBaseName.c_str();
    }

    AZ::u64 LogComponent::GetRollOverLength()
    {
        return m_rolloverLength;
    }
};
