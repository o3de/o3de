/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/DeviceDispatchRaysItem.h>
#include <Atom/RHI/DeviceIndirectArguments.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <RHI/Buffer.h>
#include <RHI/BufferPool.h>
#include <RHI/BufferView.h>
#include <RHI/CommandList.h>
#include <RHI/CommandQueue.h>
#include <RHI/Conversions.h>
#include <RHI/DescriptorContext.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/ImageView.h>
#include <RHI/IndirectBufferSignature.h>
#include <RHI/PipelineState.h>
#include <RHI/QueryPool.h>
#include <RHI/RayTracingBlas.h>
#include <RHI/RayTracingCompactionQueryPool.h>
#include <RHI/RayTracingPipelineState.h>
#include <RHI/RayTracingShaderTable.h>
#include <RHI/RayTracingTlas.h>
#include <RHI/ShaderResourceGroup.h>
#include <RHI/SwapChain.h>

#include <RHI/DispatchRaysIndirectBuffer.h>

// Conditionally disable timing at compile-time based on profile policy
#if DX12_GPU_PROFILE_MODE == DX12_GPU_PROFILE_MODE_DETAIL
#define DX12_COMMANDLIST_TIMER(id) ScopedTimer timer__(m_timerHeap, *this, TimerType::User, id);
#define DX12_COMMANDLIST_TIMER_DETAIL(id) DX12_COMMANDLIST_TIMER(name)
#elif DX12_GPU_PROFILE_MODE == DX12_GPU_PROFILE_MODE_BASIC
#define DX12_COMMANDLIST_TIMER(id) ScopedTimer timer__(m_timerHeap, *this, TimerType::User, id);
#define DX12_COMMANDLIST_TIMER_DETAIL(id)
#else
#define DX12_COMMANDLIST_TIMER(id)
#define DX12_COMMANDLIST_TIMER_DETAIL(id)
#endif

namespace AZ
{
    namespace DX12
    {

        RHI::Ptr<CommandList> CommandList::Create()
        {
            return aznew CommandList();
        }

        bool CommandList::IsInitialized() const
        {
            return CommandListBase::IsInitialized();
        }

        void CommandList::Init(Device& device, RHI::HardwareQueueClass hardwareQueueClass, const AZStd::shared_ptr<DescriptorContext>& descriptorContext, ID3D12CommandAllocator* commandAllocator)
        {
            CommandListBase::Init(device, hardwareQueueClass, commandAllocator);
            m_descriptorContext = descriptorContext;

            if (GetHardwareQueueClass() != RHI::HardwareQueueClass::Copy)
            {
                m_descriptorContext->SetDescriptorHeaps(GetCommandList());
            }
        }

        void CommandList::Shutdown()
        {
            if (IsInitialized())
            {
                m_descriptorContext = nullptr;
            }
        }

        void CommandList::Reset(ID3D12CommandAllocator* commandAllocator)
        {
            CommandListBase::Reset(commandAllocator);

            if (GetHardwareQueueClass() != RHI::HardwareQueueClass::Copy)
            {
                m_descriptorContext->SetDescriptorHeaps(GetCommandList());
            }

            // Clear any previously used name.
            SetName(Name());

            // Clear state back to empty.
            m_state = State();
        }

        void CommandList::Open(const Name& name)
        {
            SetName(name);

            if (RHI::RHISystemInterface::Get()->GpuMarkersEnabled() && r_gpuMarkersMergeGroups)
            {
                PIXBeginEvent(GetCommandList(), PIX_MARKER_CMDLIST_COL, name.GetCStr());
            }
        }

        void CommandList::Close()
        {
            FlushBarriers();
            if (RHI::RHISystemInterface::Get()->GpuMarkersEnabled() && r_gpuMarkersMergeGroups)
            {
                PIXEndEvent(GetCommandList());
            }

            CommandListBase::Close();
        }

        void CommandList::SetParentQueue(CommandQueue* parentQueue)
        {
            m_state.m_parentQueue = parentQueue;
        }

        CommandList::ShaderResourceBindings& CommandList::GetShaderResourceBindingsByPipelineType(RHI::PipelineStateType pipelineType)
        {
            return m_state.m_bindingsByPipe[static_cast<size_t>(pipelineType)];
        }

        void CommandList::SetViewports(
            const RHI::Viewport* viewports,
            uint32_t count)
        {
            m_state.m_viewportState.Set(AZStd::span<const RHI::Viewport>(viewports, count));
        }

        void CommandList::SetScissors(
            const RHI::Scissor* scissors,
            uint32_t count)
        {
            m_state.m_scissorState.Set(AZStd::span<const RHI::Scissor>(scissors, count));
        }

        void CommandList::SetShaderResourceGroupForDraw(const RHI::DeviceShaderResourceGroup& shaderResourceGroup)
        {
            SetShaderResourceGroup<RHI::PipelineStateType::Draw>(static_cast<const ShaderResourceGroup*>(&shaderResourceGroup));
        }

        void CommandList::SetShaderResourceGroupForDispatch(const RHI::DeviceShaderResourceGroup& shaderResourceGroup)
        {
            SetShaderResourceGroup<RHI::PipelineStateType::Dispatch>(static_cast<const ShaderResourceGroup*>(&shaderResourceGroup));
        }

