/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/CommandListBase.h>
#include <RHI/DescriptorContext.h>
#include <RHI/PipelineLayout.h>
#include <RHI/PipelineState.h>
#include <RHI/MemoryView.h>
#include <RHI/ShaderResourceGroup.h>
#include <RHI/Conversions.h>
#include <Atom/RHI.Reflect/ClearValue.h>
#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/CommandListValidator.h>
#include <Atom/RHI/CommandListStates.h>
#include <Atom/RHI/DeviceGeometryView.h>
#include <Atom/RHI/DeviceIndirectArguments.h>
#include <Atom/RHI/ObjectPool.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/array.h>

#define DX12_GPU_PROFILE_MODE_OFF       0       // Turn off profiling
#define DX12_GPU_PROFILE_MODE_BASIC     1       // Profiles command list lifetime
#define DX12_GPU_PROFILE_MODE_DETAIL    2       // Profiles draw call state changes
#define DX12_GPU_PROFILE_MODE DX12_GPU_PROFILE_MODE_BASIC
#define PIX_MARKER_CMDLIST_COL 0xFF00FF00

namespace AZ
{
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
            AZ_CLASS_ALLOCATOR(CommandList, AZ::SystemAllocator);

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
            void SetShaderResourceGroupForDraw(const RHI::DeviceShaderResourceGroup& shaderResourceGroup) override;
            void SetShaderResourceGroupForDispatch(const RHI::DeviceShaderResourceGroup& shaderResourceGroup) override;
            void Submit(const RHI::DeviceDrawItem& drawItem, uint32_t submitIndex = 0) override;
            void Submit(const RHI::DeviceCopyItem& copyItem, uint32_t submitIndex = 0) override;
            void Submit(const RHI::DeviceDispatchItem& dispatchItem, uint32_t submitIndex = 0) override;
            void Submit(const RHI::DeviceDispatchRaysItem& dispatchRaysItem, uint32_t submitIndex = 0) override;
            void BeginPredication(const RHI::DeviceBuffer& buffer, uint64_t offset, RHI::PredicationOp operation) override;
            void EndPredication() override;
            void BuildBottomLevelAccelerationStructure(const RHI::DeviceRayTracingBlas& rayTracingBlas) override;
            void UpdateBottomLevelAccelerationStructure(const RHI::DeviceRayTracingBlas& rayTracingBlas) override;
            void BuildTopLevelAccelerationStructure(
                const RHI::DeviceRayTracingTlas& rayTracingTlas, const AZStd::vector<const RHI::DeviceRayTracingBlas*>& changedBlasList) override;
            void QueryBlasCompactionSizes(
                const AZStd::vector<AZStd::pair<RHI::DeviceRayTracingBlas*, RHI::DeviceRayTracingCompactionQuery*>>& blasToQuery) override;
            void CompactBottomLevelAccelerationStructure(
                const RHI::DeviceRayTracingBlas& sourceBlas, const RHI::DeviceRayTracingBlas& compactBlas) override;
            void SetFragmentShadingRate(
                RHI::ShadingRate rate,
                const RHI::ShadingRateCombinators& combinators = DefaultShadingRateCombinators) override;
            //////////////////////////////////////////////////////////////////////////

            void SetRenderTargets(
                uint32_t renderTargetCount,
                const ImageView* const* renderTarget,
                const ImageView* depthStencilAttachment,
                RHI::ScopeAttachmentAccess depthStencilAccess,
                const ImageView* shadingRateAttachment);

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

                AZStd::vector<D3D12_TILE_RANGE_FLAGS> m_rangeFlags;     // pRangeFlags in UpdateTileMappings 
                AZStd::vector<uint32_t> m_rangeStartOffsets;            // pHeapRangeStartOffsets in UpdateTileMappings 
                AZStd::vector<uint32_t> m_rangeTileCounts;              // pRangeTileCounts in UpdateTileMappings 
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

            void SetStreamBuffers(const RHI::DeviceGeometryView& geometryView, const RHI::StreamBufferIndices& streamIndices);
            void SetIndexBuffer(const RHI::DeviceIndexBufferView& descriptor);
            void SetStencilRef(uint8_t stencilRef);
            void SetTopology(RHI::PrimitiveTopology topology);
            void CommitViewportState();
            void CommitScissorState();
            void CommitShadingRateState();

            void ExecuteIndirect(const RHI::DeviceIndirectArguments& arguments);

            RHI::CommandListValidator m_validator;

            struct ShaderResourceBindings
            {
                const PipelineLayout* m_pipelineLayout = nullptr;
                AZStd::array<const ShaderResourceGroup*, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_srgsByIndex;
                AZStd::array<const ShaderResourceGroup*, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_srgsBySlot;
                bool m_hasRootConstants = false;
                int m_bindlessHeapLastIndex = -1;
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

                const RHI::DevicePipelineState* m_pipelineState = nullptr;

                // Graphics-specific state
                AZStd::array<uint64_t, RHI::Limits::Pipeline::StreamCountMax> m_streamBufferHashes = {{}};
                uint64_t m_indexBufferHash = 0;
                uint32_t m_stencilRef = static_cast<uint32_t>(-1);
                RHI::PrimitiveTopology m_topology = RHI::PrimitiveTopology::Undefined;
                RHI::CommandListViewportState m_viewportState;
                RHI::CommandListScissorState m_scissorState;
                RHI::CommandListShadingRateState m_shadingRateState;

                // Array of shader resource bindings, indexed by command pipe.
                AZStd::array<ShaderResourceBindings, static_cast<size_t>(RHI::PipelineStateType::Count)> m_bindingsByPipe;

                // The command queue assigned to execute the command list.
                CommandQueue* m_parentQueue = nullptr;

                // A queue of tile mappings to execute on the command queue at submission time (prior to executing the command list).
                TileMapRequestList m_tileMapRequests;

                // The currently bound shading rate image
                const ImageView* m_shadingRateImage = nullptr;

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
    }
}
