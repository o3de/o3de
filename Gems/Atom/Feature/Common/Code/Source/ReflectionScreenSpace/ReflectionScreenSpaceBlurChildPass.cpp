/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ReflectionScreenSpaceBlurChildPass.h"
#include <Atom/RHI/FrameGraphBuilder.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RPI.Reflect/Pass/FullscreenTrianglePassData.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<ReflectionScreenSpaceBlurChildPass> ReflectionScreenSpaceBlurChildPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<ReflectionScreenSpaceBlurChildPass> pass = aznew ReflectionScreenSpaceBlurChildPass(descriptor);
            return AZStd::move(pass);
        }

        ReflectionScreenSpaceBlurChildPass::ReflectionScreenSpaceBlurChildPass(const RPI::PassDescriptor& descriptor)
            : RPI::FullscreenTrianglePass(descriptor)
        {
        }

        void ReflectionScreenSpaceBlurChildPass::FrameBeginInternal(FramePrepareParams params)
        {
            // get attachment size
            RPI::PassAttachment* inputAttachment = GetInputOutputBinding(0).m_attachment.get();
            AZ_Assert(inputAttachment, "ReflectionScreenSpaceBlurChildPass: Input binding has no attachment!");

            RHI::Size size = inputAttachment->m_descriptor.m_image.m_size;

            if (m_imageSize != size)
            {
                m_imageSize = size;
                m_outputScale = (m_passType == PassType::Vertical) ? pow(2.0f, m_mipLevel) : 1.0f;

                m_updateSrg = true;
            }

            float inverseScale = 1.0f / m_outputScale;
            uint32_t outputWidth = m_imageSize.m_width * inverseScale;
            uint32_t outputHeight = m_imageSize.m_height * inverseScale;

            params.m_viewportState = RHI::Viewport(0, static_cast<float>(outputWidth), 0, static_cast<float>(outputHeight));
            params.m_scissorState = RHI::Scissor(0, 0, outputWidth, outputHeight);

            FullscreenTrianglePass::FrameBeginInternal(params);
        }

        void ReflectionScreenSpaceBlurChildPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            if (m_updateSrg)
            {
                m_shaderResourceGroup->SetConstant(m_imageWidthIndex, m_imageSize.m_width);
                m_shaderResourceGroup->SetConstant(m_imageHeightIndex, m_imageSize.m_height);
                m_shaderResourceGroup->SetConstant(m_outputScaleIndex, m_outputScale);

                m_updateSrg = false;
            }

            FullscreenTrianglePass::CompileResources(context);
        }

    }   // namespace RPI
}   // namespace AZ