        void CommandList::Submit(const RHI::DeviceCopyItem& copyItem, uint32_t submitIndex)
        {
            ValidateSubmitIndex(submitIndex);

            switch (copyItem.m_type)
            {

            case RHI::CopyItemType::Buffer:
            {
                const RHI::DeviceCopyBufferDescriptor& descriptor = copyItem.m_buffer;
                const Buffer* sourceBuffer = static_cast<const Buffer*>(descriptor.m_sourceBuffer);
                const Buffer* destinationBuffer = static_cast<const Buffer*>(descriptor.m_destinationBuffer);

                GetCommandList()->CopyBufferRegion(
                    destinationBuffer->GetMemoryView().GetMemory(),
                    destinationBuffer->GetMemoryView().GetOffset() + descriptor.m_destinationOffset,
                    sourceBuffer->GetMemoryView().GetMemory(),
                    sourceBuffer->GetMemoryView().GetOffset() + descriptor.m_sourceOffset,
                    descriptor.m_size);
                break;
            }

            case RHI::CopyItemType::Image:
            {
                const RHI::DeviceCopyImageDescriptor& descriptor = copyItem.m_image;
                const Image* sourceImage = static_cast<const Image*>(descriptor.m_sourceImage);
                const Image* destinationImage = static_cast<const Image*>(descriptor.m_destinationImage);

                const CD3DX12_TEXTURE_COPY_LOCATION sourceLocation(
                    sourceImage->GetMemoryView().GetMemory(),
                    D3D12CalcSubresource(
                        descriptor.m_sourceSubresource.m_mipSlice,
                        descriptor.m_sourceSubresource.m_arraySlice,
                        ConvertImageAspectToPlaneSlice(descriptor.m_sourceSubresource.m_aspect),
                        sourceImage->GetDescriptor().m_mipLevels,
                        sourceImage->GetDescriptor().m_arraySize));

                const CD3DX12_TEXTURE_COPY_LOCATION destinationLocation(
                    destinationImage->GetMemoryView().GetMemory(),
                    D3D12CalcSubresource(
                        descriptor.m_destinationSubresource.m_mipSlice,
                        descriptor.m_destinationSubresource.m_arraySlice,
                        ConvertImageAspectToPlaneSlice(descriptor.m_destinationSubresource.m_aspect),
                        destinationImage->GetDescriptor().m_mipLevels,
                        destinationImage->GetDescriptor().m_arraySize));

                const CD3DX12_BOX sourceBox(
                    descriptor.m_sourceOrigin.m_left,
                    descriptor.m_sourceOrigin.m_top,
                    descriptor.m_sourceOrigin.m_front,
                    descriptor.m_sourceOrigin.m_left + descriptor.m_sourceSize.m_width,
                    descriptor.m_sourceOrigin.m_top + descriptor.m_sourceSize.m_height,
                    descriptor.m_sourceOrigin.m_front + descriptor.m_sourceSize.m_depth);

                GetCommandList()->CopyTextureRegion(
                    &destinationLocation,
                    descriptor.m_destinationOrigin.m_left,
                    descriptor.m_destinationOrigin.m_top,
                    descriptor.m_destinationOrigin.m_front,
                    &sourceLocation,
                    &sourceBox);

                break;
            }

            case RHI::CopyItemType::BufferToImage:
            {
                const RHI::DeviceCopyBufferToImageDescriptor& descriptor = copyItem.m_bufferToImage;
                const Buffer* sourceBuffer = static_cast<const Buffer*>(descriptor.m_sourceBuffer);
                const Image* destinationImage = static_cast<const Image*>(descriptor.m_destinationImage);

                D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
                footprint.Offset = sourceBuffer->GetMemoryView().GetOffset() + descriptor.m_sourceOffset;
                footprint.Footprint.Width = descriptor.m_sourceSize.m_width;
                footprint.Footprint.Height = descriptor.m_sourceSize.m_height;
                footprint.Footprint.Depth = descriptor.m_sourceSize.m_depth;
                footprint.Footprint.Format = ConvertFormat(descriptor.m_sourceFormat);
                footprint.Footprint.RowPitch = descriptor.m_sourceBytesPerRow;

                const CD3DX12_TEXTURE_COPY_LOCATION sourceLocation(sourceBuffer->GetMemoryView().GetMemory(), footprint);

                const CD3DX12_TEXTURE_COPY_LOCATION destinationLocation(
                    destinationImage->GetMemoryView().GetMemory(),
                    D3D12CalcSubresource(
                        descriptor.m_destinationSubresource.m_mipSlice,
                        descriptor.m_destinationSubresource.m_arraySlice,
                        ConvertImageAspectToPlaneSlice(descriptor.m_destinationSubresource.m_aspect),
                        destinationImage->GetDescriptor().m_mipLevels,
                        destinationImage->GetDescriptor().m_arraySize));

                GetCommandList()->CopyTextureRegion(
                    &destinationLocation,
                    descriptor.m_destinationOrigin.m_left,
                    descriptor.m_destinationOrigin.m_top,
                    descriptor.m_destinationOrigin.m_front,
                    &sourceLocation,
                    nullptr);

                break;
            }

            case RHI::CopyItemType::ImageToBuffer:
            {
                const RHI::DeviceCopyImageToBufferDescriptor& descriptor = copyItem.m_imageToBuffer;
                const Image* sourceImage = static_cast<const Image*>(descriptor.m_sourceImage);
                const Buffer* destinationBuffer = static_cast<const Buffer*>(descriptor.m_destinationBuffer);

                const CD3DX12_TEXTURE_COPY_LOCATION sourceLocation(
                    sourceImage->GetMemoryView().GetMemory(),
                    D3D12CalcSubresource(
                        descriptor.m_sourceSubresource.m_mipSlice,
                        descriptor.m_sourceSubresource.m_arraySlice,
                        ConvertImageAspectToPlaneSlice(descriptor.m_sourceSubresource.m_aspect),
                        sourceImage->GetDescriptor().m_mipLevels,
                        sourceImage->GetDescriptor().m_arraySize));

                D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
                footprint.Offset = destinationBuffer->GetMemoryView().GetOffset() + descriptor.m_destinationOffset;
                footprint.Footprint.Width = descriptor.m_sourceSize.m_width;
                footprint.Footprint.Height = descriptor.m_sourceSize.m_height;
                footprint.Footprint.Depth = descriptor.m_sourceSize.m_depth;
                footprint.Footprint.Format = ConvertFormat(descriptor.m_destinationFormat);
                footprint.Footprint.RowPitch = descriptor.m_destinationBytesPerRow;

                const CD3DX12_TEXTURE_COPY_LOCATION destinationLocation(destinationBuffer->GetMemoryView().GetMemory(), footprint);
                GetCommandList()->CopyTextureRegion(&destinationLocation, 0, 0, 0, &sourceLocation, nullptr);
                break;
            }
            case RHI::CopyItemType::QueryToBuffer:
            {
                const RHI::DeviceCopyQueryToBufferDescriptor& descriptor = copyItem.m_queryToBuffer;

                GetCommandList()->ResolveQueryData(
                    static_cast<const QueryPool*>(descriptor.m_sourceQueryPool)->GetHeap(),
                    ConvertQueryType(descriptor.m_sourceQueryPool->GetDescriptor().m_type, RHI::QueryControlFlags::None),
                    descriptor.m_firstQuery.GetIndex(),
                    descriptor.m_queryCount,
                    static_cast<const Buffer*>(descriptor.m_destinationBuffer)->GetMemoryView().GetMemory(),
                    descriptor.m_destinationOffset);
                break;
            }
            default:
                AZ_Assert(false, "Invalid CopyItem type");
                return;
            }
        }

