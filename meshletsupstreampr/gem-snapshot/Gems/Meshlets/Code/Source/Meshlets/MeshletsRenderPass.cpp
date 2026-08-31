/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/DeviceDrawPacketBuilder.h>
#include <Atom/RHI/DevicePipelineState.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI.Reflect/BufferScopeAttachmentDescriptor.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>

#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/Scene.h>

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Pass/RasterPassData.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <MeshletsRenderPass.h>
#include <MeshletsUtilities.h>
#include <MeshletsFeatureProcessor.h>

namespace AZ
{
    namespace Meshlets
    {
        // --- Creation & Initialization ---
        RPI::Ptr<MeshletsRenderPass> MeshletsRenderPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<MeshletsRenderPass> pass = aznew MeshletsRenderPass(descriptor);
            return pass;
        }

        MeshletsRenderPass::MeshletsRenderPass(const RPI::PassDescriptor& descriptor)
            : RasterPass(descriptor),
            m_passDescriptor(descriptor)
        {
            // Read the shader path from the PassData's PassSrgShaderAsset field.
            // This lets multiple instances of MeshletsRenderPass load different
            // shaders (e.g., depth-only vs forward) based on the pass template
            // data rather than a hardcoded path.
            const RPI::RasterPassData* passData = RPI::PassUtils::GetPassData<RPI::RasterPassData>(m_passDescriptor);
            if (passData && passData->m_passSrgShaderReference.m_filePath.size() > 0)
            {
                // PassData stores the source .shader path (e.g., "Shaders/MeshletsDepthPass.shader").
                // The AP product uses .azshader extension with lowercase name.
                // LoadAssetByProductPath handles case-insensitive matching.
                AZStd::string productPath = passData->m_passSrgShaderReference.m_filePath;
                AZStd::size_t dotShader = productPath.rfind(".shader");
                if (dotShader != AZStd::string::npos)
                {
                    productPath.replace(dotShader, 7, ".azshader");
                }
                SetShaderPath(productPath.c_str());
            }
            else
            {
                SetShaderPath("Shaders/meshletsdebugrendershader.azshader");
            }
            LoadShader();
        }

        bool MeshletsRenderPass::AcquireFeatureProcessor()
        {
            if (m_featureProcessor)
            {
                return true;
            }

            RPI::Scene* scene = GetScene();
            if (!scene)
            {
                return false;
            }

            m_featureProcessor = scene->GetFeatureProcessor<MeshletsFeatureProcessor>();
            if (!m_featureProcessor)
            {
                AZ_Warning("Meshlets", false,
                    "MeshletsRenderPass [%s] - Failed to retrieve Meshlets feature processor from the scene",
                    GetName().GetCStr());
                return false;
            }
            return true;
        }

        void MeshletsRenderPass::InitializeInternal()
        {
            if (GetScene())
            {
                RasterPass::InitializeInternal();
            }
        }


