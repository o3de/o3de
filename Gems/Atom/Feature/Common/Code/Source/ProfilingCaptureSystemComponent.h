/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>

#include <Atom/Feature/Utils/ProfilingCaptureBus.h>

namespace AZ
{
    namespace RPI
    {
        class Pass;
    }

    namespace Render
    {
        //! Helper class that executes a function when a certain amount of frames have passed.
        class DelayedQueryCaptureHelper
        {
            using CaptureCallback = AZStd::function<void()>;

            enum DelayedCaptureState
            {
                Idle,
                Pending
            };

            // Wait 6 frames before calling the callback.
            const uint32_t FrameThreshold = 6u;

        public:
            //! Starts a delayed capture.
            bool StartCapture(CaptureCallback&& saveCallback);

            //! Decrements the threshold value, and executes the functor when the threshold reaches 0.
            void UpdateCapture();

            //! Returns whether the state is idle.
            bool IsIdle() const;

        private:
            uint32_t m_frameThreshold = 0u;
            DelayedCaptureState m_state = DelayedCaptureState::Idle;
            CaptureCallback m_captureCallback;
        };

        //! System component to handle ProfilingCaptureSystemComponent.
        class ProfilingCaptureSystemComponent final
            : public AZ::Component
            , public ProfilingCaptureRequestBus::Handler
            , public AZ::TickBus::Handler
        {
        public:
            AZ_COMPONENT(ProfilingCaptureSystemComponent, "{B715C113-E697-41D3-87BF-27D0ED1055BA}");

            static void Reflect(AZ::ReflectContext* context);

            void Activate() override;
            void Deactivate() override;

            // ProfilingCaptureRequestBus overrides...
            bool CapturePassTimestamp(const AZStd::string& outputFilePath) override;
            bool CaptureCpuFrameTime(const AZStd::string& outputFilePath) override;
            bool CapturePassPipelineStatistics(const AZStd::string& outputFilePath) override;
            bool CaptureBenchmarkMetadata(const AZStd::string& benchmarkName, const AZStd::string& outputFilePath) override;

        private:
            void OnTick(float deltaTime, ScriptTimePoint time) override;

            // Recursively collect all the passes from the root pass.
            AZStd::vector<const RPI::Pass*> CollectPassesRecursively(const RPI::Pass* root) const;

            DelayedQueryCaptureHelper m_timestampCapture;
            DelayedQueryCaptureHelper m_cpuFrameTimeStatisticsCapture;
            DelayedQueryCaptureHelper m_pipelineStatisticsCapture;
            DelayedQueryCaptureHelper m_benchmarkMetadataCapture;
        };
    }
}
