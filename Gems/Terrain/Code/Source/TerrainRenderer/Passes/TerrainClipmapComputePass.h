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
#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus.h>
#include <Atom/RPI.Reflect/Pass/ComputePassData.h>
#include <Atom/RPI.Reflect/Pass/PassDescriptor.h>
#include <TerrainRenderer/TerrainClipmapManager.h>

#include <Atom/RPI.Reflect/Pass/RenderPassData.h>

namespace Terrain
{
    //! The compute pass to generate macro texture clipmaps.
    //! DetailClipmapGenerationPass has images depending on this pass.
    //! It will gather all the data from the macro materials into a clipmap stack.
    class TerrainMacroClipmapGenerationPass
        : public AZ::RPI::ComputePass
    {
        AZ_RPI_PASS(TerrainMacroClipmapGenerationPass);

    public:
        AZ_RTTI(Terrain::TerrainMacroClipmapGenerationPass, "{E1F7C18F-E77A-496E-ABD7-1EC7D75AA4B0}", AZ::RPI::ComputePass);
        AZ_CLASS_ALLOCATOR(Terrain::TerrainMacroClipmapGenerationPass, AZ::SystemAllocator);
        virtual ~TerrainMacroClipmapGenerationPass() = default;

        static AZ::RPI::Ptr<TerrainMacroClipmapGenerationPass> Create(const AZ::RPI::PassDescriptor& descriptor);

        void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;
        void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;

        //! Besides the standard enable flag,
        //! the pass can be disabled by the case that no update is triggered.
        bool IsEnabled() const override;
    private:
        TerrainMacroClipmapGenerationPass(const AZ::RPI::PassDescriptor& descriptor);

        //! Macro clipmap only contains color and normal. Bound as RW.
        AZ::RHI::ShaderInputNameIndex m_macroColorClipmapsIndex =
            TerrainClipmapManager::ClipmapImageShaderInput[TerrainClipmapManager::ClipmapName::MacroColor];
        AZ::RHI::ShaderInputNameIndex m_macroNormalClipmapsIndex =
            TerrainClipmapManager::ClipmapImageShaderInput[TerrainClipmapManager::ClipmapName::MacroNormal];

        //! Flag to rebind clipmap images.
        bool m_needsUpdate = true;
    };

    //! The compute pass to generate macro texture clipmaps.
    //! It depends on MacroClipmapGenerationPass the generate macro color clipmaps first.
    //! It will gather all the data from the detail materials into a clipmap stack.
    class TerrainDetailClipmapGenerationPass
        : public AZ::RPI::ComputePass
    {
        AZ_RPI_PASS(TerrainDetailClipmapGenerationPass);

    public:
        AZ_RTTI(Terrain::TerrainDetailClipmapGenerationPass, "{BD504E93-87F4-484E-A17A-E337C3F2279C}", AZ::RPI::ComputePass);
        AZ_CLASS_ALLOCATOR(Terrain::TerrainDetailClipmapGenerationPass, AZ::SystemAllocator);
        virtual ~TerrainDetailClipmapGenerationPass() = default;

        static AZ::RPI::Ptr<TerrainDetailClipmapGenerationPass> Create(const AZ::RPI::PassDescriptor& descriptor);

        void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;
        void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;

        //! Besides the standard enable flag,
        //! the pass can be disabled by the case that no update is triggered.
        bool IsEnabled() const override;
        //! Used to check if clipmap rendering is enabled.
        bool ClipmapFeatureIsEnabled() const;
    private:
        TerrainDetailClipmapGenerationPass(const AZ::RPI::PassDescriptor& descriptor);

        //! It takes in all clipmaps including macro. Macro clipmaps are bound as RO and detail ones RW.
        AZ::RHI::ShaderInputNameIndex m_clipmapImageIndex[TerrainClipmapManager::ClipmapName::Count];

        //! Flag to rebind clipmap images.
        bool m_needsUpdate = true;
    };
} // namespace Terrain
