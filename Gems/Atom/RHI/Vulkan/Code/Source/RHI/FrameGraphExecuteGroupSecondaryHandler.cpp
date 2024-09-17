/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/CommandList.h>
#include <RHI/Device.h>
#include <RHI/FrameGraphExecuteGroupSecondaryHandler.h>
#include <RHI/FrameGraphExecuteGroupSecondary.h>
#include <RHI/RenderPass.h>
#include <RHI/RenderPassBuilder.h>

namespace AZ::Vulkan
{
    RHI::ResultCode FrameGraphExecuteGroupSecondaryHandler::InitInternal(
        Device& device, const AZStd::vector<RHI::FrameGraphExecuteGroup*>& executeGroups)
    {
        // We first need to build the renderpass that will be used by all groups.
        RenderPassBuilder builder(device);
        AZStd::string name = executeGroups.size() > 1 ? "[Merged]" : "";
        for (auto executeGroupBase : executeGroups)
        {
            FrameGraphExecuteGroupSecondary* executeGroup = static_cast<FrameGraphExecuteGroupSecondary*>(executeGroupBase);
            AZ_Assert(executeGroup, "Invalid execute group on FrameGraphExecuteGroupHandler");
            AZ_Assert(executeGroup->GetScopes().size() == 1, "Incorrect number of scopes (%d) in group on FrameGraphExecuteGroupHandler", executeGroup->GetScopes().size());
            auto* scope = executeGroup->GetScopes()[0];
            builder.AddScopeAttachments(*scope);
            name = AZStd::string::format("%s;%s", name.c_str(), scope->GetName().GetCStr());
        }

        // Check if we actually need to build a renderpass
        if (builder.CanBuild())
        {
            // This will update the m_renderPassContext with the proper renderpass and framebuffer.
            RHI::ResultCode result = builder.End(m_renderPassContext);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);
        }
        m_renderPassContext.SetName(AZ::Name(name));

        // Set the RenderPassContext for each group.
        uint32_t subpassIndex = 0;
        for (uint32_t i = 0; i < executeGroups.size(); ++i)
        {
            auto executeGroup = azrtti_cast<FrameGraphExecuteGroupSecondary*>(executeGroups[i]);
            if (executeGroup->GetScope()->UsesRenderpass())
            {
                executeGroup->SetRenderContext(m_renderPassContext, subpassIndex++);
            }
        }

