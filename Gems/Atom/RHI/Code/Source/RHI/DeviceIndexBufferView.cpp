/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/DeviceIndexBufferView.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ::RHI
{
    DeviceIndexBufferView::DeviceIndexBufferView(
        const DeviceBuffer& buffer,
        uint32_t byteOffset,
        uint32_t byteCount,
        IndexFormat format)
        : m_buffer{&buffer}
        , m_byteOffset{byteOffset}
        , m_byteCount{byteCount}
        , m_format{format}
    {
        m_hash = TypeHash64(*this);
    }

    AZ::HashValue64 DeviceIndexBufferView::GetHash() const
    {
        return m_hash;
    }

    const DeviceBuffer* DeviceIndexBufferView::GetBuffer() const
    {
        return m_buffer;
    }

    uint32_t DeviceIndexBufferView::GetByteOffset() const
    {
        return m_byteOffset;
    }

    uint32_t DeviceIndexBufferView::GetByteCount() const
    {
        return m_byteCount;
    }

    IndexFormat DeviceIndexBufferView::GetIndexFormat() const
    {
        return m_format;
    }
}
