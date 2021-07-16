/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/PostProcessFeatureProcessor.h>
#include <PostProcessing/DepthUpsamplePass.h>
#include <AzCore/Math/MathUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<DepthUpsamplePass> DepthUpsamplePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<DepthUpsamplePass> pass = aznew DepthUpsamplePass(descriptor);
            return AZStd::move(pass);
        }
        
        DepthUpsamplePass::DepthUpsamplePass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        { }
        
        void DepthUpsamplePass::FrameBeginInternal(FramePrepareParams params)
        {
            // Must match the struct in DepthUpsample.azsl
            struct UpsampleConstants
            {
                // The size of a pixel in the input image relative to screenspace UV
                // Calculated by taking the inverse of the texture dimensions
                AZStd::array<float, 2> m_inputPixelSize;

                // The size of a pixel in the output image relative to screenspace UV
                // Calculated by taking the inverse of the texture dimensions
                AZStd::array<float, 2> m_outputPixelSize;

            } upsampleConstants{};

            AZ_Assert(GetInputCount() == 3, "DepthUpsamplePass requires three inputs!");
            AZ_Assert(GetOutputCount() == 1, "DepthUpsamplePass requires one output!");
            RPI::PassAttachment* inputAttachment = GetInputBinding(2).m_attachment.get();
            RPI::PassAttachment* outputAttachment = GetOutputBinding(0).m_attachment.get();

            AZ_Assert(inputAttachment != nullptr, "DepthUpsamplePass: Input binding has no attachment!");
            AZ_Assert(outputAttachment != nullptr, "DepthUpsamplePass: Output binding has no attachment!");
            RHI::Size inputSize = inputAttachment->m_descriptor.m_image.m_size;
            RHI::Size outputSize = outputAttachment->m_descriptor.m_image.m_size;

            upsampleConstants.m_inputPixelSize[0] = 1.0f / float(inputSize.m_width);
            upsampleConstants.m_inputPixelSize[1] = 1.0f / float(inputSize.m_height);
            upsampleConstants.m_outputPixelSize[0] = 1.0f / float(outputSize.m_width);
            upsampleConstants.m_outputPixelSize[1] = 1.0f / float(outputSize.m_height);

            // TargetThreadCount has same dimensions as the input image + 1
            // This +1 only applies if the dimension is an even number
            // For a more detailed explanation, see the Algorithm Overview section in DepthUpsample.azsl
            u32 targetThreadCountX = inputSize.m_width;
            u32 targetThreadCountY = inputSize.m_height;

            if ((outputSize.m_width & 1) == 0)
            {
                ++targetThreadCountX;
            }
            if ((outputSize.m_height & 1) == 0)
            {
                ++targetThreadCountY;
            }

            SetTargetThreadCounts(targetThreadCountX, targetThreadCountY, 1);

            m_shaderResourceGroup->SetConstant(m_constantsIndex, upsampleConstants);
        
            RPI::ComputePass::FrameBeginInternal(params);
        }

    }   // namespace Render
}   // namespace AZ
