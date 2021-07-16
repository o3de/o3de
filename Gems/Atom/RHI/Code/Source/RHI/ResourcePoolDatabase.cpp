/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/ResourcePoolDatabase.h>
#include <Atom/RHI/BufferPoolBase.h>
#include <Atom/RHI/ImagePoolBase.h>
#include <Atom/RHI/ShaderResourceGroupPool.h>

#include <AzCore/std/algorithm.h>

namespace AZ
{
    namespace RHI
    {
        ResourcePoolDatabase::~ResourcePoolDatabase()
        {
            AZ_Assert(m_pools.empty(), "Pool container is not empty!");
            AZ_Assert(m_poolResolvers.empty(), "Pool resolver container is not empty!");
            AZ_Assert(m_bufferPools.empty(), "Buffer pool container is not empty!");
            AZ_Assert(m_imagePools.empty(), "Image pool container is not empty!");
            AZ_Assert(m_shaderResourceGroupPools.empty(), "ShaderResourceGroup pool container is not empty!");
        }

        void ResourcePoolDatabase::AttachPool(ResourcePool* resourcePool)
        {
            AZStd::unique_lock<AZStd::shared_mutex> lock(m_mutex);

            // Search for the set of core pools. Those get stored in their own set separate from the union set.

            if (BufferPoolBase* bufferPool = azrtti_cast<BufferPoolBase*>(resourcePool))
            {
                m_bufferPools.emplace_back(bufferPool);
            }
            else if (ImagePoolBase* imagePool = azrtti_cast<ImagePoolBase*>(resourcePool))
            {
                m_imagePools.emplace_back(imagePool);
            }
            else if (ShaderResourceGroupPool* srgPool = azrtti_cast<ShaderResourceGroupPool*>(resourcePool))
            {
                m_shaderResourceGroupPools.emplace_back(srgPool);
            }

            // Other pool types may exist. All pools go into the union set.

            m_pools.emplace_back(resourcePool);

            if (ResourcePoolResolver* poolResolver = resourcePool->GetResolver())
            {
                m_poolResolvers.emplace_back(poolResolver);
            }
        }

        void ResourcePoolDatabase::DetachPool(ResourcePool* resourcePool)
        {
            AZStd::unique_lock<AZStd::shared_mutex> lock(m_mutex);

            // Search for the set of core pools. Those get stored in their own set separate from the union set.
            if (BufferPoolBase* bufferPool = azrtti_cast<BufferPoolBase*>(resourcePool))
            {
                auto it = AZStd::find(m_bufferPools.begin(), m_bufferPools.end(), bufferPool);
                AZ_Assert(it != m_bufferPools.end(), "Buffer pool does not exist in database.");
                m_bufferPools.erase(it);
            }
            else if (ImagePoolBase* imagePool = azrtti_cast<ImagePoolBase*>(resourcePool))
            {
                auto it = AZStd::find(m_imagePools.begin(), m_imagePools.end(), imagePool);
                AZ_Assert(it != m_imagePools.end(), "Image pool does not exist in database.");
                m_imagePools.erase(it);
            }
            else if (ShaderResourceGroupPool* srgPool = azrtti_cast<ShaderResourceGroupPool*>(resourcePool))
            {
                auto it = AZStd::find(m_shaderResourceGroupPools.begin(), m_shaderResourceGroupPools.end(), srgPool);
                AZ_Assert(it != m_shaderResourceGroupPools.end(), "ShaderResourceGroup pool does not exist in database.");
                m_shaderResourceGroupPools.erase(it);
            }

            // Other pool types may exist. All pools go into the union set.

            {
                auto it = AZStd::find(m_pools.begin(), m_pools.end(), resourcePool);
                AZ_Assert(it != m_pools.end(), "Pool does not exist in database.");
                m_pools.erase(it);
            }

            if (ResourcePoolResolver* poolResolver = resourcePool->GetResolver())
            {
                auto it = AZStd::find(m_poolResolvers.begin(), m_poolResolvers.end(), poolResolver);
                AZ_Assert(it != m_poolResolvers.end(), "Pool resolver does not exist in database.");
                m_poolResolvers.erase(it);
            }
        }
    }
}
