/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/FrameEventBus.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/FeatureProcessor.h>

#include <AzCore/std/containers/unordered_map.h>

namespace AZ
{
    namespace RHI
    {
        class ScopeProducer;
    }

    namespace RPI
    {
        class Pass;
    }
} // namespace AZ

namespace AFR
{
    class AFRFeatureProcessor
        : public AZ::RPI::FeatureProcessor
        , public AZ::RHI::FrameEventBus::Handler
        , public AZ::RHI::RHISystemNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(AFRFeatureProcessor, AZ::SystemAllocator)
        AZ_RTTI(AFR::AFRFeatureProcessor, "{78F458F3-E68D-4390-86B6-9154C4AAFE4E}", AZ::RPI::FeatureProcessor);
        AZ_DISABLE_COPY_MOVE(AFRFeatureProcessor);
        AZ_FEATURE_PROCESSOR(AFRFeatureProcessor);

        static void Reflect(AZ::ReflectContext* context);

        AFRFeatureProcessor() = default;
        ~AFRFeatureProcessor() = default;

        //! AZ::RPI::FeatureProcessor overrides...
        void Activate() override;
        void Deactivate() override;

        //! FrameEventBus::Handler
        //! Called just after the frame scheduler begins a frame.
        void OnFrameBegin() override;

        //! called from RenderPipeline::OnPassModified, when a Pipeline is added/removed or Passes are added/removed
        void OnRenderPipelineChanged(AZ::RPI::RenderPipeline* renderPipeline, RenderPipelineChangeType pipelineChangeType) override;

    private:
        enum class AFRSetupState
        {
            NOT_INITIALIZED,
            INITIALIZING,
            SETUP_DONE,
            SCHEDULING
        };

        //! AFR Methods
        void addAFRCopyPass(AZ::RPI::RenderPipeline* renderPipeline, int deviceIndex);
        void scheduleRTASP(AZ::RPI::RenderPipeline* renderPipeline);
        void CollectPassesToSchedule(AZ::RPI::RenderPipeline* renderPipeline);

        //! AFR members
        int m_deviceCount{};
        AZStd::vector<AZ::RHI::Ptr<AZ::RPI::Pass>> m_scheduledPasses;
        AZStd::unordered_map<int, AZ::RHI::Ptr<AZ::RPI::Pass>> m_afrCopyPasses;
        AFRSetupState m_afrSetupState{AFRFeatureProcessor::AFRSetupState::NOT_INITIALIZED};
        AZStd::pair<AZ::RPI::RenderPipelineId, AZ::RPI::RenderPipeline*> m_afrRenderPipeline;
        AZStd::string m_afrPipelineName;
    };
} // namespace AFR
