/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Reflect/ResourcePoolAsset.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/MultiDeviceImagePool.h>

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

        Data::Instance<AttachmentImagePool> AttachmentImagePool::CreateInternal(
            RHI::MultiDevice::DeviceMask deviceMask, ResourcePoolAsset& poolAsset)
        {
            Data::Instance<AttachmentImagePool> imagePool = aznew AttachmentImagePool();
            RHI::ResultCode resultCode = imagePool->Init(deviceMask, poolAsset);

            if (resultCode == RHI::ResultCode::Success)
            {
                return imagePool;
            }

            return nullptr;
        }

        RHI::ResultCode AttachmentImagePool::Init(RHI::MultiDevice::DeviceMask deviceMask, ResourcePoolAsset& poolAsset)
        {
            RHI::Ptr<RHI::MultiDeviceImagePool> imagePool = aznew RHI::MultiDeviceImagePool;
            if (!imagePool)
            {
                AZ_Error("RPI::ImagePool", false, "Failed to create RHI::MultiDeviceImagePool");
                return RHI::ResultCode::Fail;
            }

            RHI::ImagePoolDescriptor *desc = azrtti_cast<RHI::ImagePoolDescriptor*>(poolAsset.GetPoolDescriptor().get());
            if (!desc)
            {
                AZ_Error("RPI::ImagePool", false, "The resource pool asset does not contain an image pool descriptor.");
                return RHI::ResultCode::Fail;
            }

            imagePool->SetName(AZ::Name(poolAsset.GetPoolName()));
            RHI::ResultCode resultCode = imagePool->Init(deviceMask, *desc);
            if (resultCode == RHI::ResultCode::Success)
            {
                m_pool = imagePool;
            }
            return resultCode;
        }

        const RHI::MultiDeviceImagePool* AttachmentImagePool::GetRHIPool() const
        {
            return m_pool.get();
        }

        RHI::MultiDeviceImagePool* AttachmentImagePool::GetRHIPool()
        {
            return m_pool.get();
        }
    }
}
