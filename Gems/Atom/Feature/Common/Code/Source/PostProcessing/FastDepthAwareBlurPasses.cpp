/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcessing/FastDepthAwareBlurPasses.h>
#include <AzCore/Math/MathUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace Render
    {
        // --- COMMON ---

        void FastDepthAwareBlurPassConstants::InitializeFromSize(RHI::Size outputTextureSize)
        {
            m_outputSize[0] = outputTextureSize.m_width;
            m_outputSize[1] = outputTextureSize.m_height;

            m_pixelSize[0] = 1.0f / float(outputTextureSize.m_width);
            m_pixelSize[1] = 1.0f / float(outputTextureSize.m_height);

            m_halfPixelSize[0] = 0.5f * m_pixelSize[0];
            m_halfPixelSize[1] = 0.5f * m_pixelSize[1];
        }

        void FastDepthAwareBlurPassConstants::SetConstants(float constFalloff, float depthFalloffThreshold, float depthFalloffStrength)
        {
            m_constFalloff = constFalloff;
            m_depthFalloffThreshold = depthFalloffThreshold;
            m_depthFalloffStrength = depthFalloffStrength;
        }

        // --- HORIZONTAL BLUR ---

        RPI::Ptr<FastDepthAwareBlurHorPass> FastDepthAwareBlurHorPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<FastDepthAwareBlurHorPass> pass = aznew FastDepthAwareBlurHorPass(descriptor);
            return AZStd::move(pass);
        }

        FastDepthAwareBlurHorPass::FastDepthAwareBlurHorPass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        {
            // Though this is a fullscreen pass, the algorithm used makes each thread output 3 blurred pixels, so
            // it's not a 1-to-1 ratio and requires custom calculation of target thread counts
            m_fullscreenDispatch = false;
        }

        void FastDepthAwareBlurHorPass::SetConstants(float constFalloff, float depthFalloffThreshold, float depthFalloffStrength)
        {
            m_passConstants.m_constFalloff = constFalloff;
            m_passConstants.m_depthFalloffThreshold = depthFalloffThreshold;
            m_passConstants.m_depthFalloffStrength = depthFalloffStrength;
        }

        void FastDepthAwareBlurHorPass::FrameBeginInternal(FramePrepareParams params)
        {
            AZ_Assert(GetOutputCount() > 0, "FastDepthAwareBlurHorPass: No output bindings!");
            RPI::PassAttachment* outputAttachment = GetOutputBinding(0).GetAttachment().get();

            AZ_Assert(outputAttachment != nullptr, "FastDepthAwareBlurHorPass: Output binding has no attachment!");
            RHI::Size size = outputAttachment->m_descriptor.m_image.m_size;

            m_passConstants.InitializeFromSize(size);

            m_shaderResourceGroup->SetConstant(m_constantsIndex, m_passConstants);

            // The algorithm has each thread output three pixels in the blur direction
            u32 targetThreadCountX = (size.m_width + 2) / 3;
            SetTargetThreadCounts(targetThreadCountX, size.m_height, 1);

            RPI::ComputePass::FrameBeginInternal(params);
        }

        // --- VERTICAL BLUR ---

        RPI::Ptr<FastDepthAwareBlurVerPass> FastDepthAwareBlurVerPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<FastDepthAwareBlurVerPass> pass = aznew FastDepthAwareBlurVerPass(descriptor);
            return AZStd::move(pass);
        }

        FastDepthAwareBlurVerPass::FastDepthAwareBlurVerPass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        {
            // Though this is a fullscreen pass, the algorithm used makes each thread output 3 blurred pixels, so
            // it's not a 1-to-1 ratio and requires custom calculation of target thread counts
            m_fullscreenDispatch = false;
        }

        void FastDepthAwareBlurVerPass::SetConstants(float constFalloff, float depthFalloffThreshold, float depthFalloffStrength)
        {
            m_passConstants.m_constFalloff = constFalloff;
            m_passConstants.m_depthFalloffThreshold = depthFalloffThreshold;
            m_passConstants.m_depthFalloffStrength = depthFalloffStrength;
        }

        void FastDepthAwareBlurVerPass::FrameBeginInternal(FramePrepareParams params)
        {
            AZ_Assert(GetOutputCount() > 0, "FastDepthAwareBlurVerPass: No output bindings!");
            RPI::PassAttachment* outputAttachment = GetOutputBinding(0).GetAttachment().get();

            AZ_Assert(outputAttachment != nullptr, "FastDepthAwareBlurVerPass: Output binding has no attachment!");
            RHI::Size size = outputAttachment->m_descriptor.m_image.m_size;

            m_passConstants.InitializeFromSize(size);

            m_shaderResourceGroup->SetConstant(m_constantsIndex, m_passConstants);

            // The algorithm has each thread output three pixels in the blur direction
            u32 targetThreadCountY = (size.m_height + 2) / 3;
            SetTargetThreadCounts(size.m_width, targetThreadCountY, 1);

            RPI::ComputePass::FrameBeginInternal(params);
        }

    }   // namespace Render
}   // namespace AZ
