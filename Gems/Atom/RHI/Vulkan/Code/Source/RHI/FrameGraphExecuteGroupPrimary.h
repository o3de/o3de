/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/FrameGraphExecuteGroup.h>
#include <RHI/RenderPassBuilder.h>

namespace AZ::Vulkan
{
    class CommandList;

    //! Execute group that uses one primary command list to record the work of multiple scopes.
    //! The renderpasses and framebuffers (if needed) are created by the FrameGraphExecuteGroupPrimaryHandler but they
    //! are managed (start and end) by the class itself.
    class FrameGraphExecuteGroupPrimary final
        : public FrameGraphExecuteGroup
    {
        using Base = FrameGraphExecuteGroup;
    public:
        AZ_CLASS_ALLOCATOR(FrameGraphExecuteGroupPrimary, AZ::PoolAllocator);
        AZ_RTTI(FrameGraphExecuteGroupPrimary, "{85DE8F45-3CA1-4FD9-9B0E-EE98518D2717}", Base);

        FrameGraphExecuteGroupPrimary() = default;

        void Init(Device& device, AZStd::vector<Scope*>&& scopes);
        //! Set the command list that the group will use.
        void SetPrimaryCommandList(CommandList& commandList);
        //! Set the list of renderpasses that the group will use.
        void SetRenderPasscontexts(AZStd::span<const RenderPassContext> renderPassContexts);
        //! Set the name of the commandlist the group will use
        void SetName(const Name& name);

        //////////////////////////////////////////////////////////////////////////
        // FrameGraphExecuteGroup
        AZStd::span<const Scope* const> GetScopes() const override;
        AZStd::span<Scope* const> GetScopes() override;
        AZStd::span<const RHI::Ptr<CommandList>> GetCommandLists() const override;
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
        AZStd::vector<Scope*> m_scopes;
        // Primary command list used to record the work.
        RHI::Ptr<CommandList> m_commandList;
        // List of renderpasses and framebuffers used by the scopes in the group.
        AZStd::span<const RenderPassContext> m_renderPassContexts;
        // Name used by the command list encoding the work for this group
        AZ::Name m_name{ "Merged" };
    };
}
