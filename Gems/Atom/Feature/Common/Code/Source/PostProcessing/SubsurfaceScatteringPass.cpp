/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcessing/SubsurfaceScatteringPass.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameScheduler.h>

#include <Atom/RPI.Reflect/Pass/PassTemplate.h>

#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/Pass/PassAttachment.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace RPI
    {
        Ptr<SubsurfaceScatteringPass> SubsurfaceScatteringPass::Create(const PassDescriptor& descriptor)
        {
            Ptr<SubsurfaceScatteringPass> pass = aznew SubsurfaceScatteringPass(descriptor);
            return pass;
        }

        SubsurfaceScatteringPass::SubsurfaceScatteringPass(const PassDescriptor& descriptor)
            : ComputePass(descriptor)
        { }

        void SubsurfaceScatteringPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            RHI::Size targetImageSize;
            if (m_fullscreenDispatch)
            {
                auto attachment = m_fullscreenSizeSourceBinding->GetAttachment();
                if (attachment)
                {
                    auto imageDescriptor = context.GetImageDescriptor(attachment->GetAttachmentId());
                    targetImageSize = imageDescriptor.m_size;
                }
            }

            // Update shader constant
            m_shaderResourceGroup->SetConstant(m_screenSizeInputIndex, AZ::Vector2(static_cast<float>(targetImageSize.m_width), static_cast<float>(targetImageSize.m_height)));

            ComputePass::CompileResources(context);
        }
    }   // namespace RPI
}   // namespace AZ
