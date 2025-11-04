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
#include <RHI/RenderPassBuilder.h>

namespace AZ::Metal
{
    RHI::ResultCode FrameGraphExecuteGroupSecondaryHandler::InitInternal(
        Device& device, const AZStd::vector<RHI::FrameGraphExecuteGroup*>& executeGroups)
    {
        // We first need to build the renderpass that will be used by all groups.
        RenderPassBuilder builder;
        builder.Init();
        for (auto executeGroupBase : executeGroups)
        {
            FrameGraphExecuteGroupSecondary* executeGroup = static_cast<FrameGraphExecuteGroupSecondary*>(executeGroupBase);
            AZ_Assert(executeGroup, "Invalid execute group on FrameGraphExecuteGroupHandler");
            AZ_Assert(executeGroup->GetScopes().size() == 1, "Incorrect number of scopes (%d) in group on FrameGraphExecuteGroupHandler", executeGroup->GetScopes().size());
            auto* scope = static_cast<Scope*>(executeGroup->GetScopes()[0]);
            builder.AddScopeAttachments(scope);
        }

        builder.End(m_renderPassContext);

        // Set the RenderPassContext for each group.
        // Also encode all wait events before que create the parallel encoder.
        for (auto executeGroupBase : executeGroups)
        {
            FrameGraphExecuteGroupSecondary* executeGroup = static_cast<FrameGraphExecuteGroupSecondary*>(executeGroupBase);
            executeGroup->SetRenderContext(m_renderPassContext);
            executeGroup->EncondeAllWaitEvents();
        }
        const char* label = executeGroups.size() > 1 ? "SubpassGroupCB" : "GroupCB";
        m_commandBuffer.GetMtlCommandBuffer().label = [NSString stringWithCString:label encoding:NSUTF8StringEncoding];
        return RHI::ResultCode::Success;
    }

    void FrameGraphExecuteGroupSecondaryHandler::EndInternal()
    {
        m_commandBuffer.FlushParallelEncoder();
        for (auto executeGroupBase : m_executeGroups)
        {
            FrameGraphExecuteGroupSecondary* executeGroup = static_cast<FrameGraphExecuteGroupSecondary*>(executeGroupBase);
            AddWorkRequest(executeGroup->AcquireWorkRequest());
            executeGroup->EncodeAllSignalEvents();
        }
        m_workRequest.m_commandBuffer = &m_commandBuffer;
    }

    void FrameGraphExecuteGroupSecondaryHandler::BeginGroupInternal([[maybe_unused]] const FrameGraphExecuteGroup* group)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_m_secondaryEncodersMutex);
        if (!m_secondaryEncodersCreated)
        {
            // We delay as much as possible getting the next drawable from the swapchain
            UpdateSwapChain(m_renderPassContext);

            // Since the parallel encoder needs the renderpass, only after updating the renderpass with the swapchain
            // texture we can create the secondary encoders.
            // Create all the render encoders before in order to establish order. This is because MTLCommandBuffer
            // always match the execution order of the sub render encoders to the order in which they were created.
            for (auto executeGroupBase : m_executeGroups)
            {
                FrameGraphExecuteGroupSecondary* executeGroup = static_cast<FrameGraphExecuteGroupSecondary*>(executeGroupBase);
                // The first group will create the parallel encoder needed for creating the secondary encoders.
                executeGroup->CreateSecondaryEncoders();
            }
            m_secondaryEncodersCreated = true;
        }
    }
}
