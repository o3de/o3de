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
        //! A scope producer which reads back buffer or image attachments 
        class AttachmentReadback
        {
        public:
            AZ_RTTI(AttachmentReadback, "{9C70ACD3-8694-4EF3-A556-9DA25BD1237C}");
            AZ_CLASS_ALLOCATOR(AttachmentReadback, SystemAllocator, 0);

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

            //! Readback a pass attachment.
            //! @param readbackName is the name for the readback buffer. And it will be saved in ReadbackResult::m_name. If the name is empty, a name will be generated automatically.
            //! Return true if the pass attachment readback request was submitted.  
            //! The callback function set by SetCallback(CallbackFunction callback) will be called once the readback is finished
            bool ReadPassAttachment(const PassAttachment* attachment, const AZ::Name& readbackName);

            struct ReadbackResult
            {
                ReadbackState m_state = ReadbackState::Idle;
                RHI::AttachmentType m_attachmentType;
                AZStd::shared_ptr<AZStd::vector<uint8_t>> m_dataBuffer;
                AZ::Name m_name;

                // only valid for image attachments
                RHI::ImageDescriptor m_imageDescriptor;
            };

            void Reset();

            ReadbackState GetReadbackState();

            using CallbackFunction = AZStd::function<void(const ReadbackResult&)>;

            //! Set a callback function which will be called when readback is finished or failed
            void SetCallback(CallbackFunction callback);

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
            ReadbackResult GetReadbackResult() const;

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
            AZStd::fixed_vector<bool, RHI::Limits::Device::FrameCountMax> m_isReadbackComplete;
            uint32_t m_readbackBufferCurrentIndex = 0u;
            AZ::Name m_readbackName;

            RHI::AttachmentId m_copyAttachmentId;

            // Data buffer for final result
            AZStd::shared_ptr<AZStd::vector<uint8_t>> m_dataBuffer;

            // The input image attachment's descriptor
            RHI::ImageDescriptor m_imageDescriptor;
            
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
