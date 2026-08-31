/*
* Modifications Copyright (c) Contributors to the Open 3D Engine Project. 
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
* 
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/parallel/thread.h>

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>

#include <AzCore/Component/TickBus.h>

#include <AtomCore/Instance/Instance.h>
#include <Atom/RPI.Public/FeatureProcessor.h>

#include <Atom/RHI/IndirectBufferSignature.h>
#include <Atom/RHI/PipelineStateDescriptor.h>
#include <Atom/RHI.Reflect/InputStreamLayout.h>

#include <AzCore/Math/Frustum.h>
#include <Atom/RPI.Public/Pass/GpuDriven/HiZGeneratePass.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Vector2.h>
#include <MeshletsPageResidency.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzCore/std/containers/map.h>

#include <Atom/RHI/GeometryView.h>

#include <Atom/Feature/TransformService/TransformServiceFeatureProcessorInterface.h>

#include <Meshlets/MeshletsFeatureProcessorInterface.h>
#include <Meshlets/PackResolver.h>

#include <SharedBuffer.h>
#include <MultiDispatchComputePass.h>
#include <MeshletsRenderPass.h>

namespace AZ
{
    namespace RPI
    {
        class RenderPipeline;
    }

    namespace Meshlets
    {
        class MeshletsRenderObject;

        class MeshletsFeatureProcessor final
            : public MeshletsFeatureProcessorInterface
            , private AZ::TickBus::Handler
        {
            Name MeshletsComputePassName;
            Name MeshletsRenderPassName;
            //! GPU cull: the early (pre-DepthPrePass) compute + barrier passes.
            Name MeshletsCullComputePassName;
            Name MeshletsCullBarrierPassName;

        public:
            AZ_CLASS_ALLOCATOR(MeshletsFeatureProcessor, AZ::SystemAllocator)
            AZ_RTTI(MeshletsFeatureProcessor, "{1D93DE27-2DC4-4E9B-90B3-DCDCB941C920}", MeshletsFeatureProcessorInterface);
            AZ_FEATURE_PROCESSOR(MeshletsFeatureProcessor);

            static void Reflect(AZ::ReflectContext* context);

            MeshletsFeatureProcessor();
            virtual ~MeshletsFeatureProcessor();

            void Init(RPI::RenderPipeline* pipeline);

            // FeatureProcessor overrides ...
            void Activate() override;
            void Deactivate() override;
            void AddRenderPasses(RPI::RenderPipeline* renderPipeline) override;
            void Simulate(const FeatureProcessor::SimulatePacket& packet) override;
            void Render(const FeatureProcessor::RenderPacket& packet) override;

            bool InitComputePass(const Name& passName);
            bool InitDepthShader();
            bool InitRenderPass(const Name& passName);
            bool InitShadowShader();
            bool InitForwardShader();
            bool InitMotionShader();
            //! Phase 5 (hardware mesh shader): load MeshletsForwardMeshShader (Mesh +
            //! Fragment entry points) and acquire its DispatchMesh pipeline state.
            //! Mirrors InitForwardShader but with NO input-stream layout (the mesh
            //! path has no input assembler). Safe no-op retry if the shader asset
            //! hasn't been processed yet.
            bool InitMeshForwardShader();

            //! Phase 5 AS/triangle cull (opt-in r_meshletsMsCullAS): load
            //! MeshletsForwardMeshShaderCulled (Amplification + Mesh + Fragment entry
            //! points) and acquire its DispatchMesh pipeline state. A SEPARATE
            //! shader/PSO from m_meshForwardShader -- see the comment in
            //! MeshletsForwardMeshShaderCulled.shader for why. Safe no-op retry if the
            //! shader asset hasn't been processed yet.
            bool InitMeshCullForwardShader();

            //! Hardware mesh-shader DEPTH prepass: load MeshletsDepthMeshShader (a
            //! single Mesh entry, no Fragment) and acquire its DispatchMesh pipeline
            //! state for the standard "depth" DrawListTag. Without this item the mesh
            //! path never reaches MainPipeline's once-per-frame resolved Depth copy,
            //! so DepthOfField/Transparent sample stale depth and the meshlets read
            //! as see-through. Safe no-op retry if the shader asset isn't processed yet.
            bool InitMeshDepthShader();

            //! Hardware mesh-shader MOTION VECTOR pass. Needed for correctness, not only speed:
            //! motion DrawItems are added by BuildInstanceDrawPacket, which the mesh-shader path
            //! short-circuits, so before this a mesh-shader meshlet emitted NO motion vectors and
            //! ghosted under TAA and any temporal upscaler.
            bool InitMeshMotionShader();

            //! AS-culled DEPTH and MOTION PSOs (opt-in r_meshletsMsCullAS): the culled
            //! forward packet binds ONE geometry view (AS group counts), so its depth and
            //! motion items need PSOs that carry the SAME shared cluster-cull AS
            //! (MeshletsCullAS.azsli) rather than the plain Mesh-only PSOs. Shared retry
            //! helper — safe no-op until the shader asset is processed.
            bool InitCulledMeshVariant(
                const char* shaderPath, const char* label,
                Data::Instance<RPI::Shader>& shaderOut, const RHI::PipelineState*& pipelineStateOut);

            //! Writes the per-instance AS cull constants (world rows, frustum planes,
            //! camera, toggles) WITHOUT compiling — callers decide when to Compile().
            void WriteMeshShaderCullConstants(
                const Data::Instance<RPI::ShaderResourceGroup>& srg, const AZ::Frustum& frustum,
                const AZ::Vector3& cameraPos, const AZ::Matrix4x4& objectToWorld, bool dagCutActive);

            //! Hardware mesh-shader SHADOW pass. Non-culling by design: a light's view must
            //! rasterize every cluster, including ones back-facing the camera, so this uses the
            //! plain all-clusters geometry view rather than the AS-culled one.
            bool InitMeshShadowShader();

            //! Phase 6b: one-time creation of the indirect-draw command signature
            //! (a single non-indexed Draw command). Shared by every meshlet
            //! instance's indirect DrawItems. Idempotent.
            bool InitIndirectDrawSignature();

            //! Accessor so the per-mesh indirect-args setup can build its
            //! IndirectBufferView against the shared signature.
            const RHI::IndirectBufferSignature* GetDrawIndirectSignature() const
            {
                return m_drawIndirectSignature.get();
            }

            // AZ::TickBus::Handler overrides
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
            int GetTickOrder() override;

            void SetTransform(
                const Render::TransformServiceFeatureProcessorInterface::ObjectId objectId,
                const AZ::Transform& transform);

            //! Adds a new instance of \p meshletsRenderObject. The render object holds the
            //! shared geometry; multiple instances share its compute output and vertex
            //! buffers. Each call here produces a separate ObjectId, per-draw SRG, and
            //! DrawPacket.
            //! Returns nullptr on failure. The returned pointer is owned by the feature
            //! processor; pass it back to RemoveInstance to release it.
            MeshletsRenderInstance* AddInstance(MeshletsRenderObject* meshletsRenderObject);

            //! Releases a single instance. Does not affect other instances of the same
            //! render object.
            void RemoveInstance(MeshletsRenderInstance* instance);

            //! Backwards-compatible wrapper that adds the render object to the scene with
            //! exactly one instance. Returns the new instance's ObjectId. Prefer
            //! AddInstance() for new code that wants explicit instance handles.
            Render::TransformServiceFeatureProcessorInterface::ObjectId AddMeshletsRenderObject(
                MeshletsRenderObject* meshletsRenderObject);

            //! Removes the render object's first registered instance. Prefer RemoveInstance().
            void RemoveMeshletsRenderObject(MeshletsRenderObject* meshletsRenderObject);

            // ---- MeshletsFeatureProcessorInterface ----
            // Cross-gem opaque API used by clients (e.g. MeshComponent's "Use Meshlets"
            // toggle). Internally builds-or-reuses a MeshletsRenderObject keyed by
            // ModelAsset id, then adds a per-call instance.
            InstanceHandle AcquireInstance(const Data::Asset<RPI::ModelAsset>& modelAsset) override;
            void ReleaseInstance(InstanceHandle handle) override;
            void SetInstanceTransform(InstanceHandle handle, const AZ::Transform& worldTransform) override;

            //! Phase 7 streaming stats (debug ImGui).
            uint32_t GetStreamingResidentPages() const { return m_streamingResidentPages; }
            uint32_t GetStreamingTrackedPages() const { return m_streamingTrackedPages; }
            uint32_t GetStreamingSlotCapacity() const { return m_pageResidency.GetSlotCapacity(); }
            uint32_t GetStreamingLoadsThisFrame() const { return m_streamingLoadsThisFrame; }
            uint32_t GetStreamingEvictsThisFrame() const { return m_streamingEvictsThisFrame; }
            uint64_t GetStreamingChurnCount() const { return m_pageResidency.GetChurnCount(); }
            uint32_t GetStreamingStarvedPages() const { return m_streamingStarvedPages; }

            Data::Instance<RPI::Shader> GetComputeShader() { return m_computeShader; }
            Data::Instance<RPI::Shader> GetRenderShader() { return m_renderShader; }

            // ---- Debug (ImGui) API ----
            //! Per-render-object debug info row.
            struct DebugObjectInfo
            {
                AZStd::string m_name;
                uint32_t m_clusters = 0;
                uint32_t m_triangles = 0;
                uint32_t m_vertices = 0;
                uint32_t m_lods = 0;
                uint32_t m_instances = 0;
                bool     m_materialResolved = false;
            };
            //! Snapshot of meshlet system state for the debug tab. Computed on demand
            //! from the main thread (OnImGuiUpdate).
            struct DebugStats
            {
                uint32_t m_renderObjectCount = 0;
                uint32_t m_instanceCount = 0;
                uint64_t m_totalClusters = 0;
                uint64_t m_totalTriangles = 0;
                uint64_t m_totalVertices = 0;
                bool m_depthActive = false;
                bool m_shadowActive = false;
                bool m_forwardActive = false;
                bool m_motionActive = false;
                bool m_indirectActive = false;
                // ---- LOD debug (Step C) ----
                //! Max LOD slots available across loaded packs (1 == no LOD hierarchy
                //! baked; a stale single-LOD pack). >1 means LODs are present.
                uint32_t m_maxLodCount = 1;
                //! Instances currently resolved to each LOD (index = LOD, [0..7]).
                AZStd::array<uint32_t, 8> m_lodHistogram = {};
                //! Vertices actually submitted this query given each instance's CURRENT
                //! LOD, vs if every instance drew LOD0. The ratio is the LOD vertex win.
                uint64_t m_renderedVertices = 0;   //!< sum over instances of selected-LOD vert count
                uint64_t m_fullVertices = 0;       //!< sum over instances of LOD0 vert count
                AZStd::vector<DebugObjectInfo> m_objects;
            };
            //! Runtime debug toggles edited by the ImGui tab. Pass-enable / shader
            //! changes require a DrawPacket rebuild (call InvalidateAllDrawPackets).
            struct DebugControls
            {
                bool m_depthPassEnabled   = true;
                bool m_shadowPassEnabled  = true;
                bool m_forwardPassEnabled = true;
                bool m_motionPassEnabled  = true;
                bool m_useDebugShader     = false;  //!< Force the UV debug shader over PBR forward.
                //! Debug: flat per-cluster (meshlet) coloring on the forward pass.
                //! Cull-independent — visualizes the meshlet decomposition directly.
                bool m_meshletColorMode   = false;
                // ---- LOD debug (Step C) ----
                //! Force every instance to this LOD, overriding screen-coverage selection.
                //! -1 = auto (screen-coverage, the shipping behaviour). 0..K-1 forces that
                //! LOD (clamped per-instance to what its pack actually has) — the key A/B
                //! tool: force LOD0 vs a coarse LOD and compare GPU pass times + visuals.
                int m_forceLodIndex = -1;
                // Cluster culling — opt-in (off by default so the proven whole-mesh
                // path is untouched until toggled on in the debug tab).
                bool m_cullEnabled  = false;
                bool m_frustumCull  = true;
                bool m_coneCull     = true;
                //! Cull on the GPU (compute) instead of the CPU. When true and
                //! m_cullEnabled, a compute pass culls clusters and writes the
                //! indirect-args buffer the raster passes draw from. The CPU path is
                //! untouched (still the default when this is false).
                bool m_gpuCull      = false;
                //! Per-cluster HiZ occlusion cull (GPU path only). Additional opt-in on
                //! top of m_cullEnabled+m_gpuCull: even when true, the shader-side
                //! dimension check makes this a safe no-op until a pipeline actually
                //! wires MeshletsCullSrg::m_hiZTexture to a HiZ pyramid (not done yet --
                //! see MeshletsFeatureProcessor.cpp UpdateGpuCullInstance).
                bool m_hiZCull      = false;
                //! Freeze the cull camera at its current pose: keep culling against the
                //! frozen frustum/position while you fly the real camera around, so you
                //! can see exactly which clusters were culled.
                bool m_freezeCullCamera = false;
                // Cull stats (read-only, updated by the cull each frame):
                uint64_t m_visibleClusters = 0;
                uint64_t m_culledClusters  = 0;
            };
            DebugStats GetDebugStats() const;
            DebugControls& GetDebugControls() { return m_debugControls; }

            //! Null every live DrawPacket so they are rebuilt on the next Render().
            //! Call this whenever the pipeline state changes (shader reload, pass
            //! reconstruction) — DrawPackets hold raw pointers to the pipeline state
            //! and SRGs, so they become dangling when those objects are freed.
            void InvalidateAllDrawPackets();

            PackResolutionStatus GetPackStatus(const AZ::Data::AssetId& modelAssetId) const override;

        protected:
            // RPI::SceneNotificationBus overrides ...
            void OnRenderPipelineChanged(RPI::RenderPipeline* pipeline, RPI::SceneNotification::RenderPipelineChangeType changeType) override;

            bool BuildInstanceDrawPacket(MeshletsRenderInstance& instance, MeshRenderData& meshRenderData);

            // ---- Phase 5: hardware mesh-shader render path (r_meshletsHwMeshShader) ----
            //! Lazily build the per-mesh mesh-shader resources: the triangle-word +
            //! vertex-indirection StructuredBuffers (copied from the pack-owned slices in
            //! ComputeBuffersDescriptors), the MeshletsMeshObjectSrg (cluster descriptors +
            //! vertex streams + constants), and the DispatchMesh geometry view (one
            //! threadgroup per cluster). Requires EnsureCullGpuBuffers to have created
            //! ClusterDescBuffer first. Idempotent.
            //! \param forCull build the per-mesh SRG from the AS-cull shader's layout
            //!        (which retains m_clusterBounds) instead of the uncull shader's.
            //!        The two layouts differ because azslc strips unused SRG entries, so
            //!        an SRG must be created from the same asset as the PSO it binds to.
            bool EnsureMeshShaderResources(MeshRenderData& meshRenderData, bool forCull);
            //! Build the instance's camera packet for the hardware mesh-shader path: a
            //! single forward DrawItem whose GeometryView carries DispatchMeshDirect
            //! {clusterCount,1,1} (no index buffer, no IA streams). First slice renders
            //! the flat per-cluster debug color; shadow/depth/motion items are not
            //! emitted (ShadowDrawPacket is cleared). Returns false to fall back to the
            //! vertex-pull path (unsupported device, shader not ready, resources missing).
            bool BuildMeshShaderDrawPacket(MeshletsRenderInstance& instance, MeshRenderData& meshRenderData);

            //! Phase 5 AS/triangle cull (opt-in r_meshletsMsCullAS, per frame): refresh
            //! the instance's MeshShaderInstanceSrg frustum/world-row/camera/toggle
            //! constants (the AS-only fields; harmless no-op when the non-cull PSO is
            //! active for this instance) and recompile. Cheap constant-only update --
            //! does NOT rebuild the DrawPacket. m_doHiZCull is always left 0 (no HiZ
            //! pyramid wired to the AS in this slice).
            void UpdateMeshShaderCullInstance(
                MeshletsRenderInstance& instance, const AZ::Frustum& frustum,
                const AZ::Vector3& cameraPos, const AZ::Matrix4x4& objectToWorld);

            // ---- Step B: hardware instancing (cull-off default path) ----
            //! Key identifying instances that share one MeshRenderData (one model+lod+
            //! mesh). All members are drawn by a single hardware-instanced DrawIndexed.
            struct InstanceGroupKey
            {
                MeshletsRenderObject* m_renderObject = nullptr;
                uint32_t m_lodIndex = 0;
                uint32_t m_meshIndex = 0;
                bool operator<(const InstanceGroupKey& o) const
                {
                    if (m_renderObject != o.m_renderObject) return m_renderObject < o.m_renderObject;
                    if (m_lodIndex != o.m_lodIndex) return m_lodIndex < o.m_lodIndex;
                    return m_meshIndex < o.m_meshIndex;
                }
            };
            //! Per-group GPU + draw state for the hardware-instanced (cull-off) path.
            //! Rebuilt only when the group is dirty (membership changed); transforms
            //! come from SceneSrg by objectId and are always current (never a rebuild
            //! trigger). One DrawIndexedInstanced per group per pass replaces the N
            //! per-instance packets.
            struct InstanceGroup
            {
                //! Live members, in stable order; SV_InstanceID i -> members[i]'s objectId.
                AZStd::vector<MeshletsRenderInstance*> m_members;
                //! StructuredBuffer<uint> (ShaderRead) of member objectIds, member order.
                Data::Instance<RPI::Buffer> m_objectIdBuffer;
                //! CPU staging for the objectId buffer (kept alive for async upload).
                AZStd::vector<AZ::u32> m_objectIdStaging;
                //! Per-group instanced SRG (PerDraw): binds m_instanceObjectIds = the
                //! group's objectId buffer + sets m_useInstancing=1 so every pass VS reads
                //! m_instanceObjectIds[SV_InstanceID] instead of the per-draw m_objectId.
                Data::Instance<RPI::ShaderResourceGroup> m_instanceSrg;
                //! DIRECT (non-indirect) DrawIndexed geometry view, configured exactly
                //! like MeshRenderData::IndirectGeometryView (same index + 5 streams,
                //! same order) but with DrawIndexed args so SetDrawInstanceArguments
                //! drives instanceCount. BLOCKING FIX B1.
                RHI::GeometryView m_instancedGeometryView { RHI::MultiDevice::AllDevices };
                //! Camera packet (depth/motion/forward instanced items) + separate
                //! whole-mesh shadow packet (all members cast). Owning Ptrs keep them alive.
                RHI::Ptr<RHI::DrawPacket> m_cameraPacket;
                RHI::Ptr<RHI::DrawPacket> m_shadowPacket;
                bool m_dirty = true;   //!< membership changed (or first build) -> rebuild.
            };

            //! Add/remove an instance to/from its group (keyed by render object + lod +
            //! mesh), marking the group dirty. Called from AddInstance / RemoveInstance.
            void AddInstanceToGroup(MeshletsRenderInstance* instance);
            void RemoveInstanceFromGroup(MeshletsRenderInstance* instance);
            //! Rebuild a dirty group's objectId buffer + instanced camera/shadow packets.
            //! No-op (returns true) if not dirty. Builds the DIRECT-DrawIndexed instanced
            //! geometry view, the per-group instanced SRG, and one packet per pass.
            bool RebuildInstanceGroup(InstanceGroup& group, const InstanceGroupKey& key, MeshRenderData& meshRenderData);

            //! Phase 6 CPU cull (per frame, when enabled): frustum/cone-cull the
            //! instance's clusters against \p frustum / \p cameraPos, upload the
            //! compacted DrawIndexedIndirect commands to the instance's RingBuffer,
            //! point CameraGeometryView at them, and rebuild the instance packet.
            //! Sets outVisible/outCulled cluster counts.
            void CullInstanceAndRebuildPacket(
                MeshletsRenderInstance& instance, MeshRenderData& meshRenderData,
                const AZ::Frustum& frustum, const AZ::Vector3& cameraPos,
                uint32_t& outVisible, uint32_t& outCulled);

            //! Phase 6 GPU cull: lazily create this instance's GPU cull resources —
            //! the cull SRG (binds the mesh's GPU bounds/descriptor buffers + a
            //! dedicated args buffer), the args buffer, the static indirect geometry
            //! view (clusterCount fixed-slot commands), and the cull dispatch item.
            //! Idempotent. Returns true once GpuCullResourcesReady.
            bool EnsureGpuCullResources(MeshletsRenderInstance& instance, MeshRenderData& meshRenderData);

            //! Phase 6 GPU cull (per frame, when enabled): update the instance's cull
            //! SRG (transform + world-space frustum planes + flags), recompile it, and
            //! queue the cull dispatch + the args-buffer attachment. The dispatch fills
            //! the args buffer on the GPU; the static packet draws clusterCount
            //! commands (culled clusters have instanceCount=0). Cheap — no per-frame
            //! packet rebuild. outVisible/outCulled are estimates (the GPU does the
            //! real cull; counts here reflect the CPU-side equivalent for the HUD).
            void UpdateGpuCullInstance(
                MeshletsRenderInstance& instance, MeshRenderData& meshRenderData,
                const AZ::Frustum& frustum, const AZ::Vector3& cameraPos,
                const AZ::Matrix4x4& objectToWorld, const AZ::Matrix4x4& worldToClip);

            //! Append this instance's compacted + args buffers (with finalize + barrier
            //! usage metadata) to \p outList. Used both for the compute pass (changed
            //! instances) and the barrier pass (all instances, incl. skipped ones).
            void AppendGpuCullAttachments(
                MeshletsRenderInstance& instance, uint32_t indexCount,
                AZStd::vector<MeshletsImportedAttachment>& outList);

            //! Find the early GPU-cull compute + barrier passes in the pipeline.
            //! Mirrors InitComputePass. Returns true if both were found.
            bool InitCullPasses(RPI::RenderPipeline* renderPipeline);

            //! Inject the early GPU-cull pass (compute + barrier) before DepthPrePass
            //! so the cull runs and the args buffers transition to Indirect before the
            //! standard depth/forward/shadow passes consume them. Returns true on success.
            bool TryAutoInjectCullPass(RPI::RenderPipeline* renderPipeline);

            bool HasMeshletPasses(RPI::RenderPipeline* renderPipeline);

            //! If the project hasn't placed MeshletsParentPass into its pipeline template,
            //! attempt to inject it after a known opaque-stage pass slot. Returns true on
            //! success. Tries multiple insertion-point names so we cover common pipelines
            //! (MainPipeline, LowEndPipeline) without hardcoding any single one.
            bool TryAutoInjectPasses(RPI::RenderPipeline* renderPipeline);

            //! One-shot: bind the SharedBuffer to the compute and render pass slots.
            //! Called from OnRenderPipelineChanged after Init() establishes the passes
            //! and CreateResources allocates the buffer. Replaces the per-frame retry
            //! loop that was needed when timing wasn't yet understood.
            //! Safe no-op if either the buffer or the passes are absent.
            void AttachSharedBufferToPasses();

            void CreateResources();
            void CleanResources();
            void CleanPasses();

            void DeletePendingMeshletsRenderObjects();

        private:
            AZ_DISABLE_COPY_MOVE(MeshletsFeatureProcessor);

            AZStd::unique_ptr<Meshlets::SharedBuffer> m_sharedBuffer;  // used for all meshlets geometry buffers.

            Data::Instance<MultiDispatchComputePass> m_computePass;
            Data::Instance<MeshletsRenderPass> m_renderPass;
            //! GPU cull: early compute pass (runs cull dispatches, writes args UAV)
            //! and barrier pass (declares args Indirect → UAV->Indirect transition
            //! before the standard raster passes consume them). Both are
            //! MultiDispatchComputePass instances injected before DepthPrePass.
            Data::Instance<MultiDispatchComputePass> m_cullComputePass;
            Data::Instance<MultiDispatchComputePass> m_cullBarrierPass;
            bool m_sharedBufferAttached = false;

            //! HiZ per-cluster occlusion (opt-in m_debugControls.m_hiZCull): the persistent
            //! double-buffered HiZ pyramid pass instance in this pipeline (MainPipeline's
            //! GpuCullAndDrawPass/HiZGeneratePass, now HiZGeneratePersistentTemplate).
            //! Found in InitCullPasses; null when the pipeline has none → HiZ cull stays off.
            RPI::Ptr<RPI::HiZGeneratePass> m_hiZGeneratePass;
            //! Per-frame resolved bind state: the LAST-COMPLETED pyramid slot (safe to read
            //! early in the frame) + the world->clip of the camera that rendered it (the
            //! PREVIOUS frame's — projecting current bounds with the pyramid's own matrix
            //! keeps the depth comparison in the pyramid's clip space). Valid only when the
            //! toggle is on, the pyramid is populated, and the barrier pass exists to
            //! transition the image to shader-read.
            Data::Instance<RPI::AttachmentImage> m_hiZBindImage;
            AZ::Matrix4x4 m_hiZBindWorldToClip = AZ::Matrix4x4::CreateIdentity();
            bool m_hiZBindValid = false;
            AZ::Matrix4x4 m_hiZPrevCullMat = AZ::Matrix4x4::CreateIdentity();
            bool m_hasHiZPrevCullMat = false;

            //! Phase 6 cluster-DAG cut: per-frame projection stash. m_dagProjScale =
            //! cot(FovY/2) * viewportHeight * 0.5 (pure projection, rotation-independent);
            //! m_dagViewport = render-target size in pixels (also feeds the per-triangle
            //! pixel-size gates). Valid only when a camera + pipeline size resolved this
            //! frame; while invalid, every DAG-aware shader falls back to leaf-only.
            float m_dagProjScale = 0.0f;
            AZ::Vector2 m_dagViewport = AZ::Vector2::CreateZero();
            bool m_dagBindValid = false;
            //! Tracks r_meshletsDagLod so a toggle rebuilds packets/resources (dispatch
            //! counts and SRG constants bake the DAG range).
            bool m_lastDagLod = false;

            //! Two-pass occlusion (opt-in r_meshletsTwoPassOcclusion): the injected
            //! late-depth pass (PASS 2 — draws disoccluded clusters after this frame's
            //! HiZ reduce), the late PSO, and a monotonic frame id for the per-cluster
            //! visibility ledgers (starts at 1: a fresh zeroed ledger never matches).
            Data::Instance<MeshletsRenderPass> m_lateDepthPass;
            Data::Instance<RPI::Shader> m_meshLateShader;
            const RHI::PipelineState* m_meshLatePipelineState = nullptr;
            RHI::DrawListTag m_lateDrawListTag;
            Data::Asset<RPI::AnyAsset> m_latePassRequestAsset;
            uint32_t m_frameId = 1;
            bool m_lastTwoPass = false;
            //! Per-frame scratch: visibility-ledger imports for the barrier pass
            //! (ReadWrite/compute — establishes UAV state + a sync point before the
            //! depth prepass) and the late pass (ReadWrite on its own scope — the sync
            //! point AFTER pass 1's AS writes).
            AZStd::vector<MeshletsImportedAttachment> m_visFrameAttachmentsScratch;

            bool TryAutoInjectLatePass(RPI::RenderPipeline* renderPipeline);

            //! Phase 3 streaming: rebuild \p mrd's per-cluster paged map (residency +
            //! group-completeness bits + slot/local addressing), recreate its GPU
            //! buffer, and push the paged state into the mesh's object SRG.
            void RebuildPagedClusterMap(MeshRenderData& mrd);

            //! Phase 7 streaming (opt-in r_meshletsStreaming; v1 = design phase 2:
            //! pages classify/load/evict against the slot budget while RENDERING still
            //! draws the monolithic buffers — the paged GPU pools + residency-aware
            //! cut are the next work package; this fixes the interfaces and proves the
            //! residency behavior with live stats).
            MeshletsPageResidency m_pageResidency;
            bool m_pageResidencyInitialized = false;
            AZStd::vector<MeshletsPageResidency::PageRequest> m_pageRequestScratch;
            //! Phase 3 paged GPU pool + upload machinery. The pool is ONE global
            //! StructuredBuffer<uint> of fixed PageSlotU32s-word slots; uploads run as
            //! compute dispatches on the cull compute pass (staging created via the
            //! proven ReadOnly initial-data path, pool imported ReadWrite + finalized
            //! there, transitioned to shader-read by the barrier pass).
            Data::Instance<RPI::Buffer> m_pagePoolBuffer;
            AZ::Name m_pagePoolAttachmentId{ "MeshletsPagePool" };
            Data::Instance<RPI::Shader> m_pageUploadShader;
            uint32_t m_pagePoolSlotCount = 0;
            //! Per-frame upload work (appended into the cull pass's lists at feed time
            //! — the cull scratches are cleared later in Render than the streaming
            //! block runs).
            AZStd::vector<RHI::DispatchItem*> m_pageUploadItemsScratch;
            AZStd::vector<MeshletsImportedAttachment> m_pageUploadAttachmentsScratch;
            //! Keeps this frame's staging buffers + dispatch items (and a few frames
            //! back, for in-flight safety) alive until the GPU consumed them.
            struct PageUploadHold
            {
                AZStd::vector<Data::Instance<RPI::Buffer>> m_staging;
                AZStd::vector<Data::Instance<RPI::ShaderResourceGroup>> m_srgs;
                AZStd::vector<AZStd::unique_ptr<MeshletsDispatchItem>> m_dispatches;
            };
            AZStd::array<PageUploadHold, 4> m_pageUploadHold;
            uint32_t m_pageUploadHoldIndex = 0;
            //! Per-frame page-key -> (mesh, page index) reverse lookup.
            AZStd::unordered_map<uint64_t, AZStd::pair<MeshRenderData*, uint32_t>> m_pageKeyLookup;
            //! Meshes whose paged cluster map must be rebuilt this frame.
            AZStd::unordered_set<MeshRenderData*> m_pagedMapDirty;

            //! Streaming stats surfaced in the debug ImGui.
            uint32_t m_streamingResidentPages = 0;
            uint32_t m_streamingLoadsThisFrame = 0;
            uint32_t m_streamingEvictsThisFrame = 0;
            uint32_t m_streamingTrackedPages = 0;
            uint32_t m_streamingStarvedPages = 0;
            //! Detects live r_meshletsStreamingPoolMB changes (phase-4 budget sweeps).
            uint32_t m_lastStreamingPoolMB = 0;

            AZStd::vector<MeshletsRenderObject*> m_meshletsRenderObjects;
            AZStd::vector<AZStd::unique_ptr<MeshletsRenderInstance>> m_instances;

            // Step B group containers (the InstanceGroupKey / InstanceGroup types are
            // declared in the protected section above so the method signatures can use them).
            AZStd::map<InstanceGroupKey, InstanceGroup> m_instanceGroups;
            //! Last-logged Step B collapse counts (instrumentation; logs on change).
            size_t m_lastStepBInstances = SIZE_MAX;
            size_t m_lastStepBGroups = SIZE_MAX;
            size_t m_lastStepBCameraPackets = SIZE_MAX;
            size_t m_lastStepBShadowPackets = SIZE_MAX;

            // For AcquireInstance: cache MeshletsRenderObject per ModelAsset id with a
            // refcount, so N instances of the same model share one compute dispatch.
            // The refcount is the number of live AcquireInstance handles referencing
            // the entry; on ReleaseInstance it drops by 1, and at zero the render
            // object is queued for deletion.
            struct SharedRenderObjectEntry
            {
                MeshletsRenderObject* m_renderObject = nullptr;
                uint32_t m_refCount = 0;
            };
            AZStd::unordered_map<AZ::Data::AssetId, SharedRenderObjectEntry> m_sharedRenderObjectsByAsset;

            PackResolver m_packResolver;
            mutable AZStd::unordered_map<AZ::Data::AssetId, PackResolutionStatus> m_packStatusByModel;
            mutable AZStd::mutex m_packStatusMutex;

            //! The render pipeline is acquired and set when a pipeline is created or changed
            //! and accordingly the passes and the feature processor are associated.
            //! Notice that scene can contain several pipelines all using the same feature
            //! processor.  On the pass side, it will acquire the scene and request the FP,
            //! but on the FP side, it will only associate to the latest pass hence such a case
            //! might still be a problem.  If needed, it can be resolved using a map for each
            //! pass name per pipeline.
            RPI::RenderPipeline* m_renderPipeline = nullptr;

            //! Pipelines for which we've already logged the disabled-warning. Prevents
            //! per-frame log spam when PassChanged fires repeatedly. Cleared on Removed.
            AZStd::unordered_set<const RPI::RenderPipeline*> m_loggedDisabledPipelines;

            //! Cached pass-request asset used by TryAutoInjectPasses. Loaded once.
            AZ::Data::Asset<AZ::RPI::AnyAsset> m_passRequestAsset;

            Render::TransformServiceFeatureProcessorInterface* m_transformServiceFeatureProcessor = nullptr;

            AZStd::vector<MeshletsRenderObject*> m_renderObjectsMarkedForDeletion;

            // Reusable per-frame scratch vectors to avoid per-frame heap allocations
            AZStd::vector<RHI::DispatchItem*> m_dispatchItemsScratch;
            AZStd::vector<const RHI::DrawPacket*> m_drawPacketsScratch;
            // GPU cull per-frame scratch: cull dispatch items + the args-buffer
            // attachments (declared ReadWrite on the cull compute pass, Indirect on
            // the barrier pass).
            AZStd::vector<RHI::DispatchItem*> m_cullDispatchItemsScratch;
            //! Compute-pass attachments (ReadWrite + SRG finalize) — only instances that
            //! actually re-cull this frame (changed camera/transform).
            AZStd::vector<MeshletsImportedAttachment> m_cullArgsAttachmentsScratch;
            //! Barrier-pass attachments (compacted->SRV, args->Indirect) — EVERY active
            //! GPU-cull instance every frame, so a skipped instance's buffers stay in the
            //! read state the standard passes consume (they keep last frame's compacted
            //! data, which is still valid because nothing moved).
            AZStd::vector<MeshletsImportedAttachment> m_cullBarrierAttachmentsScratch;

            //! Cached pass-request asset that injects the early GPU-cull pass
            //! (compute + barrier) before DepthPrePass. Loaded once.
            AZ::Data::Asset<AZ::RPI::AnyAsset> m_cullPassRequestAsset;

            // SP1 diagnostic: last-reported per-frame counts (Render hook). Used to
            // emit a trace only when the queue counts change rather than every frame.
            int32_t m_lastRenderedDispatchCount   = -1;
            int32_t m_lastRenderedDrawPacketCount = -1;

            Data::Instance<RPI::Shader> m_renderShader;
            Data::Instance<RPI::Shader> m_depthShader;
            Data::Instance<RPI::Shader> m_shadowShader;
            Data::Instance<RPI::Shader> m_forwardShader;
            Data::Instance<RPI::Shader> m_motionShader;
            Data::Instance<RPI::Shader> m_computeShader;
            //! GPU cull compute shader (MeshletsCull.shader). Loaded standalone (its
            //! pipeline state drives the per-instance cull dispatch items).
            Data::Instance<RPI::Shader> m_cullComputeShader;

            // Shadow pass: pipeline state + DrawListTag for the "shadow" DrawItem.
            // Unlike depth/forward which use gem-private MeshletsRenderPass instances,
            // the shadow system uses the standard ShadowParent pass which automatically
            // picks up any DrawItem tagged "shadow". We only need the PSO + tag here.
            const RHI::PipelineState* m_shadowPipelineState = nullptr;
            RHI::DrawListTag m_shadowDrawListTag;

            // Forward PBR pass: pipeline state + DrawListTag for the "forward"
            // DrawItem. Like shadow, this is rendered by the STANDARD Atom
            // ForwardPass (which binds the ForwardPassSrg lighting resources for
            // every draw item it submits), so no gem-private pass is needed —
            // only the PSO + tag. Replaces the debug-shader render path.
            const RHI::PipelineState* m_forwardPipelineState = nullptr;
            RHI::DrawListTag m_forwardDrawListTag;

            // Phase 5: hardware mesh-shader forward path (MeshletsForwardMeshShader —
            // Mesh + Fragment entry points, DispatchMesh PSO, same "forward" tag so the
            // standard ForwardPass renders it). Gated by r_meshletsHwMeshShader + the
            // device's m_meshShader feature bit.
            Data::Instance<RPI::Shader> m_meshForwardShader;
            const RHI::PipelineState* m_meshForwardPipelineState = nullptr;
            RHI::DrawListTag m_meshForwardDrawListTag;
            //! Device mesh-shader support (RHI DeviceFeatures::m_meshShader), queried once.
            bool m_meshShaderSupported = false;
            bool m_meshShaderSupportQueried = false;
            //! Tracks the r_meshletsHwMeshShader toggle so a change rebuilds all packets.
            bool m_lastHwMeshShader = false;

            // Phase 5 AS/triangle cull (opt-in r_meshletsMsCullAS): separate shader/PSO
            // adding the Amplification stage + payload-driven, per-triangle-culled Mesh
            // entry (MeshletsForwardMeshShaderCulled.shader). Same "forward"
            // DrawListTag as m_meshForwardShader (reused -- both .shader files declare
            // "DrawList":"forward"). instance.MeshShaderInstanceSrg and
            // meshRenderData.MeshShaderObjectSrg are SHARED between this PSO and the
            // default mesh-shader PSO (both compiled from the same MeshletsMeshRenderSrg.azsli
            // SRG declarations), mirroring how instance.InstanceSrg is already shared
            // across the depth/shadow/forward/motion vertex-pull pipelines.
            Data::Instance<RPI::Shader> m_meshCullForwardShader;
            const RHI::PipelineState* m_meshCullForwardPipelineState = nullptr;
            //! Tracks the r_meshletsMsCullAS toggle so a change rebuilds all packets.
            bool m_lastMsCullAS = false;

            // Hardware mesh-shader depth prepass (MeshletsDepthMeshShader — Mesh entry
            // only, no Fragment). Reuses the existing "depth" DrawListTag
            // (m_depthDrawListTag) since both .shader files declare "DrawList":"depth",
            // so only a separate PSO is needed (the depth pass's render-attachment
            // config differs from forward's). SRGs are shared with the mesh forward
            // path exactly like m_meshCullForwardShader above.
            Data::Instance<RPI::Shader> m_meshDepthShader;
            const RHI::PipelineState* m_meshDepthPipelineState = nullptr;

            // Mesh-shader MOTION VECTOR pass. Shares the "motion" DrawListTag with the
            // vertex-pull MeshletsMotionVector.shader; only a separate PSO is needed.
            Data::Instance<RPI::Shader> m_meshMotionShader;
            const RHI::PipelineState* m_meshMotionPipelineState = nullptr;

            // AS-culled depth/motion PSOs (opt-in r_meshletsMsCullAS) — same shared
            // cluster-cull AS as the culled forward PSO so all items in the packet
            // consume the same AS-group-count geometry view. Same "depth"/"motion" tags.
            Data::Instance<RPI::Shader> m_meshDepthCullShader;
            const RHI::PipelineState* m_meshDepthCullPipelineState = nullptr;
            Data::Instance<RPI::Shader> m_meshMotionCullShader;
            const RHI::PipelineState* m_meshMotionCullPipelineState = nullptr;
            //! Phase 6 shadow-side DAG cut: AS-culled shadow PSO (cut-only mode — the
            //! shadow instance SRG zeroes every camera-view cull toggle). Used only
            //! while the DAG cut is active; otherwise shadows stay on the plain
            //! all-leaves mesh shadow PSO.
            Data::Instance<RPI::Shader> m_meshShadowCullShader;
            const RHI::PipelineState* m_meshShadowCullPipelineState = nullptr;

            // Mesh-shader SHADOW pass. Shares the "shadow" DrawListTag with the vertex-pull
            // MeshletsShadowPass.shader; only a separate PSO is needed.
            Data::Instance<RPI::Shader> m_meshShadowShader;
            const RHI::PipelineState* m_meshShadowPipelineState = nullptr;

            // Depth prepass: pipeline state + DrawListTag for the standard "depth"
            // DrawItem, rendered by Atom's early DepthPrePass into the main depth
            // buffer. Mirrors the shadow/forward "standard tag + PSO" pattern. This
            // replaced the old gem-private depth pass (now retired), which wrote
            // meshlet depth too late in the frame for the depth-consuming effects.
            const RHI::PipelineState* m_depthPipelineState = nullptr;
            RHI::DrawListTag m_depthDrawListTag;
            //! PERF: hardware-IA POSITION input layout for the depth PSO (used to validate
            //! the geometry view's streams before adding the depth draw item).
            RHI::InputStreamLayout m_depthInputLayout;
            //! PERF: hardware-IA input layouts for the shadow/motion (POSITION-only) and
            //! forward (POSITION,NORMAL,TANGENT,BITANGENT,UV) PSOs. The forward channel order
            //! MUST match the geometry views' stream order (see EnsureIndirectArgs).
            RHI::InputStreamLayout m_shadowInputLayout;
            RHI::InputStreamLayout m_motionInputLayout;
            RHI::InputStreamLayout m_forwardInputLayout;

            // Motion-vector pass: pipeline state + DrawListTag for the "motion"
            // DrawItem, rendered by Atom's standard MeshMotionVector pass. Produces
            // per-pixel screen-space motion (reading the previous-frame transform
            // from SceneSrg::GetObjectToWorldMatrixPrev) so TAA / temporal upscaling
            // don't ghost the meshlet when the camera or object moves.
            const RHI::PipelineState* m_motionPipelineState = nullptr;
            RHI::DrawListTag m_motionDrawListTag;
            RHI::GeometryView m_geometryView { RHI::MultiDevice::AllDevices };

            //! Debug toggles edited by the ImGui debug tab.
            DebugControls m_debugControls;

            //! Freeze-frustum debug state: when m_debugControls.m_freezeCullCamera turns
            //! on we snapshot the live camera here and cull against it until released.
            AZ::Frustum m_frozenFrustum;
            AZ::Vector3 m_frozenCameraPos = AZ::Vector3::CreateZero();
            AZ::Matrix4x4 m_frozenCamMatrix = AZ::Matrix4x4::CreateIdentity();
            bool m_cullCameraFrozen = false;

            //! Cull optimization: cache the effective cull camera + params so we can
            //! skip the per-frame cull+rebuild for instances when nothing moved.
            AZ::Matrix4x4 m_lastEffectiveCamMatrix = AZ::Matrix4x4::CreateZero();
            bool m_lastFrustumCull = true;
            bool m_lastConeCull = true;
            bool m_haveLastCullCamera = false;

            //! Last-applied meshlet debug-color toggle. When it differs from
            //! m_debugControls.m_meshletColorMode the per-object SRG flags are
            //! re-applied (once, on the change).
            bool m_lastMeshletColorMode = false;
            //! Tracks r_meshletsDagDebugColor for the same on-change SRG refresh.
            bool m_lastDagDebugColor = false;
            //! Tracks the cull toggle so a disable transition rebuilds whole-mesh packets.
            bool m_cullWasEnabled = false;
            //! Tracks the CPU/GPU cull mode so a switch rebuilds packets with the
            //! correct geometry view (CPU ring vs GPU args buffer).
            bool m_lastGpuCull = false;

            // Phase 6b: shared indirect-draw command signature (one DrawIndexed
            // command). Per-mesh indirect-args buffers reference it.
            RHI::Ptr<RHI::IndirectBufferSignature> m_drawIndirectSignature;
            //! GPU cull single-compacted-draw: a non-indexed Draw command signature
            //! ({vertexCount, instanceCount, startVertex, startInstance}).
            RHI::Ptr<RHI::IndirectBufferSignature> m_drawNonIndexedSignature;
        };
    } // namespace Meshlets
} // namespace AZ
