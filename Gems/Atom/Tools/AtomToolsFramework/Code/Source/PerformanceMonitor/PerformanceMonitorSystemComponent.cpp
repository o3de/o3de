/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <PerformanceMonitor/PerformanceMonitorSystemComponent.h>

namespace AtomToolsFramework
{
    void PerformanceMonitorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PerformanceMonitorService"));
    }

    void PerformanceMonitorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<PerformanceMonitorSystemComponent, AZ::Component>()->Version(0);
        }
    }

    void PerformanceMonitorSystemComponent::Init()
    {
    }

    void PerformanceMonitorSystemComponent::Activate()
    {
        AZ::TickBus::Handler::BusConnect();
        PerformanceMonitorRequestBus::Handler::BusConnect();
    }

    void PerformanceMonitorSystemComponent::Deactivate()
    {
        PerformanceMonitorRequestBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
    }

    void PerformanceMonitorSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (!m_profilingEnabled)
        {
            return;
        }

        if (++m_sample > SampleCount)
        {
            m_sample = 0;

            UpdateMetrics();
            ResetStats();
        }

        double frameTime = AZ::RHI::RHISystemInterface::Get()->GetCpuFrameTime();
        if (frameTime > 0)
        {
            m_cpuFrameTimeMs.PushSample(frameTime);
        }

        AZ::RHI::Ptr<AZ::RPI::ParentPass> rootPass = AZ::RPI::PassSystemInterface::Get()->GetRootPass();
        if (rootPass)
        {
            AZ::RPI::TimestampResult timestampResult = rootPass->GetLatestTimestampResult();
            double gpuFrameTimeMs = aznumeric_cast<double>(timestampResult.GetDurationInNanoseconds()) / 1000000;
            m_gpuFrameTimeMs.PushSample(gpuFrameTimeMs);
        }
    }

    void PerformanceMonitorSystemComponent::SetProfilerEnabled(bool enabled)
    {
        if (m_profilingEnabled == enabled)
        {
            return;
        }

        AZ::RHI::Ptr<AZ::RPI::ParentPass> rootPass = AZ::RPI::PassSystemInterface::Get()->GetRootPass();
        if (rootPass)
        {
            rootPass->SetTimestampQueryEnabled(enabled);
        }
        else
        {
            AZ_Error("PerformanceMonitorSystemComponent", false, "Failed to find root pass.");
        }

        if (enabled)
        {
            ResetStats();
        }

        m_profilingEnabled = enabled;
    }

    const PerformanceMetrics& PerformanceMonitorSystemComponent::GetMetrics()
    {
        UpdateMetrics();
        return m_metrics;
    }

    void PerformanceMonitorSystemComponent::UpdateMetrics()
    {
        m_metrics.m_cpuFrameTimeMs = m_cpuFrameTimeMs.GetAverage();
        m_metrics.m_gpuFrameTimeMs = m_gpuFrameTimeMs.GetAverage();
    }

    void PerformanceMonitorSystemComponent::ResetStats()
    {
        m_cpuFrameTimeMs.Reset();
        m_gpuFrameTimeMs.Reset();
    }
} // namespace AtomToolsFramework
