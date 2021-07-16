/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI/CommandList.h>
#include <Atom/RPI.Public/Buffer/BufferSystemInterface.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <PostProcessing/DepthOfFieldCopyFocusDepthToCpuPass.h>

namespace AZ
{
    namespace Render
    {

        RPI::Ptr<DepthOfFieldCopyFocusDepthToCpuPass> DepthOfFieldCopyFocusDepthToCpuPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<DepthOfFieldCopyFocusDepthToCpuPass> depthOfFieldCopyFocusDepthToCpuPass = aznew DepthOfFieldCopyFocusDepthToCpuPass(descriptor);
            return depthOfFieldCopyFocusDepthToCpuPass;
        }

        DepthOfFieldCopyFocusDepthToCpuPass::DepthOfFieldCopyFocusDepthToCpuPass(const RPI::PassDescriptor& descriptor)
            : Pass(descriptor)
        {
        }

        void DepthOfFieldCopyFocusDepthToCpuPass::SetBufferRef(RPI::Ptr<RPI::Buffer> bufferRef)
        {
            m_bufferRef = bufferRef;
        }

        float DepthOfFieldCopyFocusDepthToCpuPass::GetFocusDepth()
        {
            float depth = 0.0f;
            if (m_readbackBuffer)
            {
                void* buf = m_readbackBuffer->Map(m_copyDescriptor.m_size, 0);
                if (buf)
                {
                    memcpy(&depth, buf, sizeof(depth));
                    m_readbackBuffer->Unmap();
                }
            }
            return depth;
        }

        void DepthOfFieldCopyFocusDepthToCpuPass::BuildInternal()
        {
            SetScopeId(RHI::ScopeId(GetPathName()));
        }

        void DepthOfFieldCopyFocusDepthToCpuPass::FrameBeginInternal(FramePrepareParams params)
        {
            AZ_Assert(m_bufferRef != nullptr, "%s has a null buffer when calling FrameBeginInternal.", GetPathName().GetCStr());

            if (m_needsInitialize)
            {
                RPI::CommonBufferDescriptor desc;
                desc.m_bufferName = GetPathName().GetStringView();
                desc.m_poolType = RPI::CommonBufferPoolType::ReadBack;
                desc.m_byteCount = sizeof(float);
                desc.m_elementSize = aznumeric_cast<uint32_t>(desc.m_byteCount);
                desc.m_bufferData = nullptr;
                m_readbackBuffer = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);

                m_copyDescriptor.m_sourceBuffer = m_bufferRef->GetRHIBuffer();
                m_copyDescriptor.m_sourceOffset = 0;
                m_copyDescriptor.m_destinationBuffer = m_readbackBuffer->GetRHIBuffer();
                m_copyDescriptor.m_destinationOffset = 0;
                m_copyDescriptor.m_size = sizeof(float);

                m_needsInitialize = false;
            }

            params.m_frameGraphBuilder->ImportScopeProducer(*this);
        }

        void DepthOfFieldCopyFocusDepthToCpuPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            AZ_Assert(m_bufferRef != nullptr, "%s has a null buffer when calling Prepare.", GetPathName().GetCStr());

            RHI::BufferScopeAttachmentDescriptor desc;
            desc.m_attachmentId = m_bufferRef->GetAttachmentId();
            desc.m_bufferViewDescriptor = m_bufferRef->GetBufferViewDescriptor();
            desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::DontCare;
            frameGraph.UseCopyAttachment(desc, AZ::RHI::ScopeAttachmentAccess::Read);
        }

        void DepthOfFieldCopyFocusDepthToCpuPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            AZ_UNUSED(context);
        }

        void DepthOfFieldCopyFocusDepthToCpuPass::BuildCommandList(const RHI::FrameGraphExecuteContext& context)
        {
            context.GetCommandList()->Submit(m_copyDescriptor);
        }

    }   // namespace RPI
}   // namespace AZ
