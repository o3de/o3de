/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI.Reflect/Format.h>

#include <Atom/RPI.Public/Pass/AttachmentReadback.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/Pass/Specific/RenderToTexturePass.h>

namespace AZ
{
    namespace RPI
    {
        Ptr<RenderToTexturePass> RenderToTexturePass::Create(const PassDescriptor& descriptor)
        {
            Ptr<RenderToTexturePass> pass = aznew RenderToTexturePass(descriptor);
            return pass;
        }

        Ptr<ParentPass> RenderToTexturePass::Recreate() const
        {
            PassDescriptor desc = GetPassDescriptor();
            Ptr<ParentPass> pass = aznew RenderToTexturePass(desc);
            return pass;
        }

        RenderToTexturePass::RenderToTexturePass(const PassDescriptor& descriptor)
            : ParentPass(descriptor)
        {
            // Save the pass data for easier access
            const RPI::RenderToTexturePassData* passData = RPI::PassUtils::GetPassData<RPI::RenderToTexturePassData>(descriptor);
            if (passData)
            {
                m_passData = *passData;
                OnUpdateOutputSize();
            }
        }

        RenderToTexturePass::~RenderToTexturePass()
        {
        }
        
        void RenderToTexturePass::BuildInternal()
        {
            m_outputAttachment = aznew PassAttachment();
            m_outputAttachment->m_name = "RenderTarget";
            m_outputAttachment->ComputePathName(GetPathName());

            RHI::ImageDescriptor outputImageDesc;
            outputImageDesc.m_bindFlags = RHI::ImageBindFlags::Color | RHI::ImageBindFlags::ShaderRead | RHI::ImageBindFlags::CopyWrite;
            outputImageDesc.m_size.m_width = m_passData.m_width;
            outputImageDesc.m_size.m_height = m_passData.m_height;
            outputImageDesc.m_format = m_passData.m_format;
            m_outputAttachment->m_descriptor = outputImageDesc;

            m_ownedAttachments.push_back(m_outputAttachment);

            PassAttachmentBinding outputBinding;
            outputBinding.m_name = "Output";
            outputBinding.m_slotType = PassSlotType::Output;
            outputBinding.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::RenderTarget;
            outputBinding.SetAttachment(m_outputAttachment);

            AddAttachmentBinding(outputBinding);
            
            Base::BuildInternal();
        }

        void RenderToTexturePass::FrameBeginInternal(FramePrepareParams params)
        {
            params.m_scissorState = m_scissor;
            params.m_viewportState = m_viewport;

            Base::FrameBeginInternal(params);
        }

        void RenderToTexturePass::ResizeOutput(uint32_t width, uint32_t height)
        {
            m_passData.m_width = width;
            m_passData.m_height = height;
            OnUpdateOutputSize();
            QueueForBuildAndInitialization();
        }

        void RenderToTexturePass::OnUpdateOutputSize()
        {
            // update scissor and viewport when output size changed
            m_scissor.m_minX = 0;
            m_scissor.m_maxX = m_passData.m_width;
            m_scissor.m_minY = 0;
            m_scissor.m_maxY = m_passData.m_height;
            
            m_viewport.m_minX = 0;
            m_viewport.m_maxX = aznumeric_cast<float>(m_passData.m_width);
            m_viewport.m_minX = 0;
            m_viewport.m_maxY = aznumeric_cast<float>(m_passData.m_height);
            m_viewport.m_minZ = 0;
            m_viewport.m_maxZ = 1;
        }

        void RenderToTexturePass::ReadbackOutput(AZStd::shared_ptr<AttachmentReadback> readback)
        {
            if (m_outputAttachment)
            {
                m_readbackOption = PassAttachmentReadbackOption::Output;
                m_attachmentReadback = readback;
                AZStd::string readbackName = AZStd::string::format("%s_%s", m_outputAttachment->GetAttachmentId().GetCStr(), GetName().GetCStr());
                m_attachmentReadback->ReadPassAttachment(m_outputAttachment.get(), AZ::Name(readbackName));
            }
        }

    }   // namespace RPI
}   // namespace AZ
