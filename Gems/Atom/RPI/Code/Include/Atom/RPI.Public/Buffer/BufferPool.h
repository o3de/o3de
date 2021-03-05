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

#pragma once

#include <Atom/RHI/BufferPool.h>

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
            AZ_CLASS_ALLOCATOR(BufferPool, AZ::SystemAllocator, 0);

            /// Instantiates or returns an existing buffer pool using a paired resource pool asset.
            static Data::Instance<BufferPool> FindOrCreate(const Data::Asset<ResourcePoolAsset>& resourcePoolAsset);

            ~BufferPool() = default;

            RHI::BufferPool* GetRHIPool();

            const RHI::BufferPool* GetRHIPool() const;

        private:
            BufferPool() = default;

            // Standard asset creation path.
            static Data::Instance<BufferPool> CreateInternal(RHI::Device& device, ResourcePoolAsset& poolAsset);
            RHI::ResultCode Init(RHI::Device& device, ResourcePoolAsset& poolAsset);

            RHI::Ptr<RHI::BufferPool> m_pool;
        };
    }
}
