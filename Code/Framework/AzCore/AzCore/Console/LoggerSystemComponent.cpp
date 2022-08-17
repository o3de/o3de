/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/LoggerSystemComponent.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    const char* GetEnumString(LogLevel logLevel)
    {
        switch (logLevel)
        {
        case LogLevel::Trace:
            return "Trace";
        case LogLevel::Debug:
            return "Debug";
        case LogLevel::Info:
            return "Info";
        case LogLevel::Notice:
            return "Notice";
        case LogLevel::Warn:
            return "Warn";
        case LogLevel::Error:
            return "Error";
        case LogLevel::Fatal:
            return "Fatal";
        default:
            break;
        }
        return "UNKNOWN";
    }

    void LoggerSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<LoggerSystemComponent, AZ::Component>()
                ->Version(1);
        }
    }

    void LoggerSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("LoggerService"));
    }

    void LoggerSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("LoggerService"));
    }

    LoggerSystemComponent::LoggerSystemComponent()
    {
        AZ::Interface<ILogger>::Register(this);
        ILoggerRequestBus::Handler::BusConnect();
    }

    LoggerSystemComponent::~LoggerSystemComponent()
    {
        ILoggerRequestBus::Handler::BusDisconnect();
        AZ::Interface<ILogger>::Unregister(this);
    }

    void LoggerSystemComponent::Activate()
    {
        ;
    }

    void LoggerSystemComponent::Deactivate()
    {
        ;
    }

    void LoggerSystemComponent::SetLogName(const char* logName)
    {
        m_logName = logName;
    }

    const char* LoggerSystemComponent::GetLogName() const
    {
        return m_logName.c_str();
    }

    void LoggerSystemComponent::SetLogLevel(LogLevel logLevel)
    {
        m_logLevel = logLevel;
    }

    LogLevel LoggerSystemComponent::GetLogLevel() const
    {
        return m_logLevel;
    }

    void LoggerSystemComponent::BindLogHandler(LogEvent::Handler& handler)
    {
        handler.Connect(m_logEvent);
    }

    bool LoggerSystemComponent::IsTagEnabled(AZ::HashValue32 hashValue)
    {
        if (!m_quickHash.test(static_cast<AZStd::size_t>(hashValue) & (BitsetSize - 1)))
        {
            return false;
        }

        return IsTagEnabledHelper(hashValue);
    }

    void LoggerSystemComponent::Flush()
    {
        ;
    }

    void LoggerSystemComponent::LogInternalV(LogLevel level, const char* format, const char* file, const char* function, int32_t line, va_list args)
    {
        constexpr AZStd::size_t MaxLogBufferSize = 1000;
        auto buffer = AZStd::fixed_string<MaxLogBufferSize>::format_arg(format, args);
        m_logEvent.Signal(level, buffer.c_str(), file, function, line);
        buffer += '\n';

        // use %s to avoid potential format security issues
        switch (level)
        {
        case LogLevel::Warn:
            AZ_Warning(Debug::Trace::GetDefaultSystemWindow(), false, "%s", buffer.c_str());
            break;
        case LogLevel::Error:
            AZ_Error(Debug::Trace::GetDefaultSystemWindow(), false, "%s", buffer.c_str());
            break;
        default:
            AZ::Debug::Trace::Instance().Printf(Debug::Trace::GetDefaultSystemWindow(), "%s", buffer.c_str());
            break;
        }
    }

    void LoggerSystemComponent::SetLevel(const AZ::ConsoleCommandContainer& arguments)
    {
        if (arguments.empty())
        {
            return;
        }

        AZ::CVarFixedString argument{ arguments.front() };
        char* endPtr = nullptr;
        int64_t value = static_cast<int64_t>(strtoll(argument.c_str(), &endPtr, 0));

        if ((value < static_cast<int32_t>(LogLevel::Trace)) || (value > static_cast<int32_t>(LogLevel::Fatal)))
        {
            AZLOG_ERROR("Invalid log level: %d", static_cast<int32_t>(value));
            return;
        }

        m_logLevel = static_cast<LogLevel>(value);
    }

    void LoggerSystemComponent::EnableLog(const AZ::ConsoleCommandContainer& arguments)
    {
        for (uint32_t i = 0; i < arguments.size(); ++i)
        {
            AZ::CVarFixedString argument{ arguments[i] };
            const AZ::HashValue32 tagHash = AZ::TypeHash32(argument.c_str());
            if (!IsTagEnabledHelper(tagHash))
            {
                EnableLogHelper(tagHash);
            }
        }
    }

    void LoggerSystemComponent::DisableLog(const AZ::ConsoleCommandContainer& arguments)
    {
        for (uint32_t i = 0; i < arguments.size(); ++i)
        {
            AZ::CVarFixedString argument{ arguments[i] };
            const AZ::HashValue32 tagHash = AZ::TypeHash32(argument.c_str());
            if (IsTagEnabledHelper(tagHash))
            {
                DisableLogHelper(tagHash);
            }
        }
    }

    void LoggerSystemComponent::ToggleLog(const AZ::ConsoleCommandContainer& arguments)
    {
        for (uint32_t i = 0; i < arguments.size(); ++i)
        {
            AZ::CVarFixedString argument{ arguments[i] };
            const AZ::HashValue32 tagHash = AZ::TypeHash32(argument.c_str());
            if (IsTagEnabledHelper(tagHash))
            {
                DisableLogHelper(tagHash);
            }
            else
            {
                EnableLogHelper(tagHash);
            }
        }
    }

    void LoggerSystemComponent::EnableLogHelper(AZ::HashValue32 hashValue)
    {
        m_quickHash.set(static_cast<AZStd::size_t>(hashValue) & (BitsetSize - 1), true);

        AZStd::scoped_lock lock(m_enabledTagsMutex);
        m_enabledTags.push_back(hashValue);
    }

    void LoggerSystemComponent::DisableLogHelper(AZ::HashValue32 hashValue)
    {
        AZStd::scoped_lock lock(m_enabledTagsMutex);

        m_enabledTags.erase(AZStd::find(m_enabledTags.begin(), m_enabledTags.end(), hashValue));

        m_quickHash.reset();
        for (auto enabledTag : m_enabledTags)
        {
            m_quickHash.set(static_cast<AZStd::size_t>(enabledTag) & (BitsetSize - 1), true);
        }
    }

    bool LoggerSystemComponent::IsTagEnabledHelper(AZ::HashValue32 hashValue)
    {
        AZStd::scoped_lock lock(m_enabledTagsMutex);
        return AZStd::find(m_enabledTags.begin(), m_enabledTags.end(), hashValue) != m_enabledTags.end();
    }
}
