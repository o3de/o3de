/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Terrain/Passes/TerrainDetailTextureComputePass.h>
#include <TerrainRenderer/TerrainFeatureProcessor.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>

namespace Terrain
{
    void TerrainDetailTextureComputePassData::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Terrain::TerrainDetailTextureComputePassData, AZ::RPI::ComputePassData>()
                ->Version(1);
        }
    }

    AZ::RPI::Ptr<TerrainDetailTextureComputePass> TerrainDetailTextureComputePass::Create(const AZ::RPI::PassDescriptor& descriptor)
    {
        AZ::RPI::Ptr<TerrainDetailTextureComputePass> pass = aznew TerrainDetailTextureComputePass(descriptor);
        return pass;
    }

    TerrainDetailTextureComputePass::TerrainDetailTextureComputePass(const AZ::RPI::PassDescriptor& descriptor)
        : AZ::RPI::ComputePass(descriptor)
    {
        const TerrainDetailTextureComputePass* passData = AZ::RPI::PassUtils::GetPassData<TerrainDetailTextureComputePass>(descriptor);
        if (passData)
        {
            // Copy data to pass

        }
    }

    void TerrainDetailTextureComputePass::BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context)
    {
        ComputePass::BuildCommandListInternal(context);
    }

    void TerrainDetailTextureComputePass::SetFeatureProcessor()
    {
        m_terrainFeatureProcessor = GetRenderPipeline()->GetScene()->GetFeatureProcessor<TerrainFeatureProcessor>();
    }

    void TerrainDetailTextureComputePass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
    {
        ComputePass::CompileResources(context);
    }

} // namespace Terrain
