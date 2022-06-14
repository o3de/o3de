/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Pass/Pass.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Statistics/RunningStatistic.h>
#include <AtomToolsFramework/PerformanceMonitor/PerformanceMonitorRequestBus.h>

namespace AtomToolsFramework
{
    //! PerformanceMonitorSystemComponent monitors performance
    class PerformanceMonitorSystemComponent
        : public AZ::Component
        , private PerformanceMonitorRequestBus::Handler
        , private AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(PerformanceMonitorSystemComponent, "{C2F54D1B-A106-4922-82BE-ACB7A168D4AF}");

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void Reflect(AZ::ReflectContext* context);

        PerformanceMonitorSystemComponent() = default;
        ~PerformanceMonitorSystemComponent() = default;

    private:
        // AZ::Component overrides...
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // AZ::TickBus::Handler overrides...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // PerformanceMonitorRequestBus::Handler overrides...
        void SetProfilerEnabled(bool enabled) override;
        const PerformanceMetrics& GetMetrics() override;

        void UpdateMetrics();
        void ResetStats();

        bool m_profilingEnabled = false;

        AZ::Statistics::RunningStatistic m_cpuFrameTimeMs;
        AZ::Statistics::RunningStatistic m_gpuFrameTimeMs;

        PerformanceMetrics m_metrics;

        // Number of samples to average for each metric
        static constexpr int SampleCount = 10;
        int m_sample = 0;
    };
} // namespace AtomToolsFramework
