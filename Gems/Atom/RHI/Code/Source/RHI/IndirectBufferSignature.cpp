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
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ::RHI
{
    DeviceIndirectBufferSignatureDescriptor IndirectBufferSignatureDescriptor::GetDeviceIndirectBufferSignatureDescriptor(
        int deviceIndex) const
    {
        DeviceIndirectBufferSignatureDescriptor descriptor{ m_layout };

        if (m_pipelineState)
        {
            descriptor.m_pipelineState = m_pipelineState->GetDevicePipelineState(deviceIndex).get();
        }

        return descriptor;
    }

    ResultCode IndirectBufferSignature::Init(
        MultiDevice::DeviceMask deviceMask, const IndirectBufferSignatureDescriptor& descriptor)
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

                if(m_byteStride == UNINITIALIZED_VALUE)
                {
                    // Cache byteStride since it is the same for all devices
                    m_byteStride = GetDeviceIndirectBufferSignature(deviceIndex)->GetByteStride();
                }

                return resultCode == ResultCode::Success;
            });

        if (const auto& name = GetName(); !name.IsEmpty())
        {
            SetName(name);
        }

        m_descriptor = descriptor;

        return resultCode;
    }

    uint32_t IndirectBufferSignature::GetByteStride() const
    {
        AZ_Assert(IsInitialized(), "Signature is not initialized");
        return m_byteStride;
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

            if (index.GetIndex() >= m_descriptor.m_layout.GetCommands().size())
            {
                AZ_Assert(false, "Index %d is greater than the number of commands on the layout", index.GetIndex());
                return 0;
            }
        }

        auto offset{ UNINITIALIZED_VALUE };

        IterateObjects<DeviceIndirectBufferSignature>([&offset, &index]([[maybe_unused]] auto deviceIndex, auto deviceSignature)
        {
            auto deviceOffset{ deviceSignature->GetOffset(index) };

            if (offset == UNINITIALIZED_VALUE)
            {
                offset = deviceOffset;
            }

            AZ_Assert(deviceOffset == offset, "Device Signature offsets do not match");
        });

        return offset;
    }

    const IndirectBufferSignatureDescriptor& IndirectBufferSignature::GetDescriptor() const
    {
        return m_descriptor;
    }

    const AZ::RHI::IndirectBufferLayout& IndirectBufferSignature::GetLayout() const
    {
        return m_descriptor.m_layout;
    }

    void IndirectBufferSignature::Shutdown()
    {
        MultiDeviceObject::Shutdown();
    }
} // namespace AZ::RHI
