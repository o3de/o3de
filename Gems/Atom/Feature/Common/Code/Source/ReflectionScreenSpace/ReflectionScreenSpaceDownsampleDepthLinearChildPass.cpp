/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ReflectionScreenSpaceDownsampleDepthLinearChildPass.h"
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <SpecularReflections/SpecularReflectionsFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<ReflectionScreenSpaceDownsampleDepthLinearChildPass> ReflectionScreenSpaceDownsampleDepthLinearChildPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<ReflectionScreenSpaceDownsampleDepthLinearChildPass> pass = aznew ReflectionScreenSpaceDownsampleDepthLinearChildPass(descriptor);
            return AZStd::move(pass);
        }

        ReflectionScreenSpaceDownsampleDepthLinearChildPass::ReflectionScreenSpaceDownsampleDepthLinearChildPass(const RPI::PassDescriptor& descriptor)
            : RPI::FullscreenTrianglePass(descriptor)
        {
        }

        void ReflectionScreenSpaceDownsampleDepthLinearChildPass::FrameBeginInternal(FramePrepareParams params)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            SpecularReflectionsFeatureProcessorInterface* specularReflectionsFeatureProcessor = scene->GetFeatureProcessor<SpecularReflectionsFeatureProcessorInterface>();
            AZ_Assert(specularReflectionsFeatureProcessor, "ReflectionScreenSpaceDownsampleDepthLinearChildPass requires the SpecularReflectionsFeatureProcessor");

            // get attachment size
            RPI::PassAttachment* inputAttachment = GetInputBinding(0).GetAttachment().get();
            AZ_Assert(inputAttachment, "ReflectionScreenSpaceDownsampleDepthLinearChildPass: Input binding has no attachment!");

            RHI::Size size = inputAttachment->m_descriptor.m_image.m_size;
            bool halfResolution = specularReflectionsFeatureProcessor->GetSSROptions().m_halfResolution;

            if (m_imageSize != size || m_halfResolution != halfResolution)
            {
                m_imageSize = size;
                m_halfResolution = halfResolution;

                m_invOutputScale = (m_mipLevel == 0 && m_halfResolution) ? 2 : static_cast<float>(pow(2.0f, m_mipLevel));
                m_updateSrg = true;
            }

            float outputScale = 1.0f / m_invOutputScale;
            uint32_t outputWidth = static_cast<uint32_t>(m_imageSize.m_width * outputScale);
            uint32_t outputHeight = static_cast<uint32_t>(m_imageSize.m_height * outputScale);

            params.m_viewportState = RHI::Viewport(0, static_cast<float>(outputWidth), 0, static_cast<float>(outputHeight));
            params.m_scissorState = RHI::Scissor(0, 0, outputWidth, outputHeight);

            FullscreenTrianglePass::FrameBeginInternal(params);
        }

        void ReflectionScreenSpaceDownsampleDepthLinearChildPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            if (m_updateSrg)
            {
                m_shaderResourceGroup->SetConstant(m_invOutputScaleNameIndex, m_invOutputScale);
                m_shaderResourceGroup->SetConstant(m_mipLevelNameIndex, m_mipLevel);
                m_shaderResourceGroup->SetConstant(m_halfResolutionNameIndex, m_halfResolution);

                // Note: when processing mip0 both the mipLevel and previousMipLevel are set to 0
                uint32_t previousMipLevel = AZStd::max(0, aznumeric_cast<int32_t>(m_mipLevel) - 1);
                RHI::Size previousMipImageSize = m_mipLevel == 0 ? m_imageSize : m_imageSize.GetReducedMip(previousMipLevel);

                m_shaderResourceGroup->SetConstant(m_previousMipLevelNameIndex, previousMipLevel);
                m_shaderResourceGroup->SetConstant(m_previousMipWidthNameIndex, previousMipImageSize.m_width);
                m_shaderResourceGroup->SetConstant(m_previousMipHeightNameIndex, previousMipImageSize.m_height);

                m_updateSrg = false;
            }

            FullscreenTrianglePass::CompileResources(context);
        }

    }   // namespace RPI
}   // namespace AZ
