/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <AzCore/base.h>

namespace AZ::Meshlets
{
    //! On-disk byte-layout structs for .azmeshletpack v1.
    //! See docs/superpowers/specs/2026-05-08-meshlets-asset-pipeline-design.md section 4.
    //!
    //! AssetId is stored as raw GUID bytes + sub-id (see PackHeaderRecord);
    //! consumers that want to reconstruct AZ::Data::AssetId include
    //! AzCore/Asset/AssetCommon.h themselves at the use site.

    inline constexpr AZ::u32 PackMagic   = 0x504C544D;  // 'MTLP' little-endian
    inline constexpr AZ::u32 PackVersion = 2;  // SP1 v2: adds SectionKind::ExpandedIndices.
    //! Phase 6 v3: adds SectionKind::DagNodes + MeshDescriptorLodEntry::m_dagClusterCount.
    //! Written ONLY for packs whose sidecar opts into "generate_cluster_dag" -- non-DAG
    //! packs stay byte-identical v2. Readers accept both.
    inline constexpr AZ::u32 PackVersionDag = 3;
    //! v4: v3 + PageTable/PageData/ParentIndex (sidecar "generate_pages"). Keeps the
    //! full v3 sections as fallback, so streaming-off renders identically.
    inline constexpr AZ::u32 PackVersionPaged = 4;

    //! Hard page cap: fixed-size pool slots keep the runtime allocator a
    //! fragmentation-proof free-list (~150 KB payload at the 128v/256t budget).
    inline constexpr AZ::u32 PageMaxClusters = 16;
    inline constexpr AZ::u32 PackHeader = PackMagic | (PackVersion << 16);

    enum class SectionKind : AZ::u32
    {
        PackHeader         = 0,
        MeshDescriptors    = 1,
        ClusterDescriptors = 2,
        TriangleIndices    = 3,
        VertexIndirection  = 4,
        VertexStreams      = 5,
        // Reserved for SP2-SP6:
        ConeBounds         = 6,
        DagNodes           = 7,
        LodError           = 8,
        HzbCullMetadata    = 9,
        ClusterMaterialMap = 10,
        PageTable          = 11,
        PageData           = 12,
        //! v4: per-cluster pack-global index of the FIRST parent (0xFFFFFFFF = root),
        //! parallel to ClusterDescriptors -- exact group->children mapping for the
        //! residency cut, no float-matching of group records.
        ParentIndex        = 14,
        // SP1 v2: pre-baked flat triangle index list (3 u32 indices per
        // triangle, all clusters of all meshes concatenated in pack-global
        // order). Per-mesh slice start = 3 * triBase, length = 3 * meshTotal
        // Triangles. Mirrors what the compute shader USED to write at runtime
        // into a UAV; instead the builder pre-computes it so the runtime
        // can point an SRV's m_bufferData directly at pack-asset memory
        // (the same upload pattern the vertex-stream buffers use). This
        // avoids the cross-pass compute->render data flow entirely.
        ExpandedIndices    = 13,
    };

    enum class StreamSemanticKind : AZ::u32
    {
        Position  = 0,
        Normal    = 1,
        Tangent   = 2,
        Bitangent = 3,
        UV0       = 4,
    };

#pragma pack(push, 1)

    //! 16-byte file header.
    struct FileHeader
    {
        AZ::u32 m_magic;        //!< Must equal PackMagic.
        AZ::u32 m_version;      //!< Must equal PackVersion in SP1.
        AZ::u16 m_tocCount;     //!< Number of SectionTocEntry records that follow.
        AZ::u16 m_flags;        //!< bit 0: compressed body (SP6, must be 0 in SP1).
        AZ::u32 m_reserved;     //!< Must be 0.
    };
    static_assert(sizeof(FileHeader) == 16, "FileHeader must be 16 bytes");

    //! 32-byte ToC entry.
    struct SectionTocEntry
    {
        AZ::u32 m_kind;          //!< SectionKind value.
        AZ::u32 m_flags;         //!< Reserved (0 in SP1).
        AZ::u64 m_offset;        //!< Bytes from start of file.
        AZ::u64 m_size;          //!< Section size in bytes.
        AZ::u64 m_reserved;      //!< Must be 0.
    };
    static_assert(sizeof(SectionTocEntry) == 32, "SectionTocEntry must be 32 bytes");

    //! Kind 0: PackHeader (1 record). Carries the source-model AssetId so the
    //! runtime can verify the pack matches the requested model.
    //!
    //! NOTE: AssetId is stored as raw GUID bytes + sub-id rather than embedding
    //! AZ::Data::AssetId directly. AssetId is 32 bytes in memory (alignas(16) Uuid
    //! + u32 sub-id + 12 bytes trailing pad) and `#pragma pack(1)` cannot strip
    //! that embedded pad. Storing the components inline gives a stable 60-byte
    //! on-disk record independent of any future O3DE-internal AssetId layout
    //! changes. Writers and readers convert via AZ::Uuid + AZ::Data::AssetId
    //! constructors.
    struct PackHeaderRecord
    {
        AZ::u8  m_sourceModelGuid[16];   //!< Uuid raw bytes.
        AZ::u32 m_sourceModelSubId;
        AZ::u32 m_reserved0;             //!< Must be 0.
        AZ::u32 m_meshCount;
        AZ::u16 m_maxVerticesPerCluster;
        AZ::u16 m_maxTrianglesPerCluster;
        float   m_coneWeight;
        float   m_aabbMin[3];
        float   m_aabbMax[3];
    };
    static_assert(sizeof(PackHeaderRecord) == 60, "PackHeaderRecord must be 60 bytes");

