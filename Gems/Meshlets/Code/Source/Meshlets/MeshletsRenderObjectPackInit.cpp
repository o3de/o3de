/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <MeshletsRenderObject.h>
#include <MeshletsUtilities.h>
#include <Meshlets/Reflect/MeshletPackFormat.h>
#include <AzCore/Console/IConsole.h>
#include <cstring>

namespace AZ::Meshlets
{
    // Phase 4 VRAM-reclaim switch: for v4 (paged) meshes, skip creating every
    // monolithic geometry buffer -- the mesh renders exclusively through the AS-cull
    // paged path (requires r_meshletsHwMeshShader + r_meshletsMsCullAS +
    // r_meshletsDagLod + r_meshletsStreaming at runtime; with any of them off such
    // meshes render nothing, by design). Load-time decision: flip + reload the level.
    AZ_CVAR(bool, r_meshletsStreamingExclusive, false, nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "If true at pack load, paged (v4) meshes skip their monolithic geometry "
        "buffers entirely -- the actual VRAM reclaim. They then render ONLY via the "
        "AS-cull + DAG + streaming path; every other meshlet path self-skips.");

    MeshletsRenderObject::MeshletsRenderObject(
        Data::Asset<RPI::ModelAsset> sourceModelAsset,
        Data::Asset<MeshletPackAsset> packAsset,
        MeshletsFeatureProcessor* meshletsFeatureProcessor)
    {
        // Initialize the same fields as the old constructor where they don't
        // depend on the pack. (Replicate the boilerplate so the old constructor
        // can be deleted cleanly in Phase 5 without touching this one.)
        s_textureCoordinatesName = Name{ "UV" };
        s_indicesName            = Name{ "INDICES" };
        m_featureProcessor       = meshletsFeatureProcessor;
        m_name = "Model_" + AZStd::to_string(s_modelNumber++);
        m_aabb = Aabb::CreateNull();

        if (!SetShaders())
        {
            AZ_Error("Meshlets", false, "MeshletsRenderObject: SetShaders failed");
            return;
        }

        AZ_TracePrintf("Meshlets",
            "MeshletsRenderObject: constructing for model=%s pack=%s\n",
            sourceModelAsset.GetId().ToString<AZStd::string>().c_str(),
            packAsset.GetId().ToString<AZStd::string>().c_str());

        if (!packAsset.IsReady())
        {
            AZ_Error("Meshlets", false,
                "MeshletsRenderObject: packAsset is not ready (id=%s)",
                packAsset.GetId().ToString<AZStd::string>().c_str());
            return;
        }

        const PackHeaderRecord* hdr = packAsset->GetPackHeader();
        if (!hdr)
        {
            AZ_Error("Meshlets", false, "MeshletsRenderObject: pack missing PackHeader section");
            return;
        }

        // Reconstruct AssetId from raw GUID + sub-id (Task 2 spec correction).
        AZ::Uuid headerGuid;
        std::memcpy(&headerGuid, hdr->m_sourceModelGuid, sizeof(hdr->m_sourceModelGuid));
        const AZ::Data::AssetId headerModelId(headerGuid, hdr->m_sourceModelSubId);
        if (headerModelId != sourceModelAsset.GetId())
        {
            AZ_Error("Meshlets", false,
                "MeshletsRenderObject: pack source model id mismatch "
                "(pack=%s, requested=%s)",
                headerModelId.ToString<AZStd::string>().c_str(),
                sourceModelAsset.GetId().ToString<AZStd::string>().c_str());
            return;
        }

        // Iterate the pack's MeshDescriptors section and build one
        // MeshRenderData per mesh x LOD.
        const auto& reader = packAsset->GetReader();
        auto meshDescBytes  = reader.GetSection(SectionKind::MeshDescriptors);
        auto clusterBytes   = reader.GetSection(SectionKind::ClusterDescriptors);
        auto triBytes       = reader.GetSection(SectionKind::TriangleIndices);
        auto indirBytes     = reader.GetSection(SectionKind::VertexIndirection);
        auto vsBytes        = reader.GetSection(SectionKind::VertexStreams);
        // SP1 v2: pre-baked flat triangle index list (3 u32 per triangle for
        // all clusters of all meshes in pack-global order). Per-mesh slice
        // start = 3 * triBase (where triBase = first cluster's pack-global
        // triangleOffset). Length = 3 * sum(cluster.triangleCount). Use this
        // directly as the m_indices SRV's data -- no runtime CPU expansion or
        // compute pass needed.
        auto expandedIdxBytes = reader.GetSection(SectionKind::ExpandedIndices);
        // Phase 6: per-cluster bounds (sphere + normal cone) for cluster culling.
        // Optional -- packs built before builder v6 lack it; culling then draws every cluster.
        auto coneBoundsBytes = reader.GetSection(SectionKind::ConeBounds);
        const ClusterBoundsRecord* clusterBoundsAll = coneBoundsBytes.empty()
            ? nullptr : reinterpret_cast<const ClusterBoundsRecord*>(coneBoundsBytes.data());

        // Phase 6 cluster DAG (pack v3): per-cluster cut records, parallel to
        // ClusterDescriptors like ConeBounds. Optional -- v2 packs lack it and the
        // runtime keeps the discrete-LOD behavior.
        auto dagNodesBytes = reader.GetSection(SectionKind::DagNodes);
        const DagNodeRecord* dagNodesAll = dagNodesBytes.empty()
            ? nullptr : reinterpret_cast<const DagNodeRecord*>(dagNodesBytes.data());

        // Phase 7 streaming (pack v4): leaf pages + exact group->children mapping.
        // Optional -- v2/v3 packs lack them and streaming simply stays unavailable.
        auto pageTableBytes = reader.GetSection(SectionKind::PageTable);
        const PageTableRecord* pageTableAll = pageTableBytes.empty()
            ? nullptr : reinterpret_cast<const PageTableRecord*>(pageTableBytes.data());
        const size_t pageTableCount = pageTableBytes.size() / sizeof(PageTableRecord);
        auto pageDataBytes = reader.GetSection(SectionKind::PageData);
        auto parentIndexBytes = reader.GetSection(SectionKind::ParentIndex);
        const AZ::u32* parentIndexAll = parentIndexBytes.empty()
            ? nullptr : reinterpret_cast<const AZ::u32*>(parentIndexBytes.data());
        const size_t parentIndexCount = parentIndexBytes.size() / sizeof(AZ::u32);

        // Geometric-error LOD metric (optional -- packs built before builder v9
        // lack this section). One float per MeshDescriptorLodEntry record, in the
        // same pack-global (mesh, LOD) order those records are read below.
        auto lodErrorBytes = reader.GetSection(SectionKind::LodError);
        const float* lodErrorsAll = lodErrorBytes.empty()
            ? nullptr : reinterpret_cast<const float*>(lodErrorBytes.data());
        const size_t lodErrorCount = lodErrorBytes.size() / sizeof(float);
        AZ::u32 lodEntryGlobalIndex = 0;

        // Validate sections exist.
        if (clusterBytes.empty() || triBytes.empty() || indirBytes.empty() || vsBytes.empty())
        {
            AZ_Error("Meshlets", false,
                "MeshletsRenderObject: pack missing required SP1 sections");
            return;
        }

        // ----------------------------------------------------------------
        // Parse the VertexStreams sub-header + 5 per-stream descriptors.
        // Layout: VertexStreamSubHeader (8 B) + streamCount x VertexStreamDescriptor (24 B each)
        // followed by the concatenated stream data.
        // ----------------------------------------------------------------
        if (vsBytes.size() < sizeof(VertexStreamSubHeader))
        {
            AZ_Error("Meshlets", false, "MeshletsRenderObject: VertexStreams section too small");
            return;
        }
        const auto* vsSub = reinterpret_cast<const VertexStreamSubHeader*>(vsBytes.data());
        const AZ::u32 streamCount = vsSub->m_streamCount;
        const AZ::u32 totalPackVertexCount = vsSub->m_totalVertexCount;
        const AZ::u64 vsDescHeaderBytes = sizeof(VertexStreamSubHeader) + streamCount * sizeof(VertexStreamDescriptor);
        if (vsBytes.size() < vsDescHeaderBytes)
        {
            AZ_Error("Meshlets", false, "MeshletsRenderObject: VertexStreams section truncated");
            return;
        }
        const auto* vsDescs = reinterpret_cast<const VertexStreamDescriptor*>(vsBytes.data() + sizeof(VertexStreamSubHeader));
        (void)totalPackVertexCount;

        // Pack-global arrays (all meshes concatenated).
        const ClusterDescriptor* clustersAll = reinterpret_cast<const ClusterDescriptor*>(clusterBytes.data());
        const AZ::u32* trisAll   = reinterpret_cast<const AZ::u32*>(triBytes.data());
        const AZ::u32* indirAll  = reinterpret_cast<const AZ::u32*>(indirBytes.data());
        const AZ::u8* expandedIdxAll = expandedIdxBytes.empty() ? nullptr : expandedIdxBytes.data();
        const AZ::u64 expandedIdxBytesSize = expandedIdxBytes.size();
        if (expandedIdxAll == nullptr)
        {
            AZ_Warning("Meshlets", false,
                "MeshletsRenderObject: pack is missing ExpandedIndices section. "
                "Regenerate via the asset builder (BuilderVersion >= 8, which also "
                "bakes/generates per-mesh LODs). "
                "Meshlet rendering will likely produce no visible geometry.");
        }

        // ----------------------------------------------------------------
        // MeshDescriptors layout: for each mesh, one 40-byte MeshDescriptorPrefix
        // followed by prefix.m_lodCount 32-byte MeshDescriptorLodEntry records,
        // then (after all meshes) a name blob we don't read at runtime.
        //
        // m_modelRenderData is indexed [lod][meshIdx]. We pre-scan the prefixes to
        // find the maximum LOD count across all meshes so the outer array is sized
        // once; a mesh with fewer LODs simply contributes nullptr entries to the
        // higher LOD slots (GetMeshletsRenderData clamps an out-of-range lodIdx to
        // size-1, so selecting LOD>available returns the coarsest built LOD).
        // ----------------------------------------------------------------
        const AZ::u8* const descBegin = meshDescBytes.data();
        const AZ::u8* const end = meshDescBytes.data() + meshDescBytes.size();

        // First pass: scan prefixes ONLY (advancing past their LOD entries without
        // dereferencing them) to find maxLodCount. This walk uses the SAME atomic
        // advance as the build pass below so the two stay in lockstep.
        AZ::u16 maxLodCount = 1;
        {
            const AZ::u8* sp = descBegin;
            for (AZ::u32 m = 0; m < hdr->m_meshCount; ++m)
            {
                if (sp + sizeof(MeshDescriptorPrefix) > end)
                {
                    break;   // truncated -- stop scanning.
                }
                const auto* prefix = reinterpret_cast<const MeshDescriptorPrefix*>(sp);
                const AZ::u16 lodCount = prefix->m_lodCount;
                const AZ::u64 lodEntriesBytes =
                    static_cast<AZ::u64>(lodCount) * sizeof(MeshDescriptorLodEntry);
                // Bounds-check that all K LOD entries fit before advancing past them.
                if (static_cast<AZ::u64>(end - sp) <
                    sizeof(MeshDescriptorPrefix) + lodEntriesBytes)
                {
                    break;   // truncated -- the entries don't fit.
                }
                if (lodCount > maxLodCount)
                {
                    maxLodCount = lodCount;
                }
                sp += sizeof(MeshDescriptorPrefix) + lodEntriesBytes;
            }
        }
        // Each LOD slot is indexed by meshIdx and pre-sized to the full mesh count
        // with nullptr placeholders. A mesh that has fewer LODs than maxLodCount
        // leaves nullptr at its meshIdx in the higher LOD slots -- keeping
        // m_modelRenderData[lod][meshIdx] aligned to logical mesh meshIdx for EVERY
        // lod (consumers index by MeshIndex and already null-check the slot).
        m_modelRenderData.assign(maxLodCount, ModelLodDataArray(hdr->m_meshCount, nullptr));

        AZ_TracePrintf("Meshlets",
            "MeshletsRenderObject: pack streamCount=%u totalVertexCount=%u meshCount=%u "
            "maxLodCount=%u maxVerts/cluster=%u maxTris/cluster=%u\n",
            streamCount, totalPackVertexCount, hdr->m_meshCount, maxLodCount,
            hdr->m_maxVerticesPerCluster, hdr->m_maxTrianglesPerCluster);

        // Second pass: build render data. The raw pointer p ALWAYS advances by the
        // FULL per-mesh record (prefix + ALL K LOD entries) before the next mesh --
        // it is NEVER left mid-record. We snapshot the per-mesh LOD-entry block
        // bounds up front, advance p past the whole mesh, THEN instantiate each
        // LOD's render data from the snapshotted pointers. This decoupling is the
        // corruption-safety fix: a build failure or a clamp can never desync p.
        const AZ::u8* p = descBegin;
        for (AZ::u32 m = 0; m < hdr->m_meshCount; ++m)
        {
            if (p + sizeof(MeshDescriptorPrefix) > end)
            {
                AZ_Error("Meshlets", false,
                    "MeshletsRenderObject: MeshDescriptors truncated at mesh %u (no prefix)", m);
                break;
            }
            const auto* prefix = reinterpret_cast<const MeshDescriptorPrefix*>(p);
            const AZ::u16 lodCount = prefix->m_lodCount;
            const AZ::u64 lodEntriesBytes =
                static_cast<AZ::u64>(lodCount) * sizeof(MeshDescriptorLodEntry);

            // Bounds-check the K LOD entries fit BEFORE we read any of them.
            if (static_cast<AZ::u64>(end - p) <
                sizeof(MeshDescriptorPrefix) + lodEntriesBytes)
            {
                AZ_Error("Meshlets", false,
                    "MeshletsRenderObject: MeshDescriptors truncated at mesh %u "
                    "(need %llu bytes for prefix+%u LOD entries, have %llu)",
                    m,
                    (unsigned long long)(sizeof(MeshDescriptorPrefix) + lodEntriesBytes),
                    lodCount, (unsigned long long)(end - p));
                break;
            }

            // Snapshot the LOD-entry block, then ATOMICALLY advance p past the
            // entire mesh record (prefix + all K entries). After this line p points
            // at the next mesh's prefix regardless of what happens below.
            const AZ::u8* const lodEntriesBase = p + sizeof(MeshDescriptorPrefix);
            p += sizeof(MeshDescriptorPrefix) + lodEntriesBytes;

            // Instantiate render data for EVERY LOD of this mesh.
            for (AZ::u16 lod = 0; lod < lodCount; ++lod)
            {
                const auto* lodEntry = reinterpret_cast<const MeshDescriptorLodEntry*>(
                    lodEntriesBase + static_cast<AZ::u64>(lod) * sizeof(MeshDescriptorLodEntry));

                auto* meshRenderData = aznew MeshRenderData();

                // Geometric-error LOD metric: lodErrorsAll[lodEntryGlobalIndex] parallels
                // this exact (mesh, LOD) record -- same pack-global order the builder wrote
                // it in (mesh order outer, LOD order inner). Bounds-checked since older
                // packs may have a shorter (empty) section.
                if (lodErrorsAll != nullptr && lodEntryGlobalIndex < lodErrorCount)
                {
                    meshRenderData->LodGeometricError = lodErrorsAll[lodEntryGlobalIndex];
                    meshRenderData->HasLodError = true;
                }
                ++lodEntryGlobalIndex;

                const AZ::u32 meshClusterCount = lodEntry->m_clusterCount;
                const AZ::u32 meshVertexCount  = lodEntry->m_vertexCount;
                const AZ::u32 meshVertexFirst  = lodEntry->m_vertexFirst;
                // Upper-bound size for the output index slab -- every cluster
                // can have AT MOST maxTrianglesPerCluster triangles, but the
                // ACTUAL written count is sum(cluster.m_triangleCount)*3, which
                // we compute in the cluster scan below and assign to
                // meshRenderData->IndexCount. Using the upper bound for the
                // RWBuffer allocation is fine (it's just a capacity ceiling);
                // using it for the DRAW vertex count is what hung the GPU
                // (DXGI_ERROR_DEVICE_HUNG) -- the vertex shader fetched
                // uninitialized indices from m_meshletsSharedBuffer slots the
                // compute never touched. Now IndexCount is set from the actual
                // totalTriangles*3 sum after the cluster scan.
                const AZ::u32 meshIndexCount   =
                    meshClusterCount * hdr->m_maxTrianglesPerCluster * 3;

                AZ_TracePrintf("Meshlets",
                    "MeshletsRenderObject: mesh %u LOD %u clusters=%u verts=%u "
                    "vertFirst=%u clusterFirst=%u\n",
                    m, lod, meshClusterCount, meshVertexCount,
                    meshVertexFirst, lodEntry->m_clusterFirst);

                meshRenderData->MeshletsCount = meshClusterCount;
                meshRenderData->VertexCount = meshVertexCount;
                // IndexCount intentionally set below -- needs totalTriangles first.

                // --------------------------------------------------------
                // Step 1: Slice + re-base cluster descriptors for this mesh.
                //
                // Pack clusters have vertexOffset / triangleOffset relative to
                // the pack-global indirection / triangle arrays.  We copy this
                // mesh's slice into a local buffer and re-base those offsets so
                // they're relative to the per-mesh slab bases we compute below.
                // --------------------------------------------------------
                const ClusterDescriptor* meshClusters = clustersAll + lodEntry->m_clusterFirst;

                // Phase 6 cluster DAG (pack v3): m_clusterCount is the LEAF count and
                // m_dagClusterCount (when nonzero, always >= leaf count) is the full
                // leaf+interior range, laid out leaves-first. The per-mesh slabs
                // (triangles/indirection/descriptors/bounds) must cover the FULL range
                // so the DAG-aware AS path can decode interior clusters; every
                // whole-mesh DRAW quantity (IndexCount, MeshletsCount) stays leaf-only
                // so non-DAG-aware paths never double-draw interiors.
                const bool hasDag = dagNodesAll != nullptr && lodEntry->m_dagClusterCount > meshClusterCount;
                const AZ::u32 sliceClusterCount = hasDag ? lodEntry->m_dagClusterCount : meshClusterCount;

                // Walk the cluster slice to find per-mesh triangle / indirection ranges.
                AZ::u32 triBase  = 0;
                AZ::u32 indirBase = 0;
                AZ::u32 totalTriangles  = 0;       // FULL slice (incl. DAG interiors)
                AZ::u32 totalIndirection = 0;      // FULL slice
                AZ::u32 leafTriangles = 0;         // leaves only -- the drawable set
                bool firstCluster = true;
                for (AZ::u32 c = 0; c < sliceClusterCount; ++c)
                {
                    const ClusterDescriptor& cl = meshClusters[c];
                    if (firstCluster)
                    {
                        triBase   = cl.m_triangleOffset;
                        indirBase = cl.m_vertexOffset;
                        firstCluster = false;
                    }
                    // Count how many triangle u32s this cluster spans.
                    // triangleOffset is in u32 units; each cluster occupies
                    // m_triangleCount u32s (one per triangle, three 8-bit indices packed).
                    totalTriangles  += cl.m_triangleCount;
                    totalIndirection += cl.m_vertexCount;
                    if (c < meshClusterCount)
                    {
                        leafTriangles += cl.m_triangleCount;
                    }
                }

                // ActualIndexCount = triangles in the whole slab (leaf + interior);
                // IndexCount = the LEAF-only draw count. Drawing past the written
                // range hangs the GPU (uninitialized indices), and drawing past the
                // LEAF range double-draws DAG interiors on top of leaves.
                const AZ::u32 actualIndexCount = totalTriangles * 3;
                meshRenderData->IndexCount = leafTriangles * 3;
                AZ_TracePrintf("Meshlets",
                    "MeshletsRenderObject: mesh %u IndexCount=%u (capacity=%u, totalTriangles=%u, totalIndirection=%u, dagClusters=%u)\n",
                    m, meshRenderData->IndexCount, meshIndexCount, totalTriangles, totalIndirection,
                    hasDag ? sliceClusterCount : 0u);

                // Build re-based copy of cluster descriptors (FULL slice).
                AZStd::vector<ClusterDescriptor> rebasedClusters(sliceClusterCount);
                for (AZ::u32 c = 0; c < sliceClusterCount; ++c)
                {
                    rebasedClusters[c] = meshClusters[c];
                    rebasedClusters[c].m_triangleOffset -= triBase;
                    rebasedClusters[c].m_vertexOffset   -= indirBase;
                }

                // Phase 6 culling: keep persistent CPU copies of the rebased cluster
                // descriptors (for building indirect commands: each cluster's expanded
                // indices live at [triangleOffset*3, +triangleCount*3)) and the matching
                // per-cluster bounds slice (object-space sphere + cone for the cull test).
                // Both cover the FULL DAG range for DAG packs -- consumers that must stay
                // leaf-only cap at MeshletsCount, not at these vectors' sizes.
                meshRenderData->PersistentClusterDescriptors.assign(
                    rebasedClusters.begin(), rebasedClusters.end());
                if (clusterBoundsAll != nullptr)
                {
                    const ClusterBoundsRecord* meshBounds = clusterBoundsAll + lodEntry->m_clusterFirst;
                    meshRenderData->PersistentClusterBounds.assign(
                        meshBounds, meshBounds + sliceClusterCount);
                }
                if (hasDag)
                {
                    const DagNodeRecord* meshDagNodes = dagNodesAll + lodEntry->m_clusterFirst;
                    meshRenderData->PersistentDagNodes.assign(
                        meshDagNodes, meshDagNodes + sliceClusterCount);
                    meshRenderData->DagClusterCount = sliceClusterCount;
                }

                // Phase 7 streaming: this mesh's leaf pages (matched by LOD-entry
                // index) with m_clusterFirst rebased mesh-local, plus the per-cluster
                // parent mapping (also rebased; 0xFFFFFFFF roots preserved).
                if (hasDag && pageTableAll != nullptr && !pageDataBytes.empty())
                {
                    // lodEntryGlobalIndex was already incremented for this entry above.
                    const AZ::u32 thisLodEntryIndex = lodEntryGlobalIndex - 1;
                    for (size_t pg = 0; pg < pageTableCount; ++pg)
                    {
                        if (pageTableAll[pg].m_lodEntryIndex != thisLodEntryIndex)
                        {
                            continue;
                        }
                        PageTableRecord rec = pageTableAll[pg];
                        // Bounds-check the payload range before trusting it.
                        if (rec.m_dataOffset > pageDataBytes.size() ||
                            rec.m_dataSize > pageDataBytes.size() - rec.m_dataOffset ||
                            rec.m_clusterFirst < lodEntry->m_clusterFirst ||
                            rec.m_clusterFirst + rec.m_clusterCount >
                                lodEntry->m_clusterFirst + meshClusterCount)
                        {
                            AZ_Warning("Meshlets", false,
                                "MeshletsRenderObject: malformed page record %zu -- skipped.", pg);
                            continue;
                        }
                        rec.m_clusterFirst -= lodEntry->m_clusterFirst;   // -> mesh-local
                        meshRenderData->PersistentPageTable.push_back(rec);
                    }
                    meshRenderData->PageData = pageDataBytes;

                    if (parentIndexAll != nullptr &&
                        lodEntry->m_clusterFirst + sliceClusterCount <= parentIndexCount)
                    {
                        meshRenderData->PersistentParentIndex.reserve(sliceClusterCount);
                        for (AZ::u32 c = 0; c < sliceClusterCount; ++c)
                        {
                            const AZ::u32 parent = parentIndexAll[lodEntry->m_clusterFirst + c];
                            meshRenderData->PersistentParentIndex.push_back(
                                parent == 0xFFFFFFFFu ? 0xFFFFFFFFu : parent - lodEntry->m_clusterFirst);
                        }
                    }

                    // Phase 4 VRAM reclaim: fully paged mesh + the exclusive switch =>
                    // never create the monolithic geometry buffers for this mesh.
                    if (r_meshletsStreamingExclusive && !meshRenderData->PersistentPageTable.empty())
                    {
                        meshRenderData->MonolithicDropped = true;
                        AZ_TracePrintf("Meshlets",
                            "MeshletsRenderObject: mesh %u is STREAMING-EXCLUSIVE -- "
                            "monolithic geometry buffers skipped (paged path only).\n", m);
                    }
                }

                // --------------------------------------------------------
                // Step 2: Prepare ComputeBuffersDescriptors (mirrors
                // PrepareComputeSrgDescriptors from the pre-cleanup code,
                // feeding pack-derived sizes / pointers).
                // --------------------------------------------------------
                meshRenderData->ComputeBuffersDescriptors.resize(
                    uint8_t(ComputeStreamsSemantics::NumBufferStreams));

                // [0] MeshletsData -- StructuredBuffer<ClusterDescriptor>
                meshRenderData->ComputeBuffersDescriptors[uint8_t(ComputeStreamsSemantics::MeshletsData)] =
                    SrgBufferDescriptor(
                        RPI::CommonBufferPoolType::ReadOnly,
                        RHI::Format::Unknown,  // StructuredBuffer
                        RHI::BufferBindFlags::ShaderRead,
                        sizeof(ClusterDescriptor), sliceClusterCount,
                        Name{ "MESHLETS" }, Name{ "m_meshletsDescriptors" }, 0, 0,
                        reinterpret_cast<uint8_t*>(rebasedClusters.data())
                    );

                // [1] MeshletsTriangles -- Buffer<uint>
                meshRenderData->ComputeBuffersDescriptors[uint8_t(ComputeStreamsSemantics::MeshletsTriangles)] =
                    SrgBufferDescriptor(
                        RPI::CommonBufferPoolType::ReadOnly,
                        RHI::Format::R32_UINT,
                        RHI::BufferBindFlags::ShaderRead,
                        sizeof(AZ::u32), totalTriangles,
                        Name{ "MESHLETS_TRIANGLES" }, Name{ "m_meshletsTriangles" }, 1, 0,
                        // Pointer into pack-owned span; pack asset outlives this object.
                        const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(trisAll + triBase))
                    );

                // [2] MeshletsIndicesLookup -- Buffer<uint>
                meshRenderData->ComputeBuffersDescriptors[uint8_t(ComputeStreamsSemantics::MeshletsIndicesIndirection)] =
                    SrgBufferDescriptor(
                        RPI::CommonBufferPoolType::ReadOnly,
                        RHI::Format::R32_UINT,
                        RHI::BufferBindFlags::ShaderRead,
                        sizeof(AZ::u32), totalIndirection,
                        Name{ "MESHLETS_LOOKUP" }, Name{ "m_meshletsIndicesLookup" }, 2, 0,
                        const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(indirAll + indirBase))
                    );

                // SP1 fix: pre-bake the EXPANDED triangle index list and the
                // per-vertex debug UVs on the CPU here, BEFORE buffer creation,
                // so they can be passed as initial data into
                // CreateBufferFromCommonPool. The previous post-creation
                // UpdateData path was returning success but reads on the GPU
                // came back as zeros (apparent state-tracking / staging-flush
                // issue with ReadWrite-pool dedicated buffers on AMD). Using
                // the initial-data path routes the upload through the engine's
                // StreamBuffer fenced mechanism, which is the same path the
                // vertex-stream buffers (positions/normals/etc.) use -- and
                // those have always rendered correctly.
                //
                // The expanded data is stored as members of meshRenderData
                // (PersistentExpandedIndices / PersistentExpandedUVs) so the
                // m_bufferData pointers we hand to the SrgBufferDescriptor
                // ctors remain valid through the post-creation UpdateData
                // calls in PackInit (lifetime: end of this loop iteration).
                // It also matches positions' pattern where m_bufferData points
                // at PERSISTENT pack-asset memory rather than a local vector.
                AZStd::vector<AZ::u32>& expandedIndices = meshRenderData->PersistentExpandedIndices;
                expandedIndices.assign(actualIndexCount, 0u);
                AZStd::vector<float>& expandedUVs = meshRenderData->PersistentExpandedUVs;
                expandedUVs.assign((size_t)meshVertexCount * 2u, 0.0f);
                {
                    const AZ::u32* tris  = trisAll  + triBase;
                    const AZ::u32* indir = indirAll + indirBase;
                    // FULL slice: DAG interiors get expanded index ranges too, so the
                    // compute-cull path can draw any cut cluster's slice. Leaf-only
                    // paths draw [0, IndexCount) which spans exactly the leaves.
                    for (AZ::u32 c = 0; c < sliceClusterCount; ++c)
                    {
                        const ClusterDescriptor& cl = rebasedClusters[c];
                        const AZ::u32 uvDebugIndex = c + 7;
                        const float u = float(uvDebugIndex % 3u) * 0.5f;
                        const float v = float((uvDebugIndex / 3u) % 3u) * 0.5f;
                        for (AZ::u32 t = 0; t < cl.m_triangleCount; ++t)
                        {
                            const AZ::u32 encodedTri = tris[cl.m_triangleOffset + t];
                            const AZ::u32 localXYZ[3] = {
                                (encodedTri >>  0) & 0xff,
                                (encodedTri >>  8) & 0xff,
                                (encodedTri >> 16) & 0xff
                            };
                            // LOD-CORRECTNESS FIX: the builder bakes VertexIndirection
                            // values as PACK-GLOBAL vertex indices (local + this LOD's
                            // vertexFirst -- MeshletPackBuilderCore.cpp:572 'idx + vertexCursor').
                            // But each LOD's vertex IA buffer is sliced/offset by
                            // meshVertexFirst (this file ~line 648), so its element 0 is the
                            // LOD's first vertex. The index buffer must therefore be LOD-LOCAL
                            // (0-based within the slice). Subtracting meshVertexFirst converts
                            // global->local. LOD0 has vertexFirst==0 (no change, why it always
                            // rendered); LOD1/2/3 had vertexFirst>0 -> global indices ran off the
                            // end of their slice -> blank. This makes ALL LODs render, with no
                            // re-bake (transforms existing global packs at load).
                            const AZ::u32 vX = indir[cl.m_vertexOffset + localXYZ[0]] - meshVertexFirst;
                            const AZ::u32 vY = indir[cl.m_vertexOffset + localXYZ[1]] - meshVertexFirst;
                            const AZ::u32 vZ = indir[cl.m_vertexOffset + localXYZ[2]] - meshVertexFirst;
                            const AZ::u32 dst = (cl.m_triangleOffset + t) * 3;
                            if (dst + 2 < actualIndexCount)
                            {
                                expandedIndices[dst + 0] = vX;
                                expandedIndices[dst + 1] = vY;
                                expandedIndices[dst + 2] = vZ;
                            }
                            // Per-vertex debug UV (last-cluster-wins for shared
                            // vertices; matches the compute shader's race-write
                            // semantics).
                            if (vX < meshVertexCount) { expandedUVs[vX * 2 + 0] = u; expandedUVs[vX * 2 + 1] = v; }
                            if (vY < meshVertexCount) { expandedUVs[vY * 2 + 0] = u; expandedUVs[vY * 2 + 1] = v; }
                            if (vZ < meshVertexCount) { expandedUVs[vZ * 2 + 0] = u; expandedUVs[vZ * 2 + 1] = v; }
                        }
                    }
                    AZ_TracePrintf("Meshlets",
                        "Pre-baked %u expanded triangle indices for mesh %u "
                        "(clusters=%u)\n",
                        actualIndexCount, m, meshClusterCount);
                }

                // [3] UVs -- Buffer<float2>, CPU-baked debug coloring (no compute writes for SP1).
                //
                // SP1 critical fix: use the ReadOnly pool (same as the vertex
                // streams positions/normals/tangents/bitangents on
                // MeshletsRenderObject.cpp lines 79/96/110/121). The ReadWrite
                // pool's StreamBuffer upload path is silently broken on AMD
                // here -- uploads return success and the fence signals, but the
                // device buffer ends up with garbage (uninitialized GPU memory
                // patterns rather than the CPU-supplied data). Routing through
                // the ReadOnly pool uses the proven upload path that the
                // vertex streams have always used and that always lands data.
                //
                // The compute SRG (MeshletsDataSrg) declares these slots as
                // RWBuffer; the bind will reject a Read-only-flagged buffer.
                // That's fine because compute is suppressed (see
                // MeshletsFeatureProcessor::Render -- we never push the
                // dispatch item to the compute pass). CreateAndBindCompute-
                // SrgAndDispatch is now a no-op at construction.
                //
                // m_uvs and m_indices are sized to actualIndexCount / vertex
                // count exactly (not capacity) since we no longer need the
                // over-allocation that the compute UAV-output used.
                meshRenderData->ComputeBuffersDescriptors[uint8_t(ComputeStreamsSemantics::UVs)] =
                    SrgBufferDescriptor(
                        RPI::CommonBufferPoolType::ReadOnly,
                        RHI::Format::R32G32_FLOAT,
                        RHI::BufferBindFlags::ShaderRead,
                        sizeof(float) * 2, meshVertexCount,
                        Name{ "UV" }, Name{ "m_uvs" }, 3, 0,
                        reinterpret_cast<uint8_t*>(expandedUVs.data())
                    );

                // [4] Indices -- Buffer<uint>, CPU-baked expanded index list.
                // Size to actualIndexCount (244332) instead of capacity
                // (meshIndexCount = 244416). The render draws only actualIndex
                // Count vertices so the capacity slots were unused, and using
                // capacity caused a memcpy(977664 from 977328-byte source)
                // overread inside BufferAssetCreator::SetBuffer.
                meshRenderData->ComputeBuffersDescriptors[uint8_t(ComputeStreamsSemantics::Indices)] =
                    SrgBufferDescriptor(
                        RPI::CommonBufferPoolType::ReadOnly,
                        RHI::Format::R32_UINT,
                        RHI::BufferBindFlags::ShaderRead,
                        sizeof(AZ::u32), actualIndexCount,
                        Name{ "INDICES" }, Name{ "m_indices" }, 4, 0,
                        reinterpret_cast<uint8_t*>(expandedIndices.data())
                    );

                // --------------------------------------------------------
                // Step 3: Create compute buffers as dedicated RPI::Buffer
                // instances (all streams, including UVs and Indices).
                // --------------------------------------------------------
                const AZ::u32 computeStreams = uint8_t(ComputeStreamsSemantics::NumBufferStreams);
                meshRenderData->ComputeBuffers.resize(computeStreams);

                bool computeOk = true;
                // Track buffers whose initial-data upload may be async (>64KB
                // triggers the StreamBuffer fenced path). We block on all such
                // uploads after the creation loop to ensure data is committed
                // before the SRG bind.
                AZStd::vector<Data::Instance<RPI::Buffer>> streamingBuffers;
                for (AZ::u32 stream = 0; stream < computeStreams; ++stream)
                {
                    SrgBufferDescriptor& bufDesc =
                        meshRenderData->ComputeBuffersDescriptors[stream];

                    // SP1 fix: allocate ALL compute-side buffers (including the RW
                    // outputs UVs and Indices) as dedicated RPI::Buffer instances
                    // instead of sub-views into the SharedBuffer. The SharedBuffer
                    // sub-view path was causing GPU page faults on AMD: the
                    // frame-graph state-tracking for two passes (compute UAV ->
                    // render SRV) operating on different views of the same
                    // underlying resource was emitting wrong / incomplete
                    // barriers, leaving the GPU reading uninitialized/UAV-state
                    // memory during the vertex shader's index fetch. With
                    // dedicated buffers each gets its own attachment, the
                    // frame graph tracks state cleanly, and no m_indicesOffset
                    // arithmetic is needed in shaders. m_viewOffsetInBytes is
                    // forced to 0 here so the render-side render pass picks up
                    // a sensible offset constant when it later mirrors this
                    // value into the ObjectSrg's m_indicesOffset.
                    bufDesc.m_viewOffsetInBytes = 0;
                    meshRenderData->ComputeBuffers[stream] =
                        UtilityClass::CreateBuffer("Meshlets", bufDesc);
                    computeOk &= (meshRenderData->ComputeBuffers[stream] != nullptr);

                    // SP1: track buffers that used the initial-data path; their
                    // upload may be async (>64KB triggers StreamBuffer which is
                    // fenced). We must wait for those fences before the SRG bind
                    // and any GPU read of the buffer, otherwise the render pass
                    // reads zeros (the buffer's GPU memory hasn't been written
                    // yet). Without this, even a successful initial-data path
                    // produces the same "invisible mesh, all-zero indices"
                    // symptom that the prior post-creation UpdateData hack had.
                    if (meshRenderData->ComputeBuffers[stream] && bufDesc.m_bufferData != nullptr)
                    {
                        streamingBuffers.emplace_back(meshRenderData->ComputeBuffers[stream]);
                    }
                }

                // SP1: block until all streaming uploads complete. WaitForUpload
                // is a no-op for buffers that used the synchronous initial-data
                // path (≤ 64KB) or post-creation UpdateData; for the StreamBuffer
                // path it does a fenced CPU wait per device. This adds a
                // one-time construction-cost block but eliminates the data race.
                if (!streamingBuffers.empty())
                {
                    for (auto& sb : streamingBuffers)
                    {
                        sb->WaitForUpload();
                    }
                }

                // Upload read-only input data.
                // SP1 note: this UpdateData is now REDUNDANT for streams that
                // already set m_bufferData (those get the data via the
                // initial-data path above). It's kept for backward compatibility
                // -- UpdateData on an already-populated buffer is harmless.
                if (computeOk)
                {
                    // [0] MeshletsData: re-based cluster descriptors (local vector).
                    {
                        SrgBufferDescriptor& bd =
                            meshRenderData->ComputeBuffersDescriptors[uint8_t(ComputeStreamsSemantics::MeshletsData)];
                        const size_t sz = (size_t)bd.m_elementCount * bd.m_elementSize;
                        computeOk &= meshRenderData->ComputeBuffers[uint8_t(ComputeStreamsSemantics::MeshletsData)]
                            ->UpdateData(rebasedClusters.data(), sz, 0);
                    }
                    // [1] MeshletsTriangles: slice of pack-global triangle array.
                    {
                        SrgBufferDescriptor& bd =
                            meshRenderData->ComputeBuffersDescriptors[uint8_t(ComputeStreamsSemantics::MeshletsTriangles)];
                        const size_t sz = (size_t)bd.m_elementCount * bd.m_elementSize;
                        computeOk &= meshRenderData->ComputeBuffers[uint8_t(ComputeStreamsSemantics::MeshletsTriangles)]
                            ->UpdateData(trisAll + triBase, sz, 0);
                    }
                    // [2] MeshletsIndicesIndirection: slice of pack-global indirection array.
                    {
                        SrgBufferDescriptor& bd =
                            meshRenderData->ComputeBuffersDescriptors[uint8_t(ComputeStreamsSemantics::MeshletsIndicesIndirection)];
                        const size_t sz = (size_t)bd.m_elementCount * bd.m_elementSize;
                        computeOk &= meshRenderData->ComputeBuffers[uint8_t(ComputeStreamsSemantics::MeshletsIndicesIndirection)]
                            ->UpdateData(indirAll + indirBase, sz, 0);
                    }
                    AZ_Error("Meshlets", computeOk, "Failed to upload compute input data for mesh %u", m);

                    // SP1 fix: also call UpdateData for Indices and UVs on top
                    // of the initial-data path. The vertex stream buffers
                    // (positions, normals, tangents, bitangents) in
                    // MeshletsRenderObject::CreateAndBindRenderBuffers always
                    // do BOTH (initial-data via the constructor argument AND
                    // post-creation UpdateData on lines 273-308) and that's
                    // the pattern that reliably lands data on AMD here. The
                    // initial-data path alone (via the StreamBuffer fenced
                    // upload) was returning success but the buffer still read
                    // garbage after WaitForUpload -- the post-creation
                    // UpdateData is what actually commits the data.
                    {
                        SrgBufferDescriptor& bd =
                            meshRenderData->ComputeBuffersDescriptors[uint8_t(ComputeStreamsSemantics::Indices)];
                        const size_t sz = (size_t)bd.m_elementCount * bd.m_elementSize;
                        if (bd.m_bufferData && meshRenderData->ComputeBuffers[uint8_t(ComputeStreamsSemantics::Indices)])
                        {
                            const bool ok =
                                meshRenderData->ComputeBuffers[uint8_t(ComputeStreamsSemantics::Indices)]
                                    ->UpdateData(bd.m_bufferData, sz, 0);
                            AZ_Error("Meshlets", ok,
                                "UpdateData failed for Indices buffer (mesh %u)", m);
                        }
                    }
                    {
                        SrgBufferDescriptor& bd =
                            meshRenderData->ComputeBuffersDescriptors[uint8_t(ComputeStreamsSemantics::UVs)];
                        const size_t sz = (size_t)bd.m_elementCount * bd.m_elementSize;
                        if (bd.m_bufferData && meshRenderData->ComputeBuffers[uint8_t(ComputeStreamsSemantics::UVs)])
                        {
                            const bool ok =
                                meshRenderData->ComputeBuffers[uint8_t(ComputeStreamsSemantics::UVs)]
                                    ->UpdateData(bd.m_bufferData, sz, 0);
                            AZ_Error("Meshlets", ok,
                                "UpdateData failed for UVs buffer (mesh %u)", m);
                        }
                    }
                }

                // --------------------------------------------------------
                // Step 4: Prepare RenderBuffersDescriptors (mirrors
                // PrepareRenderSrgDescriptors).
                // --------------------------------------------------------
                PrepareRenderSrgDescriptors(*meshRenderData, meshVertexCount, meshIndexCount);
                // Wire pack-byte pointers for each vertex stream into the
                // render descriptors so CreateAndBindRenderBuffers can
                // upload them via bufferDesc.m_bufferData.
                for (AZ::u32 sd = 0; sd < streamCount && sd < 5; ++sd)
                {
                    const VertexStreamDescriptor& vd = vsDescs[sd];
                    const StreamSemanticKind sem = static_cast<StreamSemanticKind>(vd.m_semanticKind);

                    RenderStreamsSemantics renderSlot;
                    switch (sem)
                    {
                    case StreamSemanticKind::Position:  renderSlot = RenderStreamsSemantics::Positions;   break;
                    case StreamSemanticKind::Normal:    renderSlot = RenderStreamsSemantics::Normals;     break;
                    case StreamSemanticKind::Tangent:   renderSlot = RenderStreamsSemantics::Tangents;    break;
                    case StreamSemanticKind::Bitangent: renderSlot = RenderStreamsSemantics::BiTangents;  break;
                    case StreamSemanticKind::UV0:       renderSlot = RenderStreamsSemantics::UVs;         break;
                    default: continue;
                    }

                    SrgBufferDescriptor& rd =
                        meshRenderData->RenderBuffersDescriptors[uint8_t(renderSlot)];
                    // Point into the pack's section span at the per-mesh vertex slice.
                    rd.m_bufferData = const_cast<AZ::u8*>(
                        vsBytes.data() + vd.m_byteOffsetInSection +
                        (AZ::u64)meshVertexFirst * vd.m_byteStride);
                    // Re-derive element count from the stride stored in the descriptor.
                    // PrepareRenderSrgDescriptors already set elementCount correctly
                    // for most slots; tangent stride is 16 B (4 floats), others 12 B
                    // (3 floats) or 8 B (2 floats for UV). Trust what was set there.
                }

                // SP1 v2: wire the per-mesh slice of the pre-baked ExpandedIndices
                // section. Per-mesh slice start = 3 * triBase u32s into the
                // section. Length = actualIndexCount u32s.
                if (expandedIdxAll != nullptr)
                {
                    const AZ::u64 indexByteOffset = (AZ::u64)triBase * 3 * sizeof(AZ::u32);
                    const AZ::u64 indexByteSize   = (AZ::u64)actualIndexCount * sizeof(AZ::u32);
                    if (indexByteOffset + indexByteSize > expandedIdxBytesSize)
                    {
                        AZ_Error("Meshlets", false,
                            "Pack ExpandedIndices section too small (need %llu+%llu, have %llu)",
                            (unsigned long long)indexByteOffset,
                            (unsigned long long)indexByteSize,
                            (unsigned long long)expandedIdxBytesSize);
                    }
                    else
                    {
                        SrgBufferDescriptor& rd =
                            meshRenderData->RenderBuffersDescriptors[uint8_t(RenderStreamsSemantics::Indices)];
                        rd.m_bufferData   = const_cast<AZ::u8*>(expandedIdxAll + indexByteOffset);
                        rd.m_elementCount = actualIndexCount;
                    }
                }

                // --------------------------------------------------------
                // Step 5: Create ObjectSrg + render buffers (mirrors
                // CreateAndBindRenderBuffers).  We call the existing helper
                // which reads bufferDesc.m_bufferData we wired above.
                // --------------------------------------------------------
                if (computeOk && !CreateAndBindRenderBuffers(*meshRenderData))
                {
                    AZ_Error("Meshlets", false,
                        "CreateAndBindRenderBuffers failed for mesh %u", m);
                    delete meshRenderData;
                    continue;
                }

                // SP1: compute pass is fully suppressed (see comments around
                // the ReadOnly pool selection above). We do NOT create the
                // compute SRG / dispatch item at construction either -- the
                // RWBuffer<uint> slots in MeshletsDataSrg would reject the
                // now-ReadOnly-pool buffers, and there is no GPU work that
                // would consume the SRG anyway. Tier 2 / Epic E re-introduces
                // compute (meshlet culling); when it does, the indices/uvs
                // buffers will need to flip back to ReadWrite pool AND the
                // upload path needs to be fixed (or the data needs to come
                // from compute writes again).

                // Store at THIS LOD's slot, indexed by meshIdx (NOT push_back, NOT
                // always [0]). lod < lodCount <= maxLodCount == size(), and m <
                // m_meshCount == each slot's size, so both indices are valid and
                // meshIdx stays aligned across LODs.
                m_modelRenderData[lod][m] = meshRenderData;
                // Only LOD0 contributes to the representative meshlet count so the
                // figure (GetMeshletsCount / debug HUD) isn't inflated by the
                // coarser LODs that are only drawn for distant instances.
                if (lod == 0)
                {
                    m_meshletsCount += meshRenderData->MeshletsCount;
                }
            }
            // Name bytes live at the end of the section; we never advance p into
            // them (the loop bound is hdr->m_meshCount and p tracks only
            // prefix+LOD records), so nothing to skip here.
        }

        m_sourceModelAsset = sourceModelAsset;
        // Hold a strong ref to the pack asset so its byte buffer outlives the
        // SrgBufferDescriptors and BufferViews built above -- those reference
        // raw pointers into MeshletPackAsset::m_bytes via the reader's spans.
        m_packAsset = packAsset;
    }

} // namespace AZ::Meshlets
