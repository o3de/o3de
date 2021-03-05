/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
