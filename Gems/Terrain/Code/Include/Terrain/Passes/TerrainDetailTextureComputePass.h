/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Reflect/Pass/ComputePassData.h>
#include <Atom/RPI.Reflect/Pass/PassDescriptor.h>

namespace Terrain
{
    class TerrainFeatureProcessor;

    struct TerrainDetailTextureComputePassData
        : public AZ::RPI::ComputePassData
    {
        AZ_RTTI(Terrain::TerrainDetailTextureComputePassData, "{387F7457-16E5-4AA6-8D96-56ED4532CA8D}", AZ::RPI::ComputePassData);
        AZ_CLASS_ALLOCATOR(Terrain::TerrainDetailTextureComputePassData, AZ::SystemAllocator, 0);

        TerrainDetailTextureComputePassData() = default;
        virtual ~TerrainDetailTextureComputePassData() = default;

        static void Reflect(AZ::ReflectContext* context);
    };

    class TerrainDetailTextureComputePass
        : public AZ::RPI::ComputePass
    {
        AZ_RPI_PASS(TerrainDetailTextureComputePass);

    public:
        AZ_RTTI(Terrain::TerrainDetailTextureComputePass, "{69A8207B-3311-4BB1-BD4E-A08B5E0424B5}", AZ::RPI::ComputePass);
        AZ_CLASS_ALLOCATOR(Terrain::TerrainDetailTextureComputePass, AZ::SystemAllocator, 0);
        virtual ~TerrainDetailTextureComputePass() = default;

        static AZ::RPI::Ptr<TerrainDetailTextureComputePass> Create(const AZ::RPI::PassDescriptor& descriptor);

        void SetFeatureProcessor();

        void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;
    private:
        TerrainDetailTextureComputePass(const AZ::RPI::PassDescriptor& descriptor);

        void BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context) override;

        TerrainFeatureProcessor* m_terrainFeatureProcessor;
    };
} // namespace AZ::Render
