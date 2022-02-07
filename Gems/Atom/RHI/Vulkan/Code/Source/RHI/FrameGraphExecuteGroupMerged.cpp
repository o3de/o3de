/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/FrameGraphExecuteGroupMerged.h>
#include <RHI/Scope.h>
#include <RHI/SwapChain.h>

namespace AZ
{
    namespace Vulkan
    {
        void FrameGraphExecuteGroupMerged::Init(
            Device& device,
            AZStd::vector<const Scope*>&& scopes)
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
            AZStd::vector<RHI::ScopeId> scopeIds;
            scopeIds.reserve(m_scopes.size());
            for (const Scope* scope : m_scopes)
            {
                scopeIds.push_back(scope->GetId());
                swapChainsToPresent.reserve(swapChainsToPresent.size() + scope->GetSwapChainsToPresent().size());
                for (RHI::SwapChain* swapChain : scope->GetSwapChainsToPresent())
                {
                    swapChainsToPresent.push_back(static_cast<SwapChain*>(swapChain));
                }
                const auto& waitSemaphores = scope->GetWaitSemaphores();
                const auto& signalSemaphores = scope->GetSignalSemaphores();
                const auto& signalFences = scope->GetSignalFences();

                m_workRequest.m_semaphoresToWait.insert(m_workRequest.m_semaphoresToWait.end(), waitSemaphores.begin(), waitSemaphores.end());
                m_workRequest.m_semaphoresToSignal.insert(m_workRequest.m_semaphoresToSignal.end(), signalSemaphores.begin(), signalSemaphores.end());
                m_workRequest.m_fencesToSignal.insert(m_workRequest.m_fencesToSignal.end(), signalFences.begin(), signalFences.end());
            }

            InitMergedRequest request;
            request.m_scopeIds = scopeIds.data();
            request.m_scopeCount = static_cast<uint32_t>(scopeIds.size());
            Base::Init(request);

            m_workRequest.m_debugLabel = "FrameGraph Merged Group";
        }

        void FrameGraphExecuteGroupMerged::BeginInternal()
        {
            m_commandList = AcquireCommandList(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
            m_commandList->BeginCommandBuffer();
            m_workRequest.m_commandList = m_commandList;
        }

        void FrameGraphExecuteGroupMerged::EndInternal()
        {
            m_commandList->EndCommandBuffer();
        }

        void FrameGraphExecuteGroupMerged::BeginContextInternal(
            RHI::FrameGraphExecuteContext& context,
            uint32_t contextIndex)
        {
            AZ_Assert(static_cast<uint32_t>(m_lastCompletedScope + 1) == contextIndex, "Contexts must be recorded in order!");

            const Scope* scope = m_scopes[contextIndex];
            context.SetCommandList(*m_commandList);

            scope->EmitScopeBarriers(*m_commandList, Scope::BarrierSlot::Aliasing);
            scope->ProcessClearRequests(*m_commandList);
            scope->EmitScopeBarriers(*m_commandList, Scope::BarrierSlot::Prologue);
            scope->ResetQueryPools(*m_commandList);
            scope->Begin(*m_commandList);

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

        void FrameGraphExecuteGroupMerged::EndContextInternal(
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
            scope->End(*commandList);
            scope->EmitScopeBarriers(*m_commandList, Scope::BarrierSlot::Epilogue);
        }

        AZStd::array_view<const Scope*> FrameGraphExecuteGroupMerged::GetScopes() const
        {
            return m_scopes;
        }

        AZStd::array_view<RHI::Ptr<CommandList>> FrameGraphExecuteGroupMerged::GetCommandLists() const
        {
            return AZStd::array_view<RHI::Ptr<CommandList>>(&m_commandList, 1);
        }

        void FrameGraphExecuteGroupMerged::SetPrimaryCommandList(CommandList& commandList)
        {
            m_commandList = &commandList;
        }

        void FrameGraphExecuteGroupMerged::SetRenderPasscontexts(AZStd::array_view<RenderPassContext> renderPassContexts)
        {
            m_renderPassContexts = renderPassContexts;
        }
    }
}
