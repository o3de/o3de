/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/EBus/Event.h>
#include <ScriptCanvas/Core/ExecutionNotificationsBus.h>

#include <ScriptCanvas/Core/Core.h>

#if !defined(_RELEASE)
// the markers are defined in test, but the system that listens for calls won't always be enabled
#define SCRIPT_CANVAS_PERFORMANCE_TRACKING_ENABLED
#endif

#if defined(SCRIPT_CANVAS_PERFORMANCE_TRACKING_ENABLED)
#define SCRIPT_CANVAS_PERFORMANCE_FINALIZE_TIMER(executionState) ScriptCanvas::Execution::FinalizePerformanceReport(executionState);
#define SCRIPT_CANVAS_PERFORMANCE_SCOPE_INITIALIZATION(executionState) ScriptCanvas::Execution::PerformanceScopeInitialization initializationScope(executionState);
#define SCRIPT_CANVAS_PERFORMANCE_SCOPE_EXECUTION(executionState) ScriptCanvas::Execution::PerformanceScopeExecution executionScope(executionState);
#define SCRIPT_CANVAS_PERFORMANCE_SCOPE_LATENT(executionState) ScriptCanvas::Execution::PerformanceScopeLatent latentScope(executionState);
// use this to protect nodeables from implementation changes
#define SCRIPT_CANVAS_PERFORMANCE_SCOPE_LATENT_NODEABLE ScriptCanvas::Execution::PerformanceScopeLatent latentScope(GetExecutionState());
#else
#define SCRIPT_CANVAS_PERFORMANCE_FINALIZE_TIMER(executionState)
#define SCRIPT_CANVAS_PERFORMANCE_SCOPE_INITIALIZATION(executionState)
#define SCRIPT_CANVAS_PERFORMANCE_SCOPE_EXECUTION(executionState)
#define SCRIPT_CANVAS_PERFORMANCE_SCOPE_LATENT(executionState)
#define SCRIPT_CANVAS_PERFORMANCE_SCOPE_LATENT_NODEABLE
#endif

namespace ScriptCanvas
{
    class ExecutionState;

    namespace Execution
    {
        using PerformanceKey = const ExecutionState*;

        struct PerformanceTimingReport
        {
            AZ_TYPE_INFO(PerformanceTimingReport, "{AEBF259D-D51F-40F6-B78E-160C9B9FC5B4}");
            AZ_CLASS_ALLOCATOR(PerformanceTimingReport, AZ::SystemAllocator);

            AZStd::sys_time_t initializationTime = 0;
            AZStd::sys_time_t executionTime = 0;
            AZStd::sys_time_t latentTime = 0;
            AZ::u32 latentExecutions = 0;
            AZStd::sys_time_t totalTime = 0;

            PerformanceTimingReport& operator+=(const PerformanceTimingReport& rhs);
        };

        struct PerformanceTrackingReport
        {
            AZ_TYPE_INFO(PerformanceTrackingReport, "{48CD6F7A-CB3D-466A-9291-567DA9E0E961}");
            AZ_CLASS_ALLOCATOR(PerformanceTrackingReport, AZ::SystemAllocator);

            PerformanceTimingReport timing;
            AZ::u32 activationCount = 0;

            PerformanceTrackingReport& operator+=(const PerformanceTrackingReport& rhs);
        };

        using PerformanceReportByAsset = AZStd::unordered_map<AZ::Data::AssetId, PerformanceTrackingReport>;

        struct PerformanceReport
        {
            AZ_TYPE_INFO(PerformanceReport, "{D0FFBFFA-6662-44D4-A25E-65C65D4B422A}");
            AZ_CLASS_ALLOCATOR(PerformanceTrackingReport, AZ::SystemAllocator);

            PerformanceTrackingReport tracking;
            PerformanceReportByAsset byAsset;
        };

        void FinalizePerformanceReport(PerformanceKey key);

        class PerformanceScope
        {
        public:
            PerformanceScope(const PerformanceKey& key);

        protected:
            PerformanceKey m_key;
            AZStd::chrono::steady_clock::time_point m_startTime;
        };

        class PerformanceScopeExecution : public PerformanceScope
        {
        public:
            PerformanceScopeExecution(const PerformanceKey& key);
            ~PerformanceScopeExecution();
        };

        class PerformanceScopeInitialization : public PerformanceScope
        {
        public:
            PerformanceScopeInitialization(const PerformanceKey& key);
            ~PerformanceScopeInitialization();
        };

        class PerformanceScopeLatent : public PerformanceScope
        {
        public:
            PerformanceScopeLatent(const PerformanceKey& key);
            ~PerformanceScopeLatent();
        };
    }
}
