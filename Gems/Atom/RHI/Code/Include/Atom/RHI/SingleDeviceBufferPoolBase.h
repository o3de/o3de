/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/SingleDeviceResourcePool.h>
#include <Atom/RHI/SingleDeviceBuffer.h>

namespace AZ::RHI
{
    //! A simple base class for buffer pools. This mainly exists so that various
    //! buffer pool implementations can have some type safety separate from other
    //! resource pool types.
    class SingleDeviceBufferPoolBase
        : public SingleDeviceResourcePool
    {
    public:
        AZ_RTTI(SingleDeviceBufferPoolBase, "{28D265BB-3B90-4676-BBA9-3F933F14CB01}", SingleDeviceResourcePool);
        virtual ~SingleDeviceBufferPoolBase() override = default;

    protected:
        SingleDeviceBufferPoolBase() = default;

        ResultCode InitBuffer(
            SingleDeviceBuffer* buffer,
            const BufferDescriptor& descriptor,
            PlatformMethod platformInitResourceMethod);

        /// Validates that the map operation succeeded by printing a warning otherwise. Increments
        /// the map reference counts for the buffer and the pool.
        void ValidateBufferMap(SingleDeviceBuffer& buffer, bool isDataValid);

        /// Validates that the buffer map reference count isn't negative. Decrements the global
        /// reference count.
        bool ValidateBufferUnmap(SingleDeviceBuffer& buffer);

        uint32_t GetMapRefCount() const;

    private:
        using SingleDeviceResourcePool::InitResource;

        /// Returns whether there are any mapped buffers.
        bool ValidateNoMappedBuffers() const;

        /// Debug reference count used to track map / unmap operations across all buffers in the pool.
        AZStd::atomic_uint m_mapRefCount = {0};
    };
}
