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

namespace AZ::Metal
{
    //! Handler for one ExecuteGroupPrimary (which contains one or more scopes).
    //! The handler is in charge of creating renderpasses that
    //! each scope in the execute group will use. These are not shared among scopes.
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
        void BeginGroupInternal(const FrameGraphExecuteGroup* group) override;
        void BeginInternal() override {}
        void EndInternal() override;
        //////////////////////////////////////////////////////////////////////////

        // List of renderpasses used by the execute group.
        AZStd::vector<RenderPassContext> m_renderPassContexts;
    };
}
