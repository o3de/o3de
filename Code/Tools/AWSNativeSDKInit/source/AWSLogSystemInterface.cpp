/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AWSNativeSDKInit/AWSLogSystemInterface.h>

#include <AzCore/base.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Module/Environment.h>

#include <stdarg.h>

#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
AZ_PUSH_DISABLE_WARNING(4251 4996, "-Wunknown-warning-option")
#include <aws/core/utils/logging/AWSLogging.h>
#include <aws/core/utils/logging/DefaultLogSystem.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
AZ_POP_DISABLE_WARNING
#endif

namespace AWSNativeSDKInit
{
    AZ_CVAR(int, bg_awsLogLevel, -1, nullptr, AZ::ConsoleFunctorFlags::Null,
        "AWSLogLevel used to control verbosity of logging system. Off = 0, Fatal = 1, Error = 2, Warn = 3, Info = 4, Debug = 5, Trace = 6");

    const char* AWSLogSystemInterface::AWS_API_LOG_PREFIX = "AwsApi-";
    const int AWSLogSystemInterface::MAX_MESSAGE_LENGTH = 4096;
    const char* AWSLogSystemInterface::MESSAGE_FORMAT = "[AWS] %s - %s";
    const char* AWSLogSystemInterface::ERROR_WINDOW_NAME = "AwsNativeSDK"; 

    AWSLogSystemInterface::AWSLogSystemInterface(Aws::Utils::Logging::LogLevel logLevel)
        : m_logLevel(logLevel)
    {
    }

    /**
    * Gets the currently configured log level for this logger.
    */
    Aws::Utils::Logging::LogLevel AWSLogSystemInterface::GetLogLevel() const
    {
        Aws::Utils::Logging::LogLevel newLevel = m_logLevel;
        if (auto console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
        {
            int awsLogLevel = -1;
            console->GetCvarValue("bg_awsLogLevel", awsLogLevel);
            if (awsLogLevel >= 0)
            {
                newLevel = static_cast<Aws::Utils::Logging::LogLevel>(awsLogLevel);
            }
        }
        return newLevel;
    }

    /**
    * Does a printf style output to the output stream. Don't use this, it's unsafe. See LogStream
    */
    void AWSLogSystemInterface::Log(Aws::Utils::Logging::LogLevel logLevel, const char* tag, const char* formatStr, ...)
    {

        if (!ShouldLog(logLevel))
        {
            return;
        }

        char message[MAX_MESSAGE_LENGTH];

        va_list mark;
        va_start(mark, formatStr);
        azvsnprintf(message, MAX_MESSAGE_LENGTH, formatStr, mark);
        va_end(mark);

        ForwardAwsApiLogMessage(logLevel, tag, message);

    }

    /**
    * Writes the stream to the output stream.
    */
    void AWSLogSystemInterface::LogStream(Aws::Utils::Logging::LogLevel logLevel, const char* tag, const Aws::OStringStream &messageStream)
    {
        if(!ShouldLog(logLevel)) 
        {
            return;
        }

        ForwardAwsApiLogMessage(logLevel, tag, messageStream.str().c_str());
    }

    bool AWSLogSystemInterface::ShouldLog(Aws::Utils::Logging::LogLevel logLevel)
    {
#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
        Aws::Utils::Logging::LogLevel newLevel = GetLogLevel();

        if (newLevel != m_logLevel)
        {
            SetLogLevel(newLevel);
        }
#endif
        return (logLevel <= m_logLevel);
    }

    void AWSLogSystemInterface::SetLogLevel(Aws::Utils::Logging::LogLevel newLevel)
    {
#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
        Aws::Utils::Logging::ShutdownAWSLogging();
        Aws::Utils::Logging::InitializeAWSLogging(Aws::MakeShared<AWSLogSystemInterface>("AWS", newLevel));
        m_logLevel = newLevel;
#endif
    }

    void AWSLogSystemInterface::ForwardAwsApiLogMessage(Aws::Utils::Logging::LogLevel logLevel, const char* tag, const char* message)
    {
#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
        switch (logLevel)
        {

        case Aws::Utils::Logging::LogLevel::Off:
            break;

        case Aws::Utils::Logging::LogLevel::Fatal:
            AZ::Debug::Trace::Instance().Error(__FILE__, __LINE__, AZ_FUNCTION_SIGNATURE, AWSLogSystemInterface::ERROR_WINDOW_NAME, MESSAGE_FORMAT, tag, message);
            break;

        case Aws::Utils::Logging::LogLevel::Error:
            AZ::Debug::Trace::Instance().Error(__FILE__, __LINE__, AZ_FUNCTION_SIGNATURE, AWSLogSystemInterface::ERROR_WINDOW_NAME, MESSAGE_FORMAT, tag, message);
            break;

        case Aws::Utils::Logging::LogLevel::Warn:
            AZ::Debug::Trace::Instance().Warning(__FILE__, __LINE__, AZ_FUNCTION_SIGNATURE, AWSLogSystemInterface::ERROR_WINDOW_NAME, MESSAGE_FORMAT, tag, message);
            break;

        case Aws::Utils::Logging::LogLevel::Info:
            AZ::Debug::Trace::Instance().Printf(AWSLogSystemInterface::ERROR_WINDOW_NAME, MESSAGE_FORMAT, tag, message);
            break;

        case Aws::Utils::Logging::LogLevel::Debug:
            AZ::Debug::Trace::Instance().Printf(AWSLogSystemInterface::ERROR_WINDOW_NAME, MESSAGE_FORMAT, tag, message);
            break;

        case Aws::Utils::Logging::LogLevel::Trace:
            AZ::Debug::Trace::Instance().Printf(AWSLogSystemInterface::ERROR_WINDOW_NAME, MESSAGE_FORMAT, tag, message);
            break;

        default:
            break;

        }
#endif
    }

    void AWSLogSystemInterface::Flush()
    {
#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
        // No-op AZ Debug Trace doesn't have a flush API
#endif
    }
}
