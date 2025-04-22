/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/DeviceStreamBufferView.h>
#include <Atom/RHI.Reflect/InputStreamLayout.h>

namespace AZ::RHI
{
    DeviceStreamBufferView::DeviceStreamBufferView(
        const DeviceBuffer& buffer,
        uint32_t byteOffset,
        uint32_t byteCount,
        uint32_t byteStride)
        : m_buffer{&buffer}
        , m_byteOffset{byteOffset}
        , m_byteCount{byteCount}
        , m_byteStride{byteStride}
    {
        size_t seed = 0;
        AZStd::hash_combine(seed, m_buffer);
        AZStd::hash_combine(seed, m_byteOffset);
        AZStd::hash_combine(seed, m_byteCount);
        AZStd::hash_combine(seed, m_byteStride);
        m_hash = static_cast<HashValue64>(seed);
    }

    HashValue64 DeviceStreamBufferView::GetHash() const
    {
        return m_hash;
    }

    const DeviceBuffer* DeviceStreamBufferView::GetBuffer() const
    {
        return m_buffer;
    }

    uint32_t DeviceStreamBufferView::GetByteOffset() const
    {
        return m_byteOffset;
    }

    uint32_t DeviceStreamBufferView::GetByteCount() const
    {
        return m_byteCount;
    }

    uint32_t DeviceStreamBufferView::GetByteStride() const
    {
        return m_byteStride;
    }

    bool DeviceStreamBufferView::operator==(const DeviceStreamBufferView& other) const
    {
        return (m_hash == other.m_hash) &&
            (m_buffer == other.m_buffer) &&
            (m_byteOffset == other.m_byteOffset) &&
            (m_byteCount == other.m_byteCount) &&
            (m_byteStride == other.m_byteStride);
    }


    bool ValidateStreamBufferViews(const RHI::InputStreamLayout& inputStreamLayout, AZStd::span<const RHI::DeviceStreamBufferView> streamBufferViews)
    {
        bool ok = true;

        if (Validation::IsEnabled())
        {
            if (!inputStreamLayout.IsFinalized())
            {
                AZ_Error("InputStreamLayout", false, "InputStreamLayout is not finalized.");
                ok = false;
            }

            if (inputStreamLayout.GetStreamBuffers().size() != streamBufferViews.size())
            {
                AZ_Error("InputStreamLayout", false, "InputStreamLayout references %d stream buffers but %d StreamBufferViews were provided.",
                    inputStreamLayout.GetStreamBuffers().size(), streamBufferViews.size());
                ok = false;
            }

            for (int i = 0; i < inputStreamLayout.GetStreamBuffers().size() && i < streamBufferViews.size(); ++i)
            {
                auto bufferDescriptors = inputStreamLayout.GetStreamBuffers();
                auto& bufferDescriptor = bufferDescriptors[i];
                auto& bufferView = streamBufferViews[i];

                // It can be valid to have a null buffer if this stream is not actually used by the shader, which can be the case for streams marked optional.
                if (bufferView.GetBuffer() == nullptr)
                {
                    continue;
                }

                if (bufferDescriptor.m_byteStride != bufferView.GetByteStride())
                {
                    AZ_Error("InputStreamLayout", false, "InputStreamLayout's buffer[%d] has stride=%d but DeviceStreamBufferView[%d] has stride=%d.",
                        i, bufferDescriptor.m_byteStride, i, bufferView.GetByteStride());
                    ok = false;
                }
            }
        }

        return ok;
    }
}
