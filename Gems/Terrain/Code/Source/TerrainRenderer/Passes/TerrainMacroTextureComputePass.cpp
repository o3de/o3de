/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Terrain/Passes/TerrainMacroTextureComputePass.h>
#include <TerrainRenderer/TerrainFeatureProcessor.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>

namespace Terrain
{
    void TerrainMacroTextureComputePassData::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Terrain::TerrainMacroTextureComputePassData, AZ::RPI::ComputePassData>()
                ->Version(1);
        }
    }

    AZ::RPI::Ptr<TerrainMacroTextureComputePass> TerrainMacroTextureComputePass::Create(const AZ::RPI::PassDescriptor& descriptor)
    {
        AZ::RPI::Ptr<TerrainMacroTextureComputePass> pass = aznew TerrainMacroTextureComputePass(descriptor);
        return pass;
    }

    TerrainMacroTextureComputePass::TerrainMacroTextureComputePass(const AZ::RPI::PassDescriptor& descriptor)
        : AZ::RPI::ComputePass(descriptor)
    {
        const TerrainMacroTextureComputePass* passData = AZ::RPI::PassUtils::GetPassData<TerrainMacroTextureComputePass>(descriptor);
        if (passData)
        {
            // Copy data to pass

        }
    }

    void TerrainMacroTextureComputePass::BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context)
    {
        ComputePass::BuildCommandListInternal(context);
    }

    void TerrainMacroTextureComputePass::SetFeatureProcessor()
    {
        m_terrainFeatureProcessor = GetRenderPipeline()->GetScene()->GetFeatureProcessor<TerrainFeatureProcessor>();
    }

    void TerrainMacroTextureComputePass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
    {
        ComputePass::CompileResources(context);
    }

} // namespace Terrain
