/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/PipelineState.h>
#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/CommandListStates.h>
#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI/DeviceObject.h>
#include <Atom/RHI/PipelineStateDescriptor.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/bitset.h>

namespace AZ::WebGPU
{
    class BindGroup;
    class Device;
    class ShaderResourceGroup;
    class BufferView;

    class CommandList final
        : public RHI::CommandList
        , public RHI::DeviceObject 
    {
        using Base = RHI::CommandList;
    public:
        AZ_CLASS_ALLOCATOR(CommandList, AZ::SystemAllocator);
        AZ_RTTI(CommandList, "{32B7EE7F-8EC1-4101-A6EB-F0D7AF67C88B}", Base);

        static RHI::Ptr<CommandList> Create();

        void Init(Device& device);

        //! Begin the CommandEncoder of the CommandList
        void Begin();
        //! Ends the CommandEncoder of the CommandList
        void End();

        //! Begins a renderpass
        void BeginRenderPass(const wgpu::RenderPassDescriptor& descriptor);
        //! Ends a renderpass
        void EndRenderPass();

        //! Begins a computepass
        void BeginComputePass();
        //! Ends a computepass
        void EndComputePass();

        //! Returns the CommandBuffer. Must Begin/End the command list in order to have
        //! a valid CommandBuffer
        wgpu::CommandBuffer& GetNativeCommandBuffer();

        //////////////////////////////////////////////////////////////////////////
        // RHI::CommandList
        void SetViewports(const RHI::Viewport* viewports, uint32_t count) override;
        void SetScissors(const RHI::Scissor* scissors, uint32_t count) override;
        void SetShaderResourceGroupForDraw(const RHI::DeviceShaderResourceGroup& shaderResourceGroup) override;
        void SetShaderResourceGroupForDispatch(const RHI::DeviceShaderResourceGroup& shaderResourceGroup) override;
        void Submit(const RHI::DeviceDrawItem& drawItem, uint32_t submitIndex = 0) override;
        void Submit(const RHI::DeviceCopyItem& copyItem, uint32_t submitIndex = 0) override;
        void Submit(const RHI::DeviceDispatchItem& dispatchItem, uint32_t submitIndex = 0) override;
        void Submit([[maybe_unused]] const RHI::DeviceDispatchRaysItem& dispatchRaysItem, [[maybe_unused]] uint32_t submitIndex = 0) override {}
        void BeginPredication([[maybe_unused]] const RHI::DeviceBuffer& buffer, [[maybe_unused]] uint64_t offset, [[maybe_unused]] RHI::PredicationOp operation) override {}
        void EndPredication() override {}
        void BuildBottomLevelAccelerationStructure([[maybe_unused]] const RHI::DeviceRayTracingBlas& rayTracingBlas) override {}
        void UpdateBottomLevelAccelerationStructure([[maybe_unused]] const RHI::DeviceRayTracingBlas& rayTracingBlas) override {}
        void BuildTopLevelAccelerationStructure([[maybe_unused]] const RHI::DeviceRayTracingTlas& rayTracingTlas, [[maybe_unused]] const AZStd::vector<const RHI::DeviceRayTracingBlas*>& changedBlasList) override {}
        void SetFragmentShadingRate(
            [[maybe_unused]] RHI::ShadingRate rate,
            [[maybe_unused]] const RHI::ShadingRateCombinators& combinators = DefaultShadingRateCombinators) override {}
        /////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////
        // RHI::DeviceObject
        void Shutdown() override;
        ///////////////////////////////////////////////////////////////////

    private:
        //! Binding info for a pipeline type
        struct ShaderResourceBindings
        {
            const PipelineState* m_pipelineState = nullptr;
            AZStd::array<const ShaderResourceGroup*, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_SRGByAzslBindingSlot = { {} };
            AZStd::array<WGPUBindGroup, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_bindGroups = { { nullptr } };
            const BindGroup* m_rootConstantBindGroup = nullptr;
            const BufferView* m_rootConstantBufferView = nullptr;
            uint32_t m_rootConstantOffset = 0;
        };

        template<class Item>
        bool CommitShaderResource(const Item& item);
        void CommitViewportState();
        void CommitScissorState();
        void CommitBindGroups(RHI::PipelineStateType type);
        void CommitRootConstants(RHI::PipelineStateType type, uint8_t rootConstantSize, const uint8_t* rootConstants);
        void SetStreamBuffers(const RHI::DeviceGeometryView& geometryView, const RHI::StreamBufferIndices& streamIndices);
        void SetIndexBuffer(const RHI::DeviceIndexBufferView& indexBufferView);
        void SetStencilRef(uint8_t stencilRef);
        void SetShaderResourceGroup(const RHI::DeviceShaderResourceGroup& shaderResourceGroup, RHI::PipelineStateType type);
        bool BindPipeline(const PipelineState* pipelineState);
        ShaderResourceBindings& GetShaderResourceBindingsByPipelineType(RHI::PipelineStateType type);

        void ValidateShaderResourceGroups(RHI::PipelineStateType type) const;

        struct State
        {
            //! Array of shader resource bindings, indexed by command pipe.
            AZStd::array<ShaderResourceBindings, static_cast<size_t>(RHI::PipelineStateType::Count)> m_bindingsByPipe;

            // Graphics-specific state
            AZStd::array<uint64_t, RHI::Limits::Pipeline::StreamCountMax> m_streamBufferHashes = { {} };
            uint64_t m_indexBufferHash = 0;
            RHI::CommandListScissorState m_scissorState;
            RHI::CommandListViewportState m_viewportState;
            wgpu::RenderPassEncoder m_wgpuRenderPassEncoder = nullptr;

            // Compute-specific state
            wgpu::ComputePassEncoder m_wgpuComputePassEncoder = nullptr;
        };

        State m_state;

        // Common state
        wgpu::CommandEncoder m_wgpuCommandEncoder = nullptr;
        wgpu::CommandBuffer m_wgpuCommandBuffer = nullptr;
    };

    template<class Item>
    bool CommandList::CommitShaderResource(const Item& item)
    {
        const PipelineState* pipelineState = static_cast<const PipelineState*>(item.m_pipelineState);
        AZ_Assert(pipelineState, "Pipeline state is null.");
        if (!pipelineState)
        {
            return false;
        }

        AZ_Assert(pipelineState->GetPipelineLayout(), "Pipeline layout is null.");
        if (!pipelineState->GetPipelineLayout())
        {
            return false;
        }

        // Set the pipeline state first
        if (!BindPipeline(pipelineState))
        {
            return false;
        }
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

        // Set root constants values if needed.
        auto pipelineLayout = pipelineState->GetPipelineLayout();
        if (item.m_rootConstantSize && pipelineLayout->GetRootConstantSize() > 0)
        {
            CommitRootConstants(pipelineType, item.m_rootConstantSize, item.m_rootConstants);
        }

        // Set BindGroups based on the assigned SRGs.
        CommitBindGroups(pipelineType);

        return true;
    }
}
