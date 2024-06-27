/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/BufferPoolBase.h>

namespace AZ::RHI
{
    ResultCode BufferPoolBase::InitBuffer(
        Buffer* buffer, const BufferDescriptor& descriptor, PlatformMethod platformInitResourceMethod)
    {
        // The descriptor is assigned regardless of whether initialization succeeds. Descriptors are considered
        // undefined for uninitialized resources. This makes the buffer descriptor well defined at initialization
        // time rather than leftover data from the previous usage.
        buffer->SetDescriptor(descriptor);

        return ResourcePool::InitResource(buffer, platformInitResourceMethod);
    }
} // namespace AZ::RHI
