/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/FrameGraphExecuteGroup.h>
#include <RHI/CommandList.h>
#include <RHI/CommandQueue.h>
#include <RHI/FrameGraphExecuteGroupBase.h>
#include <RHI/RenderPassBuilder.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;
        class FrameBuffer;
        class Scope;
        class SwapChain;
        struct RenderPassContext;

        //! Execute group for one scope that uses one or more secondary command lists to
        //! record it's work. Renderpass and framebuffers are handled by the FrameGraphExecuteGroupHandler.
        class FrameGraphExecuteGroup final
            : public FrameGraphExecuteGroupBase
        {
            using Base = FrameGraphExecuteGroupBase;

        public:
            AZ_CLASS_ALLOCATOR(FrameGraphExecuteGroup, AZ::SystemAllocator, 0);
            AZ_RTTI(FrameGraphExecuteGroup, "{D3BDC3AC-06A7-4067-9E71-0DBB3A80B188}", Base);

            FrameGraphExecuteGroup() = default;
            ~FrameGraphExecuteGroup() = default;

            void Init(Device& device,
                const Scope& scope,
                uint32_t commandListCount,
                RHI::JobPolicy globalJobPolicy);

            //////////////////////////////////////////////////////////////////////////
            // FrameGraphExecuteGroupBase
            AZStd::array_view<const Scope*> GetScopes() const override;
            AZStd::array_view<RHI::Ptr<CommandList>> GetCommandLists() const override;
            //////////////////////////////////////////////////////////////////////////

            //! Set the render context and subpass that will be used by this execute group.
            void SetRenderContext(const RenderPassContext& renderPassContext, uint32_t subpassIndex = 0);

        private:
            //////////////////////////////////////////////////////////////////////////
            // RHI::ExecuteContextGroupBase
            void BeginInternal() override;
            void BeginContextInternal(
                RHI::FrameGraphExecuteContext& context,
                uint32_t contextIndex) override;
            void EndContextInternal(
                RHI::FrameGraphExecuteContext& context,
                uint32_t contextIndex) override;
            void EndInternal() override;
            //////////////////////////////////////////////////////////////////////////

            const Scope* m_scope = nullptr;
            AZStd::vector<RHI::Ptr<CommandList>> m_secondaryCommands;
            // Render context that contains the framebuffer that this group will use.
            RenderPassContext m_renderPassContext;
            // Subpass that the group will use.
            uint32_t m_subpassIndex = 0;
        };
    }
}
