/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/DeviceBuffer.h>
#include <Atom/RHI/BufferFrameAttachment.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>

namespace AZ::RHI
{
    void DeviceBuffer::SetDescriptor(const BufferDescriptor& descriptor)
    {
        m_descriptor = descriptor;
    }

    const RHI::BufferDescriptor& DeviceBuffer::GetDescriptor() const
    {
        return m_descriptor;
    }

    const BufferFrameAttachment* DeviceBuffer::GetFrameAttachment() const
    {
        return static_cast<const BufferFrameAttachment*>(DeviceResource::GetFrameAttachment());
    }

    void DeviceBuffer::ReportMemoryUsage(MemoryStatisticsBuilder& builder) const
    {
        const BufferDescriptor& descriptor = GetDescriptor();

        MemoryStatistics::Buffer* bufferStats = builder.AddBuffer();
        bufferStats->m_name = GetName();
        bufferStats->m_bindFlags = descriptor.m_bindFlags;
        bufferStats->m_sizeInBytes = descriptor.m_byteCount;
    }
    
    Ptr<DeviceBufferView> DeviceBuffer::GetBufferView(const BufferViewDescriptor& bufferViewDescriptor)
    {
        return Base::GetResourceView(bufferViewDescriptor);
    }

    const HashValue64 DeviceBuffer::GetHash() const
    {
        HashValue64 hash = HashValue64{ 0 };
        hash = m_descriptor.GetHash();
        return hash;
    }
}
