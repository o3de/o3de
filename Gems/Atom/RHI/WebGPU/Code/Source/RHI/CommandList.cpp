/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/Buffer.h>
#include <RHI/BindGroup.h>
#include <RHI/CommandList.h>
#include <RHI/ComputePipeline.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/PipelineState.h>
#include <RHI/RenderPipeline.h>
#include <RHI/ShaderResourceGroup.h>

namespace AZ::WebGPU
{
    RHI::Ptr<CommandList> CommandList::Create()
    {
        return aznew CommandList();
    }

    void CommandList::Init(Device& device)
    {
        RHI::DeviceObject::Init(device);
    }

    void CommandList::Begin()
    {
        AZ_Assert(!m_wgpuCommandEncoder, "Command encoder already created");
        m_wgpuCommandEncoder = static_cast<Device&>(GetDevice()).GetNativeDevice().CreateCommandEncoder();
        m_wgpuCommandEncoder.SetLabel(GetName().GetCStr());
        AZ_Assert(m_wgpuCommandEncoder, "Failed to create command encoder");
    }

    void CommandList::End()
    {
        AZ_Assert(m_wgpuCommandEncoder, "Command encoder has not been created");
        AZ_Assert(!m_wgpuCommandBuffer, "Command buffer already created");
        m_wgpuCommandBuffer = m_wgpuCommandEncoder.Finish();
        AZ_Assert(m_wgpuCommandBuffer, "Failed to create command buffer");
    }

    void CommandList::BeginRenderPass(const wgpu::RenderPassDescriptor& descriptor)
    {
        AZ_Assert(m_wgpuCommandEncoder, "Command encoder has not been created");
        AZ_Assert(!m_state.m_wgpuComputePassEncoder, "Compute encoder already created");
        m_state.m_wgpuRenderPassEncoder = m_wgpuCommandEncoder.BeginRenderPass(&descriptor);
        AZ_Assert(m_state.m_wgpuRenderPassEncoder, "Failed to begin render pass");
    }

    void CommandList::EndRenderPass()
    {
        AZ_Assert(m_state.m_wgpuRenderPassEncoder, "RenderPass encoder has not been created");
        m_state.m_wgpuRenderPassEncoder.End();
        m_state = State();
    }

    void CommandList::BeginComputePass()
    {
        AZ_Assert(m_wgpuCommandEncoder, "Command encoder has not been created");
        AZ_Assert(!m_state.m_wgpuRenderPassEncoder, "Render encoder already created");
        m_state.m_wgpuComputePassEncoder = m_wgpuCommandEncoder.BeginComputePass();
        AZ_Assert(m_state.m_wgpuComputePassEncoder, "Failed to begin compute pass");
    }

    void CommandList::EndComputePass()
    {
        AZ_Assert(m_state.m_wgpuComputePassEncoder, "ComputePass encoder has not been created");
        m_state.m_wgpuComputePassEncoder.End();
        m_state = State();
    }

    wgpu::CommandBuffer& CommandList::GetNativeCommandBuffer()
    {
        return m_wgpuCommandBuffer;
    }

    void CommandList::SetViewports(const RHI::Viewport* viewports, uint32_t count)
    {
        m_state.m_viewportState.Set(AZStd::span<const RHI::Viewport>(viewports, count));
    }

    void CommandList::SetScissors(const RHI::Scissor* scissors, uint32_t count)
    {
        m_state.m_scissorState.Set(AZStd::span<const RHI::Scissor>(scissors, count));
    }

    void CommandList::SetShaderResourceGroupForDraw(const RHI::DeviceShaderResourceGroup& shaderResourceGroup)
    {
        SetShaderResourceGroup(shaderResourceGroup, RHI::PipelineStateType::Draw);
    }

    void CommandList::SetShaderResourceGroupForDispatch(const RHI::DeviceShaderResourceGroup& shaderResourceGroup)
    {
        SetShaderResourceGroup(shaderResourceGroup, RHI::PipelineStateType::Dispatch);
    }

