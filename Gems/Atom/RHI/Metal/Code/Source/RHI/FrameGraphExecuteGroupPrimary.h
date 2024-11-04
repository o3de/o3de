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

namespace AZ::Metal
{
    class CommandList;

    //! Execute group that uses one command buffer to record the work of multiple scopes.
    //! The renderpasses (if needed) are created by the FrameGraphExecuteGroupPrimaryHandler but they
    //! are managed (start and end) by the class itself.
    class FrameGraphExecuteGroupPrimary final
        : public FrameGraphExecuteGroup
    {
        using Base = FrameGraphExecuteGroup;
    public:
        AZ_CLASS_ALLOCATOR(FrameGraphExecuteGroupPrimary, AZ::PoolAllocator);
        AZ_RTTI(FrameGraphExecuteGroupPrimary, "{1D5D2D6F-F4E6-4EF0-8E55-3E4E201B6358}", Base);

        FrameGraphExecuteGroupPrimary() = default;

        void Init(Device& device, AZStd::vector<Scope*>&& scopes);
        //! Set the list of renderpasses that the group will use.
        void SetRenderPasscontexts(AZStd::span<const RenderPassContext> renderPassContexts);

        //////////////////////////////////////////////////////////////////////////
        // FrameGraphExecuteGroup
        AZStd::span<const Scope* const> GetScopes() const override;
        AZStd::span<Scope* const> GetScopes() override;
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
        // List of renderpasses used by the scopes in the group.
        AZStd::span<const RenderPassContext> m_renderPassContexts;
    };
}
