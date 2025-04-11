/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Device.h>
#include <RHI/FrameGraphExecuteGroupPrimary.h>
#include <RHI/Scope.h>
#include <RHI/SwapChain.h>

namespace AZ::Metal
{
    void FrameGraphExecuteGroupPrimary::Init(
        Device& device,
        AZStd::vector<Scope*>&& scopes)
    {
        AZ_Assert(!scopes.empty(), "Empty list of scopes for Merged group");
        // Use the max graphGroup id as the id of the execute group.
        RHI::GraphGroupId groupId = scopes.back()->GetFrameGraphGroupId();
        AZStd::for_each(scopes.begin(), scopes.end(), [&groupId](const Scope* scope)
        {
            groupId = AZStd::max(groupId, scope->GetFrameGraphGroupId());
        });

        Base::InitBase(device, groupId, scopes.back()->GetHardwareQueueClass());
        
        m_scopes = AZStd::move(scopes);
        m_workRequest.m_commandLists.resize(1);
        
        // We split scopes based on fence boundary and hence we only neeed to wait for any relevant
        // fences on the top scope and signal fence for the last scope.
        m_workRequest.m_waitFenceValues = m_scopes.front()->GetWaitFences();
        m_workRequest.m_signalFenceValue = m_scopes.back()->GetSignalFenceValue();
        
        AZStd::vector<InitMergedRequest::ScopeEntry> scopeEntries;
        scopeEntries.reserve(m_scopes.size());
        for (const Scope* scope : m_scopes)
        {
            scopeEntries.push_back({ scope->GetId(), scope->GetEstimatedItemCount() });
            
            m_workRequest.m_swapChainsToPresent.reserve(m_workRequest.m_swapChainsToPresent.size() + scope->GetSwapChainsToPresent().size());
            for (RHI::DeviceSwapChain* swapChain : scope->GetSwapChainsToPresent())
            {
                m_workRequest.m_swapChainsToPresent.push_back(static_cast<SwapChain*>(swapChain));
            }

            auto& fencesToSignal = m_workRequest.m_scopeFencesToSignal;
            fencesToSignal.reserve(fencesToSignal.size() + scope->GetFencesToSignal().size());
            for (const RHI::Ptr<RHI::Fence>& fence : scope->GetFencesToSignal())
            {
                fencesToSignal.push_back(&static_cast<FenceImpl&>(*fence->GetDeviceFence(scope->GetDeviceIndex())).Get());
            }
        }

        InitMergedRequest request;
        request.m_deviceIndex = device.GetDeviceIndex();
        request.m_scopeEntries = scopeEntries.data();
        request.m_scopeCount = static_cast<uint32_t>(scopeEntries.size());
        Base::Init(request);
    }

    void FrameGraphExecuteGroupPrimary::BeginInternal()
    {
        Base::BeginInternal();
        id <MTLCommandBuffer> mtlCommandBuffer = m_commandBuffer->GetMtlCommandBuffer();
        m_workRequest.m_commandBuffer = m_commandBuffer;
        
        //Encode any wait events at the start of the group. This should grab the wait fence across all queues from the top
        //scope and encode it here.
        EncodeWaitEvents();

        CommandList* commandList = AcquireCommandList();
        commandList->Open(mtlCommandBuffer);
        m_workRequest.m_commandLists.back() = commandList;
    }

    void FrameGraphExecuteGroupPrimary::EndInternal()
    {
        AZ_Assert(!m_workRequest.m_commandLists.empty(), "commandlist is empty.");
        static_cast<CommandList*>(m_workRequest.m_commandLists.back())->Close();
        Base::EndInternal();
    }

    void FrameGraphExecuteGroupPrimary::BeginContextInternal(
        RHI::FrameGraphExecuteContext& context,
        uint32_t contextIndex)
    {
        Base::BeginContextInternal(context, contextIndex);
        AZ_Assert((m_lastCompletedScope + 1) == contextIndex, "Contexts must be recorded in order!");

        Scope* scope = m_scopes[contextIndex];
        scope->SetRenderPassInfo(m_renderPassContexts[contextIndex]);

        AZ_Assert(!m_workRequest.m_commandLists.empty(), "commandlist is empty.");
        CommandList* commandList = static_cast<CommandList*>(m_workRequest.m_commandLists.back());
        context.SetCommandList(*commandList);
        scope->WaitOnAllResourceFences(*commandList);
        scope->Begin(*commandList, context.GetCommandListIndex(), context.GetCommandListCount());
    }

    void FrameGraphExecuteGroupPrimary::EndContextInternal(
        RHI::FrameGraphExecuteContext& context,
        uint32_t contextIndex)
    {
        m_lastCompletedScope = contextIndex;
        
        const Scope* scope = m_scopes[contextIndex];
        CommandList* commandList = static_cast<CommandList*>(context.GetCommandList());
        scope->End(*commandList);
        scope->SignalAllResourceFences(*commandList);
        Base::EndContextInternal(context, contextIndex);
    }

    AZStd::span<const Scope* const> FrameGraphExecuteGroupPrimary::GetScopes() const
    {
        return m_scopes;
    }

    AZStd::span<Scope* const> FrameGraphExecuteGroupPrimary::GetScopes()
    {
        return m_scopes;
    }

    void FrameGraphExecuteGroupPrimary::SetRenderPasscontexts(AZStd::span<const RenderPassContext> renderPassContexts)
    {
        m_renderPassContexts = renderPassContexts;
    }
}
