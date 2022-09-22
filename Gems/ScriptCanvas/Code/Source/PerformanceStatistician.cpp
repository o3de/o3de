/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Execution/ExecutionBus.h>
#include <ScriptCanvas/Execution/ExecutionPerformanceTimer.h>
#include <ScriptCanvas/PerformanceStatistician.h>
#include <ScriptCanvas/PerformanceTracker.h>
#include <ScriptCanvas/SystemComponent.h>

namespace ScriptCanvas
{
    namespace Execution
    {
        void PerformanceStatistics::CalculateSecondary()
        {
            scriptCostPercent = aznumeric_cast<double>(report.tracking.timing.totalTime) / aznumeric_cast<double>(duration);
        }

        AZStd::string ToConsoleString(const PerformanceStatistics& stats)
        {
            AZStd::string consoleString("\n");

            const double initializingMs = aznumeric_caster(stats.report.tracking.timing.initializationTime / 1000.0);
            const double executionMs = aznumeric_caster(stats.report.tracking.timing.executionTime / 1000.0);
            const double latentMs = aznumeric_caster(stats.report.tracking.timing.latentTime / 1000.0);
            const double totalMs = aznumeric_caster(stats.report.tracking.timing.totalTime / 1000.0);

            consoleString += "[ INITIALIZE] ";
            consoleString += AZStd::string::format("%7.3f ms \n", initializingMs);

            consoleString += "[  EXECUTION] ";
            consoleString += AZStd::string::format("%7.3f ms \n", executionMs);

            consoleString += "[     LATENT] ";
            consoleString += AZStd::string::format("%7.3f ms \n", latentMs);

            consoleString += "[      TOTAL] ";
            consoleString += AZStd::string::format("%7.3f ms \n", totalMs);

            consoleString += "[SCRIPT COST] ";
            consoleString += AZStd::string::format("%7.4f%% of duration \n", stats.scriptCostPercent);

            return consoleString;
        }

        PerformanceStatistician::PerformanceStatistician()
        {
            PerformanceStatisticsEBus::Handler::BusConnect();
        }

        void PerformanceStatistician::ClearSnaphotStatistics()
        {
            m_executedScripts.clear();

            auto perfTracker = SystemComponent::ModPerformanceTracker();
            perfTracker->ClearGlobalReport();
            perfTracker->ClearSnapshotReport();
        }

        void PerformanceStatistician::ClearTrackingState()
        {
            m_trackingState = TrackingState::None;

            if (AZ::SystemTickBus::Handler::BusIsConnected())
            {
                AZ::SystemTickBus::Handler::BusDisconnect();
            }
        }

        void PerformanceStatistician::ConnectToSystemTickBus()
        {
            if (!AZ::SystemTickBus::Handler::BusIsConnected())
            {
                AZ::SystemTickBus::Handler::BusConnect();
            }
        }

        AZStd::vector<AZStd::string> PerformanceStatistician::GetExecutedScriptsSinceLastSnapshot() const
        {
            AZStd::vector<AZStd::string> scripts;
            scripts.reserve(m_executedScripts.size());

            for (auto& iter : m_executedScripts)
            {
                scripts.push_back(iter.second);
            }

            return scripts;
        }

        const PerformanceStatistics& PerformanceStatistician::GetStatistics() const
        {
            return m_accumulatedStats;
        }

        void PerformanceStatistician::OnStartTrackingRequested()
        {
            m_accumulatedStats.tickCount = 0;
            m_accumulatedStartTime = AZStd::chrono::steady_clock::now();
        }

        void PerformanceStatistician::OnSystemTick()
        {
            switch (m_trackingState)
            {
            case TrackingState::AccumulatedInProgress:
                UpdateTickCounts();
                break;

            case TrackingState::AccumulatedStartRequested:
                OnStartTrackingRequested();
                m_trackingState = TrackingState::AccumulatedInProgress;
                break;

            case TrackingState::AccumulatedStopRequested:
                UpdateAccumulatedTime();
                UpdateStatisticsFromTracker();
                UpdateAccumulatedStatistics();
                ClearTrackingState();
                break;

            case TrackingState::PerFrameInProgress:
                UpdateTickCounts();
                UpdateStatisticsFromTracker();
                break;

            case TrackingState::PerFrameStartRequested:
                OnStartTrackingRequested();
                m_trackingState = TrackingState::PerFrameInProgress;
                break;

            case TrackingState::PerFrameStopRequested:
                UpdateAccumulatedTime();
                UpdateStatisticsFromTracker();
                ClearTrackingState();
                break;

            default:
                break;
            }
        }

