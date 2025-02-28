/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/BufferFrameAttachment.h>
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>


namespace AZ::RHI
{
    void Buffer::SetDescriptor(const BufferDescriptor& descriptor)
    {
        m_descriptor = descriptor;
    }

    void Buffer::Invalidate()
    {
        m_deviceObjects.clear();
    }

    const RHI::BufferDescriptor& Buffer::GetDescriptor() const
    {
        return m_descriptor;
    }

    const BufferFrameAttachment* Buffer::GetFrameAttachment() const
    {
        return static_cast<const BufferFrameAttachment*>(Resource::GetFrameAttachment());
    }

    Ptr<BufferView> Buffer::BuildBufferView(const BufferViewDescriptor& bufferViewDescriptor)
    {
        return Base::GetResourceView(bufferViewDescriptor);
    }

    const HashValue64 Buffer::GetHash() const
    {
        HashValue64 hash = HashValue64{ 0 };
        hash = m_descriptor.GetHash();
        return hash;
    }

    void Buffer::Shutdown()
    {
        Resource::Shutdown();
    }
} // namespace AZ::RHI