    void CommandList::Submit(const RHI::DeviceDrawItem& drawItem, uint32_t submitIndex)
    {
        ValidateSubmitIndex(submitIndex);

        if (!CommitShaderResource(drawItem))
        {
            AZ_Warning("CommandList", false, "Failed to bind shader resources for draw item. Skipping Draw Item.");
            return;
        }

        SetStencilRef(drawItem.m_stencilRef);
        SetStreamBuffers(drawItem.m_streamBufferViews, drawItem.m_streamBufferViewCount);

        RHI::CommandListScissorState scissorState;
        if (drawItem.m_scissorsCount)
        {
            scissorState = m_state.m_scissorState;
            SetScissors(drawItem.m_scissors, drawItem.m_scissorsCount);
        }

        RHI::CommandListViewportState viewportState;
        if (drawItem.m_viewportsCount)
        {
            viewportState = m_state.m_viewportState;
            SetViewports(drawItem.m_viewports, drawItem.m_viewportsCount);
        }

        CommitScissorState();
        CommitViewportState();

        switch (drawItem.m_arguments.m_type)
        {
        case RHI::DrawType::Indexed:
            {
                AZ_Assert(drawItem.m_indexBufferView, "IndexBufferView is null.");

                const RHI::DrawIndexed& indexed = drawItem.m_arguments.m_indexed;
                SetIndexBuffer(*drawItem.m_indexBufferView);

                m_state.m_wgpuRenderPassEncoder.DrawIndexed(
                    indexed.m_indexCount, indexed.m_instanceCount, indexed.m_indexOffset, indexed.m_vertexOffset, indexed.m_instanceOffset);
                break;
            }
        case RHI::DrawType::Linear:
            {
                const RHI::DrawLinear& linear = drawItem.m_arguments.m_linear;

                m_state.m_wgpuRenderPassEncoder.Draw(
                    linear.m_vertexCount, linear.m_instanceCount, linear.m_vertexOffset, linear.m_instanceOffset);
                break;
            }
        case RHI::DrawType::Indirect:
            {
                break;
            }
        default:
            AZ_Assert(false, "DrawType is invalid.");
            break;
        }

        // Restore the scissors if needed.
        if (scissorState.IsValid())
        {
            SetScissors(scissorState.m_states.data(), aznumeric_caster(scissorState.m_states.size()));
        }

        // Restore the viewports if needed.
        if (viewportState.IsValid())
        {
            SetViewports(viewportState.m_states.data(), aznumeric_caster(viewportState.m_states.size()));
        }
    }

    void CommandList::Submit(const RHI::DeviceCopyItem& copyItem, uint32_t submitIndex)
    {
        ValidateSubmitIndex(submitIndex);

        switch (copyItem.m_type)
        {
        case RHI::CopyItemType::Buffer:
            {
                const RHI::DeviceCopyBufferDescriptor& descriptor = copyItem.m_buffer;
                const auto* sourceBuffer = static_cast<const Buffer*>(descriptor.m_sourceBuffer);
                const auto* destinationBuffer = static_cast<const Buffer*>(descriptor.m_destinationBuffer);

                m_wgpuCommandEncoder.CopyBufferToBuffer(
                    sourceBuffer->GetNativeBuffer(),
                    descriptor.m_sourceOffset,
                    destinationBuffer->GetNativeBuffer(),
                    descriptor.m_destinationOffset,
                    descriptor.m_size);
                break;
            }
        case RHI::CopyItemType::BufferToImage:
            {
                const RHI::DeviceCopyBufferToImageDescriptor& descriptor = copyItem.m_bufferToImage;
                const auto* sourceBuffer = static_cast<const Buffer*>(descriptor.m_sourceBuffer);
                const auto* destinationImage = static_cast<const Image*>(descriptor.m_destinationImage);

                wgpu::ImageCopyBuffer wgpuSourceDesc = {};
                wgpuSourceDesc.layout.offset = 0;
                wgpuSourceDesc.layout.bytesPerRow = descriptor.m_sourceBytesPerRow;
                wgpuSourceDesc.layout.rowsPerImage = descriptor.m_sourceBytesPerImage / descriptor.m_sourceBytesPerRow;
                wgpuSourceDesc.buffer = sourceBuffer->GetNativeBuffer();
                wgpu::ImageCopyTexture wgpuDestDesc = {};
                wgpuDestDesc.texture = destinationImage->GetNativeTexture();
                wgpuDestDesc.mipLevel = descriptor.m_destinationSubresource.m_mipSlice;
                wgpuDestDesc.origin.x = descriptor.m_destinationOrigin.m_left;
                wgpuDestDesc.origin.y = descriptor.m_destinationOrigin.m_top;
                wgpuDestDesc.origin.z = descriptor.m_destinationOrigin.m_front;
                wgpuDestDesc.aspect = ConvertImageAspect(descriptor.m_destinationSubresource.m_aspect);
                wgpu::Extent3D wgpuSize = {};
                wgpuSize.width = descriptor.m_sourceSize.m_width;
                wgpuSize.height = descriptor.m_sourceSize.m_height;
                wgpuSize.depthOrArrayLayers = descriptor.m_sourceSize.m_depth;

                m_wgpuCommandEncoder.CopyBufferToTexture(&wgpuSourceDesc, &wgpuDestDesc, &wgpuSize);
                break;
            }
        default:
            AZ_Assert(false, "Invalid copy-item type.");
        }
    }

