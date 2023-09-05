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
#include <Atom/RHI/DeviceObject.h>
#include <Atom/RHI/IndirectBufferSignature.h>

#include <Atom/RHI/MultiDeviceObject.h>

namespace AZ::RHI
{
    class Device;
    class MultiDevicePipelineState;

    //! A multi-device descriptor for the MultiDeviceIndirectBufferSignature, holding both an IndirectBufferLayout (identical across
    //! devices) as well as a MultiDevicePipelineState
    struct MultiDeviceIndirectBufferSignatureDescriptor
    {
        //! Returns the device-specific IndirectBufferSignatureDescriptor for the given index
        IndirectBufferSignatureDescriptor GetDeviceIndirectBufferSignatureDescriptor(int deviceIndex) const;

        const MultiDevicePipelineState* m_pipelineState{ nullptr };
        IndirectBufferLayout m_layout;
    };

    //! The MultiDeviceIndirectBufferSignature is an implementation object that represents
    //! the signature of the commands contained in an Indirect Buffer.
    //! Indirect Buffers hold the commands that will be used for
    //! doing Indirect Rendering.
    //!
    //! It also exposes implementation dependent offsets for the commands in
    //! a layout. This information is useful when writing commands into a buffer.
    class MultiDeviceIndirectBufferSignature : public MultiDeviceObject
    {
        using Base = RHI::MultiDeviceObject;

    public:
        AZ_CLASS_ALLOCATOR(MultiDeviceIndirectBufferSignature, AZ::SystemAllocator, 0);
        AZ_RTTI(MultiDeviceIndirectBufferSignature, "{3CCFF81D-DC5E-4B12-AC05-DC26D5D0C65C}", Base);
        AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(IndirectBufferSignature);
        MultiDeviceIndirectBufferSignature() = default;
        virtual ~MultiDeviceIndirectBufferSignature() = default;

        //! Initialize an IndirectBufferSignature object.
        //! @param deviceMask The deviceMask denoting all devices that will contain the signature.
        //! @param descriptor Descriptor with the necessary information for initializing the signature.
        //! @return A result code denoting the status of the call. If successful, the MultiDeviceIndirectBufferSignature is considered
        //! initialized and can be used. If failure, the MultiDeviceIndirectBufferSignature remains uninitialized.
        ResultCode Init(MultiDevice::DeviceMask deviceMask, const MultiDeviceIndirectBufferSignatureDescriptor& descriptor);

        //! Returns the stride in bytes of the command sequence defined by the provided layout.
        uint32_t GetByteStride() const;

        //! Returns the offset of the command in the position indicated by the index.
        //! @param index The location in the layout of the command.
        uint32_t GetOffset(IndirectCommandIndex index) const;

        const MultiDeviceIndirectBufferSignatureDescriptor& GetDescriptor() const;

        const IndirectBufferLayout& GetLayout() const;

        void Shutdown() final;

    private:
        MultiDeviceIndirectBufferSignatureDescriptor m_mdDescriptor;
    };
} // namespace AZ::RHI
