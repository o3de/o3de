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
#include "Atom_RHI_Vulkan_precompiled.h"
#include <RHI/MergedShaderResourceGroup.h>
#include <RHI/MergedShaderResourceGroupPool.h>

namespace AZ
{
    namespace Vulkan
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

            MergedShaderResourceGroup* mergedSRG;
            {
                AZStd::shared_lock<AZStd::shared_mutex> lock(m_databaseMutex);
                mergedSRG = m_cacheDatabase.Find(key);
            }

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

                {
                    AZStd::unique_lock<AZStd::shared_mutex> lock(m_databaseMutex);
                    m_cacheDatabase.Insert(key, mergedSRG);
                }
            }

            return mergedSRG;
        }

        RHI::ResultCode MergedShaderResourceGroupPool::InitInternal(RHI::Device& deviceBase, const RHI::ShaderResourceGroupPoolDescriptor& descriptor)
        {
            m_cacheDatabase.SetCapacity(CacheDatabaseCapacity);
            return Base::InitInternal(deviceBase, descriptor);
        }

        void MergedShaderResourceGroupPool::ShutdownInternal()
        {
            {
                AZStd::unique_lock<AZStd::shared_mutex> lock(m_databaseMutex);
                m_cacheDatabase.Clear();
            }
            Base::ShutdownInternal();
        }
    }
}
