/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/FrameGraphExecuteGroupHandlerBase.h>
#include <RHI/RenderPassBuilder.h>

namespace AZ
{
    namespace Vulkan
    {
        //! Handler for one ExecuteGroupMerged. The handler is in charge of creating the
        //! renderpasses and framebuffers that the execute group will use. The primary command list
        //! will be pass to the execute group for recording all the work.
        class FrameGraphExecuteGroupMergedHandler final
            : public FrameGraphExecuteGroupHandlerBase
        {
            using Base = FrameGraphExecuteGroupHandlerBase;

        public:
            AZ_CLASS_ALLOCATOR(FrameGraphExecuteGroupMergedHandler, AZ::SystemAllocator, 0);

            FrameGraphExecuteGroupMergedHandler() = default;
            ~FrameGraphExecuteGroupMergedHandler() = default;

        private:
            //////////////////////////////////////////////////////////////////////////
            // FrameGraphExecuteGroupHandlerBase
            RHI::ResultCode InitInternal(Device& device, const AZStd::vector<RHI::FrameGraphExecuteGroup*>& executeGroups) override;
            void EndInternal() override;
            //////////////////////////////////////////////////////////////////////////

            // List of renderpasses and framebuffers used by the execute group.
            AZStd::vector<RenderPassContext> m_renderPassContexts;
            // Primary command list that the execute group merged will use.
            RHI::Ptr<CommandList> m_primaryCommandList;
        };
    }
}
