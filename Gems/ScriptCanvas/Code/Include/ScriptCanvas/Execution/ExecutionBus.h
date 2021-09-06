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
#define SCRIPT_CANVAS_PERFORMANCE_FINALIZE_TIMER(scriptCanvasId, assetId) ScriptCanvas::Execution::FinalizePerformanceReport(scriptCanvasId, assetId);
#define SCRIPT_CANVAS_PERFORMANCE_SCOPE_INITIALIZATION(scriptCanvasId, assetId) ScriptCanvas::Execution::PerformanceScopeInitialization initializationScope(scriptCanvasId, assetId);
#define SCRIPT_CANVAS_PERFORMANCE_SCOPE_EXECUTION(scriptCanvasId, assetId) ScriptCanvas::Execution::PerformanceScopeExecution executionScope(scriptCanvasId, assetId);
#define SCRIPT_CANVAS_PERFORMANCE_SCOPE_LATENT(scriptCanvasId, assetId) ScriptCanvas::Execution::PerformanceScopeLatent latentScope(scriptCanvasId, assetId);
#else
#define SCRIPT_CANVAS_PERFORMANCE_FINALIZE_TIMER(scriptCanvasId, assetId)
#define SCRIPT_CANVAS_PERFORMANCE_SCOPE_INITIALIZATION(scriptCanvasId, assetId)
#define SCRIPT_CANVAS_PERFORMANCE_SCOPE_EXECUTION(scriptCanvasId, assetId)
#define SCRIPT_CANVAS_PERFORMANCE_SCOPE_LATENT(scriptCanvasId, assetId)
#endif

namespace ScriptCanvas
{    
    GraphInfo CreateGraphInfo(ScriptCanvasId executionId, const GraphIdentifier& graphIdentifier);

    namespace Execution
    {
        using PerformanceKey = AZ::EntityId; // ScriptCanvasId

        struct PerformanceTimingReport
        {
            AZ_TYPE_INFO(PerformanceTimingReport, "{AEBF259D-D51F-40F6-B78E-160C9B9FC5B4}");
            AZ_CLASS_ALLOCATOR(PerformanceTimingReport, AZ::SystemAllocator, 0);

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
            AZ_CLASS_ALLOCATOR(PerformanceTrackingReport, AZ::SystemAllocator, 0);

            PerformanceTimingReport timing;
            AZ::u32 activationCount = 0;

            PerformanceTrackingReport& operator+=(const PerformanceTrackingReport& rhs);
        };

        using PerformanceReportByAsset = AZStd::unordered_map<AZ::Data::AssetId, PerformanceTrackingReport>;

        struct PerformanceReport
        {
            AZ_TYPE_INFO(PerformanceReport, "{D0FFBFFA-6662-44D4-A25E-65C65D4B422A}");
            AZ_CLASS_ALLOCATOR(PerformanceTrackingReport, AZ::SystemAllocator, 0);

            PerformanceTrackingReport tracking;
            PerformanceReportByAsset byAsset;
        };

        void FinalizePerformanceReport(PerformanceKey key, const AZ::Data::AssetId& assetId);

        class PerformanceScope
        {
        public:
            PerformanceScope(const PerformanceKey& key, const AZ::Data::AssetId& assetId);

        protected:
            PerformanceKey m_key;
            AZ::Data::AssetId m_assetId;
            AZStd::chrono::system_clock::time_point m_startTime;
        };

        class PerformanceScopeExecution : public PerformanceScope
        {
        public:
            PerformanceScopeExecution(const PerformanceKey& key, const AZ::Data::AssetId& assetId);
            ~PerformanceScopeExecution();
        };

        class PerformanceScopeInitialization : public PerformanceScope
        {
        public:
            PerformanceScopeInitialization(const PerformanceKey& key, const AZ::Data::AssetId& assetId);
            ~PerformanceScopeInitialization();
        };

        class PerformanceScopeLatent : public PerformanceScope
        {
        public:
            PerformanceScopeLatent(const PerformanceKey& key, const AZ::Data::AssetId& assetId);
            ~PerformanceScopeLatent();
        };
    }
}
