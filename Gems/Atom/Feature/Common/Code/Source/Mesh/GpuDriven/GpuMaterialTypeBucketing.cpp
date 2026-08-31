/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Mesh/GpuDriven/GpuMaterialTypeBucketing.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ::Render
{
    uint32_t GpuMaterialTypeBuckets::GetTotalBatches() const
    {
        uint32_t total = 0;
        for (const auto& list : m_batchIdsPerType)
        {
            total += static_cast<uint32_t>(list.size());
        }
        return total;
    }

    GpuMaterialTypeBuckets BucketBatchesByMaterialType(const AZStd::vector<uint32_t>& batchMaterialType)
    {
        GpuMaterialTypeBuckets buckets;
        AZStd::unordered_map<uint32_t, uint32_t> typeToSlot; // materialTypeId -> dense slot

        for (uint32_t batchId = 0; batchId < batchMaterialType.size(); ++batchId)
        {
            const uint32_t typeId = batchMaterialType[batchId];
            if (typeId == GpuDrivenInvalidMaterialType)
            {
                continue;
            }

            uint32_t slot;
            auto it = typeToSlot.find(typeId);
            if (it == typeToSlot.end())
            {
                slot = buckets.m_typeCount++;
                typeToSlot.emplace(typeId, slot);
                buckets.m_materialTypeIds.push_back(typeId);
                buckets.m_batchIdsPerType.emplace_back();
            }
            else
            {
                slot = it->second;
            }
            buckets.m_batchIdsPerType[slot].push_back(batchId);
        }
        return buckets;
    }
}
