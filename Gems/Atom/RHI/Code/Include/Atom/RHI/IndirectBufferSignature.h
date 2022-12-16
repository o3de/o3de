/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI.Reflect/IndirectBufferLayout.h>
#include <Atom/RHI/DeviceIndirectBufferSignature.h>

#include <Atom/RHI/MultiDeviceObject.h>

namespace AZ
{
    namespace RHI
    {
        class Device;
        class PipelineState;

        struct IndirectBufferSignatureDescriptor
        {
            DeviceIndirectBufferSignatureDescriptor GetDeviceIndirectBufferSignatureDescriptor(int deviceIndex) const;

            const PipelineState* m_pipelineState = nullptr;
            DeviceIndirectBufferSignatureDescriptor m_descriptor;
        };

        //! The IndirectBufferSignature is an implementation object that represents
        //! the signature of the commands contained in an Indirect Buffer.
        //! Indirect Buffers hold the commands that will be used for
        //! doing Indirect Rendering.
        //!
        //! It also exposes implementation dependent offsets for the commands in
        //! a layout. This information is useful when writing commands into a buffer.
        class IndirectBufferSignature : public MultiDeviceObject
        {
            using Base = RHI::MultiDeviceObject;

        public:
            AZ_CLASS_ALLOCATOR(IndirectBufferSignature, AZ::SystemAllocator, 0);
            AZ_RTTI(IndirectBufferSignature, "IndirectBufferSignature", Base);

            static RHI::Ptr<IndirectBufferSignature> Create()
            {
                return aznew IndirectBufferSignature;
            }

            const RHI::Ptr<DeviceIndirectBufferSignature>& GetDeviceIndirectBufferSignature(int deviceIndex) const
            {
                AZ_Assert(
                    m_deviceIndirectBufferSignatures.find(deviceIndex) != m_deviceIndirectBufferSignatures.end(),
                    "No DeviceIndirectBufferSignature found for device index %d\n",
                    deviceIndex);
                return m_deviceIndirectBufferSignatures.at(deviceIndex);
            }

            //! Initialize an IndirectBufferSignature object.
            //! @param device The device that will contain the signature.
            //! @param descriptor Descriptor with the necessary information for initializing the signature.
            //! @return A result code denoting the status of the call. If successful, the IndirectBufferSignature is considered
            //!      initialized and can be used. If failure, the IndirectBufferSignature remains uninitialized.
            ResultCode Init(DeviceMask deviceMask, const IndirectBufferSignatureDescriptor& descriptor);

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
            IndirectBufferSignatureDescriptor m_descriptor;
            AZStd::unordered_map<int, Ptr<DeviceIndirectBufferSignature>> m_deviceIndirectBufferSignatures;
        };
    } // namespace RHI
} // namespace AZ
