/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/MultiDeviceIndirectBufferSignature.h>
#include <Atom/RHI/MultiDevicePipelineState.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ::RHI
{
    IndirectBufferSignatureDescriptor MultiDeviceIndirectBufferSignatureDescriptor::GetDeviceIndirectBufferSignatureDescriptor(
        int deviceIndex) const
    {
        AZ_Assert(m_pipelineState, "No MultiDevicePipelineState available\n");

        IndirectBufferSignatureDescriptor descriptor{ m_layout };

        if (m_pipelineState)
        {
            descriptor.m_pipelineState = m_pipelineState->GetDevicePipelineState(deviceIndex).get();
        }

        return descriptor;
    }

    ResultCode MultiDeviceIndirectBufferSignature::Init(
        MultiDevice::DeviceMask deviceMask, const MultiDeviceIndirectBufferSignatureDescriptor& descriptor)
    {
        MultiDeviceObject::Init(deviceMask);

        ResultCode resultCode{ ResultCode::Success };

        IterateDevices(
            [this, &descriptor, &resultCode](int deviceIndex)
            {
                auto device = RHISystemInterface::Get()->GetDevice(deviceIndex);

                m_deviceObjects[deviceIndex] = Factory::Get().CreateIndirectBufferSignature();
                resultCode = GetDeviceIndirectBufferSignature(deviceIndex)->Init(
                    *device, descriptor.GetDeviceIndirectBufferSignatureDescriptor(deviceIndex));

                return resultCode == ResultCode::Success;
            });

        m_mdDescriptor = descriptor;

        return resultCode;
    }

    uint32_t MultiDeviceIndirectBufferSignature::GetByteStride() const
    {
        AZ_Assert(IsInitialized(), "Signature is not initialized");

        uint32_t byteStride{ std::numeric_limits<uint32_t>::max() };

        IterateObjects<IndirectBufferSignature>([&byteStride]([[maybe_unused]] auto deviceIndex, auto deviceSignature)
        {
            auto deviceByteStride{ deviceSignature->GetByteStride() };

            if (byteStride == std::numeric_limits<uint32_t>::max())
            {
                byteStride = deviceByteStride;
            }

            AZ_Assert(deviceByteStride == byteStride, "Device Signature byte strides do not match");
        });

        return byteStride;
    }

    uint32_t MultiDeviceIndirectBufferSignature::GetOffset(IndirectCommandIndex index) const
    {
        AZ_Assert(IsInitialized(), "Signature is not initialized");
        if (Validation::IsEnabled())
        {
            if (index.IsNull())
            {
                AZ_Assert(false, "Invalid index");
                return 0;
            }

            if (index.GetIndex() >= m_mdDescriptor.m_layout.GetCommands().size())
            {
                AZ_Assert(false, "Index %d is greater than the number of commands on the layout", index.GetIndex());
                return 0;
            }
        }

        uint32_t offset{ std::numeric_limits<uint32_t>::max() };

        IterateObjects<IndirectBufferSignature>([&offset, &index]([[maybe_unused]] auto deviceIndex, auto deviceSignature)
        {
            auto deviceOffset{ deviceSignature->GetOffset(index) };

            if (offset == std::numeric_limits<uint32_t>::max())
            {
                offset = deviceOffset;
            }

            AZ_Assert(deviceOffset == offset, "Device Signature offsets do not match");
        });

        return offset;
    }

    const MultiDeviceIndirectBufferSignatureDescriptor& MultiDeviceIndirectBufferSignature::GetDescriptor() const
    {
        return m_mdDescriptor;
    }

    const AZ::RHI::IndirectBufferLayout& MultiDeviceIndirectBufferSignature::GetLayout() const
    {
        return m_mdDescriptor.m_layout;
    }

    void MultiDeviceIndirectBufferSignature::Shutdown()
    {
        MultiDeviceObject::Shutdown();
    }
} // namespace AZ::RHI
