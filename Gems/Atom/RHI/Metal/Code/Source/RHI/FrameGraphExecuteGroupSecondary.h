/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/CommandList.h>
#include <RHI/CommandQueue.h>
#include <RHI/FrameGraphExecuteGroup.h>
#include <RHI/RenderPassBuilder.h>

namespace AZ::Metal
{
    class Device;
    class Scope;
    class SwapChain;
    struct RenderPassContext;

    //! Execute group for one scope that uses multiple encoders to
    //! record it's work. Renderpass (if needed) are handled by the FrameGraphExecuteGroupSecondaryHandler.
    class FrameGraphExecuteGroupSecondary final
        : public FrameGraphExecuteGroup
    {
        using Base = FrameGraphExecuteGroup;

    public:
        AZ_CLASS_ALLOCATOR(FrameGraphExecuteGroupSecondary, AZ::SystemAllocator);
        AZ_RTTI(FrameGraphExecuteGroupSecondary, "{17E690A6-CE8F-4C7B-8C48-E0939152FBC0}", Base);

        FrameGraphExecuteGroupSecondary() = default;
        ~FrameGraphExecuteGroupSecondary() = default;

        void Init(Device& device,
            Scope& scope,
            uint32_t commandListCount,
            RHI::JobPolicy globalJobPolicy);

        //////////////////////////////////////////////////////////////////////////
        // FrameGraphExecuteGroup
        AZStd::span<const Scope* const> GetScopes() const override;
        AZStd::span<Scope* const> GetScopes() override;
        //////////////////////////////////////////////////////////////////////////

        //! Set the render context and subpass that will be used by this execute group.
        //! This render context is the same for all other FrameGraphExecuteGroupSecondary of the handler.
        void SetRenderContext(const RenderPassContext& renderPassContext);

        //! Returns the scope of this group. There's only one scope.
        const Scope* GetScope() const;

        //! Encodes all wait events for the group and the scope
        void EncondeAllWaitEvents() const;
        //! Encodes all signal events for the group and the scope
        void EncodeAllSignalEvents() const;

        //! Creates the sub encoders that will be used for recording the work of the scope.
        void CreateSecondaryEncoders();

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

        Scope* m_scope = nullptr;
        
        // Render context that contains the renderpass that this group will use.
        RenderPassContext m_renderPassContext;

        struct SubEncoderData
        {
            CommandList* m_commandList;
            id <MTLCommandEncoder> m_subRenderEncoder;
        };
        
        // Container to hold commandlist and render encoder to be used per contextId.
        AZStd::vector<SubEncoderData > m_subRenderEncoders;
    };
}
