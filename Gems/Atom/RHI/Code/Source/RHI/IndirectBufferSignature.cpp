/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/IndirectBufferSignature.h>
#include <Atom/RHI/PipelineState.h>

#include <limits>

namespace AZ
{
    namespace RHI
    {
        DeviceIndirectBufferSignatureDescriptor IndirectBufferSignatureDescriptor::GetDeviceIndirectBufferSignatureDescriptor(
            int deviceIndex) const
        {
            DeviceIndirectBufferSignatureDescriptor descriptor{ m_descriptor };

            if (m_pipelineState)
                descriptor.m_pipelineState = m_pipelineState->GetDevicePipelineState(deviceIndex).get();

            return descriptor;
        }

        ResultCode IndirectBufferSignature::Init(DeviceMask deviceMask, const IndirectBufferSignatureDescriptor& descriptor)
        {
            MultiDeviceObject::Init(deviceMask);

            ResultCode resultCode{ ResultCode::Success };

            IterateDevices(
                [this, &descriptor, &resultCode](int deviceIndex)
                {
                    auto device = RHISystemInterface::Get()->GetDevice(deviceIndex);

                    m_deviceIndirectBufferSignatures[deviceIndex] = Factory::Get().CreateIndirectBufferSignature();
                    resultCode = m_deviceIndirectBufferSignatures[deviceIndex]->Init(
                        *device, descriptor.GetDeviceIndirectBufferSignatureDescriptor(deviceIndex));

                    return resultCode == ResultCode::Success;
                });

            m_descriptor = descriptor;

            return resultCode;
        }

        uint32_t IndirectBufferSignature::GetByteStride() const
        {
            AZ_Assert(IsInitialized(), "Signature is not initialized");

            uint32_t byteStride{ std::numeric_limits<uint32_t>::max() };

            for (const auto& [deviceIndex, signature] : m_deviceIndirectBufferSignatures)
            {
                auto deviceByteStride{ signature->GetByteStride() };

                if (byteStride == std::numeric_limits<uint32_t>::max())
                    byteStride = deviceByteStride;

                AZ_Assert(deviceByteStride == byteStride, "Device Signature byte strides do not match");
            }

            return byteStride;
        }

        uint32_t IndirectBufferSignature::GetOffset(IndirectCommandIndex index) const
        {
            AZ_Assert(IsInitialized(), "Signature is not initialized");
            if (Validation::IsEnabled())
            {
                if (index.IsNull())
                {
                    AZ_Assert(false, "Invalid index");
                    return 0;
                }

                if (index.GetIndex() >= m_descriptor.m_descriptor.m_layout.GetCommands().size())
                {
                    AZ_Assert(false, "Index %d is greater than the number of commands on the layout", index.GetIndex());
                    return 0;
                }
            }

            uint32_t offset{ std::numeric_limits<uint32_t>::max() };

            for (const auto& [deviceIndex, signature] : m_deviceIndirectBufferSignatures)
            {
                auto deviceOffset{ signature->GetOffset(index) };

                if (offset == std::numeric_limits<uint32_t>::max())
                    offset = deviceOffset;

                AZ_Assert(deviceOffset == offset, "Device Signature offsets do not match");
            }

            return offset;
        }

        const IndirectBufferSignatureDescriptor& IndirectBufferSignature::GetDescriptor() const
        {
            return m_descriptor;
        }

        const AZ::RHI::IndirectBufferLayout& IndirectBufferSignature::GetLayout() const
        {
            return m_descriptor.m_descriptor.m_layout;
        }

        void IndirectBufferSignature::Shutdown()
        {
            MultiDeviceObject::Shutdown();
        }
    } // namespace RHI
} // namespace AZ
