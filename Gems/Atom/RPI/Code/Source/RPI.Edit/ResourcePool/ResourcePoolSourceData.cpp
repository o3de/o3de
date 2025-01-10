/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/ResourcePool/ResourcePoolSourceData.h>

namespace AZ::RPI
{
    void ResourcePoolSourceData::Reflect(ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<ResourcePoolSourceData>()
                ->Field("PoolName", &ResourcePoolSourceData::m_poolName)
                ->Field("PoolType", &ResourcePoolSourceData::m_poolType)
                ->Field("BudgetInBytes", &ResourcePoolSourceData::m_budgetInBytes)
                ->Field("BufferPoolHeapMemoryLevel", &ResourcePoolSourceData::m_heapMemoryLevel)
                ->Field("BufferPoolhostMemoryAccess", &ResourcePoolSourceData::m_hostMemoryAccess)
                ->Field("BufferPoolBindFlags", &ResourcePoolSourceData::m_bufferPoolBindFlags)
                ->Field("ImagePoolBindFlags", &ResourcePoolSourceData::m_imagePoolBindFlags)
                ;

            // register enum strings
            serializeContext->Enum<ResourcePoolAssetType>()
                ->Value("Unknown", ResourcePoolAssetType::Unknown)
                ->Value("BufferPool", ResourcePoolAssetType::BufferPool)
                ->Value("ImagePool", ResourcePoolAssetType::ImagePool)
                ->Value("StreamingImagePool", ResourcePoolAssetType::StreamingImagePool)
                ;
        }
    }
}
