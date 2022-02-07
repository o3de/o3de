/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef LOGGINGCOMPONENT_H
#define LOGGINGCOMPONENT_H

#include <AzCore/Component/Component.h>
#include <AzCore/Debug/TraceMessageBus.h>

#include "LogFile.h"

namespace AzFramework
{
    //! LogComponent
    //! LogComponent listens to AZ trace messages and forwards them to a log file
    class LogComponent
        : public AZ::Component
        , private AZ::Debug::TraceMessageBus::Handler
    {
    public:
        AZ_COMPONENT(LogComponent, "{04AEB2E7-7F51-4426-9423-29D66C8DE1C1}")
        static void Reflect(AZ::ReflectContext* context);

        LogComponent();
        virtual ~LogComponent();

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::Debug::TraceMessagesBus
        bool OnPrintf(const char* window, const char* message) override;
        bool OnAssert(const char* message) override;
        bool OnException(const char* message) override;
        bool OnError(const char* window, const char* message) override;
        bool OnWarning(const char* window, const char* message) override;
        //////////////////////////////////////////////////////////////////////////

        virtual void OutputMessage(LogFile::SeverityLevel severity, const char* window, const char* message);

        void SetLogFileBaseName(const char* baseName);
        void SetRollOverLength(AZ::u64 rolloverLength);
        void SetMachineReadable(bool newValue);

        const char* GetLogFileBaseName();
        AZ::u64 GetRollOverLength();

    private:
        /// \ref AZ::ComponentDescriptor::GetProvidedServices
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        /// \ref AZ::ComponentDescriptor::GetIncompatibleServices
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        void ActivateLogFile();
        void DeactivateLogFile();

        bool m_machineReadable = true;
        AZStd::string m_logFileBaseName;
        AZ::u64 m_rolloverLength;
        LogFile* m_logFile;
    };
}

#endif
