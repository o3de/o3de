/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/CommandListValidator.h>
#include <Atom/RHI/CommandListStates.h>
#include <Atom/RHI/DeviceObject.h>
#include <Atom/RHI/DeviceGeometryView.h>
#include <Atom/RHI/PipelineStateDescriptor.h>
#include <Atom/RHI/Resource.h>
#include <Atom/RHI/DeviceRayTracingAccelerationStructure.h>
#include <Atom/RHI.Reflect/ImageScopeAttachmentDescriptor.h>
#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI.Reflect/ScopeId.h>
#include <Atom/RHI.Reflect/Interval.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/bitset.h>
#include <RHI/PipelineState.h>

namespace AZ
{
    namespace Vulkan
    {
        class CommandPool;
        class CommandBuffer;
        class Device;
        class PipelineState;
        class PipelineLayout;
        class ShaderResourceGroup;
        class SwapChain;
        class Framebuffer;
        class RenderPass;

        class CommandList final
            : public RHI::CommandList, 
            public RHI::DeviceObject 
        {
            using Base = RHI::CommandList;

            friend class CommandPool;

        public:
            AZ_CLASS_ALLOCATOR(CommandList, AZ::SystemAllocator);
            AZ_RTTI(CommandList, "138BB654-124A-47F7-8426-9ED2204BCDBD", Base);

            struct InheritanceInfo
            {
                const Framebuffer* m_frameBuffer = nullptr;
                uint32_t m_subpass = 0;
            };

            struct BeginRenderPassInfo
            {
                const Framebuffer* m_frameBuffer = nullptr;
                AZStd::vector<RHI::ClearValue> m_clearValues;
                VkSubpassContents m_subpassContentType = VK_SUBPASS_CONTENTS_INLINE;
            };

            struct ResourceClearRequest
            {
                RHI::ClearValue m_clearValue;
                const RHI::ResourceView* m_resourceView = nullptr;
            };

            ~CommandList() = default;

            VkCommandBuffer GetNativeCommandBuffer() const;

            ///////////////////////////////////////////////////////////////////
            // RHI::CommandList
            void SetViewports(const RHI::Viewport* rhiViewports, uint32_t count) override;
            void SetScissors(const RHI::Scissor* rhiScissors, uint32_t count) override;
            void SetShaderResourceGroupForDraw(const RHI::DeviceShaderResourceGroup& shaderResourceGroup) override;
            void SetShaderResourceGroupForDispatch(const RHI::DeviceShaderResourceGroup& shaderResourceGroup) override;
            void Submit(const RHI::DeviceCopyItem& copyItems, uint32_t submitIndex = 0) override;
            void Submit(const RHI::DeviceDrawItem& itemList, uint32_t submitIndex = 0) override;
            void Submit(const RHI::DeviceDispatchItem& dispatchItems, uint32_t submitIndex = 0) override;
            void Submit(const RHI::DeviceDispatchRaysItem& dispatchRaysItem, uint32_t submitIndex = 0) override;
            void BeginPredication(const RHI::DeviceBuffer& buffer, uint64_t offset, RHI::PredicationOp operation) override;
            void EndPredication() override;
            void BuildBottomLevelAccelerationStructure(const RHI::DeviceRayTracingBlas& rayTracingBlas) override;
            void UpdateBottomLevelAccelerationStructure(const RHI::DeviceRayTracingBlas& rayTracingBlas) override;
            void QueryBlasCompactionSizes(
                const AZStd::vector<AZStd::pair<RHI::DeviceRayTracingBlas*, RHI::DeviceRayTracingCompactionQuery*>>& blasToQuery) override;
            void CompactBottomLevelAccelerationStructure(
                const RHI::DeviceRayTracingBlas& sourceBlas, const RHI::DeviceRayTracingBlas& compactBlas) override;
            void BuildTopLevelAccelerationStructure(
                const RHI::DeviceRayTracingTlas& rayTracingTlas, const AZStd::vector<const RHI::DeviceRayTracingBlas*>& changedBlasList) override;
            void SetFragmentShadingRate(
                RHI::ShadingRate rate,
                const RHI::ShadingRateCombinators& combinators = DefaultShadingRateCombinators) override;
            ////////////////////////////////////////////////////////////

            ///////////////////////////////////////////////////////////////////
            // RHI::DeviceObject
            void Shutdown() override;
            ///////////////////////////////////////////////////////////////////

            void BeginCommandBuffer(InheritanceInfo* inheritance = nullptr);
            void EndCommandBuffer();

            void BeginRenderPass(const BeginRenderPassInfo& beginInfo);
            void NextSubpass(VkSubpassContents contents);
            void EndRenderPass();
            bool IsInsideRenderPass() const;
            const Framebuffer* GetActiveFramebuffer() const;
            const RenderPass* GetActiveRenderpass() const;
            void ExecuteSecondaryCommandLists(const AZStd::span<const RHI::Ptr<CommandList>>& commands);

            uint32_t GetQueueFamilyIndex() const;

            void BeginDebugLabel(const char* label, AZ::Color color = Debug::DefaultLabelColor);
            void EndDebugLabel();

            void ClearImage(const ResourceClearRequest& request);
            void ClearBuffer(const ResourceClearRequest& request);

            RHI::CommandListValidator& GetValidator();

        private:
            struct Descriptor
            {
                Device* m_device = nullptr;
                CommandPool* m_commandPool = nullptr;
                VkCommandBufferLevel m_level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            };

            struct ShaderResourceBindings
            {
                const PipelineState* m_pipelineState = nullptr;
                AZStd::array<const ShaderResourceGroup*, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_SRGByAzslBindingSlot = { {} };
                // Used for better debugging (VS Natvis)
                AZStd::array<const ShaderResourceGroup*, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_SRGByVulkanBindingIndex = { {} };
                AZStd::array<VkDescriptorSet, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_descriptorSets = { {VK_NULL_HANDLE} };
                AZStd::bitset<RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_dirtyShaderResourceGroupFlags;
            };

            struct State
            {
                // Array of shader resource bindings, indexed by command pipe.
                AZStd::array<ShaderResourceBindings, static_cast<size_t>(RHI::PipelineStateType::Count)> m_bindingsByPipe;

                // Graphics-specific state
                AZStd::array<uint64_t, RHI::Limits::Pipeline::StreamCountMax> m_streamBufferHashes = { {} };
                uint64_t m_indexBufferHash = 0;
                uint32_t m_subpassIndex = 0;
                const Framebuffer* m_framebuffer = nullptr;
                RHI::CommandListScissorState m_scissorState;
                RHI::CommandListViewportState m_viewportState;
                RHI::CommandListShadingRateState m_shadingRateState;
            };

            CommandList() = default;
            static RHI::Ptr<CommandList> Create();
            RHI::ResultCode Init(const Descriptor& descriptor);

            void Reset();

            RHI::ResultCode BuildNativeCommandBuffer();

            void SetShaderResourceGroup(const RHI::DeviceShaderResourceGroup& shaderResourceGroup, RHI::PipelineStateType type);
            void SetStreamBuffers(const RHI::DeviceGeometryView& geometryView, const RHI::StreamBufferIndices& streamIndices);
            void SetIndexBuffer(const RHI::DeviceIndexBufferView& indexBufferView);
            void SetStencilRef(uint8_t stencilRef);
            void BindPipeline(const PipelineState* pipelineState);
            void CommitViewportState();
            void CommitScissorState();
            void CommitShadingRateState();

            template <class Item>
            bool CommitShaderResource(const Item& item);
            void CommitShaderResourcePushConstants(VkPipelineLayout pipelineLayout, uint8_t rootConstantSize, const uint8_t* rootConstants);
            void CommitDescriptorSets(RHI::PipelineStateType type);
            ShaderResourceBindings& GetShaderResourceBindingsByPipelineType(RHI::PipelineStateType type);
            VkPipelineBindPoint GetPipelineBindPoint(const PipelineState& pipelineState) const;

            void ValidateShaderResourceGroups(RHI::PipelineStateType type) const;

            Descriptor m_descriptor;
            VkCommandBuffer m_nativeCommandBuffer = VK_NULL_HANDLE;
            bool m_isUpdating = false; // it indicates between BeginCommandBuffer() and EndCommandBuffer().
            State m_state;
            bool m_supportsPredication = false;
            bool m_supportsDrawIndirectCount = false;
            RHI::CommandListValidator m_validator;
        };