        void PerformanceStatistician::Reflect(AZ::ReflectContext* reflectContext)
        {
            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
            {
                behaviorContext->EBus<PerformanceStatisticsEBus>("PerformanceStatisticsEBus")
                    ->Event("ClearSnaphotStatistics", &PerformanceStatisticsEBus::Events::ClearSnaphotStatistics)
                    ->Event("TrackAccumulatedStart", &PerformanceStatisticsEBus::Events::TrackAccumulatedStart)
                    ->Event("TrackAccumulatedStop", &PerformanceStatisticsEBus::Events::TrackAccumulatedStop)
                    ->Event("TrackPerFrameStart", &PerformanceStatisticsEBus::Events::TrackPerFrameStart)
                    ->Event("TrackPerFrameStop", &PerformanceStatisticsEBus::Events::TrackPerFrameStop)
                    ;
            }
        }

        void PerformanceStatistician::TrackAccumulatedStart(AZ::s32 tickCount)
        {
            if (m_trackingState != TrackingState::AccumulatedStartRequested || m_accumulatedTickCountRemaining != tickCount)
            {
                m_trackingState = TrackingState::AccumulatedStartRequested;
                m_accumulatedTickCountRemaining = tickCount;
                ConnectToSystemTickBus();
            }
        }

        void PerformanceStatistician::TrackAccumulatedStop()
        {
            if (m_trackingState == TrackingState::AccumulatedInProgress)
            {
                m_trackingState = TrackingState::AccumulatedStopRequested;
            }
        }

        void PerformanceStatistician::TrackPerFrameStart()
        {
            if (m_trackingState != TrackingState::PerFrameInProgress)
            {
                m_trackingState = TrackingState::PerFrameStartRequested;
                ConnectToSystemTickBus();
            }
        }

        void PerformanceStatistician::TrackPerFrameStop()
        {
            if (m_trackingState == TrackingState::PerFrameInProgress || m_trackingState == TrackingState::AccumulatedStartRequested)
            {
                m_trackingState = TrackingState::PerFrameStopRequested;
                AZ::SystemTickBus::Handler::BusConnect();
            }
        }

        void PerformanceStatistician::UpdateAccumulatedStatistics()
        {
            m_accumulatedStats.report = SystemComponent::ModPerformanceTracker()->GetGlobalReportFull();
            m_accumulatedStats.CalculateSecondary();
            AZ_TracePrintf("ScriptCanvas", "Global Performance Report:\n%s", ToConsoleString(m_accumulatedStats).c_str());
        }

        void PerformanceStatistician::UpdateAccumulatedTime()
        {
            m_accumulatedStats.duration = AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(AZStd::chrono::steady_clock::now() - m_accumulatedStartTime).count();
        }

        void PerformanceStatistician::UpdateStatisticsFromTracker()
        {
            auto perfTracker = SystemComponent::ModPerformanceTracker();
            perfTracker->CalculateReports();
            const PerformanceReport& snapShotReport = perfTracker->GetSnapshotReportFull();

            for (auto& snapshotIter : snapShotReport.byAsset)
            {
                auto iter = m_executedScripts.find(snapshotIter.first);
                if (iter == m_executedScripts.end())
                {
                    AZ::Data::AssetInfo info;
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(info, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, snapshotIter.first);
                    if (info.m_assetType == azrtti_typeid<ScriptCanvas::RuntimeAsset>())
                    {
                        AZStd::string fileName;
                        if (AZ::StringFunc::Path::GetFileName(info.m_relativePath.c_str(), fileName))
                        {
                            m_executedScripts.insert({ snapshotIter.first, fileName });
                        }
                    }
                }
            }
        }

        void PerformanceStatistician::UpdateTickCounts()
        {
            ++m_accumulatedStats.tickCount;
            --m_accumulatedTickCountRemaining;

            if (m_trackingState == TrackingState::AccumulatedInProgress && m_accumulatedTickCountRemaining == 0)
            {
                m_trackingState = TrackingState::AccumulatedStopRequested;
            }
        }
    }
}
