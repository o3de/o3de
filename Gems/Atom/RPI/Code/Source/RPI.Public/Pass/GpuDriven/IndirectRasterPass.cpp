/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Pass/GpuDriven/IndirectRasterPass.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DrawItem.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/IndirectBufferSignature.h>
#include <Atom/RHI/IndirectBufferView.h>
#include <Atom/RHI.Reflect/IndirectBufferLayout.h>

#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Reflect/Pass/GpuDriven/IndirectRasterPassData.h>

namespace AZ
{
    namespace RPI
    {
        Ptr<IndirectRasterPass> IndirectRasterPass::Create(const PassDescriptor& descriptor)
        {
            Ptr<IndirectRasterPass> pass = aznew IndirectRasterPass(descriptor);
            return pass;
        }

        IndirectRasterPass::IndirectRasterPass(const PassDescriptor& descriptor)
            : RasterPass(descriptor)
        {
        }

        IndirectRasterPass::~IndirectRasterPass()
        {
            ShaderReloadNotificationBus::Handler::BusDisconnect();
        }

        void IndirectRasterPass::SetMaxDrawCount(uint32_t maxCount)
        {
            m_maxDrawCount = maxCount;
        }

        void IndirectRasterPass::LoadShader()
        {
            const auto* passData = PassUtils::GetPassData<IndirectRasterPassData>(GetPassDescriptor());
            if (!passData)
            {
                AZ_Error("PassSystem", false, "[IndirectRasterPass '%s']: No valid IndirectRasterPassData!",
                    GetPathName().GetCStr());
                return;
            }

            m_indirectDrawBufferSlotName = passData->m_indirectDrawBufferSlotName;
            m_countBufferSlotName = passData->m_countBufferSlotName;
            m_maxDrawCount = passData->m_maxDrawCount;
            m_indexedDraw = passData->m_indexedDraw;

            Data::Asset<ShaderAsset> shaderAsset;
            if (passData->m_shaderReference.m_assetId.IsValid())
            {
                shaderAsset = RPI::FindShaderAsset(passData->m_shaderReference.m_assetId, passData->m_shaderReference.m_filePath);
            }

            if (!shaderAsset.IsReady())
            {
                AZ_Error("PassSystem", false, "[IndirectRasterPass '%s']: Failed to load shader '%s'!",
                    GetPathName().GetCStr(),
                    passData->m_shaderReference.m_filePath.data());
                return;
            }

            m_shader = Shader::FindOrCreate(shaderAsset);
            if (!m_shader)
            {
                AZ_Error("PassSystem", false, "[IndirectRasterPass '%s']: Failed to create shader instance from '%s'!",
                    GetPathName().GetCStr(),
                    passData->m_shaderReference.m_filePath.data());
                return;
            }

            // Load Pass SRG
            const auto passSrgLayout = m_shader->FindShaderResourceGroupLayout(SrgBindingSlot::Pass);
            if (passSrgLayout)
            {
                m_shaderResourceGroup = ShaderResourceGroup::Create(shaderAsset, m_shader->GetSupervariantIndex(), passSrgLayout->GetName());

                AZ_Assert(m_shaderResourceGroup, "[IndirectRasterPass '%s']: Failed to create Pass SRG from shader '%s'",
                    GetPathName().GetCStr(),
                    passData->m_shaderReference.m_filePath.data());

                PassUtils::BindDataMappingsToSrg(GetPassDescriptor(), m_shaderResourceGroup.get());
            }

            // Load Draw SRG
            const bool compileDrawSrg = false;
            m_drawSrg = m_shader->CreateDefaultDrawSrg(compileDrawSrg);

            // The shader (re)loaded: every previously-built per-material-type PSO/signature is
            // stale. Actual PSO compilation is deferred (per material type) to CompileResources,
            // once render target formats are known from resolved attachments.
            m_needsPipelineStateRebuild = true;

            if (m_drawSrg && m_shader->GetDefaultVariant().UseKeyFallback())
            {
                ShaderOptionGroup options = m_shader->GetDefaultShaderOptions();
                m_drawSrg->SetShaderVariantKeyFallbackValue(options.GetShaderVariantKeyFallbackValue());
            }

            ShaderReloadNotificationBus::Handler::BusDisconnect();
            ShaderReloadNotificationBus::Handler::BusConnect(passData->m_shaderReference.m_assetId);
        }

