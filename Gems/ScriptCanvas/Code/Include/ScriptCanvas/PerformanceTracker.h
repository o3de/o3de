/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <ScriptCanvas/Execution/ExecutionBus.h>
#include <ScriptCanvas/Execution/ExecutionPerformanceTimer.h>

namespace ScriptCanvas
{
    namespace Execution
    {
        class PerformanceTimer;
                
        class PerformanceTracker
        {
            friend class PerformanceScopeExecution;
            friend class PerformanceScopeInitialization;
            friend class PerformanceScopeLatent;

        public:
            AZ_TYPE_INFO(PerformanceTracker, "{D40DFC8B-D4EA-4D6A-A0CA-3FDD00604553}");
            AZ_CLASS_ALLOCATOR(PerformanceTracker, AZ::SystemAllocator);

            PerformanceTracker();

            ~PerformanceTracker();

            void CalculateReports();

            void ClearGlobalReport();

            void ClearSnapshotReport();

            void FinalizeReport(PerformanceKey key);

            PerformanceTrackingReport GetGlobalReport() const;

            PerformanceTrackingReport GetGlobalReportByAsset(const AZ::Data::AssetId& assetId) const;

            // Not thread safe
            const PerformanceReport& GetGlobalReportFull() const;

            PerformanceTrackingReport GetSnapshotReport() const;
            
            PerformanceTrackingReport GetSnapshotReportByAsset(const AZ::Data::AssetId& assetId) const;

            // Not thread safe
            const PerformanceReport& GetSnapshotReportFull() const;

        private:
            static PerformanceTrackingReport* ModOrCreateReport(PerformanceReportByAsset& reports, AZ::Data::AssetId key);
            static PerformanceTrackingReport GetReportByAsset(const PerformanceReportByAsset& report, AZ::Data::AssetId key);

            PerformanceReport m_lastCapturedSnapshot;
            PerformanceReport m_lastCapturedGlobal;

            PerformanceReport m_snapshotReport;
            PerformanceReport m_globalReport;
            
            AZStd::recursive_mutex m_activeTimerMutex;

            AZStd::unordered_map<PerformanceKey, PerformanceTimer*> m_activeTimers;

            struct AssetTimer
            {
                AZ_TYPE_INFO(AssetTimer, "{80860A85-C6B7-4544-8C30-62C76C657427}");
                AZ_CLASS_ALLOCATOR(AssetTimer, AZ::SystemAllocator);

                PerformanceTimer timer;
                AZ::u32 assetActivationCount = 0;
            };

            AZStd::unordered_map<AZ::Data::AssetId, AssetTimer*> m_timersByAsset;
                        
            PerformanceTimer* CreateTimer(PerformanceKey key);

            AssetTimer* GetOrCreateTimer(const AZ::Data::AssetId& key);

            PerformanceTimer* GetOrCreateTimer(PerformanceKey key);

            void ReportExecutionTime(PerformanceKey key, AZStd::sys_time_t);

            void ReportLatentTime(PerformanceKey key, AZStd::sys_time_t);

            void ReportInitializationTime(PerformanceKey key, AZStd::sys_time_t);
        };
    }
}
