/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/Scope.h>

namespace AZ::WebGPU
{
    class CommandList;

    //! Helper class for building the wgpu renderpass descriptor
    class RenderPassDescriptorBuilder : public wgpu::RenderPassDescriptor
    {
    public:
        RenderPassDescriptorBuilder() = default;

        //! Adds the image attachment to the renderpass descriptor
        void Add(const RHI::ImageScopeAttachment& scopeAttachment);
        //! Ends the process of adding images to the renderpass descriptor
        const wgpu::RenderPassDescriptor& End();
        explicit operator bool() const;        

        //! List of color attachment descriptors used for building the renderpass
        AZStd::fixed_vector<wgpu::RenderPassColorAttachment, RHI::Limits::Pipeline::AttachmentColorCountMax> m_wgpuColorAttachments;
        //! Depth/Stencil attachment descriptor
        wgpu::RenderPassDepthStencilAttachment m_wgpuDepthStencilAttachmentInfo = {};
        //! Signals if the renderpass descriptor is valid
        bool m_isValid = false;
    };

    class Scope final
        : public RHI::Scope
    {
        using Base = RHI::Scope;
    public:
        AZ_RTTI(Scope, "{D88CE6EC-D0E9-498B-B471-4035F5CF065C}", Base);
        AZ_CLASS_ALLOCATOR(Scope, AZ::SystemAllocator);

        static RHI::Ptr<Scope> Create();

        void Begin(CommandList& commandList, uint32_t commandListIndex, uint32_t commandListCount);
        void End(CommandList& commandList, uint32_t commandListIndex, uint32_t commandListCount);

        bool HasSignalFence() const;
        bool HasWaitFences() const;

    private: 
        Scope() = default;

        //////////////////////////////////////////////////////////////////////////
        // RHI::Scope
        void DeactivateInternal() override;
        void CompileInternal() override;
        void AddQueryPoolUse([[maybe_unused]] RHI::Ptr<RHI::QueryPool> queryPool, [[maybe_unused]] const RHI::Interval& interval, [[maybe_unused]] RHI::ScopeAttachmentAccess access) override {}
        //////////////////////////////////////////////////////////////////////////

        void BuildRenderPass();

        //! Renderpass descriptor
        RenderPassDescriptorBuilder m_renderPassBuilder = {};
    };
}
