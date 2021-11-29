/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/CommandList.h>
#include <RHI/Conversions.h>
#include <RHI/Buffer.h>
#include <RHI/BufferView.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/ImageView.h>
#include <RHI/IndirectBufferSignature.h>
#include <RHI/ShaderResourceGroup.h>
#include <RHI/DescriptorContext.h>
#include <RHI/PipelineState.h>
#include <RHI/SwapChain.h>
#include <RHI/CommandQueue.h>
#include <RHI/QueryPool.h>
#include <Atom/RHI/IndirectArguments.h>
#include <AzCore/Debug/EventTrace.h>
#include <RHI/RayTracingBlas.h>
#include <RHI/RayTracingTlas.h>
#include <RHI/RayTracingPipelineState.h>
#include <RHI/RayTracingShaderTable.h>
#include <RHI/BufferPool.h>
#include <Atom/RHI/DispatchRaysItem.h>
#include <Atom/RHI/Factory.h>

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

#define PIX_MARKER_CMDLIST_COL 0xFF0000FF

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

            PIXBeginEvent(PIX_MARKER_CMDLIST_COL, name.GetCStr());
            if (RHI::Factory::Get().IsPixModuleLoaded() || RHI::Factory::Get().IsRenderDocModuleLoaded())
            {
                PIXBeginEvent(GetCommandList(), PIX_MARKER_CMDLIST_COL, name.GetCStr());
            }
        }

        void CommandList::Close()
        {
            FlushBarriers();
            PIXEndEvent();
            if (RHI::Factory::Get().IsPixModuleLoaded() || RHI::Factory::Get().IsRenderDocModuleLoaded())
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
            m_state.m_viewportState.Set(AZStd::array_view<RHI::Viewport>(viewports, count));            
        }

        void CommandList::SetScissors(
            const RHI::Scissor* scissors,
            uint32_t count)
        {
            m_state.m_scissorState.Set(AZStd::array_view<RHI::Scissor>(scissors, count));            
        }

        void CommandList::SetShaderResourceGroupForDraw(const RHI::ShaderResourceGroup& shaderResourceGroup)
        {
            SetShaderResourceGroup<RHI::PipelineStateType::Draw>(static_cast<const ShaderResourceGroup*>(&shaderResourceGroup));
        }

        void CommandList::SetShaderResourceGroupForDispatch(const RHI::ShaderResourceGroup& shaderResourceGroup)
        {
            SetShaderResourceGroup<RHI::PipelineStateType::Dispatch>(static_cast<const ShaderResourceGroup*>(&shaderResourceGroup));
        }

        void CommandList::Submit(const RHI::CopyItem& copyItem)
        {
            switch (copyItem.m_type)
            {

            case RHI::CopyItemType::Buffer:
            {
                const RHI::CopyBufferDescriptor& descriptor = copyItem.m_buffer;
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
                const RHI::CopyImageDescriptor& descriptor = copyItem.m_image;
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
                const RHI::CopyBufferToImageDescriptor& descriptor = copyItem.m_bufferToImage;
                const Buffer* sourceBuffer = static_cast<const Buffer*>(descriptor.m_sourceBuffer);
                const Image* destinationImage = static_cast<const Image*>(descriptor.m_destinationImage);

                D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
                footprint.Offset = sourceBuffer->GetMemoryView().GetOffset() + descriptor.m_sourceOffset;
                footprint.Footprint.Width = descriptor.m_sourceSize.m_width;
                footprint.Footprint.Height = descriptor.m_sourceSize.m_height;
                footprint.Footprint.Depth = descriptor.m_sourceSize.m_depth;
                footprint.Footprint.Format = ConvertFormat(destinationImage->GetDescriptor().m_format);
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
                const RHI::CopyImageToBufferDescriptor& descriptor = copyItem.m_imageToBuffer;
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
                const RHI::CopyQueryToBufferDescriptor& descriptor = copyItem.m_queryToBuffer;

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

        void CommandList::Submit(const RHI::DispatchItem& dispatchItem)
        {
            if (!CommitShaderResources<RHI::PipelineStateType::Dispatch>(dispatchItem))
            {
                AZ_Warning("CommandList", false, "Failed to bind shader resources for draw item. Skipping.");
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

        void CommandList::Submit([[maybe_unused]] const RHI::DispatchRaysItem& dispatchRaysItem)
        {
#ifdef AZ_DX12_DXR_SUPPORT
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
            commandList->SetComputeRootSignature(rayTracingPipelineState->GetGlobalRootSignature());

            const PipelineState* globalPipelineState = static_cast<const PipelineState*>(dispatchRaysItem.m_globalPipelineState);
            const PipelineLayout& globalPipelineLayout = globalPipelineState->GetPipelineLayout();              

            // bind ShaderResourceGroups
            for (uint32_t srgIndex = 0; srgIndex < dispatchRaysItem.m_shaderResourceGroupCount; ++srgIndex)
            {
                const uint32_t srgBindingSlot = dispatchRaysItem.m_shaderResourceGroups[srgIndex]->GetBindingSlot();

                // retrieve binding
                const size_t srgBindingIndex = globalPipelineLayout.GetIndexBySlot(srgBindingSlot);
                RootParameterBinding binding = globalPipelineLayout.GetRootParameterBindingByIndex(srgBindingIndex);
                const ShaderResourceGroup* srg = static_cast<const ShaderResourceGroup*>(dispatchRaysItem.m_shaderResourceGroups[srgIndex]);
                const ShaderResourceGroupCompiledData& compiledData = srg->GetCompiledData();

                if (binding.m_resourceTable.IsValid())
                {
                    GetCommandList()->SetComputeRootDescriptorTable(binding.m_resourceTable.GetIndex(), compiledData.m_gpuViewsDescriptorHandle);
                }

                for (uint32_t unboundedArrayIndex = 0; unboundedArrayIndex < ShaderResourceGroupCompiledData::MaxUnboundedArrays; ++unboundedArrayIndex)
                {
                    if (binding.m_unboundedArrayResourceTables[unboundedArrayIndex].IsValid())
                    {
                        GetCommandList()->SetComputeRootDescriptorTable(
                            binding.m_unboundedArrayResourceTables[unboundedArrayIndex].GetIndex(),
                            compiledData.m_gpuUnboundedArraysDescriptorHandles[unboundedArrayIndex]);
                    }
                }

                if (binding.m_constantBuffer.IsValid())
                {
                    GetCommandList()->SetComputeRootConstantBufferView(binding.m_constantBuffer.GetIndex(), compiledData.m_gpuConstantAddress);
                }
            }

            // set RayTracing pipeline state
            commandList->SetPipelineState1(rayTracingPipelineState->Get());

            // setup DispatchRays() shader table and ray counts
            const RayTracingShaderTable* shaderTable = static_cast<const RayTracingShaderTable*>(dispatchRaysItem.m_rayTracingShaderTable);
            const RayTracingShaderTable::ShaderTableBuffers& shaderTableBuffers = shaderTable->GetBuffers();

            D3D12_DISPATCH_RAYS_DESC desc = {};
            desc.RayGenerationShaderRecord.StartAddress = shaderTableBuffers.m_rayGenerationTable->GetMemoryView().GetGpuAddress();
            desc.RayGenerationShaderRecord.SizeInBytes = shaderTableBuffers.m_rayGenerationTableSize;
            
            desc.MissShaderTable.StartAddress = shaderTableBuffers.m_missTable->GetMemoryView().GetGpuAddress();
            desc.MissShaderTable.SizeInBytes = shaderTableBuffers.m_missTableSize;
            desc.MissShaderTable.StrideInBytes = shaderTableBuffers.m_missTableStride;
            
            desc.HitGroupTable.StartAddress = shaderTableBuffers.m_hitGroupTable->GetMemoryView().GetGpuAddress();
            desc.HitGroupTable.SizeInBytes = shaderTableBuffers.m_hitGroupTableSize;
            desc.HitGroupTable.StrideInBytes = shaderTableBuffers.m_hitGroupTableStride;

            desc.Width = dispatchRaysItem.m_width;
            desc.Height = dispatchRaysItem.m_height;
            desc.Depth = dispatchRaysItem.m_depth;
           
            commandList->DispatchRays(&desc);
#endif
        }

        void CommandList::Submit(const RHI::DrawItem& drawItem)
        {
            if (!CommitShaderResources<RHI::PipelineStateType::Draw>(drawItem))
            {
                AZ_Warning("CommandList", false, "Failed to bind shader resources for draw item. Skipping.");
                return;
            }

            SetStreamBuffers(drawItem.m_streamBufferViews, drawItem.m_streamBufferViewCount);
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

            switch (drawItem.m_arguments.m_type)
            {
            case RHI::DrawType::Indexed:
            {
                AZ_Assert(drawItem.m_indexBufferView, "Index buffer view is null!");

                const RHI::DrawIndexed& indexed = drawItem.m_arguments.m_indexed;
                SetIndexBuffer(*drawItem.m_indexBufferView);

                GetCommandList()->DrawIndexedInstanced(
                    indexed.m_indexCount,
                    indexed.m_instanceCount,
                    indexed.m_indexOffset,
                    indexed.m_vertexOffset,
                    indexed.m_instanceOffset);
                break;
            }

            case RHI::DrawType::Linear:
            {
                const RHI::DrawLinear& linear = drawItem.m_arguments.m_linear;
                GetCommandList()->DrawInstanced(
                    linear.m_vertexCount,
                    linear.m_instanceCount,
                    linear.m_vertexOffset,
                    linear.m_instanceOffset);
                break;
            }

            case RHI::DrawType::Indirect:
            {
                ExecuteIndirect(drawItem.m_arguments.m_indirect);
                break;
            }
            default:
                AZ_Assert(false, "Invalid draw type %d", drawItem.m_arguments.m_type);
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

        void CommandList::BeginPredication(const RHI::Buffer& buffer, uint64_t offset, RHI::PredicationOp operation)
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

        void CommandList::BuildBottomLevelAccelerationStructure([[maybe_unused]] const RHI::RayTracingBlas& rayTracingBlas)
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

            // create an immediate barrier for BLAS completion
            // this is required since the buffer must be built prior to using it in the TLAS
            D3D12_RESOURCE_BARRIER barrier;
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.UAV.pResource = static_cast<Buffer*>(blasBuffers.m_blasBuffer.get())->GetMemoryView().GetMemory();
            commandList->ResourceBarrier(1, &barrier);        
#endif
        }

        void CommandList::BuildTopLevelAccelerationStructure([[maybe_unused]] const RHI::RayTracingTlas& rayTracingTlas)
        {
#ifdef AZ_DX12_DXR_SUPPORT
            const RayTracingTlas& dx12RayTracingTlas = static_cast<const RayTracingTlas&>(rayTracingTlas);
            const RayTracingTlas::TlasBuffers& tlasBuffers = dx12RayTracingTlas.GetBuffers();

            // create the TLAS
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC tlasDesc = {};
            tlasDesc.Inputs = dx12RayTracingTlas.GetInputs();
            tlasDesc.ScratchAccelerationStructureData = static_cast<Buffer*>(tlasBuffers.m_scratchBuffer.get())->GetMemoryView().GetGpuAddress();
            tlasDesc.DestAccelerationStructureData = static_cast<Buffer*>(tlasBuffers.m_tlasBuffer.get())->GetMemoryView().GetGpuAddress();
        
            ID3D12GraphicsCommandList4* commandList = static_cast<ID3D12GraphicsCommandList4*>(GetCommandList());
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

            AZ_TRACE_METHOD();
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

            AZ_TRACE_METHOD();
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

        void CommandList::ExecuteIndirect(const RHI::IndirectArguments& arguments)
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

        void CommandList::SetStreamBuffers(const RHI::StreamBufferView* streams, uint32_t count)
        {
            bool needsBinding = false;

            for (uint32_t i = 0; i < count; ++i)
            {
                if (m_state.m_streamBufferHashes[i] != static_cast<uint64_t>(streams[i].GetHash()))
                {
                    m_state.m_streamBufferHashes[i] = static_cast<uint64_t>(streams[i].GetHash());
                    needsBinding = true;
                }
            }

            if (needsBinding)
            {
                D3D12_VERTEX_BUFFER_VIEW views[RHI::Limits::Pipeline::StreamCountMax];

                for (uint32_t i = 0; i < count; ++i)
                {
                    if (streams[i].GetBuffer())
                    {
                        const Buffer* buffer = static_cast<const Buffer*>(streams[i].GetBuffer());
                        views[i].BufferLocation = buffer->GetMemoryView().GetGpuAddress() + streams[i].GetByteOffset();
                        views[i].SizeInBytes = streams[i].GetByteCount();
                        views[i].StrideInBytes = streams[i].GetByteStride();
                    }
                    else
                    {
                        views[i] = {};
                    }
                }

                GetCommandList()->IASetVertexBuffers(0, count, views);
            }
        }

        void CommandList::SetIndexBuffer(const RHI::IndexBufferView& indexBufferView)
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
            RHI::ScopeAttachmentAccess depthStencilAccess)
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
                AZ_Assert(depthStencilAttachment->IsStale() == false, "Depth Stencil view is stale!");
                DescriptorHandle depthStencilDescriptor = depthStencilAttachment->GetDepthStencilDescriptor(depthStencilAccess);
                D3D12_CPU_DESCRIPTOR_HANDLE depthStencilPlatformDescriptor = m_descriptorContext->GetCpuPlatformHandle(depthStencilDescriptor);
                GetCommandList()->OMSetRenderTargets(renderTargetCount, colorDescriptors, false, &depthStencilPlatformDescriptor);
            }
            else
            {
                GetCommandList()->OMSetRenderTargets(renderTargetCount, colorDescriptors, false, nullptr);
            }
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
