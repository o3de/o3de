/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/BufferFrameAttachment.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>

namespace AZ
{
    namespace RHI
    {
        void Buffer::SetDescriptor(const BufferDescriptor& descriptor)
        {
            m_descriptor = descriptor;
        }

        const RHI::BufferDescriptor& Buffer::GetDescriptor() const
        {
            return m_descriptor;
        }

        const BufferFrameAttachment* Buffer::GetFrameAttachment() const
        {
            return static_cast<const BufferFrameAttachment*>(Resource::GetFrameAttachment());
        }

        void Buffer::ReportMemoryUsage(MemoryStatisticsBuilder& builder) const
        {
            const BufferDescriptor& descriptor = GetDescriptor();

            MemoryStatistics::Buffer* bufferStats = builder.AddBuffer();
            bufferStats->m_name = GetName();
            bufferStats->m_bindFlags = descriptor.m_bindFlags;
            bufferStats->m_sizeInBytes = descriptor.m_byteCount;
        }
    
        Ptr<BufferView> Buffer::GetBufferView(const BufferViewDescriptor& bufferViewDescriptor)
        {
            return Base::GetResourceView(bufferViewDescriptor);
        }
    }
}
