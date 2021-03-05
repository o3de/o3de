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


#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Reflect/ResourcePoolAsset.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/ImagePool.h>

#include <Atom/RHI.Reflect/ImagePoolDescriptor.h>

#include <AtomCore/Instance/InstanceDatabase.h>

namespace AZ
{
    namespace RPI
    {
        Data::Instance<AttachmentImagePool> AttachmentImagePool::FindOrCreate(const Data::Asset<ResourcePoolAsset>& resourcePoolAsset)
        {
            return Data::InstanceDatabase<AttachmentImagePool>::Instance().FindOrCreate(
                Data::InstanceId::CreateFromAssetId(resourcePoolAsset.GetId()),
                resourcePoolAsset);
        }

        Data::Instance<AttachmentImagePool> AttachmentImagePool::CreateInternal(RHI::Device& device, ResourcePoolAsset& poolAsset)
        {
            Data::Instance<AttachmentImagePool> imagePool = aznew AttachmentImagePool();
            RHI::ResultCode resultCode = imagePool->Init(device, poolAsset);

            if (resultCode == RHI::ResultCode::Success)
            {
                return imagePool;
            }

            return nullptr;
        }

        RHI::ResultCode AttachmentImagePool::Init(RHI::Device& device, ResourcePoolAsset& poolAsset)
        {
            RHI::Ptr<RHI::ImagePool> imagePool = RHI::Factory::Get().CreateImagePool();
            if (!imagePool)
            {
                AZ_Error("RPI::ImagePool", false, "Failed to create RHI::ImagePool");
                return RHI::ResultCode::Fail;
            }

            RHI::ImagePoolDescriptor *desc = azrtti_cast<RHI::ImagePoolDescriptor*>(poolAsset.GetPoolDescriptor().get());
            if (!desc)
            {
                AZ_Error("RPI::ImagePool", false, "The resource pool asset does not contain an image pool descriptor.");
                return RHI::ResultCode::Fail;
            }

            RHI::ResultCode resultCode = imagePool->Init(device, *desc);
            if (resultCode == RHI::ResultCode::Success)
            {
                m_pool = imagePool;
                m_pool->SetName(AZ::Name{ poolAsset.GetPoolName() });
            }
            return resultCode;
        }

        const RHI::ImagePool* AttachmentImagePool::GetRHIPool() const
        {
            return m_pool.get();
        }

        RHI::ImagePool* AttachmentImagePool::GetRHIPool()
        {
            return m_pool.get();
        }
    }
}
