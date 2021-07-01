/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Shader/ShaderResourceGroupPool.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>

#include <AtomCore/Instance/InstanceDatabase.h>

namespace AZ
{
    namespace RPI
    {
        Data::Instance<ShaderResourceGroupPool> ShaderResourceGroupPool::FindOrCreate(const Data::Asset<ShaderResourceGroupAsset>& srgAsset)
        {
            return Data::InstanceDatabase<ShaderResourceGroupPool>::Instance().FindOrCreate(
                Data::InstanceId::CreateFromAssetId(srgAsset.GetId()),
                srgAsset);
        }

        Data::Instance<ShaderResourceGroupPool> ShaderResourceGroupPool::CreateInternal(ShaderResourceGroupAsset& srgAsset)
        {
            Data::Instance<ShaderResourceGroupPool> srgPool = aznew ShaderResourceGroupPool();
            const RHI::ResultCode resultCode = srgPool->Init(srgAsset);

            if (resultCode == RHI::ResultCode::Success)
            {
                return srgPool;
            }

            return nullptr;
        }

        RHI::ResultCode ShaderResourceGroupPool::Init(ShaderResourceGroupAsset& srgAsset)
        {
            RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();

            m_pool = RHI::Factory::Get().CreateShaderResourceGroupPool();
            if (!m_pool)
            {
                AZ_Error("ShaderResourceGroupPool", false, "Failed to create RHI::ShaderResourceGroupPool");
                return RHI::ResultCode::Fail;
            }

            RHI::ShaderResourceGroupPoolDescriptor poolDescriptor;
            poolDescriptor.m_layout = srgAsset.GetLayout();

            
            m_pool->SetName(Name(srgAsset.GetName()));
            const RHI::ResultCode resultCode = m_pool->Init(*device, poolDescriptor);

            AZ_Error("ShaderResourceGroupPool", resultCode == RHI::ResultCode::Success, "Failed to initialize RHI::ShaderResourceGroupPool");

            return resultCode;
        }

        RHI::Ptr<RHI::ShaderResourceGroup> ShaderResourceGroupPool::CreateRHIShaderResourceGroup()
        {
            RHI::Ptr<RHI::ShaderResourceGroup> srg = RHI::Factory::Get().CreateShaderResourceGroup();
            AZ_Error("ShaderResourceGroupPool", srg, "Failed to create RHI::ShaderResourceGroup");

            if (srg)
            {
                RHI::ResultCode result = m_pool->InitGroup(*srg);
                if (result != RHI::ResultCode::Success)
                {
                    srg.reset();
                    AZ_Error("ShaderResourceGroupPool", false, "Failed to initialize RHI::ShaderResourceGroup");
                }
            }

            return srg;
        }

        RHI::ShaderResourceGroupPool* ShaderResourceGroupPool::GetRHIPool()
        {
            return m_pool.get();
        }

        const RHI::ShaderResourceGroupPool* ShaderResourceGroupPool::GetRHIPool() const
        {
            return m_pool.get();
        }
    }
}
