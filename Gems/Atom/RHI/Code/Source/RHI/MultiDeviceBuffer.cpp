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
        //? TODO: We currently need to cache the BufferViews (as no one else is doing that atm),
        //?       can possibly be removed once everything handles MultiDeviceResources everywhere
        AZStd::unordered_map<int, Ptr<RHI::BufferView>> m_cache;
        IterateObjects<Buffer>([&bufferViewDescriptor, &m_cache](auto deviceIndex, auto deviceBuffer)
        {
            m_cache[deviceIndex] = deviceBuffer->GetBufferView(bufferViewDescriptor);
        });
        return aznew MultiDeviceBufferView{ this, bufferViewDescriptor, AZStd::move(m_cache) };
    }

    const HashValue64 MultiDeviceBuffer::GetHash() const
    {
        HashValue64 hash = HashValue64{ 0 };
        hash = m_descriptor.GetHash();
        return hash;
    }

    void MultiDeviceBuffer::Shutdown()
    {
        IterateObjects<Buffer>([]([[maybe_unused]] auto deviceIndex, auto deviceBuffer)
        {
            deviceBuffer->Shutdown();
        });

        MultiDeviceResource::Shutdown();
    }

    void MultiDeviceBuffer::InvalidateViews()
    {
        IterateObjects<Buffer>([]([[maybe_unused]] auto deviceIndex, auto deviceBuffer)
        {
            deviceBuffer->InvalidateViews();
        });
    }

    //! Given a device index, return the corresponding BufferView for the selected device
    const RHI::Ptr<RHI::BufferView> MultiDeviceBufferView::GetDeviceBufferView(int deviceIndex) const
    {
        AZ_Error(
            "MultiDeviceBufferView",
            m_cache.find(deviceIndex) != m_cache.end(),
            "No BufferView found for device index %d\n",
            deviceIndex);

        return m_cache.at(deviceIndex);
    }
} // namespace AZ::RHI