        void IndirectRasterPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            RasterPass::SetupFrameGraphDependencies(frameGraph);

            // Override the estimated item count -- IndirectRasterPass submits one ExecuteIndirect
            // call per non-empty bucket, not individual draw items. This must match exactly how
            // many submits BuildCommandListInternal will issue, or the frame graph will assign
            // submit ranges that skip/duplicate indices. With no buckets set (e.g. the depth-only
            // instance) GetEffectiveBuckets() returns exactly one, so this is still 1 there.
            uint32_t itemCount = 0;
            for (const IndirectDrawBucket& bucket : GetEffectiveBuckets())
            {
                if (bucket.m_count > 0)
                {
                    ++itemCount;
                }
            }
            frameGraph.SetEstimatedItemCount(itemCount);
        }

        void IndirectRasterPass::BuildInternal()
        {
            LoadShader();
            RasterPass::BuildInternal();

            // Resolve indirect draw buffer binding
            m_indirectDrawBufferBinding = nullptr;
            if (!m_indirectDrawBufferSlotName.IsEmpty())
            {
                m_indirectDrawBufferBinding = FindAttachmentBinding(m_indirectDrawBufferSlotName);
                AZ_Assert(m_indirectDrawBufferBinding,
                    "[IndirectRasterPass '%s']: Indirect draw buffer slot '%s' not found.",
                    GetPathName().GetCStr(), m_indirectDrawBufferSlotName.GetCStr());
            }

            // Resolve count buffer binding
            m_countBufferBinding = nullptr;
            if (!m_countBufferSlotName.IsEmpty())
            {
                m_countBufferBinding = FindAttachmentBinding(m_countBufferSlotName);
            }

            // Pass SRG buffers are resolved by name against the SRG by the base RenderPass, driven by
            // each slot's ShaderInputName -- nothing to cache here.
        }


        void IndirectRasterPass::BuildIndirectSignature(MaterialTypePso& entry)
        {
            // The RootConstants indirect argument needs the pipeline layout's root-constant slot,
            // so the signature MUST be built against a compiled pipeline state (not null).
            if (entry.m_indirectSignatureBuilt || !entry.m_pipelineState)
            {
                return;
            }

            RHI::IndirectBufferLayout layout;
            // Order matters: set the root constant, THEN draw.
            layout.AddIndirectCommand(RHI::IndirectCommandDescriptor(RHI::IndirectCommandType::RootConstants));
            layout.AddIndirectCommand(RHI::IndirectCommandDescriptor(RHI::IndirectCommandType::Draw));
            if (!layout.Finalize())
            {
                AZ_Assert(false, "[IndirectRasterPass '%s']: Failed to finalize indirect buffer layout.", GetPathName().GetCStr());
                return;
            }

            entry.m_indirectBufferSignature = aznew RHI::IndirectBufferSignature;
            RHI::IndirectBufferSignatureDescriptor signatureDescriptor;
            signatureDescriptor.m_layout = layout;
            signatureDescriptor.m_pipelineState = entry.m_pipelineState; // REQUIRED for RootConstants
            auto result = entry.m_indirectBufferSignature->Init(RHI::MultiDevice::AllDevices, signatureDescriptor);
            if (result != RHI::ResultCode::Success)
            {
                AZ_Warning("PassSystem", false,
                    "[IndirectRasterPass '%s']: Failed to init indirect signature (result=%d).",
                    GetPathName().GetCStr(), static_cast<int>(result));
                entry.m_indirectBufferSignature = nullptr;
                return;
            }
            entry.m_indirectSignatureBuilt = true;
        }

