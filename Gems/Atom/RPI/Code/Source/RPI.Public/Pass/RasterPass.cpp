/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DrawListTagRegistry.h>
#include <Atom/RHI/RHISystemInterface.h>

#include <Atom/RPI.Public/DynamicDraw/DynamicDrawInterface.h>
#include <Atom/RPI.Public/Pass/RasterPass.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Pass/RasterPassData.h>

namespace AZ
{
    namespace RPI
    {
        // --- Creation & Initialization ---

        Ptr<RasterPass> RasterPass::Create(const PassDescriptor& descriptor)
        {
            Ptr<RasterPass> pass = aznew RasterPass(descriptor);
            return pass;
        }

        RasterPass::RasterPass(const PassDescriptor& descriptor)
            : RenderPass(descriptor)
        {
            const RasterPassData* rasterData = PassUtils::GetPassData<RasterPassData>(descriptor);

            // If we successfully retrieved our custom data, use it to set the DrawListTag
            if (rasterData == nullptr)
            {
                return;
            }

            SetDrawListTag(rasterData->m_drawListTag);
            m_drawListSortType = rasterData->m_drawListSortType;

            RHI::RHISystemInterface::Get()->SetDrawListTagEnabledByDefault(m_drawListTag, rasterData->m_enableDrawItemsByDefault);

            LoadShaderResourceGroup();

            if (!rasterData->m_overrideScissor.IsNull())
            {
                m_scissorState = rasterData->m_overrideScissor;
                m_overrideScissorSate = true;
            }
            if (!rasterData->m_overrideViewport.IsNull())
            {
                m_viewportState = rasterData->m_overrideViewport;
                m_overrideViewportState = true;
            }
            m_viewportAndScissorTargetOutputIndex = rasterData->m_viewportAndScissorTargetOutputIndex;
        }

        RasterPass::~RasterPass()
        {
            if (m_drawListTag != RHI::DrawListTag::Null)
            {
                RHI::RHISystemInterface* rhiSystem = RHI::RHISystemInterface::Get();
                rhiSystem->GetDrawListTagRegistry()->ReleaseTag(m_drawListTag);
            }
        }

        void RasterPass::SetDrawListTag(Name drawListName)
        {
            // Use AcquireTag to register a draw list tag if it doesn't exist. 
            RHI::RHISystemInterface* rhiSystem = RHI::RHISystemInterface::Get();
            m_drawListTag = rhiSystem->GetDrawListTagRegistry()->AcquireTag(drawListName);
            m_flags.m_hasDrawListTag = true;
        }

        void RasterPass::SetPipelineStateDataIndex(uint32_t index)
        {
            m_pipelineStateDataIndex.m_index = index;
        }

        ShaderResourceGroup* RasterPass::GetShaderResourceGroup()
        {
            return m_shaderResourceGroup.get();
        }

        uint32_t RasterPass::GetDrawItemCount()
        {
            return m_drawItemCount;
        }

        // --- Pass behaviour overrides ---

        void RasterPass::Validate(PassValidationResults& validationResults)
        {
            AZ_RPI_PASS_ERROR(m_drawListTag.IsValid(), "DrawListTag for RasterPass [%s] is invalid", GetPathName().GetCStr());
            AZ_RPI_PASS_ERROR(!GetPipelineViewTag().IsEmpty(), "ViewTag for RasterPass [%s] is invalid", GetPathName().GetCStr());
            RenderPass::Validate(validationResults);
        }

        void RasterPass::FrameBeginInternal(FramePrepareParams params)
        {
            // Binding to use for viewport and scissor calculations
            PassAttachmentBinding* viewportTarget = nullptr;

            // If a target binding for viewport calculation is specified
            if (m_viewportAndScissorTargetOutputIndex >= 0)
            {
                u32 idx = u32(m_viewportAndScissorTargetOutputIndex);
                // First check outputs
                if (GetOutputCount() > idx)
                {
                    viewportTarget = &GetOutputBinding(idx);
                }
                // If not an output, check input/outputs
                else if (GetInputOutputCount() > idx)
                {
                    viewportTarget = &GetInputOutputBinding(idx);
                }
            }
            // Build viewport and scissor from target binding if specified
            if (viewportTarget)
            {
                u32 targetWidth = viewportTarget->GetAttachment()->m_descriptor.m_image.m_size.m_width;
                u32 targetHeight = viewportTarget->GetAttachment()->m_descriptor.m_image.m_size.m_height;
                m_scissorState = RHI::Scissor(0, 0, targetWidth, targetHeight);
                m_viewportState = RHI::Viewport(0, static_cast<float>(targetWidth), 0, static_cast<float>(targetHeight));
            }
            // Otherwise check whether viewport/scissor overrides were manually provided
            else
            {
                if (!m_overrideScissorSate)
                {
                    m_scissorState = params.m_scissorState;
                }
                if (!m_overrideViewportState)
                {
                    m_viewportState = params.m_viewportState;
                }
            }
            UpdateDrawList();

            RenderPass::FrameBeginInternal(params);
        }

        void RasterPass::InitializeInternal()
        {
            BuildRenderAttachmentConfiguration();
            if (m_shaderResourceGroup &&
                m_shaderResourceGroup->GetShaderAsset()->GetSupervariantIndex(GetSuperVariantName()) != m_shaderResourceGroup->GetSupervariantIndex())
            {
                LoadShaderResourceGroup();
            }
            RenderPass::InitializeInternal();
        }

