/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/CopyItem.h>
#include <Atom/RHI/DispatchItem.h>
#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Pass/AttachmentReadback.h>

namespace AZ
{
    namespace RPI
    {
        //! A scope producer which can read back multiple buffers or image attachments
        //! at once. If you only need to capture a single attachment, then you should use
        //! the base class AttachmentReadback.
        class AttachmentsReadbackGroup final : public AttachmentReadback
        {
        public:
            AZ_RTTI(AttachmentsReadbackGroup, "{21151516-FC16-40D8-AAC4-C808C04BE475}", AttachmentReadback);
            AZ_CLASS_ALLOCATOR(AttachmentsReadbackGroup, SystemAllocator);

            AttachmentsReadbackGroup(const RHI::ScopeId& scopeId);

            virtual ~AttachmentsReadbackGroup();

            ///////////////////////////////////////////////////////////
            // AttachmentReadback public overrides ...
            bool ReadPassAttachments(const AZStd::vector<ReadbackRequestInfo>& readbackAttachmentRequests) override;
            void Reset() override;
            ///////////////////////////////////////////////////////////

            struct MipInfo
            {
                uint16_t m_slice = 0;
                RHI::Size m_size = {};
            };

            // Unlike AttachmentReadback, when this class reports
            // back the result, it includes basic Mip level info
            // about the attachment that was read to CPU memory.
            struct ReadbackResultWithMip : public ReadbackResult
            {
                MipInfo m_mipInfo;
            };

        private:

            ///////////////////////////////////////////////////////////
            // AttachmentReadback overrides ...
            // Scope producer functions for copy
            void CopyPrepare(RHI::FrameGraphInterface frameGraph) override;
            void CopyCompile(const RHI::FrameGraphCompileContext& context) override;
            void CopyExecute(const RHI::FrameGraphExecuteContext& context) override;
            ///////////////////////////////////////////////////////////

            // In a single call to this function, this function copies
            // data from the read back buffer (readbackItem.m_readbackBufferArray[readbackBufferIndex])
            // to the smart pointer data buffer (readbackItem.m_dataBuffer)
            // for ALL the Attachments.
            bool CopyBufferData(uint32_t readbackBufferIndex);

            struct AttachmentReadbackItem
            {
                // Attachment to be read back
                RHI::AttachmentId m_attachmentId;
                RHI::AttachmentType m_attachmentType;

                // For copy scope producer ...
                // The buffer attachment's size in bytes
                uint64_t m_bufferAttachmentByteSize;

                // The copy item used to copy an image or buffer to a read back buffer
                RHI::CopyItem m_copyItem;

                // Host accessible buffer to save read back result
                // Using triple buffer pointers, as it allows use to clear the buffer outside the async callback.
                // It helps with an issue where during the buffer cleanup there was a chance to hit the assert
                // related to disconnecting a bus during a dispatch on a lockless Bus.
                AZStd::fixed_vector<Data::Instance<Buffer>, RHI::Limits::Device::FrameCountMax> m_readbackBufferArray;

                AZ::Name m_readbackName;
                RHI::AttachmentId m_copyAttachmentId;
                // Data buffer for final result
                AZStd::shared_ptr<AZStd::vector<uint8_t>> m_dataBuffer;

                // The input image attachment's descriptor
                RHI::ImageDescriptor m_imageDescriptor;
                RHI::ImageViewDescriptor m_imageViewDescriptor;
                MipInfo m_imageMipInfo;
            };
            AZStd::vector<AttachmentReadbackItem> m_attachmentReadbackItems;

            // Get read back data in the final callback structure for a single AttachmentReadbackItem.
            ReadbackResultWithMip GetReadbackResult(const AttachmentReadbackItem& readbackItem) const;

            // Scope producer for copy image or buffer to read-back buffer
            AZStd::shared_ptr<AZ::RHI::ScopeProducer> m_copyScopeProducer;
        };
        
    }   // namespace RPI
}   // namespace AZ
