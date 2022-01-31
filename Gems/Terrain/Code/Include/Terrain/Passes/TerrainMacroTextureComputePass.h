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
    
    struct TerrainMacroTextureComputePassData
        : public AZ::RPI::ComputePassData
    {
        AZ_RTTI(Terrain::TerrainMacroTextureComputePassData, "{BB11DACF-AF47-402D-92C6-33C644F6F530}", AZ::RPI::ComputePassData);
        AZ_CLASS_ALLOCATOR(Terrain::TerrainMacroTextureComputePassData, AZ::SystemAllocator, 0);

        TerrainMacroTextureComputePassData() = default;
        virtual ~TerrainMacroTextureComputePassData() = default;

        static void Reflect(AZ::ReflectContext* context);
    };

    class TerrainMacroTextureComputePass
        : public AZ::RPI::ComputePass
    {
        AZ_RPI_PASS(TerrainMacroTextureComputePass);

    public:
        AZ_RTTI(Terrain::TerrainMacroTextureComputePass, "{E493C3D2-D657-49ED-A5B1-A29B2995F6A8}", AZ::RPI::ComputePass);
        AZ_CLASS_ALLOCATOR(Terrain::TerrainMacroTextureComputePass, AZ::SystemAllocator, 0);
        virtual ~TerrainMacroTextureComputePass() = default;

        static AZ::RPI::Ptr<TerrainMacroTextureComputePass> Create(const AZ::RPI::PassDescriptor& descriptor);

        void SetFeatureProcessor();

        void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;
    private:
        TerrainMacroTextureComputePass(const AZ::RPI::PassDescriptor& descriptor);

        void BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context) override;

        TerrainFeatureProcessor* m_terrainFeatureProcessor;
    };
} // namespace AZ::Render