        bool MeshletsRenderPass::LoadShader()
        {
            RPI::ShaderReloadNotificationBus::Handler::BusDisconnect();

            const RPI::RasterPassData* passData = RPI::PassUtils::GetPassData<RPI::RasterPassData>(m_passDescriptor);

            // If we successfully retrieved our custom data, use it to set the DrawListTag
            if (!passData)
            {
                AZ_Error("Meshlets", false, "Missing pass raster data");
                return false;
            }

            // Load Shader
            const char* shaderFilePath = m_shaderPath.c_str();
            Data::Asset<RPI::ShaderAsset> shaderAsset =
                RPI::AssetUtils::LoadAssetByProductPath<RPI::ShaderAsset>(shaderFilePath, RPI::AssetUtils::TraceLevel::Error);

            if (!shaderAsset.GetId().IsValid())
            {
                AZ_Error("Meshlets", false, "Invalid shader asset for shader '%s'!", shaderFilePath);
                return false;
            }

            m_shader = RPI::Shader::FindOrCreate(shaderAsset);
            if (!m_shader)
            {
                AZ_Error("Meshlets", false, "Pass failed to load shader '%s'!", shaderFilePath);
                return false;
            }

            // Per Pass Srg
            //
            // SP1 fix: the meshlets render shader no longer declares (or needs)
            // a PassSrg — all data is per-object (MeshletsObjectRenderSrg
            // m_indices/m_uvs/m_positions/...) or per-instance
            // (MeshletsInstanceRenderSrg). The base RasterPass tolerates a null
            // m_shaderResourceGroup (CompileResources early-outs on null), so
            // make creation conditional: only create the PassSrg if the shader
            // asset actually declares the layout. This avoids the
            // "ShaderResourceGroup cannot be initialized due to invalid
            // ShaderResourceGroupLayout" assert that fires when the shader is
            // re-authored without a PassSrg but C++ still tries to bind one.
            {
                // Reuse the outer 'shaderAsset' loaded at the top of LoadShader.
                const RPI::SupervariantIndex supervariantIndex =
                    shaderAsset->GetSupervariantIndex(AZ::Name(""));
                const auto& passSrgLayout =
                    shaderAsset->FindShaderResourceGroupLayout(
                        AZ::Name{ "PassSrg" }, supervariantIndex);
                if (passSrgLayout)
                {
                    m_shaderResourceGroup = UtilityClass::CreateShaderResourceGroup(
                        m_shader, "PassSrg", "Meshlets");
                    if (!m_shaderResourceGroup)
                    {
                        AZ_Error("Meshlets", false, "Failed to create the per pass srg");
                        return false;
                    }
                }
                else
                {
                    // Shader has no PassSrg — that's intentional after the
                    // dedicated-per-object-buffer refactor. Leave
                    // m_shaderResourceGroup null; RasterPass handles that.
                    m_shaderResourceGroup = nullptr;
                    AZ_TracePrintf("Meshlets",
                        "MeshletsRenderPass: shader has no PassSrg layout; "
                        "skipping per-pass SRG creation (intentional).\n");
                }
            }
            RPI::ShaderReloadNotificationBus::Handler::BusConnect(shaderAsset.GetId());

            return true;
        }

        bool MeshletsRenderPass::InitializePipelineState()
        {
            if (!m_shader)
            {
                return false;
            }

            const RPI::ShaderVariant& shaderVariant = m_shader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
            RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);

            RPI::Scene* scene = GetScene();
            if (!scene)
            {
                AZ_Error("Meshlets", false, "Scene could not be acquired");
                return false;
            }
            RHI::DrawListTag drawListTag = m_shader->GetDrawListTag();
            scene->ConfigurePipelineState(drawListTag, pipelineStateDescriptor);

            pipelineStateDescriptor.m_renderAttachmentConfiguration = GetRenderAttachmentConfiguration();

            // Vertex-pull rendering: replace any input stream channels that the shader
            // variant or scene config may have populated with a clean empty layout.
            // Our VS reads only SV_VertexID; if we leave stale channels in place, the
            // IA stage expects vertex buffers we never bind and the GPU hangs (we hit
            // exactly this -- DXGI_ERROR_DEVICE_HUNG even with a hardcoded-triangle
            // VS that touched no SRG data). FullscreenTrianglePass uses the same
            // pattern for the same reason.
            {
                RHI::InputStreamLayout emptyLayout;
                emptyLayout.SetTopology(AZ::RHI::PrimitiveTopology::TriangleList);
                emptyLayout.Finalize();
                pipelineStateDescriptor.m_inputStreamLayout = emptyLayout;
            }

            // Winding is preserved end-to-end: meshopt_buildMeshlets keeps
            // per-triangle {v0,v1,v2} order, the pack encoder/decoder mirrors
            // the same bit layout, and the vertex-pull shader fetches linearly
            // via SV_VertexID. Safe to cull back-faces (CCW front).
            pipelineStateDescriptor.m_renderStates.m_rasterState.m_cullMode =
                AZ::RHI::CullMode::Back;

            m_pipelineState = m_shader->AcquirePipelineState(pipelineStateDescriptor);
            if (!m_pipelineState)
            {
                AZ_Error("Meshlets", false, "Pipeline state could not be acquired");
                return false;
            }

