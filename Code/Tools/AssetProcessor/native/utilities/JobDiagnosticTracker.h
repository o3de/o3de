/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <native/resourcecompiler/RCCommon.h>

namespace AssetProcessor
{
    struct JobDiagnosticInfo
    {
        JobDiagnosticInfo() = default;
        JobDiagnosticInfo(AZ::u32 warningCount, AZ::u32 errorCount)
            : m_warningCount(warningCount), m_errorCount(errorCount)
        {}

        bool operator==(const JobDiagnosticInfo& rhs) const;
        bool operator!=(const JobDiagnosticInfo& rhs) const;

        AZ::u32 m_warningCount = 0;
        AZ::u32 m_errorCount = 0;
    };

    enum class WarningLevel : AZ::u8
    {
        Default = 0,
        FatalErrors,
        FatalErrorsAndWarnings
    };

    class JobDiagnosticRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        typedef AZStd::recursive_mutex MutexType;

        virtual JobDiagnosticInfo GetDiagnosticInfo(AZ::u64 jobRunKey) const = 0;
        virtual void RecordDiagnosticInfo(AZ::u64 jobRunKey, JobDiagnosticInfo info) = 0;
        virtual WarningLevel GetWarningLevel() const = 0;
        virtual void SetWarningLevel(WarningLevel level) = 0;
    };

    using JobDiagnosticRequestBus = AZ::EBus<JobDiagnosticRequests>;

    class JobDiagnosticTracker
        : public JobDiagnosticRequestBus::Handler
    {
    public:
        JobDiagnosticTracker();
        ~JobDiagnosticTracker();

        JobDiagnosticInfo GetDiagnosticInfo(AZ::u64 jobRunKey) const override;
        void RecordDiagnosticInfo(AZ::u64 jobRunKey, JobDiagnosticInfo info) override;
        WarningLevel GetWarningLevel() const override;
        void SetWarningLevel(WarningLevel level) override;

        WarningLevel m_warningLevel = WarningLevel::Default;
        AZStd::unordered_map<AZ::u64, JobDiagnosticInfo> m_jobInfo;
    };
} // namespace AssetProcessor
