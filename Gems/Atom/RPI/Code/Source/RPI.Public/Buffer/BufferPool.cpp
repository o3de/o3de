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


#include <Atom/RPI.Public/Buffer/BufferPool.h>
#include <Atom/RPI.Reflect/ResourcePoolAsset.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/BufferPool.h>

#include <Atom/RHI.Reflect/BufferPoolDescriptor.h>

#include <AtomCore/Instance/InstanceDatabase.h>

namespace AZ
{
    namespace RPI
    {
        Data::Instance<BufferPool> BufferPool::FindOrCreate(const Data::Asset<ResourcePoolAsset>& resourcePoolAsset)
        {
            return Data::InstanceDatabase<BufferPool>::Instance().FindOrCreate(
                Data::InstanceId::CreateFromAssetId(resourcePoolAsset.GetId()),
                resourcePoolAsset);
        }

        Data::Instance<BufferPool> BufferPool::CreateInternal(RHI::Device& device, ResourcePoolAsset& poolAsset)
        {
            Data::Instance<BufferPool> bufferPool = aznew BufferPool();
            RHI::ResultCode resultCode = bufferPool->Init(device, poolAsset);
            if (resultCode == RHI::ResultCode::Success)
            {
                return bufferPool;
            }

            return nullptr;
        }

        RHI::ResultCode BufferPool::Init(RHI::Device& device, ResourcePoolAsset& poolAsset)
        {
            RHI::Ptr<RHI::BufferPool> bufferPool = RHI::Factory::Get().CreateBufferPool();
            if (!bufferPool)
            {
                AZ_Error("RPI::BufferPool", false, "Failed to create RHI::BufferPool");
                return RHI::ResultCode::Fail;
            }

            RHI::BufferPoolDescriptor *desc = azrtti_cast<RHI::BufferPoolDescriptor*>(poolAsset.GetPoolDescriptor().get());
            if (!desc)
            {
                AZ_Error("RPI::BufferPool", false, "The resource pool asset does not contain a buffer pool descriptor.");
                return RHI::ResultCode::Fail;
            }

            RHI::ResultCode resultCode = bufferPool->Init(device, *desc);
            if (resultCode == RHI::ResultCode::Success)
            {
                bufferPool->SetName(AZ::Name{ poolAsset.GetPoolName() });
                m_pool = bufferPool;
            }

            return resultCode;
        }

        const RHI::BufferPool* BufferPool::GetRHIPool() const
        {
            return m_pool.get();
        }

        RHI::BufferPool* BufferPool::GetRHIPool()
        {
            return m_pool.get();
        }
    }
}
