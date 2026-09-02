/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Mesh/MeshInstanceGroupKey.h>
#include <Atom/RPI.Public/Material/PersistentIndexAllocator.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/mutex.h>

namespace AZ::Render
{
    //! Assigns a dense [0, GetBatchCount()) batchId to each unique MeshInstanceGroupKey
    //! (mesh + LOD + material + sortKey). Reference-counted: the id is freed and reusable
    //! when the last instance referencing the key is released. Thread-safe.
    //!
    //! Independent of r_meshInstancingEnabled -- this is the GPU-driven batch identity,
    //! used to coalesce survivors into instanced indirect draws.
    class GpuBatchTable
    {
    public:
        //! Get-or-create the batchId for a key, incrementing its refcount.
        //! indexCount is the draw's vertexCountPerInstance for this batch (all instances share it).
        uint32_t Acquire(const MeshInstanceGroupKey& key, uint32_t indexCount);

        //! Decrement the key's refcount; free the batchId when it reaches zero.
        void Release(const MeshInstanceGroupKey& key);

        //! High-water batch count (== allocator MaxCount). Drives buffer sizing and the
        //! Finalize/scan dispatch domain. Freed-but-not-top ids remain as no-op holes.
        uint32_t GetBatchCount() const;

        //! Per-batch index count (vertexCountPerInstance), indexed by batchId. Grows monotonically.
        const AZStd::vector<uint32_t>& GetIndexCounts() const { return m_indexCount; }
        uint32_t GetIndexCount(uint32_t batchId) const;

        //! Read the batchId for a key WITHOUT changing its refcount. Returns false if the key
        //! has no live entry. Used to populate instance data on updates after the owning ref exists.
        bool TryGetBatchId(const MeshInstanceGroupKey& key, uint32_t& outBatchId) const;

        //! True if any Acquire/Release changed the table since the last MarkUploaded().
        bool NeedsUpload() const { return m_needsUpload; }
        void MarkUploaded() { m_needsUpload = false; }

    private:
        struct Entry
        {
            int32_t  m_batchId = -1;
            uint32_t m_refCount = 0;
        };

        mutable AZStd::mutex m_mutex;
        RPI::PersistentIndexAllocator<int32_t> m_indices;
        AZStd::unordered_map<MeshInstanceGroupKey, Entry> m_map;
        AZStd::vector<uint32_t> m_indexCount; // indexed by batchId
        bool m_needsUpload = false;
    };
} // namespace AZ::Render
