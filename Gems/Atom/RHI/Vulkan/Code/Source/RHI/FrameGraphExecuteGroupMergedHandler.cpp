/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "Atom_RHI_Vulkan_precompiled.h"
#include <Atom/RHI/ImageScopeAttachment.h>
#include <RHI/CommandList.h>
#include <RHI/Device.h>
#include <RHI/FrameGraphExecuteGroupMergedHandler.h>
#include <RHI/FrameGraphExecuteGroupMerged.h>
#include <RHI/RenderPass.h>
#include <RHI/RenderPassBuilder.h>
#include <RHI/Scope.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::ResultCode FrameGraphExecuteGroupMergedHandler::InitInternal(Device& device, const AZStd::vector<RHI::FrameGraphExecuteGroup*>& executeGroups)
        {
            AZ_Assert(executeGroups.size() == 1, "Too many execute groups when initializing context");
            FrameGraphExecuteGroupMerged* group = static_cast<FrameGraphExecuteGroupMerged*>(executeGroups.back());
            AZ_Assert(group, "Invalid execute group for FrameGraphExecuteGroupMergedHandler");
            // Creates the renderpasses and framebufers that will be used.
            auto groupScopes = group->GetScopes();
            m_renderPassContexts.resize(groupScopes.size());
            for (uint32_t i = 0; i < groupScopes.size(); ++i)
            {
                const Scope* scope = groupScopes[i];
                if (!scope->UsesRenderpass())
                {
                    continue;
                }

                RenderPassBuilder builder(device, 1);
                builder.AddScopeAttachments(*scope);
                // This will update the m_renderPassContexts with the proper renderpass and framebuffer.
                RHI::ResultCode result = builder.End(m_renderPassContexts[i]);
                RETURN_RESULT_IF_UNSUCCESSFUL(result);
            }

            // Set the command list and renderpass contexts.
            m_primaryCommandList = device.AcquireCommandList(m_hardwareQueueClass);
            group->SetPrimaryCommandList(*m_primaryCommandList);
            group->SetRenderPasscontexts(m_renderPassContexts);

            return RHI::ResultCode::Success;
        }

        void FrameGraphExecuteGroupMergedHandler::EndInternal()
        {
            AZ_Assert(m_executeGroups.size() == 1, "Too many execute groups when initializing context");
            FrameGraphExecuteGroupBase* group = static_cast<FrameGraphExecuteGroupBase*>(m_executeGroups.back());
            AddWorkRequest(group->GetWorkRequest());
            m_workRequest.m_commandList = m_primaryCommandList;
        }
    }
}
