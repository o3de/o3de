/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/BufferFrameAttachment.h>
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
        return aznew BufferView{ this, bufferViewDescriptor, GetDeviceMask() };
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

    bool Buffer::IsInResourceCache(const BufferViewDescriptor& bufferViewDescriptor)
    {
        bool isInResourceCache{true};
        IterateObjects<DeviceBuffer>([&isInResourceCache, &bufferViewDescriptor]([[maybe_unused]] auto deviceIndex, auto deviceBuffer)
        {
            isInResourceCache &= deviceBuffer->IsInResourceCache(bufferViewDescriptor);
        });
        return isInResourceCache;
    }

    //! Given a device index, return the corresponding DeviceBufferView for the selected device
    const RHI::Ptr<RHI::DeviceBufferView> BufferView::GetDeviceBufferView(int deviceIndex) const
    {
        AZStd::lock_guard lock(m_bufferViewMutex);

        if (m_buffer->GetDeviceMask() != m_deviceMask)
        {
            m_deviceMask = m_buffer->GetDeviceMask();

            MultiDeviceObject::IterateDevices(
                m_deviceMask,
                [this](int deviceIndex)
                {
                    if (auto it{ m_cache.find(deviceIndex) }; it != m_cache.end())
                    {
                        m_cache.erase(it);
                    }
                    return true;
                });
        }

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
        // TODO: Figure out why `iterator->second` is sometimes null given that `m_buffer` is non-null
        // Add null check for `iterator->second` to avoid empty pointer
        else if (!iterator->second || &iterator->second->GetBuffer() != m_buffer->GetDeviceBuffer(deviceIndex).get())
        {
            iterator->second = m_buffer->GetDeviceBuffer(deviceIndex)->GetBufferView(m_descriptor);
        }

        return iterator->second;
    }

    AZStd::unordered_map<int, uint32_t> BufferView::GetBindlessReadIndex() const
    {
        AZStd::unordered_map<int, uint32_t> result;

        MultiDeviceObject::IterateDevices(
            m_buffer->GetDeviceMask(),
            [this, &result](int deviceIndex)
            {
                result[deviceIndex] = GetDeviceBufferView(deviceIndex)->GetBindlessReadIndex();
                return true;
            });

        return result;
    }
} // namespace AZ::RHI
