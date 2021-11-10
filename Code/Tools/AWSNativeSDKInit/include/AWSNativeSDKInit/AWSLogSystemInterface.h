/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)

#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4251 4996, "-Wunknown-warning-option")
#include <aws/core/utils/logging/LogSystemInterface.h>
AZ_POP_DISABLE_WARNING
#else
#include <sstream>
namespace Aws
{
    using OStringStream = std::basic_ostringstream<char>;
    namespace Utils
    {
        namespace Logging
        {
            using LogLevel = int;
        }
    }
}
#endif
namespace AWSNativeSDKInit
{
    class AWSLogSystemInterface 
#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
        : public Aws::Utils::Logging::LogSystemInterface
#endif
    {

    public:
        static const char* AWS_API_LOG_PREFIX;
        static const int MAX_MESSAGE_LENGTH;
        static const char* MESSAGE_FORMAT;
        static const char* ERROR_WINDOW_NAME;
        static const char* LOG_ENV_VAR;

        AWSLogSystemInterface(Aws::Utils::Logging::LogLevel logLevel);

        /**
        * Gets the currently configured log level for this logger.
        */
#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
        Aws::Utils::Logging::LogLevel GetLogLevel(void) const override;
#else
        Aws::Utils::Logging::LogLevel GetLogLevel(void) const;
#endif
        /**
        * Does a printf style output to the output stream. Don't use this, it's unsafe. See LogStream
        */
#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
        void Log(Aws::Utils::Logging::LogLevel logLevel, const char* tag, const char* formatStr, ...) override;
#else
        void Log(Aws::Utils::Logging::LogLevel logLevel, const char* tag, const char* formatStr, ...);
#endif

        /**
        * Writes the stream to the output stream.
        */
#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
        void LogStream(Aws::Utils::Logging::LogLevel logLevel, const char* tag, const Aws::OStringStream &messageStream) override;
#else
        void LogStream(Aws::Utils::Logging::LogLevel logLevel, const char* tag, const Aws::OStringStream& messageStream);
#endif

#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
        void Flush() override;
#else
        void Flush();
#endif

    private:
        bool ShouldLog(Aws::Utils::Logging::LogLevel logLevel);
        void SetLogLevel(Aws::Utils::Logging::LogLevel newLevel);
        void ForwardAwsApiLogMessage(Aws::Utils::Logging::LogLevel logLevel, const char* tag, const char* message);

        Aws::Utils::Logging::LogLevel m_logLevel;
    };

}
