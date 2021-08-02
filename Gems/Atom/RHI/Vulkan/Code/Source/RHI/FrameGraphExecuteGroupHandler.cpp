/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/CommandList.h>
#include <RHI/Device.h>
#include <RHI/FrameGraphExecuteGroupHandler.h>
#include <RHI/FrameGraphExecuteGroup.h>
#include <RHI/RenderPass.h>
#include <RHI/RenderPassBuilder.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::ResultCode FrameGraphExecuteGroupHandler::InitInternal(Device& device, const AZStd::vector<RHI::FrameGraphExecuteGroup*>& executeGroups)
        {
            // We first need to build the renderpass that will be used by all groups.
            RenderPassBuilder builder(device, static_cast<uint32_t>(executeGroups.size()));
            for (auto executeGroupBase : executeGroups)
            {
                const FrameGraphExecuteGroup* executeGroup = static_cast<const FrameGraphExecuteGroup*>(executeGroupBase);
                AZ_Assert(executeGroup, "Invalid execute group on FrameGraphExecuteGroupHandler");
                AZ_Assert(executeGroup->GetScopes().size() == 1, "Incorrect number of scopes (%d) in group on FrameGraphExecuteGroupHandler", executeGroup->GetScopes().size());
                builder.AddScopeAttachments(*executeGroup->GetScopes()[0]);
            }

            // This will update the m_renderPassContext with the proper renderpass and framebuffer.
            RHI::ResultCode result = builder.End(m_renderPassContext);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            // Set the RenderPassContext for each group.
            for (uint32_t i = 0; i < executeGroups.size(); ++i)
            {
                auto executeGroup = azrtti_cast<FrameGraphExecuteGroup*>(executeGroups[i]);
                executeGroup->SetRenderContext(m_renderPassContext, i);
            }

            return RHI::ResultCode::Success;
        }

        void FrameGraphExecuteGroupHandler::EmitScopeBarriers(CommandList& commandList, Scope::BarrierSlot slot) const
        {
            for (auto group : m_executeGroups)
            {
                const FrameGraphExecuteGroup* executeGroup = static_cast<const FrameGraphExecuteGroup*>(group);
                // There's only one scope in this type of handler.
                executeGroup->GetScopes()[0]->EmitScopeBarriers(commandList, slot);
            }
        }


        void FrameGraphExecuteGroupHandler::ProcessClearRequests(CommandList& commandList) const
        {
            for (auto group : m_executeGroups)
            {
                const FrameGraphExecuteGroup* executeGroup = static_cast<const FrameGraphExecuteGroup*>(group);
                // There's only one scope in this type of handler.
                executeGroup->GetScopes()[0]->ProcessClearRequests(commandList);
            }
        }

        void FrameGraphExecuteGroupHandler::ResetQueryPools(CommandList& commandList) const
        {
            for (auto group : m_executeGroups)
            {
                const FrameGraphExecuteGroup* executeGroup = static_cast<const FrameGraphExecuteGroup*>(group);
                // Reset the RHI Queries of the scope before starting the renderpass.
                // There's only one scope in this type of handler.
                executeGroup->GetScopes()[0]->ResetQueryPools(commandList);
            }
        }

        void FrameGraphExecuteGroupHandler::EndInternal()
        {
            RHI::Ptr<CommandList> commandList = m_device->AcquireCommandList(m_hardwareQueueClass);
            commandList->BeginCommandBuffer();
            // First emit all scope barriers outside of the renderpass.
            EmitScopeBarriers(*commandList, Scope::BarrierSlot::Aliasing);
            ProcessClearRequests(*commandList);
            EmitScopeBarriers(*commandList, Scope::BarrierSlot::Prologue);
            CommandList::BeginRenderPassInfo beginInfo;
            beginInfo.m_frameBuffer = m_renderPassContext.m_framebuffer.get();
            beginInfo.m_clearValues = AZStd::move(m_renderPassContext.m_clearValues);
            beginInfo.m_subpassContentType = VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;
            // Reset the RHI QueryPools.
            ResetQueryPools(*commandList);
            // Start the renderpass.
            commandList->BeginRenderPass(beginInfo);
            for (uint32_t i = 0; i < m_executeGroups.size(); ++i)
            {
                FrameGraphExecuteGroupBase* group = static_cast<FrameGraphExecuteGroupBase*>(m_executeGroups[i]);
                auto commandLists = group->GetCommandLists();
                commandList->ExecuteSecondaryCommandLists(commandLists);
                // Move to the next subpass. If this is the last one, the NextSubpass function will do nothing.
                commandList->NextSubpass(VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

                AddWorkRequest(group->GetWorkRequest());
            }
            // End the renderpass
            commandList->EndRenderPass();
            // Emit epilogue barriers outside the renderpass.
            EmitScopeBarriers(*commandList, Scope::BarrierSlot::Epilogue);
            commandList->EndCommandBuffer();

            m_workRequest.m_commandList = commandList;
        }
    }
}
