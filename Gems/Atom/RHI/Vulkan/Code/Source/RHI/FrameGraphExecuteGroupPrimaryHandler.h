/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/FrameGraphExecuteGroupHandler.h>
#include <RHI/RenderPassBuilder.h>

namespace AZ::Vulkan
{
    //! Handler for one ExecuteGroupPrimary (contains one or more scopes).
    //! The handler is in charge of creating renderpasses and framebuffers that
    //! each scope in the execute group will use. These are not shared among scopes.
    //! It also contains the primary command list that each scope uses for recording its work.
    class FrameGraphExecuteGroupPrimaryHandler final
        : public FrameGraphExecuteGroupHandler
    {
        using Base = FrameGraphExecuteGroupHandler;

    public:
        AZ_CLASS_ALLOCATOR(FrameGraphExecuteGroupPrimaryHandler, AZ::SystemAllocator);

        FrameGraphExecuteGroupPrimaryHandler() = default;
        ~FrameGraphExecuteGroupPrimaryHandler() = default;

    private:
        //////////////////////////////////////////////////////////////////////////
        // FrameGraphExecuteGroupHandler
        RHI::ResultCode InitInternal(Device& device, const AZStd::vector<RHI::FrameGraphExecuteGroup*>& executeGroups) override;
        void EndInternal() override;
        //////////////////////////////////////////////////////////////////////////

        // List of renderpasses and framebuffers used by the execute group.
        AZStd::vector<RenderPassContext> m_renderPassContexts;
        // Primary command list that the execute group merged will use.
        RHI::Ptr<CommandList> m_primaryCommandList;
    };
}