    void CommandList::Submit(const RHI::DeviceDispatchItem& dispatchItem, uint32_t submitIndex)
    {
        ValidateSubmitIndex(submitIndex);

        if (!CommitShaderResource(dispatchItem))
        {
            AZ_Warning("CommandList", false, "Failed to bind shader resources for dispatch item. Skipping.");
            return;
        }

        switch (dispatchItem.m_arguments.m_type)
        {
        case RHI::DispatchType::Direct:
            {
                const auto& arguments = dispatchItem.m_arguments.m_direct;
                m_state.m_wgpuComputePassEncoder.DispatchWorkgroups(
                    arguments.GetNumberOfGroupsX(), arguments.GetNumberOfGroupsY(), arguments.GetNumberOfGroupsZ());
                break;
            }
        case RHI::DispatchType::Indirect:
            {
                break;
            }
        default:
            AZ_Assert(false, "Invalid dispatch type");
            break;
        }
    }

    void CommandList::Shutdown()
    {
        m_state = State();
        m_wgpuCommandEncoder = nullptr;
        m_wgpuCommandBuffer = nullptr;
        DeviceObject::Shutdown();
    }

    void CommandList::CommitViewportState()
    {
        if (!m_state.m_viewportState.m_isDirty)
        {
            return;
        }

        const auto& rhiViewports = m_state.m_viewportState.m_states;
        AZ_Assert(rhiViewports.size() == 1, "Multiple viewports is not supported by WebGPU");
        const RHI::Viewport& rvp = rhiViewports.front();
        m_state.m_wgpuRenderPassEncoder.SetViewport(
            rvp.m_minX, rvp.m_minY, rvp.m_maxX - rvp.m_minX, rvp.m_maxY - rvp.m_minY, rvp.m_minZ, rvp.m_maxZ);
        m_state.m_viewportState.m_isDirty = false;
    }

    void CommandList::CommitScissorState()
    {
        if (!m_state.m_scissorState.m_isDirty)
        {
            return;
        }

        const auto& rhiScissors = m_state.m_scissorState.m_states;
        AZ_Assert(rhiScissors.size() == 1, "Multiple scissors is not supported by WebGPU");
        const RHI::Scissor& rsc = rhiScissors.front();
        m_state.m_wgpuRenderPassEncoder.SetScissorRect(rsc.m_minX, rsc.m_minY, rsc.m_maxX - rsc.m_minX, rsc.m_maxY - rsc.m_minY);
        m_state.m_scissorState.m_isDirty = false;
    }

    void CommandList::CommitBindGroups(RHI::PipelineStateType type)
    {
        ShaderResourceBindings& bindings = GetShaderResourceBindingsByPipelineType(type);
        const PipelineState& pipelineState = *bindings.m_pipelineState;
        const PipelineLayout& pipelineLayout = *pipelineState.GetPipelineLayout();
        const RHI::PipelineLayoutDescriptor& pipelineLayoutDescriptor = pipelineLayout.GetPipelineLayoutDescriptor();
        for (uint32_t srgIndex = 0; srgIndex < pipelineLayoutDescriptor.GetShaderResourceGroupLayoutCount(); ++srgIndex)
        {
            uint32_t bindingSlot = pipelineLayoutDescriptor.GetShaderResourceGroupLayout(srgIndex)->GetBindingSlot();
            uint32_t bindingGroupIndex = pipelineLayout.GetIndexBySlot(bindingSlot);
            RHI::ConstPtr<ShaderResourceGroup> shaderResourceGroup = bindings.m_SRGByAzslBindingSlot[bindingSlot];

            if (shaderResourceGroup)
            {
                auto& compiledData = shaderResourceGroup->GetCompiledData();
                const wgpu::BindGroup& bindGroup = compiledData.GetNativeBindGroup();
                if (bindings.m_bindGroups[bindingGroupIndex] != bindGroup.Get())
                {
                    switch (pipelineState.GetType())
                    {
                    case RHI::PipelineStateType::Draw:
                        m_state.m_wgpuRenderPassEncoder.SetBindGroup(bindingGroupIndex, bindGroup);
                        break;
                    case RHI::PipelineStateType::Dispatch:
                        m_state.m_wgpuComputePassEncoder.SetBindGroup(bindingGroupIndex, bindGroup);
                        break;
                    default:
                        AZ_Assert(false, "Invalid pipeline state %d", pipelineState.GetType());
                        return;
                    }
                    bindings.m_bindGroups[bindingGroupIndex] = bindGroup.Get();
                }
            }
        }
    }

