/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/MultiDeviceBuffer.h>
#include <Atom/RHI/MultiDeviceIndexBufferView.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ::RHI
{
    MultiDeviceIndexBufferView::MultiDeviceIndexBufferView(
        const MultiDeviceBuffer& buffer, uint32_t byteOffset, uint32_t byteCount, IndexFormat format)
        : m_mdBuffer{ &buffer }
        , m_byteOffset{ byteOffset }
        , m_byteCount{ byteCount }
        , m_format{ format }
    {
        m_hash = TypeHash64(*this);
    }

    SingleDeviceIndexBufferView MultiDeviceIndexBufferView::GetDeviceIndexBufferView(int deviceIndex) const
    {
        AZ_Assert(m_mdBuffer, "No MultiDeviceBuffer available\n");

        return SingleDeviceIndexBufferView(*m_mdBuffer->GetDeviceBuffer(deviceIndex), m_byteOffset, m_byteCount, m_format);
    }

    AZ::HashValue64 MultiDeviceIndexBufferView::GetHash() const
    {
        return m_hash;
    }

    const MultiDeviceBuffer* MultiDeviceIndexBufferView::GetBuffer() const
    {
        return m_mdBuffer;
    }

    uint32_t MultiDeviceIndexBufferView::GetByteOffset() const
    {
        return m_byteOffset;
    }

    uint32_t MultiDeviceIndexBufferView::GetByteCount() const
    {
        return m_byteCount;
    }

    IndexFormat MultiDeviceIndexBufferView::GetIndexFormat() const
    {
        return m_format;
    }
} // namespace AZ::RHI
