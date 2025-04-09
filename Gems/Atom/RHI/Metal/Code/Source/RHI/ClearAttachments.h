/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Metal/Metal.h>
#include <RHI/PipelineState.h>
#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/DeviceObject.h>
#include <Atom/RHI/PipelineStateCache.h>
#include <Atom/RHI/StreamBufferView.h>
#include <Atom/RHI.Reflect/ClearValue.h>
#include <Atom/RHI.Reflect/ImageEnums.h>
#include <Atom/RHI.Reflect/Limits.h>

namespace AZ::Metal
{
    class CommandList;

    //! Utility class for clearing render attachments in the middle of a renderpass using a full screen triangle.
    //! Metal only supports clearing using a Load action but for subpasses we need to be able
    //! to clear an attachment at the beginning of a subpass.
    class ClearAttachments:
        public RHI::DeviceObject
    {
    public:
        AZ_CLASS_ALLOCATOR(ClearAttachments, AZ::SystemAllocator);
        
        ClearAttachments() = default;
        //! Initialize the object
        RHI::ResultCode Init(RHI::Device& device);
        //! Shutdown the object
        void Shutdown() override;

        //! Information about clearing a render attachment.
        struct ClearData
        {
            //! Index of the color attachment in the MTLRenderPassDescriptor. Not used for depth/stencil attachments
            uint32_t m_attachmentIndex = RHI::Limits::Pipeline::AttachmentColorCountMax;
            //! Clear value to use
            RHI::ClearValue m_clearValue;
            //! Which aspect of the image to clear. Used when clearing depth/stencil.
            RHI::ImageAspectFlags m_imageAspects = RHI::ImageAspectFlags::None;
        };
        //! Clears a group of attachments (color or depth/stencil) from the renderpass.
        //! It uses a full screen triangle to do the clearing.
        RHI::ResultCode Clear(CommandList& cmdList, MTLRenderPassDescriptor* renderpassDesc, const AZStd::vector<ClearData>& clearAttachmentsData);
        
    private:
        //! Push constants used for specifying the clear color or depth value.
        struct alignas(16) PushConstants
        {
            float m_color[RHI::Limits::Pipeline::AttachmentColorCountMax][4];
            float m_depth[4];
        };

        //! PipelineStateDescriptor used for rendering the full screen triangle
        RHI::PipelineStateDescriptorForDraw m_pipelineStateDescriptor;
        //! Cache of pipeline states used for clearing.
        RHI::PipelineStateSet m_pipelineCache;
        //! Mutex for accessing the pipeline state cache.
        mutable AZStd::mutex m_pipelineCacheMutex;
    };
}
