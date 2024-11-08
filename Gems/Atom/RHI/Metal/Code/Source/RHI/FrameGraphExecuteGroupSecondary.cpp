/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/ScopeId.h>
#include <Atom/RHI/FrameGraph.h>
#include <RHI/CommandList.h>
#include <RHI/Device.h>
#include <RHI/FrameGraphExecuteGroupSecondary.h>
#include <RHI/Scope.h>
#include <RHI/SwapChain.h>

namespace AZ::Metal
{
    void FrameGraphExecuteGroupSecondary::Init(
        Device& device,
        Scope& scope,
        uint32_t commandListCount,
        RHI::JobPolicy globalJobPolicy)
    {
        Base::InitBase(device, scope.GetFrameGraphGroupId(), scope.GetHardwareQueueClass());
        m_scope = &scope;
        
        m_workRequest.m_waitFenceValues = scope.GetWaitFences();
        m_workRequest.m_signalFenceValue = scope.GetSignalFenceValue();
        m_workRequest.m_commandLists.resize(commandListCount);
        m_workRequest.m_swapChainsToPresent.reserve(scope.GetSwapChainsToPresent().size());
        for (RHI::DeviceSwapChain* swapChain : scope.GetSwapChainsToPresent())
        {
            m_workRequest.m_swapChainsToPresent.push_back(static_cast<SwapChain*>(swapChain));
        }

        auto& fencesToSignal = m_workRequest.m_scopeFencesToSignal;
        fencesToSignal.reserve(scope.GetFencesToSignal().size());
        for (const RHI::Ptr<RHI::Fence>& fence : scope.GetFencesToSignal())
        {
            fencesToSignal.push_back(&static_cast<FenceImpl&>(*fence->GetDeviceFence(scope.GetDeviceIndex())).Get());
        }
        
        InitRequest request;
        request.m_scopeId = scope.GetId();
        request.m_deviceIndex = scope.GetDeviceIndex();
        request.m_submitCount = scope.GetEstimatedItemCount();
        request.m_commandLists = reinterpret_cast<RHI::CommandList* const*>(m_workRequest.m_commandLists.data());
        request.m_commandListCount = commandListCount;
        request.m_jobPolicy = globalJobPolicy;
        Base::Init(request);
    }

    void FrameGraphExecuteGroupSecondary::BeginInternal()
    {
        Base::BeginInternal();
        AZ_Assert(m_commandBuffer->GetMtlCommandBuffer(), "Metal command Buffer was not created");
        m_workRequest.m_commandBuffer = m_commandBuffer;
        m_scope->SetRenderPassInfo(m_renderPassContext);
    }

    void FrameGraphExecuteGroupSecondary::BeginContextInternal(RHI::FrameGraphExecuteContext& context, uint32_t contextIndex)
    {
        Base::BeginContextInternal(context, contextIndex);
        CommandList* commandList = m_subRenderEncoders[contextIndex].m_commandList;
        commandList->Open(m_subRenderEncoders[contextIndex].m_subRenderEncoder, m_commandBuffer->GetMtlCommandBuffer());
        m_workRequest.m_commandLists[contextIndex] = commandList;
        context.SetCommandList(*commandList);

        m_scope->Begin(*static_cast<CommandList*>(context.GetCommandList()), context.GetCommandListIndex(), context.GetCommandListCount());
    }

    void FrameGraphExecuteGroupSecondary::EndContextInternal(RHI::FrameGraphExecuteContext& context, uint32_t contextIndex)
    {
        m_scope->End(*static_cast<CommandList*>(context.GetCommandList()));
        static_cast<CommandList*>(context.GetCommandList())->Close();
        Base::EndContextInternal(context, contextIndex);
    }

    void FrameGraphExecuteGroupSecondary::EndInternal()
    {
        Base::EndInternal();
    }

    AZStd::span<const Scope* const> FrameGraphExecuteGroupSecondary::GetScopes() const
    {
        return AZStd::span<const Scope* const>(&m_scope, 1);
    }

    AZStd::span<Scope* const> FrameGraphExecuteGroupSecondary::GetScopes()
    {
        return AZStd::span<Scope* const>(&m_scope, 1);
    }

    void FrameGraphExecuteGroupSecondary::SetRenderContext(const RenderPassContext& renderPassContext)
    {
        m_renderPassContext = renderPassContext;
    }

    const Scope* FrameGraphExecuteGroupSecondary::GetScope() const
    {
        return m_scope;
    }

    void FrameGraphExecuteGroupSecondary::EncondeAllWaitEvents() const
    {
        //Encode any wait events from the attached scope at the start of the group.
        EncodeWaitEvents();
                  
        //Wait on all the fences related to transient resources, before the
        //encoders are created as per driver specs
        m_scope->WaitOnAllResourceFences(m_commandBuffer->GetMtlCommandBuffer());
    }

    void FrameGraphExecuteGroupSecondary::EncodeAllSignalEvents() const
    {
        //Signal all the fences related to transient resources, after the encoders
        //are flushed as per driver specs
        m_scope->SignalAllResourceFences(m_commandBuffer->GetMtlCommandBuffer());
    }

    void FrameGraphExecuteGroupSecondary::CreateSecondaryEncoders()
    {
        for(int i = 0 ; i < m_workRequest.m_commandLists.size(); i++)
        {
            CommandList* commandList = AcquireCommandList();
            id<MTLCommandEncoder> subRenderEncoder = m_commandBuffer->AcquireSubRenderEncoder(m_renderPassContext.m_renderPassDescriptor, m_scope->GetId().GetCStr());
            m_subRenderEncoders.push_back(SubEncoderData {commandList, subRenderEncoder});
        }
    }
}