    void CommandList::SetStreamBuffers(const RHI::DeviceStreamBufferView* streams, uint32_t count)
    {
        for (uint32_t index = 0; index < count; ++index)
        {
            if (m_state.m_streamBufferHashes[index] != static_cast<uint64_t>(streams[index].GetHash()))
            {
                m_state.m_streamBufferHashes[index] = static_cast<uint64_t>(streams[index].GetHash());
                m_state.m_wgpuRenderPassEncoder.SetVertexBuffer(
                    index,
                    static_cast<const Buffer*>(streams[index].GetBuffer())->GetNativeBuffer(),
                    streams[index].GetByteOffset(),
                    streams[index].GetByteCount());
            }
        }
    }

    void CommandList::SetIndexBuffer(const RHI::DeviceIndexBufferView& indexBufferView)
    {
        uint64_t indexBufferHash = static_cast<uint64_t>(indexBufferView.GetHash());
        if (indexBufferHash != m_state.m_indexBufferHash)
        {
            const Buffer* indexBuffer = static_cast<const Buffer*>(indexBufferView.GetBuffer());
            m_state.m_wgpuRenderPassEncoder.SetIndexBuffer(
                indexBuffer->GetNativeBuffer(),
                ConvertIndexFormat(indexBufferView.GetIndexFormat()),
                indexBufferView.GetByteOffset(),
                indexBufferView.GetByteCount());
            m_state.m_indexBufferHash = indexBufferHash;
        }
    }

    void CommandList::SetStencilRef(uint8_t stencilRef)
    {
        m_state.m_wgpuRenderPassEncoder.SetStencilReference(stencilRef);
    }

    void CommandList::SetShaderResourceGroup(const RHI::DeviceShaderResourceGroup& shaderResourceGroupBase, RHI::PipelineStateType type)
    {
        const uint32_t bindingSlot = shaderResourceGroupBase.GetBindingSlot();
        const auto& shaderResourceGroup = static_cast<const ShaderResourceGroup&>(shaderResourceGroupBase);
        auto& bindings = m_state.m_bindingsByPipe[static_cast<uint32_t>(type)];
        if (bindings.m_SRGByAzslBindingSlot[bindingSlot] != &shaderResourceGroup)
        {
            bindings.m_SRGByAzslBindingSlot[bindingSlot] = &shaderResourceGroup;
        }
    }

    bool CommandList::BindPipeline(const PipelineState* pipelineState)
    {
        ShaderResourceBindings& bindings = GetShaderResourceBindingsByPipelineType(pipelineState->GetType());
        if (bindings.m_pipelineState != pipelineState)
        {
            if (bindings.m_pipelineState && pipelineState)
            {
                auto newPipelineLayoutHash = pipelineState->GetPipelineLayout()->GetPipelineLayoutDescriptor().GetHash();
                auto oldPipelineLayoutHash = bindings.m_pipelineState->GetPipelineLayout()->GetPipelineLayoutDescriptor().GetHash();
                if (newPipelineLayoutHash != oldPipelineLayoutHash)
                {
                    bindings.m_bindGroups.fill(nullptr);
                }
            }

            bindings.m_pipelineState = pipelineState;

            switch (pipelineState->GetType())
            {
            case RHI::PipelineStateType::Draw:
                {
                    const auto& wgpuRenderPipeline = static_cast<RenderPipeline*>(pipelineState->GetPipeline())->GetNativeRenderPipeline();
                    if (!wgpuRenderPipeline)
                    {
                        return false;
                    }
                    m_state.m_wgpuRenderPassEncoder.SetPipeline(wgpuRenderPipeline);
                    break;
                }
            case RHI::PipelineStateType::Dispatch:
                {
                    const auto& wgpuComputePipeline = static_cast<ComputePipeline*>(pipelineState->GetPipeline())->GetNativeComputePipeline();
                    if (!wgpuComputePipeline)
                    {
                        return false;
                    }
                    m_state.m_wgpuComputePassEncoder.SetPipeline(wgpuComputePipeline);
                    break;
                }
            default:
                AZ_Assert(false, "Unsupported pipeline type %d", pipelineState->GetType());
                return false;
            }
        }
        return true;
    }

    CommandList::ShaderResourceBindings& CommandList::GetShaderResourceBindingsByPipelineType(RHI::PipelineStateType type)
    {
        return m_state.m_bindingsByPipe[static_cast<uint32_t>(type)];
    }

    void CommandList::ValidateShaderResourceGroups([[maybe_unused]] RHI::PipelineStateType type) const
    {
    }
} // namespace AZ::WebGPU