    //! Kind 1 prefix (40 bytes), followed by m_lodCount MeshDescriptorLodEntry records.
    struct MeshDescriptorPrefix
    {
        AZ::u32 m_nameOffset;     //!< Into per-section name blob.
        AZ::u32 m_nameSize;       //!< Bytes (UTF-8, no terminator).
        AZ::u16 m_lodCount;       //!< SP1 always 1.
        AZ::u16 m_reserved0;
        AZ::u32 m_reserved1;
        float   m_aabbMin[3];
        float   m_aabbMax[3];
    };
    static_assert(sizeof(MeshDescriptorPrefix) == 40, "MeshDescriptorPrefix must be 40 bytes");

    struct MeshDescriptorLodEntry
    {
        AZ::u32 m_clusterFirst;
        AZ::u32 m_clusterCount;   //!< LEAF cluster count. DAG packs (v3) lay out clusters
                                  //!< leaves-first; every path that draws "all clusters"
                                  //!< (uncull MS, shadows, vertex-pull) uses THIS count so
                                  //!< interior DAG nodes are never double-drawn.
        AZ::u32 m_vertexFirst;
        AZ::u32 m_vertexCount;
        AZ::u32 m_materialId;     //!< Reserved for SP5; must be 0xFFFFFFFF in SP1.
        AZ::u32 m_dagClusterCount; //!< Phase 6 (v3): TOTAL clusters incl. interior DAG
                                   //!< levels, starting at m_clusterFirst. 0 = no DAG
                                   //!< (was m_reserved0 -- always 0 in v1/v2 packs).
        AZ::u64 m_reserved1;
    };
    static_assert(sizeof(MeshDescriptorLodEntry) == 32, "MeshDescriptorLodEntry must be 32 bytes");

    //! Kind 2: 16-byte cluster descriptor. Layout-compatible with meshopt_Meshlet.
    struct ClusterDescriptor
    {
        AZ::u32 m_vertexOffset;     //!< Into VertexIndirection.
        AZ::u32 m_triangleOffset;   //!< Into TriangleIndices (in u32 units).
        AZ::u32 m_vertexCount;
        AZ::u32 m_triangleCount;
    };
    static_assert(sizeof(ClusterDescriptor) == 16, "ClusterDescriptor must be 16 bytes");

    //! Kind 6 (ConeBounds): per-cluster culling bounds, one record per cluster in
    //! pack-global order (parallel to the ClusterDescriptors array). Computed by
    //! the builder via meshopt_computeMeshletBounds. The GPU cull compute uses the
    //! bounding sphere for frustum culling and the normal cone for backface
    //! culling. 48 bytes = 3x float4, so it maps cleanly to a GPU StructuredBuffer.
    struct ClusterBoundsRecord
    {
        float m_center[3];    //!< Bounding-sphere center (object space).
        float m_radius;       //!< Bounding-sphere radius.
        float m_coneApex[3];  //!< Normal-cone apex (object space).
        float m_coneCutoff;   //!< cos(half-angle); backface when dot(axis, normalize(apex-eye)) >= cutoff.
        float m_coneAxis[3];  //!< Normal-cone axis (unit, object space).
        float m_pad;          //!< Padding to 48 bytes / float4 alignment.
    };
    static_assert(sizeof(ClusterBoundsRecord) == 48, "ClusterBoundsRecord must be 48 bytes");

    //! Kind 7 (DagNodes, v3): per-cluster cut record, parallel to ClusterDescriptors.
    //! Drawn iff errPx(self) <= tau < errPx(parent). Crack-freedom invariants the
    //! builder guarantees: group-shared records (a group's parents share one self
    //! record; its children share it as their parent record), max-propagated
    //! monotonic errors, parent spheres enclose child spheres. Leaves: selfError 0;
    //! roots: parentError FLT_MAX.
    struct DagNodeRecord
    {
        float m_selfSphere[4];    //!< xyz = center, w = radius (object space, group-shared).
        float m_parentSphere[4];
        float m_selfError;        //!< Object-space error of the group this cluster belongs to.
        float m_parentError;      //!< Error of the group this cluster was simplified into.
        float m_pad[2];
    };
    static_assert(sizeof(DagNodeRecord) == 48, "DagNodeRecord must be 48 bytes");

