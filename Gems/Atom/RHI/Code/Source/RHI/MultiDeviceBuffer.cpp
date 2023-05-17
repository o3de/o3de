/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/BufferFrameAttachment.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>
#include <Atom/RHI/MultiDeviceBuffer.h>

namespace AZ
{
    namespace RHI
    {
        void MultiDeviceBuffer::SetDescriptor(const BufferDescriptor& descriptor)
        {
            m_descriptor = descriptor;
        }

        void MultiDeviceBuffer::Invalidate()
        {
            m_deviceBuffers.clear();
        }

        const RHI::BufferDescriptor& MultiDeviceBuffer::GetDescriptor() const
        {
            return m_descriptor;
        }

        const BufferFrameAttachment* MultiDeviceBuffer::GetFrameAttachment() const
        {
            return static_cast<const BufferFrameAttachment*>(MultiDeviceResource::GetFrameAttachment());
        }

        const HashValue64 MultiDeviceBuffer::GetHash() const
        {
            HashValue64 hash = HashValue64{ 0 };
            hash = m_descriptor.GetHash();
            return hash;
        }

        void MultiDeviceBuffer::Shutdown()
        {
            for (auto& [_, deviceBuffer] : m_deviceBuffers)
                deviceBuffer->Shutdown();
            MultiDeviceResource::Shutdown();
        }

        void MultiDeviceBuffer::InvalidateViews()
        {
            for (auto& [_, deviceBuffer] : m_deviceBuffers)
                deviceBuffer->InvalidateViews();
        }
    } // namespace RHI
} // namespace AZ
