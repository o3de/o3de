/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Terrain/Pass/ClipmapGenerationPassData.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>

namespace Terrain
{
    void ClipmapGenerationPassData::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Terrain::ClipmapGenerationPassData, AZ::RPI::ComputePassData>()
                ->Version(1);
        }
    }

    AZ::RPI::Ptr<ClipmapGenerationPass> ClipmapGenerationPass::Create(const AZ::RPI::PassDescriptor& descriptor)
    {
        AZ::RPI::Ptr<ClipmapGenerationPass> pass = aznew ClipmapGenerationPass(descriptor);
        return pass;
    }

    ClipmapGenerationPass::ClipmapGenerationPass(const AZ::RPI::PassDescriptor& descriptor)
        : AZ::RPI::ComputePass(descriptor)
    {
        const ClipmapGenerationPassData* clipmapPassData = AZ::RPI::PassUtils::GetPassData<ClipmapGenerationPassData>(descriptor);
        if (clipmapPassData)
        {
            // Copy data to pass

        }
    }

    void ClipmapGenerationPass::BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context)
    {
        ComputePass::BuildCommandListInternal(context);
    }

    void ClipmapGenerationPass::SetFeatureProcessor(TerrainFeatureProcessor* terrainFeatureProcessor)
    {
        m_terrainFeatureProcessor = terrainFeatureProcessor;
    }

    void ClipmapGenerationPass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
    {
        ComputePass::CompileResources(context);
    }

} // namespace Terrain
