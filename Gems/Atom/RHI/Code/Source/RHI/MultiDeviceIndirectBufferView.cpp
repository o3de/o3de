/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/MultiDeviceBuffer.h>
#include <Atom/RHI/MultiDeviceIndirectBufferView.h>
#include <AzCore/std/hash.h>

namespace AZ::RHI
{
    MultiDeviceIndirectBufferView::MultiDeviceIndirectBufferView(
        const MultiDeviceBuffer& buffer,
        const MultiDeviceIndirectBufferSignature& signature,
        uint32_t byteOffset,
        uint32_t byteCount,
        uint32_t byteStride)
        : m_mdBuffer(&buffer)
        , m_byteOffset(byteOffset)
        , m_byteCount(byteCount)
        , m_byteStride(byteStride)
        , m_mdSignature(&signature)
    {
        size_t seed = 0;
        AZStd::hash_combine(seed, m_mdBuffer);
        AZStd::hash_combine(seed, m_byteOffset);
        AZStd::hash_combine(seed, m_byteCount);
        AZStd::hash_combine(seed, m_byteStride);
        AZStd::hash_combine(seed, m_mdSignature);
        m_hash = static_cast<HashValue64>(seed);
    }

    HashValue64 MultiDeviceIndirectBufferView::GetHash() const
    {
        return m_hash;
    }

    const MultiDeviceBuffer* MultiDeviceIndirectBufferView::GetBuffer() const
    {
        return m_mdBuffer;
    }

    uint32_t MultiDeviceIndirectBufferView::GetByteOffset() const
    {
        return m_byteOffset;
    }

    uint32_t MultiDeviceIndirectBufferView::GetByteCount() const
    {
        return m_byteCount;
    }

    uint32_t MultiDeviceIndirectBufferView::GetByteStride() const
    {
        return m_byteStride;
    }

    const MultiDeviceIndirectBufferSignature* MultiDeviceIndirectBufferView::GetSignature() const
    {
        return m_mdSignature;
    }
} // namespace AZ::RHI
