/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/CommandList.h>
#include <RHI/Device.h>
#include <RHI/FrameGraphExecuteGroupPrimaryHandler.h>
#include <RHI/FrameGraphExecuteGroupPrimary.h>
#include <RHI/RenderPassBuilder.h>
#include <RHI/Scope.h>

namespace AZ::Metal
{
    RHI::ResultCode FrameGraphExecuteGroupPrimaryHandler::InitInternal(
        Device& device, const AZStd::vector<RHI::FrameGraphExecuteGroup*>& executeGroups)
    {
        AZ_Assert(executeGroups.size() == 1, "Too many execute groups when initializing context");
        FrameGraphExecuteGroupPrimary* group = static_cast<FrameGraphExecuteGroupPrimary*>(executeGroups.back());
        AZ_Assert(group, "Invalid execute group for FrameGraphExecuteGroupPrimaryHandler");
        // Creates the renderpass descriptors that will be used.
        auto groupScopes = group->GetScopes();
        m_renderPassContexts.resize(groupScopes.size());
        for (uint32_t i = 0; i < groupScopes.size(); ++i)
        {
            Scope* scope = groupScopes[i];
            RenderPassBuilder builder;
            builder.Init();
            builder.AddScopeAttachments(scope);
            builder.End(m_renderPassContexts[i]);
        }

        // Set the renderpass contexts.
        group->SetRenderPasscontexts(m_renderPassContexts);
        m_commandBuffer.GetMtlCommandBuffer().label = [NSString stringWithCString:"MergedGroupCB" encoding:NSUTF8StringEncoding];
        return RHI::ResultCode::Success;
    }

    void FrameGraphExecuteGroupPrimaryHandler::EndInternal()
    {
        AZ_Assert(m_executeGroups.size() == 1, "Too many execute groups when initializing context");
        FrameGraphExecuteGroup* group = static_cast<FrameGraphExecuteGroup*>(m_executeGroups.back());
        AddWorkRequest(group->AcquireWorkRequest());
    }

    void FrameGraphExecuteGroupPrimaryHandler::BeginGroupInternal([[maybe_unused]] const FrameGraphExecuteGroup* group)
    {
        // There's only one group so this should be call once.
        for (auto& context : m_renderPassContexts)
        {
            UpdateSwapChain(context);
        }
    }
}
