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