        template<class Item>
        bool CommandList::CommitShaderResource(const Item& item)
        {
            const PipelineState* pipelineState = static_cast<const PipelineState*>(item.m_pipelineState);
            AZ_Assert(pipelineState, "Pipeline state is null.");
            if(!pipelineState)
            {
                AZ_Warning("CommandList", false, "Pipeline state is null.");
                return false;
            }
            
            AZ_Assert(pipelineState->GetPipelineLayout(), "Pipeline layout is null.");
            if(!pipelineState->GetPipelineLayout())
            {
                AZ_Warning("CommandList", false, "Pipeline layout is null.");
                return false;
            }
            
            // Set the pipeline state first
            BindPipeline(pipelineState);
            const RHI::PipelineStateType pipelineType = pipelineState->GetType();

            // Assign shader resource groups from the item to slot bindings.
            for (uint32_t srgIndex = 0; srgIndex < item.m_shaderResourceGroupCount; ++srgIndex)
            {
                SetShaderResourceGroup(*item.m_shaderResourceGroups[srgIndex], pipelineType);
            }

            // Set per draw/dispatch SRGs.
            if (item.m_uniqueShaderResourceGroup)
            {
                SetShaderResourceGroup(*item.m_uniqueShaderResourceGroup, pipelineType);
            }

            ValidateShaderResourceGroups(pipelineType);

            // Set Descriptor Sets based on the assigned SRGs.
            CommitDescriptorSets(pipelineType);

            // Set push constants values if needed.
            auto pipelineLayout = pipelineState->GetPipelineLayout();
            if (item.m_rootConstantSize && pipelineLayout->GetPushContantsSize() > 0)
            {
                CommitShaderResourcePushConstants(pipelineLayout->GetNativePipelineLayout(), item.m_rootConstantSize, item.m_rootConstants);
            }

            m_state.m_bindingsByPipe[static_cast<uint32_t>(pipelineType)].m_dirtyShaderResourceGroupFlags.reset();
            return true;
        }
    }
}
