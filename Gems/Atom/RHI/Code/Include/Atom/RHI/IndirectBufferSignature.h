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
#include <Atom/RHI/DeviceIndirectBufferSignature.h>

#include <Atom/RHI/MultiDeviceObject.h>

namespace AZ::RHI
{
    class Device;
    class PipelineState;

    //! A multi-device descriptor for the IndirectBufferSignature, holding both an IndirectBufferLayout (identical across
    //! devices) as well as a PipelineState
    struct IndirectBufferSignatureDescriptor
    {
        //! Returns the device-specific DeviceIndirectBufferSignatureDescriptor for the given index
        DeviceIndirectBufferSignatureDescriptor GetDeviceIndirectBufferSignatureDescriptor(int deviceIndex) const;

        const PipelineState* m_pipelineState{ nullptr };
        IndirectBufferLayout m_layout;
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
        AZ_RTTI(IndirectBufferSignature, "{3CCFF81D-DC5E-4B12-AC05-DC26D5D0C65C}", Base);
        AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(IndirectBufferSignature);
        IndirectBufferSignature() = default;
        virtual ~IndirectBufferSignature() = default;

        //! Initialize an DeviceIndirectBufferSignature object.
        //! @param deviceMask The deviceMask denoting all devices that will contain the signature.
        //! @param descriptor Descriptor with the necessary information for initializing the signature.
        //! @return A result code denoting the status of the call. If successful, the IndirectBufferSignature is considered
        //! initialized and can be used. If failure, the IndirectBufferSignature remains uninitialized.
        ResultCode Init(MultiDevice::DeviceMask deviceMask, const IndirectBufferSignatureDescriptor& descriptor);

        //! Returns the stride in bytes of the command sequence defined by the provided layout.
        uint32_t GetByteStride() const;

        //! Returns the offset of the command in the position indicated by the index.
        //! @param index The location in the layout of the command.
        uint32_t GetOffset(IndirectCommandIndex index) const;

        const IndirectBufferSignatureDescriptor& GetDescriptor() const;

        const IndirectBufferLayout& GetLayout() const;

        void Shutdown() final;

    private:
        IndirectBufferSignatureDescriptor m_descriptor;
        static constexpr uint32_t UNINITIALIZED_VALUE{ std::numeric_limits<uint32_t>::max() };
        uint32_t m_byteStride{ UNINITIALIZED_VALUE };
    };
} // namespace AZ::RHI
