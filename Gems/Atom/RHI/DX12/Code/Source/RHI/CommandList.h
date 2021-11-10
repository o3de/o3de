/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/CommandListBase.h>
#include <RHI/PipelineLayout.h>
#include <RHI/PipelineState.h>
#include <RHI/MemoryView.h>
#include <RHI/ShaderResourceGroup.h>
#include <RHI/Conversions.h>
#include <Atom/RHI.Reflect/ClearValue.h>
#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/CommandListValidator.h>
#include <Atom/RHI/CommandListStates.h>
#include <Atom/RHI/ObjectPool.h>
#include <AtomCore/std/containers/array_view.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/array.h>

#define DX12_GPU_PROFILE_MODE_OFF       0       // Turn off profiling
#define DX12_GPU_PROFILE_MODE_BASIC     1       // Profiles command list lifetime
#define DX12_GPU_PROFILE_MODE_DETAIL    2       // Profiles draw call state changes
#define DX12_GPU_PROFILE_MODE DX12_GPU_PROFILE_MODE_BASIC

namespace AZ
{
    namespace RHI
    {
        struct IndirectArguments;
    }

    namespace DX12
    {
        class CommandQueue;
        class DescriptorContext;
        class ShaderResourceGroup;
        class SwapChain;

        class CommandList
            : public RHI::CommandList
            , public CommandListBase
        {
        public:
            AZ_CLASS_ALLOCATOR(CommandList, AZ::SystemAllocator, 0);

            static RHI::Ptr<CommandList> Create();

            bool IsInitialized() const;

            void Init(
                Device& device,
                RHI::HardwareQueueClass hardwareQueueClass,
                const AZStd::shared_ptr<DescriptorContext>& descriptorContext,
                ID3D12CommandAllocator* commandAllocator);

            void Shutdown() override;

            void Open(const Name& name);
            void Close() override;

            //////////////////////////////////////////////////////////////////////////
            // CommandListBase
            void Reset(ID3D12CommandAllocator* commandAllocator) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::CommandList
            void SetViewports(const RHI::Viewport* viewports, uint32_t count) override;
            void SetScissors(const RHI::Scissor* scissors, uint32_t count) override;
            void SetShaderResourceGroupForDraw(const RHI::ShaderResourceGroup& shaderResourceGroup) override;
            void SetShaderResourceGroupForDispatch(const RHI::ShaderResourceGroup& shaderResourceGroup) override;
            void Submit(const RHI::DrawItem& drawItem) override;
            void Submit(const RHI::CopyItem& copyItem) override;
            void Submit(const RHI::DispatchItem& dispatchItem) override;
            void Submit(const RHI::DispatchRaysItem& dispatchRaysItem) override;
            void BeginPredication(const RHI::Buffer& buffer, uint64_t offset, RHI::PredicationOp operation) override;
            void EndPredication() override;
            void BuildBottomLevelAccelerationStructure(const RHI::RayTracingBlas& rayTracingBlas) override;
            void BuildTopLevelAccelerationStructure(const RHI::RayTracingTlas& rayTracingTlas) override;
            //////////////////////////////////////////////////////////////////////////

            void SetRenderTargets(
                uint32_t renderTargetCount,
                const ImageView* const* renderTarget,
                const ImageView* depthStencilAttachment,
                RHI::ScopeAttachmentAccess depthStencilAccess);

            //////////////////////////////////////////////////////////////////////////
            // Tile Mapping Methods

            /**
             * Maps a single subresource region of tiles for a source resource
             * to individual tiles of a destination heap. Used for cases where
             * tiles are pool allocated from the heap and assigned individually
             * to tiles of the source resource.
             *
             * This was written specifically to enable mip streaming, which is the
             * only system currently designed to utilize it.
             *
             * The request can either map tiles from the source resource to tiles
             * in the destination resource, or it can clear the existing mappings
             * on the source resource to null.
             *  - To map against the heap, specify a destination heap and the tile association map.
             *  - To clear existing mappings, leave the destination parts null / empty.
             */
            struct TileMapRequest
            {
                // The resource containing source tiles.
                Memory* m_sourceMemory = nullptr;

                // The start coordinate of the source tiles.
                D3D12_TILED_RESOURCE_COORDINATE m_sourceCoordinate;

                // The size of the source tile region.
                D3D12_TILE_REGION_SIZE m_sourceRegionSize;

                // The heap containing destination tiles. If this is null, all source tile mappings
                // are mapped to null.
                ID3D12Heap* m_destinationHeap = nullptr;

                // If m_destinationHeap is valid, the size of this vector must match the number of tiles
                // in m_sourceSize. Each index is the source tile index in the resource, and the value is
                // the destination heap tile index. If m_destinationHeap is null, this vector is ignored and
                // may be left empty.
                AZStd::vector<uint32_t> m_destinationTileMap;
            };

            // Queues a new tile map requests.
            void QueueTileMapRequest(const TileMapRequest& request);

            // Returns whether the command list has tile map requests.
            bool HasTileMapRequests();

            // Returns the list of queued tile map requests.
            using TileMapRequestList = AZStd::vector<TileMapRequest>;
            const TileMapRequestList& GetTileMapRequests() const;

            //////////////////////////////////////////////////////////////////////////
            // Clear Methods
            struct ImageClearRequest
            {
                /// The clear value used to clear the image.
                RHI::ClearValue m_clearValue;

                /// Clear flags for depth stencil images (ignored otherwise).
                D3D12_CLEAR_FLAGS m_clearFlags = (D3D12_CLEAR_FLAGS)0;

                /// The image view to clear.
                const ImageView* m_imageView = nullptr;
            };

            struct BufferClearRequest
            {
                /// The clear value for this buffer. Must be Float4 or Uint4.
                RHI::ClearValue m_clearValue;

                /// The buffer view to clear.
                const BufferView* m_bufferView = nullptr;
            };

            void ClearRenderTarget(const ImageClearRequest& request);
            void ClearUnorderedAccess(const ImageClearRequest& request);
            void ClearUnorderedAccess(const BufferClearRequest& request);
            void DiscardResource(ID3D12Resource* resource);

            RHI::CommandListValidator& GetValidator();

        private:
            friend CommandQueue;

            void SetParentQueue(CommandQueue* commandQueue);

            // NOTE: Uses templates to remove branching. Specialized between draw / dispatch paths.
            // Assigns a shader resource group to a logical slot. Does not bind to the command list
            // (CommitShaderResources does the command list bind).
            template <RHI::PipelineStateType>
            void SetShaderResourceGroup(const ShaderResourceGroup* shaderResourceGroup);

            // NOTE: Uses templates to remove branching. Specialized between draw / dispatch paths.
            // Binds the pipeline state / pipeline layout, then the shader resources associated with a
            // draw / dispatch call. Uses a pull model to bind state to the command list. Returns
            // whether the operation succeeded.
            template <RHI::PipelineStateType, typename Item>
            bool CommitShaderResources(const Item& item);

            void SetStreamBuffers(const RHI::StreamBufferView* descriptors, uint32_t count);
            void SetIndexBuffer(const RHI::IndexBufferView& descriptor);
            void SetStencilRef(uint8_t stencilRef);
            void SetTopology(RHI::PrimitiveTopology topology);
            void CommitViewportState();
            void CommitScissorState();

            void ExecuteIndirect(const RHI::IndirectArguments& arguments);

            RHI::CommandListValidator m_validator;

            struct ShaderResourceBindings
            {
                const PipelineLayout* m_pipelineLayout = nullptr;
                AZStd::array<const ShaderResourceGroup*, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_srgsByIndex;
                AZStd::array<const ShaderResourceGroup*, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_srgsBySlot;
                bool m_hasRootConstants = false;
            };

            ShaderResourceBindings& GetShaderResourceBindingsByPipelineType(RHI::PipelineStateType pipelineType);

            /**
             * This is kept as a separate struct so that we can robustly reset it. Every property
             * on this struct should be in-class-initialized so that there are no "missed" states.
             * Otherwise, it results in hard-to-track bugs down the road as it's too easy to add something
             * here and then miss adding the initialization elsewhere.
             */
            struct State
            {
                State() = default;

                const RHI::PipelineState* m_pipelineState = nullptr;

                // Graphics-specific state
                AZStd::array<uint64_t, RHI::Limits::Pipeline::StreamCountMax> m_streamBufferHashes = {{}};
                uint64_t m_indexBufferHash = 0;
                uint32_t m_stencilRef = static_cast<uint32_t>(-1);
                RHI::PrimitiveTopology m_topology = RHI::PrimitiveTopology::Undefined;
                RHI::CommandListViewportState m_viewportState;
                RHI::CommandListScissorState m_scissorState;

                // Array of shader resource bindings, indexed by command pipe.
                AZStd::array<ShaderResourceBindings, static_cast<size_t>(RHI::PipelineStateType::Count)> m_bindingsByPipe;

                // The command queue assigned to execute the command list.
                CommandQueue* m_parentQueue = nullptr;

                // A queue of tile mappings to execute on the command queue at submission time (prior to executing the command list).
                TileMapRequestList m_tileMapRequests;

                // Signal if the commandlist is using custom sample positions for multisample
                bool m_customSamplePositions = false;

            } m_state;

            AZStd::shared_ptr<DescriptorContext> m_descriptorContext;
        };

