/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/CopyItem.h>
#include <Atom/RHI/DispatchItem.h>
#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/Configuration.h>
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
        //! Adds one or two scope producers (the second serves to decompose an MS Texture)
        //! which read back one or more mip levels for a particular attachment to CPU memory.
        //! Both buffer and image attachments are supported.
        //! In case of images it can also capture specific Mip Levels, as defined in the image view descriptors.
        //! Also, for images, Volume Texture image attachments (aka Texture3D) are supported too.
        class ATOM_RPI_PUBLIC_API AttachmentReadback final
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

            //! Reads back one or more mip levels from a single pass attachment.
            //! @param readbackName is the name for the readback buffer. And it will be saved in ReadbackResult::m_name. If the name is empty, a name will be generated automatically.
            //! Return true if the pass attachment readback request was submitted.  
            //! The callback function set by SetCallback(CallbackFunction callback) will be called once the readback is finished
            //! @param mipsRange If null, by default it is assumed that @attachment refers to Mip Level 0.
            //!        When different than null, it is used to specify which mip levels to readback from.
            bool ReadPassAttachment(const PassAttachment* attachment, const AZ::Name& readbackName, const RHI::ImageSubresourceRange* mipsRange = nullptr);

            // Helper struct that records mip level and mip dimensions.
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

                // Only valid for image attachments
                RHI::ImageDescriptor m_imageDescriptor;
                // REMARK: For compatibility reasons, The above @m_dataBuffer will point
                // to the the buffer of the first Mip Level.
                struct MipDataBuffer
                {
                    AZStd::shared_ptr<AZStd::vector<uint8_t>> m_mipBuffer;
                    MipInfo m_mipInfo;
                };
                // With this vector of buffers, we can notify in a single
                // call, all the data for all the requested mip levels.
                AZStd::vector<MipDataBuffer> m_mipDataBuffers;
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
            bool CopyBufferData(uint32_t readbackBufferIndex, int deviceIndex);

            // Get read back data in a structure
            struct ReadbackItem;
            ReadbackResult GetReadbackResult() const;

            // Attachment to be read back
            RHI::AttachmentId m_attachmentId;
            RHI::AttachmentType m_attachmentType;

            // For copy scope producer ...
            // The buffer attachment's size in bytes
            uint64_t m_bufferAttachmentByteSize = 0;

            AZ::Name m_readbackName;

            RHI::AttachmentId m_copyAttachmentId;

            // The input image attachment's descriptor
            RHI::ImageDescriptor m_imageDescriptor;
            RHI::ImageSubresourceRange m_imageMipsRange;

            struct ReadbackItem
            {
                // The copy item used to read back a buffer, or a particular mip level of an image
                RHI::CopyItem m_copyItem;

                // Host accessible buffer to save read back result
                // Using triple buffer pointers, as it allows use to clear the buffer outside the async callback.
                // It helps with an issue where during the buffer cleanup there was a chance to hit the assert
                // related to disconnecting a bus during a dispatch on a lockless Bus.
                AZStd::fixed_vector<Data::Instance<Buffer>, RHI::Limits::Device::FrameCountMax> m_readbackBufferArray;

                // Data buffer for final result
                AZStd::shared_ptr<AZStd::vector<uint8_t>> m_dataBuffer;

                MipInfo m_mipInfo; // Only relevant for image type.
            };
            // Contains all the copy items.
            // When m_attachmentType is Buffer, there will be only one item
            // in this vector.
            // When m_attachmentType is Image, there will be one item for each mip level
            // specified in the input image view descriptor.
            AZStd::vector<ReadbackItem> m_readbackItems;

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
