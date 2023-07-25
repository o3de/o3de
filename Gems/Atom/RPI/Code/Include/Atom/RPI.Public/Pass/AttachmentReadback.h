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

namespace AZ
{
    namespace RHI
    {
        class Fence;
    }

    namespace RPI
    {
        //! Adds one or two scope producers ((the second is to decompose an MS Texture)
        //! which read back one or more attachments to CPU memory.
        //! Both buffer and image attachments are supported.
        //! In case of images it can also capture specific Mip Levels, as defined in the image view descriptors.
        //! Also, for images, Volume Texture image attachments (aka Texture3D) are supported too.
        class AttachmentReadback final
        {
        public:
            AZ_RTTI(AttachmentReadback, "{9C70ACD3-8694-4EF3-A556-9DA25BD1237C}");
            AZ_CLASS_ALLOCATOR(AttachmentReadback, SystemAllocator);

            enum class ReadbackState : uint32_t
            {
                Uninitialized = 0,
                Idle,
                AttachmentSet,
                Reading,
                Success,
                Failed
            };

            AttachmentReadback(const RHI::ScopeId& scopeId);
            virtual ~AttachmentReadback();

            //! Reads back a single pass attachment.
            //! @param readbackName is the name for the readback buffer. And it will be saved in ReadbackResult::m_name. If the name is empty, a name will be generated automatically.
            //! Return true if the pass attachment readback request was submitted.  
            //! The callback function set by SetCallback(CallbackFunction callback) will be called once the readback is finished
            //! @param imageViewDescriptor If null, by default it is assumed that @attachment refers to Mip Level 0.
            //!        When different than null, the image view descriptor can be used to specify which mip level is the attachment bound to.
            bool ReadPassAttachment(const PassAttachment* attachment, const AZ::Name& readbackName, const RHI::ImageViewDescriptor* imageViewDescriptor = nullptr);


            struct ReadbackRequestInfo
            {
                // Same semantic as ReadPassAttachment Arg 0.
                const PassAttachment* m_attachment = nullptr;
                // Same semantic as ReadPassAttachment Arg 1.
                Name m_readbackName;
                // With a properly set imageViewDescriptor we can readback
                // a particular mip level.
                RHI::ImageViewDescriptor m_imageViewDescriptor;
            };
            //! Reads back multiple pass attachments in one frame.
            bool ReadPassAttachments(const AZStd::vector<ReadbackRequestInfo>& readbackAttachmentRequests);

            // Helper struct that records mip level and mip dimensions
            // for a particular attachment.
            struct MipInfo
            {
                uint16_t m_slice = 0;
                RHI::Size m_size = {};
            };

            struct ReadbackResult
            {
                ReadbackState m_state = ReadbackState::Idle;
                RHI::AttachmentType m_attachmentType;
                AZStd::shared_ptr<AZStd::vector<uint8_t>> m_dataBuffer;
                AZ::Name m_name;
                uint32_t m_userIdentifier;

                // only valid for image attachments
                RHI::ImageDescriptor m_imageDescriptor;
                MipInfo m_mipInfo;
            };

            void Reset();

            ReadbackState GetReadbackState();

            using CallbackFunction = AZStd::function<void(const ReadbackResult&)>;

            //! Set a callback function which will be called when readback is finished or failed
            void SetCallback(CallbackFunction callback);

            //! Set the using systems identifier tag
            void SetUserIdentifier(uint32_t userIdentifier);

            //! Whether the previous readback is finished
            bool IsFinished() const;

            //! Whether it's ready to readback an attachment
            //! Returns false if it's not initialized or it's in the process of readback from another attachment
            bool IsReady() const;

            //! Prepare this scope producer for the frame.
            void FrameBegin(Pass::FramePrepareParams params);

        private:

            // Scope producer functions for copy
            void CopyPrepare(RHI::FrameGraphInterface frameGraph);
            void CopyCompile(const RHI::FrameGraphCompileContext& context);
            void CopyExecute(const RHI::FrameGraphExecuteContext& context);

            // Scope producer functions for decomposing multi-sample image
            void DecomposePrepare(RHI::FrameGraphInterface frameGraph);
            void DecomposeCompile(const RHI::FrameGraphCompileContext& context);
            void DecomposeExecute(const RHI::FrameGraphExecuteContext& context);

            // copy data from the read back buffer (m_readbackBuffer) to the data buffer (m_dataBuffer)
            bool CopyBufferData(uint32_t readbackBufferIndex);

            // Get read back data in a structure
            struct AttachmentReadbackItem;
            ReadbackResult GetReadbackResult(const AttachmentReadbackItem& readbackItem) const;

            // A helper function.
            // This function is only called if @attachment is an image attachment.
            RHI::ImageDescriptor GetImageDescriptorFromAttachment(const PassAttachment* attachment);

            // A helper function that creates a default image view descriptor
            // for cases when ReadPassAttachment() is called with null image view descriptor.
            // The returned view descriptor will assume that the attachment is bound to Mip Level 0.
            // This function is only called if @attachment is an image attachment.
            RHI::ImageViewDescriptor CreateDefaultImageViewDescriptorFromAttachment(const RHI::ImageDescriptor& imageDescriptor);

            struct AttachmentReadbackItem
            {
                // Attachment to be read back
                RHI::AttachmentId m_attachmentId;
                RHI::AttachmentType m_attachmentType;

                // For copy scope producer ...
                // The buffer attachment's size in bytes
                uint64_t m_bufferAttachmentByteSize = 0;

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
            AZStd::vector<AttachmentReadbackItem> m_attachmentReadbackItems; // Contains all the copy items.

            AZStd::fixed_vector<bool, RHI::Limits::Device::FrameCountMax> m_isReadbackComplete;
            uint32_t m_readbackBufferCurrentIndex = 0u;
            uint32_t m_userIdentifier = static_cast<uint32_t>(-1); // needs to match AZ::Render::InvalidFrameCaptureId

            ReadbackState m_state = ReadbackState::Uninitialized;

            Ptr<RHI::Fence> m_fence;

            // Callback function when read back finished
            CallbackFunction m_callback = nullptr;

            // For decompose multisample image to an non-multisample image
            Data::Instance<Shader> m_decomposeShader;
            Data::Instance<ShaderResourceGroup> m_decomposeSrg;
            RHI::ShaderInputImageIndex m_decomposeInputImageIndex;
            RHI::ShaderInputImageIndex m_decomposeOutputImageIndex;
            RHI::DispatchItem m_dispatchItem;

            // Scope producer for decomposing multi-sample image
            AZStd::shared_ptr<AZ::RHI::ScopeProducer> m_decomposeScopeProducer;

            // Scope producer for copy image or buffer to read-back buffer
            AZStd::shared_ptr<AZ::RHI::ScopeProducer> m_copyScopeProducer;
        };
        
    }   // namespace RPI
}   // namespace AZ
