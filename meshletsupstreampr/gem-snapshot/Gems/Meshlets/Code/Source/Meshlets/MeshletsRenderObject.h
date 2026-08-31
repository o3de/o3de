/*
* Modifications Copyright (c) Contributors to the Open 3D Engine Project. 
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
* 
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#pragma once

#include <AzCore/base.h>
#include <AzCore/Name/Name.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/array.h>

#include <AtomCore/Instance/Instance.h>
#include <AtomCore/Instance/InstanceData.h>

#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/Buffer/RingBuffer.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RHI/DrawPacketBuilder.h>
#include <Atom/RHI/IndirectBufferView.h>

#include <AzCore/Math/Frustum.h>
#include <AzCore/Math/Matrix4x4.h>
#include <Atom/RHI/IndirectBufferView.h>
#include <Atom/RHI/IndirectBufferSignature.h>
#include <Atom/RHI/GeometryView.h>

#include <Atom/Feature/TransformService/TransformServiceFeatureProcessorInterface.h>

#include <SharedBuffer.h>
#include <MeshletsDispatchItem.h>
#include <Meshlets/Reflect/MeshletPackAsset.h>

namespace AZ
{
    namespace RPI
    {
        class MeshDrawPacket;
    }

    namespace Meshlets
    {
        const uint32_t maxVerticesPerMeshlet = 64;     // matching wave/warp groups size multiplier
//        const uint32_t maxTrianglesPerMeshlet = 124; // NVidia-recommended 126, rounded down to a multiple of 4
        const uint32_t maxTrianglesPerMeshlet = 64;    // Set it to 64 per inspection of both GPU threads / generated data

        class MeshletsFeatureProcessor;

        //! Per-mesh, per-LOD data shared across all instances of the mesh.
        //! Holds compute resources (one dispatch builds the index buffer for every
        //! instance) and the per-object render SRG (vertex buffers shared by every
        //! instance). Per-instance data lives in MeshletsRenderInstance.
        struct MeshRenderData
        {
            uint32_t MeshletsCount = 0;
            uint32_t IndexCount = 0;
            uint32_t VertexCount = 0;   //!< Total vertex count for bounds validation in the vertex shader.

            //! Compute render data (one dispatch per object per frame).
            Data::Instance<RPI::ShaderResourceGroup> ComputeSrg;
            AZStd::vector<SrgBufferDescriptor> ComputeBuffersDescriptors;
            AZStd::vector<Data::Instance<RPI::Buffer>> ComputeBuffers;
            MeshletsDispatchItem MeshDispatchItem;

            //! Render data shared by every instance (vertex streams, index buffer).
            //! ObjectSrg contains the vertex buffers (positions/normals/...). It is
            //! bound by every instance's DrawPacket.
            Data::Instance<RPI::ShaderResourceGroup> ObjectSrg;
            AZStd::vector<SrgBufferDescriptor> RenderBuffersDescriptors;
            RHI::IndexBufferView IndexBufferView;
            AZStd::vector<Data::Instance<RPI::Buffer>> RenderBuffers;

            //! Phase 4: per-material SRG (SRG_PerMaterial frequency) holding this
            //! mesh's PBR textures + factors, extracted at runtime from the source
            //! model's MaterialAsset. Shared by every instance of this mesh and
            //! bound on the forward DrawItem. Null until EnsureMaterialSrg builds it
            //! (lazily, once the forward shader is loaded). Built once per mesh.
            Data::Instance<RPI::ShaderResourceGroup> MaterialSrg;
            //! True once material resolution has been attempted (success or graceful
            //! fallback to defaults), so we don't re-resolve every frame.
            bool MaterialResolved = false;

            //! Phase 6b (increment 1a): indirect-draw resources. The geometry view
            //! holds a DrawIndirect referencing IndirectArgsView, which references
            //! IndirectArgsBuffer (filled with one DrawIndirectCommand). 1a uses
            //! static args {IndexCount,1,0,0}; later increments have the cull
            //! compute write per-visible-cluster commands + a count buffer.
            Data::Instance<RPI::Buffer> IndirectArgsBuffer;
            RHI::IndirectBufferView IndirectArgsView;
            RHI::GeometryView IndirectGeometryView { RHI::MultiDevice::AllDevices };
            //! Identity index buffer [0,1,...,IndexCount-1] for the DrawIndexedIndirect
            //! path. With indexed draw, SV_VertexID = identity[StartIndexLocation + i] =
            //! StartIndexLocation + i (the linear index), so the existing
            //! m_indices[SV_VertexID] vertex shader works UNCHANGED while per-cluster
            //! commands select their slice via StartIndexLocation. (StartVertexLocation
            //! on a non-indexed draw does NOT offset SV_VertexID in vertex-pull — that
            //! was the C1 failure.)
            Data::Instance<RPI::Buffer> IndexBuffer;
            RHI::IndexBufferView IndexBufferViewRHI;

            //! PERF (hardware input-assembly): a DEDICATED InputAssembly vertex buffer for
            //! POSITION (R32G32B32_FLOAT), holding the same data as the m_positions
            //! StructuredBuffer but bound through the hardware vertex fetcher instead of
            //! 3 scalar SRV loads/vertex. Kept SEPARATE from the SRV stream buffer (AMD
            //! hangs when InputAssembly+ShaderRead share one buffer — see SharedBuffer.cpp).
            //! Added to every geometry view; the depth/shadow/motion pipeline states declare
            //! a matching POSITION-only InputStreamLayout and select it via DrawRequest
            //! stream indices. PositionStreamValid gates whether the IA path is active.
            Data::Instance<RPI::Buffer> PositionIaBuffer;
            RHI::StreamBufferView PositionStreamView;
            bool PositionStreamValid = false;
            //! PERF (hardware input-assembly, FORWARD): dedicated InputAssembly vertex
            //! buffers for the remaining four attributes used by the forward PBR pass,
            //! mirroring PositionIaBuffer exactly (each its own buffer; InputAssembly bind
            //! flag ONLY, never combined with ShaderRead — AMD DEVICE_HUNG otherwise). The
            //! forward PSO declares a 5-channel input layout (POSITION,NORMAL,TANGENT,
            //! BITANGENT,UV) and selects these via DrawRequest stream indices. The
            //! StructuredBuffer SRV copies are kept in the PerObject SRG for cull/debug.
            //! ForwardStreamsValid gates whether the forward IA path is active; it is true
            //! ONLY when POSITION + all four of these allocated successfully (a partial set
            //! would desync the stream indices and hang the forward layout).
            Data::Instance<RPI::Buffer> NormalIaBuffer;
            Data::Instance<RPI::Buffer> TangentIaBuffer;
            Data::Instance<RPI::Buffer> BitangentIaBuffer;
            Data::Instance<RPI::Buffer> UvIaBuffer;
            RHI::StreamBufferView NormalStreamView;
            RHI::StreamBufferView TangentStreamView;
            RHI::StreamBufferView BitangentStreamView;
            RHI::StreamBufferView UvStreamView;
            bool ForwardStreamsValid = false;
            AZStd::vector<AZ::u32> PersistentIdentityIndices;  //!< Backing data for IndexBuffer (kept alive for async upload).
            //! CPU staging for the indirect command list: 4 u32 per DrawIndirectCommand
            //! {vertexCount, instanceCount, startVertex, startInstance}. C1 writes one
            //! command per cluster (all clusters); C2's CPU cull writes only visible ones.
            AZStd::vector<AZ::u32> IndirectArgsData;
            bool IndirectReady = false;

            //! Phase 6 culling: rebased per-cluster descriptors (triangleOffset/Count in
            //! the mesh's expanded-index space) used to build indirect commands, and the
            //! per-cluster bounds (object-space sphere + normal cone) used by the cull
            //! test. Both are persistent CPU copies populated at pack load.
            AZStd::vector<ClusterDescriptor> PersistentClusterDescriptors;
            AZStd::vector<ClusterBoundsRecord> PersistentClusterBounds;

            //! Phase 6 cluster DAG (pack v3): per-cluster cut records + the full
            //! leaf+interior cluster count. For DAG packs, PersistentClusterDescriptors/
            //! Bounds (and the expanded-index slab) cover the FULL DagClusterCount range,
            //! leaves first; MeshletsCount/IndexCount stay leaf-only so whole-mesh draws
            //! never touch interiors. DagClusterCount == 0 => no DAG (v2 pack).
            AZStd::vector<DagNodeRecord> PersistentDagNodes;
            AZ::u32 DagClusterCount = 0;

            //! Phase 7 streaming (pack v4): this mesh's leaf pages. m_clusterFirst is
            //! rebased MESH-LOCAL; payloads are slices of PageData (pack-asset memory,
            //! kept alive by the render object's pack reference — same lifetime
            //! contract as ComputeBuffersDescriptors' m_bufferData pointers).
            AZStd::vector<PageTableRecord> PersistentPageTable;
            //! Per-cluster FIRST-parent index (rebased mesh-local; 0xFFFFFFFF = root),
            //! parallel to PersistentClusterDescriptors. Empty for v2/v3 packs.
            AZStd::vector<AZ::u32> PersistentParentIndex;
            //! The whole pack's PageData section bytes (page payload base).
            AZStd::span<const AZ::u8> PageData;

            //! Phase 3 streaming runtime state (built lazily on first streaming use):
            //! exact simplification-group bookkeeping derived from PersistentParentIndex
            //! (the fail-safe-coarse cut needs group-shared completeness bits), the
            //! cluster->page lookup, and the per-cluster paged map GPU buffer
            //! (recreated on any residency change — the proven initial-data path).
            struct LeafGroup
            {
                AZ::u32 m_parentFirst = 0;    //!< Mesh-local first parent cluster.
                AZ::u32 m_parentCount = 0;
                AZStd::vector<AZ::u32> m_pages;   //!< Page indices covering the group's leaves.
            };
            AZStd::vector<LeafGroup> LeafGroups;
            AZStd::vector<AZ::u32> LeafToGroup;   //!< Leaf cluster -> LeafGroups index (0xFFFFFFFF = rootless leaf).
            AZStd::vector<AZ::u32> ClusterToPage; //!< Every DAG cluster -> page index (0xFFFFFFFF = none).
            AZStd::vector<AZ::u32> InteriorToGroup; //!< (cluster - leafCount) -> LeafGroups index for level-1 parents (0xFFFFFFFF = level-2+).
            bool PagedLookupsBuilt = false;
            Data::Instance<RPI::Buffer> PagedClusterMapBuffer;
            //! True once every always-resident (interior) page is uploaded — the
            //! object SRG's m_pagedMode mirrors this.
            bool PagedModeActive = false;
            //! Phase 4 VRAM-reclaim switch (r_meshletsStreamingExclusive at pack load):
            //! the monolithic geometry buffers (vertex-stream SRVs, expanded-index SRV,
            //! index/IA buffers, MS triangle/indirection copies) are NEVER created for
            //! this mesh — it renders ONLY through the AS-cull paged path. Every other
            //! path sees permanently-not-ready resources and self-skips (the lazy-load
            //! guards). Decided at load; a cvar flip requires a level reload.
            bool MonolithicDropped = false;
            //! GPU copy of PersistentDagNodes (StructuredBuffer<DagNodeRecord>), created
            //! by EnsureCullGpuBuffers alongside the bounds/descriptor buffers.
            Data::Instance<RPI::Buffer> DagNodesBuffer;

            //! Debug coloring: one cluster id per triangle (size IndexCount/3),
            //! bound to the per-object SRG as m_triangleCluster. Lets the forward
            //! shader resolve a meshlet color from the global triangle index,
            //! independent of whether culling is active. Built once at load.
            AZStd::vector<AZ::u32> PersistentTriangleCluster;
            Data::Instance<RPI::Buffer> TriangleClusterBuffer;

            //! Phase 6 GPU-cull groundwork: the per-cluster bounds (sphere + cone)
            //! and descriptors (triangle offset/count) uploaded as GPU
            //! StructuredBuffers, ready to bind to the cull compute SRG
            //! (MeshletsCullSrg). Created lazily by EnsureCullGpuBuffers when the
            //! GPU cull dispatch is wired; null until then (CPU cull doesn't need them).
            Data::Instance<RPI::Buffer> ClusterBoundsBuffer;
            Data::Instance<RPI::Buffer> ClusterDescBuffer;
            bool CullGpuBuffersReady = false;

            //! Phase 5 (hardware mesh shader): per-mesh resources for the DispatchMesh
            //! render path. The cluster triangle words + vertex indirection are uploaded
            //! as StructuredBuffer<uint> copies (from the pack-owned slices recorded in
            //! ComputeBuffersDescriptors — the compute path's typed Buffer<uint> SRVs are
            //! unproven on this AMD GPU, see the typed-buffer note above). The object SRG
            //! is MeshletsMeshObjectSrg (cluster + vertex-stream data, mesh-shader shader
            //! layout); the geometry view carries DispatchMeshDirect{clusterCount,1,1}
            //! (one threadgroup per cluster) and no index/stream buffers. Built lazily by
            //! MeshletsFeatureProcessor::EnsureMeshShaderResources.
            Data::Instance<RPI::Buffer> MeshShaderTrianglesBuffer;
            Data::Instance<RPI::Buffer> MeshShaderIndirectionBuffer;
            Data::Instance<RPI::ShaderResourceGroup> MeshShaderObjectSrg;
            RHI::GeometryView MeshShaderGeometryView { RHI::MultiDevice::AllDevices };
            bool MeshShaderResourcesReady = false;
            //! Which mesh-shader PSO MeshShaderObjectSrg was built for. azslc runs with
            //! --strip-unused-srgs, so the uncull and AS-cull shaders have DIFFERENT
            //! MeshletsMeshObjectSrg layouts (m_clusterBounds is stripped from the uncull
            //! one — only the AS reads it). An SRG must therefore be created from the SAME
            //! shader asset as the PSO it is bound to; flipping r_meshletsMsCullAS forces a
            //! rebuild rather than sharing one instance across both.
            bool MeshShaderSrgBuiltForCull = false;

            //! Phase 5 AS/triangle cull (opt-in r_meshletsMsCullAS): DispatchMesh
            //! geometry view for the AMPLIFICATION-shader PSO. Carries the OUTER AS
            //! group count -- ceil(clusterCount / MESHLETS_AS_GROUP) -- NOT clusterCount
            //! directly like MeshShaderGeometryView; the AS itself launches per-survivor
            //! MS groups via the DispatchMesh() HLSL intrinsic. Built alongside
            //! MeshShaderGeometryView by EnsureMeshShaderResources (reuses the same
            //! MeshShaderObjectSrg, which also carries m_clusterBounds for the AS cull test).
            RHI::GeometryView MeshShaderCullGeometryView { RHI::MultiDevice::AllDevices };

            //! Two-level cull: a conservative object-space bounding sphere for the WHOLE
            //! mesh (union of cluster bounds). The per-instance frustum test against this
            //! decides whether to skip the instance (fully outside), draw it whole-mesh
            //! with no per-cluster cull (fully inside — nothing to frustum-cull, so the
            //! compaction compute is pure overhead), or run the GPU cluster cull (straddles
            //! the frustum). MeshBoundsRadius < 0 means "not computed yet".
            AZ::Vector3 MeshBoundsCenter = AZ::Vector3::CreateZero();
            float MeshBoundsRadius = -1.0f;

            //! Geometric-error LOD metric (SectionKind::LodError): meshopt_simplify's
            //! resultError for this (mesh, LOD) — already scale-independent ("relative
            //! to mesh extents", dimensionless [0,1], per meshoptimizer's own docs).
            //! 0.0 for LOD0 and for source-supplied/"baked" LODs. HasLodError is false
            //! for packs built before builder v9 (section absent); the LOD selector
            //! falls back to screen-coverage in that case.
            float LodGeometricError = 0.0f;
            bool HasLodError = false;

            //! SP1: persistent CPU-baked data for the m_indices and m_uvs
            //! buffers. Stored as members (not local vectors in PackInit)
            //! so the pointers we hand to SrgBufferDescriptor.m_bufferData
            //! remain valid through CreateAndBindRenderBuffers and the
            //! post-creation UpdateData calls. The vertex stream buffers
            //! (positions/normals/...) point m_bufferData at pack-asset
            //! memory which persists for the object's lifetime; without
            //! these members our indices/UVs were pointing at PackInit-local
            //! vectors that go out of scope before UpdateData runs.
            AZStd::vector<AZ::u32> PersistentExpandedIndices;
            AZStd::vector<float> PersistentExpandedUVs;
        };
        using ModelLodDataArray = AZStd::vector<MeshRenderData*>;    // MeshRenderData per mesh in the Lod

        //! Per-instance render data. Many MeshletsRenderInstances can reference
        //! the same MeshletsRenderObject; each gets its own ObjectId, transform,
        //! per-draw SRG (holding the ObjectId), and DrawPacket.
        struct MeshletsRenderInstance
        {
            class MeshletsRenderObject* RenderObject = nullptr;
            uint32_t LodIndex = 0;
            uint32_t MeshIndex = 0;

            // ---- Per-frame screen-coverage LOD selection (hysteresis state) ----
            //! The LOD the coverage heuristic currently wants. When it differs from
            //! LodIndex it must persist for LodPendingFrames consecutive frames (the
            //! deadband below) before LodIndex actually changes — this stops an
            //! instance hovering on a LOD boundary from rebuilding its instance group
            //! every frame (per-frame group churn hazard).
            uint32_t LodPendingIndex = 0;
            //! Consecutive frames LodPendingIndex has wanted the same (non-current) LOD.
            uint32_t LodPendingFrames = 0;
            Render::TransformServiceFeatureProcessorInterface::ObjectId ObjectId;
            Data::Instance<RPI::ShaderResourceGroup> InstanceSrg;
            //! Phase 5 (hardware mesh shader): per-instance SRG for the mesh-shader path
            //! (MeshletsMeshInstanceSrg — carries m_objectId). Created from the mesh-shader
            //! shader asset; independent of InstanceSrg (different SRG layout).
            Data::Instance<RPI::ShaderResourceGroup> MeshShaderInstanceSrg;
            //! Which mesh-shader PSO MeshShaderInstanceSrg was built for — same
            //! stripped-layout rule as MeshRenderData::MeshShaderSrgBuiltForCull. The
            //! uncull MS references only m_objectId, so every AS cull field
            //! (m_frustumPlanes/m_worldToClip/m_cameraPosition/m_do*Cull/m_worldRow*/
            //! m_hiZTexture) is stripped from its layout; UpdateMeshShaderCullInstance
            //! silently no-ops against such an SRG.
            bool MeshShaderInstanceSrgBuiltForCull = false;
            //! Uncull-layout MeshletsMeshInstanceSrg for the mesh-shader SHADOW packet
            //! when r_meshletsMsCullAS is active: the shadow MS is the UNCULL shader
            //! (a light's view must rasterize every cluster), so it cannot bind the
            //! cull-layout MeshShaderInstanceSrg above. Unused (null) when the camera
            //! packet is uncull too — MeshShaderInstanceSrg is shared then.
            Data::Instance<RPI::ShaderResourceGroup> MeshShadowInstanceSrg;
            //! Owning ref to the DrawPacket. DrawPacketBuilder::End() returns an
            //! RHI::Ptr; if we stored a raw pointer instead, the temporary Ptr would
            //! free the packet immediately and leave a dangling pointer (causing
            //! random Invalid-position crashes in DrawListContext::AddDrawPacket).
            //! This is the CAMERA packet (depth/forward/motion). With CPU culling on,
            //! it is rebuilt each frame from the culled command set; otherwise built once.
            RHI::Ptr<RHI::DrawPacket> DrawPacket;
            //! Shadow packet (shadow DrawItem only), always the full cluster set
            //! (un-culled) so shadows stay correct regardless of the camera. Built once.
            RHI::Ptr<RHI::DrawPacket> ShadowDrawPacket;
            //! Phase 6 occlusion-safe depth: under the AS-cull path the depth prepass
            //! item lives in its OWN packet with its own instance SRG (MeshDepthInstanceSrg)
            //! so the depth draw NEVER applies HiZ occlusion — a HiZ-culled depth prepass
            //! would poison the next frame's pyramid (false-culls feed back). Null when
            //! the depth item rides the camera packet (non-AS path).
            RHI::Ptr<RHI::DrawPacket> DepthDrawPacket;
            //! Cull-layout instance SRG for DepthDrawPacket: same constants as
            //! MeshShaderInstanceSrg each frame EXCEPT m_doHiZCull stays 0 — unless
            //! two-pass occlusion is active, when it becomes PASS 1 (prev-frame HiZ +
            //! visMode 1, safe because the late pass completes the depth).
            Data::Instance<RPI::ShaderResourceGroup> MeshDepthInstanceSrg;

            //! Two-pass occlusion PASS 2 (opt-in r_meshletsTwoPassOcclusion): the
            //! late-depth packet ("meshletslatedepth" tag → MeshletsLateDepthPass,
            //! after this frame's HiZ reduce) + its cull-layout instance SRG (visMode
            //! 2, THIS frame's pyramid) + the per-cluster visibility ledger (one u32
            //! frame-id per DAG-range cluster; frame-counter encoding, never cleared).
            RHI::Ptr<RHI::DrawPacket> LateDepthDrawPacket;
            Data::Instance<RPI::ShaderResourceGroup> MeshLateInstanceSrg;
            Data::Instance<RPI::Buffer> VisFrameBuffer;
            AZ::Name VisFrameAttachmentId;
            //! Which layout MeshShadowInstanceSrg was built with: false = uncull shadow
            //! shader (plain leaf shadow packet), true = the AS-culled shadow shader
            //! (Phase 6 shadow-side DAG cut). Rebuilt on mismatch.
            bool MeshShadowInstanceSrgIsCull = false;

            // ---- Phase 6 CPU cluster culling (per-instance) ----
            //! Triple-buffered indirect-args (DrawIndexedIndirect commands) for the
            //! culled camera draw. RingBuffer avoids the async-UpdateData frame-overlap
            //! hazard (the AMD root-cause) by rotating buffers per frame.
            AZStd::unique_ptr<RPI::RingBuffer> CullArgsRing;
            RHI::IndirectBufferView CullArgsView;
            RHI::GeometryView CameraGeometryView { RHI::MultiDevice::AllDevices };
            AZStd::vector<AZ::u32> CullCommandStaging;  //!< 5 u32 per visible cluster.
            uint32_t LastVisibleClusters = 0;
            uint32_t LastCulledClusters = 0;
            bool CullResourcesReady = false;
            //! Optimization: the world transform used for the last cull. If neither it
            //! nor the camera changed, the cull (and per-frame packet rebuild) is skipped.
            AZ::Transform LastCullTransform = AZ::Transform::CreateIdentity();
            bool HasLastCullTransform = false;

            // ---- Phase 6 GPU cluster culling (per-instance) ----
            //! Per-instance cull compute SRG (MeshletsCullSrg): binds the mesh-shared
            //! bounds/descriptor buffers + this instance's output args buffer, and
            //! carries the per-frame cull params (transform, frustum planes, flags).
            Data::Instance<RPI::ShaderResourceGroup> CullSrg;
            //! Per-cluster (surfel) cull output. The compute appends ONE
            //! DrawIndexedIndirect command (5 u32) per VISIBLE cluster into CullCommandBuffer
            //! (sized clusterCount), and writes the visible-command count into CullCountBuffer.
            //! The instance is drawn with a single DrawIndexedIndirectCount over the mesh's
            //! STATIC index buffer — each command renders one visible cluster's slice, with
            //! full vertex-cache reuse. Both UAV-written by the compute and consumed as
            //! indirect args; dedicated per-instance (the frame-graph UAV->Indirect barrier +
            //! single-queue in-order execution handle cross-frame hazards). No compacted
            //! index buffer exists, so there is no UAV->SRV index sync to get wrong.
            Data::Instance<RPI::Buffer> CullCommandBuffer;
            Data::Instance<RPI::Buffer> CullCountBuffer;
            RHI::IndirectBufferView CullArgsGpuView;   //!< over CullCommandBuffer (DrawIndexed signature)
            RHI::GeometryView GpuCullGeometryView { RHI::MultiDevice::AllDevices };
            //! One cull dispatch (one group) bound to CullSrg. Built once; each frame only
            //! the SRG constants are updated, and the UAVs are bound + compiled in the pass.
            MeshletsDispatchItem CullDispatchItem;
            //! Stable frame-graph attachment ids (compute writes ReadWrite, barrier pass
            //! declares Indirect).
            AZ::Name CullArgsAttachmentId;    //!< CullCommandBuffer
            AZ::Name CullCountAttachmentId;   //!< CullCountBuffer
            uint32_t GpuCullClusterCount = 0;
            bool GpuCullResourcesReady = false;
            //! True on frames this instance runs the GPU cluster cull (visible). When false
            //! it is skipped (fully off-screen). BuildInstanceDrawPacket reads it to pick the
            //! culled (DrawIndexedIndirectCount) vs whole-mesh geometry view.
            bool GpuCullDrawActive = false;
        };

        //! Currently assuming single model without Lods so that the handling of the
        //! meshlet creation and handling of the array is easier. If several meshes or Lods
        //! exist, they will be created as separate models and the last model's instance
        //! will be kept in this class.
        //! To enhance this, add inheritance to lower levels of the model / mesh.
        //! MeshletsModel represents a combined model that can contain an array
        //! of ModelLods.
        //! Each one of the ModelLods contains a vector of meshes, representing possible multiple
        //! element within the mesh.
        class MeshletsRenderObject
        {
        public:
            static uint32_t s_modelNumber;

            Name s_textureCoordinatesName;
            Name s_indicesName;

            // NEW — pack-driven constructor. Coexists with the old one until Phase 5.
            MeshletsRenderObject(Data::Asset<RPI::ModelAsset> sourceModelAsset,
                                 Data::Asset<AZ::Meshlets::MeshletPackAsset> packAsset,
                                 MeshletsFeatureProcessor* meshletsFeatureProcessor);

            ~MeshletsRenderObject();

            static  Data::Instance<RPI::ShaderResourceGroup> CreateShaderResourceGroup(
                Data::Instance<RPI::Shader> shader,
                const char* shaderResourceGroupId,
                [[maybe_unused]] const char* moduleName
            );

            AZStd::string& GetName() { return m_name;  }

            ModelLodDataArray& GetMeshletsRenderData(uint32_t lodIdx = 0)
            {
                AZ_Assert(m_modelRenderData.size(), "Meshlets - model does not contain any render data");
                return m_modelRenderData[AZStd::min(lodIdx, (uint32_t)m_modelRenderData.size() - 1)];
            }

            //! Number of LOD levels this object loaded (== m_modelRenderData outer
            //! size). 1 for a stale single-LOD pack; up to K for a multi-LOD pack.
            //! Per-frame LOD selection clamps its chosen LOD into [0, count-1].
            uint32_t GetLodCount() const
            {
                return static_cast<uint32_t>(m_modelRenderData.size());
            }


            // This method is binding the buffers to the Srg and is separated from
            // the creation method to allow frame sync when the data is compiled
            static bool CreateAndBindComputeSrgAndDispatch(Data::Instance<RPI::Shader> computeShader, MeshRenderData& meshRenderData);

            uint32_t GetMeshletsCount() { return m_meshletsCount; }

            // The prep of this data should be used to create the shared buffer alignment
            static void PrepareRenderSrgDescriptors(MeshRenderData &meshRenderData, uint32_t vertexCount, uint32_t indicesCount);

            //! Phase 4: lazily resolve the source model's material for \p meshIndex and
            //! build the per-mesh MeshletsMaterialSrg (baseColor/normal/metallic/
            //! roughness maps + factors) used by the forward PBR shader. Idempotent —
            //! does nothing if already resolved. Requires the forward shader (for the
            //! SRG layout). Returns true if MaterialSrg is ready to bind (including the
            //! graceful default-material fallback); false only if it should be retried
            //! later (e.g. the forward shader isn't loaded yet).
            bool EnsureMaterialSrg(uint32_t meshIndex, const Data::Instance<RPI::Shader>& forwardShader);

            //! The material SRG resolved on LOD0 for \p meshIndex (null if not yet
            //! resolved / out of range). Materials are slot-shared across LODs, so a
            //! draw of any LOD of this mesh binds the SAME SRG — callers building a
            //! LOD>0 packet fetch it here rather than re-resolving per LOD. Call
            //! EnsureMaterialSrg first to populate it.
            Data::Instance<RPI::ShaderResourceGroup> GetMaterialSrgForMesh(uint32_t meshIndex) const
            {
                if (m_modelRenderData.empty() || meshIndex >= m_modelRenderData[0].size())
                {
                    return nullptr;
                }
                const MeshRenderData* lod0 = m_modelRenderData[0][meshIndex];
                return lod0 ? lod0->MaterialSrg : nullptr;
            }

            //! Phase 6b (increment 1a): lazily build this mesh's indirect-draw
            //! geometry view (a static single DrawIndirectCommand {IndexCount,1,0,0}
            //! in an Indirect-pool buffer, referenced via an IndirectBufferView built
            //! against \p signature). Idempotent. Returns true once IndirectGeometryView
            //! is ready to use; false if it should be retried (e.g. signature null).
            bool EnsureIndirectArgs(MeshRenderData& meshRenderData, const RHI::IndirectBufferSignature* signature);

            Data::Asset<RPI::ModelAsset> GetSourceModelAsset() const { return m_sourceModelAsset; }

            //! Phase 6 CPU cull: test this mesh's clusters against the camera frustum
            //! (per-cluster bounding sphere) and view direction (normal cone), and
            //! append a DrawIndexedIndirect command (5 u32: indexCount, instanceCount,
            //! startIndex, baseVertex, startInstance) per VISIBLE cluster to outCommands.
            //! Returns the visible cluster count; outCulled gets the rejected count.
            //! Falls back to "all visible" if the pack lacks ConeBounds.
            uint32_t CullClustersToCommands(
                uint32_t meshIndex,
                const AZ::Frustum& frustum,
                const AZ::Vector3& cameraPos,
                const AZ::Matrix4x4& objectToWorld,
                bool doFrustumCull, bool doConeCull,
                AZStd::vector<AZ::u32>& outCommands,
                uint32_t& outCulled) const;

            //! Debug: set the per-cluster coloring flag on every mesh's per-object
            //! SRG (re-binding + recompiling the SRG constant). Cheap and only
            //! called when the toggle actually changes.
            void SetMeshletDebugColor(bool enabled);

            //! Phase 6 GPU cull: upload this mesh's cluster bounds + descriptors to
            //! GPU StructuredBuffers (ClusterBoundsBuffer / ClusterDescBuffer) for
            //! the cull kernels (MeshletsCull.azsl compute + the AS cull path).
            //! Idempotent. Returns false if the persistent cluster data is missing.
            bool EnsureCullGpuBuffers(MeshRenderData& meshRenderData);

            //! Two-level cull: lazily compute the conservative whole-mesh bounding sphere
            //! (MeshBoundsCenter/Radius) from the cluster bounds. Idempotent; cheap. Used
            //! by the per-instance frustum test that decides skip / whole-mesh / cluster-cull.
            static void EnsureMeshBounds(MeshRenderData& meshRenderData);

        protected:
            bool BuildDrawPacket( RHI::DrawPacketBuilder::DrawRequest& drawRequest, MeshRenderData& meshRenderData);

            bool CreateAndBindRenderBuffers(MeshRenderData &meshRenderData);


            bool SetShaders();

        private:
            MeshletsFeatureProcessor* m_featureProcessor = nullptr;
            AZStd::string m_name;
            Data::Asset<RPI::ModelAsset> m_sourceModelAsset;
            //! Held to keep MeshletPackAsset::m_bytes alive for the render object's
            //! lifetime — the SrgBufferDescriptors and BufferViews built by the
            //! pack-driven constructor reference into the pack's byte buffer. Without
            //! this strong ref, the pack asset's destructor would free m_bytes while
            //! the runtime is still reading from it via the bound vertex streams.
            Data::Asset<AZ::Meshlets::MeshletPackAsset> m_packAsset;

            Aabb m_aabb;    // Should be per Lod per mesh and not global

            static AZStd::vector<SrgBufferDescriptor> m_srgBufferDescriptors;

            uint32_t m_meshletsCount = 0;

            //------------------------------------------------------------------
            // Remarks:
            // 1. Moving to indirect compute, all the buffer views will need to either
            // become offsets passed as part of each mesh dispatch, or bindless resources.
            // Having the first approach does not require bindless mechanism in place.  
            //------------------------------------------------------------------
            Data::Instance<RPI::Shader> m_renderShader;
            Data::Instance<RPI::Shader> m_computeShader;

            AZStd::vector<ModelLodDataArray> m_modelRenderData;         // Render data array of Lods.
        };

    } // namespace Meshlets
} // namespace AZ
