/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcessing/EyeAdaptationPass.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/DevicePipelineState.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <PostProcess/PostProcessFeatureProcessor.h>
#include <PostProcess/ExposureControl/ExposureControlSettings.h>

namespace AZ
{
    namespace Render
    {
        static const char* const EyeAdaptationBufferName = "EyeAdaptationBuffer";

        RPI::Ptr<EyeAdaptationPass> EyeAdaptationPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<EyeAdaptationPass> pass = aznew EyeAdaptationPass(descriptor);
            return pass;
        }

        EyeAdaptationPass::EyeAdaptationPass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        {
        }
        
        void EyeAdaptationPass::InitBuffer()
        {
            ExposureCalculationData defaultData;
            RPI::CommonBufferDescriptor desc;
            desc.m_poolType = RPI::CommonBufferPoolType::ReadWrite;
            desc.m_bufferName = EyeAdaptationBufferName;
            desc.m_byteCount = sizeof(ExposureCalculationData);
            desc.m_elementSize = aznumeric_cast<uint32_t>(desc.m_byteCount);
            desc.m_bufferData = &defaultData;

            m_buffer = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
        }

        void EyeAdaptationPass::BuildInternal()
        {
            if (!m_buffer)
            {
                InitBuffer();
            }

            AttachBufferToSlot(EyeAdaptationDataInputOutputSlotName, m_buffer);
        }

        bool EyeAdaptationPass::IsEnabled() const
        {
            if (!ComputePass::IsEnabled() || m_pipeline == nullptr)
            {
                return false;
            }

            AZ::RPI::Scene* scene = GetScene();
            bool enabled = false;

            if (scene)
            {
                PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
                AZ::RPI::ViewPtr view = GetRenderPipeline()->GetFirstView(GetPipelineViewTag());
                if (fp)
                {
                    PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                    if (postProcessSettings)
                    {
                        ExposureControlSettings* settings = postProcessSettings->GetExposureControlSettings();
                        if (settings)
                        {
                            enabled = true;
                        }
                    }
                }
            }

            return enabled;
        }


        void EyeAdaptationPass::FrameBeginInternal(FramePrepareParams params)
        {
            AZ::RPI::ComputePass::FrameBeginInternal(params);

            AZ::RPI::Scene* scene = GetScene();
            if (scene)
            {
                PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
                if (fp)
                {
                    AZ::RPI::ViewPtr view = GetRenderPipeline()->GetFirstView(GetPipelineViewTag());
                    PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                    if (postProcessSettings)
                    {
                        ExposureControlSettings* settings = postProcessSettings->GetExposureControlSettings();
                        if (settings)
                        {
                            settings->UpdateBuffer();
                            view->GetShaderResourceGroup()->SetBufferView(m_exposureControlBufferInputIndex, settings->GetBufferView());
                        }
                    }
                }
            }
        }
    }   // namespace Render
}   // namespace AZ
