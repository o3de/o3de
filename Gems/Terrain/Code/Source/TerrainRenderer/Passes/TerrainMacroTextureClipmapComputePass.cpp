/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Terrain/Passes/TerrainMacroTextureClipmapComputePass.h>
#include <TerrainRenderer/TerrainFeatureProcessor.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

namespace Terrain
{
    AZ::RPI::Ptr<TerrainMacroTextureClipmapComputePass> TerrainMacroTextureClipmapComputePass::Create(
        const AZ::RPI::PassDescriptor& descriptor)
    {
        AZ::RPI::Ptr<TerrainMacroTextureClipmapComputePass> pass = aznew TerrainMacroTextureClipmapComputePass(descriptor);
        return pass;
    }

    TerrainMacroTextureClipmapComputePass::TerrainMacroTextureClipmapComputePass(const AZ::RPI::PassDescriptor& descriptor)
        : AZ::RPI::ParentPass(descriptor)
    {
    }

    
    void TerrainMacroTextureClipmapComputePass::BuildInternal()
    {
        BuildChildPasses();
        AZ::RPI::ParentPass::BuildInternal();
    }

    void TerrainMacroTextureClipmapComputePass::BuildChildPasses()
    {

    }

    AZ::RPI::Ptr<TerrainMacroTextureClipmapGenerationPass> TerrainMacroTextureClipmapGenerationPass::Create(
        const AZ::RPI::PassDescriptor& descriptor)
    {
        AZ::RPI::Ptr<TerrainMacroTextureClipmapGenerationPass> pass = aznew TerrainMacroTextureClipmapGenerationPass(descriptor);
        return pass;
    }

    TerrainMacroTextureClipmapGenerationPass::TerrainMacroTextureClipmapGenerationPass(const AZ::RPI::PassDescriptor& descriptor)
        : AZ::RPI::ComputePass(descriptor)
    {
    }

    void TerrainMacroTextureClipmapGenerationPass::InitializeInternal()
    {
        m_currentViewPositionIndex.Reset();

        ComputePass::InitializeInternal();
    }

    void TerrainMacroTextureClipmapGenerationPass::FrameBeginInternal(FramePrepareParams params)
    {
        AZ::RPI::Scene* scene = m_pipeline->GetScene();
        TerrainFeatureProcessor* terrainFeatureProcessor = scene->GetFeatureProcessor<TerrainFeatureProcessor>();
        if (terrainFeatureProcessor)
        {
            const AZ::Aabb worldBounds = terrainFeatureProcessor->GetTerrainBounds();
            m_shaderResourceGroup->SetConstant(m_worldBoundsMinIndex, worldBounds.GetMin());
            m_shaderResourceGroup->SetConstant(m_worldBoundsMaxIndex, worldBounds.GetMax());
        }
        else
        {
            m_shaderResourceGroup->SetConstant(m_worldBoundsMinIndex, AZ::Vector3(0.0, 0.0, 0.0));
            m_shaderResourceGroup->SetConstant(m_worldBoundsMaxIndex, AZ::Vector3(0.0, 0.0, 0.0));
        }

        AZ::RPI::ViewPtr view = GetView();
        AZ_Assert(view, "TerrainMacroTextureClipmapGenerationPass should have the MainCamera as the view.");

        m_previousViewPosition = m_currentViewPosition;
        m_currentViewPosition = view->GetViewToWorldMatrix().GetTranslation();

        m_shaderResourceGroup->SetConstant(m_currentViewPositionIndex, m_previousViewPosition);
        m_shaderResourceGroup->SetConstant(m_previousViewPositionIndex, m_previousViewPosition);

        ComputePass::FrameBeginInternal(params);
    }

    void TerrainMacroTextureClipmapGenerationPass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
    {
        AZ::RPI::Scene* scene = m_pipeline->GetScene();
        TerrainFeatureProcessor* terrainFeatureProcessor = scene->GetFeatureProcessor<TerrainFeatureProcessor>();
        if (terrainFeatureProcessor)
        {
            auto terrainSrg = terrainFeatureProcessor->GetTerrainShaderResourceGroup();
            if (terrainSrg)
            {
                if (!terrainSrg->IsQueuedForCompile())
                {
                    terrainSrg->Compile();
                }
                BindSrg(terrainFeatureProcessor->GetTerrainShaderResourceGroup()->GetRHIShaderResourceGroup());
            }

            auto material = terrainFeatureProcessor->GetMaterial();
            if (material)
            {
                BindSrg(material->GetRHIShaderResourceGroup());
            }
        }

        ComputePass::CompileResources(context);
    }

} // namespace Terrain