        IndirectRasterPass::MaterialTypePso& IndirectRasterPass::EnsurePsoEntry(uint32_t materialTypeId)
        {
            auto it = m_psoByMaterialType.find(materialTypeId);
            if (it != m_psoByMaterialType.end())
            {
                return it->second;
            }

            MaterialTypePso& entry = m_psoByMaterialType[materialTypeId];

            // ponytail: every material type resolves to the same GpuDrivenForward shader today
            // (its bindless FallbackPBRMaterial path already covers every authored material
            // type -- see the shader's header comment), so every entry is built from the one
            // m_shader this pass loaded. Upgrade path: if a future shader genuinely needs a
            // per-type variant, select it here by materialTypeId instead of always using m_shader.
            entry.m_pipelineStateForDraw.Init(m_shader, m_shader->GetDefaultShaderOptions().GetShaderVariantId());
            entry.m_pipelineStateForDraw.SetOutputFromPass(this);

            // For vertex-pulling shaders (using SV_VertexID instead of vertex assembly),
            // the InputStreamLayout is empty. Set topology and finalize explicitly.
            RHI::InputStreamLayout inputStreamLayout;
            inputStreamLayout.SetTopology(RHI::PrimitiveTopology::TriangleList);
            inputStreamLayout.Finalize();
            entry.m_pipelineStateForDraw.SetInputStreamLayout(inputStreamLayout);

            entry.m_pipelineState = entry.m_pipelineStateForDraw.Finalize();
            BuildIndirectSignature(entry);

            if (!entry.m_pipelineState)
            {
                AZ_Warning("PassSystem", false,
                    "[IndirectRasterPass '%s']: Pipeline state failed to compile for material type %u. "
                    "That bucket will be skipped.",
                    GetPathName().GetCStr(), materialTypeId);
            }

            return entry;
        }

        AZStd::vector<IndirectRasterPass::IndirectDrawBucket> IndirectRasterPass::GetEffectiveBuckets() const
        {
            if (!m_buckets.empty())
            {
                return m_buckets;
            }

            // No buckets supplied (e.g. IndirectDepthPass, which never calls SetIndirectBuckets):
            // a single implicit bucket spanning the whole buffer, exactly like before this class
            // grew multi-bucket support.
            return { IndirectDrawBucket{ /*materialTypeId*/ 0, /*offset*/ 0, /*count*/ m_maxDrawCount } };
        }

        void IndirectRasterPass::SetIndirectBuckets(AZStd::vector<IndirectDrawBucket> buckets)
        {
            m_buckets = AZStd::move(buckets);
        }

        void IndirectRasterPass::InitializeInternal()
        {
            if (!m_shader)
            {
                RasterPass::InitializeInternal();
                return;
            }

            // Build render attachment configuration from resolved output attachments.
            // This provides the render target formats needed for PSO compilation.
            BuildRenderAttachmentConfiguration();

            RasterPass::InitializeInternal();

            if (m_needsPipelineStateRebuild)
            {
                // Shader (re)loaded -> every cached per-material-type PSO/signature is stale.
                // Entries are rebuilt lazily (per material type) in CompileResources.
                m_psoByMaterialType.clear();
                m_needsPipelineStateRebuild = false;
            }
        }

        void IndirectRasterPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            // This used to hand-bind exactly two Pass SRG buffers by fixed index, because
            // RasterPass::CompileResources -> BindPassSrg auto-binds every input buffer attachment and
            // the Indirect-usage buffers (IndirectDrawArgs, DrawCount) are consumed by ExecuteIndirect
            // rather than read as shader resources -- auto-binding them shifted the sequential buffer
            // index and produced stride/type mismatches.
            //
            // That hack capped the shader at those two buffers: any other Pass SRG resource compiled
            // fine and then silently never got bound, which is why the GPU-driven forward shader could
            // not reach shadow maps, the tiled light-culling buffers, or the BRDF LUT that IBL needs.
            //
            // The pass system already has the right mechanism: a slot whose ShaderInputName is "NoBind"
            // is skipped by BindAttachment (RenderPass.cpp), and every other slot resolves BY NAME
            // against the SRG rather than by sequential index. The Indirect slots are now marked
            // "NoBind" in IndirectRaster.pass / GpuCullDebug.pass, so the base implementation binds
            // everything else correctly and new Pass SRG resources work without touching this function.
            RasterPass::CompileResources(context);

            if (m_shaderResourceGroup)
            {
                BindSrg(m_shaderResourceGroup->GetRHIShaderResourceGroup());
            }
            if (m_drawSrg)
            {
                BindSrg(m_drawSrg->GetRHIShaderResourceGroup());
                m_drawSrg->Compile();
            }

            // Cache buffer pointers for BuildCommandListInternal (which only has FrameGraphExecuteContext)
            m_cachedIndirectBuffer = nullptr;
            m_cachedCountBuffer = nullptr;

            if (m_indirectDrawBufferBinding && m_indirectDrawBufferBinding->GetAttachment())
            {
                m_cachedIndirectBuffer = context.GetBuffer(m_indirectDrawBufferBinding->GetAttachment()->GetAttachmentId());
            }

