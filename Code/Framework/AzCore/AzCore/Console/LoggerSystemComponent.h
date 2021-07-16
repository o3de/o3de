/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Console/ILogger.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Component/Component.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/bitset.h>

namespace AZ
{
    //! Implementation of the ILogger system interface.
    class LoggerSystemComponent
        : public AZ::Component
        , public ILoggerRequestBus::Handler
    {
    public:

        AZ_COMPONENT(LoggerSystemComponent, "{56746640-9258-4D41-B255-663737493811}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        LoggerSystemComponent();
        virtual ~LoggerSystemComponent();

        //! AZ::Component overrides.
        //! @{
        void Activate() override;
        void Deactivate() override;
        //! @}

        //! ILogger overrides.
        //! @{
        void SetLogName(const char* logName) override;
        const char* GetLogName() const override;
        void SetLogLevel(LogLevel logLevel) override;
        LogLevel GetLogLevel() const override;
        void BindLogHandler(LogEvent::Handler& hander) override;
        bool IsTagEnabled(AZ::HashValue32 hashValue) override;
        void Flush() override;
        void LogInternalV(LogLevel level, const char* format, const char* file, const char* function, int32_t line, va_list args) override;
        //! @}

        //! Console commands.
        //! @{
        void SetLevel(const AZ::ConsoleCommandContainer& arguments);
        void EnableLog(const AZ::ConsoleCommandContainer& arguments);
        void DisableLog(const AZ::ConsoleCommandContainer& arguments);
        void ToggleLog(const AZ::ConsoleCommandContainer& arguments);
        //! @}

    private:

        void EnableLogHelper(AZ::HashValue32 a_HashValue);
        void DisableLogHelper(AZ::HashValue32 a_HashValue);
        bool IsTagEnabledHelper(AZ::HashValue32 a_HashValue);

        AZ_CONSOLEFUNC(LoggerSystemComponent, SetLevel,   AZ::ConsoleFunctorFlags::Null, "Sets the Logger log level");
        AZ_CONSOLEFUNC(LoggerSystemComponent, EnableLog,  AZ::ConsoleFunctorFlags::Null, "Enables conditional logs with the provided tag");
        AZ_CONSOLEFUNC(LoggerSystemComponent, DisableLog, AZ::ConsoleFunctorFlags::Null, "Disables conditional logs with the provided tag");
        AZ_CONSOLEFUNC(LoggerSystemComponent, ToggleLog,  AZ::ConsoleFunctorFlags::Null, "Toggles conditional logs with the provided tag");

        // Store a trivial bloom filter using the lower 10 bits.  This filter can be safely checked outside of lock to reduce contention.
        static constexpr uint32_t BitsetSize = 1024;
        static_assert(IsPowerOfTwo(BitsetSize), "Bloom filter bitset size must be a power of two");

        AZStd::string m_logName;
        LogLevel m_logLevel = LogLevel::Info;
        LogEvent m_logEvent;
        AZStd::bitset<BitsetSize> m_quickHash;
        AZStd::mutex m_enabledTagsMutex;
        AZStd::vector<AZ::HashValue32> m_enabledTags;
    };
}
