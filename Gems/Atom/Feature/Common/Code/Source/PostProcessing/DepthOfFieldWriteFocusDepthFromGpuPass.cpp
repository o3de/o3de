/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <Atom/RHI/CommandList.h>
#include <PostProcessing/DepthOfFieldWriteFocusDepthFromGpuPass.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<DepthOfFieldWriteFocusDepthFromGpuPass> DepthOfFieldWriteFocusDepthFromGpuPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<DepthOfFieldWriteFocusDepthFromGpuPass> pass = aznew DepthOfFieldWriteFocusDepthFromGpuPass(descriptor);
            return pass;
        }

        DepthOfFieldWriteFocusDepthFromGpuPass::DepthOfFieldWriteFocusDepthFromGpuPass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        {
        }

        void DepthOfFieldWriteFocusDepthFromGpuPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            AZ_Assert(m_bufferRef != nullptr, "%s has a null buffer when calling Prepare.", GetPathName().GetCStr());

            ComputePass::SetupFrameGraphDependencies(frameGraph);

            RHI::BufferScopeAttachmentDescriptor desc;
            desc.m_attachmentId = m_bufferRef->GetAttachmentId();
            desc.m_bufferViewDescriptor = m_bufferRef->GetBufferViewDescriptor();
            desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::DontCare;
            frameGraph.UseShaderAttachment(desc, AZ::RHI::ScopeAttachmentAccess::Write);
        }

        void DepthOfFieldWriteFocusDepthFromGpuPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            AZ_Assert(m_shaderResourceGroup != nullptr, "%s has a null shader resource group when calling Compile.", GetPathName().GetCStr());

            m_shaderResourceGroup->SetConstant(m_autoFocusScreenPositionIndex, m_autoFocusScreenPosition);
            m_shaderResourceGroup->SetBufferView(m_autoFocusDataBufferIndex, m_bufferRef->GetBufferView());

            BindPassSrg(context, m_shaderResourceGroup);
            m_shaderResourceGroup->Compile();
        }

        void DepthOfFieldWriteFocusDepthFromGpuPass::SetScreenPosition(const AZ::Vector2& screenPosition)
        {
            m_autoFocusScreenPosition = screenPosition;
        }

        void DepthOfFieldWriteFocusDepthFromGpuPass::SetBufferRef(RPI::Ptr<RPI::Buffer> bufferRef)
        {
            m_bufferRef = bufferRef;
        }

        void DepthOfFieldWriteFocusDepthFromGpuPass::BuildAttachmentsInternal()
        {
            AZ_Assert(m_bufferRef != nullptr, "%s has a null buffer when calling BuildAttachmentsInternal.", GetPathName().GetCStr());

            AttachBufferToSlot(Name("DofDepthInputOutput"), m_bufferRef);
        }

    }   // namespace Render
}   // namespace AZ
