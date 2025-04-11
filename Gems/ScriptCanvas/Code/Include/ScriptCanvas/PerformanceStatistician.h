/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/TickBus.h>
#include <ScriptCanvas/Execution/ExecutionBus.h>
#include <ScriptCanvas/PerformanceStatisticsBus.h>

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvas
{
    namespace Execution
    {
        struct PerformanceStatistics
        {
            // primary stats are from the initial tracking
            AZ::u32 tickCount;
            AZStd::sys_time_t duration;
            PerformanceReport report;

            // secondary stats are inferred from the primary stats
            double scriptCostPercent = 0.0;

            void CalculateSecondary();
        };

        AZStd::string ToConsoleString(const PerformanceStatistics& stats);

        class PerformanceStatistician
            : public PerformanceStatisticsEBus::Handler
            , public AZ::SystemTickBus::Handler
        {
        public:
            AZ_TYPE_INFO(PerformanceStatistician, "{3B93771A-B539-4F49-82E9-F15A75BFC703}");
            AZ_CLASS_ALLOCATOR(PerformanceStatistician, AZ::SystemAllocator);

            static void Reflect(AZ::ReflectContext* context);

            PerformanceStatistician();

            // PerformanceStatisticsEBus::Handler...
            void ClearSnaphotStatistics() override;
            void TrackAccumulatedStart(AZ::s32 tickCount) override;
            void TrackAccumulatedStop() override;
            void TrackPerFrameStart() override;
            void TrackPerFrameStop() override;
            ///

            AZStd::vector<AZStd::string> GetExecutedScriptsSinceLastSnapshot() const;

            const PerformanceStatistics& GetStatistics() const;

        protected:

            enum class TrackingState
            {
                None,
                AccumulatedInProgress,
                AccumulatedStartRequested,
                AccumulatedStopRequested,
                PerFrameStartRequested,
                PerFrameStopRequested,
                PerFrameInProgress,
            };

            TrackingState m_trackingState = TrackingState::None;
            AZ::s32 m_accumulatedTickCountRemaining = 0;
            AZStd::unordered_map<AZ::Data::AssetId, AZStd::string> m_executedScripts;
            AZStd::chrono::steady_clock::time_point m_accumulatedStartTime;
            PerformanceStatistics m_accumulatedStats;

            void ClearTrackingState();

            void ConnectToSystemTickBus();

            void OnStartTrackingRequested();

            void OnSystemTick() override;

            void UpdateAccumulatedStatistics();

            void UpdateAccumulatedTime();

            void UpdateStatisticsFromTracker();

            void UpdateTickCounts();
        };
    }
}
