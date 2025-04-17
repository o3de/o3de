/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/IndexBufferView.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ::RHI
{
    IndexBufferView::IndexBufferView(
        const Buffer& buffer, uint32_t byteOffset, uint32_t byteCount, IndexFormat format)
        : m_buffer{ &buffer }
        , m_byteOffset{ byteOffset }
        , m_byteCount{ byteCount }
        , m_format{ format }
    {
        m_hash = TypeHash64(*this);
    }

    DeviceIndexBufferView IndexBufferView::GetDeviceIndexBufferView(int deviceIndex) const
    {
        AZ_Assert(m_buffer, "No Buffer available\n");

        return DeviceIndexBufferView(*m_buffer->GetDeviceBuffer(deviceIndex), m_byteOffset, m_byteCount, m_format);
    }

    AZ::HashValue64 IndexBufferView::GetHash() const
    {
        return m_hash;
    }

    const Buffer* IndexBufferView::GetBuffer() const
    {
        return m_buffer;
    }

    uint32_t IndexBufferView::GetByteOffset() const
    {
        return m_byteOffset;
    }

    uint32_t IndexBufferView::GetByteCount() const
    {
        return m_byteCount;
    }

    IndexFormat IndexBufferView::GetIndexFormat() const
    {
        return m_format;
    }
} // namespace AZ::RHI
