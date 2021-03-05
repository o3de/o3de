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

#include <RHI/FrameGraphExecuteGroupBase.h>
#include <RHI/RenderPassBuilder.h>

namespace AZ
{
    namespace Vulkan
    {
        class CommandList;

        //! Execute group that uses one primary command list to record the work of multiple scopes.
        //! The renderpasses and framebuffers are created by the FrameGraphExecuteGroupMergedHandler but they
        //! are managed (start and end) by the class itself.
        class FrameGraphExecuteGroupMerged final
            : public FrameGraphExecuteGroupBase
        {
            using Base = FrameGraphExecuteGroupBase;
        public:
            AZ_CLASS_ALLOCATOR(FrameGraphExecuteGroupMerged, AZ::PoolAllocator, 0);
            AZ_RTTI(FrameGraphExecuteGroupMerged, "{85DE8F45-3CA1-4FD9-9B0E-EE98518D2717}", Base);

            FrameGraphExecuteGroupMerged() = default;

            void Init(Device& device, AZStd::vector<const Scope*>&& scopes);
            //! Set the command list that the group will use.
            void SetPrimaryCommandList(CommandList& commandList);
            //! Set the list of renderpasses that the group will use.
            void SetRenderPasscontexts(AZStd::array_view<RenderPassContext> renderPassContexts);

            //////////////////////////////////////////////////////////////////////////
            // FrameGraphExecuteGroupBase
            AZStd::array_view<const Scope*> GetScopes() const override;
            AZStd::array_view<RHI::Ptr<CommandList>> GetCommandLists() const override;
            //////////////////////////////////////////////////////////////////////////

        private:
            //////////////////////////////////////////////////////////////////////////
            // RHI::FrameGraphExecuteGroup
            void BeginInternal() override;
            void BeginContextInternal(
                RHI::FrameGraphExecuteContext& context,
                uint32_t contextIndex) override;
            void EndContextInternal(
                RHI::FrameGraphExecuteContext& context,
                uint32_t contextIndex) override;
            void EndInternal() override;
            //////////////////////////////////////////////////////////////////////////

            int32_t m_lastCompletedScope = -1;
            // List of scopes in the group.
            AZStd::vector<const Scope*> m_scopes;
            // Primary command list used to record the work.
            RHI::Ptr<CommandList> m_commandList;
            // List of renderpasses and framebuffers used by the scopes in the group.
            AZStd::array_view<RenderPassContext> m_renderPassContexts;
        };
    }
}