    //! Kind 11 (PageTable, v4): one record per streaming page. A page's clusters are
    //! contiguous in pack-global order (leaves are permuted into page order before
    //! the DAG builds on them). Payload layout (tight, 16-byte aligned):
    //!   PagedClusterRecord[clusterCount] | triangleWords | indirection(PAGE-local
    //!   vertex ids) | pos/norm/tan/bitan/uv floats.
    //! Self-contained: shared vertices are duplicated across pages so rendering a
    //! resident page touches nothing outside its payload.
    struct PageTableRecord
    {
        AZ::u64 m_dataOffset;         //!< Bytes from the start of the PageData section.
        AZ::u32 m_dataSize;           //!< Payload bytes (unpadded).
        AZ::u32 m_lodEntryIndex;      //!< Pack-global MeshDescriptorLodEntry index this page belongs to.
        AZ::u32 m_clusterFirst;       //!< PACK-GLOBAL id of the page's first (leaf) cluster.
        AZ::u32 m_clusterCount;       //!< <= PageMaxClusters; clusters are contiguous from m_clusterFirst.
        AZ::u32 m_vertexCount;        //!< Page-local unique vertices.
        AZ::u32 m_triangleWords;      //!< Actual triangle words in the payload.
        AZ::u32 m_indirCount;         //!< Actual indirection entries (== sum of cluster vertex counts).
        //! Bit 0 (PageFlagAlwaysResident): interior-level page, pinned resident --
        //! the coarse fallback that keeps every budget hole-free.
        AZ::u32 m_flags;
        float   m_aabbMin[3];         //!< Object-space bounds of the page's clusters.
        float   m_aabbMax[3];
        //! max leaf parentError: the classifier wants this page iff its projection
        //! exceeds tau (some leaf's parent is no longer good enough on screen).
        float   m_maxParentError;
        AZ::u32 m_reserved1;
    };
    static_assert(sizeof(PageTableRecord) == 72, "PageTableRecord must be 72 bytes");

    //! Per-cluster record at the head of each page payload (page-local offsets).
    struct PagedClusterRecord
    {
        AZ::u32 m_triangleWordFirst;  //!< Into the page's triangleWords array.
        AZ::u32 m_triangleCount;
        AZ::u32 m_indirFirst;         //!< Into the page's indirection array.
        AZ::u32 m_vertexCount;        //!< Cluster-local vertex count (== indir entries).
    };
    static_assert(sizeof(PagedClusterRecord) == 16, "PagedClusterRecord must be 16 bytes");

    inline constexpr AZ::u32 PageFlagAlwaysResident = 1u << 0;

    //! Runtime pool slot layout (StructuredBuffer<uint>, floats as bit patterns):
    //! 4-word header {clusterCount, vertexCount, triWords, indirCount}, then the
    //! payload verbatim. PageSlotU32s = capacity for a full page at the 128v/256t
    //! clamp ceiling; oversize pages are rejected at load.
    inline constexpr AZ::u32 PageSlotHeaderU32s = 4;
    inline constexpr AZ::u32 PageSlotU32s =
        PageSlotHeaderU32s +
        PageMaxClusters * 4 +                       // cluster records
        PageMaxClusters * 256 +                     // triangle words (<= 256 tris/cluster)
        PageMaxClusters * 128 +                     // indirection (<= 128 verts/cluster)
        PageMaxClusters * 128 * (3 + 3 + 4 + 3 + 2);// unique verts upper bound x 15 floats
    static_assert(PageSlotU32s * 4 < 256 * 1024, "page slot must stay well under 256KB");

    //! Kind 8 (LodError): one 4-byte float per MeshDescriptorLodEntry record, in the
    //! same pack-global (mesh, LOD) order the LOD entries are written into
    //! MeshDescriptors (parallel to them the same way ConeBounds is parallel to
    //! ClusterDescriptors). Value = meshopt_simplify's resultError for that LOD --
    //! meshoptimizer documents this as already "relative to mesh extents"
    //! (dimensionless, [0,1] range), i.e. already scale-independent, so no further
    //! normalization is applied on top of it. 0.0 for LOD0 (never simplified) and
    //! for source-supplied ("baked") LODs the builder does not run meshopt_simplify on.
    //! Optional -- packs built before builder v9 lack this section; the runtime
    //! falls back to screen-coverage LOD selection when it's empty/absent.

    //! Kind 5 sub-header (8 bytes) followed by stream_count VertexStreamDescriptor records.
    struct VertexStreamSubHeader
    {
        AZ::u32 m_totalVertexCount;
        AZ::u32 m_streamCount;       //!< SP1 = 5.
    };
    static_assert(sizeof(VertexStreamSubHeader) == 8, "VertexStreamSubHeader must be 8 bytes");

    struct VertexStreamDescriptor
    {
        AZ::u32 m_format;             //!< AZ::RHI::Format value.
        AZ::u32 m_byteOffsetInSection;
        AZ::u32 m_byteStride;
        AZ::u32 m_semanticKind;       //!< StreamSemanticKind.
        AZ::u32 m_reserved0;
        AZ::u32 m_reserved1;
    };
    static_assert(sizeof(VertexStreamDescriptor) == 24, "VertexStreamDescriptor must be 24 bytes");

#pragma pack(pop)

    inline constexpr AZ::u32 InvalidMaterialId = 0xFFFFFFFFu;
    inline constexpr AZ::u16 SectionAlignment  = 16;

} // namespace AZ::Meshlets
