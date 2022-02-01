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
#include <RHI/CommandPool.h>
#include <RHI/Device.h>
#include <RHI/FrameGraphExecuteGroup.h>
#include <RHI/Scope.h>
#include <RHI/SwapChain.h>

namespace AZ
{
    namespace Vulkan
    {
        void FrameGraphExecuteGroup::Init(
            Device& device,
            const Scope& scope,
            uint32_t commandListCount,
            RHI::JobPolicy globalJobPolicy)
        {
            Base::InitBase(device, scope.GetFrameGraphGroupId(), scope.GetHardwareQueueClass());
            m_scope = &scope;
            m_secondaryCommands.resize(commandListCount);

            m_workRequest.m_swapChainsToPresent.reserve(scope.GetSwapChainsToPresent().size());
            for (RHI::SwapChain* swapchainBase : scope.GetSwapChainsToPresent())
            {
                m_workRequest.m_swapChainsToPresent.emplace_back(static_cast<SwapChain*>(swapchainBase));
            }

            m_workRequest.m_semaphoresToWait = scope.GetWaitSemaphores();
            m_workRequest.m_semaphoresToSignal = scope.GetSignalSemaphores();
            m_workRequest.m_fencesToSignal = scope.GetSignalFences();
            
            InitRequest request;
            request.m_scopeId = scope.GetId();
            request.m_commandLists = reinterpret_cast<RHI::CommandList*const*>(m_secondaryCommands.data());
            request.m_commandListCount = commandListCount;
            request.m_jobPolicy = globalJobPolicy;
            Base::Init(request);

            m_workRequest.m_debugLabel = AZStd::string::format("Framegraph %s Group", m_scope->GetId().GetCStr());
        }

        void FrameGraphExecuteGroup::BeginInternal()
        {
            // Nothing to do.
        }

        void FrameGraphExecuteGroup::BeginContextInternal(RHI::FrameGraphExecuteContext& context, uint32_t contextIndex)
        {
            AZ_Assert(m_scope, "Scope is null.");
            AZ_Assert(m_scope->GetFrameGraph(), "FrameGraph is null.");            

            // Create secondary command list for this context
            RHI::Ptr<CommandList> commandList = AcquireCommandList(VK_COMMAND_BUFFER_LEVEL_SECONDARY);
            m_secondaryCommands[contextIndex] = commandList;
            context.SetCommandList(*commandList);

            commandList->SetName(Name{ AZStd::string::format("Secondary Buffer %d", static_cast<int>(contextIndex)) });

            CommandList::InheritanceInfo inheritanceInfo{ m_renderPassContext.m_framebuffer.get(), m_subpassIndex };
            commandList->BeginCommandBuffer(&inheritanceInfo);
            m_scope->Begin(*commandList);
        }

        void FrameGraphExecuteGroup::EndContextInternal(RHI::FrameGraphExecuteContext& context, [[maybe_unused]] uint32_t contextIndex)
        {
            CommandList& commandList = static_cast<CommandList&>(*context.GetCommandList());
            m_scope->End(commandList);
            commandList.EndCommandBuffer();
        }

        void FrameGraphExecuteGroup::EndInternal()
        {
        }

        AZStd::span<const Scope* const> FrameGraphExecuteGroup::GetScopes() const
        {
            return AZStd::span<const Scope* const>(&m_scope, 1);
        }

        AZStd::span<const RHI::Ptr<CommandList>> FrameGraphExecuteGroup::GetCommandLists() const
        {
            return m_secondaryCommands;
        }

        void FrameGraphExecuteGroup::SetRenderContext(const RenderPassContext& renderPassContext, uint32_t subpassIndex /*= 0*/)
        {
            m_renderPassContext = renderPassContext;
            m_subpassIndex = subpassIndex;
        }
    }
}
