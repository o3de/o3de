/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/SingleDeviceBufferPool.h>

#include <AtomCore/Instance/InstanceData.h>

namespace AZ
{
    namespace RHI
    {
        class Device;
    }

    namespace RPI
    {
        class ResourcePoolAsset;

        class BufferPool final
            : public Data::InstanceData
        {
            friend class BufferSystem;

        public:
            AZ_INSTANCE_DATA(BufferPool, "{1EBB16AD-AC85-4560-BAC4-133D43826566}");
            AZ_CLASS_ALLOCATOR(BufferPool, AZ::SystemAllocator);

            /// Instantiates or returns an existing buffer pool using a paired resource pool asset.
            static Data::Instance<BufferPool> FindOrCreate(const Data::Asset<ResourcePoolAsset>& resourcePoolAsset);

            ~BufferPool() = default;

            RHI::SingleDeviceBufferPool* GetRHIPool();

            const RHI::SingleDeviceBufferPool* GetRHIPool() const;

        private:
            BufferPool() = default;

            // Standard asset creation path.
            static Data::Instance<BufferPool> CreateInternal(RHI::Device& device, ResourcePoolAsset& poolAsset);
            RHI::ResultCode Init(RHI::Device& device, ResourcePoolAsset& poolAsset);

            RHI::Ptr<RHI::SingleDeviceBufferPool> m_pool;
        };
    }
}
