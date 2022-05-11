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
    struct TerrainClipmapGenerationPassData : public AZ::RPI::RenderPassData
    {
        AZ_RTTI(TerrainClipmapGenerationPassData, "{07C90E11-6607-4BD2-B041-96CEF46F8C55}", AZ::RPI::RenderPassData);
        AZ_CLASS_ALLOCATOR(TerrainClipmapGenerationPassData, AZ::SystemAllocator, 0);

        TerrainClipmapGenerationPassData() = default;
        virtual ~TerrainClipmapGenerationPassData() = default;

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<TerrainClipmapGenerationPassData, AZ::RPI::RenderPassData>()->Version(1)->Field(
                    "ShaderAsset", &TerrainClipmapGenerationPassData::m_shaderReference);
            }
        }

        AZ::RPI::AssetReference m_shaderReference;
    };

    class TerrainClipmapGenerationPass
        : public AZ::RPI::RenderPass
        , private AZ::RPI::ShaderReloadNotificationBus::Handler
    {
        AZ_RPI_PASS(TerrainClipmapGenerationPass);

    public:
        AZ_RTTI(Terrain::TerrainClipmapGenerationPass, "{EA713973-1214-498C-BA05-A9A8B1AA99C7}", AZ::RPI::RenderPass);
        AZ_CLASS_ALLOCATOR(Terrain::TerrainClipmapGenerationPass, AZ::SystemAllocator, 0);
        virtual ~TerrainClipmapGenerationPass() = default;

        void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;
        void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;
        void BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context) override;

    protected:
        TerrainClipmapGenerationPass(const AZ::RPI::PassDescriptor& descriptor);

        void OnShaderReinitialized(const AZ::RPI::Shader& shader) override;
        void OnShaderAssetReinitialized(const AZ::Data::Asset<AZ::RPI::ShaderAsset>& shaderAsset) override;
        void OnShaderVariantReinitialized(const AZ::RPI::ShaderVariant& shaderVariant) override;

        void LoadShader();

        // Default draw SRG for using the shader option system's variant fallback key
        AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> m_drawSrg = nullptr;

        // The draw item submitted by this pass
        AZ::RHI::DispatchItem m_dispatchItem;

        AZ::RPI::PassDescriptor m_passDescriptor;

        // The compute shader that will be used by the pass
        AZ::Data::Instance<AZ::RPI::Shader> m_shader = nullptr;
    };

    class TerrainMacroClipmapGenerationPass
        : public TerrainClipmapGenerationPass
    {
        AZ_RPI_PASS(TerrainMacroClipmapGenerationPass);

    public:
        AZ_RTTI(Terrain::TerrainMacroClipmapGenerationPass, "{E1F7C18F-E77A-496E-ABD7-1EC7D75AA4B0}", TerrainClipmapGenerationPass);
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
        : public TerrainClipmapGenerationPass
    {
        AZ_RPI_PASS(TerrainDetailClipmapGenerationPass);

    public:
        AZ_RTTI(Terrain::TerrainDetailClipmapGenerationPass, "{BD504E93-87F4-484E-A17A-E337C3F2279C}", TerrainClipmapGenerationPass);
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
