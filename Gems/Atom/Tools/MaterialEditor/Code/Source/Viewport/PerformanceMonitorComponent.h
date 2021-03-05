/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Statistics/RunningStatistic.h>

#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/Viewport/PerformanceMonitorRequestBus.h>

namespace MaterialEditor
{
    //! PerformanceMonitorComponent monitors performance within Material Editor
    class PerformanceMonitorComponent
        : public AZ::Component
        , private PerformanceMonitorRequestBus::Handler
    {
    public:
        AZ_COMPONENT(PerformanceMonitorComponent, "{C2F54D1B-A106-4922-82BE-ACB7A168D4AF}");

        static void Reflect(AZ::ReflectContext* context);

        PerformanceMonitorComponent();
        ~PerformanceMonitorComponent() = default;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

    private:
        // AZ::Component overrides...
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // PerformanceMonitorRequestBus::Handler interface overrides...
        void SetProfilerEnabled(bool enabled) override;
        void GatherMetrics() override;
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
}
