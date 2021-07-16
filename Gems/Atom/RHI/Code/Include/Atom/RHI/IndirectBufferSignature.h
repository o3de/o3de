/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceObject.h>
#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI.Reflect/IndirectBufferLayout.h>

namespace AZ
{
    namespace RHI
    {
        class Device;
        class PipelineState;
 

        struct IndirectBufferSignatureDescriptor
        {
            IndirectBufferLayout m_layout;
            const PipelineState* m_pipelineState = nullptr;
        };

        //! The IndirectBufferSignature is an implementation object that represents
        //! the signature of the commands contained in an Indirect Buffer.
        //! Indirect Buffers hold the commands that will be used for
        //! doing Indirect Rendering.
        //!
        //! It also exposes implementation dependent offsets for the commands in
        //! a layout. This information is useful when writing commands into a buffer.
        class IndirectBufferSignature :
            public DeviceObject
        {
            using Base = RHI::DeviceObject;
        public:
            AZ_RTTI(IndirectBufferSignature, "IndirectBufferSignature", Base);
            virtual ~IndirectBufferSignature() {};

            //! Initialize an IndirectBufferSignature object.
            //! @param device The device that will contain the signature.
            //! @param descriptor Descriptor with the necessary information for initializing the signature.
            //! @return A result code denoting the status of the call. If successful, the IndirectBufferSignature is considered
            //!      initialized and can be used. If failure, the IndirectBufferSignature remains uninitialized.
            ResultCode Init(Device& device, const IndirectBufferSignatureDescriptor& descriptor);

            /// Returns the stride in bytes of the command sequence defined by the provided layout.
            uint32_t GetByteStride() const;

            //! Returns the offset of the command in the position indicated by the index.
            //! @param index The location in the layout of the command.
            uint32_t GetOffset(IndirectCommandIndex index) const;

            const IndirectBufferSignatureDescriptor& GetDescriptor() const;

            const IndirectBufferLayout& GetLayout() const;

            void Shutdown() final;

        protected:
            IndirectBufferSignature() = default;

        private:
            ///////////////////////////////////////////////////////////////////
            // Platform API

            /// Called when initializing the object,
            virtual RHI::ResultCode InitInternal(Device& device, const IndirectBufferSignatureDescriptor& descriptor) = 0;
            /// Called when requesting the stride of the command sequence.
            virtual uint32_t GetByteStrideInternal() const = 0;
            /// Called when requesting the offset of a command.
            virtual uint32_t GetOffsetInternal(IndirectCommandIndex index) const = 0;
            /// Called when the signature is being shutdown.
            virtual void ShutdownInternal() = 0;

            ///////////////////////////////////////////////////////////////////

            IndirectBufferSignatureDescriptor m_descriptor;
        };
    }
}
