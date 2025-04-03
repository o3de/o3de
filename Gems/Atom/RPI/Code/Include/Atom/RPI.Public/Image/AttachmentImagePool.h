/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/Device.h>
#include <Atom/RPI.Public/Configuration.h>
#include <AtomCore/Instance/InstanceData.h>

namespace AZ
{
    namespace RHI
    {
        class Device;
        class ImagePool;
    }

    namespace RPI
    {
        class ResourcePoolAsset;

        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_PUBLIC_API AttachmentImagePool final
            : public Data::InstanceData
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            friend class ImageSystem;

        public:
            AZ_INSTANCE_DATA(AttachmentImagePool, "{18295AA3-1098-4346-8B31-71E3A2BE0AC7}");
            AZ_CLASS_ALLOCATOR(AttachmentImagePool, AZ::SystemAllocator);

            //! Instantiates or returns an existing attachment image pool using a paired resource pool asset.
            static Data::Instance<AttachmentImagePool> FindOrCreate(const Data::Asset<ResourcePoolAsset>& resourcePoolAsset);

            ~AttachmentImagePool() override;

            RHI::ImagePool* GetRHIPool();

            const RHI::ImagePool* GetRHIPool() const;

        private:
            AttachmentImagePool();

            // Standard asset creation path.
            static Data::Instance<AttachmentImagePool> CreateInternal(ResourcePoolAsset& poolAsset);
            RHI::ResultCode Init(ResourcePoolAsset& poolAsset);

            /// The RHI image pool instance.
            RHI::Ptr<RHI::ImagePool> m_pool;
        };
    }
}
