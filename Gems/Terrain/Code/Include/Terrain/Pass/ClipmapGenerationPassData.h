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

    //! Custom data for the Clipmap Generation Pass.
    struct ClipmapGenerationPassData
        : public AZ::RPI::ComputePassData
    {
        AZ_RTTI(Terrain::ClipmapGenerationPassData, "{387F7457-16E5-4AA6-8D96-56ED4532CA8D}", AZ::RPI::ComputePassData);
        AZ_CLASS_ALLOCATOR(Terrain::ClipmapGenerationPassData, AZ::SystemAllocator, 0);

        ClipmapGenerationPassData() = default;
        virtual ~ClipmapGenerationPassData() = default;

        static void Reflect(AZ::ReflectContext* context);
    };

    class ClipmapGenerationPass
        : public AZ::RPI::ComputePass
    {
        AZ_RPI_PASS(ClipmapGenerationPass);

    public:
        AZ_RTTI(Terrain::ClipmapGenerationPass, "{69A8207B-3311-4BB1-BD4E-A08B5E0424B5}", AZ::RPI::ComputePass);
        AZ_CLASS_ALLOCATOR(Terrain::ClipmapGenerationPass, AZ::SystemAllocator, 0);
        virtual ~ClipmapGenerationPass() = default;

        static AZ::RPI::Ptr<ClipmapGenerationPass> Create(const AZ::RPI::PassDescriptor& descriptor);

        void SetFeatureProcessor(TerrainFeatureProcessor* terrainFeatureProcessor);

        void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;
    private:
        ClipmapGenerationPass(const AZ::RPI::PassDescriptor& descriptor);

        void BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context) override;

        TerrainFeatureProcessor* m_terrainFeatureProcessor;
    };
} // namespace AZ::Render
