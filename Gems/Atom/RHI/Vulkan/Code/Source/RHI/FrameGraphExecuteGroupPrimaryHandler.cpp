/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/ImageScopeAttachment.h>
#include <RHI/CommandList.h>
#include <RHI/Device.h>
#include <RHI/FrameGraphExecuteGroupPrimaryHandler.h>
#include <RHI/FrameGraphExecuteGroupPrimary.h>
#include <RHI/RenderPass.h>
#include <RHI/RenderPassBuilder.h>
#include <RHI/Scope.h>

namespace AZ::Vulkan
{
    RHI::ResultCode FrameGraphExecuteGroupPrimaryHandler::InitInternal(
        Device& device, const AZStd::vector<RHI::FrameGraphExecuteGroup*>& executeGroups)
    {
        AZ_Assert(executeGroups.size() == 1, "Too many execute groups when initializing context");
        FrameGraphExecuteGroupPrimary* group = static_cast<FrameGraphExecuteGroupPrimary*>(executeGroups.back());
        AZ_Assert(group, "Invalid execute group for FrameGraphExecuteGroupPrimaryHandler");
        // Creates the renderpasses and framebuffers that will be used.
        auto groupScopes = group->GetScopes();
        m_renderPassContexts.resize(groupScopes.size());
        for (uint32_t i = 0; i < groupScopes.size(); ++i)
        {
            Scope* scope = groupScopes[i];
            if (!scope->UsesRenderpass())
            {
                continue;
            }

            RenderPassBuilder builder(device);
            builder.AddScopeAttachments(*scope);
            // This will update the m_renderPassContexts with the proper renderpass and framebuffer.
            RHI::ResultCode result = builder.End(m_renderPassContexts[i]);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);
            m_renderPassContexts[i].SetName(scope->GetName());
        }

        // Set the renderpass contexts.
        group->SetRenderPasscontexts(m_renderPassContexts);

        return RHI::ResultCode::Success;
    }

    void FrameGraphExecuteGroupPrimaryHandler::EndInternal()
    {
        AZ_Assert(m_executeGroups.size() == 1, "Too many execute groups when initializing context");
        FrameGraphExecuteGroup* group = static_cast<FrameGraphExecuteGroup*>(m_executeGroups.back());
        AddWorkRequest(group->GetWorkRequest());
        //Merged handler will only have one commandlist.
        m_workRequest.m_commandList = group->GetCommandLists()[0];
    }
}
