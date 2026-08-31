/*
* Modifications Copyright (c) Contributors to the Open 3D Engine Project. 
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
* 
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Console/IConsole.h>

#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Frustum.h>
#include <AzCore/Math/Plane.h>
#include <AzCore/Math/Vector4.h>

#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Pass/PassRequest.h>
#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>
#include <Atom/RHI/GeometryView.h>   // RHI::ValidateStreamBufferViews
#include <Atom/RHI/RHISystemInterface.h>   // device DeviceFeatures::m_meshShader gate

#include <Atom/Feature/RenderCommon.h>

#include <MeshletsRenderObject.h>
#include <MeshletsFeatureProcessor.h>
#include <MeshletsUtilities.h>
#include <Meshlets/Reflect/MeshletPackAsset.h>

namespace AZ
{
    namespace Meshlets
    {
        // GPU cull thread-group size -- MUST match MESHLETS_CULL_GROUP in MeshletsCull.azsl
        // AND MeshletsDispatchItem's MESHLETS_THREAD_GROUP_SIZE (so InitDispatch issues
        // exactly one group). The compaction shader strides clusters within the group.
        static constexpr uint32_t MeshletsCullGroupSize = 64;

        // Kill-switch for the actual GPU dispatch path. When 0 (default),
        // AcquireInstance returns InvalidInstanceHandle BEFORE constructing
        // a MeshletsRenderObject -- i.e. the meshlet asset pipeline still
        // runs (sidecar -> .azmeshletpack, PackResolver maps model->pack)
        // and the editor's "Use Virtual Geometry" toggle still surfaces
        // pack status, but no compute dispatch or vertex-pull draw runs.
        // With GPU-side bounds clamping (m_indicesCount / m_vertexCount in
        // MeshletsObjectRenderSrg) and CPU-side render-buffer validation
        // in BuildInstanceDrawPacket, the render path is safe to enable
        // by default. Set to 0 from the editor console (~ key) to
        // temporarily disable meshlet rendering for debugging.
        AZ_CVAR(bool, r_meshletsRenderEnabled, true, nullptr,
            AZ::ConsoleFunctorFlags::Null,
            "If true, AcquireInstance proceeds to the full GPU render path. "
            "Set to false to disable meshlet rendering while keeping the "
            "asset pipeline + construction for validation purposes.");

        // Per-frame, per-instance LOD selection by approximate screen coverage.
        // ON (default): distant instances pick a coarser LOD (fewer verts/triangles).
        // OFF: every instance is pinned to LOD0 (the pre-LOD behaviour) for A/B
        // comparison and as an escape hatch if a pack's coarse LODs look wrong.
        AZ_CVAR(bool, r_meshletsLodSelection, true, nullptr,
            AZ::ConsoleFunctorFlags::Null,
            "If true, meshlet instances select their LOD each frame from approximate "
            "screen coverage (distant instances use coarser LODs). If false, every "
            "instance is pinned to LOD0.");

        // Selects WHICH metric r_meshletsLodSelection uses. OFF (default) = the original
        // screen-coverage bands. ON = meshopt_simplify's own scale-independent geometric
        // error per LOD (SectionKind::LodError), compared as
        // aabb_pixel_size * lod_error <= acceptable_pixel_error -- strictly the better
        // metric at the same runtime cost, but it changes LOD choices for every existing
        // pack, so it stays opt-in until someone has compared the two on a real frame.
        // Falls back to screen coverage automatically for packs with no LodError section.
        AZ_CVAR(bool, r_meshletsGeometricLod, false, nullptr,
            AZ::ConsoleFunctorFlags::Null,
            "If true, meshlet LOD selection uses per-LOD geometric error (pixel-error "
            "budget) instead of screen-coverage bands. Requires a pack built with a "
            "LodError section; falls back to screen coverage when absent.");

        // Phase 5: hardware mesh-shader render path. When on (and the GPU reports
        // mesh-shader support), each meshlet instance's camera packet is a single
        // forward DrawItem driven by DispatchMesh -- one threadgroup per cluster, the
        // mesh shader pulls cluster vertices/triangles directly (no index buffer, no
        // input assembler). FIRST SLICE: forward pass only with a flat per-cluster
        // debug color (proves the mesh-shader emit); no shadow/depth/motion items.
        // Off (default) = the shipping vertex-pull hardware-IA path, untouched.
        AZ_CVAR(bool, r_meshletsHwMeshShader, false, nullptr,
            AZ::ConsoleFunctorFlags::Null,
            "If true and the GPU supports hardware mesh shaders, meshlet instances "
            "render through DispatchMesh (one threadgroup per cluster, flat per-cluster "
            "debug color, forward pass only -- first bring-up slice). If false (default), "
            "the vertex-pull hardware-IA path is used.");

        // Phase 5: amplification-shader per-cluster cull + per-triangle cull (backface/
        // degenerate/micro-polygon), evaluated in a SEPARATE PSO from the default mesh-
        // shader path (MeshletsForwardMeshShaderCulled.shader). Has no effect unless
        // r_meshletsHwMeshShader is also on and the device supports mesh shaders. Off
        // (default) = the existing r_meshletsHwMeshShader path (no AS, every cluster
        // drawn), untouched. Nothing in this cull path has been verified against a
        // rendered frame yet -- leave off until it has.
        AZ_CVAR(bool, r_meshletsMsCullAS, false, nullptr,
            AZ::ConsoleFunctorFlags::Null,
            "If true (and r_meshletsHwMeshShader is also true and the GPU supports "
            "mesh shaders), meshlet instances render through an amplification shader "
            "that culls clusters (frustum + cone) and compacts survivors before "
            "DispatchMesh, and the mesh shader additionally culls backfacing/degenerate/"
            "sub-pixel triangles before emitting them. If false (default), the plain "
            "DispatchMesh(clusterCount,1,1) path (no cull stage) is used.");

        // Freeze the cluster-cull camera from outside the editor. Mirrors the
        // "Freeze cull camera" checkbox in the Meshlets ImGui debug tab; either source
        // freezes (they OR together). This exists so gems that do not depend on Meshlets
        // (e.g. WDDebugView's detachable debug camera) can freeze meshlet cluster culling
        // in lockstep with RPI's frustum freeze without a build dependency.
        AZ_CVAR(bool, r_meshletsFreezeCull, false, nullptr,
            AZ::ConsoleFunctorFlags::Null,
            "If true, meshlet cluster culling keeps culling against the camera pose "
            "captured when this turned on, so you can fly the real camera around and see "
            "which clusters were culled. Has no effect unless cluster culling is enabled.");

        // Cluster-DAG continuous LOD (design doc: 2026-08-31-meshlets-phase6-*).
        // Needs a "generate_cluster_dag" pack; replaces the per-instance LOD ladder.
        AZ_CVAR(bool, r_meshletsDagLod, false, nullptr,
            AZ::ConsoleFunctorFlags::Null,
            "If true, meshes baked with a cluster DAG (sidecar generate_cluster_dag) "
            "select per-cluster detail via the DAG screen-space-error cut instead of "
            "the per-instance discrete LOD ladder. AS cull path (r_meshletsMsCullAS) "
            "and the compute GPU-cull path both honor the cut.");
        AZ_CVAR(float, r_meshletsDagErrorPx, 1.0f, nullptr,
            AZ::ConsoleFunctorFlags::Null,
            "Cluster-DAG cut threshold in pixels: a cluster is drawn when its own "
            "projected simplification error is <= this and its parent's is > this. "
            "Smaller = finer geometry; sweep it to verify crack-freedom.");
        AZ_CVAR(bool, r_meshletsDagDebugColor, false, nullptr,
            AZ::ConsoleFunctorFlags::Null,
            "Color mesh-shader meshlets by DAG depth (quantized simplification error): "
            "leaves render blue-gray, coarser levels step through hues. Makes the cut "
            "boundary directly visible.");

        // Geometry streaming (design doc: 2026-08-31-meshlets-streaming-paging-*).
        AZ_CVAR(bool, r_meshletsStreaming, false, nullptr,
            AZ::ConsoleFunctorFlags::Null,
            "If true, leaf streaming pages of v4 meshlet packs are classified and "
            "loaded/evicted against the r_meshletsStreamingPoolMB budget (v1: residency "
            "tracking + stats; rendering still uses the resident fallback data).");
        AZ_CVAR(uint32_t, r_meshletsStreamingPoolMB, 256, nullptr,
            AZ::ConsoleFunctorFlags::Null,
            "Streaming pool budget in MB. Slot count = budget / ~192KB nominal page slot.");
        AZ_CVAR(uint32_t, r_meshletsStreamingMaxLoadsPerFrame, 8, nullptr,
            AZ::ConsoleFunctorFlags::Null,
            "Upper bound on page loads issued per frame (spreads upload cost). Sweep "
            "upward when profiling teleport recovery time (phase 4 soak).");
        AZ_CVAR(float, r_meshletsStreamingHysteresis, 1.5f, nullptr,
            AZ::ConsoleFunctorFlags::Null,
            "Streaming eviction hysteresis: pages evict only below tau/thisFactor. "
            "Tune until the ImGui churn counter stays flat during normal camera motion.");

        // Two-pass occlusion (design doc: 2026-08-31-meshlets-two-pass-occlusion-*).
        AZ_CVAR(bool, r_meshletsTwoPassOcclusion, false, nullptr,
            AZ::ConsoleFunctorFlags::Null,
            "If true (with r_meshletsMsCullAS), meshlet HiZ occlusion runs as two-pass: "
            "the depth prepass culls with last frame's pyramid and records what it drew; "
            "a late depth pass re-tests only the skipped clusters against this frame's "
            "pyramid and draws the disoccluded ones the same frame.");

        namespace
        {
            //! One shared derivation for a page's residency key -- the classifier
            //! request path and the load/evict reverse path MUST agree.
            uint64_t MeshletsPageKey(const MeshRenderData* mrd, uint32_t pageIndex)
            {
                return reinterpret_cast<uint64_t>(mrd) * 0x9E3779B97F4A7C15ull +
                    static_cast<uint64_t>(pageIndex);
            }
        }

        MeshletsFeatureProcessor::MeshletsFeatureProcessor()
        {
            MeshletsComputePassName = Name("MeshletsComputePass");
            MeshletsRenderPassName = Name("MeshletsRenderPass");
            MeshletsCullComputePassName = Name("MeshletsCullComputePass");
            MeshletsCullBarrierPassName = Name("MeshletsCullBarrierPass");
            CreateResources();
        }

        MeshletsFeatureProcessor::~MeshletsFeatureProcessor()
        {
            CleanResources();
        }

        void MeshletsFeatureProcessor::CreateResources()
        {
            if (!Meshlets::SharedBufferInterface::Get())
            {   // Since there can be several pipelines, allocate the shared buffer only for the
                // first one and from that moment on it will be used through its interface
                AZStd::string sharedBufferName = "MeshletsSharedBuffer";
                uint32_t bufferSize = 256 * 1024 * 1024;

                // Prepare Render Srg descriptors for calculating the required alignment for the shared buffer
                MeshRenderData tempRenderData;
                MeshletsRenderObject::PrepareRenderSrgDescriptors(tempRenderData, 1, 1);

                m_sharedBuffer = AZStd::make_unique<Meshlets::SharedBuffer>(sharedBufferName, bufferSize, tempRenderData.RenderBuffersDescriptors);
            }

            m_renderObjectsMarkedForDeletion.clear();
            DeletePendingMeshletsRenderObjects();
        }

        void MeshletsFeatureProcessor::CleanResources()
        {
            m_sharedBuffer.reset();
        }

        void MeshletsFeatureProcessor::CleanPasses()
        {
            // Null out every live DrawPacket BEFORE releasing the pass pointers.
            // DrawPackets store raw pointers to the SRG objects that live inside the
            // RenderObject / Instance. Those SRGs reference the pipeline state and
            // shader that belong to the pass being torn down. If the DrawPacket
            // survives into the next frame (e.g. already queued in the View's draw
            // list) while the pass -- and its pipeline state -- are destroyed, the
            // item.m_shaderResourceGroups pointer inside the DrawItem becomes dangling
            // and CommitShaderResources faults with a garbage address.
            //
            // By nulling every DrawPacket here, the Render() loop in the NEXT frame
            // skips these instances (DrawPacket == nullptr), and InitRenderPass()
            // rebuilds them with the new pipeline state when it is called.
            for (auto& instance : m_instances)
            {
                if (instance)
                {
                    instance->DrawPacket = nullptr;
                    instance->DepthDrawPacket = nullptr;
                    instance->LateDepthDrawPacket = nullptr;
                    instance->ShadowDrawPacket = nullptr;
                }
            }

            // Step B: the instanced packets hold raw pointers to the instanced PSOs and the
            // per-group SRGs. Null them BEFORE releasing the pass pipeline states, and mark
            // every group dirty so RebuildInstanceGroup rebuilds them with the new PSOs.
            for (auto& [key, group] : m_instanceGroups)
            {
                group.m_cameraPacket = nullptr;
                group.m_shadowPacket = nullptr;
                group.m_dirty = true;
            }

            m_computePass = nullptr;
            m_depthShader = nullptr;
            m_depthPipelineState = nullptr;
            m_renderPass = nullptr;
            m_shadowShader = nullptr;
            m_shadowPipelineState = nullptr;
            m_forwardShader = nullptr;
            m_forwardPipelineState = nullptr;
            m_motionShader = nullptr;
            m_motionPipelineState = nullptr;
            m_sharedBufferAttached = false;
            m_hiZGeneratePass = nullptr;
            m_hiZBindImage = nullptr;
            m_hiZBindValid = false;
            m_lateDepthPass = nullptr;
            m_meshLateShader = nullptr;
            m_meshLatePipelineState = nullptr;
        }

        void MeshletsFeatureProcessor::AttachSharedBufferToPasses()
        {
            if (m_sharedBufferAttached)
            {
                return;
            }
            auto* sbi = SharedBufferInterface::Get();
            if (!sbi || !m_computePass || !m_renderPass)
            {
                return;  // Will be retried on the next OnRenderPipelineChanged.
            }
            Data::Instance<RPI::Buffer> buf = sbi->GetBuffer();
            if (!buf)
            {
                return;
            }
            const Name bufferSlotName("MeshletsSharedBuffer");

            auto* computeBinding = m_computePass->FindAttachmentBinding(bufferSlotName);
            if (computeBinding && !computeBinding->GetAttachment())
            {
                m_computePass->AttachBufferToSlot(bufferSlotName, buf);
            }

            auto* renderBinding = m_renderPass->FindAttachmentBinding(bufferSlotName);
            if (renderBinding && !renderBinding->GetAttachment())
            {
                m_renderPass->AttachBufferToSlot(bufferSlotName, buf);
            }

            m_sharedBufferAttached = true;
        }

        void MeshletsFeatureProcessor::Init([[maybe_unused]]RPI::RenderPipeline* pipeline)
        {
            InitIndirectDrawSignature();
            InitComputePass(MeshletsComputePassName);
            InitDepthShader();
            InitRenderPass(MeshletsRenderPassName);
            InitShadowShader();
            InitForwardShader();
            InitMeshForwardShader();
            InitMeshDepthShader();
            InitMeshMotionShader();
            InitMeshShadowShader();
            InitMotionShader();
            // GPU cull (optional): locate the early compute + barrier passes if the
            // cull pass request was injected. Non-fatal if absent.
            InitCullPasses(pipeline);
        }

        void MeshletsFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<MeshletsFeatureProcessor, RPI::FeatureProcessor>()
                    ->Version(0);
            }
        }

        void MeshletsFeatureProcessor::Activate()
        {
            m_transformServiceFeatureProcessor = GetParentScene()->GetFeatureProcessor<Render::TransformServiceFeatureProcessorInterface>();
            AZ_Assert(m_transformServiceFeatureProcessor, "MeshFeatureProcessor requires a TransformServiceFeatureProcessor on its parent scene.");

            EnableSceneNotification();
            TickBus::Handler::BusConnect();
            m_packResolver.RebuildIndex();
        }

        void MeshletsFeatureProcessor::Deactivate()
        {
            DisableSceneNotification();
            TickBus::Handler::BusDisconnect();
        }

        int MeshletsFeatureProcessor::GetTickOrder()
        {
            return AZ::TICK_PRE_RENDER;
        }

        bool MeshletsFeatureProcessor::HasMeshletPasses(RPI::RenderPipeline* renderPipeline)
        {
            RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassName(MeshletsComputePassName, renderPipeline);
            RPI::Ptr<RPI::Pass> desiredPass = RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);
            return desiredPass ? true : false;
        }

        bool MeshletsFeatureProcessor::InitComputePass(const Name& passName)
        {
            m_computePass = Data::Instance<MultiDispatchComputePass>();
            RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassName(passName, m_renderPipeline);
            RPI::Ptr<RPI::Pass> desiredPass = RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);

            if (desiredPass)
            {
                m_computePass = static_cast<MultiDispatchComputePass*>(desiredPass.get());
                m_computeShader = m_computePass->GetShader();
            }
            else
            {
                AZ_Error("Meshlets", false,
                    "%s does not exist in this pipeline. Check your game project's .pass assets.",
                    passName.GetCStr());
                return false;
            }
            return true;
        }

        bool MeshletsFeatureProcessor::InitCullPasses(RPI::RenderPipeline* renderPipeline)
        {
            m_cullComputePass = nullptr;
            m_cullBarrierPass = nullptr;
            if (!renderPipeline)
            {
                return false;
            }
            {
                RPI::PassFilter f = RPI::PassFilter::CreateWithPassName(MeshletsCullComputePassName, renderPipeline);
                if (RPI::Ptr<RPI::Pass> p = RPI::PassSystemInterface::Get()->FindFirstPass(f))
                {
                    m_cullComputePass = static_cast<MultiDispatchComputePass*>(p.get());
                    m_cullComputeShader = m_cullComputePass->GetShader();
                }
            }
            {
                RPI::PassFilter f = RPI::PassFilter::CreateWithPassName(MeshletsCullBarrierPassName, renderPipeline);
                if (RPI::Ptr<RPI::Pass> p = RPI::PassSystemInterface::Get()->FindFirstPass(f))
                {
                    m_cullBarrierPass = static_cast<MultiDispatchComputePass*>(p.get());
                }
            }
            // Two-pass occlusion PASS 2 (optional -- absent pipeline just leaves it off).
            m_lateDepthPass = nullptr;
            {
                RPI::PassFilter f = RPI::PassFilter::CreateWithPassName(Name("MeshletsLateDepthPass"), renderPipeline);
                if (RPI::Ptr<RPI::Pass> p = RPI::PassSystemInterface::Get()->FindFirstPass(f))
                {
                    m_lateDepthPass = static_cast<MeshletsRenderPass*>(p.get());
                }
            }
            // HiZ per-cluster occlusion: the persistent double-buffered pyramid instance
            // (MainPipeline's GpuCullAndDrawPass child). Optional -- absent pipeline just
            // leaves HiZ cull off.
            m_hiZGeneratePass = nullptr;
            {
                RPI::PassFilter f =
                    RPI::PassFilter::CreateWithTemplateName(Name("HiZGeneratePersistentTemplate"), renderPipeline);
                if (RPI::Ptr<RPI::Pass> p = RPI::PassSystemInterface::Get()->FindFirstPass(f))
                {
                    m_hiZGeneratePass = static_cast<RPI::HiZGeneratePass*>(p.get());
                }
            }
            // Not fatal if absent -- GPU cull simply stays unavailable; CPU cull and the
            // whole-mesh path are unaffected.
            return m_cullComputePass && m_cullBarrierPass;
        }

        bool MeshletsFeatureProcessor::TryAutoInjectCullPass(RPI::RenderPipeline* renderPipeline)
        {
            if (!renderPipeline || renderPipeline->IsExecuteOnce())
            {
                return false;
            }
            // Idempotent: if the early cull passes already exist, nothing to do.
            {
                RPI::PassFilter f = RPI::PassFilter::CreateWithPassName(MeshletsCullComputePassName, renderPipeline);
                if (RPI::PassSystemInterface::Get()->FindFirstPass(f))
                {
                    return true;
                }
            }
            if (!m_cullPassRequestAsset.GetId().IsValid())
            {
                const char* path = "Passes/MeshletsCullPassRequest.azasset";
                m_cullPassRequestAsset =
                    AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::AnyAsset>(path, AZ::RPI::AssetUtils::TraceLevel::Warning);
            }
            if (!m_cullPassRequestAsset || !m_cullPassRequestAsset->IsReady())
            {
                return false;
            }
            const AZ::RPI::PassRequest* passRequest = m_cullPassRequestAsset->GetDataAs<AZ::RPI::PassRequest>();
            if (!passRequest)
            {
                return false;
            }
            RPI::Ptr<RPI::Pass> pass = RPI::PassSystemInterface::Get()->CreatePassFromRequest(passRequest);
            if (!pass)
            {
                return false;
            }
            // The cull compute + Indirect barrier must run BEFORE the standard passes
            // that consume the args buffers. DepthPrePass is the earliest consumer
            // (meshlet "depth" DrawItems). Place the cull pass right before it.
            static const AZStd::array<const char*, 2> insertionPoints = { "DepthPrePass", "DepthPrePassMSAA" };
            for (const char* before : insertionPoints)
            {
                if (renderPipeline->AddPassBefore(pass, Name(before)))
                {
                    return true;
                }
            }
            AZ_Warning("Meshlets", false,
                "TryAutoInjectCullPass: could not find a DepthPrePass insertion point; GPU cull unavailable in this pipeline.");
            return false;
        }

        bool MeshletsFeatureProcessor::TryAutoInjectLatePass(RPI::RenderPipeline* renderPipeline)
        {
            if (!renderPipeline || renderPipeline->IsExecuteOnce())
            {
                return false;
            }
            // Idempotent: already injected?
            {
                RPI::PassFilter f = RPI::PassFilter::CreateWithPassName(Name("MeshletsLateDepthPass"), renderPipeline);
                if (RPI::PassSystemInterface::Get()->FindFirstPass(f))
                {
                    return true;
                }
            }
            if (!m_latePassRequestAsset.GetId().IsValid())
            {
                const char* path = "Passes/MeshletsLatePassRequest.azasset";
                m_latePassRequestAsset =
                    AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::AnyAsset>(path, AZ::RPI::AssetUtils::TraceLevel::Warning);
            }
            if (!m_latePassRequestAsset || !m_latePassRequestAsset->IsReady())
            {
                return false;
            }
            const AZ::RPI::PassRequest* passRequest = m_latePassRequestAsset->GetDataAs<AZ::RPI::PassRequest>();
            if (!passRequest)
            {
                return false;
            }
            RPI::Ptr<RPI::Pass> pass = RPI::PassSystemInterface::Get()->CreatePassFromRequest(passRequest);
            if (!pass)
            {
                return false;
            }
            // Pass 2 must follow this frame's HiZ reduce (hosted by GpuCullAndDrawPass).
            if (renderPipeline->AddPassAfter(pass, Name("GpuCullAndDrawPass")))
            {
                return true;
            }
            AZ_TracePrintf("Meshlets",
                "TryAutoInjectLatePass: no GpuCullAndDrawPass in pipeline [%s]; "
                "two-pass occlusion unavailable there.\n", renderPipeline->GetId().GetCStr());
            return false;
        }

        bool MeshletsFeatureProcessor::EnsureGpuCullResources(
            MeshletsRenderInstance& instance, MeshRenderData& meshRenderData)
        {
            if (instance.GpuCullResourcesReady)
            {
                return true;
            }
            if (!m_cullComputeShader || !m_drawIndirectSignature)
            {
                return false;   // Cull pass/shader/signature not ready yet -- retry next frame.
            }
            if (!instance.RenderObject->EnsureCullGpuBuffers(meshRenderData))
            {
                return false;   // Per-mesh cluster bounds/descriptors not on the GPU yet.
            }
            // The per-cluster cull draws slices of the mesh's STATIC index buffer -- ensure it
            // (and IndexBufferViewRHI / IndirectGeometryView for the whole-mesh shadow) exist.
            if (!instance.RenderObject->EnsureIndirectArgs(meshRenderData, m_drawIndirectSignature.get()))
            {
                return false;
            }
            // Size to the full leaf+interior DAG range; with the cut off,
            // UpdateGpuCullInstance clamps m_clusterCount to leaves.
            const uint32_t clusterCount =
                static_cast<uint32_t>(meshRenderData.PersistentClusterDescriptors.size());
            const uint32_t indexCount = meshRenderData.IndexCount;   // LEAF triangle corners
            if (clusterCount == 0 || indexCount == 0)
            {
                return false;
            }

            // The mesh's STATIC index buffer (real expanded vertex indices) must exist -- the
            // per-cluster cull draws slices of it (EnsureCullGpuBuffers/EnsureIndirectArgs ran above).
            (void)indexCount;

            instance.CullSrg = RPI::ShaderResourceGroup::Create(
                m_cullComputeShader->GetAsset(), AZ::Name{ "MeshletsCullSrg" });
            if (!instance.CullSrg)
            {
                AZ_Error("Meshlets", false, "EnsureGpuCullResources: failed to create MeshletsCullSrg.");
                return false;
            }

            // Per-cluster cull outputs (both UAV-written by the compute, consumed as indirect
            // args). Indirect pool = ShaderReadWrite | Indirect.
            //   m_outCommands : up to clusterCount DrawIndexedIndirect commands (5 u32 each),
            //                   one per VISIBLE cluster, compacted from slot 0.
            //   m_outCount    : 1 u32 = visible-command count (drives DrawIndexedIndirectCount).
            {
                SrgBufferDescriptor d(
                    RPI::CommonBufferPoolType::Indirect, RHI::Format::Unknown,
                    RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::Indirect,
                    5u * static_cast<uint32_t>(sizeof(AZ::u32)), clusterCount,
                    Name{ "MeshletsCullCommands" }, Name{ "m_outCommands" }, 0, 0, nullptr);
                instance.CullCommandBuffer = UtilityClass::CreateBuffer("Meshlets", d, nullptr);
            }
            {
                SrgBufferDescriptor d(
                    RPI::CommonBufferPoolType::Indirect, RHI::Format::Unknown,
                    RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::Indirect,
                    static_cast<uint32_t>(sizeof(AZ::u32)), 1,
                    Name{ "MeshletsCullCount" }, Name{ "m_outCount" }, 0, 0, nullptr);
                instance.CullCountBuffer = UtilityClass::CreateBuffer("Meshlets", d, nullptr);
            }
            if (!instance.CullCommandBuffer || !instance.CullCommandBuffer->GetRHIBuffer() ||
                !instance.CullCountBuffer || !instance.CullCountBuffer->GetRHIBuffer())
            {
                AZ_Error("Meshlets", false, "EnsureGpuCullResources: failed to create GPU cull output buffers.");
                return false;
            }

            // Bind the read-only cluster SRVs. The UAVs (m_outCommands, m_outCount) are bound to
            // scope-backed views + the SRG compiled inside the cull compute pass's CompileResources.
            {
                SrgBufferDescriptor bind;
                bind.m_paramNameInSrg = Name{ "m_clusterBounds" };
                if (!UtilityClass::BindBufferToSrg("Meshlets", meshRenderData.ClusterBoundsBuffer, bind, instance.CullSrg))
                {
                    return false;
                }
                bind.m_paramNameInSrg = Name{ "m_clusterDescriptors" };
                if (!UtilityClass::BindBufferToSrg("Meshlets", meshRenderData.ClusterDescBuffer, bind, instance.CullSrg))
                {
                    return false;
                }
                // Phase 6 DAG cut records (v3 packs only) -- optional; m_doDagCut stays
                // 0 when unbound so the shader never reads it then.
                if (meshRenderData.DagNodesBuffer)
                {
                    bind.m_paramNameInSrg = Name{ "m_dagNodes" };
                    UtilityClass::BindBufferToSrg("Meshlets", meshRenderData.DagNodesBuffer, bind, instance.CullSrg);
                }
            }

            // ONE thread-group per instance (the shader strides clusters + a groupshared counter).
            instance.CullDispatchItem.InitDispatch(m_cullComputeShader.get(), instance.CullSrg, MeshletsCullGroupSize);

            // Geometry view: DrawIndexedIndirectCount over the mesh's STATIC index buffer. Each
            // command renders one visible cluster's slice [startIndex .. startIndex+indexCount).
            // maxSequenceCount = clusterCount (upper bound); the count buffer caps it to visible.
            instance.CullArgsGpuView = RHI::IndirectBufferView(
                *instance.CullCommandBuffer->GetRHIBuffer(), *m_drawIndirectSignature,
                0, clusterCount * 5u * static_cast<uint32_t>(sizeof(AZ::u32)),
                m_drawIndirectSignature->GetByteStride());
            instance.GpuCullGeometryView.SetIndexBufferView(meshRenderData.IndexBufferViewRHI);
            // Hardware-IA POSITION stream for depth/shadow/motion (must be present on EVERY
            // geometry view those passes draw through, or the position-layout PSO hangs).
            if (meshRenderData.PositionStreamValid)
            {
                instance.GpuCullGeometryView.AddStreamBufferView(meshRenderData.PositionStreamView);
            }
            // Forward hardware-IA streams: add NORMAL,TANGENT,BITANGENT,UV in the SAME order
            // right after POSITION so this view's stream layout is identical to
            // IndirectGeometryView ([POSITION,NORMAL,TANGENT,BITANGENT,UV]). Without these the
            // forward 5-channel layout would have unbound channels on the GPU-cull path -> hang.
            if (meshRenderData.ForwardStreamsValid)
            {
                instance.GpuCullGeometryView.AddStreamBufferView(meshRenderData.NormalStreamView);
                instance.GpuCullGeometryView.AddStreamBufferView(meshRenderData.TangentStreamView);
                instance.GpuCullGeometryView.AddStreamBufferView(meshRenderData.BitangentStreamView);
                instance.GpuCullGeometryView.AddStreamBufferView(meshRenderData.UvStreamView);
            }
            RHI::DrawIndirect indirectArgs(
                clusterCount, instance.CullArgsGpuView, 0,
                instance.CullCountBuffer->GetRHIBuffer(), 0);
            instance.GpuCullGeometryView.SetDrawArguments(RHI::DrawArguments(indirectArgs));

            instance.CullArgsAttachmentId = AZ::Name(
                AZStd::string::format("MeshletsCullCmd_%u", instance.ObjectId.GetIndex()));
            instance.CullCountAttachmentId = AZ::Name(
                AZStd::string::format("MeshletsCullCnt_%u", instance.ObjectId.GetIndex()));
            instance.GpuCullClusterCount = clusterCount;
            instance.GpuCullResourcesReady = true;
            instance.DrawPacket = nullptr;   // rebuild against GpuCullGeometryView
            return true;
        }

        void MeshletsFeatureProcessor::UpdateGpuCullInstance(
            MeshletsRenderInstance& instance, MeshRenderData& meshRenderData,
            const AZ::Frustum& frustum, const AZ::Vector3& cameraPos,
            const AZ::Matrix4x4& objectToWorld, const AZ::Matrix4x4& worldToClip)
        {
            if (!EnsureGpuCullResources(instance, meshRenderData))
            {
                // GPU cull not ready yet (cull pass/shader still loading). Fall back to
                // the whole-mesh packet so the model still renders rather than vanishing.
                if (!instance.DrawPacket)
                {
                    BuildInstanceDrawPacket(instance, meshRenderData);
                }
                return;
            }
            auto& srg = instance.CullSrg;

            // Object->world as 3 rows (shader does explicit row*point -- no matrix major-ness).
            const RHI::ShaderInputConstantIndex r0 = srg->FindShaderInputConstantIndex(Name("m_worldRow0"));
            const RHI::ShaderInputConstantIndex r1 = srg->FindShaderInputConstantIndex(Name("m_worldRow1"));
            const RHI::ShaderInputConstantIndex r2 = srg->FindShaderInputConstantIndex(Name("m_worldRow2"));
            srg->SetConstant(r0, objectToWorld.GetRow(0));
            srg->SetConstant(r1, objectToWorld.GetRow(1));
            srg->SetConstant(r2, objectToWorld.GetRow(2));

            // 6 world-space frustum planes (xyz=inward normal, w=d) -- same convention as
            // the CPU IntersectSphere path so GPU/CPU cull agree.
            {
                float planeData[24];
                for (int i = 0; i < AZ::Frustum::PlaneId::MAX; ++i)
                {
                    const AZ::Vector4 coeffs =
                        frustum.GetPlane(static_cast<AZ::Frustum::PlaneId>(i)).GetPlaneEquationCoefficients();
                    coeffs.StoreToFloat4(&planeData[i * 4]);
                }
                const RHI::ShaderInputConstantIndex planesIdx = srg->FindShaderInputConstantIndex(Name("m_frustumPlanes"));
                srg->SetConstantRaw(planesIdx, planeData, static_cast<uint32_t>(sizeof(planeData)));
            }

            const RHI::ShaderInputConstantIndex camIdx = srg->FindShaderInputConstantIndex(Name("m_cameraPosition"));
            srg->SetConstant(camIdx, AZ::Vector4::CreateFromVector3(cameraPos));

            // Phase 6 DAG cut: only while active may m_clusterCount span the FULL
            // leaf+interior range (GpuCullClusterCount, sized at EnsureGpuCullResources);
            // otherwise clamp to leaves so interiors never draw through this path.
            const bool dagCutActive = r_meshletsDagLod && m_dagBindValid &&
                meshRenderData.DagClusterCount > 0 && meshRenderData.DagNodesBuffer;
            const uint32_t shaderClusterCount = dagCutActive
                ? instance.GpuCullClusterCount
                : AZStd::GetMin(instance.GpuCullClusterCount, meshRenderData.MeshletsCount);
            const RHI::ShaderInputConstantIndex countIdx = srg->FindShaderInputConstantIndex(Name("m_clusterCount"));
            srg->SetConstant(countIdx, shaderClusterCount);
            srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_doDagCut")), dagCutActive ? 1u : 0u);
            srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_dagProjScale")), m_dagProjScale);
            srg->SetConstant(
                srg->FindShaderInputConstantIndex(Name("m_dagErrorPx")), static_cast<float>(r_meshletsDagErrorPx));
            const RHI::ShaderInputConstantIndex frustIdx = srg->FindShaderInputConstantIndex(Name("m_doFrustumCull"));
            srg->SetConstant(frustIdx, m_debugControls.m_frustumCull ? 1u : 0u);
            const RHI::ShaderInputConstantIndex coneIdx = srg->FindShaderInputConstantIndex(Name("m_doConeCull"));
            srg->SetConstant(coneIdx, m_debugControls.m_coneCull ? 1u : 0u);

            // HiZ compares in the pyramid's own clip space -- use its frame matrix.
            const RHI::ShaderInputConstantIndex worldToClipIdx = srg->FindShaderInputConstantIndex(Name("m_worldToClip"));
            srg->SetConstant(worldToClipIdx, m_hiZBindValid ? m_hiZBindWorldToClip : worldToClip);
            // Per-cluster HiZ occlusion: bind the last-completed pyramid when available.
            // Harmless when m_hiZTexture is unbound (the shader's own dimension check
            // no-ops) -- see MeshletsCull.azsl.
            if (m_hiZBindValid)
            {
                srg->SetImage(srg->FindShaderInputImageIndex(Name("m_hiZTexture")), m_hiZBindImage);
            }
            const RHI::ShaderInputConstantIndex hiZIdx = srg->FindShaderInputConstantIndex(Name("m_doHiZCull"));
            srg->SetConstant(hiZIdx, (m_debugControls.m_hiZCull && m_hiZBindValid) ? 1u : 0u);

            // NOTE: do NOT Compile() the SRG here. The m_outArgs UAV must be bound to its
            // scope-backed view and the SRG compiled inside the cull compute pass's
            // CompileResources (after the buffer is imported as a frame attachment). The
            // constants/SRVs set above are staged and flushed by that single compile.

            // Queue the dispatch. The compute pass declares BOTH output buffers ReadWrite
            // (it UAV-writes them) and finalizes the cull SRG (binds m_outCommands + m_outCount
            // from scope-backed views, then compiles). The barrier pass transitions both to
            // Indirect (the DrawIndexedIndirectCount consumes the command + count buffers).
            if (RHI::DispatchItem* di = instance.CullDispatchItem.GetDispatchItem())
            {
                m_cullDispatchItemsScratch.push_back(di);
            }
            // This instance re-culls this frame: declare its buffers on BOTH the compute
            // pass (ReadWrite + finalize) and the barrier pass (transition to read state).
            AppendGpuCullAttachments(instance, meshRenderData.IndexCount, m_cullArgsAttachmentsScratch);
            AppendGpuCullAttachments(instance, meshRenderData.IndexCount, m_cullBarrierAttachmentsScratch);

            // Build the packet once (the GPU geometry view + compacted-index SRG binding
            // are static; the compute refills the buffers each frame).
            if (!instance.DrawPacket)
            {
                BuildInstanceDrawPacket(instance, meshRenderData);
            }
        }

        void MeshletsFeatureProcessor::AppendGpuCullAttachments(
            MeshletsRenderInstance& instance, [[maybe_unused]] uint32_t indexCount,
            AZStd::vector<MeshletsImportedAttachment>& outList)
        {
            // Command buffer: up to clusterCount DrawIndexedIndirect commands. Compute writes
            // it (ReadWrite + finalize m_outCommands); barrier pass transitions it to Indirect
            // so the DrawIndexedIndirectCount consumes it.
            MeshletsImportedAttachment cmdAtt;
            cmdAtt.m_attachmentId = instance.CullArgsAttachmentId;
            cmdAtt.m_rhiBuffer = instance.CullCommandBuffer->GetRHIBuffer();
            cmdAtt.m_viewDescriptor = RHI::BufferViewDescriptor::CreateStructured(
                0, instance.GpuCullClusterCount, 5u * static_cast<uint32_t>(sizeof(AZ::u32)));
            cmdAtt.m_finalizeSrg = instance.CullSrg;
            cmdAtt.m_finalizeInputName = Name("m_outCommands");
            cmdAtt.m_barrierUsage = RHI::ScopeAttachmentUsage::Indirect;
            cmdAtt.m_barrierStage = RHI::ScopeAttachmentStage::DrawIndirect;
            outList.push_back(cmdAtt);

            // Count buffer: 1 u32, consumed as the indirect draw's count.
            MeshletsImportedAttachment cntAtt;
            cntAtt.m_attachmentId = instance.CullCountAttachmentId;
            cntAtt.m_rhiBuffer = instance.CullCountBuffer->GetRHIBuffer();
            cntAtt.m_viewDescriptor = RHI::BufferViewDescriptor::CreateStructured(
                0, 1, static_cast<uint32_t>(sizeof(AZ::u32)));
            cntAtt.m_finalizeSrg = instance.CullSrg;
            cntAtt.m_finalizeInputName = Name("m_outCount");
            cntAtt.m_barrierUsage = RHI::ScopeAttachmentUsage::Indirect;
            cntAtt.m_barrierStage = RHI::ScopeAttachmentStage::DrawIndirect;
            outList.push_back(cntAtt);
        }

        bool MeshletsFeatureProcessor::InitShadowShader()
        {
            // The shadow pass doesn't need a gem-private pass. The standard
            // ShadowParent system picks up any DrawItem with tag "shadow".
            // We just need to load the shader, build a PSO, and acquire the tag.
            const char* shaderPath = "Shaders/MeshletsShadowPass.azshader";
            Data::Asset<RPI::ShaderAsset> shaderAsset =
                RPI::AssetUtils::LoadAssetByProductPath<RPI::ShaderAsset>(
                    shaderPath, RPI::AssetUtils::TraceLevel::Warning);
            if (!shaderAsset.GetId().IsValid())
            {
                AZ_Warning("Meshlets", false,
                    "Shadow shader '%s' not found. Meshlets will not cast shadows.", shaderPath);
                m_shadowShader = nullptr;
                m_shadowPipelineState = nullptr;
                return false;
            }

            m_shadowShader = RPI::Shader::FindOrCreate(shaderAsset);
            if (!m_shadowShader)
            {
                AZ_Warning("Meshlets", false,
                    "Failed to create shadow shader instance from '%s'.", shaderPath);
                return false;
            }

            // Acquire the "shadow" DrawListTag from the shader's .shader descriptor
            m_shadowDrawListTag = m_shadowShader->GetDrawListTag();

            // Build the pipeline state for the shadow draw item.
            // Shadow maps use their own render targets (per-cascade), so we cannot
            // call SetOutputFromPass here -- the PSO render target config is set up
            // by the shadow system at draw time. The shader variant's default state
            // plus the scene's ConfigurePipelineState provides the correct depth bias,
            // cull mode, and depth compare function.
            const RPI::ShaderVariant& shaderVariant =
                m_shadowShader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
            RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);

            RPI::Scene* scene = GetParentScene();
            if (scene)
            {
                if (!scene->ConfigurePipelineState(m_shadowDrawListTag, pipelineStateDescriptor))
                {
                    AZ_Warning("Meshlets", false,
                        "InitShadowShader: Scene::ConfigurePipelineState returned false for "
                        "DrawListTag 'shadow' (tag=%u). The shadow render attachment "
                        "configuration will be empty -- the PSO may fail to compile on "
                        "some RHI backends. This usually means the shadow passes haven't "
                        "registered their pipeline state yet. Will retry on next "
                        "pipeline change.",
                        m_shadowDrawListTag.GetIndex());
                    // Don't abort -- AcquirePipelineState may still succeed on
                    // backends that tolerate an empty attachment config for
                    // depth-only passes, and the PSO is re-created on every
                    // pipeline-changed event anyway.
                }
            }

            // PERF (hardware input-assembly): POSITION-only stream layout, exactly like
            // InitDepthShader. The shadow VS now reads a hardware POSITION channel instead of
            // pulling from a StructuredBuffer; the per-mesh PositionIaBuffer feeds it via the
            // geometry view's POSITION StreamBufferView + a POSITION-only DrawRequest index set.
            {
                RHI::InputStreamLayoutBuilder layoutBuilder;
                layoutBuilder.AddBuffer()->Channel("POSITION", RHI::Format::R32G32B32_FLOAT);
                m_shadowInputLayout = layoutBuilder.End();
                pipelineStateDescriptor.m_inputStreamLayout = m_shadowInputLayout;
            }

            // Back-face culling (CCW front), same as depth/forward passes
            pipelineStateDescriptor.m_renderStates.m_rasterState.m_cullMode =
                AZ::RHI::CullMode::Back;

            m_shadowPipelineState = m_shadowShader->AcquirePipelineState(pipelineStateDescriptor);
            if (!m_shadowPipelineState)
            {
                AZ_Warning("Meshlets", false,
                    "Failed to acquire shadow pipeline state. Meshlets will not cast shadows.");
                return false;
            }

            AZ_TracePrintf("Meshlets",
                "InitShadowShader: OK -- shadowDrawListTag=%u, pipelineState=%p\n",
                m_shadowDrawListTag.GetIndex(), m_shadowPipelineState);
            return true;
        }

        bool MeshletsFeatureProcessor::InitDepthShader()
        {
            // Like shadow/forward: the meshlet writes depth via a standard
            // "depth"-tagged DrawItem that Atom's early DepthPrePass renders into
            // the main depth buffer. No gem-private pass needed -- we only build the
            // PSO + acquire the tag. This is what makes meshlets occlude correctly
            // and receive shadows (the late gem depth pass ran after the
            // depth-consuming effects, causing translucency + shadow leak-through).
            const char* shaderPath = "Shaders/MeshletsDepthPass.azshader";
            Data::Asset<RPI::ShaderAsset> shaderAsset =
                RPI::AssetUtils::LoadAssetByProductPath<RPI::ShaderAsset>(
                    shaderPath, RPI::AssetUtils::TraceLevel::Warning);
            if (!shaderAsset.GetId().IsValid())
            {
                AZ_Warning("Meshlets", false,
                    "Depth shader '%s' not found. Meshlets will not write the depth prepass.", shaderPath);
                m_depthShader = nullptr;
                m_depthPipelineState = nullptr;
                return false;
            }

            m_depthShader = RPI::Shader::FindOrCreate(shaderAsset);
            if (!m_depthShader)
            {
                AZ_Warning("Meshlets", false,
                    "Failed to create depth shader instance from '%s'.", shaderPath);
                return false;
            }

            // Acquire the "depth" DrawListTag from the shader's .shader descriptor.
            m_depthDrawListTag = m_depthShader->GetDrawListTag();

            const RPI::ShaderVariant& shaderVariant =
                m_depthShader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
            RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);

            // Pull the DepthPrePass render-attachment configuration (depth-only,
            // reverse-Z, MSAA) so the PSO is compatible with that pass's scope.
            RPI::Scene* scene = GetParentScene();
            if (scene)
            {
                if (!scene->ConfigurePipelineState(m_depthDrawListTag, pipelineStateDescriptor))
                {
                    AZ_Warning("Meshlets", false,
                        "InitDepthShader: Scene::ConfigurePipelineState returned false for "
                        "DrawListTag 'depth' (tag=%u). The depth render attachment config "
                        "will be empty; will retry on next pipeline change.",
                        m_depthDrawListTag.GetIndex());
                }
            }

            // PERF (hardware input-assembly): POSITION stream layout. The depth VS now
            // reads a hardware POSITION channel instead of pulling 3 scalar loads from a
            // StructuredBuffer. The index buffer still drives vertex reuse. Per-mesh
            // PositionIaBuffer + DrawRequest stream indices feed this; meshes whose IA
            // buffer failed to allocate skip the depth item (no stale-channel hang).
            {
                RHI::InputStreamLayoutBuilder layoutBuilder;
                layoutBuilder.AddBuffer()->Channel("POSITION", RHI::Format::R32G32B32_FLOAT);
                m_depthInputLayout = layoutBuilder.End();
                pipelineStateDescriptor.m_inputStreamLayout = m_depthInputLayout;
            }

            // Back-face culling (CCW front), same as shadow/forward passes.
            pipelineStateDescriptor.m_renderStates.m_rasterState.m_cullMode =
                AZ::RHI::CullMode::Back;

            m_depthPipelineState = m_depthShader->AcquirePipelineState(pipelineStateDescriptor);
            if (!m_depthPipelineState)
            {
                AZ_Warning("Meshlets", false,
                    "Failed to acquire depth pipeline state. Meshlets will not write the depth prepass.");
                return false;
            }

            AZ_TracePrintf("Meshlets",
                "InitDepthShader: OK -- depthDrawListTag=%u, pipelineState=%p\n",
                m_depthDrawListTag.GetIndex(), m_depthPipelineState);
            return true;
        }

        bool MeshletsFeatureProcessor::InitForwardShader()
        {
            // The forward PBR pass doesn't need a gem-private pass. The standard
            // Atom ForwardPass picks up any DrawItem tagged "forward" and binds
            // the ForwardPassSrg (shadowmaps, tile light data, BRDF) for every
            // draw item it submits. We just load the shader, build a PSO whose
            // render-attachment config matches the forward pass, and acquire the
            // "forward" tag. Mirrors InitShadowShader.
            const char* shaderPath = "Shaders/MeshletsForwardPass.azshader";
            Data::Asset<RPI::ShaderAsset> shaderAsset =
                RPI::AssetUtils::LoadAssetByProductPath<RPI::ShaderAsset>(
                    shaderPath, RPI::AssetUtils::TraceLevel::Warning);
            if (!shaderAsset.GetId().IsValid())
            {
                AZ_Warning("Meshlets", false,
                    "Forward shader '%s' not found. Meshlets will fall back to the debug render shader.",
                    shaderPath);
                m_forwardShader = nullptr;
                m_forwardPipelineState = nullptr;
                return false;
            }

            m_forwardShader = RPI::Shader::FindOrCreate(shaderAsset);
            if (!m_forwardShader)
            {
                AZ_Warning("Meshlets", false,
                    "Failed to create forward shader instance from '%s'.", shaderPath);
                return false;
            }

            // Acquire the "forward" DrawListTag from the shader's .shader descriptor.
            m_forwardDrawListTag = m_forwardShader->GetDrawListTag();

            const RPI::ShaderVariant& shaderVariant =
                m_forwardShader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
            RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;

            // The forward shader has ~29 lighting options implemented as
            // specialization constants. Bake them with their default values
            // (shadows on, all light types on, IBL on, opacity = Opaque) -- these
            // are exactly the full-featured defaults we want. Passing the option
            // group also silences the "Configuring PipelineStateDescriptor
            // without specializing option o_..." errors that the no-arg overload
            // produces for shaders that use specialization constants.
            RPI::ShaderOptionGroup forwardOptions = m_forwardShader->GetDefaultShaderOptions();
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor, forwardOptions);

            // Pull the forward pass's render-attachment configuration (5 MRTs +
            // depth/stencil) and multisample state so the PSO is compatible with
            // the standard ForwardPass's output merger.
            RPI::Scene* scene = GetParentScene();
            if (scene)
            {
                if (!scene->ConfigurePipelineState(m_forwardDrawListTag, pipelineStateDescriptor))
                {
                    AZ_Warning("Meshlets", false,
                        "InitForwardShader: Scene::ConfigurePipelineState returned false for "
                        "DrawListTag 'forward' (tag=%u). The forward render attachment "
                        "configuration will be empty and the PSO will likely fail to "
                        "compile. This usually means the forward pass hasn't registered "
                        "its pipeline state yet; will retry on next pipeline change.",
                        m_forwardDrawListTag.GetIndex());
                }
            }

            // PERF (hardware input-assembly): 5-channel stream layout. Each attribute lives in
            // its OWN AddBuffer() (= a separate vertex buffer), matching the dedicated per-
            // attribute IA buffers created in EnsureIndirectArgs. The channel ORDER here MUST
            // match the geometry views' AddStreamBufferView order exactly
            // (POSITION,NORMAL,TANGENT,BITANGENT,UV) -- a mismatch passes validation but renders
            // garbage. Formats mirror the pack's vertex-stream formats.
            {
                RHI::InputStreamLayoutBuilder layoutBuilder;
                layoutBuilder.AddBuffer()->Channel("POSITION",  RHI::Format::R32G32B32_FLOAT);
                layoutBuilder.AddBuffer()->Channel("NORMAL",    RHI::Format::R32G32B32_FLOAT);
                layoutBuilder.AddBuffer()->Channel("TANGENT",   RHI::Format::R32G32B32A32_FLOAT);
                layoutBuilder.AddBuffer()->Channel("BITANGENT", RHI::Format::R32G32B32_FLOAT);
                layoutBuilder.AddBuffer()->Channel("UV",        RHI::Format::R32G32_FLOAT);
                m_forwardInputLayout = layoutBuilder.End();
                pipelineStateDescriptor.m_inputStreamLayout = m_forwardInputLayout;
            }

            // Back-face culling (CCW front), same as depth/shadow passes.
            pipelineStateDescriptor.m_renderStates.m_rasterState.m_cullMode =
                AZ::RHI::CullMode::Back;

            m_forwardPipelineState = m_forwardShader->AcquirePipelineState(pipelineStateDescriptor);
            if (!m_forwardPipelineState)
            {
                AZ_Warning("Meshlets", false,
                    "Failed to acquire forward pipeline state. Meshlets will fall back to the debug render shader.");
                return false;
            }

            AZ_TracePrintf("Meshlets",
                "InitForwardShader: OK -- forwardDrawListTag=%u, pipelineState=%p\n",
                m_forwardDrawListTag.GetIndex(), m_forwardPipelineState);
            return true;
        }

        bool MeshletsFeatureProcessor::InitMeshForwardShader()
        {
            if (m_meshForwardPipelineState)
            {
                return true;   // already initialized
            }

            // Phase 5 (hardware mesh shader): Mesh + Fragment entry points, rendered by
            // the STANDARD ForwardPass via the same "forward" tag. Mirrors
            // InitForwardShader except there is NO input-stream layout -- the mesh path
            // has no input assembler (the DX12 backend builds a mesh stream-PSO when
            // the variant carries a mesh function).
            const char* shaderPath = "Shaders/MeshletsForwardMeshShader.azshader";
            Data::Asset<RPI::ShaderAsset> shaderAsset =
                RPI::AssetUtils::LoadAssetByProductPath<RPI::ShaderAsset>(
                    shaderPath, RPI::AssetUtils::TraceLevel::Warning);
            if (!shaderAsset.GetId().IsValid())
            {
                // Not processed yet (or missing) -- retry on the next pipeline change.
                return false;
            }

            m_meshForwardShader = RPI::Shader::FindOrCreate(shaderAsset);
            if (!m_meshForwardShader)
            {
                AZ_Warning("Meshlets", false,
                    "Failed to create mesh-shader forward shader instance from '%s'.", shaderPath);
                return false;
            }

            m_meshForwardDrawListTag = m_meshForwardShader->GetDrawListTag();

            const RPI::ShaderVariant& shaderVariant =
                m_meshForwardShader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
            RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;
            RPI::ShaderOptionGroup meshOptions = m_meshForwardShader->GetDefaultShaderOptions();
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor, meshOptions);

            // Pull the forward pass's render-attachment configuration (5 MRTs +
            // depth/stencil) + multisample state -- the mesh stream-PSO needs them
            // exactly like the classic graphics PSO does.
            RPI::Scene* scene = GetParentScene();
            if (scene)
            {
                if (!scene->ConfigurePipelineState(m_meshForwardDrawListTag, pipelineStateDescriptor))
                {
                    AZ_Warning("Meshlets", false,
                        "InitMeshForwardShader: Scene::ConfigurePipelineState returned false for "
                        "the forward DrawListTag (tag=%u); will retry on next pipeline change.",
                        m_meshForwardDrawListTag.GetIndex());
                }
            }

            // NO m_inputStreamLayout -- the mesh-shader pipeline has no input assembler.
            // Back-face culling to match the vertex-pull forward pass.
            pipelineStateDescriptor.m_renderStates.m_rasterState.m_cullMode =
                AZ::RHI::CullMode::Back;

            m_meshForwardPipelineState = m_meshForwardShader->AcquirePipelineState(pipelineStateDescriptor);
            if (!m_meshForwardPipelineState)
            {
                AZ_Warning("Meshlets", false,
                    "Failed to acquire mesh-shader forward pipeline state. "
                    "r_meshletsHwMeshShader will fall back to the vertex-pull path.");
                return false;
            }

            AZ_TracePrintf("Meshlets",
                "InitMeshForwardShader: OK -- meshForwardDrawListTag=%u, pipelineState=%p\n",
                m_meshForwardDrawListTag.GetIndex(), m_meshForwardPipelineState);
            return true;
        }

        bool MeshletsFeatureProcessor::InitMeshDepthShader()
        {
            if (m_meshDepthPipelineState)
            {
                return true;   // already initialized
            }

            // Hardware mesh-shader depth prepass: a single Mesh entry, no Fragment.
            // Mirrors InitMeshForwardShader (no input-stream layout) but pulls the
            // DEPTH pass's render-attachment config instead of the forward pass's.
            const char* shaderPath = "Shaders/MeshletsDepthMeshShader.azshader";
            Data::Asset<RPI::ShaderAsset> shaderAsset =
                RPI::AssetUtils::LoadAssetByProductPath<RPI::ShaderAsset>(
                    shaderPath, RPI::AssetUtils::TraceLevel::Warning);
            if (!shaderAsset.GetId().IsValid())
            {
                // Not processed yet (or missing) -- retry on the next pipeline change.
                return false;
            }

            m_meshDepthShader = RPI::Shader::FindOrCreate(shaderAsset);
            if (!m_meshDepthShader)
            {
                AZ_Warning("Meshlets", false,
                    "Failed to create mesh-shader depth shader instance from '%s'.", shaderPath);
                return false;
            }

            // Reuse the existing "depth" DrawListTag -- both MeshletsDepthPass.shader
            // and MeshletsDepthMeshShader.shader declare "DrawList":"depth", so the
            // tags are identical and Atom's early DepthPrePass picks this item up.
            const RHI::DrawListTag meshDepthDrawListTag = m_meshDepthShader->GetDrawListTag();
            if (!m_depthDrawListTag.IsValid())
            {
                m_depthDrawListTag = meshDepthDrawListTag;
            }

            const RPI::ShaderVariant& shaderVariant =
                m_meshDepthShader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
            RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);

            RPI::Scene* scene = GetParentScene();
            if (scene)
            {
                if (!scene->ConfigurePipelineState(meshDepthDrawListTag, pipelineStateDescriptor))
                {
                    AZ_Warning("Meshlets", false,
                        "InitMeshDepthShader: Scene::ConfigurePipelineState returned false for "
                        "the depth DrawListTag (tag=%u); will retry on next pipeline change.",
                        meshDepthDrawListTag.GetIndex());
                }
            }

            // NO m_inputStreamLayout -- the mesh-shader pipeline has no input assembler.
            // Back-face culling to match the vertex-pull depth prepass.
            pipelineStateDescriptor.m_renderStates.m_rasterState.m_cullMode =
                AZ::RHI::CullMode::Back;

            m_meshDepthPipelineState = m_meshDepthShader->AcquirePipelineState(pipelineStateDescriptor);
            if (!m_meshDepthPipelineState)
            {
                AZ_Warning("Meshlets", false,
                    "Failed to acquire mesh-shader depth pipeline state. Meshlets will render "
                    "without a depth prepass contribution (translucent-looking under DoF).");
                return false;
            }

            AZ_TracePrintf("Meshlets",
                "InitMeshDepthShader: OK -- depthDrawListTag=%u, pipelineState=%p\n",
                meshDepthDrawListTag.GetIndex(), m_meshDepthPipelineState);
            return true;
        }

        bool MeshletsFeatureProcessor::InitMeshMotionShader()
        {
            if (m_meshMotionPipelineState)
            {
                return true;   // already initialized
            }

            // Hardware mesh-shader motion-vector pass. Mirrors InitMeshDepthShader, but
            // this one exists for CORRECTNESS as much as speed: motion DrawItems are added
            // by BuildInstanceDrawPacket, which the mesh-shader path short-circuits, so a
            // mesh-shader meshlet previously contributed nothing to the motion buffer and
            // ghosted under TAA and any temporal upscaler.
            const char* shaderPath = "Shaders/MeshletsMotionVectorMeshShader.azshader";
            Data::Asset<RPI::ShaderAsset> shaderAsset =
                RPI::AssetUtils::LoadAssetByProductPath<RPI::ShaderAsset>(
                    shaderPath, RPI::AssetUtils::TraceLevel::Warning);
            if (!shaderAsset.GetId().IsValid())
            {
                // Not processed yet (or missing) - retry on the next pipeline change.
                return false;
            }

            m_meshMotionShader = RPI::Shader::FindOrCreate(shaderAsset);
            if (!m_meshMotionShader)
            {
                AZ_Warning("Meshlets", false,
                    "Failed to create mesh-shader motion instance from '%s'.", shaderPath);
                return false;
            }

            // Reuse the existing "motion" DrawListTag - both MeshletsMotionVector.shader and
            // MeshletsMotionVectorMeshShader.shader declare "DrawList":"motion", so the tags
            // are identical and Atom's MotionVectorPass picks this item up unchanged.
            const RHI::DrawListTag meshMotionDrawListTag = m_meshMotionShader->GetDrawListTag();
            if (!m_motionDrawListTag.IsValid())
            {
                m_motionDrawListTag = meshMotionDrawListTag;
            }

            const RPI::ShaderVariant& shaderVariant =
                m_meshMotionShader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
            RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);

            RPI::Scene* scene = GetParentScene();
            if (scene)
            {
                if (!scene->ConfigurePipelineState(meshMotionDrawListTag, pipelineStateDescriptor))
                {
                    AZ_Warning("Meshlets", false,
                        "InitMeshMotionShader: Scene::ConfigurePipelineState returned false for "
                        "the motion DrawListTag (tag=%u); will retry on next pipeline change.",
                        meshMotionDrawListTag.GetIndex());
                }
            }

            // NO m_inputStreamLayout - the mesh-shader pipeline has no input assembler.
            pipelineStateDescriptor.m_renderStates.m_rasterState.m_cullMode =
                AZ::RHI::CullMode::Back;

            m_meshMotionPipelineState = m_meshMotionShader->AcquirePipelineState(pipelineStateDescriptor);
            if (!m_meshMotionPipelineState)
            {
                AZ_Warning("Meshlets", false,
                    "Failed to acquire mesh-shader motion pipeline state. Meshlets will emit no "
                    "motion vectors on the mesh path and will ghost under TAA.");
                return false;
            }

            AZ_TracePrintf("Meshlets",
                "InitMeshMotionShader: OK - motionDrawListTag=%u, pipelineState=%p\n",
                meshMotionDrawListTag.GetIndex(), m_meshMotionPipelineState);
            return true;
        }

        bool MeshletsFeatureProcessor::InitMeshShadowShader()
        {
            if (m_meshShadowPipelineState)
            {
                return true;   // already initialized
            }

            // Hardware mesh-shader shadow pass. Mirrors InitMeshDepthShader but pulls the
            // SHADOW pass render-attachment config. Deliberately non-culling: a light view
            // must emit every cluster, so this PSO pairs with MeshShaderGeometryView (all
            // clusters), never MeshShaderCullGeometryView.
            const char* shaderPath = "Shaders/MeshletsShadowMeshShader.azshader";
            Data::Asset<RPI::ShaderAsset> shaderAsset =
                RPI::AssetUtils::LoadAssetByProductPath<RPI::ShaderAsset>(
                    shaderPath, RPI::AssetUtils::TraceLevel::Warning);
            if (!shaderAsset.GetId().IsValid())
            {
                return false;   // not processed yet - retry on the next pipeline change
            }

            m_meshShadowShader = RPI::Shader::FindOrCreate(shaderAsset);
            if (!m_meshShadowShader)
            {
                AZ_Warning("Meshlets", false,
                    "Failed to create mesh-shader shadow instance from '%s'.", shaderPath);
                return false;
            }

            // Same "shadow" tag as the vertex-pull MeshletsShadowPass.shader, so every
            // shadowmap pass (cascades, projected) picks this item up unchanged.
            const RHI::DrawListTag meshShadowDrawListTag = m_meshShadowShader->GetDrawListTag();
            if (!m_shadowDrawListTag.IsValid())
            {
                m_shadowDrawListTag = meshShadowDrawListTag;
            }

            const RPI::ShaderVariant& shaderVariant =
                m_meshShadowShader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
            RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);

            RPI::Scene* scene = GetParentScene();
            if (scene)
            {
                if (!scene->ConfigurePipelineState(meshShadowDrawListTag, pipelineStateDescriptor))
                {
                    AZ_Warning("Meshlets", false,
                        "InitMeshShadowShader: Scene::ConfigurePipelineState returned false for "
                        "the shadow DrawListTag (tag=%u); will retry on next pipeline change.",
                        meshShadowDrawListTag.GetIndex());
                }
            }

            // NO m_inputStreamLayout - the mesh-shader pipeline has no input assembler.
            pipelineStateDescriptor.m_renderStates.m_rasterState.m_cullMode =
                AZ::RHI::CullMode::Back;

            m_meshShadowPipelineState = m_meshShadowShader->AcquirePipelineState(pipelineStateDescriptor);
            if (!m_meshShadowPipelineState)
            {
                AZ_Warning("Meshlets", false,
                    "Failed to acquire mesh-shader shadow pipeline state; falling back to the "
                    "vertex-pull shadow path.");
                return false;
            }

            AZ_TracePrintf("Meshlets",
                "InitMeshShadowShader: OK - shadowDrawListTag=%u, pipelineState=%p\n",
                meshShadowDrawListTag.GetIndex(), m_meshShadowPipelineState);
            return true;
        }

        bool MeshletsFeatureProcessor::InitMeshCullForwardShader()
        {
            if (m_meshCullForwardPipelineState)
            {
                return true;   // already initialized
            }

            // Phase 5 AS/triangle cull: Amplification + Mesh + Fragment entry points, a
            // SEPARATE shader/PSO from m_meshForwardShader compiled from the SAME .azsl
            // source (see MeshletsForwardMeshShaderCulled.shader for why it must be
            // separate). Same "forward" DrawListTag string, so it reuses
            // m_meshForwardDrawListTag (callers gate on InitMeshForwardShader() having
            // already run, which is guaranteed by BuildMeshShaderDrawPacket).
            const char* shaderPath = "Shaders/MeshletsForwardMeshShaderCulled.azshader";
            Data::Asset<RPI::ShaderAsset> shaderAsset =
                RPI::AssetUtils::LoadAssetByProductPath<RPI::ShaderAsset>(
                    shaderPath, RPI::AssetUtils::TraceLevel::Warning);
            if (!shaderAsset.GetId().IsValid())
            {
                // Not processed yet (or missing) -- retry on the next pipeline change.
                return false;
            }

            m_meshCullForwardShader = RPI::Shader::FindOrCreate(shaderAsset);
            if (!m_meshCullForwardShader)
            {
                AZ_Warning("Meshlets", false,
                    "Failed to create AS-cull mesh-shader forward shader instance from '%s'.", shaderPath);
                return false;
            }

            const RHI::DrawListTag cullDrawListTag = m_meshCullForwardShader->GetDrawListTag();

            const RPI::ShaderVariant& shaderVariant =
                m_meshCullForwardShader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
            RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;
            RPI::ShaderOptionGroup meshOptions = m_meshCullForwardShader->GetDefaultShaderOptions();
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor, meshOptions);

            RPI::Scene* scene = GetParentScene();
            if (scene)
            {
                if (!scene->ConfigurePipelineState(cullDrawListTag, pipelineStateDescriptor))
                {
                    AZ_Warning("Meshlets", false,
                        "InitMeshCullForwardShader: Scene::ConfigurePipelineState returned false for "
                        "the forward DrawListTag (tag=%u); will retry on next pipeline change.",
                        cullDrawListTag.GetIndex());
                }
            }

            // NO m_inputStreamLayout (no input assembler). Back-face culling to match
            // the vertex-pull forward pass and the default mesh-shader PSO -- the
            // per-triangle backface test in MeshletsForwardPassMSCulled is a redundant
            // (earlier, cheaper) pre-filter, not a replacement for this.
            pipelineStateDescriptor.m_renderStates.m_rasterState.m_cullMode =
                AZ::RHI::CullMode::Back;

            m_meshCullForwardPipelineState = m_meshCullForwardShader->AcquirePipelineState(pipelineStateDescriptor);
            if (!m_meshCullForwardPipelineState)
            {
                AZ_Warning("Meshlets", false,
                    "Failed to acquire AS-cull mesh-shader pipeline state. "
                    "r_meshletsMsCullAS will fall back to the uncull mesh-shader path.");
                return false;
            }

            AZ_TracePrintf("Meshlets",
                "InitMeshCullForwardShader: OK -- pipelineState=%p\n", m_meshCullForwardPipelineState);
            return true;
        }

        bool MeshletsFeatureProcessor::InitCulledMeshVariant(
            const char* shaderPath, const char* label,
            Data::Instance<RPI::Shader>& shaderOut, const RHI::PipelineState*& pipelineStateOut)
        {
            if (pipelineStateOut)
            {
                return true;   // already initialized
            }

            // Same lazy-retry pattern as InitMeshCullForwardShader: the *Culled.shader
            // asset may not be processed yet -- safe no-op until it is.
            Data::Asset<RPI::ShaderAsset> shaderAsset =
                RPI::AssetUtils::LoadAssetByProductPath<RPI::ShaderAsset>(
                    shaderPath, RPI::AssetUtils::TraceLevel::Warning);
            if (!shaderAsset.GetId().IsValid())
            {
                return false;
            }

            shaderOut = RPI::Shader::FindOrCreate(shaderAsset);
            if (!shaderOut)
            {
                AZ_Warning("Meshlets", false,
                    "InitCulledMeshVariant(%s): failed to create shader instance from '%s'.", label, shaderPath);
                return false;
            }

            const RHI::DrawListTag drawListTag = shaderOut->GetDrawListTag();
            const RPI::ShaderVariant& shaderVariant =
                shaderOut->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
            RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);

            if (RPI::Scene* scene = GetParentScene())
            {
                if (!scene->ConfigurePipelineState(drawListTag, pipelineStateDescriptor))
                {
                    AZ_Warning("Meshlets", false,
                        "InitCulledMeshVariant(%s): Scene::ConfigurePipelineState returned false "
                        "(tag=%u); will retry on next pipeline change.", label, drawListTag.GetIndex());
                }
            }

            // NO m_inputStreamLayout (no input assembler); back-face culling to match
            // every other meshlet PSO.
            pipelineStateDescriptor.m_renderStates.m_rasterState.m_cullMode = AZ::RHI::CullMode::Back;

            pipelineStateOut = shaderOut->AcquirePipelineState(pipelineStateDescriptor);
            if (!pipelineStateOut)
            {
                AZ_Warning("Meshlets", false,
                    "InitCulledMeshVariant(%s): failed to acquire pipeline state.", label);
                return false;
            }

            AZ_TracePrintf("Meshlets", "InitCulledMeshVariant(%s): OK -- pipelineState=%p\n",
                label, pipelineStateOut);
            return true;
        }

        bool MeshletsFeatureProcessor::InitMotionShader()
        {
            // Same standard-tag pattern as depth/shadow/forward: the meshlet emits a
            // "motion"-tagged DrawItem rendered by Atom's standard MeshMotionVector
            // pass. No gem-private pass needed -- only the PSO + tag.
            const char* shaderPath = "Shaders/MeshletsMotionVector.azshader";
            Data::Asset<RPI::ShaderAsset> shaderAsset =
                RPI::AssetUtils::LoadAssetByProductPath<RPI::ShaderAsset>(
                    shaderPath, RPI::AssetUtils::TraceLevel::Warning);
            if (!shaderAsset.GetId().IsValid())
            {
                AZ_Warning("Meshlets", false,
                    "Motion shader '%s' not found. Meshlets will not produce motion vectors "
                    "(TAA may ghost them).", shaderPath);
                m_motionShader = nullptr;
                m_motionPipelineState = nullptr;
                return false;
            }

            m_motionShader = RPI::Shader::FindOrCreate(shaderAsset);
            if (!m_motionShader)
            {
                AZ_Warning("Meshlets", false,
                    "Failed to create motion shader instance from '%s'.", shaderPath);
                return false;
            }

            // Acquire the "motion" DrawListTag from the shader's .shader descriptor.
            m_motionDrawListTag = m_motionShader->GetDrawListTag();

            const RPI::ShaderVariant& shaderVariant =
                m_motionShader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
            RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);

            // Pull the MeshMotionVector pass's render-attachment configuration
            // (R16G16_FLOAT motion target + depth, reverse-Z, MSAA) so the PSO is
            // compatible with that pass's scope.
            RPI::Scene* scene = GetParentScene();
            if (scene)
            {
                if (!scene->ConfigurePipelineState(m_motionDrawListTag, pipelineStateDescriptor))
                {
                    AZ_Warning("Meshlets", false,
                        "InitMotionShader: Scene::ConfigurePipelineState returned false for "
                        "DrawListTag 'motion' (tag=%u). The motion render attachment config "
                        "will be empty; will retry on next pipeline change.",
                        m_motionDrawListTag.GetIndex());
                }
            }

            // PERF (hardware input-assembly): POSITION-only stream layout, exactly like
            // InitDepthShader. The motion VS reads a hardware POSITION channel; the prev-frame
            // transform still comes from SceneSrg (objectId), so motion vectors stay correct.
            {
                RHI::InputStreamLayoutBuilder layoutBuilder;
                layoutBuilder.AddBuffer()->Channel("POSITION", RHI::Format::R32G32B32_FLOAT);
                m_motionInputLayout = layoutBuilder.End();
                pipelineStateDescriptor.m_inputStreamLayout = m_motionInputLayout;
            }

            // Back-face culling (CCW front), same as the other meshlet passes.
            pipelineStateDescriptor.m_renderStates.m_rasterState.m_cullMode =
                AZ::RHI::CullMode::Back;

            m_motionPipelineState = m_motionShader->AcquirePipelineState(pipelineStateDescriptor);
            if (!m_motionPipelineState)
            {
                AZ_Warning("Meshlets", false,
                    "Failed to acquire motion pipeline state. Meshlets will not produce motion vectors.");
                return false;
            }

            AZ_TracePrintf("Meshlets",
                "InitMotionShader: OK -- motionDrawListTag=%u, pipelineState=%p\n",
                m_motionDrawListTag.GetIndex(), m_motionPipelineState);
            return true;
        }

        bool MeshletsFeatureProcessor::InitIndirectDrawSignature()
        {
            if (m_drawIndirectSignature)
            {
                return true;  // Already created (one-time, pipeline-independent).
            }

            // An indexed Draw command per indirect sequence. Indexed (not the
            // non-indexed Draw) because StartVertexLocation does NOT offset
            // SV_VertexID in the vertex-pull (empty-IA) path, whereas an index
            // buffer's StartIndexLocation is well-defined. Matches the 20-byte
            // DrawIndexedIndirectCommand {indexCount, instanceCount, startIndex,
            // baseVertex, startInstance} -- one per visible cluster.
            RHI::IndirectBufferLayout layout;
            layout.AddIndirectCommand(RHI::IndirectCommandDescriptor(RHI::IndirectCommandType::DrawIndexed));
            if (!layout.Finalize())
            {
                AZ_Warning("Meshlets", false, "InitIndirectDrawSignature: failed to finalize indirect layout.");
                return false;
            }

            RHI::IndirectBufferSignatureDescriptor descriptor;
            descriptor.m_layout = layout;
            descriptor.m_pipelineState = nullptr;  // null is valid for a plain Draw command.

            m_drawIndirectSignature = aznew RHI::IndirectBufferSignature;
            const RHI::ResultCode rc =
                m_drawIndirectSignature->Init(RHI::MultiDevice::AllDevices, descriptor);
            if (rc != RHI::ResultCode::Success)
            {
                AZ_Warning("Meshlets", false, "InitIndirectDrawSignature: signature Init failed (rc=%d).", static_cast<int>(rc));
                m_drawIndirectSignature = nullptr;
                return false;
            }

            AZ_TracePrintf("Meshlets",
                "InitIndirectDrawSignature: OK -- byteStride=%u\n",
                m_drawIndirectSignature->GetByteStride());

            // GPU cull single-compacted-draw path: a NON-indexed Draw command (16-byte
            // {vertexCount, instanceCount, startVertex, startInstance}). The cull compute
            // compacts visible clusters' mesh-vertex-indices into a per-instance buffer
            // the VS reads directly (no index buffer), so the draw is non-indexed.
            if (!m_drawNonIndexedSignature)
            {
                RHI::IndirectBufferLayout niLayout;
                niLayout.AddIndirectCommand(RHI::IndirectCommandDescriptor(RHI::IndirectCommandType::Draw));
                if (niLayout.Finalize())
                {
                    RHI::IndirectBufferSignatureDescriptor niDesc;
                    niDesc.m_layout = niLayout;
                    niDesc.m_pipelineState = nullptr;
                    m_drawNonIndexedSignature = aznew RHI::IndirectBufferSignature;
                    if (m_drawNonIndexedSignature->Init(RHI::MultiDevice::AllDevices, niDesc) != RHI::ResultCode::Success)
                    {
                        AZ_Warning("Meshlets", false, "InitIndirectDrawSignature: non-indexed signature Init failed.");
                        m_drawNonIndexedSignature = nullptr;
                    }
                }
            }
            return true;
        }

        bool MeshletsFeatureProcessor::InitRenderPass(const Name& passName)
        {
            m_renderPass = Data::Instance<MeshletsRenderPass>();
            RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassName(passName, m_renderPipeline);
            RPI::Ptr<RPI::Pass> desiredPass = RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);

            if (desiredPass)
            {
                m_renderPass = static_cast<MeshletsRenderPass*>(desiredPass.get());
                m_renderShader = m_renderPass->GetShader();
            }
            else
            {
                AZ_Error("Meshlets", false,
                    "%s does not exist in this pipeline. Check your game project's .pass assets.",
                    passName.GetCStr());
                return false;
            }

            // Components can call AddInstance() before the render pipeline is ready.
            // In that case the instance was created but its DrawPacket was deferred
            // (m_renderPass was null). Now that the pipeline is up, build the packets
            // we owe so those instances actually render.
            for (auto& instance : m_instances)
            {
                if (!instance || instance->DrawPacket || !instance->RenderObject)
                {
                    continue;
                }
                ModelLodDataArray& lodArray = instance->RenderObject->GetMeshletsRenderData(instance->LodIndex);
                if (instance->MeshIndex < lodArray.size() && lodArray[instance->MeshIndex])
                {
                    BuildInstanceDrawPacket(*instance, *lodArray[instance->MeshIndex]);
                }
            }

            return true;
        }

        // Pipeline integration model:
        //   1. Look up MeshletsParentPass in the pipeline. If the project's pipeline
        //      template declared it, use that.
        //   2. Otherwise, attempt to auto-inject after a known opaque-stage pass slot.
        //      This covers vanilla MainPipeline / LowEndPipeline so the gem "just works"
        //      when dropped into a stock project.
        //   3. If neither finds nor injects, self-disable for that pipeline and log
        //      exactly once.
        //
        // Sample wiring lives in Assets/Passes/MeshletsPassRequest.azasset. Projects
        // that need control can either modify their pipeline template directly or
        // disable auto-injection by removing that asset from the gem's deployed assets.
        void MeshletsFeatureProcessor::AddRenderPasses(RPI::RenderPipeline* renderPipeline)
        {
            if (!HasMeshletPasses(renderPipeline))
            {
                TryAutoInjectPasses(renderPipeline);
            }
            // GPU cull: inject the early compute + barrier pass before DepthPrePass.
            // Best-effort and independent of the main meshlet passes -- failure just
            // leaves GPU cull unavailable (CPU cull / whole-mesh paths still work).
            TryAutoInjectCullPass(renderPipeline);
        }

        bool MeshletsFeatureProcessor::TryAutoInjectPasses(RPI::RenderPipeline* renderPipeline)
        {
            if (!renderPipeline)
            {
                return false;
            }

            // Skip one-shot pipelines (BRDF integration, IBL baking, reflection probe
            // captures, etc.). They don't have an opaque-stage and don't render scene
            // geometry; auto-injecting meshlet passes there would always fail and
            // produces noisy warnings.
            if (renderPipeline->IsExecuteOnce())
            {
                return false;
            }

            // Load the pass request asset on first use.
            if (!m_passRequestAsset.GetId().IsValid())
            {
                const char* passRequestAssetFilePath = "Passes/MeshletsPassRequest.azasset";
                m_passRequestAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::AnyAsset>(
                    passRequestAssetFilePath, AZ::RPI::AssetUtils::TraceLevel::Warning);
            }
            if (!m_passRequestAsset || !m_passRequestAsset->IsReady())
            {
                return false;
            }

            const AZ::RPI::PassRequest* passRequest = m_passRequestAsset->GetDataAs<AZ::RPI::PassRequest>();
            if (!passRequest)
            {
                return false;
            }

            RPI::Ptr<RPI::Pass> pass = RPI::PassSystemInterface::Get()->CreatePassFromRequest(passRequest);
            if (!pass)
            {
                return false;
            }

            // Try several known insertion points. Stop at the first one that succeeds.
            // MainPipeline has OpaquePass; LowEndPipeline reaches its post-opaque
            // composition through MSAAResolvePass. ForwardPass is a defensive fallback
            // for custom pipelines that follow the standard naming.
            static const AZStd::array<const char*, 3> candidateInsertionPoints = {
                "OpaquePass", "MSAAResolvePass", "ForwardPass"
            };
            for (const char* insertAfter : candidateInsertionPoints)
            {
                if (renderPipeline->AddPassAfter(pass, Name(insertAfter)))
                {
                    return true;
                }
            }
            return false;
        }

        void MeshletsFeatureProcessor::SetTransform(
            const Render::TransformServiceFeatureProcessorInterface::ObjectId objectId, 
            const AZ::Transform& transform)
        {
            m_transformServiceFeatureProcessor->SetTransformForId(objectId, transform);
        }

        //==============================================================================
        // Per-instance draw packet construction.
        // Builds a DrawPacket bound to the instance's per-draw SRG (containing m_objectId)
        // and the object's per-object SRG (containing the vertex buffers shared by every
        // instance). The compute pass writes one index buffer per object; every instance
        // of that object reads from it.
        //
        // For LODs, today we only build a draw packet for the first LOD (mesh.LOD0). When
        // hierarchical LOD selection lands in Tier 2 the LOD index becomes part of the
        // instance state and the draw packet selection moves into the per-frame loop.
        bool MeshletsFeatureProcessor::EnsureMeshShaderResources(MeshRenderData& meshRenderData, bool forCull)
        {
            // Rebuild when the cull mode flips: the AS-cull and uncull shaders have
            // DIFFERENT MeshletsMeshObjectSrg layouts (azslc --strip-unused-srgs drops
            // m_clusterBounds from the uncull one), so a cached SRG built for the other
            // PSO cannot be reused.
            if (meshRenderData.MeshShaderResourcesReady &&
                meshRenderData.MeshShaderSrgBuiltForCull == forCull)
            {
                return true;
            }
            if (!m_meshForwardShader)
            {
                return false;
            }
            // Create the SRG from the SAME shader asset as the PSO it will be bound to.
            const Data::Instance<RPI::Shader>& srgSourceShader =
                (forCull && m_meshCullForwardShader) ? m_meshCullForwardShader : m_meshForwardShader;
            // Only the AS-cull path (cut active) may dispatch the full DAG range;
            // uncull/shadow stay leaf-only or interiors double-draw on top of leaves.
            const uint32_t sliceClusterCount =
                static_cast<uint32_t>(meshRenderData.PersistentClusterDescriptors.size());
            const uint32_t leafClusterCount = meshRenderData.MeshletsCount;
            const bool dagActive = r_meshletsDagLod && forCull &&
                meshRenderData.DagClusterCount > 0 && meshRenderData.DagNodesBuffer;
            const uint32_t cullClusterCount = dagActive ? meshRenderData.DagClusterCount : leafClusterCount;
            const uint32_t clusterCount = cullClusterCount;   // what the AS tests (m_clusterCount)
            if (sliceClusterCount == 0 || leafClusterCount == 0 || !meshRenderData.ClusterDescBuffer)
            {
                return false;
            }

            // StructuredBuffer<uint> copies of the pack-owned slices -- NOT the typed
            // Buffer<uint> path (AMD NumElements bug).
            if (meshRenderData.ComputeBuffersDescriptors.size() <=
                static_cast<size_t>(ComputeStreamsSemantics::MeshletsIndicesIndirection))
            {
                return false;
            }
            // Phase 4 VRAM reclaim: streaming-exclusive meshes never get the monolithic
            // triangle/indirection copies -- all geometry decode goes through the page
            // pool (the AS refuses any cluster the pool cannot serve).
            if (!meshRenderData.MonolithicDropped)
            {
                if (!meshRenderData.MeshShaderTrianglesBuffer)
                {
                    const SrgBufferDescriptor& src = meshRenderData.ComputeBuffersDescriptors[
                        static_cast<uint8_t>(ComputeStreamsSemantics::MeshletsTriangles)];
                    if (!src.m_bufferData || src.m_elementCount == 0)
                    {
                        return false;
                    }
                    SrgBufferDescriptor d(
                        RPI::CommonBufferPoolType::ReadOnly, RHI::Format::Unknown,
                        RHI::BufferBindFlags::ShaderRead,
                        static_cast<uint32_t>(sizeof(AZ::u32)), src.m_elementCount,
                        Name{ "MeshletsMSTriangles" }, Name{ "m_meshletsTriangles" }, 0, 0,
                        src.m_bufferData);
                    meshRenderData.MeshShaderTrianglesBuffer = UtilityClass::CreateBuffer("Meshlets", d, nullptr);
                }
                if (!meshRenderData.MeshShaderIndirectionBuffer)
                {
                    const SrgBufferDescriptor& src = meshRenderData.ComputeBuffersDescriptors[
                        static_cast<uint8_t>(ComputeStreamsSemantics::MeshletsIndicesIndirection)];
                    if (!src.m_bufferData || src.m_elementCount == 0)
                    {
                        return false;
                    }
                    SrgBufferDescriptor d(
                        RPI::CommonBufferPoolType::ReadOnly, RHI::Format::Unknown,
                        RHI::BufferBindFlags::ShaderRead,
                        static_cast<uint32_t>(sizeof(AZ::u32)), src.m_elementCount,
                        Name{ "MeshletsMSIndirection" }, Name{ "m_meshletsIndicesLookup" }, 0, 0,
                        src.m_bufferData);
                    meshRenderData.MeshShaderIndirectionBuffer = UtilityClass::CreateBuffer("Meshlets", d, nullptr);
                }
                if (!meshRenderData.MeshShaderTrianglesBuffer || !meshRenderData.MeshShaderIndirectionBuffer)
                {
                    AZ_Warning("Meshlets", false,
                        "EnsureMeshShaderResources: failed to create triangle/indirection buffers.");
                    return false;
                }
            }

            // Vertex-stream SRVs: the same proven StructuredBuffers the vertex-pull
            // per-object SRG binds (RenderBuffers[Positions..UVs]).
            if (meshRenderData.RenderBuffers.size() <=
                static_cast<size_t>(RenderStreamsSemantics::UVs))
            {
                return false;
            }

            meshRenderData.MeshShaderObjectSrg = RPI::ShaderResourceGroup::Create(
                srgSourceShader->GetAsset(), AZ::Name{ "MeshletsMeshObjectSrg" });
            meshRenderData.MeshShaderSrgBuiltForCull = forCull;
            if (!meshRenderData.MeshShaderObjectSrg)
            {
                AZ_Error("Meshlets", false, "EnsureMeshShaderResources: failed to create MeshletsMeshObjectSrg.");
                return false;
            }

            auto& srg = meshRenderData.MeshShaderObjectSrg;
            SrgBufferDescriptor bind;
            bind.m_paramNameInSrg = Name{ "m_meshletsDescriptors" };
            bool ok = UtilityClass::BindBufferToSrg("Meshlets", meshRenderData.ClusterDescBuffer, bind, srg);
            if (!meshRenderData.MonolithicDropped)
            {
                bind.m_paramNameInSrg = Name{ "m_meshletsTriangles" };
                ok &= UtilityClass::BindBufferToSrg("Meshlets", meshRenderData.MeshShaderTrianglesBuffer, bind, srg);
                bind.m_paramNameInSrg = Name{ "m_meshletsIndicesLookup" };
                ok &= UtilityClass::BindBufferToSrg("Meshlets", meshRenderData.MeshShaderIndirectionBuffer, bind, srg);
                bind.m_paramNameInSrg = Name{ "m_positions" };
                ok &= UtilityClass::BindBufferToSrg("Meshlets",
                    meshRenderData.RenderBuffers[static_cast<uint8_t>(RenderStreamsSemantics::Positions)], bind, srg);
                bind.m_paramNameInSrg = Name{ "m_normals" };
                ok &= UtilityClass::BindBufferToSrg("Meshlets",
                    meshRenderData.RenderBuffers[static_cast<uint8_t>(RenderStreamsSemantics::Normals)], bind, srg);
                bind.m_paramNameInSrg = Name{ "m_tangents" };
                ok &= UtilityClass::BindBufferToSrg("Meshlets",
                    meshRenderData.RenderBuffers[static_cast<uint8_t>(RenderStreamsSemantics::Tangents)], bind, srg);
                bind.m_paramNameInSrg = Name{ "m_uvs" };
                ok &= UtilityClass::BindBufferToSrg("Meshlets",
                    meshRenderData.RenderBuffers[static_cast<uint8_t>(RenderStreamsSemantics::UVs)], bind, srg);
            }
            // Streaming-exclusive: the monolithic stream slots stay null (never read --
            // the AS's m_pagedExclusive gate rejects every cluster until paged mode is
            // live, and paged clusters fetch from the pool exclusively).
            // --strip-unused-srgs: the uncull layout lacks m_clusterBounds, and a
            // failed bind would fail the whole ok &= chain -- bind only when present.
            if (meshRenderData.ClusterBoundsBuffer &&
                srg->FindShaderInputBufferIndex(Name{ "m_clusterBounds" }).IsValid())
            {
                bind.m_paramNameInSrg = Name{ "m_clusterBounds" };
                ok &= UtilityClass::BindBufferToSrg("Meshlets", meshRenderData.ClusterBoundsBuffer, bind, srg);
            }
            // Phase 6 DAG cut records -- same guarded pattern as m_clusterBounds (the
            // layout only carries m_dagNodes where a compiled entry references it, and
            // the buffer only exists for v3 packs).
            if (meshRenderData.DagNodesBuffer &&
                srg->FindShaderInputBufferIndex(Name{ "m_dagNodes" }).IsValid())
            {
                bind.m_paramNameInSrg = Name{ "m_dagNodes" };
                ok &= UtilityClass::BindBufferToSrg("Meshlets", meshRenderData.DagNodesBuffer, bind, srg);
            }
            // Phase 7 streaming: carry the current paged state across SRG rebuilds
            // (RebuildPagedClusterMap updates the live SRG on residency changes).
            if (srg->FindShaderInputBufferIndex(Name{ "m_pagedClusterMap" }).IsValid())
            {
                if (meshRenderData.PagedClusterMapBuffer)
                {
                    bind.m_paramNameInSrg = Name{ "m_pagedClusterMap" };
                    UtilityClass::BindBufferToSrg("Meshlets", meshRenderData.PagedClusterMapBuffer, bind, srg);
                }
                if (m_pagePoolBuffer)
                {
                    bind.m_paramNameInSrg = Name{ "m_pagePool" };
                    UtilityClass::BindBufferToSrg("Meshlets", m_pagePoolBuffer, bind, srg);
                }
                srg->SetConstant(
                    srg->FindShaderInputConstantIndex(Name{ "m_pagedMode" }),
                    meshRenderData.PagedModeActive ? 1u : 0u);
            }
            if (!ok)
            {
                AZ_Error("Meshlets", false, "EnsureMeshShaderResources: failed to bind buffers to MeshletsMeshObjectSrg.");
                meshRenderData.MeshShaderObjectSrg = nullptr;
                return false;
            }

            srg->SetConstant(srg->FindShaderInputConstantIndex(Name{ "m_vertexCount" }), meshRenderData.VertexCount);
            srg->SetConstant(srg->FindShaderInputConstantIndex(Name{ "m_clusterCount" }), clusterCount);
            srg->SetConstant(srg->FindShaderInputConstantIndex(Name{ "m_meshletDebugColor" }),
                m_debugControls.m_meshletColorMode ? 1u : (r_meshletsDagDebugColor ? 2u : 0u));
            // Leaf-only count: the AS refuses interior DAG clusters whenever the cut is
            // off, so a DAG-range dispatch can never double-draw (see MeshletsCullAS.azsli).
            srg->SetConstant(srg->FindShaderInputConstantIndex(Name{ "m_leafClusterCount" }), leafClusterCount);
            // Streaming-exclusive: with no monolithic fallback, the AS must reject
            // EVERYTHING until paged mode is live (startup frames would otherwise
            // fetch null stream buffers). Guarded -- only the culled layout carries it.
            if (srg->FindShaderInputConstantIndex(Name{ "m_pagedExclusive" }).IsValid())
            {
                srg->SetConstant(
                    srg->FindShaderInputConstantIndex(Name{ "m_pagedExclusive" }),
                    meshRenderData.MonolithicDropped ? 1u : 0u);
            }
            srg->Compile();

            // DispatchMesh geometry view: one threadgroup per cluster, no index buffer,
            // no IA streams -- the mesh shader pulls everything from the SRG. LEAF count:
            // the uncull MS path has no cut and must never emit DAG interiors.
            meshRenderData.MeshShaderGeometryView.SetDrawArguments(
                RHI::DrawArguments(RHI::DispatchMeshDirect(leafClusterCount, 1, 1)));

            // Phase 5 AS/triangle cull: the OUTER dispatch now sizes the amplification
            // shader, not the mesh shader directly -- ceil(clusterCount / MESHLETS_AS_GROUP)
            // AS groups; the AS launches per-survivor MS groups itself via DispatchMesh().
            // With the DAG cut active this covers the FULL leaf+interior range.
            constexpr uint32_t MeshletsAsGroupSize = 128;   // MUST match MESHLETS_AS_GROUP in the AZSL.
            const uint32_t asGroupCount = (cullClusterCount + MeshletsAsGroupSize - 1) / MeshletsAsGroupSize;
            meshRenderData.MeshShaderCullGeometryView.SetDrawArguments(
                RHI::DrawArguments(RHI::DispatchMeshDirect(asGroupCount, 1, 1)));

            meshRenderData.MeshShaderResourcesReady = true;
            AZ_TracePrintf("Meshlets",
                "EnsureMeshShaderResources: OK -- %u leaf / %u cull clusters (dag=%d), DispatchMesh(%u,1,1).\n",
                leafClusterCount, cullClusterCount, dagActive ? 1 : 0, leafClusterCount);
            return true;
        }

        bool MeshletsFeatureProcessor::BuildMeshShaderDrawPacket(
            MeshletsRenderInstance& instance, MeshRenderData& meshRenderData)
        {
            // Device gate: the RHI must report hardware mesh-shader support (DX12
            // MeshShaderTier >= 1 + SM6.5 on this backend). Queried once.
            if (!m_meshShaderSupportQueried)
            {
                m_meshShaderSupportQueried = true;
                if (RHI::Device* device = RHI::RHISystemInterface::Get()->GetDevice())
                {
                    m_meshShaderSupported = device->GetFeatures().m_meshShader;
                }
                AZ_TracePrintf("Meshlets", "Hardware mesh-shader support: %s\n",
                    m_meshShaderSupported ? "YES" : "no (r_meshletsHwMeshShader will fall back)");
            }
            if (!m_meshShaderSupported)
            {
                return false;
            }
            // Shader/PSO: lazy retry -- the shader asset may not have been processed
            // when the pipeline initialized.
            if (!m_meshForwardPipelineState && !InitMeshForwardShader())
            {
                return false;
            }
            if (!m_meshForwardDrawListTag.IsValid() || !instance.RenderObject)
            {
                return false;
            }
            // Phase 5 AS/triangle cull (opt-in): decide the mode BEFORE building resources.
            // azslc runs with --strip-unused-srgs, so the uncull and AS-cull shaders have
            // DIFFERENT SRG layouts even though both compile from MeshletsMeshRenderSrg.azsli:
            // the uncull MS references only m_objectId, so the AS's m_clusterBounds (object
            // SRG) and every cull field (instance SRG: m_frustumPlanes/m_worldToClip/
            // m_cameraPosition/m_do*Cull/m_worldRow*/m_hiZTexture) are stripped from its
            // layouts. SRGs therefore CANNOT be shared across the two PSOs -- each must be
            // created from the same shader asset as the PSO it binds to, and rebuilt when
            // the mode flips.
            const bool useCullAS = r_meshletsMsCullAS && InitMeshCullForwardShader();

            // Streaming-exclusive meshes have NO monolithic geometry: only the AS-cull
            // paged path can draw them. Without it, build nothing (and never fall
            // through to a path that would fetch null buffers).
            if (meshRenderData.MonolithicDropped && !useCullAS)
            {
                instance.DrawPacket = nullptr;
                instance.DepthDrawPacket = nullptr;
                instance.LateDepthDrawPacket = nullptr;
                instance.ShadowDrawPacket = nullptr;
                AZ_Warning("Meshlets", false,
                    "Streaming-exclusive mesh cannot render: r_meshletsMsCullAS is off "
                    "(or the culled shader is not processed). Enable the AS-cull + DAG + "
                    "streaming cvars, or reload with r_meshletsStreamingExclusive off.");
                return true;
            }

            // Per-mesh resources (cluster descriptor buffer first, then the rest).
            // EnsureMeshShaderResources rebuilds the object SRG from the matching asset
            // when useCullAS differs from what it was last built for.
            if (!instance.RenderObject->EnsureCullGpuBuffers(meshRenderData) ||
                !EnsureMeshShaderResources(meshRenderData, useCullAS))
            {
                return false;
            }

            // Per-instance SRG: m_objectId for the SceneSrg transform lookup (+
            // frustum/cull constants for the AS path, refreshed every frame by
            // UpdateMeshShaderCullInstance while r_meshletsMsCullAS is active).
            // Rebuilt on a cull-mode flip for the same stripped-layout reason.
            if (instance.MeshShaderInstanceSrg &&
                instance.MeshShaderInstanceSrgBuiltForCull != useCullAS)
            {
                instance.MeshShaderInstanceSrg = nullptr;
            }
            if (!instance.MeshShaderInstanceSrg)
            {
                const Data::Instance<RPI::Shader>& instanceSrgShader =
                    (useCullAS && m_meshCullForwardShader) ? m_meshCullForwardShader : m_meshForwardShader;
                instance.MeshShaderInstanceSrg = RPI::ShaderResourceGroup::Create(
                    instanceSrgShader->GetAsset(), AZ::Name{ "MeshletsMeshInstanceSrg" });
                instance.MeshShaderInstanceSrgBuiltForCull = useCullAS;
                if (!instance.MeshShaderInstanceSrg)
                {
                    AZ_Error("Meshlets", false, "BuildMeshShaderDrawPacket: failed to create MeshletsMeshInstanceSrg.");
                    return false;
                }
                instance.MeshShaderInstanceSrg->SetConstant(
                    instance.MeshShaderInstanceSrg->FindShaderInputConstantIndex(Name{ "m_objectId" }),
                    instance.ObjectId.GetIndex());
                // Write REAL cull constants before the one creation Compile(): the RHI
                // discards a same-frame recompile, so zero-init here meant rebuild
                // frames culled with a zero world matrix (meshlets vanished during
                // LOD-migrating transforms).
                if (useCullAS)
                {
                    AZ::Frustum frustum;   // default: cull toggles below still guard usage
                    AZ::Vector3 cameraPos = AZ::Vector3::CreateZero();
                    bool haveCamera = false;
                    if (m_renderPipeline)
                    {
                        if (RPI::ViewPtr view = m_renderPipeline->GetDefaultView())
                        {
                            frustum = AZ::Frustum::CreateFromMatrixColumnMajor(view->GetWorldToClipMatrix());
                            cameraPos = view->GetViewToWorldMatrix().GetTranslation();
                            haveCamera = true;
                        }
                    }
                    AZ::Matrix4x4 objectToWorld = AZ::Matrix4x4::CreateIdentity();
                    if (m_transformServiceFeatureProcessor)
                    {
                        objectToWorld = AZ::Matrix4x4::CreateFromTransform(
                            m_transformServiceFeatureProcessor->GetTransformForId(instance.ObjectId));
                    }
                    if (haveCamera)
                    {
                        const bool dagCutActive = r_meshletsDagLod && m_dagBindValid &&
                            meshRenderData.DagClusterCount > 0 && meshRenderData.DagNodesBuffer;
                        WriteMeshShaderCullConstants(
                            instance.MeshShaderInstanceSrg, frustum, cameraPos, objectToWorld, dagCutActive);
                    }
                    else
                    {
                        // No camera yet -- force passthrough for this frame rather than
                        // culling against garbage planes.
                        instance.MeshShaderInstanceSrg->SetConstant(
                            instance.MeshShaderInstanceSrg->FindShaderInputConstantIndex(Name{ "m_doFrustumCull" }), 0u);
                        instance.MeshShaderInstanceSrg->SetConstant(
                            instance.MeshShaderInstanceSrg->FindShaderInputConstantIndex(Name{ "m_doConeCull" }), 0u);
                        instance.MeshShaderInstanceSrg->SetConstant(
                            instance.MeshShaderInstanceSrg->FindShaderInputConstantIndex(Name{ "m_doHiZCull" }), 0u);
                    }
                }
                instance.MeshShaderInstanceSrg->Compile();
            }

            RHI::DrawPacketBuilder drawPacketBuilder(RHI::MultiDevice::AllDevices);
            drawPacketBuilder.Begin(nullptr);
            drawPacketBuilder.SetGeometryView(
                useCullAS ? &meshRenderData.MeshShaderCullGeometryView : &meshRenderData.MeshShaderGeometryView);
            drawPacketBuilder.AddShaderResourceGroup(meshRenderData.MeshShaderObjectSrg->GetRHIShaderResourceGroup());
            drawPacketBuilder.AddShaderResourceGroup(instance.MeshShaderInstanceSrg->GetRHIShaderResourceGroup());

            // Forward PBR material SRG (SRG_PerMaterial) -- the source model's PBR
            // textures + factors. Resolve from the model's MaterialAsset and bind as
            // the 3rd DrawItem SRG, exactly like the vertex-pull forward path
            // (BuildInstanceDrawPacket). The mesh shader now declares MeshletsMaterialSrg,
            // so a layout-compatible SRG is created from m_meshForwardShader's asset.
            // Defer the whole packet if it isn't ready yet (material asset still loading)
            // -- binding the forward item without it would read garbage.
            {
                Data::Instance<RPI::ShaderResourceGroup> materialSrg;
                if (instance.RenderObject->EnsureMaterialSrg(instance.MeshIndex, m_meshForwardShader))
                {
                    materialSrg = instance.RenderObject->GetMaterialSrgForMesh(instance.MeshIndex);
                    if (materialSrg && !meshRenderData.MaterialSrg)
                    {
                        meshRenderData.MaterialSrg = materialSrg;
                        meshRenderData.MaterialResolved = true;
                    }
                }
                if (!materialSrg)
                {
                    AZ_TracePrintf("Meshlets",
                        "Deferring mesh-shader DrawPacket: material SRG for mesh %u not ready yet.\n",
                        instance.MeshIndex);
                    instance.DrawPacket = nullptr;
                    return true;  // success-with-deferral; retry next frame
                }
                drawPacketBuilder.AddShaderResourceGroup(materialSrg->GetRHIShaderResourceGroup());
            }

            // Single forward DrawItem: DispatchMesh through the standard ForwardPass.
            // No stream indices (no input assembler on the mesh path). Stencil ref
            // matches the vertex-pull forward item so downstream fullscreen passes
            // treat meshlet pixels identically.
            RHI::DrawPacketBuilder::DrawRequest meshDrawRequest;
            meshDrawRequest.m_listTag = m_meshForwardDrawListTag;
            meshDrawRequest.m_pipelineState = useCullAS ? m_meshCullForwardPipelineState : m_meshForwardPipelineState;
            meshDrawRequest.m_stencilRef = static_cast<uint8_t>(
                Render::StencilRefs::UseIBLSpecularPass | Render::StencilRefs::UseDiffuseGIPass);
            meshDrawRequest.m_sortKey = 0;
            drawPacketBuilder.AddDrawItem(meshDrawRequest);

            // DEPTH prepass item ("depth" tag) -- reuses the SAME geometry view /
            // DispatchMesh args and the packet-wide SRGs already bound above; the
            // depth shader simply doesn't declare MeshletsMaterialSrg, so that extra
            // binding is harmlessly unused (exactly as the vertex-pull depth/motion
            // items already ignore it). Without this, the mesh path never reaches
            // MainPipeline's once-per-frame resolved Depth/DepthLinear copy (produced
            // early from this tag only) and DepthOfField/Transparent sample stale
            // depth -> meshlets look see-through.
            //
            // The packet's one geometry view carries AS group counts under useCullAS,
            // so the depth item needs the AS-culled depth PSO.
            const RHI::PipelineState* depthPso = nullptr;
            if (useCullAS)
            {
                if (InitCulledMeshVariant(
                        "Shaders/MeshletsDepthMeshShaderCulled.azshader", "depth",
                        m_meshDepthCullShader, m_meshDepthCullPipelineState))
                {
                    depthPso = m_meshDepthCullPipelineState;
                    if (!m_depthDrawListTag.IsValid())
                    {
                        m_depthDrawListTag = m_meshDepthCullShader->GetDrawListTag();
                    }
                }
            }
            else if (m_meshDepthPipelineState || InitMeshDepthShader())
            {
                depthPso = m_meshDepthPipelineState;
            }
            instance.DepthDrawPacket = nullptr;
            instance.LateDepthDrawPacket = nullptr;
            if (depthPso && m_depthDrawListTag.IsValid() && m_debugControls.m_depthPassEnabled)
            {
                if (useCullAS)
                {
                    // Own packet + own SRG so depth never gets the HiZ override --
                    // an occluded depth prepass would poison next frame's pyramid.
                    if (!instance.MeshDepthInstanceSrg && m_meshDepthCullShader)
                    {
                        instance.MeshDepthInstanceSrg = RPI::ShaderResourceGroup::Create(
                            m_meshDepthCullShader->GetAsset(), AZ::Name{ "MeshletsMeshInstanceSrg" });
                        if (instance.MeshDepthInstanceSrg)
                        {
                            instance.MeshDepthInstanceSrg->SetConstant(
                                instance.MeshDepthInstanceSrg->FindShaderInputConstantIndex(Name{ "m_objectId" }),
                                instance.ObjectId.GetIndex());
                            // Zeroed SRG = all culls off = leaf set only, until the
                            // per-frame update writes real constants.
                            instance.MeshDepthInstanceSrg->Compile();
                        }
                    }
                    if (instance.MeshDepthInstanceSrg)
                    {
                        RHI::DrawPacketBuilder depthBuilder(RHI::MultiDevice::AllDevices);
                        depthBuilder.Begin(nullptr);
                        depthBuilder.SetGeometryView(&meshRenderData.MeshShaderCullGeometryView);
                        depthBuilder.AddShaderResourceGroup(meshRenderData.MeshShaderObjectSrg->GetRHIShaderResourceGroup());
                        depthBuilder.AddShaderResourceGroup(instance.MeshDepthInstanceSrg->GetRHIShaderResourceGroup());
                        RHI::DrawPacketBuilder::DrawRequest meshDepthDrawRequest;
                        meshDepthDrawRequest.m_listTag = m_depthDrawListTag;
                        meshDepthDrawRequest.m_pipelineState = depthPso;
                        meshDepthDrawRequest.m_stencilRef = 0;
                        meshDepthDrawRequest.m_sortKey = 0;
                        depthBuilder.AddDrawItem(meshDepthDrawRequest);
                        instance.DepthDrawPacket = depthBuilder.End();
                    }
                }
                else
                {
                    RHI::DrawPacketBuilder::DrawRequest meshDepthDrawRequest;
                    meshDepthDrawRequest.m_listTag = m_depthDrawListTag;
                    meshDepthDrawRequest.m_pipelineState = depthPso;
                    meshDepthDrawRequest.m_stencilRef = 0;
                    meshDepthDrawRequest.m_sortKey = 0;
                    drawPacketBuilder.AddDrawItem(meshDepthDrawRequest);
                }
            }

            // Two-pass PASS 2: visibility ledger + late-depth packet.
            instance.LateDepthDrawPacket = nullptr;
            if (useCullAS && r_meshletsTwoPassOcclusion && m_lateDepthPass &&
                InitCulledMeshVariant(
                    "Shaders/MeshletsDepthMeshShaderLate.azshader", "latedepth",
                    m_meshLateShader, m_meshLatePipelineState))
            {
                if (!m_lateDrawListTag.IsValid())
                {
                    m_lateDrawListTag = m_meshLateShader->GetDrawListTag();
                }
                if (!instance.VisFrameBuffer)
                {
                    // One frame-id per cluster, GPU-only (AMD CPU uploads to the
                    // ReadWrite pool are unreliable). m_frameId >= 1 => no clear pass.
                    SrgBufferDescriptor d(
                        RPI::CommonBufferPoolType::ReadWrite, RHI::Format::Unknown,
                        RHI::BufferBindFlags::ShaderReadWrite,
                        static_cast<uint32_t>(sizeof(AZ::u32)), AZStd::GetMax(1u, sliceClusterCount),
                        Name{ "MeshletsVisFrame" }, Name{ "m_clusterVisFrame" }, 0, 0, nullptr);
                    instance.VisFrameBuffer = UtilityClass::CreateBuffer("Meshlets", d, nullptr);
                    instance.VisFrameAttachmentId = AZ::Name(
                        AZStd::string::format("MeshletsVisFrame_%u", instance.ObjectId.GetIndex()));
                }
                if (!instance.MeshLateInstanceSrg && m_meshLateShader)
                {
                    instance.MeshLateInstanceSrg = RPI::ShaderResourceGroup::Create(
                        m_meshLateShader->GetAsset(), AZ::Name{ "MeshletsMeshInstanceSrg" });
                    if (instance.MeshLateInstanceSrg)
                    {
                        instance.MeshLateInstanceSrg->SetConstant(
                            instance.MeshLateInstanceSrg->FindShaderInputConstantIndex(Name{ "m_objectId" }),
                            instance.ObjectId.GetIndex());
                        // visMode 2 + zeroed frameId == zeroed ledger => emits nothing
                        // until the per-frame update writes real constants.
                        instance.MeshLateInstanceSrg->SetConstant(
                            instance.MeshLateInstanceSrg->FindShaderInputConstantIndex(Name{ "m_visMode" }), 2u);
                        instance.MeshLateInstanceSrg->Compile();
                    }
                }
                // Ledger UAV binds into its two writers (depth + late SRGs).
                auto bindVisLedger = [&](const Data::Instance<RPI::ShaderResourceGroup>& s)
                {
                    if (s && instance.VisFrameBuffer &&
                        s->FindShaderInputBufferIndex(Name{ "m_clusterVisFrame" }).IsValid())
                    {
                        SrgBufferDescriptor visBind;
                        visBind.m_paramNameInSrg = Name{ "m_clusterVisFrame" };
                        UtilityClass::BindBufferToSrg("Meshlets", instance.VisFrameBuffer, visBind, s);
                    }
                };
                bindVisLedger(instance.MeshLateInstanceSrg);
                bindVisLedger(instance.MeshDepthInstanceSrg);

                if (instance.MeshLateInstanceSrg && instance.VisFrameBuffer &&
                    m_lateDrawListTag.IsValid())
                {
                    RHI::DrawPacketBuilder lateBuilder(RHI::MultiDevice::AllDevices);
                    lateBuilder.Begin(nullptr);
                    lateBuilder.SetGeometryView(&meshRenderData.MeshShaderCullGeometryView);
                    lateBuilder.AddShaderResourceGroup(meshRenderData.MeshShaderObjectSrg->GetRHIShaderResourceGroup());
                    lateBuilder.AddShaderResourceGroup(instance.MeshLateInstanceSrg->GetRHIShaderResourceGroup());
                    RHI::DrawPacketBuilder::DrawRequest lateDrawRequest;
                    lateDrawRequest.m_listTag = m_lateDrawListTag;
                    lateDrawRequest.m_pipelineState = m_meshLatePipelineState;
                    lateDrawRequest.m_stencilRef = 0;
                    lateDrawRequest.m_sortKey = 0;
                    lateBuilder.AddDrawItem(lateDrawRequest);
                    instance.LateDepthDrawPacket = lateBuilder.End();
                }
            }

            // MOTION VECTOR item ("motion" tag) - same geometry view / DispatchMesh args and
            // packet-wide SRGs as the forward and depth items above. Without this the mesh
            // path emits NOTHING to the motion buffer, because motion items are added by
            // BuildInstanceDrawPacket and this function short-circuits it: meshlet geometry
            // then ghosts under TAA and reads as stale to any temporal upscaler.
            //
            // Same PSO split as depth: AS-culled motion PSO under useCullAS, plain
            // Mesh-only PSO otherwise.
            const RHI::PipelineState* motionPso = nullptr;
            if (useCullAS)
            {
                if (InitCulledMeshVariant(
                        "Shaders/MeshletsMotionVectorMeshShaderCulled.azshader", "motion",
                        m_meshMotionCullShader, m_meshMotionCullPipelineState))
                {
                    motionPso = m_meshMotionCullPipelineState;
                    if (!m_motionDrawListTag.IsValid())
                    {
                        m_motionDrawListTag = m_meshMotionCullShader->GetDrawListTag();
                    }
                }
            }
            else if (m_meshMotionPipelineState || InitMeshMotionShader())
            {
                motionPso = m_meshMotionPipelineState;
            }
            if (motionPso && m_motionDrawListTag.IsValid() && m_debugControls.m_motionPassEnabled)
            {
                RHI::DrawPacketBuilder::DrawRequest meshMotionDrawRequest;
                meshMotionDrawRequest.m_listTag = m_motionDrawListTag;
                meshMotionDrawRequest.m_pipelineState = motionPso;
                meshMotionDrawRequest.m_stencilRef = 0;
                meshMotionDrawRequest.m_sortKey = 0;
                drawPacketBuilder.AddDrawItem(meshMotionDrawRequest);
            }

            instance.DrawPacket = drawPacketBuilder.End();

            // ---- Shadow casting ----
            // Separate DrawPacket over the WHOLE mesh: camera-side per-cluster culling
            // is wrong for a light's point of view.
            instance.ShadowDrawPacket = nullptr;
            instance.DepthDrawPacket = nullptr;
            instance.LateDepthDrawPacket = nullptr;

            // Preferred: mesh-shader shadow packet over ALL clusters (no AS) -- emits
            // exactly the vertex-pull set, so nothing is culled from the light's view.
            // Under useCullAS it needs its own uncull-layout instance SRG.
            const bool meshShadowReady = m_meshShadowPipelineState || InitMeshShadowShader();
            // Shadow DAG cut: same cut as the camera passes, AS in cut-only mode
            // (camera-view culls zeroed -- a light sees outside the camera frustum).
            const bool dagShadow = r_meshletsDagLod && useCullAS &&
                meshRenderData.DagClusterCount > 0 && meshRenderData.DagNodesBuffer &&
                InitCulledMeshVariant(
                    "Shaders/MeshletsShadowMeshShaderCulled.azshader", "shadow",
                    m_meshShadowCullShader, m_meshShadowCullPipelineState);
            // Streaming-exclusive: the plain (uncull) mesh shadow reads the monolithic
            // stream buffers, which do not exist -- only the DAG-cut shadow can draw.
            const RHI::PipelineState* shadowPso = dagShadow ? m_meshShadowCullPipelineState
                : ((meshShadowReady && !meshRenderData.MonolithicDropped) ? m_meshShadowPipelineState : nullptr);
            Data::Instance<RPI::ShaderResourceGroup> shadowInstanceSrg = instance.MeshShaderInstanceSrg;
            if (shadowPso && useCullAS)
            {
                // SRG layout must match the shadow PSO (uncull vs cull); rebuild on flip.
                if (instance.MeshShadowInstanceSrg && instance.MeshShadowInstanceSrgIsCull != dagShadow)
                {
                    instance.MeshShadowInstanceSrg = nullptr;
                }
                if (!instance.MeshShadowInstanceSrg)
                {
                    const Data::Instance<RPI::Shader>& shadowSrgShader =
                        dagShadow ? m_meshShadowCullShader : m_meshShadowShader;
                    instance.MeshShadowInstanceSrg = RPI::ShaderResourceGroup::Create(
                        shadowSrgShader->GetAsset(), AZ::Name{ "MeshletsMeshInstanceSrg" });
                    instance.MeshShadowInstanceSrgIsCull = dagShadow;
                    if (instance.MeshShadowInstanceSrg)
                    {
                        instance.MeshShadowInstanceSrg->SetConstant(
                            instance.MeshShadowInstanceSrg->FindShaderInputConstantIndex(Name{ "m_objectId" }),
                            instance.ObjectId.GetIndex());
                        // Zeroed SRG = leaf set only until the per-frame update runs.
                        instance.MeshShadowInstanceSrg->Compile();
                    }
                }
                shadowInstanceSrg = instance.MeshShadowInstanceSrg;

                // Preserve the vertex-pull fallback's side effect: the compute GPU-cull
                // path reuses IndirectGeometryView/IA buffers this builds (idempotent,
                // cheap after the first call).
                instance.RenderObject->EnsureIndirectArgs(meshRenderData, m_drawIndirectSignature.get());
            }
            if (shadowPso && m_shadowDrawListTag.IsValid() &&
                m_debugControls.m_shadowPassEnabled && meshRenderData.MeshShaderObjectSrg &&
                shadowInstanceSrg)
            {
                RHI::DrawPacketBuilder meshShadowBuilder(RHI::MultiDevice::AllDevices);
                meshShadowBuilder.Begin(nullptr);
                // DAG-cut shadows dispatch the AS-group-count view over the FULL DAG
                // range (the AS selects the cut); plain shadows dispatch all leaves.
                meshShadowBuilder.SetGeometryView(dagShadow
                    ? &meshRenderData.MeshShaderCullGeometryView
                    : &meshRenderData.MeshShaderGeometryView);
                // MeshShaderObjectSrg, NOT ObjectSrg -- the vertex-pull layout at the
                // same frequency decoded garbage clusters and hung the GPU.
                meshShadowBuilder.AddShaderResourceGroup(meshRenderData.MeshShaderObjectSrg->GetRHIShaderResourceGroup());
                meshShadowBuilder.AddShaderResourceGroup(shadowInstanceSrg->GetRHIShaderResourceGroup());

                RHI::DrawPacketBuilder::DrawRequest meshShadowDrawRequest;
                meshShadowDrawRequest.m_listTag = m_shadowDrawListTag;
                meshShadowDrawRequest.m_pipelineState = shadowPso;
                meshShadowDrawRequest.m_stencilRef = 0;
                meshShadowDrawRequest.m_sortKey = 0;
                meshShadowBuilder.AddDrawItem(meshShadowDrawRequest);
                instance.ShadowDrawPacket = meshShadowBuilder.End();
            }

            // FALLBACK: vertex-pull shadow packet, for when the mesh shadow shader is
            // not processed yet. Building it also runs EnsureIndirectArgs (the useCullAS
            // branch above preserves that side effect explicitly).
            if (!instance.ShadowDrawPacket &&
                m_shadowPipelineState && m_shadowDrawListTag.IsValid() &&
                m_debugControls.m_shadowPassEnabled && meshRenderData.ObjectSrg &&
                instance.RenderObject->EnsureIndirectArgs(meshRenderData, m_drawIndirectSignature.get()))
            {
                if (!instance.InstanceSrg && m_renderShader)
                {
                    instance.InstanceSrg = RPI::ShaderResourceGroup::Create(
                        m_renderShader->GetAsset(), AZ::Name{ "MeshletsInstanceRenderSrg" });
                    if (instance.InstanceSrg)
                    {
                        instance.InstanceSrg->SetConstant(
                            instance.InstanceSrg->FindShaderInputConstantIndex(Name("m_objectId")),
                            instance.ObjectId.GetIndex());
                        // Single-instance path: the VS must read m_objectId, not m_instanceObjectIds.
                        instance.InstanceSrg->SetConstant(
                            instance.InstanceSrg->FindShaderInputConstantIndex(Name("m_useInstancing")), 0u);
                        // Bind a valid (but unread) buffer so the SRG has no unbound view at compile.
                        const uint8_t idxSem = static_cast<uint8_t>(RenderStreamsSemantics::Indices);
                        if (idxSem < meshRenderData.RenderBuffers.size() && meshRenderData.RenderBuffers[idxSem])
                        {
                            SrgBufferDescriptor idxBind;
                            idxBind.m_paramNameInSrg = Name{ "m_instanceObjectIds" };
                            UtilityClass::BindBufferToSrg(
                                "Meshlets", meshRenderData.RenderBuffers[idxSem], idxBind, instance.InstanceSrg);
                        }
                        instance.InstanceSrg->Compile();
                    }
                }

                RHI::StreamBufferIndices posOnly;
                posOnly.AddIndex(0);   // POSITION-only hardware-IA layout.
                if (instance.InstanceSrg && meshRenderData.PositionStreamValid &&
                    RHI::ValidateStreamBufferViews(m_shadowInputLayout, meshRenderData.IndirectGeometryView, posOnly))
                {
                    RHI::DrawPacketBuilder shadowBuilder(RHI::MultiDevice::AllDevices);
                    shadowBuilder.Begin(nullptr);
                    shadowBuilder.SetGeometryView(&meshRenderData.IndirectGeometryView);
                    shadowBuilder.AddShaderResourceGroup(meshRenderData.ObjectSrg->GetRHIShaderResourceGroup());
                    shadowBuilder.AddShaderResourceGroup(instance.InstanceSrg->GetRHIShaderResourceGroup());
                    RHI::DrawPacketBuilder::DrawRequest shadowDrawRequest;
                    shadowDrawRequest.m_listTag = m_shadowDrawListTag;
                    shadowDrawRequest.m_pipelineState = m_shadowPipelineState;
                    shadowDrawRequest.m_streamIndices = posOnly;
                    shadowDrawRequest.m_stencilRef = 0;
                    shadowDrawRequest.m_sortKey = 0;
                    shadowBuilder.AddDrawItem(shadowDrawRequest);
                    instance.ShadowDrawPacket = shadowBuilder.End();
                }
            }

            if (!instance.DrawPacket)
            {
                AZ_Error("Meshlets", false, "BuildMeshShaderDrawPacket: DrawPacketBuilder::End() failed.");
                return false;
            }
            return true;
        }

        void MeshletsFeatureProcessor::UpdateMeshShaderCullInstance(
            MeshletsRenderInstance& instance, const AZ::Frustum& frustum,
            const AZ::Vector3& cameraPos, const AZ::Matrix4x4& objectToWorld)
        {
            if (!instance.MeshShaderInstanceSrg)
            {
                return;   // packet not built yet this frame -- nothing to refresh
            }
            if (!instance.MeshShaderInstanceSrgBuiltForCull)
            {
                // The SRG was created from the UNCULL shader, whose layout has every AS
                // cull field stripped (azslc --strip-unused-srgs). Writing them here would
                // silently no-op on invalid indices. The packet rebuild that follows a
                // r_meshletsMsCullAS flip recreates this SRG from the AS-cull layout.
                return;
            }
            // Per-instance DAG cut eligibility: needs this mesh's DagNodes buffer (bound
            // into the object SRG by EnsureMeshShaderResources) + a valid projection stash.
            const MeshRenderData* dagMrd = nullptr;
            if (instance.RenderObject)
            {
                ModelLodDataArray& lodArr = instance.RenderObject->GetMeshletsRenderData(instance.LodIndex);
                if (instance.MeshIndex < lodArr.size())
                {
                    dagMrd = lodArr[instance.MeshIndex];
                }
            }
            const bool dagCutActive = r_meshletsDagLod && m_dagBindValid &&
                dagMrd && dagMrd->DagClusterCount > 0 && dagMrd->DagNodesBuffer;

            // Per-instance two-pass eligibility.
            Data::Instance<RPI::AttachmentImage> currentPyramid;
            if (m_hiZGeneratePass)
            {
                currentPyramid = m_hiZGeneratePass->GetCurrentPyramid();
            }
            const bool twoPassActive = r_meshletsTwoPassOcclusion && m_lateDepthPass &&
                m_cullBarrierPass && currentPyramid &&
                instance.LateDepthDrawPacket && instance.VisFrameBuffer;

            WriteMeshShaderCullConstants(instance.MeshShaderInstanceSrg, frustum, cameraPos, objectToWorld, dagCutActive);
            // Camera-SRG HiZ: two-pass ON = CURRENT pyramid/matrix (legal via the
            // late pass's declared HiZInput read; m_hiZPrevCullMat already holds this
            // frame's matrix here); OFF = last-completed pyramid as before.
            if (twoPassActive)
            {
                auto& srg = instance.MeshShaderInstanceSrg;
                srg->SetImage(srg->FindShaderInputImageIndex(Name("m_hiZTexture")), currentPyramid);
                srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_worldToClip")), m_hiZPrevCullMat);
                srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_doHiZCull")), m_debugControls.m_hiZCull ? 1u : 0u);
            }
            else if (m_hiZBindValid)
            {
                auto& srg = instance.MeshShaderInstanceSrg;
                srg->SetImage(srg->FindShaderInputImageIndex(Name("m_hiZTexture")), m_hiZBindImage);
                srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_worldToClip")), m_hiZBindWorldToClip);
                srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_doHiZCull")), 1u);
            }
            // A rebuild frame already compiled this SRG; a second Compile is discarded.
            if (!instance.MeshShaderInstanceSrg->IsQueuedForCompile())
            {
                instance.MeshShaderInstanceSrg->Compile();
            }

            // Depth SRG: two-pass ON = pass 1 (prev-frame HiZ safe again, visMode 1
            // records the ledger); OFF = no HiZ ever (pyramid-feedback protection).
            if (instance.MeshDepthInstanceSrg)
            {
                auto& depthSrg = instance.MeshDepthInstanceSrg;
                WriteMeshShaderCullConstants(depthSrg, frustum, cameraPos, objectToWorld, dagCutActive);
                if (twoPassActive && m_hiZBindValid)
                {
                    depthSrg->SetImage(depthSrg->FindShaderInputImageIndex(Name("m_hiZTexture")), m_hiZBindImage);
                    depthSrg->SetConstant(depthSrg->FindShaderInputConstantIndex(Name("m_worldToClip")), m_hiZBindWorldToClip);
                    depthSrg->SetConstant(
                        depthSrg->FindShaderInputConstantIndex(Name("m_doHiZCull")), m_debugControls.m_hiZCull ? 1u : 0u);
                }
                depthSrg->SetConstant(
                    depthSrg->FindShaderInputConstantIndex(Name("m_visMode")), twoPassActive ? 1u : 0u);
                depthSrg->SetConstant(depthSrg->FindShaderInputConstantIndex(Name("m_frameId")), m_frameId);
                if (!depthSrg->IsQueuedForCompile())
                {
                    depthSrg->Compile();
                }
            }

            // Late SRG (pass 2): only pass-1-skipped clusters vs THIS frame's pyramid.
            if (instance.MeshLateInstanceSrg && twoPassActive)
            {
                auto& lateSrg = instance.MeshLateInstanceSrg;
                WriteMeshShaderCullConstants(lateSrg, frustum, cameraPos, objectToWorld, dagCutActive);
                lateSrg->SetImage(lateSrg->FindShaderInputImageIndex(Name("m_hiZTexture")), currentPyramid);
                lateSrg->SetConstant(lateSrg->FindShaderInputConstantIndex(Name("m_worldToClip")), m_hiZPrevCullMat);
                lateSrg->SetConstant(
                    lateSrg->FindShaderInputConstantIndex(Name("m_doHiZCull")), m_debugControls.m_hiZCull ? 1u : 0u);
                lateSrg->SetConstant(lateSrg->FindShaderInputConstantIndex(Name("m_visMode")), 2u);
                lateSrg->SetConstant(lateSrg->FindShaderInputConstantIndex(Name("m_frameId")), m_frameId);
                if (!lateSrg->IsQueuedForCompile())
                {
                    lateSrg->Compile();
                }
            }

            // Shadow SRG: cut-only -- camera-view culls forced off (a light sees
            // clusters the camera cannot).
            if (instance.MeshShadowInstanceSrg && instance.MeshShadowInstanceSrgIsCull)
            {
                auto& shadowSrg = instance.MeshShadowInstanceSrg;
                WriteMeshShaderCullConstants(shadowSrg, frustum, cameraPos, objectToWorld, dagCutActive);
                shadowSrg->SetConstant(shadowSrg->FindShaderInputConstantIndex(Name("m_doFrustumCull")), 0u);
                shadowSrg->SetConstant(shadowSrg->FindShaderInputConstantIndex(Name("m_doConeCull")), 0u);
                if (!shadowSrg->IsQueuedForCompile())
                {
                    shadowSrg->Compile();
                }
            }
        }

        void MeshletsFeatureProcessor::WriteMeshShaderCullConstants(
            const Data::Instance<RPI::ShaderResourceGroup>& srg, const AZ::Frustum& frustum,
            const AZ::Vector3& cameraPos, const AZ::Matrix4x4& objectToWorld, bool dagCutActive)
        {
            // Object->world as 3 rows (shader does explicit row*point -- no matrix
            // major-ness), same convention as MeshletsCull.azsl's per-instance update.
            srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_worldRow0")), objectToWorld.GetRow(0));
            srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_worldRow1")), objectToWorld.GetRow(1));
            srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_worldRow2")), objectToWorld.GetRow(2));

            {
                float planeData[24];
                for (int i = 0; i < AZ::Frustum::PlaneId::MAX; ++i)
                {
                    const AZ::Vector4 coeffs =
                        frustum.GetPlane(static_cast<AZ::Frustum::PlaneId>(i)).GetPlaneEquationCoefficients();
                    coeffs.StoreToFloat4(&planeData[i * 4]);
                }
                srg->SetConstantRaw(
                    srg->FindShaderInputConstantIndex(Name("m_frustumPlanes")), planeData, static_cast<uint32_t>(sizeof(planeData)));
            }

            srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_cameraPosition")), AZ::Vector4::CreateFromVector3(cameraPos));
            srg->SetConstant(
                srg->FindShaderInputConstantIndex(Name("m_doFrustumCull")), m_debugControls.m_frustumCull ? 1u : 0u);
            srg->SetConstant(
                srg->FindShaderInputConstantIndex(Name("m_doConeCull")), m_debugControls.m_coneCull ? 1u : 0u);
            // HiZ pyramid is never wired to the AS path in this slice -- always off, a
            // safe no-op (see MeshletsMeshRenderSrg.azsli's m_hiZTexture note).
            srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_doHiZCull")), 0u);

            // Phase 6 DAG cut + real viewport for the per-triangle pixel gates.
            // dagCutActive is per-INSTANCE (requires this mesh's DagNodes buffer bound
            // and the projection stash valid): enabling the cut against an unbound
            // m_dagNodes would reject every cluster (zero records fail parent > tau).
            srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_viewportSize")), m_dagViewport);
            srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_dagProjScale")), m_dagProjScale);
            srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_dagErrorPx")),
                static_cast<float>(r_meshletsDagErrorPx));
            srg->SetConstant(srg->FindShaderInputConstantIndex(Name("m_doDagCut")), dagCutActive ? 1u : 0u);
        }

        void MeshletsFeatureProcessor::RebuildPagedClusterMap(MeshRenderData& mrd)
        {
            const uint32_t dagCount = mrd.DagClusterCount;
            const uint32_t leafCount = mrd.MeshletsCount;
            if (dagCount == 0 || mrd.PersistentPageTable.empty())
            {
                return;
            }

            // ---- One-time lookups: cluster->page, leaf simplification groups (via
            // ParentIndex -- exact, no float matching), level-1 parent -> group.
            if (!mrd.PagedLookupsBuilt)
            {
                mrd.ClusterToPage.assign(dagCount, 0xFFFFFFFFu);
                for (uint32_t pg = 0; pg < mrd.PersistentPageTable.size(); ++pg)
                {
                    const PageTableRecord& rec = mrd.PersistentPageTable[pg];
                    for (uint32_t c = 0; c < rec.m_clusterCount; ++c)
                    {
                        if (rec.m_clusterFirst + c < dagCount)
                        {
                            mrd.ClusterToPage[rec.m_clusterFirst + c] = pg;
                        }
                    }
                }

                mrd.LeafGroups.clear();
                mrd.LeafToGroup.assign(leafCount, 0xFFFFFFFFu);
                mrd.InteriorToGroup.assign(dagCount > leafCount ? dagCount - leafCount : 0, 0xFFFFFFFFu);
                if (mrd.PersistentParentIndex.size() >= dagCount)
                {
                    AZStd::unordered_map<uint32_t, uint32_t> groupByFirstParent;
                    for (uint32_t leaf = 0; leaf < leafCount; ++leaf)
                    {
                        const uint32_t firstParent = mrd.PersistentParentIndex[leaf];
                        if (firstParent == 0xFFFFFFFFu)
                        {
                            continue;   // rootless leaf -- no coarser fallback exists
                        }
                        auto [it, added] = groupByFirstParent.emplace(
                            firstParent, static_cast<uint32_t>(mrd.LeafGroups.size()));
                        if (added)
                        {
                            MeshRenderData::LeafGroup group;
                            group.m_parentFirst = firstParent;
                            mrd.LeafGroups.push_back(AZStd::move(group));
                        }
                        const uint32_t g = it->second;
                        mrd.LeafToGroup[leaf] = g;
                        const uint32_t page = mrd.ClusterToPage[leaf];
                        auto& pages = mrd.LeafGroups[g].m_pages;
                        if (page != 0xFFFFFFFFu &&
                            AZStd::find(pages.begin(), pages.end(), page) == pages.end())
                        {
                            pages.push_back(page);
                        }
                    }
                    // Parent ranges: groups' first parents partition level 1; the last
                    // range ends where level 2 starts (= the smallest finite parent id
                    // any interior cluster points at), or at the DAG's end.
                    uint32_t level2First = dagCount;
                    for (uint32_t c = leafCount; c < dagCount; ++c)
                    {
                        const uint32_t p = mrd.PersistentParentIndex[c];
                        if (p != 0xFFFFFFFFu)
                        {
                            level2First = AZStd::GetMin(level2First, p);
                        }
                    }
                    AZStd::vector<uint32_t> sortedGroups(mrd.LeafGroups.size());
                    for (uint32_t g = 0; g < sortedGroups.size(); ++g) { sortedGroups[g] = g; }
                    AZStd::sort(sortedGroups.begin(), sortedGroups.end(),
                        [&](uint32_t a, uint32_t b)
                        { return mrd.LeafGroups[a].m_parentFirst < mrd.LeafGroups[b].m_parentFirst; });
                    for (size_t i = 0; i < sortedGroups.size(); ++i)
                    {
                        MeshRenderData::LeafGroup& group = mrd.LeafGroups[sortedGroups[i]];
                        const uint32_t rangeEnd = (i + 1 < sortedGroups.size())
                            ? mrd.LeafGroups[sortedGroups[i + 1]].m_parentFirst
                            : level2First;
                        group.m_parentCount = rangeEnd > group.m_parentFirst ? rangeEnd - group.m_parentFirst : 0;
                        for (uint32_t p = group.m_parentFirst;
                             p < group.m_parentFirst + group.m_parentCount && p < dagCount; ++p)
                        {
                            if (p >= leafCount)
                            {
                                mrd.InteriorToGroup[p - leafCount] = sortedGroups[i];
                            }
                        }
                    }
                }
                mrd.PagedLookupsBuilt = true;
            }

            // ---- Per-rebuild: residency + group completeness -> the packed map.
            auto pageSlot = [&](uint32_t pg) -> uint32_t
            {
                return m_pageResidency.GetSlot(MeshletsPageKey(&mrd, pg));
            };
            AZStd::vector<bool> groupComplete(mrd.LeafGroups.size(), false);
            for (uint32_t g = 0; g < mrd.LeafGroups.size(); ++g)
            {
                bool complete = !mrd.LeafGroups[g].m_pages.empty();
                for (uint32_t pg : mrd.LeafGroups[g].m_pages)
                {
                    complete = complete && (pageSlot(pg) != MeshletsPageResidency::InvalidSlot);
                }
                groupComplete[g] = complete;
            }
            bool allInteriorResident = true;
            for (uint32_t pg = 0; pg < mrd.PersistentPageTable.size(); ++pg)
            {
                if ((mrd.PersistentPageTable[pg].m_flags & PageFlagAlwaysResident) &&
                    pageSlot(pg) == MeshletsPageResidency::InvalidSlot)
                {
                    allInteriorResident = false;
                }
            }

            AZStd::vector<AZ::u32> map(dagCount, 0u);
            for (uint32_t c = 0; c < dagCount; ++c)
            {
                AZ::u32 value = 0;
                const uint32_t pg = mrd.ClusterToPage[c];
                if (pg != 0xFFFFFFFFu)
                {
                    const uint32_t slot = pageSlot(pg);
                    if (slot != MeshletsPageResidency::InvalidSlot && slot < 0x400000u)
                    {
                        const uint32_t local = c - mrd.PersistentPageTable[pg].m_clusterFirst;
                        value |= (1u << 30) | ((slot & 0x3FFFFFu) << 8) | (local & 0xFFu);
                    }
                }
                bool complete;
                if (c < leafCount)
                {
                    const uint32_t g = mrd.LeafToGroup[c];
                    // Rootless leaves have no fallback: "complete" == own page resident.
                    complete = (g != 0xFFFFFFFFu) ? groupComplete[g] : ((value >> 30) & 1u) != 0u;
                }
                else
                {
                    const uint32_t g = (c - leafCount < mrd.InteriorToGroup.size())
                        ? mrd.InteriorToGroup[c - leafCount] : 0xFFFFFFFFu;
                    // Level-1 parents mirror their leaf group; level-2+ children are
                    // always-resident interiors -- always refinable.
                    complete = (g != 0xFFFFFFFFu) ? groupComplete[g] : true;
                }
                value |= complete ? (1u << 31) : 0u;
                map[c] = value;
            }

            // Recreate the map buffer via the proven initial-data path (rare events).
            SrgBufferDescriptor mapDesc(
                RPI::CommonBufferPoolType::ReadOnly, RHI::Format::Unknown,
                RHI::BufferBindFlags::ShaderRead,
                static_cast<uint32_t>(sizeof(AZ::u32)), dagCount,
                Name{ "MeshletsPagedClusterMap" }, Name{ "m_pagedClusterMap" }, 0, 0,
                reinterpret_cast<uint8_t*>(map.data()));
            mrd.PagedClusterMapBuffer = UtilityClass::CreateBuffer("Meshlets", mapDesc, nullptr);
            mrd.PagedModeActive =
                allInteriorResident && m_pagePoolBuffer && mrd.PagedClusterMapBuffer;

            // Push the paged state into the live object SRG (also applied on any
            // future EnsureMeshShaderResources rebuild).
            if (mrd.MeshShaderObjectSrg)
            {
                auto& srg = mrd.MeshShaderObjectSrg;
                if (srg->FindShaderInputBufferIndex(Name{ "m_pagedClusterMap" }).IsValid())
                {
                    SrgBufferDescriptor bind;
                    bind.m_paramNameInSrg = Name{ "m_pagedClusterMap" };
                    if (mrd.PagedClusterMapBuffer)
                    {
                        UtilityClass::BindBufferToSrg("Meshlets", mrd.PagedClusterMapBuffer, bind, srg);
                    }
                    if (m_pagePoolBuffer)
                    {
                        bind.m_paramNameInSrg = Name{ "m_pagePool" };
                        UtilityClass::BindBufferToSrg("Meshlets", m_pagePoolBuffer, bind, srg);
                    }
                    srg->SetConstant(
                        srg->FindShaderInputConstantIndex(Name{ "m_pagedMode" }),
                        mrd.PagedModeActive ? 1u : 0u);
                    if (!srg->IsQueuedForCompile())
                    {
                        srg->Compile();
                    }
                }
            }
        }

        bool MeshletsFeatureProcessor::BuildInstanceDrawPacket(
            MeshletsRenderInstance& instance, MeshRenderData& meshRenderData)
        {
            // Phase 5: hardware mesh-shader path (r_meshletsHwMeshShader). On success the
            // instance's camera packet is the DispatchMesh forward item; on any failure
            // (unsupported device, shader not processed yet, resources missing) fall
            // through to the shipping vertex-pull path below.
            if (r_meshletsHwMeshShader && BuildMeshShaderDrawPacket(instance, meshRenderData))
            {
                return true;
            }

            // Streaming-exclusive meshes must NEVER fall through to the vertex-pull
            // path -- its ObjectSrg stream views were deliberately never created.
            // Null packets render nothing until the paged path's prerequisites arrive.
            if (meshRenderData.MonolithicDropped)
            {
                instance.DrawPacket = nullptr;
                instance.ShadowDrawPacket = nullptr;
                return true;
            }

            if (!m_renderPass)
            {
                return false;
            }
            if (!meshRenderData.ObjectSrg)
            {
                AZ_Error("Meshlets", false, "Cannot build draw packet: missing per-object SRG");
                return false;
            }
            if (!m_renderShader)
            {
                AZ_Error("Meshlets", false, "Cannot build draw packet: render shader not loaded");
                return false;
            }

            // Per-instance SRG: holds the object id used to index transform arrays in SceneSrg.
            // Created once; the per-frame cull rebuild reuses it (recreating + recompiling
            // an SRG every frame would be wasteful).
            if (!instance.InstanceSrg)
            {
                instance.InstanceSrg = RPI::ShaderResourceGroup::Create(
                    m_renderShader->GetAsset(), AZ::Name{ "MeshletsInstanceRenderSrg" });
                if (!instance.InstanceSrg)
                {
                    AZ_Error("Meshlets", false, "Failed to create per-instance Render Srg");
                    return false;
                }
                RHI::ShaderInputConstantIndex objectIdHandle =
                    instance.InstanceSrg->FindShaderInputConstantIndex(Name("m_objectId"));
                if (!instance.InstanceSrg->SetConstant(objectIdHandle, instance.ObjectId.GetIndex()))
                {
                    AZ_Error("Meshlets", false, "Failed to bind Render Constant [m_objectId]");
                    return false;
                }
                // Step B: this is the NON-instanced (cull-on / single-instance) path -- force
                // m_useInstancing = 0 so the VS reads m_objectId, never m_instanceObjectIds.
                // Explicit for safety even though SRG constants default to 0.
                const RHI::ShaderInputConstantIndex useInstHandle =
                    instance.InstanceSrg->FindShaderInputConstantIndex(Name("m_useInstancing"));
                instance.InstanceSrg->SetConstant(useInstHandle, 0u);
                // Bind a VALID (but unread, since m_useInstancing=0) buffer to the
                // m_instanceObjectIds SRV so the SRG has no unbound resource view at compile.
                // The mesh's m_indices StructuredBuffer<uint> is a type-compatible, always-present
                // stand-in; the VS never samples it on this path.
                {
                    const uint8_t idxSem = static_cast<uint8_t>(RenderStreamsSemantics::Indices);
                    if (idxSem < meshRenderData.RenderBuffers.size() && meshRenderData.RenderBuffers[idxSem])
                    {
                        SrgBufferDescriptor bind;
                        bind.m_paramNameInSrg = Name{ "m_instanceObjectIds" };
                        UtilityClass::BindBufferToSrg(
                            "Meshlets", meshRenderData.RenderBuffers[idxSem], bind, instance.InstanceSrg);
                    }
                }
                instance.InstanceSrg->Compile();
            }

            // Validate that all render-side buffers were successfully created.
            // A missing buffer (null m_bufferData during construction, or a failed
            // CreateBufferAndBindToSrg) leaves the ObjectSrg with an uninitialized
            // SRV slot. The vertex shader would read garbage and the GPU would hang.
            {
                const uint32_t streamCount = static_cast<uint32_t>(meshRenderData.RenderBuffers.size());
                for (uint32_t s = 0; s < streamCount; ++s)
                {
                    if (!meshRenderData.RenderBuffers[s])
                    {
                        AZ_Error("Meshlets", false,
                            "BuildInstanceDrawPacket: render stream %u (%s) has no buffer -- "
                            "cannot build a safe DrawPacket",
                            s, meshRenderData.RenderBuffersDescriptors[s].m_bufferName.GetCStr());
                        return false;
                    }
                }
                if (meshRenderData.IndexCount == 0)
                {
                    AZ_Error("Meshlets", false,
                        "BuildInstanceDrawPacket: IndexCount is 0 -- nothing to draw");
                    return false;
                }
            }

            // Build the DrawPacket with one DrawItem per active pass. The View
            // distributes each DrawItem to the draw list matching its tag, so the
            // depth, shadow and forward passes each see their respective items.
            //
            // The forward PBR item is the primary color path: it is rendered by
            // the STANDARD Atom ForwardPass (which binds the ForwardPassSrg
            // lighting resources for every draw item it submits). The gem-private
            // debug render pass is kept only as a fallback for when the forward
            // shader failed to load/compile.

            // Phase 6b (increment 1a): draw indirectly. EnsureIndirectArgs builds a
            // per-mesh geometry view holding a single static DrawIndirectCommand
            // {IndexCount,1,0,0} -- same output as the prior DrawLinear, but on the
            // indirect path the cull compute will later drive. Falls back to direct
            // DrawLinear if the signature/buffer aren't available.
            // Ensure the per-mesh whole-mesh indirect args + index buffer exist (the
            // index buffer is also needed by the per-instance culled geometry view).
            const bool indirectReady =
                instance.RenderObject->EnsureIndirectArgs(meshRenderData, m_drawIndirectSignature.get());

            RHI::GeometryView* geometryView = nullptr;
            if (m_debugControls.m_cullEnabled && m_debugControls.m_gpuCull &&
                instance.GpuCullResourcesReady && instance.GpuCullDrawActive)
            {
                // GPU cluster cull active (this instance straddles the frustum): a single
                // non-indexed DrawIndirect over the compacted visible-index stream the cull
                // compute fills each frame. Fully-inside instances fall through to the
                // whole-mesh indirect view below (no compaction overhead).
                geometryView = &instance.GpuCullGeometryView;
            }
            else if (m_debugControls.m_cullEnabled && !m_debugControls.m_gpuCull && instance.CullResourcesReady)
            {
                // CPU cluster cull active: use the per-instance culled command set
                // (set up this frame by CullInstanceAndRebuildPacket).
                geometryView = &instance.CameraGeometryView;
            }
            else if (indirectReady)
            {
                geometryView = &meshRenderData.IndirectGeometryView;
            }
            else
            {
                // Fallback: direct non-indexed draw of IndexCount linear vertices.
                RHI::DrawLinear drawLinear;
                drawLinear.m_vertexCount = meshRenderData.IndexCount;
                drawLinear.m_vertexOffset = 0;
                m_geometryView.SetDrawArguments(drawLinear);
                geometryView = &m_geometryView;
            }

            RHI::DrawPacketBuilder drawPacketBuilder(RHI::MultiDevice::AllDevices);
            drawPacketBuilder.Begin(nullptr);
            drawPacketBuilder.SetGeometryView(geometryView);

            // Bind both SRGs: per-object (vertex streams, shared) + per-instance (object id).
            // Scene/View/Pass SRGs are bound by the passes themselves.
            drawPacketBuilder.AddShaderResourceGroup(meshRenderData.ObjectSrg->GetRHIShaderResourceGroup());
            drawPacketBuilder.AddShaderResourceGroup(instance.InstanceSrg->GetRHIShaderResourceGroup());

            // Forward PBR (primary) is rendered by the standard ForwardPass and
            // requires the per-material SRG (frequency PerMaterial). Resolve + bind
            // it here so it applies to the forward DrawItem. If it isn't ready yet
            // (material asset still loading), defer the whole packet and retry --
            // binding the forward item without its material SRG would read garbage.
            // Debug tab can force the UV debug shader or disable the forward item.
            const bool useForward = (m_forwardPipelineState && m_forwardDrawListTag.IsValid())
                && m_debugControls.m_forwardPassEnabled && !m_debugControls.m_useDebugShader;
            if (useForward)
            {
                // Materials are slot-shared across LODs: resolve on LOD0, then bind
                // LOD0's SRG even when this instance draws a coarser LOD (its own
                // MeshRenderData.MaterialSrg is never populated by EnsureMaterialSrg,
                // which only resolves LOD0). Mirror it onto this LOD's MeshRenderData
                // so subsequent frames find it directly.
                Data::Instance<RPI::ShaderResourceGroup> materialSrg;
                if (instance.RenderObject->EnsureMaterialSrg(instance.MeshIndex, m_forwardShader))
                {
                    materialSrg = instance.RenderObject->GetMaterialSrgForMesh(instance.MeshIndex);
                    if (materialSrg && !meshRenderData.MaterialSrg)
                    {
                        meshRenderData.MaterialSrg = materialSrg;
                        meshRenderData.MaterialResolved = true;
                    }
                }
                if (!materialSrg)
                {
                    AZ_TracePrintf("Meshlets",
                        "Deferring DrawPacket build for instance: material SRG for mesh %u "
                        "not ready yet. Will retry next frame.\n", instance.MeshIndex);
                    instance.DrawPacket = nullptr;
                    return true;  // success-with-deferral
                }
                drawPacketBuilder.AddShaderResourceGroup(
                    materialSrg->GetRHIShaderResourceGroup());
            }

            int drawItemCount = 0;

            // PERF (hardware input-assembly): explicit stream-index sets. The depth/shadow/
            // motion PSOs declare a POSITION-only layout (1 channel), so they select stream
            // [0] only -- NOT the view's full set (the view now carries 5 streams; using
            // GetFullStreamBufferIndices() there would feed a 5-index set into a 1-channel
            // layout and DXGI_DEVICE_HUNG on AMD). The forward PSO declares the 5-channel
            // layout and selects [0..4] in the matching order. These are stream INDICES into
            // the selected geometry view, whose order is [POSITION,NORMAL,TANGENT,BITANGENT,UV].
            RHI::StreamBufferIndices posOnly;
            posOnly.AddIndex(0);
            RHI::StreamBufferIndices allFive;
            for (AZ::u8 i = 0; i < 5; ++i)
            {
                allFive.AddIndex(i);
            }

            // DrawItem: depth prepass. Tagged "depth" so Atom's early DepthPrePass
            // renders meshlet depth into the main depth buffer (read by
            // FullscreenShadow, SSAO, reflections, and the forward depth test).
            // This is the fix for meshlets appearing translucent / shadows passing
            // through -- the previous gem-private depth pass ran after OpaquePass,
            // too late for those depth-consuming effects.
            // PERF: the depth PSO uses a hardware POSITION input layout (not vertex-pull).
            // It can only be drawn when this mesh has a valid POSITION IA stream AND the
            // selected geometry view actually carries it (ValidateStreamBufferViews) -- a
            // stale/missing IA channel would DXGI_DEVICE_HUNG on AMD. If the IA buffer
            // failed to allocate, the depth item is skipped (safe; the mesh still renders
            // forward -- depth-consuming effects degrade rather than crashing).
            if (m_depthPipelineState && m_depthDrawListTag.IsValid() && m_debugControls.m_depthPassEnabled &&
                meshRenderData.PositionStreamValid)
            {
                // POSITION-only explicit index set (stream [0]). The previous fragile
                // GetStreamBufferViews().size()==layout.size() guard self-skipped depth once the
                // view grew to 5 streams (5 != 1); the explicit posOnly set is correct regardless
                // of how many streams the view carries. PositionStreamValid + the debug validate
                // check (count+stride, DEBUG-only) keep the empty-fallback-view case safe.
                if (RHI::ValidateStreamBufferViews(m_depthInputLayout, *geometryView, posOnly))
                {
                    RHI::DrawPacketBuilder::DrawRequest depthDrawRequest;
                    depthDrawRequest.m_listTag = m_depthDrawListTag;
                    depthDrawRequest.m_pipelineState = m_depthPipelineState;
                    depthDrawRequest.m_streamIndices = posOnly;
                    depthDrawRequest.m_stencilRef = 0;
                    depthDrawRequest.m_sortKey = 0;
                    drawPacketBuilder.AddDrawItem(depthDrawRequest);
                    ++drawItemCount;
                }
            }

            // NOTE: the shadow DrawItem is NOT added to this (camera) packet. With
            // per-cluster cull active the camera geometry view is the CULLED set, which is
            // wrong for shadows -- back-facing-to-camera clusters still cast shadows. The
            // shadow is built into a SEPARATE whole-mesh packet (instance.ShadowDrawPacket)
            // below so shadows always render every cluster.

            // DrawItem: motion vectors -- rendered by the standard MeshMotionVector
            // pass (tag "motion"). Produces per-pixel screen-space motion so TAA /
            // temporal upscaling don't ghost the meshlet when the camera or object
            // moves. Depth-tests (reverse-Z) against the prepass buffer.
            if (m_motionPipelineState && m_motionDrawListTag.IsValid() && m_debugControls.m_motionPassEnabled &&
                meshRenderData.PositionStreamValid &&
                RHI::ValidateStreamBufferViews(m_motionInputLayout, *geometryView, posOnly))
            {
                RHI::DrawPacketBuilder::DrawRequest motionDrawRequest;
                motionDrawRequest.m_listTag = m_motionDrawListTag;
                motionDrawRequest.m_pipelineState = m_motionPipelineState;
                motionDrawRequest.m_streamIndices = posOnly;   // POSITION-only hardware-IA layout.
                motionDrawRequest.m_stencilRef = 0;
                motionDrawRequest.m_sortKey = 0;
                drawPacketBuilder.AddDrawItem(motionDrawRequest);
                ++drawItemCount;
            }

            // DrawItem: forward PBR (primary) -- rendered by the standard ForwardPass.
            // (useForward + the material SRG were resolved/bound above.)
            // PERF (hardware input-assembly): the forward PSO declares a 5-channel layout
            // (POSITION,NORMAL,TANGENT,BITANGENT,UV). It can ONLY be drawn when the mesh has
            // valid POSITION + forward IA streams AND the selected geometry view carries all 5
            // (ValidateStreamBufferViews). If the IA buffers failed to allocate, SKIP the
            // forward item -- binding a partial/empty layout would DXGI_DEVICE_HUNG on AMD.
            const bool forwardIaReady =
                useForward && meshRenderData.PositionStreamValid && meshRenderData.ForwardStreamsValid &&
                RHI::ValidateStreamBufferViews(m_forwardInputLayout, *geometryView, allFive);
            if (forwardIaReady)
            {
                RHI::DrawPacketBuilder::DrawRequest forwardDrawRequest;
                forwardDrawRequest.m_listTag = m_forwardDrawListTag;
                forwardDrawRequest.m_pipelineState = m_forwardPipelineState;
                forwardDrawRequest.m_streamIndices = allFive;   // POSITION,NORMAL,TANGENT,BITANGENT,UV.
                // Stencil ref marks meshlet pixels so the downstream Reflections
                // (IBL specular) and DiffuseGlobalIllumination fullscreen passes
                // process them -- identical to standard opaque meshes.
                forwardDrawRequest.m_stencilRef = static_cast<uint8_t>(
                    Render::StencilRefs::UseIBLSpecularPass | Render::StencilRefs::UseDiffuseGIPass);
                forwardDrawRequest.m_sortKey = 0;
                drawPacketBuilder.AddDrawItem(forwardDrawRequest);
                ++drawItemCount;
            }
            else if (useForward)
            {
                // Forward shader is selected but its hardware-IA streams aren't ready --
                // skip the forward item this build (do NOT bind a partial layout). The
                // depth/motion/shadow items above still render; rendering degrades safely.
                AZ_WarningOnce("Meshlets", false,
                    "BuildInstanceDrawPacket: forward hardware-IA streams not ready "
                    "(PositionStreamValid=%d ForwardStreamsValid=%d) -- skipping forward DrawItem.",
                    meshRenderData.PositionStreamValid ? 1 : 0,
                    meshRenderData.ForwardStreamsValid ? 1 : 0);
            }
            else
            {
                // Fallback: gem-private debug render pass (flat UV color). Only
                // used when the forward PBR shader failed to load/compile.
                RHI::DrawPacketBuilder::DrawRequest renderDrawRequest;
                if (m_renderPass && m_renderPass->FillDrawRequestData(renderDrawRequest))
                {
                    renderDrawRequest.m_stencilRef = 0;
                    renderDrawRequest.m_sortKey = 0;
                    drawPacketBuilder.AddDrawItem(renderDrawRequest);
                    ++drawItemCount;
                }
            }

            // If no color/depth/shadow item could be built yet (pass pipeline
            // states not ready on the first frame), defer the whole packet and
            // retry next frame.
            if (drawItemCount == 0)
            {
                AZ_TracePrintf("Meshlets",
                    "Deferring DrawPacket build for instance: no pass pipeline state "
                    "ready yet. Will retry once one becomes available.\n");
                instance.DrawPacket = nullptr;
                return true;  // success-with-deferral
            }

            instance.DrawPacket = drawPacketBuilder.End();

            if (!instance.DrawPacket)
            {
                AZ_Error("Meshlets", false, "Failed to build the Meshlet DrawPacket");
                return false;
            }

            // Separate SHADOW packet: WHOLE-MESH geometry (every cluster casts a shadow),
            // sharing the per-object + per-instance SRGs. The per-cluster cull narrows only
            // the camera packet above; feeding its camera-cone-culled set to the shadow map
            // dropped back-facing clusters' shadows. Rebuilt alongside the camera packet.
            instance.ShadowDrawPacket = nullptr;
            instance.DepthDrawPacket = nullptr;
            instance.LateDepthDrawPacket = nullptr;
            // PERF (hardware input-assembly): the shadow PSO uses a POSITION-only layout, so
            // the shadow item selects stream [0] of the WHOLE-MESH IndirectGeometryView (always
            // every cluster -- shadows cast from off-camera geometry too). Gated on
            // PositionStreamValid + the debug validate check; if the IA buffer failed the shadow
            // item is skipped (meshlet stops casting shadows but does not hang).
            if (m_shadowPipelineState && m_shadowDrawListTag.IsValid() &&
                m_debugControls.m_shadowPassEnabled && indirectReady &&
                meshRenderData.PositionStreamValid &&
                RHI::ValidateStreamBufferViews(m_shadowInputLayout, meshRenderData.IndirectGeometryView, posOnly))
            {
                RHI::DrawPacketBuilder shadowBuilder(RHI::MultiDevice::AllDevices);
                shadowBuilder.Begin(nullptr);
                shadowBuilder.SetGeometryView(&meshRenderData.IndirectGeometryView);
                shadowBuilder.AddShaderResourceGroup(meshRenderData.ObjectSrg->GetRHIShaderResourceGroup());
                shadowBuilder.AddShaderResourceGroup(instance.InstanceSrg->GetRHIShaderResourceGroup());
                RHI::DrawPacketBuilder::DrawRequest shadowDrawRequest;
                shadowDrawRequest.m_listTag = m_shadowDrawListTag;
                shadowDrawRequest.m_pipelineState = m_shadowPipelineState;
                shadowDrawRequest.m_streamIndices = posOnly;   // POSITION-only hardware-IA layout.
                shadowDrawRequest.m_stencilRef = 0;
                shadowDrawRequest.m_sortKey = 0;
                shadowBuilder.AddDrawItem(shadowDrawRequest);
                instance.ShadowDrawPacket = shadowBuilder.End();
            }

            // Only trace on the non-cull (build-once) path. With culling enabled the
            // packet is rebuilt every frame, which would spam this line.
            if (!m_debugControls.m_cullEnabled)
            {
                AZ_TracePrintf("Meshlets",
                    "BuildInstanceDrawPacket: OK -- vertexCount=%u, objectId=%u, "
                    "depthPass=%s, shadowPass=%s, motionPass=%s, forwardPass=%s\n",
                    meshRenderData.IndexCount,
                    instance.ObjectId.GetIndex(),
                    m_depthPipelineState ? "yes" : "no",
                    m_shadowPipelineState ? "yes" : "no",
                    m_motionPipelineState ? "yes" : "no",
                    useForward ? "yes" : "debug-fallback");
            }

            return true;
        }

        void MeshletsFeatureProcessor::CullInstanceAndRebuildPacket(
            MeshletsRenderInstance& instance, MeshRenderData& meshRenderData,
            const AZ::Frustum& frustum, const AZ::Vector3& cameraPos,
            uint32_t& outVisible, uint32_t& outCulled)
        {
            outVisible = 0;
            outCulled = 0;
            if (!instance.RenderObject || !m_transformServiceFeatureProcessor)
            {
                return;
            }

            // The culled camera geometry view reuses the per-mesh index buffer that
            // EnsureIndirectArgs creates; make sure it exists.
            if (!instance.RenderObject->EnsureIndirectArgs(meshRenderData, m_drawIndirectSignature.get()))
            {
                return;
            }

            const AZ::Transform xform =
                m_transformServiceFeatureProcessor->GetTransformForId(instance.ObjectId);
            const AZ::Matrix4x4 objectToWorld = AZ::Matrix4x4::CreateFromTransform(xform);

            const uint32_t visible = instance.RenderObject->CullClustersToCommands(
                instance.MeshIndex, frustum, cameraPos, objectToWorld,
                m_debugControls.m_frustumCull, m_debugControls.m_coneCull,
                instance.CullCommandStaging, outCulled);
            outVisible = visible;

            if (visible == 0)
            {
                // Entire instance culled -- draw nothing this frame.
                instance.CullResourcesReady = false;
                instance.DrawPacket = nullptr;
                return;
            }

            // Per-instance ring of indirect-args buffers (rotates per frame -> no
            // CPU-writes-while-GPU-reads hazard; the AMD-safe per-frame pattern).
            if (!instance.CullArgsRing)
            {
                instance.CullArgsRing = AZStd::make_unique<RPI::RingBuffer>(
                    AZStd::string::format("MeshletsCullArgs_%u", instance.ObjectId.GetIndex()),
                    RPI::CommonBufferPoolType::Indirect,
                    static_cast<uint32_t>(sizeof(AZ::u32)));
            }
            instance.CullArgsRing->AdvanceCurrentBufferAndUpdateData(
                instance.CullCommandStaging.data(),
                instance.CullCommandStaging.size() * sizeof(AZ::u32));

            const Data::Instance<RPI::Buffer>& argsBuf = instance.CullArgsRing->GetCurrentBuffer();
            if (!argsBuf || !argsBuf->GetRHIBuffer())
            {
                instance.CullResourcesReady = false;
                return;
            }

            // Camera geometry view: shared index buffer + culled DrawIndexedIndirect
            // (one command per visible cluster; 5 u32 = 20 bytes each).
            instance.CullArgsView = RHI::IndirectBufferView(
                *argsBuf->GetRHIBuffer(), *m_drawIndirectSignature,
                0, visible * 5 * static_cast<uint32_t>(sizeof(AZ::u32)),
                m_drawIndirectSignature->GetByteStride());
            instance.CameraGeometryView.SetIndexBufferView(meshRenderData.IndexBufferViewRHI);
            // Hardware-IA streams (rebuilt per frame -- clear+add is idempotent). Re-add ALL
            // streams in the SAME order as IndirectGeometryView so the layout matches:
            // [POSITION,NORMAL,TANGENT,BITANGENT,UV]. ClearStreamBufferViews() drops the prior
            // frame's set first; we then re-add POSITION, plus the forward four when valid.
            if (meshRenderData.PositionStreamValid)
            {
                instance.CameraGeometryView.ClearStreamBufferViews();
                instance.CameraGeometryView.AddStreamBufferView(meshRenderData.PositionStreamView);
                if (meshRenderData.ForwardStreamsValid)
                {
                    instance.CameraGeometryView.AddStreamBufferView(meshRenderData.NormalStreamView);
                    instance.CameraGeometryView.AddStreamBufferView(meshRenderData.TangentStreamView);
                    instance.CameraGeometryView.AddStreamBufferView(meshRenderData.BitangentStreamView);
                    instance.CameraGeometryView.AddStreamBufferView(meshRenderData.UvStreamView);
                }
            }
            RHI::DrawIndirect indirectArgs(visible, instance.CullArgsView, 0);
            instance.CameraGeometryView.SetDrawArguments(RHI::DrawArguments(indirectArgs));
            instance.CullResourcesReady = true;

            // Rebuild the packet; BuildInstanceDrawPacket now picks CameraGeometryView.
            BuildInstanceDrawPacket(instance, meshRenderData);
        }

        MeshletsFeatureProcessor::DebugStats MeshletsFeatureProcessor::GetDebugStats() const
        {
            DebugStats stats;
            stats.m_renderObjectCount = static_cast<uint32_t>(m_meshletsRenderObjects.size());
            stats.m_instanceCount     = static_cast<uint32_t>(m_instances.size());
            stats.m_depthActive    = (m_depthPipelineState   != nullptr);
            stats.m_shadowActive   = (m_shadowPipelineState  != nullptr);
            stats.m_forwardActive  = (m_forwardPipelineState != nullptr);
            stats.m_motionActive   = (m_motionPipelineState  != nullptr);
            stats.m_indirectActive = (m_drawIndirectSignature != nullptr);

            for (MeshletsRenderObject* ro : m_meshletsRenderObjects)
            {
                if (!ro)
                {
                    continue;
                }
                DebugObjectInfo info;
                info.m_name = ro->GetName();
                for (const auto& inst : m_instances)
                {
                    if (inst && inst->RenderObject == ro)
                    {
                        ++info.m_instances;
                    }
                }
                // Report the FINEST level (LOD0) counts as the representative figure
                // (clusters/triangles/vertices shown in the HUD are the worst case;
                // distant instances draw fewer). Null mesh entries at LOD0 are
                // skipped defensively. m_lods reflects the real available LOD count.
                ModelLodDataArray& lod0 = ro->GetMeshletsRenderData(0);
                for (MeshRenderData* mrd : lod0)
                {
                    if (!mrd)
                    {
                        continue;
                    }
                    info.m_clusters  += mrd->MeshletsCount;
                    info.m_triangles += mrd->IndexCount / 3;
                    info.m_vertices  += mrd->VertexCount;
                    info.m_materialResolved = info.m_materialResolved || mrd->MaterialResolved;
                }
                info.m_lods = ro->GetLodCount();   // real LOD count (1 for a stale pack).
                stats.m_totalClusters  += info.m_clusters;
                stats.m_totalTriangles += info.m_triangles;
                stats.m_totalVertices  += info.m_vertices;
                stats.m_objects.push_back(AZStd::move(info));
            }

            // ---- LOD debug: per-instance histogram + rendered-vs-full vertex count ----
            // Shows what LOD selection is actually doing: how many instances sit at each
            // LOD, and the vertex reduction vs every instance drawing LOD0 (the win).
            for (const auto& inst : m_instances)
            {
                if (!inst || !inst->RenderObject)
                {
                    continue;
                }
                MeshletsRenderObject* ro = inst->RenderObject;
                stats.m_maxLodCount = AZStd::GetMax(stats.m_maxLodCount, ro->GetLodCount());
                const uint32_t lodIdx = inst->LodIndex;
                if (lodIdx < stats.m_lodHistogram.size())
                {
                    ++stats.m_lodHistogram[lodIdx];
                }
                // Rendered verts = this instance's CURRENT LOD vert count; full = LOD0.
                ModelLodDataArray& selLod = ro->GetMeshletsRenderData(lodIdx);
                if (inst->MeshIndex < selLod.size() && selLod[inst->MeshIndex])
                {
                    stats.m_renderedVertices += selLod[inst->MeshIndex]->VertexCount;
                }
                ModelLodDataArray& lod0 = ro->GetMeshletsRenderData(0);
                if (inst->MeshIndex < lod0.size() && lod0[inst->MeshIndex])
                {
                    stats.m_fullVertices += lod0[inst->MeshIndex]->VertexCount;
                }
            }
            return stats;
        }

        MeshletsRenderInstance* MeshletsFeatureProcessor::AddInstance(MeshletsRenderObject* meshletsRenderObject)
        {
            if (!meshletsRenderObject)
            {
                AZ_Error("Meshlets", false, "AddInstance called with null render object");
                return nullptr;
            }

            // Register the render object once. Subsequent instances of the same object
            // share its compute work and vertex buffers.
            if (AZStd::find(m_meshletsRenderObjects.begin(), m_meshletsRenderObjects.end(), meshletsRenderObject)
                == m_meshletsRenderObjects.end())
            {
                m_meshletsRenderObjects.emplace_back(meshletsRenderObject);
            }

            auto instance = AZStd::make_unique<MeshletsRenderInstance>();
            instance->RenderObject = meshletsRenderObject;
            instance->LodIndex = 0;
            instance->MeshIndex = 0;
            instance->ObjectId = m_transformServiceFeatureProcessor->ReserveObjectId();

            ModelLodDataArray& lodArray = meshletsRenderObject->GetMeshletsRenderData(instance->LodIndex);
            if (lodArray.empty() || !lodArray[instance->MeshIndex])
            {
                AZ_Error("Meshlets", false, "Render object has no mesh data for LOD %u mesh %u",
                    instance->LodIndex, instance->MeshIndex);
                m_transformServiceFeatureProcessor->ReleaseObjectId(instance->ObjectId);
                return nullptr;
            }

            if (m_renderPass && !BuildInstanceDrawPacket(*instance, *lodArray[instance->MeshIndex]))
            {
                m_transformServiceFeatureProcessor->ReleaseObjectId(instance->ObjectId);
                return nullptr;
            }

            MeshletsRenderInstance* raw = instance.get();
            m_instances.emplace_back(AZStd::move(instance));
            // Step B: register the instance in its hardware-instancing group (cull-off
            // default path), marking the group dirty so its objectId buffer + instanced
            // packets rebuild on the next Render().
            AddInstanceToGroup(raw);
            return raw;
        }

        void MeshletsFeatureProcessor::RemoveInstance(MeshletsRenderInstance* instance)
        {
            if (!instance)
            {
                return;
            }
            // Find by raw pointer; remove from the owning vector. The unique_ptr destructor
            // releases InstanceSrg and the DrawPacket reference.
            auto it = AZStd::find_if(m_instances.begin(), m_instances.end(),
                [instance](const AZStd::unique_ptr<MeshletsRenderInstance>& p) { return p.get() == instance; });
            if (it == m_instances.end())
            {
                return;
            }
            // Null the DrawPacket BEFORE destroying the instance. If this instance's
            // DrawPacket was already submitted to the View (via AddDrawPackets earlier
            // this frame), the View holds a raw pointer to it. The DrawPacket in turn
            // stores raw pointers to the SRG objects (ObjectSrg, InstanceSrg). If we
            // let the unique_ptr destroy the instance without nulling the DrawPacket
            // first, the DrawPacket's ref-count drops to zero, freeing the arena that
            // held the SRG pointer array -- but the View's draw list still holds the
            // old DrawPacket address, and SubmitDrawItems will read freed memory.
            //
            // Setting DrawPacket = nullptr releases the RHI::Ptr, freeing the packet
            // here (assuming the View doesn't hold its own Ptr -- it stores raw *).
            // That's intentional: the packet becomes invalid this frame, but the
            // View's SubmitDrawItems iterates draw items that were filtered by
            // DrawListTag during AddDrawPacket; since we null the DrawPacket *after*
            // AddDrawPackets already ran (Render -> RemoveInstance is called from
            // ReleaseInstance which is called from component teardown, not from
            // Render itself), the timing is:
            //   Frame N: Render() -> AddDrawPackets (packet in view) -> GPU submits
            //   Frame N+1: DeletePending -> RemoveInstance -> null packet -> Render()
            // The destruction happens between frames, which is safe.
            instance->DrawPacket = nullptr;
            // Step B: remove from its hardware-instancing group FIRST (marks the group
            // dirty / drops its packets) so the group no longer references this instance's
            // SRG/objectId before we free it. Done before ReleaseObjectId.
            RemoveInstanceFromGroup(instance);
            if (m_transformServiceFeatureProcessor)
            {
                m_transformServiceFeatureProcessor->ReleaseObjectId(instance->ObjectId);
            }
            *it = AZStd::move(m_instances.back());
            m_instances.pop_back();
        }

        //==============================================================================
        // Step B: hardware instancing -- group management + per-group packet build.
        //==============================================================================
        void MeshletsFeatureProcessor::AddInstanceToGroup(MeshletsRenderInstance* instance)
        {
            if (!instance || !instance->RenderObject)
            {
                return;
            }
            InstanceGroupKey key{ instance->RenderObject, instance->LodIndex, instance->MeshIndex };
            InstanceGroup& group = m_instanceGroups[key];
            // Avoid duplicate membership (defensive).
            if (AZStd::find(group.m_members.begin(), group.m_members.end(), instance) == group.m_members.end())
            {
                group.m_members.push_back(instance);
                group.m_dirty = true;
            }
        }

        void MeshletsFeatureProcessor::RemoveInstanceFromGroup(MeshletsRenderInstance* instance)
        {
            if (!instance || !instance->RenderObject)
            {
                return;
            }
            InstanceGroupKey key{ instance->RenderObject, instance->LodIndex, instance->MeshIndex };
            auto git = m_instanceGroups.find(key);
            if (git == m_instanceGroups.end())
            {
                return;
            }
            InstanceGroup& group = git->second;
            auto mit = AZStd::find(group.m_members.begin(), group.m_members.end(), instance);
            if (mit != group.m_members.end())
            {
                // Order of remaining members may shift (swap-and-pop) -- that's fine: the
                // objectId buffer is rebuilt from member order on the next dirty rebuild,
                // and SV_InstanceID indexes that fresh buffer, so consistency holds.
                *mit = group.m_members.back();
                group.m_members.pop_back();
                group.m_dirty = true;
            }
            // Drop the packets now (they referenced this member's set); they rebuild on
            // the next Render() if the group still has members.
            group.m_cameraPacket = nullptr;
            group.m_shadowPacket = nullptr;
            if (group.m_members.empty())
            {
                m_instanceGroups.erase(git);
            }
        }

        bool MeshletsFeatureProcessor::RebuildInstanceGroup(
            InstanceGroup& group, const InstanceGroupKey& key, MeshRenderData& meshRenderData)
        {
            if (!group.m_dirty)
            {
                return true;   // up to date -- nothing to do.
            }
            if (group.m_members.empty())
            {
                group.m_cameraPacket = nullptr;
                group.m_shadowPacket = nullptr;
                group.m_dirty = false;
                return true;
            }
            if (!m_renderPass || !m_renderShader || !meshRenderData.ObjectSrg)
            {
                return false;   // pipeline not ready yet -- retry next frame (still dirty).
            }

            const uint32_t memberCount = static_cast<uint32_t>(group.m_members.size());

            // ---- (1) Per-group objectId StructuredBuffer<uint> (ShaderRead SRV) ----
            // One uint per member, in member order: SV_InstanceID i -> members[i]'s objectId.
            // Rebuilt only here (membership changed); transforms are NOT in this buffer (they
            // come from SceneSrg by objectId). ReadOnly pool + ShaderRead == the same path the
            // MeshRenderData StructuredBuffer SRV streams use (NOT InputAssembly/Indirect).
            group.m_objectIdStaging.resize(memberCount);
            for (uint32_t i = 0; i < memberCount; ++i)
            {
                group.m_objectIdStaging[i] = group.m_members[i]->ObjectId.GetIndex();
            }
            {
                SrgBufferDescriptor idDesc(
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::Unknown,                 // StructuredBuffer<uint>
                    RHI::BufferBindFlags::ShaderRead,
                    sizeof(AZ::u32), memberCount,
                    Name{ "MeshletsInstanceObjectIds" }, Name{ "m_instanceObjectIds" }, 0, 0,
                    reinterpret_cast<uint8_t*>(group.m_objectIdStaging.data()));
                group.m_objectIdBuffer = UtilityClass::CreateBuffer("Meshlets", idDesc, nullptr);
            }
            if (!group.m_objectIdBuffer || !group.m_objectIdBuffer->GetRHIBuffer())
            {
                AZ_Error("Meshlets", false, "RebuildInstanceGroup: failed to create objectId buffer.");
                return false;
            }

            // ---- (2) Per-group instanced SRG (PerDraw) ----
            // Plain MeshletsInstanceRenderSrg with m_useInstancing=1 (selects the
            // SV_InstanceID -> m_instanceObjectIds path in every VS) and m_objectId=0
            // (unused on the instanced path; set for determinism). Bind the group's
            // objectId StructuredBuffer as m_instanceObjectIds. No shader-option /
            // variant-key machinery -- the branch is a plain SRG-constant runtime branch,
            // robust across all four pass shaders.
            {
                group.m_instanceSrg = RPI::ShaderResourceGroup::Create(
                    m_renderShader->GetAsset(), AZ::Name{ "MeshletsInstanceRenderSrg" });
                if (!group.m_instanceSrg)
                {
                    AZ_Error("Meshlets", false, "RebuildInstanceGroup: failed to create instanced InstanceSrg.");
                    return false;
                }
                const RHI::ShaderInputConstantIndex objectIdIdx =
                    group.m_instanceSrg->FindShaderInputConstantIndex(Name("m_objectId"));
                const RHI::ShaderInputConstantIndex useInstIdx =
                    group.m_instanceSrg->FindShaderInputConstantIndex(Name("m_useInstancing"));
                group.m_instanceSrg->SetConstant(objectIdIdx, 0u);
                if (!group.m_instanceSrg->SetConstant(useInstIdx, 1u))
                {
                    AZ_Error("Meshlets", false, "RebuildInstanceGroup: failed to set m_useInstancing.");
                    return false;
                }
                SrgBufferDescriptor bind;
                bind.m_paramNameInSrg = Name{ "m_instanceObjectIds" };
                if (!UtilityClass::BindBufferToSrg("Meshlets", group.m_objectIdBuffer, bind, group.m_instanceSrg))
                {
                    AZ_Error("Meshlets", false, "RebuildInstanceGroup: failed to bind m_instanceObjectIds.");
                    return false;
                }
                group.m_instanceSrg->Compile();
            }

            // ---- (3) Instanced DIRECT-DrawIndexed geometry view (BLOCKING FIX B1) ----
            // Ensure the per-mesh index buffer + IA streams exist (EnsureIndirectArgs creates
            // IndexBufferViewRHI + Position/Forward stream views shared by all instances).
            if (!key.m_renderObject->EnsureIndirectArgs(meshRenderData, m_drawIndirectSignature.get()))
            {
                return false;
            }
            // Configure EXACTLY like IndirectGeometryView (same index buffer, same 5 streams in
            // order POSITION,NORMAL,TANGENT,BITANGENT,UV) BUT with a DIRECT indexed draw -- NOT
            // indirect. SetDrawInstanceArguments is IGNORED on the indirect path (instanceCount
            // would stay 1 and instancing would silently draw a single instance / hang on AMD).
            group.m_instancedGeometryView.Reset();
            group.m_instancedGeometryView.SetIndexBufferView(meshRenderData.IndexBufferViewRHI);
            if (meshRenderData.PositionStreamValid)
            {
                group.m_instancedGeometryView.AddStreamBufferView(meshRenderData.PositionStreamView);
                if (meshRenderData.ForwardStreamsValid)
                {
                    group.m_instancedGeometryView.AddStreamBufferView(meshRenderData.NormalStreamView);
                    group.m_instancedGeometryView.AddStreamBufferView(meshRenderData.TangentStreamView);
                    group.m_instancedGeometryView.AddStreamBufferView(meshRenderData.BitangentStreamView);
                    group.m_instancedGeometryView.AddStreamBufferView(meshRenderData.UvStreamView);
                }
            }
            // RHI::DrawIndexed ctor is (vertexOffset, indexCount, indexOffset) -- NOT the
            // brace-init order. Use it explicitly so we don't transpose fields.
            group.m_instancedGeometryView.SetDrawArguments(
                RHI::DrawArguments(RHI::DrawIndexed(
                    /*vertexOffset*/ 0,
                    /*indexCount*/   meshRenderData.IndexCount,
                    /*indexOffset*/  0)));

            // ---- (4) Instanced packets (camera + separate whole-mesh shadow) ----
            group.m_cameraPacket = nullptr;
            group.m_shadowPacket = nullptr;

            // Same explicit stream-index sets as the per-instance path: depth/motion/shadow
            // use POSITION-only {0}; forward uses {0,1,2,3,4}.
            RHI::StreamBufferIndices posOnly;
            posOnly.AddIndex(0);
            RHI::StreamBufferIndices allFive;
            for (AZ::u8 i = 0; i < 5; ++i)
            {
                allFive.AddIndex(i);
            }

            const bool useForward = (m_forwardPipelineState && m_forwardDrawListTag.IsValid())
                && m_debugControls.m_forwardPassEnabled && !m_debugControls.m_useDebugShader;

            // Forward needs the per-mesh material SRG. Materials are slot-shared
            // across LODs, so resolve on LOD0 and bind LOD0's SRG even for a LOD>0
            // group (this group's MeshRenderData is a coarser LOD whose own
            // MaterialSrg EnsureMaterialSrg never populates). Mirror it onto this
            // LOD's MeshRenderData. If not ready, defer the rebuild (stay dirty).
            Data::Instance<RPI::ShaderResourceGroup> groupMaterialSrg;
            if (useForward)
            {
                if (!key.m_renderObject->EnsureMaterialSrg(key.m_meshIndex, m_forwardShader))
                {
                    return false;   // retry next frame; group stays dirty.
                }
                groupMaterialSrg = key.m_renderObject->GetMaterialSrgForMesh(key.m_meshIndex);
                if (!groupMaterialSrg)
                {
                    return false;   // retry next frame; group stays dirty.
                }
                if (!meshRenderData.MaterialSrg)
                {
                    meshRenderData.MaterialSrg = groupMaterialSrg;
                    meshRenderData.MaterialResolved = true;
                }
            }

            RHI::DrawPacketBuilder cameraBuilder(RHI::MultiDevice::AllDevices);
            cameraBuilder.Begin(nullptr);
            cameraBuilder.SetGeometryView(&group.m_instancedGeometryView);
            // BLOCKING FIX B3: instanceOffset MUST be 0 (SV_InstanceID is 0-based regardless of
            // StartInstanceLocation; we index solely via SV_InstanceID into m_instanceObjectIds).
            cameraBuilder.SetDrawInstanceArguments(
                RHI::DrawInstanceArguments(/*instanceCount*/ memberCount, /*instanceOffset*/ 0));
            cameraBuilder.AddShaderResourceGroup(meshRenderData.ObjectSrg->GetRHIShaderResourceGroup());
            cameraBuilder.AddShaderResourceGroup(group.m_instanceSrg->GetRHIShaderResourceGroup());
            if (useForward)
            {
                cameraBuilder.AddShaderResourceGroup(groupMaterialSrg->GetRHIShaderResourceGroup());
            }

            int cameraItemCount = 0;

            // Depth. Same root PSO as the per-instance path (the instancing branch is
            // driven by the SRG m_useInstancing constant, not a PSO variant). Same gates +
            // ValidateStreamBufferViews as per-instance.
            if (m_depthPipelineState && m_depthDrawListTag.IsValid() && m_debugControls.m_depthPassEnabled &&
                meshRenderData.PositionStreamValid &&
                RHI::ValidateStreamBufferViews(m_depthInputLayout, group.m_instancedGeometryView, posOnly))
            {
                RHI::DrawPacketBuilder::DrawRequest req;
                req.m_listTag = m_depthDrawListTag;
                req.m_pipelineState = m_depthPipelineState;
                req.m_streamIndices = posOnly;
                req.m_stencilRef = 0;
                req.m_sortKey = 0;
                cameraBuilder.AddDrawItem(req);
                ++cameraItemCount;
            }

            // Motion (POSITION-only).
            if (m_motionPipelineState && m_motionDrawListTag.IsValid() && m_debugControls.m_motionPassEnabled &&
                meshRenderData.PositionStreamValid &&
                RHI::ValidateStreamBufferViews(m_motionInputLayout, group.m_instancedGeometryView, posOnly))
            {
                RHI::DrawPacketBuilder::DrawRequest req;
                req.m_listTag = m_motionDrawListTag;
                req.m_pipelineState = m_motionPipelineState;
                req.m_streamIndices = posOnly;
                req.m_stencilRef = 0;
                req.m_sortKey = 0;
                cameraBuilder.AddDrawItem(req);
                ++cameraItemCount;
            }

            // Forward (5-channel).
            const bool forwardIaReady =
                useForward && m_forwardPipelineState &&
                meshRenderData.PositionStreamValid && meshRenderData.ForwardStreamsValid &&
                RHI::ValidateStreamBufferViews(m_forwardInputLayout, group.m_instancedGeometryView, allFive);
            if (forwardIaReady)
            {
                RHI::DrawPacketBuilder::DrawRequest req;
                req.m_listTag = m_forwardDrawListTag;
                req.m_pipelineState = m_forwardPipelineState;
                req.m_streamIndices = allFive;
                req.m_stencilRef = static_cast<uint8_t>(
                    Render::StencilRefs::UseIBLSpecularPass | Render::StencilRefs::UseDiffuseGIPass);
                req.m_sortKey = 0;
                cameraBuilder.AddDrawItem(req);
                ++cameraItemCount;
            }

            if (cameraItemCount == 0)
            {
                // No camera item could be built yet (PSO/streams not ready). Defer.
                return false;
            }
            group.m_cameraPacket = cameraBuilder.End();
            if (!group.m_cameraPacket)
            {
                AZ_Error("Meshlets", false, "RebuildInstanceGroup: failed to build instanced camera packet.");
                return false;
            }

            // Separate instanced SHADOW packet -- whole-mesh (all members cast). Uses the SAME
            // instanced geometry view + instanceCount=memberCount so EVERY instance casts.
            if (m_shadowPipelineState && m_shadowDrawListTag.IsValid() &&
                m_debugControls.m_shadowPassEnabled &&
                meshRenderData.PositionStreamValid &&
                RHI::ValidateStreamBufferViews(m_shadowInputLayout, group.m_instancedGeometryView, posOnly))
            {
                RHI::DrawPacketBuilder shadowBuilder(RHI::MultiDevice::AllDevices);
                shadowBuilder.Begin(nullptr);
                shadowBuilder.SetGeometryView(&group.m_instancedGeometryView);
                shadowBuilder.SetDrawInstanceArguments(
                    RHI::DrawInstanceArguments(/*instanceCount*/ memberCount, /*instanceOffset*/ 0));
                shadowBuilder.AddShaderResourceGroup(meshRenderData.ObjectSrg->GetRHIShaderResourceGroup());
                shadowBuilder.AddShaderResourceGroup(group.m_instanceSrg->GetRHIShaderResourceGroup());
                RHI::DrawPacketBuilder::DrawRequest req;
                req.m_listTag = m_shadowDrawListTag;
                req.m_pipelineState = m_shadowPipelineState;
                req.m_streamIndices = posOnly;
                req.m_stencilRef = 0;
                req.m_sortKey = 0;
                shadowBuilder.AddDrawItem(req);
                group.m_shadowPacket = shadowBuilder.End();
            }

            group.m_dirty = false;
            return true;
        }

        Render::TransformServiceFeatureProcessorInterface::ObjectId
            MeshletsFeatureProcessor::AddMeshletsRenderObject(MeshletsRenderObject* meshletsRenderObject)
        {
            MeshletsRenderInstance* inst = AddInstance(meshletsRenderObject);
            if (!inst)
            {
                return Render::TransformServiceFeatureProcessorInterface::ObjectId{};
            }
            AZ_Error("Meshlets", m_renderPass, "Meshlets object did not build DrawItem due to missing render pass");
            return inst->ObjectId;
        }

        void MeshletsFeatureProcessor::DeletePendingMeshletsRenderObjects()
        {
            if (m_renderObjectsMarkedForDeletion.empty())
            {
                return;
            }

            for (auto renderObject : m_renderObjectsMarkedForDeletion)
            {
                // Drop every live instance of this render object.
                // Null each DrawPacket first -- the packet holds raw SRG pointers into
                // the ObjectSrg owned by the renderObject we're about to delete.
                for (auto it = m_instances.begin(); it != m_instances.end(); )
                {
                    if ((*it)->RenderObject == renderObject)
                    {
                        (*it)->DrawPacket = nullptr;  // prevent dangling SRG pointers
                        // Step B: drop from its instancing group before freeing.
                        RemoveInstanceFromGroup(it->get());
                        if (m_transformServiceFeatureProcessor)
                        {
                            m_transformServiceFeatureProcessor->ReleaseObjectId((*it)->ObjectId);
                        }
                        *it = AZStd::move(m_instances.back());
                        m_instances.pop_back();
                    }
                    else
                    {
                        ++it;
                    }
                }

                // Step B: erase every group keyed to the render object being deleted.
                // RemoveInstanceFromGroup leaves emptied groups in the map (it only marks
                // them dirty); since the whole render object is going away, drop them so
                // they hold no stale packets/SRGs referencing the freed ObjectSrg.
                for (auto git = m_instanceGroups.begin(); git != m_instanceGroups.end(); )
                {
                    if (git->first.m_renderObject == renderObject)
                    {
                        git = m_instanceGroups.erase(git);
                    }
                    else
                    {
                        ++git;
                    }
                }

                auto it = AZStd::find(m_meshletsRenderObjects.begin(), m_meshletsRenderObjects.end(), renderObject);
                if (it != m_meshletsRenderObjects.end())
                {
                    *it = m_meshletsRenderObjects.back();
                    m_meshletsRenderObjects.pop_back();
                }
                delete renderObject;
            }
            m_renderObjectsMarkedForDeletion.clear();
        }

        void MeshletsFeatureProcessor::InvalidateAllDrawPackets()
        {
            for (auto& instance : m_instances)
            {
                if (instance)
                {
                    instance->DrawPacket = nullptr;
                    instance->DepthDrawPacket = nullptr;
                    instance->LateDepthDrawPacket = nullptr;
                    instance->ShadowDrawPacket = nullptr;
                }
            }
            // Step B: the hardware-instanced (cull-off) packets also reference the
            // (now-stale) PSOs/SRGs -- null them and mark every group dirty so the next
            // Render() rebuilds them. Keep the per-group SRG instance (recreated lazily
            // in RebuildInstanceGroup if needed) but drop the packets.
            for (auto& [key, group] : m_instanceGroups)
            {
                group.m_cameraPacket = nullptr;
                group.m_shadowPacket = nullptr;
                group.m_dirty = true;
            }
            AZ_TracePrintf("Meshlets",
                "InvalidateAllDrawPackets: nulled %zu instance DrawPacket(s) and %zu group packet(s). "
                "They will be rebuilt on the next Render() frame.\n",
                m_instances.size(), m_instanceGroups.size());
        }

        void MeshletsFeatureProcessor::RemoveMeshletsRenderObject(MeshletsRenderObject* meshletsRenderObject)
        {
            m_renderObjectsMarkedForDeletion.emplace_back(meshletsRenderObject);
        }

        // ------------------------------------------------------------------------
        // MeshletsFeatureProcessorInterface (cross-gem opaque API)
        // ------------------------------------------------------------------------
        //
        // Handle layout:
        //   The opaque InstanceHandle is the raw pointer value of the underlying
        //   MeshletsRenderInstance, reinterpreted as uint64_t. This avoids a separate
        //   handle table for now; we can swap to a generation-counter index later
        //   without changing the public API.
        //
        // Sharing:
        //   AcquireInstance is keyed by ModelAsset id. The first call for a given
        //   model constructs a MeshletsRenderObject; subsequent calls reuse it. The
        //   refcount in m_sharedRenderObjectsByAsset is incremented per instance and
        //   decremented in ReleaseInstance. When it hits zero the render object is
        //   queued for deletion.
        //
        // Failure mode:
        //   If the asset isn't ready, or no MeshletsFeatureProcessor scene is set up,
        //   we return InvalidInstanceHandle and the caller can decide whether to
        //   retry later (e.g. on OnAssetReady).

        MeshletsFeatureProcessorInterface::InstanceHandle MeshletsFeatureProcessor::AcquireInstance(
            const Data::Asset<RPI::ModelAsset>& modelAsset)
        {
            AZ_TracePrintf("Meshlets",
                "AcquireInstance: enter, modelId=%s ready=%d mapSize=%zu\n",
                modelAsset.GetId().ToString<AZStd::string>().c_str(),
                modelAsset.IsReady() ? 1 : 0,
                m_packResolver.GetMappingCount());

            if (!modelAsset.IsReady())
            {
                AZ_Error("Meshlets", false, "AcquireInstance: modelAsset not ready");
                return InvalidInstanceHandle;
            }

            // 1. Resolve sibling pack via the catalog.
            const AZ::Data::AssetId packId = m_packResolver.Find(modelAsset.GetId());
            AZ_TracePrintf("Meshlets",
                "AcquireInstance: Find returned packId=%s (valid=%d)\n",
                packId.ToString<AZStd::string>().c_str(),
                packId.IsValid() ? 1 : 0);
            if (!packId.IsValid())
            {
                {
                    AZStd::lock_guard<AZStd::mutex> lock(m_packStatusMutex);
                    m_packStatusByModel[modelAsset.GetId()] = MeshletsFeatureProcessorInterface::PackResolutionStatus::NoPack;
                }
                AZ_Warning("Meshlets", false,
                    "No .azmeshletpack product registered for model %s. Add a Meshlet Pack "
                    "rule to the source FBX or author a .meshletpack JSON sidecar.",
                    modelAsset.GetId().ToString<AZStd::string>().c_str());
                return InvalidInstanceHandle;
            }

            // 2. Blocking-load the pack on first use; cached thereafter via SharedRenderObjectEntry.
            auto packAsset =
                AZ::Data::AssetManager::Instance().GetAsset<MeshletPackAsset>(
                    packId, AZ::Data::AssetLoadBehavior::PreLoad);
            packAsset.BlockUntilLoadComplete();
            if (!packAsset.IsReady())
            {
                {
                    AZStd::lock_guard<AZStd::mutex> lock(m_packStatusMutex);
                    m_packStatusByModel[modelAsset.GetId()] = MeshletsFeatureProcessorInterface::PackResolutionStatus::LoadFailed;
                }
                AZ_Error("Meshlets", false, "Failed to load pack %s",
                         packId.ToString<AZStd::string>().c_str());
                return InvalidInstanceHandle;
            }

            // 3. Get-or-create shared render object (refcounted, keyed by modelAssetId).
            //
            // We DO build the MeshletsRenderObject even if the GPU render
            // kill-switch (r_meshletsRenderEnabled) is off, so artists still
            // exercise the validation/diagnostic path (pack header check,
            // section parsing, cluster rebasing, defensive bounds checks) the
            // moment they toggle Use Virtual Geometry on a mesh. That surfaces
            // pack-shape problems immediately in the editor log instead of
            // silently no-op'ing.
            auto& entry = m_sharedRenderObjectsByAsset[modelAsset.GetId()];
            if (entry.m_renderObject == nullptr)
            {
                entry.m_renderObject = aznew MeshletsRenderObject(modelAsset, packAsset, this);
                m_meshletsRenderObjects.push_back(entry.m_renderObject);
            }
            entry.m_refCount++;

            // Safety valve: skip wiring up the per-instance draw if the render
            // path is disabled via CVar. With GPU bounds clamping in the vertex
            // shader and CPU-side buffer validation in BuildInstanceDrawPacket,
            // the render path is safe to enable by default. Set
            // r_meshletsRenderEnabled=0 in the console if you need to
            // temporarily disable meshlet rendering for debugging.
            if (!r_meshletsRenderEnabled)
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_packStatusMutex);
                m_packStatusByModel[modelAsset.GetId()] =
                    MeshletsFeatureProcessorInterface::PackResolutionStatus::Ok;
                AZ_TracePrintf("Meshlets",
                    "AcquireInstance: r_meshletsRenderEnabled=0. "
                    "Pack resolved + render object constructed for validation, but no "
                    "GPU draw will be dispatched. Set r_meshletsRenderEnabled=1 in the "
                    "editor console to enable full meshlet rendering.\n");
                return InvalidInstanceHandle;
            }

            // 4. Add the per-call instance.
            auto* inst = AddInstance(entry.m_renderObject);
            if (!inst)
            {
                return InvalidInstanceHandle;
            }
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_packStatusMutex);
                m_packStatusByModel[modelAsset.GetId()] = MeshletsFeatureProcessorInterface::PackResolutionStatus::Ok;
            }
            return reinterpret_cast<InstanceHandle>(inst);
        }

        void MeshletsFeatureProcessor::ReleaseInstance(InstanceHandle handle)
        {
            if (handle == InvalidInstanceHandle)
            {
                return;
            }
            MeshletsRenderInstance* instance = reinterpret_cast<MeshletsRenderInstance*>(handle);

            // Drop the refcount on the shared render object; if we were the last user,
            // queue the render object for destruction.
            MeshletsRenderObject* renderObject = instance->RenderObject;
            for (auto it = m_sharedRenderObjectsByAsset.begin(); it != m_sharedRenderObjectsByAsset.end(); ++it)
            {
                if (it->second.m_renderObject == renderObject)
                {
                    if (it->second.m_refCount > 0)
                    {
                        --it->second.m_refCount;
                    }
                    if (it->second.m_refCount == 0)
                    {
                        RemoveMeshletsRenderObject(renderObject);
                        m_sharedRenderObjectsByAsset.erase(it);
                    }
                    break;
                }
            }

            // Always release the instance (DrawPacket + InstanceSrg) regardless of
            // refcount path; RemoveMeshletsRenderObject above also removes any
            // remaining instances of the same render object on the next deletion pass,
            // but explicit cleanup keeps the per-instance state crisp.
            RemoveInstance(instance);
        }

        void MeshletsFeatureProcessor::SetInstanceTransform(
            InstanceHandle handle, const AZ::Transform& worldTransform)
        {
            if (handle == InvalidInstanceHandle || !m_transformServiceFeatureProcessor)
            {
                return;
            }
            MeshletsRenderInstance* instance = reinterpret_cast<MeshletsRenderInstance*>(handle);
            m_transformServiceFeatureProcessor->SetTransformForId(instance->ObjectId, worldTransform);
        }

        MeshletsFeatureProcessorInterface::PackResolutionStatus
        MeshletsFeatureProcessor::GetPackStatus(const AZ::Data::AssetId& modelAssetId) const
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_packStatusMutex);
            auto it = m_packStatusByModel.find(modelAssetId);
            return (it != m_packStatusByModel.end()) ? it->second : MeshletsFeatureProcessorInterface::PackResolutionStatus::NotChecked;
        }

        void MeshletsFeatureProcessor::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
        {
            // OnTick can be used instead of the ::Simulate since it is set to be before the render
        }

        void MeshletsFeatureProcessor::Simulate(const RPI::FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_FUNCTION(AzRender);

            AZ_UNUSED(packet);
        }

        void MeshletsFeatureProcessor::Render(const RPI::FeatureProcessor::RenderPacket& packet)
        {
            AZ_PROFILE_FUNCTION(AzRender);

            // Remove any dangling leftovers (objects marked for deletion last frame)
            DeletePendingMeshletsRenderObjects();

            if (m_instances.empty())
            {
                return;
            }

            // Reuse persistent scratch buffers; capacity is preserved across frames
            // so a steady-state render does not heap-allocate.
            m_dispatchItemsScratch.clear();
            m_drawPacketsScratch.clear();
            m_visFrameAttachmentsScratch.clear();

            // HiZ bind resolve, once per frame before any cull-constant update:
            // last-COMPLETED pyramid slot (this frame's is being written) + LAST
            // frame's matrix (the pyramid's own clip space). Requires the barrier
            // pass -- it declares the UAV->shader-read transition.
            m_hiZBindImage = nullptr;
            m_hiZBindValid = false;
            if (m_debugControls.m_hiZCull && m_hiZGeneratePass && m_cullBarrierPass &&
                m_hiZGeneratePass->IsPersistentPyramidPopulated() && m_hasHiZPrevCullMat)
            {
                if (Data::Instance<RPI::AttachmentImage> pyramid = m_hiZGeneratePass->GetLastCompletedPyramid())
                {
                    m_hiZBindImage = pyramid;
                    m_hiZBindWorldToClip = m_hiZPrevCullMat;
                    m_hiZBindValid = true;
                }
            }
            if (m_renderPipeline)
            {
                if (RPI::ViewPtr liveView = m_renderPipeline->GetDefaultView())
                {
                    // This frame's camera renders this frame's depth -> next frame's pyramid.
                    m_hiZPrevCullMat = liveView->GetWorldToClipMatrix();
                    m_hasHiZPrevCullMat = true;
                }
            }

            // DAG-cut projection stash. Pixel scale from the PURE projection
            // (ViewToClip[1][1]); the combined matrix folds in camera rotation and
            // collapses toward 0. Invalid stash => DAG paths fall back leaf-only.
            m_dagBindValid = false;
            if (m_renderPipeline)
            {
                if (RPI::ViewPtr dagView = m_renderPipeline->GetDefaultView())
                {
                    const auto& size = m_renderPipeline->GetRenderSettings().m_size;
                    const float yScale = dagView->GetViewToClipMatrix().GetElement(1, 1);
                    if (size.m_height > 0 && yScale > 0.0f)
                    {
                        m_dagViewport = AZ::Vector2(
                            static_cast<float>(size.m_width), static_cast<float>(size.m_height));
                        m_dagProjScale = yScale * static_cast<float>(size.m_height) * 0.5f;
                        m_dagBindValid = true;
                    }
                }
            }
            // Streaming: classify pages, load/evict within the slot budget.
            m_streamingLoadsThisFrame = 0;
            m_streamingEvictsThisFrame = 0;
            if (r_meshletsStreaming && m_dagBindValid && m_transformServiceFeatureProcessor)
            {
                // Live budget change: rebuild the pool; meshes go coarse, then reload.
                if (m_pageResidencyInitialized &&
                    m_lastStreamingPoolMB != static_cast<uint32_t>(r_meshletsStreamingPoolMB))
                {
                    m_pageResidencyInitialized = false;
                    m_pagePoolBuffer = nullptr;
                    for (auto& [key, meshAndPage] : m_pageKeyLookup)
                    {
                        m_pagedMapDirty.insert(meshAndPage.first);
                    }
                    AZ_TracePrintf("Meshlets",
                        "Streaming: pool budget changed (%u MB -> %u MB) -- pool rebuilt, "
                        "pages will reload against the new slot count.\n",
                        m_lastStreamingPoolMB, static_cast<uint32_t>(r_meshletsStreamingPoolMB));
                }
                if (!m_pageResidencyInitialized)
                {
                    m_lastStreamingPoolMB = static_cast<uint32_t>(r_meshletsStreamingPoolMB);
                    // Slot count from the real fixed slot size (PageSlotU32s words).
                    const uint64_t poolBytes = static_cast<uint64_t>(r_meshletsStreamingPoolMB) * 1024u * 1024u;
                    const uint32_t slots = AZStd::GetMax(
                        1u, static_cast<uint32_t>(poolBytes / (PageSlotU32s * sizeof(AZ::u32))));
                    m_pageResidency.Init(slots);
                    m_pagePoolSlotCount = slots;
                    // GPU-only pool (never CPU-written -- the upload compute fills slots),
                    // so the ReadWrite pool's broken CPU-upload path is never exercised.
                    SrgBufferDescriptor poolDesc(
                        RPI::CommonBufferPoolType::ReadWrite, RHI::Format::Unknown,
                        RHI::BufferBindFlags::ShaderReadWrite,
                        static_cast<uint32_t>(sizeof(AZ::u32)), slots * PageSlotU32s,
                        Name{ "MeshletsPagePool" }, Name{ "m_pagePool" }, 0, 0, nullptr);
                    m_pagePoolBuffer = UtilityClass::CreateBuffer("Meshlets", poolDesc, nullptr);
                    AZ_Warning("Meshlets", m_pagePoolBuffer != nullptr,
                        "Streaming: failed to create the %u-slot page pool; paged rendering unavailable.", slots);
                    m_pageResidencyInitialized = true;
                }
                // Upload shader: lazy retry until the asset is processed.
                if (!m_pageUploadShader)
                {
                    Data::Asset<RPI::ShaderAsset> uploadAsset =
                        RPI::AssetUtils::LoadAssetByProductPath<RPI::ShaderAsset>(
                            "Shaders/MeshletsPageUpload.azshader", RPI::AssetUtils::TraceLevel::Warning);
                    if (uploadAsset.GetId().IsValid())
                    {
                        m_pageUploadShader = RPI::Shader::FindOrCreate(uploadAsset);
                    }
                }

                m_pageRequestScratch.clear();
                m_pageKeyLookup.clear();
                AZ::Vector3 cullCameraPos = AZ::Vector3::CreateZero();
                if (RPI::ViewPtr v = m_renderPipeline ? m_renderPipeline->GetDefaultView() : nullptr)
                {
                    cullCameraPos = v->GetViewToWorldMatrix().GetTranslation();
                }
                for (auto& instance : m_instances)
                {
                    if (!instance || !instance->RenderObject)
                    {
                        continue;
                    }
                    ModelLodDataArray& lodArr = instance->RenderObject->GetMeshletsRenderData(0);
                    if (instance->MeshIndex >= lodArr.size() || !lodArr[instance->MeshIndex])
                    {
                        continue;
                    }
                    MeshRenderData& mrd = *lodArr[instance->MeshIndex];
                    if (mrd.PersistentPageTable.empty())
                    {
                        continue;
                    }
                    const AZ::Transform xform =
                        m_transformServiceFeatureProcessor->GetTransformForId(instance->ObjectId);
                    const float maxScale = xform.GetUniformScale();
                    for (size_t pg = 0; pg < mrd.PersistentPageTable.size(); ++pg)
                    {
                        const PageTableRecord& rec = mrd.PersistentPageTable[pg];
                        MeshletsPageResidency::PageRequest req;
                        // Page identity is per-MESH (object-space data shared by every
                        // instance); duplicate submissions from other instances of the
                        // same mesh are handled by the residency core.
                        req.m_key = MeshletsPageKey(&mrd, static_cast<uint32_t>(pg));
                        m_pageKeyLookup.emplace(req.m_key,
                            AZStd::make_pair(&mrd, static_cast<uint32_t>(pg)));
                        req.m_worldAabb = AZ::Aabb::CreateFromMinMax(
                            AZ::Vector3(rec.m_aabbMin[0], rec.m_aabbMin[1], rec.m_aabbMin[2]),
                            AZ::Vector3(rec.m_aabbMax[0], rec.m_aabbMax[1], rec.m_aabbMax[2]))
                            .GetTransformedAabb(xform);
                        req.m_maxParentErrorWorld =
                            (rec.m_maxParentError >= std::numeric_limits<float>::max())
                                ? rec.m_maxParentError
                                : rec.m_maxParentError * maxScale;
                        m_pageRequestScratch.push_back(req);
                    }
                }
                m_streamingTrackedPages = static_cast<uint32_t>(m_pageRequestScratch.size());

                MeshletsPageResidency::CameraState cam;
                cam.m_position = cullCameraPos;
                cam.m_projScale = m_dagProjScale;
                cam.m_tauPx = static_cast<float>(r_meshletsDagErrorPx);
                cam.m_prefetchScale = AZStd::GetMax(1.0f, static_cast<float>(r_meshletsStreamingHysteresis));
                const MeshletsPageResidency::UpdateResult ops = m_pageResidency.Update(
                    m_pageRequestScratch, cam, r_meshletsStreamingMaxLoadsPerFrame);

                // Rotate the in-flight holders: entries 3 frames old are safe to drop.
                m_pageUploadHoldIndex = (m_pageUploadHoldIndex + 1) % static_cast<uint32_t>(m_pageUploadHold.size());
                PageUploadHold& hold = m_pageUploadHold[m_pageUploadHoldIndex];
                hold.m_staging.clear();
                hold.m_srgs.clear();
                hold.m_dispatches.clear();
                m_pageUploadItemsScratch.clear();
                m_pageUploadAttachmentsScratch.clear();

                // Loads: the slot copy runs on the cull compute pass, BEFORE
                // DepthPrePass -- marking resident this frame is therefore coherent.
                for (MeshletsPageResidency::PageKey key : ops.m_load)
                {
                    auto lookupIt = m_pageKeyLookup.find(key);
                    if (lookupIt == m_pageKeyLookup.end() || !m_pagePoolBuffer || !m_pageUploadShader)
                    {
                        // Phase 4 soak fix: a skipped load MUST release its reserved
                        // slot or the pool drains one slot per skip.
                        m_pageResidency.CancelLoad(key);
                        continue;
                    }
                    MeshRenderData* mrd = lookupIt->second.first;
                    const PageTableRecord& rec = mrd->PersistentPageTable[lookupIt->second.second];
                    const uint32_t payloadU32s = rec.m_dataSize / static_cast<uint32_t>(sizeof(AZ::u32));
                    if (PageSlotHeaderU32s + payloadU32s > PageSlotU32s ||
                        rec.m_dataOffset + rec.m_dataSize > mrd->PageData.size())
                    {
                        AZ_Warning("Meshlets", false,
                            "Streaming: page payload (%u words) exceeds the pool slot or the "
                            "PageData section -- page skipped (its clusters fall back coarse).",
                            payloadU32s);
                        m_pageResidency.CancelLoad(key);
                        m_pagedMapDirty.insert(mrd);
                        continue;
                    }

                    // Staging: the PROVEN ReadOnly initial-data upload path.
                    SrgBufferDescriptor stagingDesc(
                        RPI::CommonBufferPoolType::ReadOnly, RHI::Format::Unknown,
                        RHI::BufferBindFlags::ShaderRead,
                        static_cast<uint32_t>(sizeof(AZ::u32)), payloadU32s,
                        Name{ "MeshletsPageStaging" }, Name{ "m_src" }, 0, 0,
                        const_cast<uint8_t*>(mrd->PageData.data() + rec.m_dataOffset));
                    Data::Instance<RPI::Buffer> staging = UtilityClass::CreateBuffer("Meshlets", stagingDesc, nullptr);

                    Data::Instance<RPI::ShaderResourceGroup> uploadSrg =
                        RPI::ShaderResourceGroup::Create(
                            m_pageUploadShader->GetAsset(), AZ::Name{ "MeshletsPageUploadSrg" });
                    if (!staging || !uploadSrg)
                    {
                        AZ_Warning("Meshlets", false, "Streaming: page staging/SRG creation failed -- skipped.");
                        m_pageResidency.CancelLoad(key);
                        m_pagedMapDirty.insert(mrd);
                        continue;
                    }
                    const uint32_t slot = m_pageResidency.OnLoaded(key);
                    if (slot == MeshletsPageResidency::InvalidSlot)
                    {
                        continue;   // raced a Clear -- nothing reserved any more
                    }
                    {
                        SrgBufferDescriptor srcBind;
                        srcBind.m_paramNameInSrg = Name{ "m_src" };
                        UtilityClass::BindBufferToSrg("Meshlets", staging, srcBind, uploadSrg);
                    }
                    uploadSrg->SetConstant(
                        uploadSrg->FindShaderInputConstantIndex(Name{ "m_dstSlotBase" }), slot * PageSlotU32s);
                    uploadSrg->SetConstant(
                        uploadSrg->FindShaderInputConstantIndex(Name{ "m_payloadU32s" }), payloadU32s);
                    const AZ::u32 header[4] = {
                        rec.m_clusterCount, rec.m_vertexCount, rec.m_triangleWords, rec.m_indirCount };
                    uploadSrg->SetConstantRaw(
                        uploadSrg->FindShaderInputConstantIndex(Name{ "m_header" }),
                        header, static_cast<uint32_t>(sizeof(header)));
                    // NOT compiled here: the compute pass binds m_dstPool to the frame
                    // graph's scope-backed view and compiles inside CompileResources
                    // (the finalize mechanism the cull outputs already use).

                    auto dispatch = AZStd::make_unique<MeshletsDispatchItem>();
                    dispatch->InitDispatch(
                        m_pageUploadShader.get(), uploadSrg, PageSlotHeaderU32s + payloadU32s);
                    if (RHI::DispatchItem* di = dispatch->GetDispatchItem())
                    {
                        m_pageUploadItemsScratch.push_back(di);
                    }

                    MeshletsImportedAttachment poolAtt;
                    poolAtt.m_attachmentId = m_pagePoolAttachmentId;
                    poolAtt.m_rhiBuffer = m_pagePoolBuffer->GetRHIBuffer();
                    poolAtt.m_viewDescriptor = RHI::BufferViewDescriptor::CreateStructured(
                        0, m_pagePoolSlotCount * PageSlotU32s, static_cast<uint32_t>(sizeof(AZ::u32)));
                    poolAtt.m_finalizeSrg = uploadSrg;
                    poolAtt.m_finalizeInputName = Name{ "m_dstPool" };
                    m_pageUploadAttachmentsScratch.push_back(poolAtt);

                    hold.m_staging.push_back(AZStd::move(staging));
                    hold.m_srgs.push_back(AZStd::move(uploadSrg));
                    hold.m_dispatches.push_back(AZStd::move(dispatch));
                    m_pagedMapDirty.insert(mrd);
                }
                for (MeshletsPageResidency::PageKey key : ops.m_evicted)
                {
                    auto lookupIt = m_pageKeyLookup.find(key);
                    if (lookupIt != m_pageKeyLookup.end())
                    {
                        m_pagedMapDirty.insert(lookupIt->second.first);
                    }
                }

                // ---- Rebuild dirty meshes' paged cluster maps + SRG state.
                for (MeshRenderData* mrd : m_pagedMapDirty)
                {
                    RebuildPagedClusterMap(*mrd);
                }
                m_pagedMapDirty.clear();

                m_streamingLoadsThisFrame = static_cast<uint32_t>(ops.m_load.size());
                m_streamingEvictsThisFrame = static_cast<uint32_t>(ops.m_evicted.size());
                m_streamingStarvedPages = ops.m_starved;
                m_streamingResidentPages = m_pageResidency.GetResidentCount();
            }
            else if (m_pageResidencyInitialized && !r_meshletsStreaming)
            {
                m_pageResidency.Clear();
                m_pageResidencyInitialized = false;
                m_streamingResidentPages = 0;
                m_streamingTrackedPages = 0;
                m_pageUploadItemsScratch.clear();
                m_pageUploadAttachmentsScratch.clear();
                // Flip every mesh back to monolithic fetch -- a stale m_pagedMode=1
                // against a cleared residency set would read garbage slots.
                for (auto& instance : m_instances)
                {
                    if (!instance || !instance->RenderObject)
                    {
                        continue;
                    }
                    const uint32_t lodCountForObj = instance->RenderObject->GetLodCount();
                    for (uint32_t l = 0; l < lodCountForObj; ++l)
                    {
                        for (MeshRenderData* mrd : instance->RenderObject->GetMeshletsRenderData(l))
                        {
                            if (mrd && mrd->PagedModeActive)
                            {
                                mrd->PagedModeActive = false;
                                if (mrd->MeshShaderObjectSrg &&
                                    mrd->MeshShaderObjectSrg->FindShaderInputConstantIndex(
                                        Name{ "m_pagedMode" }).IsValid())
                                {
                                    mrd->MeshShaderObjectSrg->SetConstant(
                                        mrd->MeshShaderObjectSrg->FindShaderInputConstantIndex(
                                            Name{ "m_pagedMode" }), 0u);
                                    if (!mrd->MeshShaderObjectSrg->IsQueuedForCompile())
                                    {
                                        mrd->MeshShaderObjectSrg->Compile();
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Ledger frame id: never 0 (a fresh ledger's value) even across wrap.
            ++m_frameId;
            if (m_frameId == 0)
            {
                m_frameId = 1;
            }

            // DAG/two-pass toggles bake into cached packets -- force a rebuild.
            if (r_meshletsDagLod != m_lastDagLod || r_meshletsTwoPassOcclusion != m_lastTwoPass)
            {
                m_lastDagLod = r_meshletsDagLod;
                m_lastTwoPass = r_meshletsTwoPassOcclusion;
                for (auto& instance : m_instances)
                {
                    if (!instance || !instance->RenderObject)
                    {
                        continue;
                    }
                    instance->DrawPacket = nullptr;
                    instance->ShadowDrawPacket = nullptr;
                    instance->DepthDrawPacket = nullptr;
                    instance->LateDepthDrawPacket = nullptr;
                    instance->GpuCullResourcesReady = false;
                    instance->CullResourcesReady = false;
                    const uint32_t lodCountForObj = instance->RenderObject->GetLodCount();
                    for (uint32_t l = 0; l < lodCountForObj; ++l)
                    {
                        for (MeshRenderData* mrd : instance->RenderObject->GetMeshletsRenderData(l))
                        {
                            if (mrd)
                            {
                                mrd->MeshShaderResourcesReady = false;
                            }
                        }
                    }
                }
                for (auto& [key, group] : m_instanceGroups)
                {
                    group.m_dirty = true;
                }
            }

            // ===================================================================
            // Per-frame screen-coverage LOD selection.
            //   For each instance, estimate its projected screen coverage from the
            //   camera and the mesh's object-space bounding sphere, map that to a
            //   LOD index, and (with hysteresis) migrate the instance to that LOD's
            //   instance group. Distant instances pick coarser LODs (fewer verts),
            //   which is the whole point: 66 statues no longer all render LOD0.
            //
            //   Gated by r_meshletsLodSelection (default ON). When OFF, instances
            //   stay at LOD0 (the pre-LOD behaviour) for A/B comparison.
            //
            //   This runs BEFORE the per-instance retry, the cull block, and the
            //   submit block so a LOD change (which dirties the affected groups)
            //   is reflected this same frame.
            // ===================================================================
            // Run when screen-coverage selection is on OR a debug force-LOD is set
            // (force overrides the CVAR so you can pin a LOD even with selection off).
            if ((r_meshletsLodSelection || m_debugControls.m_forceLodIndex >= 0) &&
                m_transformServiceFeatureProcessor && m_renderPipeline)
            {
                RPI::ViewPtr view = m_renderPipeline->GetDefaultView();
                if (view)
                {
                    // Camera world position and the vertical projection scale.
                    // ROOT-CAUSE FIX ("lowest LOD always visible"): yScale MUST come from
                    // the PURE PROJECTION (ViewToClip), NOT the combined WorldToClip. The
                    // combined matrix folds in the camera's rotation, so element(1,1)
                    // collapses toward 0 as the camera pitches/yaws -- making coverage ~0
                    // and dumping every instance to LOD3 whenever you look around. The
                    // ViewToClip (1,1) is cot(FovY/2), rotation-independent. Fetched once.
                    const AZ::Vector3 camPos = view->GetViewToWorldMatrix().GetTranslation();
                    const float yScale = view->GetViewToClipMatrix().GetElement(1, 1);

                    constexpr float kEpsilon = 1e-3f;

                    for (auto& instancePtr : m_instances)
                    {
                        MeshletsRenderInstance* instance = instancePtr.get();
                        if (!instance || !instance->RenderObject)
                        {
                            continue;
                        }

                        // Available LOD count for THIS instance's render object. The
                        // outer m_modelRenderData array length is the LOD count; clamp
                        // the chosen LOD into [0, availableLods-1] so a coarse pick on
                        // a 1-LOD (stale) pack just resolves to LOD0.
                        MeshletsRenderObject* ro = instance->RenderObject;
                        // LOD0 mesh render data for bounds (bounds are LOD-invariant --
                        // a coarser LOD shares LOD0's extent). Guard the mesh slot.
                        ModelLodDataArray& lod0Array = ro->GetMeshletsRenderData(0);
                        if (instance->MeshIndex >= lod0Array.size() || !lod0Array[instance->MeshIndex])
                        {
                            continue;
                        }
                        MeshRenderData& lod0Mrd = *lod0Array[instance->MeshIndex];

                        // Number of LODs actually available for THIS mesh: scan the
                        // object's LOD slots upward (bounded by GetLodCount) until one
                        // lacks a non-null entry for this meshIndex. A mesh's LODs are
                        // contiguous from 0, so the first gap is the count. A stale
                        // 1-LOD pack yields GetLodCount()==1 => availableLods==1.
                        uint32_t availableLods = 1;
                        {
                            const uint32_t lodCountForObj = ro->GetLodCount();
                            for (uint32_t l = 1; l < lodCountForObj; ++l)
                            {
                                ModelLodDataArray& slot = ro->GetMeshletsRenderData(l);
                                if (instance->MeshIndex < slot.size() && slot[instance->MeshIndex])
                                {
                                    availableLods = l + 1;
                                }
                                else
                                {
                                    break;
                                }
                            }
                        }

                        // DEBUG force-LOD override: pin every instance to the requested
                        // LOD (clamped to what this pack has), bypassing coverage +
                        // hysteresis. Applied immediately so the A/B is instant.
                        if (m_debugControls.m_forceLodIndex >= 0)
                        {
                            uint32_t forced = static_cast<uint32_t>(m_debugControls.m_forceLodIndex);
                            if (forced > availableLods - 1) { forced = availableLods - 1; }
                            if (forced != instance->LodIndex)
                            {
                                RemoveInstanceFromGroup(instance);
                                instance->LodIndex = forced;
                                instance->LodPendingIndex = forced;
                                instance->LodPendingFrames = 0;
                                instance->DrawPacket = nullptr;
                                instance->ShadowDrawPacket = nullptr;
                                instance->DepthDrawPacket = nullptr;
                                instance->LateDepthDrawPacket = nullptr;
                                instance->CullResourcesReady = false;
                                AddInstanceToGroup(instance);
                            }
                            continue;   // skip coverage selection for this instance.
                        }

                            // DAG packs: the DAG is the LOD system -- pin LOD0.
                        if (r_meshletsDagLod && lod0Mrd.DagClusterCount > 0)
                        {
                            if (instance->LodIndex != 0)
                            {
                                RemoveInstanceFromGroup(instance);
                                instance->LodIndex = 0;
                                instance->LodPendingIndex = 0;
                                instance->LodPendingFrames = 0;
                                instance->DrawPacket = nullptr;
                                instance->ShadowDrawPacket = nullptr;
                                instance->DepthDrawPacket = nullptr;
                                instance->LateDepthDrawPacket = nullptr;
                                instance->CullResourcesReady = false;
                                AddInstanceToGroup(instance);
                            }
                            continue;
                        }

                        // Object-space bounds (shared across instances, LOD-invariant).
                        MeshletsRenderObject::EnsureMeshBounds(lod0Mrd);
                        if (lod0Mrd.MeshBoundsRadius < 0.0f)
                        {
                            continue;   // bounds unavailable -- leave LOD as-is.
                        }

                        const AZ::Transform xform =
                            m_transformServiceFeatureProcessor->GetTransformForId(instance->ObjectId);
                        const AZ::Matrix4x4 objectToWorld = AZ::Matrix4x4::CreateFromTransform(xform);
                        const AZ::Vector3 worldCenter =
                            (objectToWorld * AZ::Vector4::CreateFromVector3AndFloat(lod0Mrd.MeshBoundsCenter, 1.0f))
                                .GetAsVector3();
                        const float sx = objectToWorld.GetColumnAsVector3(0).GetLength();
                        const float sy = objectToWorld.GetColumnAsVector3(1).GetLength();
                        const float sz = objectToWorld.GetColumnAsVector3(2).GetLength();
                        const float worldRadius =
                            lod0Mrd.MeshBoundsRadius * AZStd::GetMax(sx, AZStd::GetMax(sy, sz));

                        const float dist = camPos.GetDistance(worldCenter);
                        // Approx projected radius as a fraction of the viewport half-
                        // height: coverage = yScale * worldRadius / distance. ~1.0 means
                        // the sphere fills the vertical view; small means tiny on screen.
                        const float approxCoverage = yScale * worldRadius / AZStd::GetMax(kEpsilon, dist);

                        uint32_t wantLod;
                        // Opt-in. The BuilderVersion bump makes the Asset Processor re-bake every
                        // existing .azmeshletpack with a LodError section, so gating on HasLodError
                        // alone would silently switch ALL existing meshlet content onto a different
                        // LOD-selection metric with no way to compare against the old behaviour --
                        // and none of this has been verified against a rendered frame yet. Default
                        // off keeps the screen-coverage bands; flip r_meshletsGeometricLod to
                        // evaluate the better metric side by side.
                        if (r_meshletsGeometricLod && lod0Mrd.HasLodError)
                        {
                            // Geometric-error metric: aabb_pixel_size * lod_error < acceptable_pixel_error.
                            // lod_error (SectionKind::LodError) is meshopt_simplify's own scale-independent
                            // "relative to mesh extents" error for that LOD (0 for LOD0/baked LODs).
                            // aabb_pixel_size reprojects the mesh's on-screen size into pixels at a fixed
                            // reference viewport height, so the pixel-error budget has a stable meaning
                            // independent of the actual render resolution.
                            // ponytail: kVirtualViewportHeight is a calibration constant, not the live
                            // viewport size (not plumbed to this feature processor) -- if the metric reads
                            // too aggressive/conservative at a given resolution, wire the real viewport
                            // height (RenderPipeline::GetRenderSettings().m_size) through here instead.
                            constexpr float kVirtualViewportHeight = 1080.0f;
                            constexpr float kAcceptablePixelError  = 1.0f;   // ~1px budget, Nanite-style default
                            constexpr float kLodDeadband           = 0.75f; // sticky margin, mirrors the old coverage deadband
                            const float aabbPixelSize = approxCoverage * kVirtualViewportHeight;

                            // Pick the COARSEST LOD whose reprojected error still fits the pixel
                            // budget (falls through to LOD0 if nothing coarser qualifies).
                            wantLod = 0;
                            for (uint32_t b = availableLods - 1; b >= 1; --b)
                            {
                                MeshRenderData* bMrd = ro->GetMeshletsRenderData(b)[instance->MeshIndex];
                                if (!bMrd) { continue; }   // shouldn't happen (b < availableLods); stay conservative
                                // Sticky: once already at LOD b or coarser, keep a lenient budget so the
                                // instance doesn't flicker back to a finer LOD; otherwise require clearing
                                // the stricter (deadband-scaled) budget to demote INTO b.
                                const float budget = (instance->LodIndex >= b)
                                    ? kAcceptablePixelError
                                    : kAcceptablePixelError * kLodDeadband;
                                if (aabbPixelSize * bMrd->LodGeometricError <= budget)
                                {
                                    wantLod = b;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            // Fallback for packs built before builder v9 (no LodError section):
                            // unchanged screen-coverage bands. Map coverage -> LOD with a
                            // DETAIL-BIASED deadband. Recalibrated so VISIBLE objects stay HIGH
                            // detail (Nanite intent): a statue filling ~18% of the vertical view
                            // is LOD0 (the old 0.5 gate needed it to fill HALF the screen, which
                            // is why on-screen statues looked coarse). The deadband -- promote to
                            // a finer LOD at the nominal boundary, demote to a coarser LOD only
                            // when coverage drops well past it -- keeps the silhouette crisp AND
                            // replaces the frame-counter hysteresis that let boundary instances
                            // thrash group membership every frame.
                            constexpr float kLodCov[3]   = { 0.18f, 0.08f, 0.03f };  // LOD0|1, 1|2, 2|3
                            constexpr float kLodDeadband = 0.75f;                    // demote below boundary*this
                            wantLod = 3;
                            for (uint32_t b = 0; b < 3; ++b)
                            {
                                // At-or-finer than band b => sticky (stay fine until coverage drops
                                // past the deadband); coarser than b => promote at the nominal bar.
                                const float th = (instance->LodIndex <= b) ? kLodCov[b] * kLodDeadband : kLodCov[b];
                                if (approxCoverage > th) { wantLod = b; break; }
                            }
                        }
                        // Clamp to what this object actually has (stale 1-LOD packs -> 0).
                        if (wantLod > availableLods - 1)
                        {
                            wantLod = availableLods - 1;
                        }
                        if (wantLod == instance->LodIndex)
                        {
                            continue;   // already correct -- no group churn.
                        }
                        // Apply immediately; the deadband above (not a frame counter) is what
                        // prevents boundary flicker.

                        // Apply the LOD change: migrate the instance to the new LOD's
                        // instance group (group key = {RenderObject, LodIndex, MeshIndex}).
                        // Remove-then-re-add dirties both the old and new groups so the
                        // submit block rebuilds their instanced packets with the correct
                        // membership. The per-instance camera packet is also dropped so
                        // the cull-ON path rebuilds it against the new LOD's geometry.
                        RemoveInstanceFromGroup(instance);
                        instance->LodIndex = wantLod;
                        instance->LodPendingFrames = 0;
                        instance->DrawPacket = nullptr;
                        instance->ShadowDrawPacket = nullptr;
                        instance->DepthDrawPacket = nullptr;
                        instance->LateDepthDrawPacket = nullptr;
                        instance->CullResourcesReady = false;
                        AddInstanceToGroup(instance);
                    }
                }
            }

            // Retry pass: any instance whose DrawPacket was deferred (because the
            // render pass's pipeline state wasn't ready when AddInstance ran) gets
            // another chance now. This is the steady-state fix for the "instance
            // registered before pipeline ready" race; InitRenderPass also retries
            // on pipeline change, but a freshly-loaded asset may not trigger that.
            //
            // Step B: the per-instance packets feed ONLY the cull-ON (opt-in) path now.
            // The cull-OFF default path draws via per-group hardware-instanced packets
            // (rebuilt below in the submit block), so this per-instance retry runs only
            // when cull is enabled -- avoids building unused per-instance packets every
            // frame in the default case.
            if (m_renderPass && m_debugControls.m_cullEnabled)
            {
                for (auto& instance : m_instances)
                {
                    if (!instance || instance->DrawPacket || !instance->RenderObject)
                    {
                        continue;
                    }
                    ModelLodDataArray& lodArray = instance->RenderObject->GetMeshletsRenderData(instance->LodIndex);
                    if (instance->MeshIndex < lodArray.size() && lodArray[instance->MeshIndex])
                    {
                        BuildInstanceDrawPacket(*instance, *lodArray[instance->MeshIndex]);
                    }
                }
            }

            // Phase 1: dispatch one compute group per active render object (NOT per instance).
            // The compute writes the per-object index buffer in the shared buffer; every
            // instance of that object reads from it, so doing the work once is correct.
            //
            // SP1: alongside the dispatch items, build the per-frame list of
            // imported attachments -- one per RW compute output (m_indices and
            // m_uvs) per object. Both passes need the same list so the frame
            // graph can wire UAV (compute) -> SRV (render) barriers between
            // the dedicated per-object buffers.
            AZStd::vector<MeshletsImportedAttachment> importedAttachments;
            importedAttachments.reserve(m_meshletsRenderObjects.size() * 2);

            for (MeshletsRenderObject* renderObject : m_meshletsRenderObjects)
            {
                // Iterate EVERY LOD slot, not just LOD0 -- each LOD has its own
                // dedicated per-mesh m_indices/m_uvs buffers, so each needs its own
                // uniquely-identified frame-graph attachment. (A null mesh slot is
                // legitimate now: a mesh with fewer LODs than the object's max
                // leaves nullptr at its meshIdx in the higher LOD slots.)
                const uint32_t lodCount = renderObject->GetLodCount();
                for (uint32_t lod = 0; lod < lodCount; ++lod)
                {
                    ModelLodDataArray& modelLodArray = renderObject->GetMeshletsRenderData(lod);
                    for (size_t meshIdx = 0; meshIdx < modelLodArray.size(); ++meshIdx)
                    {
                        auto& renderData = modelLodArray[meshIdx];
                        if (!renderData)
                        {
                            continue;   // mesh has no data at this LOD (missing level) -- skip.
                        }
                        // SP1: compute is fully suppressed. The m_indices/m_uvs
                        // buffers are populated via the CPU pre-bake at construction
                        // through the ReadOnly-pool initial-data path (see
                        // MeshletsRenderObjectPackInit.cpp). No compute SRG is
                        // built and no dispatch is queued.

                        // Build attachment entries for this mesh's RW outputs. The
                        // attachment ids must be stable per (object, LOD, mesh) across
                        // frames (the frame graph keys state tracking off the id) and
                        // unique across all of them (otherwise two attachments collide).
                        // Object address + REAL lod index + mesh index + stream name
                        // gives both -- crucially the lod must be the real index (not a
                        // hardcoded 0), else LOD0's and LOD1's buffers for the same mesh
                        // would share an attachment id and corrupt state tracking.
                        const uintptr_t objKey = reinterpret_cast<uintptr_t>(renderObject);
                        const auto attachStream = [&](uint8_t streamSemantic, const char* tag)
                        {
                            if (streamSemantic >= renderData->ComputeBuffers.size())
                            {
                                return;
                            }
                            auto& rpiBuffer = renderData->ComputeBuffers[streamSemantic];
                            if (!rpiBuffer)
                            {
                                return;
                            }
                            SrgBufferDescriptor& desc = renderData->ComputeBuffersDescriptors[streamSemantic];

                            MeshletsImportedAttachment att;
                            AZStd::string idStr = AZStd::string::format(
                                "Meshlets_%s_%llx_lod%u_mesh%zu", tag,
                                static_cast<unsigned long long>(objKey),
                                static_cast<unsigned>(lod), meshIdx);
                            att.m_attachmentId = AZ::Name{ idStr };
                            att.m_rhiBuffer = rpiBuffer->GetRHIBuffer();

                            RHI::BufferViewDescriptor viewDesc;
                            viewDesc.m_elementOffset = 0;
                            viewDesc.m_elementCount  = desc.m_elementCount;
                            viewDesc.m_elementSize   = desc.m_elementSize;
                            viewDesc.m_elementFormat = desc.m_elementFormat;
                            viewDesc.m_overrideBindFlags = desc.m_bindFlags;
                            att.m_viewDescriptor = viewDesc;
                            importedAttachments.emplace_back(AZStd::move(att));
                        };

                        attachStream(uint8_t(ComputeStreamsSemantics::Indices), "Idx");
                        attachStream(uint8_t(ComputeStreamsSemantics::UVs), "Uv");
                    }
                }
            }

            // Debug: meshlet (cluster) coloring. Apply the toggle to every object's
            // per-object SRG only when it changes -- it's a flat shader branch keyed
            // off an SRG constant, so no DrawPacket rebuild is needed.
            if (m_debugControls.m_meshletColorMode != m_lastMeshletColorMode ||
                static_cast<bool>(r_meshletsDagDebugColor) != m_lastDagDebugColor)
            {
                // Mesh-shader path: 0 = PBR, 1 = per-cluster color, 2 = DAG-depth color
                // (cluster colors win when both toggles are on).
                const uint32_t msColorMode =
                    m_debugControls.m_meshletColorMode ? 1u : (r_meshletsDagDebugColor ? 2u : 0u);
                for (MeshletsRenderObject* obj : m_meshletsRenderObjects)
                {
                    if (!obj)
                    {
                        continue;
                    }
                    obj->SetMeshletDebugColor(m_debugControls.m_meshletColorMode);
                    const uint32_t lodCount = obj->GetLodCount();
                    for (uint32_t l = 0; l < lodCount; ++l)
                    {
                        for (MeshRenderData* mrd : obj->GetMeshletsRenderData(l))
                        {
                            if (mrd && mrd->MeshShaderObjectSrg)
                            {
                                mrd->MeshShaderObjectSrg->SetConstant(
                                    mrd->MeshShaderObjectSrg->FindShaderInputConstantIndex(
                                        Name("m_meshletDebugColor")), msColorMode);
                                if (!mrd->MeshShaderObjectSrg->IsQueuedForCompile())
                                {
                                    mrd->MeshShaderObjectSrg->Compile();
                                }
                            }
                        }
                    }
                }
                m_lastMeshletColorMode = m_debugControls.m_meshletColorMode;
                m_lastDagDebugColor = r_meshletsDagDebugColor;
            }

            // Phase 6 cluster culling. When enabled, per frame: cull each instance's
            // clusters against the main camera. CPU path rebuilds each packet from the
            // visible command set; GPU path updates a cull SRG + queues a compute
            // dispatch that fills a per-instance indirect-args buffer. On a disable or
            // CPU<->GPU switch, packets are rebuilt so rendering stays correct.
            m_cullDispatchItemsScratch.clear();
            m_cullArgsAttachmentsScratch.clear();
            m_cullBarrierAttachmentsScratch.clear();

            // A cull enable/disable or CPU<->GPU switch changes which geometry view each
            // packet must use -- force a rebuild by clearing the per-instance packets.
            const bool cullModeChanged =
                (m_debugControls.m_cullEnabled != m_cullWasEnabled) ||
                (m_debugControls.m_gpuCull != m_lastGpuCull);
            if (cullModeChanged)
            {
                for (auto& instance : m_instances)
                {
                    if (instance)
                    {
                        instance->DrawPacket = nullptr;
                        instance->CullResourcesReady = false;
                    }
                }
            }
            m_lastGpuCull = m_debugControls.m_gpuCull;

            if (m_debugControls.m_cullEnabled && m_drawIndirectSignature && m_transformServiceFeatureProcessor)
            {
                RPI::ViewPtr cameraView = m_renderPipeline ? m_renderPipeline->GetDefaultView() : nullptr;
                if (cameraView)
                {
                    // Column-major, no ReverseDepth arg -- matches Atom's own view-frustum
                    // construction (e.g. SimplePointLightFeatureProcessor). Using RowMajor /
                    // ReverseDepth here inverted the frustum so every cluster read as Exterior.
                    const AZ::Matrix4x4 liveMat = cameraView->GetWorldToClipMatrix();
                    const AZ::Frustum liveFrustum =
                        AZ::Frustum::CreateFromMatrixColumnMajor(liveMat);
                    const AZ::Vector3 liveCameraPos = cameraView->GetViewToWorldMatrix().GetTranslation();

                    // Freeze-frustum debug: on the off->on edge, snapshot the live camera;
                    // while frozen, cull against the snapshot so you can fly the real camera
                    // around and see exactly which clusters were culled.
                    // Either the ImGui debug tab or r_meshletsFreezeCull (set by tools that
                    // cannot depend on this gem, e.g. WDDebugView) can request the freeze.
                    if (m_debugControls.m_freezeCullCamera || r_meshletsFreezeCull)
                    {
                        if (!m_cullCameraFrozen)
                        {
                            m_frozenFrustum = liveFrustum;
                            m_frozenCameraPos = liveCameraPos;
                            m_frozenCamMatrix = liveMat;
                            m_cullCameraFrozen = true;
                        }
                    }
                    else
                    {
                        m_cullCameraFrozen = false;
                    }
                    const AZ::Frustum& frustum = m_cullCameraFrozen ? m_frozenFrustum : liveFrustum;
                    const AZ::Vector3& cameraPos = m_cullCameraFrozen ? m_frozenCameraPos : liveCameraPos;
                    const AZ::Matrix4x4& effectiveMat = m_cullCameraFrozen ? m_frozenCamMatrix : liveMat;

                    // Optimization: re-cull an instance only when the effective cull camera,
                    // the cull params, or that instance's transform actually changed.
                    const bool cullParamsChanged =
                        (m_debugControls.m_frustumCull != m_lastFrustumCull) ||
                        (m_debugControls.m_coneCull != m_lastConeCull);
                    const bool cameraChanged =
                        !m_haveLastCullCamera || (effectiveMat != m_lastEffectiveCamMatrix) || cullParamsChanged;
                    m_lastEffectiveCamMatrix = effectiveMat;
                    m_lastFrustumCull = m_debugControls.m_frustumCull;
                    m_lastConeCull = m_debugControls.m_coneCull;
                    m_haveLastCullCamera = true;

                    uint64_t totalVisible = 0;
                    uint64_t totalCulled = 0;
                    for (auto& instance : m_instances)
                    {
                        if (!instance || !instance->RenderObject)
                        {
                            continue;
                        }
                        ModelLodDataArray& lod = instance->RenderObject->GetMeshletsRenderData(instance->LodIndex);
                        if (instance->MeshIndex >= lod.size() || !lod[instance->MeshIndex])
                        {
                            continue;
                        }

                        if (m_debugControls.m_gpuCull)
                        {
                            MeshRenderData& mrd = *lod[instance->MeshIndex];
                            const AZ::Transform xform =
                                m_transformServiceFeatureProcessor->GetTransformForId(instance->ObjectId);
                            const AZ::Matrix4x4 objectToWorld = AZ::Matrix4x4::CreateFromTransform(xform);

                            // TWO-LEVEL CULL: a cheap whole-instance frustum test against the
                            // mesh bounding sphere decides the path. This is the key win for
                            // "many objects on screen": fully-visible instances draw whole-mesh
                            // (no per-cluster compaction -- that compute copies the entire index
                            // buffer every frame for zero culling benefit when nothing is off
                            // screen), and fully-off-screen instances are dropped outright. Only
                            // instances straddling the frustum run the per-cluster GPU cull.
                            MeshletsRenderObject::EnsureMeshBounds(mrd);
                            AZ::IntersectResult instTest = AZ::IntersectResult::Overlaps; // bounds invalid => always cull
                            if (mrd.MeshBoundsRadius >= 0.0f)
                            {
                                const AZ::Vector3 centerW =
                                    (objectToWorld * AZ::Vector4::CreateFromVector3AndFloat(mrd.MeshBoundsCenter, 1.0f))
                                        .GetAsVector3();
                                const float sx = objectToWorld.GetColumnAsVector3(0).GetLength();
                                const float sy = objectToWorld.GetColumnAsVector3(1).GetLength();
                                const float sz = objectToWorld.GetColumnAsVector3(2).GetLength();
                                const float radiusW = mrd.MeshBoundsRadius * AZStd::GetMax(sx, AZStd::GetMax(sy, sz));
                                instTest = frustum.IntersectSphere(centerW, radiusW);
                            }

                            if (instTest == AZ::IntersectResult::Exterior)
                            {
                                // Fully off screen -- draw nothing (camera + shadow).
                                instance->DrawPacket = nullptr;
                                instance->ShadowDrawPacket = nullptr;
                                instance->DepthDrawPacket = nullptr;
                                instance->LateDepthDrawPacket = nullptr;
                                instance->GpuCullDrawActive = false;
                                continue;
                            }

                            // Visible -- run the per-cluster (surfel) GPU cull. The compute writes
                            // only visible clusters' DrawIndexedIndirect commands + a count; the
                            // camera packet draws them via DrawIndexedIndirectCount over the STATIC
                            // (vertex-cache-friendly) index buffer, so each visible cluster's slice
                            // reuses shaded vertices. The shadow packet stays whole-mesh. The
                            // instance-level test above just skips fully-off-screen objects cheaply.
                            if (!instance->GpuCullDrawActive)
                            {
                                instance->DrawPacket = nullptr;   // entering culled state -- rebuild packet
                                instance->GpuCullDrawActive = true;
                            }
                            // Skip the recompute when neither the camera nor this object moved -- last
                            // frame's command/count buffers are still valid; just re-declare them on
                            // the barrier pass so their state stays read-ready for the draw.
                            const bool gpuTransformChanged =
                                !instance->HasLastCullTransform || !xform.IsClose(instance->LastCullTransform);
                            // HiZ/DAG active => never skip: the pyramid ping-pongs every
                            // frame (a stale bind = the slot being written) and both
                            // tests are view-dependent.
                            const bool dagMayCut = r_meshletsDagLod && mrd.DagClusterCount > 0;
                            const bool gpuCanSkip = !m_hiZBindValid && !dagMayCut &&
                                !cameraChanged && !gpuTransformChanged &&
                                instance->GpuCullResourcesReady && instance->DrawPacket;
                            if (gpuCanSkip)
                            {
                                AppendGpuCullAttachments(*instance, mrd.IndexCount, m_cullBarrierAttachmentsScratch);
                            }
                            else
                            {
                                instance->LastCullTransform = xform;
                                instance->HasLastCullTransform = true;
                                UpdateGpuCullInstance(*instance, mrd, frustum, cameraPos, objectToWorld, effectiveMat);
                            }
                            totalVisible += instance->GpuCullClusterCount;
                            continue;
                        }

                        // CPU path: skip the cull + packet rebuild when nothing changed
                        // for this instance -- the existing packet still draws the correct set.
                        const AZ::Transform xform =
                            m_transformServiceFeatureProcessor->GetTransformForId(instance->ObjectId);
                        const bool transformChanged =
                            !instance->HasLastCullTransform ||
                            !xform.IsClose(instance->LastCullTransform);
                        if (!cameraChanged && !transformChanged &&
                            instance->CullResourcesReady && instance->DrawPacket)
                        {
                            totalVisible += instance->LastVisibleClusters;
                            totalCulled += instance->LastCulledClusters;
                            continue;
                        }
                        instance->LastCullTransform = xform;
                        instance->HasLastCullTransform = true;

                        uint32_t vis = 0, cul = 0;
                        CullInstanceAndRebuildPacket(*instance, *lod[instance->MeshIndex], frustum, cameraPos, vis, cul);
                        instance->LastVisibleClusters = vis;
                        instance->LastCulledClusters = cul;
                        totalVisible += vis;
                        totalCulled += cul;
                    }
                    m_debugControls.m_visibleClusters = totalVisible;
                    m_debugControls.m_culledClusters = totalCulled;
                }
                m_cullWasEnabled = true;
            }
            else if (m_cullWasEnabled)
            {
                // Cull just turned off -- Step B: the cull-OFF path draws via per-group
                // hardware-instanced packets, NOT per-instance packets. Drop the now-stale
                // per-instance cull state and mark every group dirty so the submit block
                // below rebuilds the instanced packets.
                for (auto& instance : m_instances)
                {
                    if (instance)
                    {
                        instance->CullResourcesReady = false;
                        instance->DrawPacket = nullptr;
                        instance->ShadowDrawPacket = nullptr;
                        instance->DepthDrawPacket = nullptr;
                        instance->LateDepthDrawPacket = nullptr;
                    }
                }
                for (auto& [key, group] : m_instanceGroups)
                {
                    group.m_dirty = true;
                }
                m_debugControls.m_visibleClusters = 0;
                m_debugControls.m_culledClusters = 0;
                m_cullWasEnabled = false;
            }

            // Hardware mesh-shader path: a CVar toggle invalidates every packet so the
            // next build routes through the newly-selected path.
            if (r_meshletsHwMeshShader && !m_meshShaderSupportQueried)
            {
                m_meshShaderSupportQueried = true;
                if (RHI::Device* device = RHI::RHISystemInterface::Get()->GetDevice())
                {
                    m_meshShaderSupported = device->GetFeatures().m_meshShader;
                }
                AZ_TracePrintf("Meshlets", "Hardware mesh-shader support: %s\n",
                    m_meshShaderSupported ? "YES" : "no (r_meshletsHwMeshShader will fall back)");
            }
            const bool hwMeshActive = r_meshletsHwMeshShader && m_meshShaderSupported;
            if (r_meshletsHwMeshShader != m_lastHwMeshShader)
            {
                m_lastHwMeshShader = r_meshletsHwMeshShader;
                InvalidateAllDrawPackets();
            }
            // Phase 5 AS/triangle cull: toggling which PSO/geometry-view an instance's
            // packet should use requires a rebuild, same as the hw-mesh-shader toggle.
            if (r_meshletsMsCullAS != m_lastMsCullAS)
            {
                m_lastMsCullAS = r_meshletsMsCullAS;
                InvalidateAllDrawPackets();
            }
            if (hwMeshActive)
            {
                for (auto& instance : m_instances)
                {
                    if (instance && instance->RenderObject && !instance->DrawPacket)
                    {
                        ModelLodDataArray& lodArray =
                            instance->RenderObject->GetMeshletsRenderData(instance->LodIndex);
                        if (instance->MeshIndex < lodArray.size() && lodArray[instance->MeshIndex])
                        {
                            BuildInstanceDrawPacket(*instance, *lodArray[instance->MeshIndex]);
                        }
                    }
                }

                // Phase 5 AS/triangle cull (opt-in, per frame): the DrawPacket above is
                // built once and reused (DispatchMesh args + SRG pointers don't need a
                // per-frame rebuild), but the AS's frustum/camera/transform constants DO
                // need refreshing every frame so culling tracks the live camera. Cheap
                // constant-only SRG update, no packet rebuild.
                if (r_meshletsMsCullAS && m_transformServiceFeatureProcessor && m_renderPipeline)
                {
                    if (RPI::ViewPtr cullView = m_renderPipeline->GetDefaultView())
                    {
                        const AZ::Matrix4x4 liveCullMat = cullView->GetWorldToClipMatrix();
                        const AZ::Frustum liveCullFrustum = AZ::Frustum::CreateFromMatrixColumnMajor(liveCullMat);
                        const AZ::Vector3 liveCullCameraPos = cullView->GetViewToWorldMatrix().GetTranslation();

                        // Freeze-cull debug (same snapshot state the CPU/compute cull block
                        // maintains -- shared members, same edge detection, so enabling both
                        // paths freezes them to the SAME camera). While frozen, the AS keeps
                        // culling against the snapshot so you can fly the real camera around
                        // and see exactly which clusters were culled.
                        if (m_debugControls.m_freezeCullCamera || r_meshletsFreezeCull)
                        {
                            if (!m_cullCameraFrozen)
                            {
                                m_frozenFrustum = liveCullFrustum;
                                m_frozenCameraPos = liveCullCameraPos;
                                m_frozenCamMatrix = liveCullMat;
                                m_cullCameraFrozen = true;
                            }
                        }
                        else
                        {
                            m_cullCameraFrozen = false;
                        }
                        const AZ::Frustum& cullFrustum = m_cullCameraFrozen ? m_frozenFrustum : liveCullFrustum;
                        const AZ::Vector3& cullCameraPos = m_cullCameraFrozen ? m_frozenCameraPos : liveCullCameraPos;
                        for (auto& instance : m_instances)
                        {
                            if (!instance || !instance->MeshShaderInstanceSrg)
                            {
                                continue;
                            }
                            const AZ::Transform xform =
                                m_transformServiceFeatureProcessor->GetTransformForId(instance->ObjectId);
                            UpdateMeshShaderCullInstance(
                                *instance, cullFrustum, cullCameraPos, AZ::Matrix4x4::CreateFromTransform(xform));
                        }
                    }
                }
            }

            // Submission: cull OFF = one hardware-instanced packet per group;
            // cull or mesh-shader ON = per-instance packets, no grouping.
            if (!m_debugControls.m_cullEnabled && !hwMeshActive)
            {
                size_t cameraPackets = 0;
                size_t shadowPackets = 0;
                for (auto& [key, group] : m_instanceGroups)
                {
                    if (group.m_members.empty() || !key.m_renderObject)
                    {
                        continue;
                    }
                    ModelLodDataArray& lod = key.m_renderObject->GetMeshletsRenderData(key.m_lodIndex);
                    if (key.m_meshIndex >= lod.size() || !lod[key.m_meshIndex])
                    {
                        continue;
                    }
                    MeshRenderData& mrd = *lod[key.m_meshIndex];

                    // Rebuild only if dirty (membership changed / first build / invalidated).
                    // Transform changes do NOT dirty the group -- SceneSrg supplies the live
                    // per-objectId transforms each frame, indexed by SV_InstanceID.
                    if (group.m_dirty)
                    {
                        RebuildInstanceGroup(group, key, mrd);
                    }

                    if (group.m_cameraPacket)
                    {
                        m_drawPacketsScratch.emplace_back(group.m_cameraPacket.get());
                        ++cameraPackets;
                    }
                    if (group.m_shadowPacket)
                    {
                        m_drawPacketsScratch.emplace_back(group.m_shadowPacket.get());
                        ++shadowPackets;
                    }
                }

                // Instrumentation: confirm N instances -> G groups collapse (log on change).
                const size_t instanceCount = m_instances.size();
                const size_t groupCount = m_instanceGroups.size();
                if (instanceCount != m_lastStepBInstances || groupCount != m_lastStepBGroups ||
                    cameraPackets != m_lastStepBCameraPackets || shadowPackets != m_lastStepBShadowPackets)
                {
                    AZ_TracePrintf("Meshlets",
                        "Meshlets Step B: instances=%zu groups=%zu cameraPackets=%zu shadowPackets=%zu\n",
                        instanceCount, groupCount, cameraPackets, shadowPackets);
                    m_lastStepBInstances = instanceCount;
                    m_lastStepBGroups = groupCount;
                    m_lastStepBCameraPackets = cameraPackets;
                    m_lastStepBShadowPackets = shadowPackets;
                }
            }
            else
            {
                // CULL ON (opt-in): per-instance path, UNCHANGED. Submit each instance's
                // camera packet + whole-mesh shadow packet. (The cull block above already
                // nulled off-screen instances' packets, so no extra frustum test here.)
                for (auto& instance : m_instances)
                {
                    if (instance && instance->DrawPacket)
                    {
                        m_drawPacketsScratch.emplace_back(instance->DrawPacket.get());
                    }
                    // Phase 6 occlusion-safe depth: the AS-cull path's depth item rides
                    // its own packet (own SRG, HiZ never applied outside two-pass).
                    if (instance && instance->DepthDrawPacket)
                    {
                        m_drawPacketsScratch.emplace_back(instance->DepthDrawPacket.get());
                    }
                    if (instance && instance->ShadowDrawPacket)
                    {
                        m_drawPacketsScratch.emplace_back(instance->ShadowDrawPacket.get());
                    }
                    // Pass 2: late-depth packet + the ledger import (RW on barrier
                    // AND late pass -- the two sync points).
                    if (instance && r_meshletsTwoPassOcclusion && instance->LateDepthDrawPacket &&
                        instance->VisFrameBuffer && instance->VisFrameBuffer->GetRHIBuffer())
                    {
                        m_drawPacketsScratch.emplace_back(instance->LateDepthDrawPacket.get());

                        MeshletsImportedAttachment visAtt;
                        visAtt.m_attachmentId = instance->VisFrameAttachmentId;
                        visAtt.m_rhiBuffer = instance->VisFrameBuffer->GetRHIBuffer();
                        visAtt.m_viewDescriptor = RHI::BufferViewDescriptor::CreateStructured(
                            0,
                            static_cast<uint32_t>(
                                instance->VisFrameBuffer->GetBufferSize() / sizeof(AZ::u32)),
                            static_cast<uint32_t>(sizeof(AZ::u32)));
                        visAtt.m_renderPassReadWrite = true;
                        m_visFrameAttachmentsScratch.push_back(visAtt);
                    }
                }
            }

            // SP1 diagnostic: trace the per-frame queue counts when they change.
            // Combined with the matching transitions inside MultiDispatchComputePass
            // and MeshletsRenderPass, this tells us whether silent drops are happening
            // BEFORE the pass classes (here in the feature processor) or AFTER
            // (inside the pass class methods).
            const int32_t curDispatches = static_cast<int32_t>(m_dispatchItemsScratch.size());
            const int32_t curDrawPackets = static_cast<int32_t>(m_drawPacketsScratch.size());
            if (curDispatches != m_lastRenderedDispatchCount ||
                curDrawPackets != m_lastRenderedDrawPacketCount)
            {
                AZ_TracePrintf("Meshlets",
                    "MeshletsFeatureProcessor::Render: instances=%zu renderObjects=%zu | "
                    "queuing %d dispatch(es) and %d drawPacket(s) "
                    "[prev: %d/%d].\n",
                    m_instances.size(), m_meshletsRenderObjects.size(),
                    curDispatches, curDrawPackets,
                    m_lastRenderedDispatchCount, m_lastRenderedDrawPacketCount);
                m_lastRenderedDispatchCount = curDispatches;
                m_lastRenderedDrawPacketCount = curDrawPackets;
            }

            // SP1: imported-attachment plumbing is now disabled. With compute
            // suppressed AND the indices/UVs buffers in the ReadOnly pool
            // (uploaded once at construction via the initial-data path),
            // there is no cross-pass UAV->SRV transition to manage. D3D12's
            // implicit promotion handles SRV reads from a COMMON-state buffer
            // automatically. Re-enable both passes' SetImportedAttachments
            // when compute is brought back in Tier 2 / Epic E.
            //
            // (We still build importedAttachments above so the diagnostic
            // counts in Render's trace line match what the frame graph would
            // see if we re-enabled it; the lists are just not pushed.)
            // m_computePass->SetImportedAttachments(importedAttachments);
            // m_renderPass->SetImportedAttachments(importedAttachments);
            m_computePass->SetImportedAttachments({});
            m_renderPass->SetImportedAttachments({});

            // PERF: the gem-private MeshletsRenderPass only renders the "MeshletsDrawList"
            // debug-fallback item, which instances emit ONLY when the forward PBR shader is
            // unavailable (same global condition as 'useForward' in BuildInstanceDrawPacket).
            // In the normal path it has zero items but a RasterPass still opens a full-res
            // color+depth scope every frame (~one whole geometry pass of cost). Tell it
            // whether there's any debug work so it can skip the scope entirely.
            const bool forwardHealthy = (m_forwardPipelineState && m_forwardDrawListTag.IsValid()) &&
                m_debugControls.m_forwardPassEnabled && !m_debugControls.m_useDebugShader;
            m_renderPass->SetHasDrawWork(!forwardHealthy);

            // The late pass's scope is what legalizes CURRENT-pyramid sampling for
            // later passes -- this gate and twoPassActive must agree (both key off
            // LateDepthDrawPacket).
            if (m_lateDepthPass)
            {
                m_lateDepthPass->SetImportedAttachments(m_visFrameAttachmentsScratch);
                m_lateDepthPass->SetHasDrawWork(!m_visFrameAttachmentsScratch.empty());
            }

            m_computePass->AddDispatchItems(m_dispatchItemsScratch);

            // GPU cull: feed the early compute + barrier passes. The compute pass runs
            // the cull dispatches and declares the args buffers ReadWrite (UAV write);
            // the barrier pass declares the SAME buffers Indirect (DrawIndirect), so the
            // frame graph emits the UAV->IndirectArgument transition before the standard
            // depth/forward/shadow passes consume them. Both passes sit before DepthPrePass.
            if (m_cullComputePass && m_cullBarrierPass)
            {
                // Page uploads ride the cull compute pass; the barrier pass declares
                // the pool Read (pre-raster) so the AS/MS sample it in-state.
                if (!m_pageUploadItemsScratch.empty())
                {
                    m_cullDispatchItemsScratch.insert(m_cullDispatchItemsScratch.end(),
                        m_pageUploadItemsScratch.begin(), m_pageUploadItemsScratch.end());
                    m_cullArgsAttachmentsScratch.insert(m_cullArgsAttachmentsScratch.end(),
                        m_pageUploadAttachmentsScratch.begin(), m_pageUploadAttachmentsScratch.end());
                }
                if (m_pagePoolBuffer && m_pagePoolBuffer->GetRHIBuffer() && r_meshletsStreaming)
                {
                    MeshletsImportedAttachment poolRead;
                    poolRead.m_attachmentId = m_pagePoolAttachmentId;
                    poolRead.m_rhiBuffer = m_pagePoolBuffer->GetRHIBuffer();
                    poolRead.m_viewDescriptor = RHI::BufferViewDescriptor::CreateStructured(
                        0, m_pagePoolSlotCount * PageSlotU32s, static_cast<uint32_t>(sizeof(AZ::u32)));
                    poolRead.m_barrierUsage = RHI::ScopeAttachmentUsage::Shader;
                    poolRead.m_barrierStage = RHI::ScopeAttachmentStage::VertexShader;
                    m_cullBarrierAttachmentsScratch.push_back(poolRead);
                }
                // Compute pass: only instances that actually re-culled this frame.
                m_cullComputePass->SetImportedAttachments(m_cullArgsAttachmentsScratch);
                m_cullComputePass->AddDispatchItems(m_cullDispatchItemsScratch);
                // Barrier pass: EVERY active instance (incl. skipped) so every compacted/
                // args buffer is transitioned to its read state for the draws -- a skipped
                // instance redraws last frame's still-valid compacted data.
                m_cullBarrierPass->SetIndirectBarrierAttachments(m_cullBarrierAttachmentsScratch);
                // The barrier pass runs no dispatches.
                m_cullBarrierPass->AddDispatchItems({});
                // Barrier pass declares the last-completed pyramid Read (UAV->SRV
                // before any consumer); cleared when HiZ is off.
                m_cullBarrierPass->SetHiZReadImage(m_hiZBindValid ? m_hiZBindImage : nullptr);
                // Ledgers declared RW here = pre-depth UAV state + sync point.
                m_cullBarrierPass->SetImportedAttachments(m_visFrameAttachmentsScratch);
            }

            // Add draw packets to EVERY view in the render packet.
            // View::AddDrawPacket distributes each DrawItem by tag, so:
            //  - main camera view receives depth ("MeshletsDepthDrawList") +
            //    forward ("MeshletsDrawList") items
            //  - shadow cascade views receive shadow ("shadow") items
            //
            // Previously we only added through m_renderPass->AddDrawPackets,
            // which submits to the render pass's own view (the main camera).
            // Shadow cascade views -- separate Views created by the shadow
            // system -- never received the meshlet DrawPackets, so meshlets
            // could not cast shadows.
            for (const RPI::ViewPtr& view : packet.m_views)
            {
                for (const RHI::DrawPacket* drawPacket : m_drawPacketsScratch)
                {
                    if (drawPacket)
                    {
                        view->AddDrawPacket(drawPacket, 0.0f);
                    }
                }
            }
        }

        void MeshletsFeatureProcessor::OnRenderPipelineChanged(RPI::RenderPipeline* renderPipeline, RPI::SceneNotification::RenderPipelineChangeType changeType)
        {
            if (changeType == RPI::SceneNotification::RenderPipelineChangeType::Removed)
            {
                if (m_renderPipeline == renderPipeline)
                {
                    m_renderPipeline = nullptr;
                    CleanPasses();
                }
                m_loggedDisabledPipelines.erase(renderPipeline);
                return;
            }

            const bool relevantChange =
                changeType == RPI::SceneNotification::RenderPipelineChangeType::Added ||
                changeType == RPI::SceneNotification::RenderPipelineChangeType::PassChanged;
            if (!relevantChange)
            {
                return;
            }

            // Step 1: prefer a project-declared pass.
            // Step 2: try auto-injection (only on Added; PassChanged means the pipeline
            //         already settled and re-injecting on every pass mutation would loop).
            const bool passPresent =
                HasMeshletPasses(renderPipeline) ||
                (changeType == RPI::SceneNotification::RenderPipelineChangeType::Added &&
                 TryAutoInjectPasses(renderPipeline) &&
                 HasMeshletPasses(renderPipeline));

            // GPU cull early pass (independent best-effort injection on Added).
            if (changeType == RPI::SceneNotification::RenderPipelineChangeType::Added)
            {
                TryAutoInjectCullPass(renderPipeline);
                // Two-pass occlusion PASS 2 (after GpuCullAndDrawPass' HiZ reduce).
                TryAutoInjectLatePass(renderPipeline);
            }

            if (!passPresent)
            {
                // Skip the warning entirely for execute-once pipelines (BRDF / IBL
                // baking / reflection probe captures). They aren't meant to render
                // scene geometry and aren't meshlet candidates.
                if (renderPipeline->IsExecuteOnce())
                {
                    return;
                }

                // Throttle: log exactly once per pipeline. Repeated PassChanged
                // notifications would otherwise spam this every frame.
                if (m_loggedDisabledPipelines.insert(renderPipeline).second)
                {
                    AZ_TracePrintf("Meshlets",
                        "Pipeline [%s] does not contain MeshletsParentPass and auto-injection failed; "
                        "meshlets rendering is disabled for this pipeline. "
                        "Add MeshletsParentPass to the pipeline template to enable.\n",
                        renderPipeline->GetId().GetCStr());
                }
                return;
            }

            m_renderPipeline = renderPipeline;
            CreateResources();
            Init(m_renderPipeline);
            AttachSharedBufferToPasses();  // one-shot; replaces per-frame retry in Render
        }
    } // namespace Meshlets
} // namespace AZ
