/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Reflect/Pass/ComputePassData.h>
#include <Atom/RPI.Reflect/Pass/PassDescriptor.h>
#include <TerrainRenderer/TerrainClipmapManager.h>

namespace Terrain
{
    class TerrainMacroClipmapGenerationPass
        : public AZ::RPI::ComputePass
    {
        AZ_RPI_PASS(TerrainMacroClipmapGenerationPass);

    public:
        AZ_RTTI(Terrain::TerrainMacroClipmapGenerationPass, "{E1F7C18F-E77A-496E-ABD7-1EC7D75AA4B0}", AZ::RPI::ComputePass);
        AZ_CLASS_ALLOCATOR(Terrain::TerrainMacroClipmapGenerationPass, AZ::SystemAllocator, 0);
        virtual ~TerrainMacroClipmapGenerationPass() = default;

        static AZ::RPI::Ptr<TerrainMacroClipmapGenerationPass> Create(const AZ::RPI::PassDescriptor& descriptor);

        void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;
        void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;

    private:
        TerrainMacroClipmapGenerationPass(const AZ::RPI::PassDescriptor& descriptor);

        AZ::RHI::ShaderInputNameIndex m_macroColorClipmapsIndex =
            TerrainClipmapManager::ClipmapImageShaderInput[TerrainClipmapManager::ClipmapName::MacroColor];
        AZ::RHI::ShaderInputNameIndex m_macroNormalClipmapsIndex =
            TerrainClipmapManager::ClipmapImageShaderInput[TerrainClipmapManager::ClipmapName::MacroNormal];

        bool m_needsUpdate = true;
    };

    class TerrainDetailClipmapGenerationPass
        : public AZ::RPI::ComputePass
    {
        AZ_RPI_PASS(TerrainDetailClipmapGenerationPass);

    public:
        AZ_RTTI(Terrain::TerrainDetailClipmapGenerationPass, "{BD504E93-87F4-484E-A17A-E337C3F2279C}", AZ::RPI::ComputePass);
        AZ_CLASS_ALLOCATOR(Terrain::TerrainDetailClipmapGenerationPass, AZ::SystemAllocator, 0);
        virtual ~TerrainDetailClipmapGenerationPass() = default;

        static AZ::RPI::Ptr<TerrainDetailClipmapGenerationPass> Create(const AZ::RPI::PassDescriptor& descriptor);

        void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;
        void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;

    private:
        TerrainDetailClipmapGenerationPass(const AZ::RPI::PassDescriptor& descriptor);

        AZ::RHI::ShaderInputNameIndex m_clipmapImageIndex[TerrainClipmapManager::ClipmapName::Count];

        bool m_needsUpdate = true;
    };
} // namespace Terrain
