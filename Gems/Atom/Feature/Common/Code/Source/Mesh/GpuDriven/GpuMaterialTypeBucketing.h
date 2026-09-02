/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/vector.h>
#include <cstdint>

namespace AZ::Render
{
    //! A batch whose material type is this value is skipped (e.g. an unclaimed / non-GPU-driven batch).
    static constexpr uint32_t GpuDrivenInvalidMaterialType = 0xFFFFFFFFu;

    //! Per-material-type grouping of surviving batch ids. One ExecuteIndirect is issued per type
    //! (multi-indirect), so that each type binds its own GpuDrivenForward PSO while all share
    //! the bindless SceneMaterialSrg.
    struct GpuMaterialTypeBuckets
    {
        uint32_t m_typeCount = 0;
        //! Dense type slot -> the materialTypeId it represents.
        AZStd::vector<uint32_t> m_materialTypeIds;
        //! Dense type slot -> ordered batchIds belonging to that type.
        AZStd::vector<AZStd::vector<uint32_t>> m_batchIdsPerType;

        const AZStd::vector<uint32_t>& GetBatchIds(uint32_t typeSlot) const { return m_batchIdsPerType[typeSlot]; }
        uint32_t GetCount(uint32_t typeSlot) const { return static_cast<uint32_t>(m_batchIdsPerType[typeSlot].size()); }
        uint32_t GetTotalBatches() const;
    };

    //! Bucket batchIds (index = batchId, value = its materialTypeId) into per-type ordered lists.
    //! GpuDrivenInvalidMaterialType entries are skipped. Type slots are assigned in first-seen order
    //! (so the dispatch order is deterministic for a given input).
    GpuMaterialTypeBuckets BucketBatchesByMaterialType(const AZStd::vector<uint32_t>& batchMaterialType);
}
