/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/MergedShaderResourceGroup.h>
#include <RHI/MergedShaderResourceGroupPool.h>
#include <RHI/MergedShaderResourceGroupPoolDescriptor.h>

namespace AZ::WebGPU
{
    RHI::Ptr<MergedShaderResourceGroupPool> MergedShaderResourceGroupPool::Create()
    {
        return aznew MergedShaderResourceGroupPool();
    }

    MergedShaderResourceGroup* MergedShaderResourceGroupPool::FindOrCreate(const ShaderResourceGroupList& shaderResourceGroupList)
    {
        MergedShaderResourceGroup::ShaderResourceGroupArray key = {};
        for (const auto& srg : shaderResourceGroupList)
        {
            key[srg->GetBindingSlot()] = srg;
        }

        {
            AZStd::lock_guard<AZStd::mutex> lock(m_databaseMutex);
            MergedShaderResourceGroup* mergedSRG = m_cacheDatabase.Find(key);

            if (!mergedSRG)
            {
                mergedSRG = aznew MergedShaderResourceGroup();
                auto result = InitGroup(*mergedSRG);
                if (result != RHI::ResultCode::Success)
                {
                    AZ_Assert(false, "Failed to initialize Merged Shader Resource Group");
                    return nullptr;
                }
                mergedSRG->m_mergedShaderResourceGroupList = key;
                m_cacheDatabase.Insert(key, mergedSRG);
            }
            return mergedSRG;
        }
    }

    RHI::ResultCode MergedShaderResourceGroupPool::InitInternal([[maybe_unused]] RHI::Device& deviceBase, const RHI::ShaderResourceGroupPoolDescriptor& descriptor)
    {
        const MergedShaderResourceGroupPoolDescriptor& mergedDescriptor =
            static_cast<const MergedShaderResourceGroupPoolDescriptor&>(descriptor);
        m_cacheDatabase.SetCapacity(CacheDatabaseCapacity);
        m_bindGroupCount = RHI::Limits::Device::FrameCountMax;
        m_bindGroupLayout = mergedDescriptor.m_bindGroupLayout;
        m_bindGroupLayout->SetName(GetName());
        return RHI::ResultCode::Success;
    }

    void MergedShaderResourceGroupPool::ShutdownInternal()
    {
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_databaseMutex);
            m_cacheDatabase.Clear();
        }
        Base::ShutdownInternal();
    }
}
