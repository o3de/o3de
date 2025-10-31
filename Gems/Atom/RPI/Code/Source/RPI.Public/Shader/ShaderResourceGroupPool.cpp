/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Shader/ShaderResourceGroupPool.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>

#include <AtomCore/Instance/InstanceDatabase.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace RPI
    {
        Data::Instance<ShaderResourceGroupPool> ShaderResourceGroupPool::FindOrCreate(
            const Data::Asset<ShaderAsset>& shaderAsset, const SupervariantIndex& supervariantIndex, const AZ::Name& srgName)
        {
            auto instanceId = ShaderResourceGroup::MakeSrgPoolInstanceId(shaderAsset, supervariantIndex, srgName);
            ShaderResourceGroup::SrgInitParams srgInitParams{ supervariantIndex, srgName };
            auto anyArgInitParams = AZStd::any(srgInitParams);
            return Data::InstanceDatabase<ShaderResourceGroupPool>::Instance().FindOrCreate(instanceId,
                shaderAsset, &anyArgInitParams);
        }

        Data::Instance<ShaderResourceGroupPool> ShaderResourceGroupPool::CreateInternal(
            [[maybe_unused]] ShaderAsset& shaderAsset, const AZStd::any* anySrgInitParams)
        {
            AZ_Assert(anySrgInitParams, "Invalid SrgInitParams");
            auto srgInitParams = AZStd::any_cast<ShaderResourceGroup::SrgInitParams>(*anySrgInitParams);

            Data::Instance<ShaderResourceGroupPool> srgPool = aznew ShaderResourceGroupPool;
            const RHI::ResultCode resultCode = srgPool->Init(shaderAsset, srgInitParams.m_supervariantIndex, srgInitParams.m_srgName);
            if (resultCode != RHI::ResultCode::Success)
            {
                return nullptr;
            }

            return srgPool;
        }

        RHI::ResultCode ShaderResourceGroupPool::Init(
            ShaderAsset& shaderAsset, const SupervariantIndex& supervariantIndex, const AZ::Name& srgName)
        {
            m_pool = aznew RHI::ShaderResourceGroupPool;
            if (!m_pool)
            {
                AZ_Error("ShaderResourceGroupPool", false, "Failed to create RHI::ShaderResourceGroupPool");
                return RHI::ResultCode::Fail;
            }

            RHI::ShaderResourceGroupPoolDescriptor poolDescriptor;
            poolDescriptor.m_layout = shaderAsset.FindShaderResourceGroupLayout(srgName, supervariantIndex).get();

            m_pool->SetName(AZ::Name(AZStd::string::format("%s_%s",shaderAsset.GetName().GetCStr(),srgName.GetCStr())));

            const RHI::ResultCode resultCode = m_pool->Init(poolDescriptor);
            return resultCode;
        }

        RHI::Ptr<RHI::ShaderResourceGroup> ShaderResourceGroupPool::CreateRHIShaderResourceGroup()
        {
            RHI::Ptr<RHI::ShaderResourceGroup> srg = aznew RHI::ShaderResourceGroup();
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
