/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/Factory.h>
#include <RHI/Device.h>
#include <RHI/FrameGraphExecuteGroupPrimary.h>
#include <RHI/Scope.h>
#include <RHI/SwapChain.h>

namespace AZ::Vulkan
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

        auto& swapChainsToPresent = m_workRequest.m_swapChainsToPresent;
        AZStd::vector<InitMergedRequest::ScopeEntry> scopeEntries;
        scopeEntries.reserve(m_scopes.size());
        for (const Scope* scope : m_scopes)
        {
            scopeEntries.push_back({ scope->GetId(), scope->GetEstimatedItemCount() });
            swapChainsToPresent.reserve(swapChainsToPresent.size() + scope->GetSwapChainsToPresent().size());
            for (RHI::DeviceSwapChain* swapChain : scope->GetSwapChainsToPresent())
            {
                swapChainsToPresent.push_back(static_cast<SwapChain*>(swapChain));
            }
            const auto& waitSemaphores = scope->GetWaitSemaphores();
            const auto& signalSemaphores = scope->GetSignalSemaphores();
            const auto& signalFences = scope->GetSignalFences();
            const auto& waitFences = scope->GetWaitFences();

            m_workRequest.m_semaphoresToWait.insert(m_workRequest.m_semaphoresToWait.end(), waitSemaphores.begin(), waitSemaphores.end());
            m_workRequest.m_semaphoresToSignal.insert(m_workRequest.m_semaphoresToSignal.end(), signalSemaphores.begin(), signalSemaphores.end());
            m_workRequest.m_fencesToSignal.insert(m_workRequest.m_fencesToSignal.end(), signalFences.begin(), signalFences.end());
            m_workRequest.m_fencesToWaitFor.insert(m_workRequest.m_fencesToWaitFor.end(), waitFences.begin(), waitFences.end());
        }

        InitMergedRequest request;
        request.m_deviceIndex = device.GetDeviceIndex();
        request.m_scopeEntries = scopeEntries.data();
        request.m_scopeCount = static_cast<uint32_t>(scopeEntries.size());
        Base::Init(request);

        m_workRequest.m_debugLabel = "FrameGraph Merged Group";
    }

    void FrameGraphExecuteGroupPrimary::BeginInternal()
    {
        m_commandList = AcquireCommandList(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        m_commandList->BeginCommandBuffer();
        if (r_gpuMarkersMergeGroups)
        {
            m_commandList->BeginDebugLabel(m_commandList->GetName().GetCStr());
        }
            
        m_workRequest.m_commandList = m_commandList;
    }

    void FrameGraphExecuteGroupPrimary::EndInternal()
    {
        if (r_gpuMarkersMergeGroups)
        {
            m_commandList->EndDebugLabel();
        }
        m_commandList->EndCommandBuffer();
    }

    void FrameGraphExecuteGroupPrimary::BeginContextInternal(
        RHI::FrameGraphExecuteContext& context,
        uint32_t contextIndex)
    {
        AZ_Assert(static_cast<uint32_t>(m_lastCompletedScope + 1) == contextIndex, "Contexts must be recorded in order!");

        const Scope* scope = m_scopes[contextIndex];
        m_commandList->SetName(m_name);
        m_commandList->BeginDebugLabel(scope->GetMarkerLabel().data());
        context.SetCommandList(*m_commandList);

        scope->ProcessClearRequests(*m_commandList);
        scope->EmitScopeBarriers(*m_commandList, Scope::BarrierSlot::Prologue);
        scope->ResetQueryPools(*m_commandList);
        scope->Begin(*m_commandList, context);

        // Begin the render pass if the scope uses one.
        const RenderPassContext& renderPassContext = m_renderPassContexts[contextIndex];
        if (scope->UsesRenderpass())
        {
            CommandList::BeginRenderPassInfo beginInfo;
            beginInfo.m_clearValues = renderPassContext.m_clearValues;
            beginInfo.m_frameBuffer = renderPassContext.m_framebuffer.get();
            beginInfo.m_subpassContentType = VK_SUBPASS_CONTENTS_INLINE;
            m_commandList->BeginRenderPass(beginInfo);
        }
    }

    void FrameGraphExecuteGroupPrimary::EndContextInternal(
        RHI::FrameGraphExecuteContext& context,
        uint32_t contextIndex)
    {
        m_lastCompletedScope = contextIndex;

        const Scope* scope = m_scopes[contextIndex];
        CommandList* commandList = static_cast<CommandList*>(context.GetCommandList());
        if (commandList->IsInsideRenderPass())
        {
            commandList->EndRenderPass();
        }
        scope->ResolveMSAAAttachments(*commandList);
        scope->End(*commandList, context);
        scope->EmitScopeBarriers(*m_commandList, Scope::BarrierSlot::Epilogue);

        commandList->EndDebugLabel();
    }

    AZStd::span<const Scope* const> FrameGraphExecuteGroupPrimary::GetScopes() const
    {
        return m_scopes;
    }

    AZStd::span<Scope* const> FrameGraphExecuteGroupPrimary::GetScopes()
    {
        return m_scopes;
    }

    AZStd::span<const RHI::Ptr<CommandList>> FrameGraphExecuteGroupPrimary::GetCommandLists() const
    {
        return AZStd::span<const RHI::Ptr<CommandList>>(&m_commandList, 1);
    }

    void FrameGraphExecuteGroupPrimary::SetPrimaryCommandList(CommandList& commandList)
    {
        m_commandList = &commandList;
    }

    void FrameGraphExecuteGroupPrimary::SetRenderPasscontexts(AZStd::span<const RenderPassContext> renderPassContexts)
    {
        m_renderPassContexts = renderPassContexts;
    }

    void FrameGraphExecuteGroupPrimary::SetName(const Name& name)
    {
        m_name = name;
    }
}
