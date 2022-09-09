/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/Passes/TerrainClipmapDebugPass.h>
#include <TerrainRenderer/TerrainFeatureProcessor.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <AzCore/Console/Console.h>

namespace Terrain
{
    AZ_CVAR(
        bool,
        r_terrainClipmapDebugEnable,
        false,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "Turn on clipmap debug rendering on the screen.");

    AZ::RPI::Ptr<TerrainClipmapDebugPass> TerrainClipmapDebugPass::Create(const AZ::RPI::PassDescriptor& descriptor)
    {
        AZ::RPI::Ptr<TerrainClipmapDebugPass> pass = aznew TerrainClipmapDebugPass(descriptor);
        return pass;
    }

    TerrainClipmapDebugPass::TerrainClipmapDebugPass(const AZ::RPI::PassDescriptor& descriptor)
        : AZ::RPI::FullscreenTrianglePass(descriptor)
    {
    }

    void TerrainClipmapDebugPass::SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph)
    {
        AZ::RPI::RenderPass::SetupFrameGraphDependencies(frameGraph);
    }

    void TerrainClipmapDebugPass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
    {
        AZ::RPI::Scene* scene = m_pipeline->GetScene();
        TerrainFeatureProcessor* terrainFeatureProcessor = scene->GetFeatureProcessor<TerrainFeatureProcessor>();
        if (terrainFeatureProcessor)
        {
            auto terrainSrg = terrainFeatureProcessor->GetTerrainShaderResourceGroup();
            if (terrainSrg)
            {
                BindSrg(terrainFeatureProcessor->GetTerrainShaderResourceGroup()->GetRHIShaderResourceGroup());
            }
        }

        AZ::RPI::FullscreenTrianglePass::CompileResources(context);
    }

    bool TerrainClipmapDebugPass::IsEnabled() const
    {
        if (!AZ::RPI::Pass::IsEnabled())
        {
            return false;
        }

        AZ::RPI::Scene* scene = m_pipeline->GetScene();
        TerrainFeatureProcessor* terrainFeatureProcessor = scene->GetFeatureProcessor<TerrainFeatureProcessor>();
        if (!terrainFeatureProcessor)
        {
            return false;
        }

        const TerrainClipmapManager& clipmapManager = terrainFeatureProcessor->GetClipmapManager();

        if (!clipmapManager.IsClipmapEnabled())
        {
            return false;
        }

        return r_terrainClipmapDebugEnable;
    }
}
