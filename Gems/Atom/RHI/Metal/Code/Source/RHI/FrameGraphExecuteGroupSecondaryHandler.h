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
    //! Handler for a list of ExecuteGroupSecondary that are part of the same graph group.
    //! All the execute groups share the same renderpass (if they use one) and each of 
    //! the groups correspond to a subpass of the renderpass (if they use one). Also each 
    //! execute group uses one or more secondary command list to record their work.
    //! The handler owns the primary command list that will execute the secondary command
    //! lists of each execute group.
    class FrameGraphExecuteGroupSecondaryHandler final
        : public FrameGraphExecuteGroupHandler
    {
        using Base = FrameGraphExecuteGroupHandler;

    public:
        AZ_CLASS_ALLOCATOR(FrameGraphExecuteGroupSecondaryHandler, AZ::SystemAllocator);

        FrameGraphExecuteGroupSecondaryHandler() = default;
        ~FrameGraphExecuteGroupSecondaryHandler() = default;

    private:
        //////////////////////////////////////////////////////////////////////////
        // FrameGraphExecuteGroupHandler
        RHI::ResultCode InitInternal(Device& device, const AZStd::vector<RHI::FrameGraphExecuteGroup*>& executeGroups) override;
        void BeginGroupInternal(const FrameGraphExecuteGroupBase* group) override;
        void BeginInternal() override {}
        void EndInternal() override;
        //////////////////////////////////////////////////////////////////////////
        
        // RenderPassContext that is shared by all groups.
        RenderPassContext m_renderPassContext;
        bool m_secondaryEncodersCreated = false;
        mutable AZStd::mutex m_m_secondaryEncodersMutex;
    };
}
