/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/CommandListStates.h>
#include <Atom/RHI/DeviceGeometryView.h>
#include <Atom/RHI/ObjectPool.h>
#include <Atom/RHI/PipelineStateDescriptor.h>
#include <Atom/RHI/ScopeAttachment.h>
#include <Atom/RHI/DeviceShaderResourceGroup.h>
#include <Atom/RHI.Reflect/ClearValue.h>
#include <Atom/RHI.Reflect/ShaderStages.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayout.h>
#include <RHI/CommandListBase.h>
#include <RHI/PipelineLayout.h>
#include <RHI/PipelineState.h>
#include <RHI/ShaderResourceGroup.h>

namespace AZ
{
    namespace Metal
    {
        class SwapChain;
        class Device;
        class ImageView;
        class PipelineState;
        
        class CommandList
            : public RHI::CommandList
            , public CommandListBase
        {
        public:
            AZ_CLASS_ALLOCATOR(CommandList, AZ::SystemAllocator);
            using MetalArgumentBufferArray = AZStd::array<id<MTLBuffer>, RHI::Limits::Pipeline::ShaderResourceGroupCountMax>;
            using MetalArgumentBufferArrayOffsets = AZStd::array<NSUInteger, RHI::Limits::Pipeline::ShaderResourceGroupCountMax>;
            
            //first = is the resource read only
            //second = native mtlresource pointer
            using ResourceProperties = AZStd::pair<bool, id<MTLResource>>;
            
            static RHI::Ptr<CommandList> Create();
            
            void Init(RHI::HardwareQueueClass hardwareQueueClass, Device* device);
            void Shutdown();

            //////////////////////////////////////////////////////////////////////////
            // CommandListBase
            void Close() override;
            void Reset() override;
            void FlushEncoder() override;
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
            void BeginPredication(const RHI::DeviceBuffer& buffer, uint64_t offset, RHI::PredicationOp operation) override {}
            void EndPredication() override {}
            void BuildBottomLevelAccelerationStructure(const RHI::DeviceRayTracingBlas &rayTracingBlas) override;
            void UpdateBottomLevelAccelerationStructure(const RHI::DeviceRayTracingBlas& rayTracingBlas) override;
            void QueryBlasCompactionSizes(
                const AZStd::vector<AZStd::pair<RHI::DeviceRayTracingBlas *, RHI::DeviceRayTracingCompactionQuery *>> &blasToQuery) override;
            void CompactBottomLevelAccelerationStructure(
                const RHI::DeviceRayTracingBlas &sourceBlas,
                const RHI::DeviceRayTracingBlas &compactBlas) override;
            void BuildTopLevelAccelerationStructure(
                const RHI::DeviceRayTracingTlas &rayTracingTlas,
                const AZStd::vector<const RHI::DeviceRayTracingBlas *> &changedBlasList) override;
            void SetFragmentShadingRate(
                [[maybe_unused]] RHI::ShadingRate rate,
                [[maybe_unused]] const RHI::ShadingRateCombinators& combinators = DefaultShadingRateCombinators) override {}

        private:
            
            void SetPipelineState(const PipelineState* pipelineState);
            void SetStreamBuffers(const RHI::DeviceGeometryView& geometryView, const RHI::StreamBufferIndices& streamIndices);
            void SetIndexBuffer(const RHI::DeviceIndexBufferView& descriptor);
            void SetStencilRef(AZ::u8 stencilRef);
            void SetRasterizerState(const RasterizerState& rastState);
                        
            bool SetArgumentBuffers(const PipelineState* pipelineState, RHI::PipelineStateType stateType);
            bool IsSRGBoundToStage(uint32_t srgIndex, uint32_t shaderStage);
            
            template <typename Item>
            void SetRootConstants(const Item& item, const PipelineState* pipelineState);
            
            template <RHI::PipelineStateType>
            void SetShaderResourceGroup(const ShaderResourceGroup* shaderResourceGroup);

            // Uses templates to remove branching. Specialized between draw / dispatch paths.
            // Binds the pipeline state / pipeline layout, then the shader resources associated with a
            // draw / dispatch call.
            template <RHI::PipelineStateType, typename Item>
            bool CommitShaderResources(const Item& item);

            void CommitViewportState();
            void CommitScissorState();
                        
            struct ShaderResourceBindings
            {
                AZStd::array<const ShaderResourceGroup*, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_srgsByIndex;
                AZStd::array<const ShaderResourceGroup*, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_srgsBySlot;
                AZStd::array<AZ::HashValue64, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_srgVisHashByIndex;
            };
                        
            void BindArgumentBuffers(RHI::ShaderStage shaderStage,
                                     uint16_t registerIdMin,
                                     uint16_t registerIdMax,
                                     MetalArgumentBufferArray& mtlArgBuffers,
                                     MetalArgumentBufferArrayOffsets mtlArgBufferOffsets);
            
            //! Helper functions that help cache untracked resources (within Bindless SRG) for compute and graphics work
            void CollectBindlessComputeUntrackedResources(const ShaderResourceGroup* shaderResourceGroup,
                                                          ArgumentBuffer::ResourcesForCompute& untrackedResourceComputeRead,
                                                          ArgumentBuffer::ResourcesForCompute& untrackedResourceComputeReadWrite);
            
            void CollectBindlessGfxUntrackedResources(const ShaderResourceGroup* shaderResourceGroup,
                                                      ArgumentBuffer::ResourcesPerStageForGraphics& untrackedResourcesGfxRead,
                                                      ArgumentBuffer::ResourcesPerStageForGraphics& untrackedResourcesGfxReadWrite);
            //! Helper function to return a bool to indicate if the resource is read only as well as the native resource pointer
            AZStd::pair<bool, id<MTLResource>> GetResourceInfo(RHI::BindlessResourceType resourceType,
                                                               const RHI::DeviceResourceView* resourceView);
            
            //Returns the SRG binding based on the PSO type
            ShaderResourceBindings& GetShaderResourceBindingsByPipelineType(RHI::PipelineStateType pipelineType);
            
            //Arrays to cache all the buffers and offsets (related to graphics work) in order to make batch calls
            MetalArgumentBufferArray m_mtlVertexArgBuffers;
            MetalArgumentBufferArrayOffsets m_mtlVertexArgBufferOffsets;
            MetalArgumentBufferArray m_mtlFragmentOrComputeArgBuffers;
            MetalArgumentBufferArrayOffsets m_mtlFragmentOrComputeArgBufferOffsets;

            //! This is kept as a separate struct so that we can robustly reset it. Every property
            //! on this struct should be in-class-initialized so that there are no "missed" states.
            //! Otherwise, it results in hard-to-track bugs down the road as it's too easy to add something
            //! here and then miss adding the initialization elsewhere.
            struct State
            {
                State() = default;
                
                // Draw State
                const RHI::DevicePipelineState* m_pipelineState = nullptr;
                const PipelineLayout* m_pipelineLayout = nullptr;
                AZStd::array<AZ::HashValue64, RHI::Limits::Pipeline::StreamCountMax> m_streamsHashes = {AZ::HashValue64{0}};
                
                AZ::HashValue64 m_rasterizerStateHash = AZ::HashValue64{0};
                uint64_t m_depthStencilStateHash = 0;
                uint32_t m_stencilRef = static_cast<uint32_t>(-1);
                RHI::CommandListScissorState m_scissorState;
                RHI::CommandListViewportState m_viewportState;
                
                RHI::Viewport m_viewport;
                // Array of shader resource bindings, indexed by command pipe.
                AZStd::array<ShaderResourceBindings, static_cast<size_t>(RHI::PipelineStateType::Count)> m_bindingsByPipe;

                // Signal that the global bindless heap is bound
                bool m_bindBindlessHeap = false;

            } m_state;
        };
        
        template <RHI::PipelineStateType pipelineType>
        void CommandList::SetShaderResourceGroup(const ShaderResourceGroup* shaderResourceGroup)
        {
#if defined (AZ_RHI_ENABLE_VALIDATION)
            if (!shaderResourceGroup)
            {
                AZ_Assert(false, "ShaderResourceGroup assigned to draw item is null. This is not allowed.");
                return;
            }
#endif
            
            const uint32_t bindingSlot = shaderResourceGroup->GetBindingSlot();
            AZ_Assert(bindingSlot<RHI::Limits::Pipeline::ShaderResourceGroupCountMax, "Binding slot higher than allowed.");
            GetShaderResourceBindingsByPipelineType(pipelineType).m_srgsBySlot[bindingSlot] = shaderResourceGroup;
        }
    }
}