        void CommandList::Submit(const RHI::DeviceDispatchItem& dispatchItem, uint32_t submitIndex)
        {
            ValidateSubmitIndex(submitIndex);

            if (!CommitShaderResources<RHI::PipelineStateType::Dispatch>(dispatchItem))
            {
                AZ_Warning("CommandList", false, "Failed to bind shader resources for dispatch item. Skipping.");
                return;
            }


            switch (dispatchItem.m_arguments.m_type)
            {
            case RHI::DispatchType::Direct:
            {
                const auto& directArguments = dispatchItem.m_arguments.m_direct;
                GetCommandList()->Dispatch(directArguments.GetNumberOfGroupsX(), directArguments.GetNumberOfGroupsY(), directArguments.GetNumberOfGroupsZ());
                break;
            }
            case RHI::DispatchType::Indirect:
                ExecuteIndirect(dispatchItem.m_arguments.m_indirect);
                break;
            default:
                AZ_Assert(false, "Invalid dispatch type");
                break;
            }
        }

        void CommandList::Submit([[maybe_unused]] const RHI::DeviceDispatchRaysItem& dispatchRaysItem, [[maybe_unused]] uint32_t submitIndex)
        {
#ifdef AZ_DX12_DXR_SUPPORT
            ValidateSubmitIndex(submitIndex);

            ID3D12GraphicsCommandList4* commandList = static_cast<ID3D12GraphicsCommandList4*>(GetCommandList());

            // manually clear the Dispatch bindings and pipeline state since it is shared with the ray tracing pipeline
            ShaderResourceBindings& bindings = GetShaderResourceBindingsByPipelineType(RHI::PipelineStateType::Dispatch);
            for (size_t i = 0; i < bindings.m_srgsByIndex.size(); ++i)
            {
                bindings.m_srgsByIndex[i] = nullptr;
            }

            m_state.m_pipelineState = nullptr;

            // [GFX TODO][ATOM-5736] Cache ray tracing pipeline state and bindings

            // set the global root signature
            const RayTracingPipelineState* rayTracingPipelineState = static_cast<const RayTracingPipelineState*>(dispatchRaysItem.m_rayTracingPipelineState);
            if (!rayTracingPipelineState)
            {
                AZ_Assert(false, "Pipeline state not provided");
                return;
            }

            commandList->SetComputeRootSignature(rayTracingPipelineState->GetGlobalRootSignature());

            const PipelineState* globalPipelineState = static_cast<const PipelineState*>(dispatchRaysItem.m_globalPipelineState);
            if (!globalPipelineState)
            {
                AZ_Assert(false, "Global Pipeline state not provided");
                return;
            }

            const PipelineLayout* globalPipelineLayout = globalPipelineState->GetPipelineLayout();
            if (!globalPipelineLayout)
            {
                AZ_Assert(false, "Pipeline layout is null.");
                return;
            }

            // bind ShaderResourceGroups
            for (uint32_t srgIndex = 0; srgIndex < dispatchRaysItem.m_shaderResourceGroupCount; ++srgIndex)
            {
                const uint32_t srgBindingSlot = dispatchRaysItem.m_shaderResourceGroups[srgIndex]->GetBindingSlot();

                // retrieve binding
                const size_t srgBindingIndex = globalPipelineLayout->GetIndexBySlot(srgBindingSlot);
                RootParameterBinding binding = globalPipelineLayout->GetRootParameterBindingByIndex(srgBindingIndex);
                const ShaderResourceGroup* srg = static_cast<const ShaderResourceGroup*>(dispatchRaysItem.m_shaderResourceGroups[srgIndex]);
                const ShaderResourceGroupCompiledData& compiledData = srg->GetCompiledData();

                if (binding.m_resourceTable.IsValid()
                    && compiledData.m_gpuViewsDescriptorHandle.ptr)
                {
                    GetCommandList()->SetComputeRootDescriptorTable(binding.m_resourceTable.GetIndex(), compiledData.m_gpuViewsDescriptorHandle);
                }

                for (uint32_t unboundedArrayIndex = 0; unboundedArrayIndex < ShaderResourceGroupCompiledData::MaxUnboundedArrays; ++unboundedArrayIndex)
                {
                    if (binding.m_bindlessTable.IsValid()
                        && compiledData.m_gpuUnboundedArraysDescriptorHandles[unboundedArrayIndex].ptr)
                    {
                        GetCommandList()->SetComputeRootDescriptorTable(
                            binding.m_bindlessTable.GetIndex(),
                            compiledData.m_gpuUnboundedArraysDescriptorHandles[unboundedArrayIndex]);
                    }
                }

                if (binding.m_constantBuffer.IsValid())
                {
                    GetCommandList()->SetComputeRootConstantBufferView(binding.m_constantBuffer.GetIndex(), compiledData.m_gpuConstantAddress);
                }
            }

            // set the bindless descriptor table if required by the shader
            const auto& device = static_cast<Device&>(GetDevice());
            for (uint32_t bindingIndex = 0; bindingIndex < globalPipelineLayout->GetRootParameterBindingCount(); ++bindingIndex)
            {
                const size_t srgSlot = globalPipelineLayout->GetSlotByIndex(bindingIndex);
                if (srgSlot == device.GetBindlessSrgSlot())
                {
                    RootParameterBinding binding = globalPipelineLayout->GetRootParameterBindingByIndex(bindingIndex);
                    if (binding.m_bindlessTable.IsValid())
                    {
                        GetCommandList()->SetComputeRootDescriptorTable(
                            binding.m_bindlessTable.GetIndex(), m_descriptorContext->GetBindlessGpuPlatformHandle());
                        break;
                    }
                    else
                    {
                        AZ_Assert(false, "The ShaderResourceGroup using the Bindless SRG Slot doesn't have bindless arrays.");
                    }
                }
            }

            // set RayTracing pipeline state
            commandList->SetPipelineState1(rayTracingPipelineState->Get());

            switch (dispatchRaysItem.m_arguments.m_type)
            {
            case RHI::DispatchRaysType::Direct:
                {
                    // setup DispatchRays() shader table and ray counts
                    const RayTracingShaderTable* shaderTable =
                        static_cast<const RayTracingShaderTable*>(dispatchRaysItem.m_rayTracingShaderTable);
                    const RayTracingShaderTable::ShaderTableBuffers& shaderTableBuffers = shaderTable->GetBuffers();

                    D3D12_DISPATCH_RAYS_DESC desc = {};
                    desc.RayGenerationShaderRecord.StartAddress = shaderTableBuffers.m_rayGenerationTable->GetMemoryView().GetGpuAddress();
                    desc.RayGenerationShaderRecord.SizeInBytes = shaderTableBuffers.m_rayGenerationTableSize;

                    desc.MissShaderTable.StartAddress =
                        shaderTableBuffers.m_missTable ? shaderTableBuffers.m_missTable->GetMemoryView().GetGpuAddress() : 0;
                    desc.MissShaderTable.SizeInBytes = shaderTableBuffers.m_missTableSize;
                    desc.MissShaderTable.StrideInBytes = shaderTableBuffers.m_missTableStride;

                    desc.CallableShaderTable.StartAddress =
                        shaderTableBuffers.m_callableTable ? shaderTableBuffers.m_callableTable->GetMemoryView().GetGpuAddress() : 0;
                    desc.CallableShaderTable.SizeInBytes = shaderTableBuffers.m_callableTableSize;
                    desc.CallableShaderTable.StrideInBytes = shaderTableBuffers.m_callableTableStride;

                    desc.HitGroupTable.StartAddress = shaderTableBuffers.m_hitGroupTable->GetMemoryView().GetGpuAddress();
                    desc.HitGroupTable.SizeInBytes = shaderTableBuffers.m_hitGroupTableSize;
                    desc.HitGroupTable.StrideInBytes = shaderTableBuffers.m_hitGroupTableStride;

                    desc.Width = dispatchRaysItem.m_arguments.m_direct.m_width;
                    desc.Height = dispatchRaysItem.m_arguments.m_direct.m_height;
                    desc.Depth = dispatchRaysItem.m_arguments.m_direct.m_depth;

                    commandList->DispatchRays(&desc);

                    break;
                }
            case RHI::DispatchRaysType::Indirect:
                {
                    const auto& arguments = dispatchRaysItem.m_arguments.m_indirect;
                    auto* dispatchIndirectBuffer = static_cast<DispatchRaysIndirectBuffer*>(arguments.m_dispatchRaysIndirectBuffer);
                    AZ_Assert(
                        dispatchIndirectBuffer, "CommandList: m_dispatchRaysIndirectBuffer is necessary for indirect raytracing commands");
                    const Buffer* dx12IndirectBuffer = static_cast<const Buffer*>(dispatchIndirectBuffer->m_buffer.get());
                    // Copy arguments from the given indirect buffer to the one we can actually use for the ExecuteIndirect call
                    {
                        constexpr ptrdiff_t widthOffset = offsetof(D3D12_DISPATCH_RAYS_DESC, Width);
                        {
                            D3D12_RESOURCE_BARRIER barrier;
                            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                            barrier.Transition.pResource = dx12IndirectBuffer->GetMemoryView().GetMemory();
                            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
                            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
                            commandList->ResourceBarrier(1, &barrier);
                        }

                        if (dispatchIndirectBuffer->m_shaderTableNeedsCopy)
                        {
                            AZ_Assert(
                                dispatchIndirectBuffer->m_shaderTableStagingMemory.IsValid(),
                                "DispatchRaysIndirectBuffer: Staging memory is not valid."
                                " The Build function must be called in the same frame as the CopyData function");
                            commandList->CopyBufferRegion(
                                dx12IndirectBuffer->GetMemoryView().GetMemory(),
                                dx12IndirectBuffer->GetMemoryView().GetOffset(),
                                dispatchIndirectBuffer->m_shaderTableStagingMemory.GetMemory(),
                                dispatchIndirectBuffer->m_shaderTableStagingMemory.GetOffset(),
                                widthOffset); // copy the shader table entries only
                            dispatchIndirectBuffer->m_shaderTableNeedsCopy = false;
                            dispatchIndirectBuffer->m_shaderTableStagingMemory = {}; // The staging memory is only valid for one frame. Make
                                                                                     // sure to not access it again
                        }
                        constexpr ptrdiff_t sizeToCopy = sizeof(uint32_t) * 3;

                        const Buffer* dx12OriginalBuffer = static_cast<const Buffer*>(arguments.m_indirectBufferView->GetBuffer());

                        {
                            D3D12_RESOURCE_BARRIER barrier;
                            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                            barrier.Transition.pResource = dx12OriginalBuffer->GetMemoryView().GetMemory();
                            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
                            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
                            commandList->ResourceBarrier(1, &barrier);
                        }

                        commandList->CopyBufferRegion(
                            dx12IndirectBuffer->GetMemoryView().GetMemory(),
                            dx12IndirectBuffer->GetMemoryView().GetOffset() + widthOffset,
                            dx12OriginalBuffer->GetMemoryView().GetMemory(),
                            arguments.m_indirectBufferView->GetByteOffset() + arguments.m_indirectBufferByteOffset,
                            sizeToCopy);

                        {
                            D3D12_RESOURCE_BARRIER barrier;
                            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                            barrier.Transition.pResource = dx12OriginalBuffer->GetMemoryView().GetMemory();
                            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
                            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
                            commandList->ResourceBarrier(1, &barrier);
                        }

                        {
                            D3D12_RESOURCE_BARRIER barrier;
                            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                            barrier.Transition.pResource = dx12IndirectBuffer->GetMemoryView().GetMemory();
                            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
                            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
                            commandList->ResourceBarrier(1, &barrier);
                        }
                    }

                    const IndirectBufferSignature* signature =
                        static_cast<const IndirectBufferSignature*>(arguments.m_indirectBufferView->GetSignature());

                    AZ_Assert(arguments.m_countBuffer == nullptr, "CommandList: Count buffer is not supported for indirect raytracing");
                    GetCommandList()->ExecuteIndirect(
                        signature->Get(),
                        arguments.m_maxSequenceCount,
                        dx12IndirectBuffer->GetMemoryView().GetMemory(),
                        dx12IndirectBuffer->GetMemoryView().GetOffset(),
                        nullptr,
                        0);
                    break;
                }
            default:
                AZ_Assert(false, "Invalid dispatch type");
                break;
            }
#endif
        }

