/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Execution/ExecutionPerformanceTimer.h>
#include <ScriptCanvas/Execution/ExecutionState.h>
#include <ScriptCanvas/PerformanceTracker.h>

namespace ScriptCanvas
{
    namespace Execution
    {
        PerformanceTracker::PerformanceTracker()
        {}

        PerformanceTracker::~PerformanceTracker()
        {
            AZStd::lock_guard lock(m_activeTimerMutex);

            for (auto iter : m_activeTimers)
            {
                delete iter.second;
            }
            m_activeTimers.clear();

            for (auto iter : m_timersByAsset)
            {
                delete iter.second;
            }
            m_timersByAsset.clear();
        }

        void PerformanceTracker::CalculateReports()
        {
            AZStd::lock_guard lock(m_activeTimerMutex);
            
            m_snapshotReport.tracking.activationCount += aznumeric_cast<AZ::u32>(m_activeTimers.size());
            m_globalReport.tracking.activationCount += aznumeric_cast<AZ::u32>(m_activeTimers.size());

            for (auto& iter : m_activeTimers)
            {
                const auto report = iter.second->GetReport();
                m_snapshotReport.tracking.timing += report;
                m_globalReport.tracking.timing += report;
                delete iter.second;
            }

            m_activeTimers.clear();

            for (auto& iter : m_timersByAsset)
            {
                const auto report = iter.second->timer.GetReport();
                auto& snapshotByAssetReport = *ModOrCreateReport(m_snapshotReport.byAsset, iter.first);
                auto& globalByAssetReport = *ModOrCreateReport(m_globalReport.byAsset, iter.first);
                snapshotByAssetReport.timing += report;
                snapshotByAssetReport.activationCount += iter.second->assetActivationCount;
                globalByAssetReport.timing += report;
                globalByAssetReport.activationCount += iter.second->assetActivationCount;
                delete iter.second;
            }

            m_timersByAsset.clear();

            m_lastCapturedSnapshot = m_snapshotReport;
            m_lastCapturedGlobal = m_globalReport;
            m_snapshotReport = {};
        }

        void PerformanceTracker::ClearGlobalReport()
        {
            AZStd::lock_guard lock(m_activeTimerMutex);
            m_globalReport = {};
        }

        void PerformanceTracker::ClearSnapshotReport()
        {
            AZStd::lock_guard lock(m_activeTimerMutex);
            m_snapshotReport = {};
        }

        PerformanceTimer* PerformanceTracker::CreateTimer(PerformanceKey key)
        {
            AZStd::lock_guard lock(m_activeTimerMutex);
            return m_activeTimers.insert({ key, aznew PerformanceTimer() }).first->second;
        }

        void PerformanceTracker::FinalizeReport(PerformanceKey key)
        {
            const AZ::Data::AssetId assetId = key->GetAssetId();
            AZStd::lock_guard lock(m_activeTimerMutex);

            auto iter = m_activeTimers.find(key);
            if (iter != m_activeTimers.end())
            {
                PerformanceTimer* timer = iter->second;
                auto report = timer->GetReport();
                auto& globalByAssetReport = *ModOrCreateReport(m_globalReport.byAsset, assetId);
                auto& snapshotByAssetReport = *ModOrCreateReport(m_snapshotReport.byAsset, assetId);
                snapshotByAssetReport.timing += report;
                globalByAssetReport.timing += report;
                m_snapshotReport.tracking.timing += report;
                m_globalReport.tracking.timing += report;
                m_activeTimers.erase(iter);
                delete timer;
            }
        }

        PerformanceTracker::AssetTimer* PerformanceTracker::GetOrCreateTimer(const AZ::Data::AssetId& key)
        {
            AZStd::lock_guard lock(m_activeTimerMutex);
            auto iter = m_timersByAsset.find(key);

            if (iter != m_timersByAsset.end())
            {
                return iter->second;
            }
            else
            {
                return m_timersByAsset.insert({ key, aznew PerformanceTracker::AssetTimer() }).first->second;
            }
        }
                
        PerformanceTimer* PerformanceTracker::GetOrCreateTimer(PerformanceKey key)
        {
            AZStd::lock_guard lock(m_activeTimerMutex);
            auto iter = m_activeTimers.find(key);

            if (iter != m_activeTimers.end())
            {
                return iter->second;
            }
            else
            {
                return m_activeTimers.insert({ key, aznew PerformanceTimer() }).first->second;
            }
        }

        PerformanceTrackingReport PerformanceTracker::GetGlobalReport() const
        {
            return m_lastCapturedGlobal.tracking;
        }

        PerformanceTrackingReport PerformanceTracker::GetGlobalReportByAsset(const AZ::Data::AssetId& assetId) const
        {
            return GetReportByAsset(m_lastCapturedGlobal.byAsset, assetId);
        }

        const PerformanceReport& PerformanceTracker::GetGlobalReportFull() const
        {
            return m_globalReport;
        }

        PerformanceTrackingReport PerformanceTracker::GetReportByAsset(const PerformanceReportByAsset& reports, AZ::Data::AssetId key)
        {
            auto iter = reports.find(key);
            return iter != reports.end() ? iter->second : PerformanceTrackingReport{};
        }

        PerformanceTrackingReport PerformanceTracker::GetSnapshotReport() const
        {
            return m_lastCapturedSnapshot.tracking;
        }

        PerformanceTrackingReport PerformanceTracker::GetSnapshotReportByAsset(const AZ::Data::AssetId& assetId) const
        {
            return GetReportByAsset(m_lastCapturedSnapshot.byAsset, assetId);
        }

        const PerformanceReport& PerformanceTracker::GetSnapshotReportFull() const
        {
            return m_lastCapturedSnapshot;
        }

        PerformanceTrackingReport* PerformanceTracker::ModOrCreateReport(PerformanceReportByAsset& reports, AZ::Data::AssetId key)
        {
            auto iter = reports.find(key);

            if (iter != reports.end())
            {
                return &iter->second;
            }
            else
            {
                return &reports.insert({ key, PerformanceTrackingReport() }).first->second;
            }
        }

        void PerformanceTracker::ReportExecutionTime(PerformanceKey key, AZStd::sys_time_t time)
        {
            GetOrCreateTimer(key)->AddExecutionTime(time);
            GetOrCreateTimer(key->GetAssetId())->timer.AddExecutionTime(time);
        }

        void PerformanceTracker::ReportLatentTime(PerformanceKey key, AZStd::sys_time_t time)
        {
            GetOrCreateTimer(key)->AddLatentTime(time);
            GetOrCreateTimer(key->GetAssetId())->timer.AddLatentTime(time);
        }

        void PerformanceTracker::ReportInitializationTime(PerformanceKey key, AZStd::sys_time_t time)
        {
             CreateTimer(key)->AddInitializationTime(time);
             AssetTimer* assetTimer = GetOrCreateTimer(key->GetAssetId());
             assetTimer->timer.AddInitializationTime(time);
             ++(assetTimer->assetActivationCount);             
        }
    }
}
