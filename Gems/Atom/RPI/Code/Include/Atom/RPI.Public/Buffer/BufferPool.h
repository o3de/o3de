/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Configuration.h>

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

        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_PUBLIC_API BufferPool final
            : public Data::InstanceData
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            friend class BufferSystem;

        public:
            AZ_INSTANCE_DATA(BufferPool, "{1EBB16AD-AC85-4560-BAC4-133D43826566}");
            AZ_CLASS_ALLOCATOR(BufferPool, AZ::SystemAllocator);

            /// Instantiates or returns an existing buffer pool using a paired resource pool asset.
            static Data::Instance<BufferPool> FindOrCreate(const Data::Asset<ResourcePoolAsset>& resourcePoolAsset);

            ~BufferPool() = default;

            RHI::BufferPool* GetRHIPool();

            const RHI::BufferPool* GetRHIPool() const;

        private:
            BufferPool() = default;

            // Standard asset creation path.
            static Data::Instance<BufferPool> CreateInternal(ResourcePoolAsset& poolAsset);
            RHI::ResultCode Init(ResourcePoolAsset& poolAsset);

            RHI::Ptr<RHI::BufferPool> m_pool;
        };
    }
}