            return true;
        }

        Data::Instance<RPI::Shader> MeshletsRenderPass::GetShader()
        {
            if (!m_shader)
            {
                AZ_Error("Meshlets", LoadShader(), "MeshletsRenderPass could not initialize pipeline or shader");
            }
            return m_shader;
        }

        bool MeshletsRenderPass::FillDrawRequestData(RHI::DrawPacketBuilder::DrawRequest& drawRequest)
        {
            if (!m_pipelineState)
            {
                return false;
            }

            drawRequest.m_listTag = m_drawListTag;
            drawRequest.m_pipelineState = m_pipelineState;

            return true;
        }

        void MeshletsRenderPass::SetImportedAttachments(const AZStd::vector<MeshletsImportedAttachment>& attachments)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_importedAttachmentsMutex);
            m_importedAttachments = attachments;
        }

        void MeshletsRenderPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            RPI::RasterPass::SetupFrameGraphDependencies(frameGraph);

            // SP1: declare Read (SRV, vertex-shader-stage) scope usage on each
            // imported attachment. The corresponding compute pass already
            // imported the buffer and declared ReadWrite usage, so the frame
            // graph has both endpoints and inserts the UAV->SRV barrier.
            //
            // We also call ImportBuffer here as a defensive idempotent op:
            // pass execution order across scenes/pipelines isn't strictly
            // ordered, and Pass.cpp's existing path treats a re-import of the
            // same (id, resource) pair as safe.
            AZStd::lock_guard<AZStd::mutex> lock(m_importedAttachmentsMutex);
            int32_t imported = 0;
            int32_t scoped   = 0;
            int32_t importFails = 0;
            int32_t useFails    = 0;
            for (const auto& att : m_importedAttachments)
            {
                if (!att.m_rhiBuffer)
                {
                    continue;
                }
                const AZ::RHI::ResultCode importRes =
                    frameGraph.GetAttachmentDatabase().ImportBuffer(
                        att.m_attachmentId, att.m_rhiBuffer);
                if (importRes == AZ::RHI::ResultCode::Success)
                {
                    ++imported;
                }
                else
                {
                    ++importFails;
                }

                RHI::BufferScopeAttachmentDescriptor scopeDesc(
                    att.m_attachmentId, att.m_viewDescriptor);
                // Two-pass occlusion: the visibility ledger is UAV on this scope (the
                // AS reads pass-1 bits and writes its own); everything else keeps the
                // SP1 Read/SRV declare. VertexShader stage maps to the pre-raster
                // (non-pixel) stages, which covers the amplification stage.
                const AZ::RHI::ResultCode useRes = frameGraph.UseShaderAttachment(
                    scopeDesc,
                    att.m_renderPassReadWrite ? RHI::ScopeAttachmentAccess::ReadWrite
                                              : RHI::ScopeAttachmentAccess::Read,
                    RHI::ScopeAttachmentStage::VertexShader);
                if (useRes == AZ::RHI::ResultCode::Success)
                {
                    ++scoped;
                }
                else
                {
                    ++useFails;
                }
            }
            const int32_t total = static_cast<int32_t>(m_importedAttachments.size());
            if (total != m_lastReportedImportCount)
            {
                AZ_TracePrintf("Meshlets",
                    "MeshletsRenderPass::SetupFrameGraphDependencies: "
                    "received %d attachment(s); imported(idempotent)=%d, scopedSRV=%d "
                    "(importFails=%d useFails=%d) [was %d].\n",
                    total, imported, scoped, importFails, useFails,
                    m_lastReportedImportCount);
                m_lastReportedImportCount = total;
            }
        }

        // Adding draw packets
        bool MeshletsRenderPass::AddDrawPackets(const AZStd::vector<const RHI::DrawPacket*>& drawPackets)
        {
            bool overallSuccess = true;

            if (!m_currentView &&
                (!(m_currentView = GetView()) || !m_currentView->HasDrawListTag(m_drawListTag)))
            {
                m_currentView = nullptr;    // set it to nullptr to prevent further attempts this frame
                AZ_Warning("Meshlets", false, "AddDrawPackets: failed to acquire or match the DrawListTag - check that your pass and shader tag name match");
                return false;
            }
            
            int32_t added = 0;
            int32_t nullPackets = 0;
            for (const RHI::DrawPacket* drawPacket : drawPackets)
            {
                if (!drawPacket)
                {   // might not be an error - the object might have just been added and the DrawPacket is
                    // scheduled to be built when the render frame begins
                    AZ_Warning("Meshlets", false, "MeshletsRenderPass - DrawPacket wasn't built");
                    overallSuccess = false;
                    ++nullPackets;
                    continue;   // other draw packets might be ok - don't break
                }
                m_currentView->AddDrawPacket(drawPacket);
                ++added;
            }

            // SP1 diagnostic: emit on every change to the added-packet count. Lets
            // us confirm DrawPackets are actually flowing into the View's draw
            // list. If we never see "added N>0" here despite the feature
            // processor having instances, the packets aren't getting built or
            // m_currentView is rejecting them.
            if (added != m_lastReportedDrawPacketCount)
            {
                AZ_TracePrintf("Meshlets",
                    "MeshletsRenderPass::AddDrawPackets: added %d packet(s) to view "
                    "(null=%d, total received=%zu) [was %d].\n",
                    added, nullPackets, drawPackets.size(),
                    m_lastReportedDrawPacketCount);
                m_lastReportedDrawPacketCount = added;
            }
            return overallSuccess;
        }

        void MeshletsRenderPass::FrameBeginInternal(FramePrepareParams params)
        {
            if (!m_shader && AcquireFeatureProcessor())
            {
                LoadShader();
            }

            if (m_shader && !m_pipelineState)
            {
                InitializePipelineState();
            }

            if (!m_shader || !m_pipelineState)
            {
                return;
            }

            // Refresh current view every frame
            m_currentView = GetView();
            if (!m_currentView || !m_currentView->HasDrawListTag(m_drawListTag))
            {
                m_currentView = nullptr;
                return;
            }

            // PERF: skip building the (full-res color+depth) RasterPass scope entirely
            // when there is no "MeshletsDrawList" work this frame. In the normal path
            // meshlets render via the standard ForwardPass, so this pass is empty yet was
            // costing a whole geometry pass per frame. m_hasDrawWork is set by the feature
            // processor each frame (true only when the forward shader failed and a
            // debug-fallback item exists). NOTE: do NOT gate on m_lastReportedDrawPacketCount
            // — that counter is fed by the now-unused AddDrawPackets() path and stays stale.
            if (!m_hasDrawWork)
            {
                return;
            }

            RPI::RasterPass::FrameBeginInternal(params);
        }

        void MeshletsRenderPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            AZ_PROFILE_FUNCTION(AzRender);

            if (!m_featureProcessor)
            {
                return;
            }

            // Compilation of remaining srgs will be done by the parent class
            RPI::RasterPass::CompileResources(context);
        }

        void MeshletsRenderPass::BuildShaderAndRenderData()
        {
            // Invalidate all DrawPackets BEFORE freeing the old pipeline state.
            // DrawPackets hold raw pointers to the pipeline state (via DrawRequest)
            // and to the SRGs. If the old pipeline state is freed while DrawPackets
            // still reference it, CommitShaderResources will read freed memory and
            // crash (DXGI_ERROR_DEVICE_HUNG or access violation at a garbage
            // address inside SetPipelineState / SetShaderResourceGroup).
            if (m_featureProcessor)
            {
                m_featureProcessor->InvalidateAllDrawPackets();
            }

            m_shader = nullptr;
            m_pipelineState = nullptr;
            // Do NOT call InitializePipelineState() here. Shader reload events can be
            // dispatched mid pass-tree rebuild (any pass block-loading a shader pumps
            // AssetManager::DispatchEvents), at which point ResetInternal has cleared
            // m_renderAttachmentConfiguration and GetRenderAttachmentConfiguration()
            // asserts. FrameBeginInternal rebuilds the pipeline state next frame when
            // the pass is built and the attachment configuration is valid again
            // (same deferral FullscreenTrianglePass uses on reload).
            if (!AcquireFeatureProcessor() || !LoadShader())
            {
                AZ_Error( "Meshlets", false, "MeshletsRenderPass::BuildShaderAndRenderData failed");
            }
        }

        void MeshletsRenderPass::OnShaderReinitialized([[maybe_unused]] const RPI::Shader & shader)
        {
            BuildShaderAndRenderData();
        }

        void MeshletsRenderPass::OnShaderAssetReinitialized([[maybe_unused]] const Data::Asset<RPI::ShaderAsset>& shaderAsset)
        {
            BuildShaderAndRenderData();
        }

        void MeshletsRenderPass::OnShaderVariantReinitialized([[maybe_unused]] const AZ::RPI::ShaderVariant& shaderVariant)
        {
            BuildShaderAndRenderData();
        }
    } // namespace Meshlets
}   // namespace AZ