        return RHI::ResultCode::Success;
    }

    void FrameGraphExecuteGroupSecondaryHandler::EmitScopeBarriers(CommandList& commandList, Scope::BarrierSlot slot) const
    {
        // Group all memory barriers of the scopes into one memory barrier.
        VkMemoryBarrier2 collectedMemoryBarrier = {};
        // Emit all barriers except the memory ones.
        BarrierTypeFlags emitMask = RHI::ResetBits(BarrierTypeFlags::All, BarrierTypeFlags::Memory);
        for (auto group : m_executeGroups)
        {
            const FrameGraphExecuteGroupSecondary* executeGroup = static_cast<const FrameGraphExecuteGroupSecondary*>(group);
            // There's only one scope in this type of handler.
            auto scope = executeGroup->GetScopes()[0];
            scope->EmitScopeBarriers(commandList, slot, emitMask);
            auto scopeMemoryBarrier = scope->CollectMemoryBarriers(slot);
            collectedMemoryBarrier.srcAccessMask |= scopeMemoryBarrier.srcAccessMask;
            collectedMemoryBarrier.srcStageMask |= scopeMemoryBarrier.srcStageMask;
            collectedMemoryBarrier.dstStageMask |= scopeMemoryBarrier.dstStageMask;
            collectedMemoryBarrier.dstAccessMask |= scopeMemoryBarrier.dstAccessMask;
        }

        if (collectedMemoryBarrier.srcAccessMask != 0 || collectedMemoryBarrier.dstAccessMask != 0)
        {
            // Now emit the grouped memory barrier.
            VkMemoryBarrier memoryBarrier{};
            memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            memoryBarrier.srcAccessMask = aznumeric_caster(collectedMemoryBarrier.srcAccessMask);
            memoryBarrier.dstAccessMask = aznumeric_caster(collectedMemoryBarrier.dstAccessMask);
            static_cast<Device&>(commandList.GetDevice())
                .GetContext()
                .CmdPipelineBarrier(
                    commandList.GetNativeCommandBuffer(),
                    aznumeric_caster(collectedMemoryBarrier.srcStageMask),
                    aznumeric_caster(collectedMemoryBarrier.dstStageMask),
                    VK_DEPENDENCY_BY_REGION_BIT,
                    1,
                    &memoryBarrier,
                    0,
                    nullptr,
                    0,
                    nullptr);
        }
    }


    void FrameGraphExecuteGroupSecondaryHandler::ProcessClearRequests(CommandList& commandList) const
    {
        for (auto group : m_executeGroups)
        {
            const FrameGraphExecuteGroupSecondary* executeGroup = static_cast<const FrameGraphExecuteGroupSecondary*>(group);
            // There's only one scope in this type of handler.
            executeGroup->GetScopes()[0]->ProcessClearRequests(commandList);
        }
    }

    void FrameGraphExecuteGroupSecondaryHandler::ResetQueryPools(CommandList& commandList) const
    {
        for (auto group : m_executeGroups)
        {
            const FrameGraphExecuteGroupSecondary* executeGroup = static_cast<const FrameGraphExecuteGroupSecondary*>(group);
            // Reset the RHI Queries of the scope before starting the renderpass.
            // There's only one scope in this type of handler.
            executeGroup->GetScopes()[0]->ResetQueryPools(commandList);
        }
    }

    void FrameGraphExecuteGroupSecondaryHandler::EndInternal()
    {
        RHI::Ptr<CommandList> commandList = m_device->AcquireCommandList(m_hardwareQueueClass);

        bool usePrimaryDebugLabel = m_executeGroups.size() > 1;
        commandList->BeginCommandBuffer();
        if (usePrimaryDebugLabel)
        {
            commandList->BeginDebugLabel("Subpass Group");
        }

        // First emit all scope barriers outside of the renderpass.
        ProcessClearRequests(*commandList);
        EmitScopeBarriers(*commandList, Scope::BarrierSlot::Prologue);
        // Reset the RHI QueryPools.
        ResetQueryPools(*commandList);

        bool usesRenderPass = m_renderPassContext.IsValid();
        if (usesRenderPass)
        {
            // Start the renderpass.
            CommandList::BeginRenderPassInfo beginInfo;
            beginInfo.m_frameBuffer = m_renderPassContext.m_framebuffer.get();
            beginInfo.m_clearValues = AZStd::move(m_renderPassContext.m_clearValues);
            beginInfo.m_subpassContentType = VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;
            commandList->BeginRenderPass(beginInfo);
        }

        for (uint32_t i = 0; i < m_executeGroups.size(); ++i)
        {
            FrameGraphExecuteGroup* group = static_cast<FrameGraphExecuteGroup*>(m_executeGroups[i]);
            auto commandLists = group->GetCommandLists();
            commandList->BeginDebugLabel(group->GetScopes().front()->GetMarkerLabel().data());
            commandList->ExecuteSecondaryCommandLists(commandLists);
            if (usesRenderPass)
            {
                // Move to the next subpass. If this is the last one, the NextSubpass function will do nothing.
                commandList->NextSubpass(VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
            }
            commandList->EndDebugLabel();
            AddWorkRequest(group->GetWorkRequest());
        }

        if (usesRenderPass)
        {
            // End the renderpass
            commandList->EndRenderPass();
        }
        // Emit epilogue barriers outside the renderpass.
        EmitScopeBarriers(*commandList, Scope::BarrierSlot::Epilogue);

        if (usePrimaryDebugLabel)
        {
            commandList->EndDebugLabel();
        }
        commandList->EndCommandBuffer();

        m_workRequest.m_commandList = commandList;
    }
}