        void RasterPass::UpdateDrawList()
        {
             // DrawLists from dynamic draw
            AZStd::vector<RHI::DrawListView> drawLists = DynamicDrawInterface::Get()->GetDrawListsForPass(this);

            // Get DrawList from view
            const AZStd::vector<ViewPtr>& views = m_pipeline->GetViews(GetPipelineViewTag());
            RHI::DrawListView viewDrawList;
            if (!views.empty())
            {
                const ViewPtr& view = views.front();

                // Draw List. May return an empty list, and that's ok.
                viewDrawList = view->GetDrawList(m_drawListTag);
            }

            // clean up data
            m_drawListView = {};
            m_combinedDrawList.clear();
            m_drawItemCount = 0;

            // draw list from view was sorted and if it's the only draw list then we can use it directly
            if (viewDrawList.size() > 0 && drawLists.size() == 0)
            {
                m_drawListView = viewDrawList;
                m_drawItemCount += static_cast<uint32_t>(viewDrawList.size());
                PassSystemInterface::Get()->IncrementFrameDrawItemCount(m_drawItemCount);
                return;
            }

            // add view's draw list to drawLists too
            drawLists.push_back(viewDrawList);

            // combine draw items from mutiple draw lists to one draw list and sort it.
            for (auto drawList : drawLists)
            {
                m_drawItemCount += static_cast<uint32_t>(drawList.size());
            }
            PassSystemInterface::Get()->IncrementFrameDrawItemCount(m_drawItemCount);
            m_combinedDrawList.resize(m_drawItemCount);
            RHI::DrawItemProperties* currentBuffer = m_combinedDrawList.data();
            for (auto drawList : drawLists)
            {
                memcpy(currentBuffer, drawList.data(), drawList.size()*sizeof(RHI::DrawItemProperties));
                currentBuffer += drawList.size();
            }
            SortDrawList(m_combinedDrawList);

            // have the final draw list point to the combined draw list.
            m_drawListView = m_combinedDrawList;
        }

        // --- DrawList and PipelineView Tags ---

        RHI::DrawListTag RasterPass::GetDrawListTag() const
        {
            return m_drawListTag;
        }

        // --- Scope producer functions ---

        void RasterPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            DeclareAttachmentsToFrameGraph(frameGraph);
            DeclarePassDependenciesToFrameGraph(frameGraph);
            AddScopeQueryToFrameGraph(frameGraph);
            frameGraph.SetEstimatedItemCount(static_cast<uint32_t>(m_drawListView.size()));
        }

        void RasterPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            if (m_shaderResourceGroup == nullptr)
            {
                return;
            }

            BindPassSrg(context, m_shaderResourceGroup);

            m_shaderResourceGroup->Compile();
        }

        void RasterPass::SubmitDrawItems(const RHI::FrameGraphExecuteContext& context, uint32_t startIndex, uint32_t endIndex, uint32_t indexOffset) const
        {
            RHI::CommandList* commandList = context.GetCommandList();

            uint32_t clampedEndIndex = AZStd::GetMin<uint32_t>(endIndex, static_cast<uint32_t>(m_drawListView.size()));
            for (uint32_t index = startIndex; index < clampedEndIndex; ++index)
            {
                const RHI::DrawItemProperties& drawItemProperties = m_drawListView[index];
                if (drawItemProperties.m_drawFilterMask & m_pipeline->GetDrawFilterMask())
                {
                    commandList->Submit(drawItemProperties.m_item->GetDeviceDrawItem(context.GetDeviceIndex()), index + indexOffset);
                }
            }
        }

        void RasterPass::LoadShaderResourceGroup()
        {
            const RasterPassData* rasterData = azrtti_cast<const RasterPassData*>(m_passData.get());
            if (!rasterData)
            {
                return;
            }

            // Get the shader asset that contains the SRG Layout.
            Data::Asset<ShaderAsset> shaderAsset;
            if (rasterData->m_passSrgShaderReference.m_assetId.IsValid())
            {
                shaderAsset =
                    AssetUtils::LoadAssetById<ShaderAsset>(rasterData->m_passSrgShaderReference.m_assetId, AssetUtils::TraceLevel::Error);
            }
            else if (!rasterData->m_passSrgShaderReference.m_filePath.empty())
            {
                shaderAsset = AssetUtils::LoadAssetByProductPath<ShaderAsset>(
                    rasterData->m_passSrgShaderReference.m_filePath.c_str(), AssetUtils::TraceLevel::Error);
            }

            if (shaderAsset)
            {
                SupervariantIndex superVariantIndex = shaderAsset->GetSupervariantIndex(GetSuperVariantName());
                const auto srgLayout = shaderAsset->FindShaderResourceGroupLayout(SrgBindingSlot::Pass, superVariantIndex);
                if (srgLayout)
                {
                    m_shaderResourceGroup = ShaderResourceGroup::Create(shaderAsset, superVariantIndex, srgLayout->GetName());

                    AZ_Assert(
                        m_shaderResourceGroup,
                        "[RasterPass '%s']: Failed to create SRG from shader asset '%s'",
                        GetPathName().GetCStr(),
                        rasterData->m_passSrgShaderReference.m_filePath.data());

                    PassUtils::BindDataMappingsToSrg(GetPassDescriptor(), m_shaderResourceGroup.get());
                }
            }
        }

        void RasterPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            RHI::CommandList* commandList = context.GetCommandList();

            if (context.GetSubmitRange().m_startIndex != context.GetSubmitRange().m_endIndex)
            {
                commandList->SetViewport(m_viewportState);
                commandList->SetScissor(m_scissorState);
                SetSrgsForDraw(context);
                SubmitDrawItems(context, context.GetSubmitRange().m_startIndex, context.GetSubmitRange().m_endIndex, 0);
            }
        }
    }   // namespace RPI
}   // namespace AZ
