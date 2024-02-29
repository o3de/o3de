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

namespace AZ::RHI
{
    void MultiDeviceBuffer::SetDescriptor(const BufferDescriptor& descriptor)
    {
        m_descriptor = descriptor;
    }

    void MultiDeviceBuffer::Invalidate()
    {
        m_deviceObjects.clear();
    }

    const RHI::BufferDescriptor& MultiDeviceBuffer::GetDescriptor() const
    {
        return m_descriptor;
    }

    const BufferFrameAttachment* MultiDeviceBuffer::GetFrameAttachment() const
    {
        return static_cast<const BufferFrameAttachment*>(MultiDeviceResource::GetFrameAttachment());
    }

    Ptr<MultiDeviceBufferView> MultiDeviceBuffer::BuildBufferView(const BufferViewDescriptor& bufferViewDescriptor)
    {
        return aznew MultiDeviceBufferView{ this, bufferViewDescriptor };
    }

    const HashValue64 MultiDeviceBuffer::GetHash() const
    {
        HashValue64 hash = HashValue64{ 0 };
        hash = m_descriptor.GetHash();
        return hash;
    }

    void MultiDeviceBuffer::Shutdown()
    {
        IterateObjects<SingleDeviceBuffer>([]([[maybe_unused]] auto deviceIndex, auto deviceBuffer)
        {
            deviceBuffer->Shutdown();
        });

        MultiDeviceResource::Shutdown();
    }

    bool MultiDeviceBuffer::IsInResourceCache(const BufferViewDescriptor& bufferViewDescriptor)
    {
        bool isInResourceCache{true};
        IterateObjects<SingleDeviceBuffer>([&isInResourceCache, &bufferViewDescriptor]([[maybe_unused]] auto deviceIndex, auto deviceBuffer)
        {
            isInResourceCache &= deviceBuffer->IsInResourceCache(bufferViewDescriptor);
        });
        return isInResourceCache;
    }

    //! Given a device index, return the corresponding SingleDeviceBufferView for the selected device
    const RHI::Ptr<RHI::SingleDeviceBufferView> MultiDeviceBufferView::GetDeviceBufferView(int deviceIndex) const
    {
        AZStd::lock_guard lock(m_bufferViewMutex);
        auto iterator{ m_cache.find(deviceIndex) };
        if (iterator == m_cache.end())
        {
            //! Buffer view is not yet in the cache
            auto [new_iterator, inserted]{ m_cache.insert(
                AZStd::make_pair(deviceIndex, m_buffer->GetDeviceBuffer(deviceIndex)->GetBufferView(m_descriptor))) };
            if (inserted)
            {
                return new_iterator->second;
            }
        }

        return iterator->second;
    }
} // namespace AZ::RHI