        template <RHI::PipelineStateType pipelineType>
        void CommandList::SetShaderResourceGroup(const ShaderResourceGroup* shaderResourceGroup)
        {
            if (AZ::RHI::Validation::IsEnabled())
            {
                if (!shaderResourceGroup)
                {
                    AZ_Assert(false, "ShaderResourceGroup assigned to draw item is null. This is not allowed.");
                    return;
                }
            }

            const uint32_t bindingSlot = shaderResourceGroup->GetBindingSlot();

            GetShaderResourceBindingsByPipelineType(pipelineType).m_srgsBySlot[bindingSlot] = shaderResourceGroup;
        }

        template <RHI::PipelineStateType pipelineType, typename Item>
        bool CommandList::CommitShaderResources(const Item& item)
        {
            ShaderResourceBindings& bindings = GetShaderResourceBindingsByPipelineType(pipelineType);

            const PipelineState* pipelineState = static_cast<const PipelineState*>(item.m_pipelineState);
            if(!pipelineState)
            {
                AZ_Assert(false, "Pipeline state not provided");
                return false;
            }
            
            bool updatePipelineState = m_state.m_pipelineState != pipelineState;
            // The pipeline state gets set first.
            if (updatePipelineState)
            {
                if (!pipelineState->IsInitialized())
                {
                    return false;
                }

                GetCommandList()->SetPipelineState(pipelineState->Get());

                const PipelineLayout* pipelineLayout = &pipelineState->GetPipelineLayout();
                // Check if we need to set custom sample positions
                if constexpr (pipelineType == RHI::PipelineStateType::Draw)
                {
                    const auto& pipelineData = pipelineState->GetPipelineStateData();
                    auto& multisampleState = pipelineData.m_drawData.m_multisampleState;
                    bool customSamplePositions = multisampleState.m_customPositionsCount > 0;
                    // Check if we need to set custom positions or reset them to the default state.
                    if (customSamplePositions || customSamplePositions != m_state.m_customSamplePositions)
                    {
                        // Need to cast to a ID3D12GraphicsCommandList1 interface in order to set custom sample positions
                        auto commandList1 = DX12ResourceCast<ID3D12GraphicsCommandList1>(GetCommandList());
                        AZ_Assert(commandList1, "Custom sample positions is not supported on this device");
                        if (commandList1)
                        {
                            if (customSamplePositions)
                            {
                                AZStd::vector<D3D12_SAMPLE_POSITION> samplePositions;
                                AZStd::transform(
                                    multisampleState.m_customPositions.begin(),
                                    multisampleState.m_customPositions.begin() + multisampleState.m_customPositionsCount,
                                    AZStd::back_inserter(samplePositions),
                                    [&](const auto& item)
                                {
                                    return ConvertSamplePosition(item);
                                });
                                commandList1->SetSamplePositions(multisampleState.m_samples, 1, samplePositions.data());
                            }
                            else
                            {
                                // This will revert the sample positions to their default values.
                                commandList1->SetSamplePositions(0, 0, NULL);
                            }
                        }
                        m_state.m_customSamplePositions = customSamplePositions;
                    }

                    SetTopology(pipelineData.m_drawData.m_primitiveTopology);
                }

                // Pipeline layouts change when pipeline states do, just not as often. If the root
                // signature changes all shader bindings are invalidated.
                if (bindings.m_pipelineLayout != pipelineLayout)
                {
                    switch (pipelineType)
                    {
                    case RHI::PipelineStateType::Draw:
                        GetCommandList()->SetGraphicsRootSignature(pipelineLayout->Get());
                        break;

                    case RHI::PipelineStateType::Dispatch:
                        GetCommandList()->SetComputeRootSignature(pipelineLayout->Get());
                        break;

                    default:
                        AZ_Assert(false, "Invalid PipelineType");
                        return false;
                    }

                    bindings.m_pipelineLayout = pipelineLayout;
                    bindings.m_hasRootConstants = pipelineLayout->HasRootConstants();

                    // We need to zero these out, since the command list root parameters are invalid.
                    for (size_t i = 0; i < bindings.m_srgsByIndex.size(); ++i)
                    {
                        bindings.m_srgsByIndex[i] = nullptr;
                    }
                }

                m_state.m_pipelineState = pipelineState;
            }

            // Assign shader resource groups from the item to slot bindings.
            for (uint32_t srgIndex = 0; srgIndex < item.m_shaderResourceGroupCount; ++srgIndex)
            {
                SetShaderResourceGroup<pipelineType>(static_cast<const ShaderResourceGroup*>(item.m_shaderResourceGroups[srgIndex]));
            }

            if (item.m_uniqueShaderResourceGroup)
            {
                SetShaderResourceGroup<pipelineType>(static_cast<const ShaderResourceGroup*>(item.m_uniqueShaderResourceGroup));
            }

            // Bind the inline constants from the item, if present.
            if (bindings.m_hasRootConstants && item.m_rootConstantSize)
            {
                AZ_Assert((item.m_rootConstantSize % 4) == 0, "Invalid inline constant data size. It must be a multiple of 32 bit.");
                switch (pipelineType)
                {
                case RHI::PipelineStateType::Draw:
                    GetCommandList()->SetGraphicsRoot32BitConstants(0, item.m_rootConstantSize / 4, item.m_rootConstants, 0);
                    break;

                case RHI::PipelineStateType::Dispatch:
                    GetCommandList()->SetComputeRoot32BitConstants(0, item.m_rootConstantSize / 4, item.m_rootConstants, 0);
                    break;

                default:
                    AZ_Assert(false, "Invalid PipelineType");
                    return false;
                }
            }

            const PipelineLayout& pipelineLayout = pipelineState->GetPipelineLayout();
            const RHI::PipelineLayoutDescriptor& pipelineLayoutDescriptor = pipelineLayout.GetPipelineLayoutDescriptor();

            // Pull from slot bindings dictated by the pipeline layout. Re-bind anything that has changed
            // at the flat index level.
            for (size_t srgIndex = 0; srgIndex < pipelineLayout.GetRootParameterBindingCount(); ++srgIndex)
            {
                const size_t srgSlot = pipelineLayout.GetSlotByIndex(srgIndex);
                const ShaderResourceGroup* shaderResourceGroup = bindings.m_srgsBySlot[srgSlot];

                if (AZ::RHI::Validation::IsEnabled())
                {
                    if (!shaderResourceGroup)
                    {
                        AZStd::string slotSrgString;
                        for (size_t slot = 0; slot < RHI::Limits::Pipeline::ShaderResourceGroupCountMax; ++slot)
                        {
                            if (bindings.m_srgsBySlot[slot])
                            {
                                if (!slotSrgString.empty())
                                {
                                    slotSrgString += ", ";
                                }

                                slotSrgString += AZStd::string::format("Slot #%zu = '%s'", slot, bindings.m_srgsBySlot[slot]->GetName().GetCStr());
                            }
                        }

                        // this assert typically happens when a shader needs a particular Srg (e.g., the ViewSrg) but the code did not bind it,
                        // check the pass code in this callstack to determine why it was not bound
                        AZ_Assert(false, "ShaderResourceGroup in slot '%d' is null at DrawItem submit time. This is not valid and means the shader is expecting an Srg that isF not currently bound in the pipeline. Current bindings: %s",
                            srgSlot,
                            slotSrgString.c_str());

                        return false;
                    }
                }

                bool updateSRG = bindings.m_srgsByIndex[srgIndex] != shaderResourceGroup;
                if (updateSRG)
                {
                    bindings.m_srgsByIndex[srgIndex] = shaderResourceGroup;

                    const ShaderResourceGroupCompiledData& compiledData = shaderResourceGroup->GetCompiledData();
                    RootParameterBinding binding = pipelineLayout.GetRootParameterBindingByIndex(srgIndex);

                    switch (pipelineType)
                    {
                    case RHI::PipelineStateType::Draw:
                        if (binding.m_resourceTable.IsValid() && compiledData.m_gpuViewsDescriptorHandle.ptr)
                        {
                            GetCommandList()->SetGraphicsRootDescriptorTable(binding.m_resourceTable.GetIndex(), compiledData.m_gpuViewsDescriptorHandle);
                        }

                        if (binding.m_constantBuffer.IsValid())
                        {
                            GetCommandList()->SetGraphicsRootConstantBufferView(binding.m_constantBuffer.GetIndex(), compiledData.m_gpuConstantAddress);
                        }

                        if (binding.m_samplerTable.IsValid() && compiledData.m_gpuSamplersDescriptorHandle.ptr)
                        {
                            GetCommandList()->SetGraphicsRootDescriptorTable(binding.m_samplerTable.GetIndex(), compiledData.m_gpuSamplersDescriptorHandle);
                        }

                        for (uint32_t unboundedArrayIndex = 0; unboundedArrayIndex < ShaderResourceGroupCompiledData::MaxUnboundedArrays; ++unboundedArrayIndex)
                        {
                            if (binding.m_unboundedArrayResourceTables[unboundedArrayIndex].IsValid() &&
                                compiledData.m_gpuUnboundedArraysDescriptorHandles[unboundedArrayIndex].ptr)
                            {
                                GetCommandList()->SetGraphicsRootDescriptorTable(
                                    binding.m_unboundedArrayResourceTables[unboundedArrayIndex].GetIndex(),
                                    compiledData.m_gpuUnboundedArraysDescriptorHandles[unboundedArrayIndex]);
                            }
                        }
                        break;

                    case RHI::PipelineStateType::Dispatch:
                        if (binding.m_resourceTable.IsValid() && compiledData.m_gpuViewsDescriptorHandle.ptr)
                        {
                            GetCommandList()->SetComputeRootDescriptorTable(binding.m_resourceTable.GetIndex(), compiledData.m_gpuViewsDescriptorHandle);
                        }

                        if (binding.m_constantBuffer.IsValid())
                        {
                            GetCommandList()->SetComputeRootConstantBufferView(binding.m_constantBuffer.GetIndex(), compiledData.m_gpuConstantAddress);
                        }

                        if (binding.m_samplerTable.IsValid() && compiledData.m_gpuSamplersDescriptorHandle.ptr)
                        {
                            GetCommandList()->SetComputeRootDescriptorTable(binding.m_samplerTable.GetIndex(), compiledData.m_gpuSamplersDescriptorHandle);
                        }

                        for (uint32_t unboundedArrayIndex = 0; unboundedArrayIndex < ShaderResourceGroupCompiledData::MaxUnboundedArrays; ++unboundedArrayIndex)
                        {
                            if (binding.m_unboundedArrayResourceTables[unboundedArrayIndex].IsValid() &&
                                compiledData.m_gpuUnboundedArraysDescriptorHandles[unboundedArrayIndex].ptr)
                            {
                                GetCommandList()->SetComputeRootDescriptorTable(
                                    binding.m_unboundedArrayResourceTables[unboundedArrayIndex].GetIndex(),
                                    compiledData.m_gpuUnboundedArraysDescriptorHandles[unboundedArrayIndex]);
                            }
                        }
                        break;

                    default:
                        AZ_Assert(false, "Invalid PipelineType");
                        return false;
                    }
                }

                if (updatePipelineState || updateSRG)
                {
                    m_validator.ValidateShaderResourceGroup(
                        *shaderResourceGroup,
                        pipelineLayoutDescriptor.GetShaderResourceGroupBindingInfo(srgIndex));
                }
            }
            return true;
        }
    }
}
