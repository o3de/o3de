/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/BufferPoolBase.h>
#include <Atom/RHI/RHIUtils.h>

namespace AZ
{
    namespace RHI
    {
        ResultCode BufferPoolBase::InitBuffer(Buffer* buffer, const BufferDescriptor& descriptor, PlatformMethod platformInitResourceMethod)
        {
            // The descriptor is assigned regardless of whether initialization succeeds. Descriptors are considered
            // undefined for uninitialized resources. This makes the buffer descriptor well defined at initialization
            // time rather than leftover data from the previous usage.             
            buffer->SetDescriptor(descriptor);

            return ResourcePool::InitResource(buffer, platformInitResourceMethod);
        }

        void BufferPoolBase::ValidateBufferMap(Buffer& buffer, bool isDataValid)
        {
            if (Validation::IsEnabled())
            {
                // No need for validation for a null rhi
                if (IsNullRHI())
                {
                    return;
                }

                if (!isDataValid)
                {
                    AZ_Error("BufferPoolBase", false, "Failed to map buffer '%s'.", buffer.GetName().GetCStr());
                }
                ++buffer.m_mapRefCount;
                ++m_mapRefCount;
            }
        }

        bool BufferPoolBase::ValidateBufferUnmap(Buffer& buffer)
        {
            if (Validation::IsEnabled())
            {
                // No need for validation for a null rhi
                if (IsNullRHI())
                {
                    return true;
                }

                if (--buffer.m_mapRefCount == -1)
                {
                    AZ_Error("BufferPoolBase", false, "Buffer '%s' was unmapped more times than it was mapped.", buffer.GetName().GetCStr());

                    // Undo the ref-count to keep the validation state sane.
                    ++buffer.m_mapRefCount;
                    return false;
                }
                else
                {
                    --m_mapRefCount;
                }
            }
            return true;
        }

        uint32_t BufferPoolBase::GetMapRefCount() const
        {
            return m_mapRefCount;
        }

    }
}
