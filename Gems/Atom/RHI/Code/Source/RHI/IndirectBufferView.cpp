/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <Atom/RHI/IndirectBufferView.h>
#include <AzCore/std/hash.h>

namespace AZ
{
    namespace RHI
    {
        IndirectBufferView::IndirectBufferView(
            const Buffer& buffer,
            const IndirectBufferSignature& signature,
            uint32_t byteOffset,
            uint32_t byteCount,
            uint32_t byteStride)
            : m_buffer(&buffer)
            , m_byteOffset(byteOffset)
            , m_byteCount(byteCount)
            , m_byteStride(byteStride)
            , m_signature(&signature)
        {
            size_t seed = 0;
            AZStd::hash_combine(seed, m_buffer);
            AZStd::hash_combine(seed, m_byteOffset);
            AZStd::hash_combine(seed, m_byteCount);
            AZStd::hash_combine(seed, m_byteStride);
            AZStd::hash_combine(seed, m_signature);
            m_hash = static_cast<HashValue64>(seed);
        }

        HashValue64 IndirectBufferView::GetHash() const
        {
            return m_hash;
        }

        const Buffer* IndirectBufferView::GetBuffer() const
        {
            return m_buffer;
        }

        uint32_t IndirectBufferView::GetByteOffset() const
        {
            return m_byteOffset;
        }

        uint32_t IndirectBufferView::GetByteCount() const
        {
            return m_byteCount;
        }

        uint32_t IndirectBufferView::GetByteStride() const
        {
            return m_byteStride;
        }

        const IndirectBufferSignature* IndirectBufferView::GetSignature() const
        {
            return m_signature;
        }
    }
}
