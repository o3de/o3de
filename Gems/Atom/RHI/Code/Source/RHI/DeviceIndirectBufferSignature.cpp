/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/DeviceIndirectBufferSignature.h>

namespace AZ::RHI
{
    ResultCode DeviceIndirectBufferSignature::Init(Device& device, const DeviceIndirectBufferSignatureDescriptor& descriptor)
    {
        ResultCode result = InitInternal(device, descriptor);
        if (result == ResultCode::Success)
        {
            Base::Init(device);
            m_descriptor = descriptor;
        }

        return result;
    }

    uint32_t DeviceIndirectBufferSignature::GetByteStride() const
    {
        AZ_Assert(IsInitialized(), "Signature is not initialized");
        return GetByteStrideInternal();
    }

    uint32_t DeviceIndirectBufferSignature::GetOffset(IndirectCommandIndex index) const
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

        return GetOffsetInternal(index);
    }

    const DeviceIndirectBufferSignatureDescriptor& DeviceIndirectBufferSignature::GetDescriptor() const
    {
        return m_descriptor;
    }

    const AZ::RHI::IndirectBufferLayout& DeviceIndirectBufferSignature::GetLayout() const
    {
        return m_descriptor.m_layout;
    }

    void DeviceIndirectBufferSignature::Shutdown()
    {
        ShutdownInternal();
        DeviceObject::Shutdown();
    }
}