            if (m_countBufferBinding && m_countBufferBinding->GetAttachment())
            {
                m_cachedCountBuffer = context.GetBuffer(m_countBufferBinding->GetAttachment()->GetAttachmentId());
            }

            // Build/refresh every bucket's PSO + indirect signature here, once, single-threaded,
            // so BuildCommandListInternal (which may run its submits from multiple worker threads
            // across the frame graph's submit range) only ever reads the map.
            if (m_shader)
            {
                for (const IndirectDrawBucket& bucket : GetEffectiveBuckets())
                {
                    EnsurePsoEntry(bucket.m_materialTypeId);
                }
            }
        }

        void IndirectRasterPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            if (!m_cachedIndirectBuffer)
            {
                return;
            }

            const auto submitRange = context.GetSubmitRange();
            if (submitRange.m_startIndex == submitRange.m_endIndex)
            {
                return;
            }

            RHI::CommandList* commandList = context.GetCommandList();
            bool preparedDrawState = false;
            uint32_t submitIndex = 0;

            for (const IndirectDrawBucket& bucket : GetEffectiveBuckets())
            {
                if (bucket.m_count == 0)
                {
                    continue; // matches SetupFrameGraphDependencies' item count, which also skips these
                }

                // Item at 'submitIndex' -- only handle it if it falls in the range this execute
                // context (this command list / worker thread) was assigned.
                if (submitIndex < submitRange.m_startIndex || submitIndex >= submitRange.m_endIndex)
                {
                    ++submitIndex;
                    continue;
                }
                const uint32_t thisSubmitIndex = submitIndex++;

                auto it = m_psoByMaterialType.find(bucket.m_materialTypeId);
                if (it == m_psoByMaterialType.end())
                {
                    continue; // EnsurePsoEntry never ran for this type (shader failed to load) -- skip it
                }
                const MaterialTypePso& entry = it->second;
                if (!entry.m_pipelineState || !entry.m_indirectBufferSignature)
                {
                    continue;
                }

                if (!preparedDrawState)
                {
                    commandList->SetViewport(m_viewportState);
                    commandList->SetScissor(m_scissorState);
                    SetSrgsForDraw(context);
                    preparedDrawState = true;
                }

                // Build the indirect buffer view for this bucket's element range.
                const uint32_t byteStride = entry.m_indirectBufferSignature->GetByteStride();
                const uint32_t byteOffset = byteStride * bucket.m_offset;
                const uint32_t byteCount = byteStride * bucket.m_count;

                RHI::IndirectBufferView indirectBufferView(
                    *m_cachedIndirectBuffer, *entry.m_indirectBufferSignature, byteOffset, byteCount, byteStride);

                // Build indirect draw arguments
                RHI::DrawIndirect indirectArgs;
                indirectArgs.m_maxSequenceCount = bucket.m_count;
                indirectArgs.m_indirectBufferView = &indirectBufferView;
                indirectArgs.m_indirectBufferByteOffset = 0;
                indirectArgs.m_countBuffer = m_cachedCountBuffer;
                indirectArgs.m_countBufferByteOffset = 0;

                // Create a geometry view with indirect draw arguments
                RHI::GeometryView geometryView(RHI::MultiDevice::AllDevices);
                geometryView.SetDrawArguments(RHI::DrawArguments(indirectArgs));

                // Build the draw item
                RHI::DrawItem drawItem(RHI::MultiDevice::AllDevices);
                drawItem.SetPipelineState(entry.m_pipelineState);
                drawItem.SetGeometryView(&geometryView);

                commandList->Submit(drawItem.GetDeviceDrawItem(context.GetDeviceIndex()), thisSubmitIndex);
            }
        }

        // ShaderReloadNotificationBus::Handler overrides
        void IndirectRasterPass::OnShaderReinitialized([[maybe_unused]] const Shader& shader)
        {
            LoadShader();
            QueueForInitialization();
        }

        void IndirectRasterPass::OnShaderAssetReinitialized([[maybe_unused]] const Data::Asset<ShaderAsset>& shaderAsset)
        {
            LoadShader();
            QueueForInitialization();
        }

        void IndirectRasterPass::OnShaderVariantReinitialized([[maybe_unused]] const ShaderVariant& shaderVariant)
        {
            if (m_shader)
            {
                m_needsPipelineStateRebuild = true;
                QueueForInitialization();
            }
        }

    } // namespace RPI
} // namespace AZ
