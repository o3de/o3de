/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Mesh/GpuDriven/GpuCullStatsReadbackPass.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/Buffer/BufferSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassAttachment.h>

namespace AZ::Render
{
    RPI::Ptr<GpuCullStatsReadbackPass> GpuCullStatsReadbackPass::Create(const RPI::PassDescriptor& descriptor)
    {
        return aznew GpuCullStatsReadbackPass(descriptor);
    }

    GpuCullStatsReadbackPass::GpuCullStatsReadbackPass(const RPI::PassDescriptor& descriptor)
        : Pass(descriptor)
    {
        m_fence = aznew RHI::Fence;
        [[maybe_unused]] const RHI::ResultCode result = m_fence->Init(RHI::MultiDevice::AllDevices, RHI::FenceState::Reset);
        AZ_Assert(result == RHI::ResultCode::Success, "GpuCullStatsReadbackPass failed to init fence");
    }

    void GpuCullStatsReadbackPass::BuildInternal()
    {
        m_survivorCountBinding = FindAttachmentBinding(Name("SurvivorCount"));
        InitScope(RHI::ScopeId(GetPathName()));
    }

    void GpuCullStatsReadbackPass::FrameBeginInternal(FramePrepareParams params)
    {
        if (m_needsInitialize)
        {
            RPI::CommonBufferDescriptor desc;
            desc.m_bufferName = GetPathName().GetStringView();
            desc.m_poolType = RPI::CommonBufferPoolType::ReadBack;
            desc.m_byteCount = sizeof(uint32_t);
            desc.m_elementSize = aznumeric_cast<uint32_t>(desc.m_byteCount);
            desc.m_bufferData = nullptr;
            m_readbackBuffer = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
            m_needsInitialize = false;
        }

        // Only participate in the frame graph once the source attachment is wired up.
        if (m_survivorCountBinding && m_survivorCountBinding->GetAttachment() && m_readbackBuffer)
        {
            params.m_frameGraphBuilder->ImportScopeProducer(*this);
        }
    }

    void GpuCullStatsReadbackPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
    {
        if (!m_survivorCountBinding || !m_survivorCountBinding->GetAttachment())
        {
            return;
        }

        RHI::BufferScopeAttachmentDescriptor desc;
        desc.m_attachmentId = m_survivorCountBinding->GetAttachment()->GetAttachmentId();
        desc.m_bufferViewDescriptor = m_survivorCountBinding->GetAttachment()->m_descriptor.m_bufferView;
        desc.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::DontCare;
        frameGraph.UseCopyAttachment(desc, RHI::ScopeAttachmentAccess::Read);
        frameGraph.SignalFence(*m_fence);
    }

    void GpuCullStatsReadbackPass::CompileResources(const RHI::FrameGraphCompileContext& context)
    {
        m_copyDescriptor.m_sourceBuffer = nullptr;
        if (!m_survivorCountBinding || !m_survivorCountBinding->GetAttachment() || !m_readbackBuffer)
        {
            return;
        }

        const RHI::AttachmentId attachmentId = m_survivorCountBinding->GetAttachment()->GetAttachmentId();
        const RHI::Buffer* sourceBuffer = context.GetBuffer(attachmentId);
        if (sourceBuffer == nullptr)
        {
            return;
        }

        m_copyDescriptor.m_sourceBuffer = const_cast<RHI::Buffer*>(sourceBuffer);
        m_copyDescriptor.m_sourceOffset = 0;
        m_copyDescriptor.m_destinationBuffer = m_readbackBuffer->GetRHIBuffer();
        m_copyDescriptor.m_destinationOffset = 0;
        m_copyDescriptor.m_size = sizeof(uint32_t);
    }

    void GpuCullStatsReadbackPass::BuildCommandList(const RHI::FrameGraphExecuteContext& context)
    {
        if (m_copyDescriptor.m_sourceBuffer == nullptr)
        {
            return;
        }

        const auto deviceIndex = context.GetDeviceIndex();
        m_fence->GetDeviceFence(deviceIndex)->WaitOnCpuAsync(
            [this, deviceIndex]()
            {
                if (m_readbackBuffer)
                {
                    auto buf = m_readbackBuffer->Map(sizeof(uint32_t), 0);
                    if (buf[deviceIndex] != nullptr)
                    {
                        uint32_t value = 0;
                        memcpy(&value, buf[deviceIndex], sizeof(uint32_t));
                        m_lastVisibleCount = value;
                        m_readbackBuffer->Unmap();
                    }
                }
                m_fence->Reset();
            });

        context.GetCommandList()->Submit(m_copyDescriptor.GetDeviceCopyBufferDescriptor(context.GetDeviceIndex()));
    }
} // namespace AZ::Render
