/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/BufferFrameAttachment.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ
{
    namespace RHI
    {
        void Buffer::SetDescriptor(const BufferDescriptor& descriptor)
        {
            m_descriptor = descriptor;
        }

        void Buffer::Invalidate()
        {
            m_deviceBuffers.clear();
        }

        const RHI::BufferDescriptor& Buffer::GetDescriptor() const
        {
            return m_descriptor;
        }

        Ptr<BufferView> Buffer::GetBufferView(const BufferViewDescriptor& bufferViewDescriptor)
        {
            return Base::GetResourceView(bufferViewDescriptor);
        }

        const HashValue64 Buffer::GetHash() const
        {
            HashValue64 hash = HashValue64{ 0 };
            hash = m_descriptor.GetHash();
            return hash;
        }
    }
}
