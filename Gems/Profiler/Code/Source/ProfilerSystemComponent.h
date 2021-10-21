/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Profiler/ProfilerBus.h>
#include <CpuProfilerImpl.h>

#include <AzCore/Component/Component.h>
#include <AzCore/std/parallel/thread.h>

namespace Profiler
{
    class ProfilerSystemComponent
        : public AZ::Component
        , protected ProfilerRequestBus::Handler
    {
    public:
        AZ_COMPONENT(ProfilerSystemComponent, "{3f52c1d7-d920-4781-8ed7-88077ec4f305}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        ProfilerSystemComponent();
        ~ProfilerSystemComponent();

    protected:
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        // ProfilerRequestBus interface implementation
        void SetProfilerEnabled(bool enabled) override;
        bool CaptureCpuProfilingStatistics(const AZStd::string& outputFilePath) override;
        bool BeginContinuousCpuProfilingCapture() override;
        bool EndContinuousCpuProfilingCapture(const AZStd::string& outputFilePath) override;


        AZStd::thread m_cpuDataSerializationThread;
        AZStd::atomic_bool m_cpuDataSerializationInProgress{ false };

        AZStd::atomic_bool m_cpuCaptureInProgress{ false };

        CpuProfilerImpl m_cpuProfiler;
    };

} // namespace Profiler
