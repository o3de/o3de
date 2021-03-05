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

#include <Atom/RHI/Device.h>

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

        class AttachmentImagePool final
            : public Data::InstanceData
        {
            friend class ImageSystem;

        public:
            AZ_INSTANCE_DATA(AttachmentImagePool, "{18295AA3-1098-4346-8B31-71E3A2BE0AC7}");
            AZ_CLASS_ALLOCATOR(AttachmentImagePool, AZ::SystemAllocator, 0);

            //! Instantiates or returns an existing attachment image pool using a paired resource pool asset.
            static Data::Instance<AttachmentImagePool> FindOrCreate(const Data::Asset<ResourcePoolAsset>& resourcePoolAsset);

            ~AttachmentImagePool() override = default;

            RHI::ImagePool* GetRHIPool();

            const RHI::ImagePool* GetRHIPool() const;

        private:
            AttachmentImagePool() = default;

            // Standard asset creation path.
            static Data::Instance<AttachmentImagePool> CreateInternal(RHI::Device& device, ResourcePoolAsset& poolAsset);
            RHI::ResultCode Init(RHI::Device& device, ResourcePoolAsset& poolAsset);

            /// The RHI image pool instance.
            RHI::Ptr<RHI::ImagePool> m_pool;
        };
    }
}
