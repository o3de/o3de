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
            auto instanceId = ShaderResourceGroup::MakeInstanceId(shaderAsset, supervariantIndex, srgName);
            auto srgPool = Data::InstanceDatabase<ShaderResourceGroupPool, false>::Instance().FindOrCreate(instanceId,
                shaderAsset);
            if (!srgPool->IsInitialized())
            {
                const RHI::ResultCode resultCode =
                    srgPool->Init(*shaderAsset.Get(), supervariantIndex, srgName);
                if (resultCode != RHI::ResultCode::Success)
                {
                    return nullptr;
                }
            }
            return srgPool;
        }

        Data::Instance<ShaderResourceGroupPool> ShaderResourceGroupPool::CreateInternal([[maybe_unused]] ShaderAsset& shaderAsset)
        {
            Data::Instance<ShaderResourceGroupPool> srgPool = aznew ShaderResourceGroupPool();
            return srgPool;
        }

        RHI::ResultCode ShaderResourceGroupPool::Init(
            ShaderAsset& shaderAsset, const SupervariantIndex& supervariantIndex, const AZ::Name& srgName)
        {
            RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();

            m_pool = RHI::Factory::Get().CreateShaderResourceGroupPool();
            if (!m_pool)
            {
                AZ_Error("ShaderResourceGroupPool", false, "Failed to create RHI::ShaderResourceGroupPool");
                return RHI::ResultCode::Fail;
            }

            RHI::ShaderResourceGroupPoolDescriptor poolDescriptor;
            poolDescriptor.m_layout = shaderAsset.FindShaderResourceGroupLayout(srgName, supervariantIndex).get();

            
            m_pool->SetName(srgName);
            const RHI::ResultCode resultCode = m_pool->Init(*device, poolDescriptor);
            m_isInitialized = (resultCode == RHI::ResultCode::Success);
            AZ_Error("ShaderResourceGroupPool", m_isInitialized, "Failed to initialize RHI::ShaderResourceGroupPool");

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
