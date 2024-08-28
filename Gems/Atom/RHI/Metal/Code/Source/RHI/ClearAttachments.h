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

    class ClearAttachments:
        public RHI::DeviceObject
    {
    public:
        AZ_CLASS_ALLOCATOR(ClearAttachments, AZ::SystemAllocator);
        
        ClearAttachments() = default;
        RHI::ResultCode Init(RHI::Device& device);
        void Shutdown() override;
        struct ClearData
        {
            uint32_t m_attachmentIndex = RHI::Limits::Pipeline::AttachmentColorCountMax;
            RHI::ClearValue m_clearValue;
            RHI::ImageAspectFlags m_imageAspects = RHI::ImageAspectFlags::None;
        };
        RHI::ResultCode Clear(CommandList& cmdList, MTLRenderPassDescriptor* renderpassDesc, const AZStd::vector<ClearData>& clearAttachmentsData);
        
    private:
        struct alignas(16) PushConstants
        {
            float m_color[RHI::Limits::Pipeline::AttachmentColorCountMax][4];
            float m_depth;
            float m_padding[3];
        };
        
        RHI::PipelineStateDescriptorForDraw m_pipelineStateDescriptor;
        RHI::PipelineStateSet m_pipelineCache;
        mutable AZStd::mutex m_pipelineCacheMutex;
    };
}