        void CommandList::Submit(const RHI::DeviceDrawItem& drawItem, uint32_t submitIndex)
        {
            ValidateSubmitIndex(submitIndex);

            if (drawItem.m_geometryView == nullptr)
            {
                AZ_Assert(false, "DrawItem being submitted without GeometryView, i.e. without draw arguments, index buffer or stream buffers!");
                return;
            }

            if (!CommitShaderResources<RHI::PipelineStateType::Draw>(drawItem))
            {
                AZ_Warning("CommandList", false, "Failed to bind shader resources for draw item. Skipping.");
                return;
            }

            SetStreamBuffers(*drawItem.m_geometryView, drawItem.m_streamIndices);
            SetStencilRef(drawItem.m_stencilRef);

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
            CommitShadingRateState();

            switch (drawItem.m_geometryView->GetDrawArguments().m_type)
            {
            case RHI::DrawType::Indexed:
            {
                AZ_Assert(drawItem.m_geometryView->GetIndexBufferView().GetBuffer(), "Index buffer view is null!");

                const RHI::DrawIndexed& indexed = drawItem.m_geometryView->GetDrawArguments().m_indexed;
                SetIndexBuffer(drawItem.m_geometryView->GetIndexBufferView());

                GetCommandList()->DrawIndexedInstanced(
                    indexed.m_indexCount,
                    drawItem.m_drawInstanceArgs.m_instanceCount,
                    indexed.m_indexOffset,
                    indexed.m_vertexOffset,
                    drawItem.m_drawInstanceArgs.m_instanceOffset);
                break;
            }

            case RHI::DrawType::Linear:
            {
                const RHI::DrawLinear& linear = drawItem.m_geometryView->GetDrawArguments().m_linear;
                GetCommandList()->DrawInstanced(
                    linear.m_vertexCount,
                    drawItem.m_drawInstanceArgs.m_instanceCount,
                    linear.m_vertexOffset,
                    drawItem.m_drawInstanceArgs.m_instanceOffset);
                break;
            }

            case RHI::DrawType::Indirect:
            {
                const auto& indirect = drawItem.m_geometryView->GetDrawArguments().m_indirect;
                const RHI::IndirectBufferLayout& layout = indirect.m_indirectBufferView->GetSignature()->GetDescriptor().m_layout;
                if (layout.GetType() == RHI::IndirectBufferLayoutType::IndexedDraw)
                {
                    AZ_Assert(drawItem.m_geometryView->GetIndexBufferView().GetBuffer(), "Index buffer view is null!");
                    SetIndexBuffer(drawItem.m_geometryView->GetIndexBufferView());
                }
                ExecuteIndirect(indirect);
                break;
            }
            default:
                AZ_Assert(false, "Invalid draw type %d", drawItem.m_geometryView->GetDrawArguments().m_type);
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

        void CommandList::BeginPredication(const RHI::DeviceBuffer& buffer, uint64_t offset, RHI::PredicationOp operation)
        {
            GetCommandList()->SetPredication(
                static_cast<const Buffer&>(buffer).GetMemoryView().GetMemory(), 
                offset, 
                ConvertPredicationOp(operation));

        }

        void CommandList::EndPredication()
        {
            GetCommandList()->SetPredication(nullptr, 0, D3D12_PREDICATION_OP_EQUAL_ZERO);
        }

        void CommandList::BuildBottomLevelAccelerationStructure([[maybe_unused]] const RHI::DeviceRayTracingBlas& rayTracingBlas)
        {
#ifdef AZ_DX12_DXR_SUPPORT
            const RayTracingBlas& dx12RayTracingBlas = static_cast<const RayTracingBlas&>(rayTracingBlas);
            const RayTracingBlas::BlasBuffers& blasBuffers = dx12RayTracingBlas.GetBuffers();

            // create the BLAS
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC blasDesc = {};
            blasDesc.Inputs = dx12RayTracingBlas.GetInputs();
            blasDesc.ScratchAccelerationStructureData = static_cast<Buffer*>(blasBuffers.m_scratchBuffer.get())->GetMemoryView().GetGpuAddress();
            blasDesc.DestAccelerationStructureData = static_cast<Buffer*>(blasBuffers.m_blasBuffer.get())->GetMemoryView().GetGpuAddress();
            ID3D12GraphicsCommandList4* commandList = static_cast<ID3D12GraphicsCommandList4*>(GetCommandList());
            commandList->BuildRaytracingAccelerationStructure(&blasDesc, 0, nullptr);
#endif
        }

        void CommandList::UpdateBottomLevelAccelerationStructure(const RHI::DeviceRayTracingBlas& rayTracingBlas)
        {
#ifdef AZ_DX12_DXR_SUPPORT
            const RayTracingBlas& dx12RayTracingBlas = static_cast<const RayTracingBlas&>(rayTracingBlas);
            const RayTracingBlas::BlasBuffers& blasBuffers = dx12RayTracingBlas.GetBuffers();

            // create the BLAS
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC blasDesc = {};
            blasDesc.Inputs = dx12RayTracingBlas.GetInputs();
            blasDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
            blasDesc.ScratchAccelerationStructureData = static_cast<Buffer*>(blasBuffers.m_scratchBuffer.get())->GetMemoryView().GetGpuAddress();
            blasDesc.SourceAccelerationStructureData = static_cast<Buffer*>(blasBuffers.m_blasBuffer.get())->GetMemoryView().GetGpuAddress();
            blasDesc.DestAccelerationStructureData = blasDesc.SourceAccelerationStructureData;
            ID3D12GraphicsCommandList4* commandList = static_cast<ID3D12GraphicsCommandList4*>(GetCommandList());
            commandList->BuildRaytracingAccelerationStructure(&blasDesc, 0, nullptr);
#endif
        }

        void CommandList::QueryBlasCompactionSizes(
            const AZStd::vector<AZStd::pair<RHI::DeviceRayTracingBlas*, RHI::DeviceRayTracingCompactionQuery*>>& blasToQuery)
        {
#ifdef AZ_DX12_DXR_SUPPORT
            ID3D12GraphicsCommandList4* commandList = static_cast<ID3D12GraphicsCommandList4*>(GetCommandList());

            // Query compaction sizes for all given Blas
            AZStd::unordered_set<RayTracingCompactionQueryPool*> usedQueryPools;
            for (auto& [blas, query] : blasToQuery)
            {
                const auto& dx12RayTracingBlas = static_cast<const RayTracingBlas*>(blas);
                const RayTracingBlas::BlasBuffers& blasBuffers = dx12RayTracingBlas->GetBuffers();

                auto* dx12CompactionQuery = static_cast<RayTracingCompactionQuery*>(query);
                int index = dx12CompactionQuery->Allocate();
                auto pool = static_cast<RayTracingCompactionQueryPool*>(dx12CompactionQuery->GetPool());

                auto queryPoolBufferAddress = static_cast<Buffer*>(pool->GetCurrentGPUBuffer())->GetMemoryView().GetGpuAddress();

                D3D12_RESOURCE_BARRIER barrier;
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
                barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                barrier.UAV.pResource = static_cast<Buffer*>(blasBuffers.m_blasBuffer.get())->GetMemoryView().GetMemory();
                commandList->ResourceBarrier(1, &barrier);

                D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC desc;
                desc.InfoType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE;
                desc.DestBuffer = queryPoolBufferAddress + index * sizeof(uint64_t);
                auto blasVirtualAddress = static_cast<Buffer*>(blasBuffers.m_blasBuffer.get())->GetMemoryView().GetGpuAddress();
                commandList->EmitRaytracingAccelerationStructurePostbuildInfo(&desc, 1, &blasVirtualAddress);
                usedQueryPools.insert(pool);
            }

            // Copy the gathered compaction sizes to the CPU buffer
            for (auto pool : usedQueryPools)
            {
                auto gpuBuffer = static_cast<Buffer*>(pool->GetCurrentGPUBuffer());
                auto cpuBuffer = static_cast<Buffer*>(pool->GetCurrentCPUBuffer());
                D3D12_RESOURCE_TRANSITION_BARRIER transitionDesc;
                transitionDesc.pResource = gpuBuffer->GetMemoryView().GetMemory();
                transitionDesc.Subresource = 0;
                transitionDesc.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                transitionDesc.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

                D3D12_RESOURCE_BARRIER barrierDesc;
                barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                barrierDesc.Transition = transitionDesc;
                commandList->ResourceBarrier(1, &barrierDesc);

                commandList->CopyBufferRegion(
                    cpuBuffer->GetMemoryView().GetMemory(),
                    cpuBuffer->GetMemoryView().GetOffset(),
                    gpuBuffer->GetMemoryView().GetMemory(),
                    gpuBuffer->GetMemoryView().GetOffset(),
                    pool->GetDescriptor().m_budget * sizeof(uint64_t));
            }
#endif
        }

        void CommandList::CompactBottomLevelAccelerationStructure(
            const RHI::DeviceRayTracingBlas& sourceBlas, const RHI::DeviceRayTracingBlas& compactBlas)
        {
#ifdef AZ_DX12_DXR_SUPPORT
            ID3D12GraphicsCommandList4* commandList = static_cast<ID3D12GraphicsCommandList4*>(GetCommandList());
            auto& dx12SourceBlas = static_cast<const RayTracingBlas&>(sourceBlas);
            auto sourceBlasVirtualAddress =
                static_cast<Buffer*>(dx12SourceBlas.GetBuffers().m_blasBuffer.get())->GetMemoryView().GetGpuAddress();
            auto& dx12CompactBlas = static_cast<const RayTracingBlas&>(compactBlas);
            auto compactBlasVirtualAddress =
                static_cast<Buffer*>(dx12CompactBlas.GetBuffers().m_blasBuffer.get())->GetMemoryView().GetGpuAddress();
            commandList->CopyRaytracingAccelerationStructure(
                compactBlasVirtualAddress, sourceBlasVirtualAddress, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT);
#endif
        }

        void CommandList::SetFragmentShadingRate(
            RHI::ShadingRate rate, const RHI::ShadingRateCombinators& combinators)
        {
            if (!RHI::CheckBitsAll(GetDevice().GetFeatures().m_shadingRateTypeMask, RHI::ShadingRateTypeFlags::PerDraw))
            {
                AZ_Assert(false, "Per Draw shading rate is not supported on this platform");
                return;
            }

            m_state.m_shadingRateState.Set(rate, combinators);
        }

        void CommandList::BuildTopLevelAccelerationStructure(
            const RHI::DeviceRayTracingTlas& rayTracingTlas, const AZStd::vector<const RHI::DeviceRayTracingBlas*>& changedBlasList)
        {
#ifdef AZ_DX12_DXR_SUPPORT
            ID3D12GraphicsCommandList4* commandList = static_cast<ID3D12GraphicsCommandList4*>(GetCommandList());
            if (!changedBlasList.empty())
            {
                // create a barrier for BLAS completion
                // this is required since all BLAS must be built prior to using it in the TLAS
                AZStd::vector<D3D12_RESOURCE_BARRIER> barriers;
                barriers.reserve(changedBlasList.size());
                for (const auto* blas : changedBlasList)
                {
                    const auto dx12RayTracingBlas = static_cast<const RayTracingBlas*>(blas);
                    const RayTracingBlas::BlasBuffers& blasBuffers = dx12RayTracingBlas->GetBuffers();

                    D3D12_RESOURCE_BARRIER barrier;
                    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
                    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                    barrier.UAV.pResource = static_cast<Buffer*>(blasBuffers.m_blasBuffer.get())->GetMemoryView().GetMemory();
                    barriers.push_back(barrier);
                }
                commandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
            }
            const RayTracingTlas& dx12RayTracingTlas = static_cast<const RayTracingTlas&>(rayTracingTlas);
            const RayTracingTlas::TlasBuffers& tlasBuffers = dx12RayTracingTlas.GetBuffers();

            // create the TLAS
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC tlasDesc = {};
            tlasDesc.Inputs = dx12RayTracingTlas.GetInputs();
            tlasDesc.ScratchAccelerationStructureData = static_cast<Buffer*>(tlasBuffers.m_scratchBuffer.get())->GetMemoryView().GetGpuAddress();
            tlasDesc.DestAccelerationStructureData = static_cast<Buffer*>(tlasBuffers.m_tlasBuffer.get())->GetMemoryView().GetGpuAddress();

            commandList->BuildRaytracingAccelerationStructure(&tlasDesc, 0, nullptr);
#endif
        }

        void CommandList::SetStencilRef(uint8_t stencilRef)
        {
            if (m_state.m_stencilRef != stencilRef)
            {
                GetCommandList()->OMSetStencilRef(stencilRef);
                m_state.m_stencilRef = stencilRef;
            }
        }

        void CommandList::SetTopology(RHI::PrimitiveTopology topology)
        {
            if (m_state.m_topology != topology)
            {
                GetCommandList()->IASetPrimitiveTopology(ConvertTopology(topology));
                m_state.m_topology = topology;
            }
        }

        void CommandList::CommitViewportState()
        {
            if (!m_state.m_viewportState.m_isDirty)
            {
                return;
            }

            AZ_PROFILE_FUNCTION(RHI);
            D3D12_VIEWPORT dx12Viewports[D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];

            const auto& viewports = m_state.m_viewportState.m_states;
            for (uint32_t i = 0; i < viewports.size(); ++i)
            {
                dx12Viewports[i].TopLeftX = viewports[i].m_minX;
                dx12Viewports[i].TopLeftY = viewports[i].m_minY;
                dx12Viewports[i].Width = viewports[i].m_maxX - viewports[i].m_minX;
                dx12Viewports[i].Height = viewports[i].m_maxY - viewports[i].m_minY;
                dx12Viewports[i].MinDepth = viewports[i].m_minZ;
                dx12Viewports[i].MaxDepth = viewports[i].m_maxZ;
            }

            GetCommandList()->RSSetViewports(aznumeric_caster(viewports.size()), dx12Viewports);
            m_state.m_viewportState.m_isDirty = false;
        }

        void CommandList::CommitScissorState()
        {
            if (!m_state.m_scissorState.m_isDirty)
            {
                return;
            }

            AZ_PROFILE_FUNCTION(RHI);
            D3D12_RECT dx12Scissors[D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];

            const auto& scissors = m_state.m_scissorState.m_states;
            for (uint32_t i = 0; i < scissors.size(); ++i)
            {
                dx12Scissors[i].left = scissors[i].m_minX;
                dx12Scissors[i].top = scissors[i].m_minY;
                dx12Scissors[i].right = scissors[i].m_maxX;
                dx12Scissors[i].bottom = scissors[i].m_maxY;
            }

            GetCommandList()->RSSetScissorRects(aznumeric_caster(scissors.size()), dx12Scissors);
            m_state.m_scissorState.m_isDirty = false;
        }

        void CommandList::CommitShadingRateState()
        {
            if (!m_state.m_shadingRateState.m_isDirty)
            {
                return;
            }

#ifdef O3DE_DX12_VRS_SUPPORT
            AZ_Assert(
                RHI::CheckBitsAll(GetDevice().GetFeatures().m_shadingRateTypeMask, RHI::ShadingRateTypeFlags::PerDraw),
                "PerDraw shading rate is not supported on this platform");

            AZStd::array<D3D12_SHADING_RATE_COMBINER, RHI::ShadingRateCombinators::array_size> d3d12Combinators;
            for (int i = 0; i < m_state.m_shadingRateState.m_shadingRateCombinators.size(); ++i)
            {
                d3d12Combinators[i] = ConvertShadingRateCombiner(m_state.m_shadingRateState.m_shadingRateCombinators[i]);
            }

            auto commandList5 = DX12ResourceCast<ID3D12GraphicsCommandList5>(GetCommandList());
            AZ_Assert(commandList5, "Failed to cast command list to ID3D12GraphicsCommandList5");
            if (commandList5)
            {
                commandList5->RSSetShadingRate(
                    ConvertShadingRateEnum(m_state.m_shadingRateState.m_shadingRate), d3d12Combinators.data());
            }
#endif
            m_state.m_shadingRateState.m_isDirty = false;
        }

        void CommandList::ExecuteIndirect(const RHI::DeviceIndirectArguments& arguments)
        {
            const IndirectBufferSignature* signature = static_cast<const IndirectBufferSignature*>(arguments.m_indirectBufferView->GetSignature());

            const Buffer* buffer = static_cast<const Buffer*>(arguments.m_indirectBufferView->GetBuffer());
            const Buffer* countBuffer = arguments.m_countBuffer ? static_cast<const Buffer*>(arguments.m_countBuffer) : nullptr;
            GetCommandList()->ExecuteIndirect(
                signature->Get(),
                arguments.m_maxSequenceCount,
                buffer->GetMemoryView().GetMemory(),
                buffer->GetMemoryView().GetOffset() + arguments.m_indirectBufferView->GetByteOffset() + arguments.m_indirectBufferByteOffset,
                countBuffer ? countBuffer->GetMemoryView().GetMemory() : nullptr,
                countBuffer ? arguments.m_countBufferByteOffset : 0
            );
        }

        void CommandList::SetStreamBuffers(const RHI::DeviceGeometryView& geometryBufferViews, const RHI::StreamBufferIndices& streamIndices)
        {
            auto streamIter = geometryBufferViews.CreateStreamIterator(streamIndices);

            bool needsBinding = false;

            for (u8 index = 0; !streamIter.HasEnded(); ++streamIter, ++index)
            {
                if (m_state.m_streamBufferHashes[index] != static_cast<uint64_t>(streamIter->GetHash()))
                {
                    m_state.m_streamBufferHashes[index] = static_cast<uint64_t>(streamIter->GetHash());
                    needsBinding = true;
                }
            }

            if (needsBinding)
            {
                D3D12_VERTEX_BUFFER_VIEW views[RHI::Limits::Pipeline::StreamCountMax];
                streamIter.Reset();

                for (u8 i = 0; !streamIter.HasEnded(); ++streamIter, ++i)
                {
                    if (streamIter->GetBuffer())
                    {
                        const Buffer* buffer = static_cast<const Buffer*>(streamIter->GetBuffer());
                        views[i].BufferLocation = buffer->GetMemoryView().GetGpuAddress() + streamIter->GetByteOffset();
                        views[i].SizeInBytes = streamIter->GetByteCount();
                        views[i].StrideInBytes = streamIter->GetByteStride();
                    }
                    else
                    {
                        views[i] = {};
                    }
                }

                GetCommandList()->IASetVertexBuffers(0, streamIndices.Size(), views);
            }
        }

        void CommandList::SetIndexBuffer(const RHI::DeviceIndexBufferView& indexBufferView)
        {
            uint64_t indexBufferHash = static_cast<uint64_t>(indexBufferView.GetHash());
            if (indexBufferHash != m_state.m_indexBufferHash)
            {
                m_state.m_indexBufferHash = indexBufferHash;
                if (const Buffer* indexBuffer = static_cast<const Buffer*>(indexBufferView.GetBuffer()))
                {
                    D3D12_INDEX_BUFFER_VIEW view;
                    view.BufferLocation = indexBuffer->GetMemoryView().GetGpuAddress() + indexBufferView.GetByteOffset();
                    view.Format = (indexBufferView.GetIndexFormat() == RHI::IndexFormat::Uint16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
                    view.SizeInBytes = indexBufferView.GetByteCount();

                    GetCommandList()->IASetIndexBuffer(&view);
                }
            }
        }

        void CommandList::SetRenderTargets(
            uint32_t renderTargetCount,
            const ImageView* const* renderTargets,
            const ImageView* depthStencilAttachment,
            RHI::ScopeAttachmentAccess depthStencilAccess,
            const ImageView* shadingRateAttachment)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE colorDescriptors[RHI::Limits::Pipeline::AttachmentColorCountMax];
            for (uint32_t i = 0; i < renderTargetCount; ++i)
            {
                AZ_Assert(renderTargets[i]->IsStale() == false, "Color view is stale!");
                colorDescriptors[i] =
                    m_descriptorContext->GetCpuPlatformHandle(renderTargets[i]->GetColorDescriptor());
            }

            if (depthStencilAttachment)
            {
                SetSamplePositions(depthStencilAttachment->GetImage().GetDescriptor().m_multisampleState);
                AZ_Assert(depthStencilAttachment->IsStale() == false, "Depth Stencil view is stale!");
                DescriptorHandle depthStencilDescriptor = depthStencilAttachment->GetDepthStencilDescriptor(depthStencilAccess);
                D3D12_CPU_DESCRIPTOR_HANDLE depthStencilPlatformDescriptor = m_descriptorContext->GetCpuPlatformHandle(depthStencilDescriptor);
                GetCommandList()->OMSetRenderTargets(renderTargetCount, colorDescriptors, false, &depthStencilPlatformDescriptor);
            }
            else
            {
                SetSamplePositions(renderTargets[0]->GetImage().GetDescriptor().m_multisampleState);
                GetCommandList()->OMSetRenderTargets(renderTargetCount, colorDescriptors, false, nullptr);
            }

#ifdef O3DE_DX12_VRS_SUPPORT
            if (m_state.m_shadingRateImage != shadingRateAttachment &&
                RHI::CheckBitsAll(GetDevice().GetFeatures().m_shadingRateTypeMask, RHI::ShadingRateTypeFlags::PerRegion))
            {
                auto commandList5 = DX12ResourceCast<ID3D12GraphicsCommandList5>(GetCommandList());
                AZ_Assert(commandList5, "Failed to cast command list to ID3D12GraphicsCommandList5");
                if (commandList5)
                {
                    if (shadingRateAttachment)
                    {
                        commandList5->RSSetShadingRateImage(shadingRateAttachment->GetMemory());
                        SetFragmentShadingRate(
                            RHI::ShadingRate::Rate1x1,
                            RHI::ShadingRateCombinators{ RHI::ShadingRateCombinerOp::Passthrough, RHI::ShadingRateCombinerOp::Override });
                    }
                    else
                    {
                        commandList5->RSSetShadingRateImage(nullptr);
                        SetFragmentShadingRate(
                            RHI::ShadingRate::Rate1x1,
                            RHI::ShadingRateCombinators{ RHI::ShadingRateCombinerOp::Override, RHI::ShadingRateCombinerOp::Passthrough });
                    }
                    m_state.m_shadingRateImage = shadingRateAttachment;
                }
            }
#endif
        }

        void CommandList::QueueTileMapRequest(const TileMapRequest& request)
        {
            m_state.m_tileMapRequests.push_back(request);
        }

        bool CommandList::HasTileMapRequests()
        {
            return !m_state.m_tileMapRequests.empty();
        }

        const CommandList::TileMapRequestList& CommandList::GetTileMapRequests() const
        {
            return m_state.m_tileMapRequests;
        }

        void CommandList::ClearRenderTarget(const ImageClearRequest& request)
        {
            if (request.m_clearValue.m_type == RHI::ClearValueType::Vector4Float)
            {
                D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle =
                    m_descriptorContext->GetCpuPlatformHandle(request.m_imageView->GetColorDescriptor());

                GetCommandList()->ClearRenderTargetView(
                    descriptorHandle,
                    request.m_clearValue.m_vector4Float.data(),
                    0, nullptr);
            }
            else if (request.m_clearValue.m_type == RHI::ClearValueType::DepthStencil)
            {
                // Need to set the custom MSAA positions (if being used) before clearing it.
                SetSamplePositions(request.m_imageView->GetImage().GetDescriptor().m_multisampleState);
                D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle =
                    m_descriptorContext->GetCpuPlatformHandle(request.m_imageView->GetDepthStencilDescriptor(RHI::ScopeAttachmentAccess::ReadWrite));

                GetCommandList()->ClearDepthStencilView(
                    descriptorHandle,
                    request.m_clearFlags,
                    request.m_clearValue.m_depthStencil.m_depth,
                    request.m_clearValue.m_depthStencil.m_stencil,
                    0, nullptr);
            }
            else
            {
                AZ_Assert(false, "Invalid clear value for output merger clear.");
            }
        }

        void CommandList::ClearUnorderedAccess(const ImageClearRequest& request)
        {
            const ImageView& imageView = *request.m_imageView;
            if (request.m_clearValue.m_type == RHI::ClearValueType::Vector4Uint)
            {
                GetCommandList()->ClearUnorderedAccessViewUint(
                    m_descriptorContext->GetGpuPlatformHandle(imageView.GetClearDescriptor()),
                    m_descriptorContext->GetCpuPlatformHandle(imageView.GetReadWriteDescriptor()),
                    imageView.GetMemory(),
                    request.m_clearValue.m_vector4Uint.data(), 0, nullptr);
            }
            else if (request.m_clearValue.m_type == RHI::ClearValueType::Vector4Float)
            {
                GetCommandList()->ClearUnorderedAccessViewFloat(
                    m_descriptorContext->GetGpuPlatformHandle(imageView.GetClearDescriptor()),
                    m_descriptorContext->GetCpuPlatformHandle(imageView.GetReadWriteDescriptor()),
                    imageView.GetMemory(),
                    request.m_clearValue.m_vector4Float.data(), 0, nullptr);
            }
            else
            {
                AZ_Assert(false, "Invalid clear value for image");
            }
        }

        void CommandList::DiscardResource(ID3D12Resource* resource)
        {
            GetCommandList()->DiscardResource(resource, nullptr);
        }

        void CommandList::ClearUnorderedAccess(const BufferClearRequest& request)
        {
            const BufferView& bufferView = *request.m_bufferView;
            if (request.m_clearValue.m_type == RHI::ClearValueType::Vector4Uint)
            {
                GetCommandList()->ClearUnorderedAccessViewUint(
                    m_descriptorContext->GetGpuPlatformHandle(bufferView.GetClearDescriptor()),
                    m_descriptorContext->GetCpuPlatformHandle(bufferView.GetReadWriteDescriptor()),
                    bufferView.GetMemory(),
                    request.m_clearValue.m_vector4Uint.data(), 0, nullptr);
            }
            else if (request.m_clearValue.m_type == RHI::ClearValueType::Vector4Float)
            {
                GetCommandList()->ClearUnorderedAccessViewFloat(
                    m_descriptorContext->GetGpuPlatformHandle(bufferView.GetClearDescriptor()),
                    m_descriptorContext->GetCpuPlatformHandle(bufferView.GetReadWriteDescriptor()),
                    bufferView.GetMemory(),
                    request.m_clearValue.m_vector4Float.data(), 0, nullptr);
            }
            else
            {
                AZ_Assert(false, "Invalid clear value for buffer");
            }
        }

        RHI::CommandListValidator& CommandList::GetValidator()
        {
            return m_validator;
        }
    }
}
