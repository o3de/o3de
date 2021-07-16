/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/Utils/TypeHash.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/EBus/Event.h>
#include <AzCore/RTTI/RTTI.h>
#include <stdarg.h>

namespace AZ
{
    //! This essentially maps to standard syslog logging priorities to allow the logger to easily plug into standard logging services
    enum class LogLevel : int8_t { Trace = 1, Debug = 2, Info = 3, Notice = 4, Warn = 5, Error = 6, Fatal = 7 };

    //! @class ILogger
    //! @brief This is an AZ::Interface<> for logging.
    //! Usage:
    //!  #include <AzCore/Console/ILogger.h>
    //!  AZLOG_INFO("Your message here");
    //!  AZLOG_WARN("Your warn message here");
    class ILogger
    {
    public:
        AZ_RTTI(ILogger, "{69950316-3626-4C9D-9DCA-2E7ABF84C0A9}");

        // LogLevel, message, file, function, line
        using LogEvent = AZ::Event<LogLevel, const char*, const char*, const char*, int32_t>;

        ILogger() = default;
        virtual ~ILogger() = default;

        //! Sets the the name of the log file.
        //! @param a_LogName the new logfile name to use
        virtual void SetLogName(const char* logName) = 0;

        //! Gets the the name of the log file.
        //! @return the current logfile name
        virtual const char* GetLogName() const = 0;

        //! Sets the log level for the logger instance.
        //! @param logLevel the minimum log level to filter out log messages at
        virtual void SetLogLevel(LogLevel logLevel) = 0;

        //! Gets the log level for the logger instance.
        //! @return the current minimum log level to filter out log messages at
        virtual LogLevel GetLogLevel() const = 0;

        //! Binds a log event handler.
        //! @param handler the handler to bind to logging events
        virtual void BindLogHandler(LogEvent::Handler& hander) = 0;

        //! Queries whether the provided logging tag is enabled.
        //! @param hashValue the hash value for the provided logging tag
        //! @return boolean true if enabled
        virtual bool IsTagEnabled(AZ::HashValue32 hashValue) = 0;

        //! Immediately Flushes any pending messages without waiting for next thread update.
        //! Should be invoked whenever unloading any shared library or module to avoid crashing on dangling string pointers
        virtual void Flush() = 0;

        //! Don't use this directly, use the logger macros defined below (AZLOG_INFO, AZLOG_WARN, AZLOG_ERROR, etc..)
        virtual void LogInternalV(LogLevel level, const char* format, const char* file, const char* function, int32_t line, va_list args) = 0;

        //! Don't use this directly, use the logger macros defined below (AZLOG_INFO, AZLOG_WARN, AZLOG_ERROR, etc..)
        inline void LogInternal(LogLevel level, const char* format, const char* file, const char* function, int32_t line, ...) AZ_FORMAT_ATTRIBUTE(3, 7)
        {
            va_list args;
            va_start(args, line);
            LogInternalV(level, format, file, function, line, args);
            va_end(args);
        }
    };

    // EBus wrapper for ScriptCanvas
    class ILoggerRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };
    using ILoggerRequestBus = AZ::EBus<ILogger, ILoggerRequests>;
}

#define AZLOG_TRACE(MESSAGE, ...)                                                                            \
{                                                                                                            \
    AZ::ILogger* logger = AZ::Interface<AZ::ILogger>::Get();                                                 \
    if (logger != nullptr && AZ::LogLevel::Trace >= logger->GetLogLevel())                                   \
    {                                                                                                        \
        logger->LogInternal(AZ::LogLevel::Trace, MESSAGE, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
    }                                                                                                        \
}

#define AZLOG_DEBUG(MESSAGE, ...)                                                                            \
{                                                                                                            \
    AZ::ILogger* logger = AZ::Interface<AZ::ILogger>::Get();                                                 \
    if (logger != nullptr && AZ::LogLevel::Debug >= logger->GetLogLevel())                                   \
    {                                                                                                        \
        logger->LogInternal(AZ::LogLevel::Debug, MESSAGE, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
    }                                                                                                        \
}

#define AZLOG_INFO(MESSAGE, ...)                                                                             \
{                                                                                                            \
    AZ::ILogger* logger = AZ::Interface<AZ::ILogger>::Get();                                                 \
    if (logger != nullptr && AZ::LogLevel::Info >= logger->GetLogLevel())                                    \
    {                                                                                                        \
        logger->LogInternal(AZ::LogLevel::Info, MESSAGE, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);   \
    }                                                                                                        \
}

#define AZLOG_NOTICE(MESSAGE, ...)                                                                           \
{                                                                                                            \
    AZ::ILogger* logger = AZ::Interface<AZ::ILogger>::Get();                                                 \
    if (logger != nullptr && AZ::LogLevel::Notice >= logger->GetLogLevel())                                  \
    {                                                                                                        \
        logger->LogInternal(AZ::LogLevel::Notice, MESSAGE, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    }                                                                                                        \
}

#define AZLOG_WARN(MESSAGE, ...)                                                                             \
{                                                                                                            \
    AZ::ILogger* logger = AZ::Interface<AZ::ILogger>::Get();                                                 \
    if (logger != nullptr && AZ::LogLevel::Warn >= logger->GetLogLevel())                                    \
    {                                                                                                        \
        logger->LogInternal(AZ::LogLevel::Warn, MESSAGE, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);   \
    }                                                                                                        \
}

#define AZLOG_ERROR(MESSAGE, ...)                                                                            \
{                                                                                                            \
    AZ::ILogger* logger = AZ::Interface<AZ::ILogger>::Get();                                                 \
    if (logger != nullptr && AZ::LogLevel::Error >= logger->GetLogLevel())                                   \
    {                                                                                                        \
        logger->LogInternal(AZ::LogLevel::Error, MESSAGE, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
    }                                                                                                        \
}

#define AZLOG_FATAL(MESSAGE, ...)                                                                            \
{                                                                                                            \
    AZ::ILogger* logger = AZ::Interface<AZ::ILogger>::Get();                                                 \
    if (logger != nullptr && AZ::LogLevel::Fatal >= logger->GetLogLevel())                                   \
    {                                                                                                        \
        logger->LogInternal(AZ::LogLevel::Fatal, MESSAGE, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
    }                                                                                                        \
}

#define AZLOG(TAG, MESSAGE, ...)                                                                             \
{                                                                                                            \
    static const AZ::HashValue32 hashValue = AZ::TypeHash32(#TAG);                                           \
    AZ::ILogger* logger = AZ::Interface<AZ::ILogger>::Get();                                                 \
    if (logger != nullptr && logger->IsTagEnabled(hashValue))                                                \
    {                                                                                                        \
        logger->LogInternal(AZ::LogLevel::Notice, MESSAGE, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    }                                                                                                        \
}

#define AZLOG_FLUSH()                                                                                        \
{                                                                                                            \
    AZ::ILogger* logger = AZ::Interface<AZ::ILogger>::Get();                                                 \
    if (logger != nullptr)                                                                                   \
    {                                                                                                        \
        logger->Flush();                                                                                     \
    }                                                                                                        \
}
