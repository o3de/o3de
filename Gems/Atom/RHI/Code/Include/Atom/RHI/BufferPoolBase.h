/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/ResourcePool.h>
#include <Atom/RHI/Buffer.h>

namespace AZ
{
    namespace RHI
    {
        /**
         * A simple base class for buffer pools. This mainly exists so that various
         * buffer pool implementations can have some type safety separate from other
         * resource pool types.
         */
        class BufferPoolBase
            : public ResourcePool
        {
        public:
            AZ_RTTI(BufferPoolBase, "{28D265BB-3B90-4676-BBA9-3F933F14CB01}", ResourcePool);
            virtual ~BufferPoolBase() override = default;

        protected:
            BufferPoolBase() = default;

            ResultCode InitBuffer(
                Buffer* buffer,
                const BufferDescriptor& descriptor,
                PlatformMethod platformInitResourceMethod);

            /// Validates that the map operation succeeded by printing a warning otherwise. Increments
            /// the map reference counts for the buffer and the pool.
            void ValidateBufferMap(Buffer& buffer, bool isDataValid);

            /// Validates that the buffer map reference count isn't negative. Decrements the global
            /// reference count.
            bool ValidateBufferUnmap(Buffer& buffer);

            uint32_t GetMapRefCount() const;

        private:
            using ResourcePool::InitResource;

            /// Returns whether there are any mapped buffers.
            bool ValidateNoMappedBuffers() const;

            /// Debug reference count used to track map / unmap operations across all buffers in the pool.
            AZStd::atomic_uint m_mapRefCount = {0};
        };
    }
}
