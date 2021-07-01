/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/ShaderResourceGroup.h>
#include <Atom/RHI/DrawListTagRegistry.h>

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Pass/RasterPass.h>

#include <Atom/RPI.Reflect/Pass/RasterPassData.h>
#include <Atom/RPI.Reflect/Shader/ShaderResourceGroupAsset.h>

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

            // Get SRG asset
            Data::Asset<ShaderResourceGroupAsset> srgAsset;
            if (rasterData->m_passSrgReference.m_assetId.IsValid())
            {
                srgAsset = AssetUtils::LoadAssetById<ShaderResourceGroupAsset>(rasterData->m_passSrgReference.m_assetId, AssetUtils::TraceLevel::Error);
            }
            else if (!rasterData->m_passSrgReference.m_filePath.empty())
            {
                srgAsset = AssetUtils::LoadAssetByProductPath<ShaderResourceGroupAsset>(rasterData->m_passSrgReference.m_filePath.c_str(), AssetUtils::TraceLevel::Error);
            }

            if (srgAsset)
            {
                m_shaderResourceGroup = ShaderResourceGroup::Create(srgAsset);

                AZ_Assert(m_shaderResourceGroup, "[RasterPass '%s']: Failed to create SRG from shader asset '%s'",
                    GetPathName().GetCStr(),
                    rasterData->m_passSrgReference.m_filePath.data());

                PassUtils::BindDataMappingsToSrg(descriptor, m_shaderResourceGroup.get());
            }


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

        void RasterPass::SetPipelineStateDataIndex(u32 index)
        {
            m_pipelineStateDataIndex.m_index = index;
        }

        ShaderResourceGroup* RasterPass::GetShaderResourceGroup()
        {
            return m_shaderResourceGroup.get();
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
            if (!m_overrideScissorSate)
            {
                m_scissorState = params.m_scissorState;
            }
            if (!m_overrideViewportState)
            {
                m_viewportState = params.m_viewportState;
            }

            // -- View & DrawList --
            const AZStd::vector<ViewPtr>& views = m_pipeline->GetViews(GetPipelineViewTag());
            m_drawListView = {};

            if (!views.empty())
            {
                const ViewPtr& view = views.front();

                // Assert the view has our draw list (the view's DrawlistTags are collected from passes using its viewTag)
                AZ_Assert(view->HasDrawListTag(m_drawListTag), "View's DrawListTags out of sync with pass'. ");

                // Draw List 
                m_drawListView = view->GetDrawList(m_drawListTag);
            }

            RenderPass::FrameBeginInternal(params);
        }

        // --- DrawList and PipelineView Tags ---

        RHI::DrawListTag RasterPass::GetDrawListTag() const
        {
            return m_drawListTag;
        }

        // --- Scope producer functions ---

        void RasterPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            RenderPass::SetupFrameGraphDependencies(frameGraph);
            frameGraph.SetEstimatedItemCount(static_cast<u32>(m_drawListView.size()));
        }

        void RasterPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);

            if (m_shaderResourceGroup == nullptr)
            {
                return;
            }

            BindPassSrg(context, m_shaderResourceGroup);

            m_shaderResourceGroup->Compile();
        }

        void RasterPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);

            RHI::CommandList* commandList = context.GetCommandList();

            const RHI::DrawListView drawListViewPartition = RHI::GetDrawListPartition(m_drawListView, context.GetCommandListIndex(), context.GetCommandListCount());

            if (!drawListViewPartition.empty())
            {
                commandList->SetViewport(m_viewportState);
                commandList->SetScissor(m_scissorState);
                SetSrgsForDraw(commandList);
            }

            for (const RHI::DrawItemProperties& drawItemProperties : drawListViewPartition)
            {
                if (drawItemProperties.m_drawFilterMask & m_pipeline->GetDrawFilterMask())
                {
                    commandList->Submit(*drawItemProperties.m_item);
                }
            }
        }

    }   // namespace RPI
}   // namespace AZ
