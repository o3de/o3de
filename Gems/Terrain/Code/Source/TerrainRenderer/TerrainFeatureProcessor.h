/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Terrain/TerrainDataRequestBus.h>

#include <TerrainRenderer/BindlessImageArrayHandler.h>
#include <TerrainRenderer/TerrainDetailMaterialManager.h>
#include <TerrainRenderer/TerrainMacroMaterialManager.h>
#include <TerrainRenderer/TerrainClipmapManager.h>
#include <TerrainRenderer/TerrainMeshManager.h>

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/Material/MaterialReloadNotificationBus.h>

namespace AZ::RPI
{
    namespace AssetUtils
    {
        class AsyncAssetLoader;
    }
    class Material;
    class RenderPass;
}

namespace Terrain
{
    class TerrainFeatureProcessor final
        : public AZ::RPI::FeatureProcessor
        , private AZ::RPI::MaterialReloadNotificationBus::Handler
        , private AzFramework::Terrain::TerrainDataNotificationBus::Handler
    {
    public:
        AZ_RTTI(TerrainFeatureProcessor, "{D7DAC1F9-4A9F-4D3C-80AE-99579BF8AB1C}", AZ::RPI::FeatureProcessor);
        AZ_DISABLE_COPY_MOVE(TerrainFeatureProcessor);
        AZ_FEATURE_PROCESSOR(TerrainFeatureProcessor);

        static void Reflect(AZ::ReflectContext* context);

        TerrainFeatureProcessor() = default;
        ~TerrainFeatureProcessor() = default;

        // AZ::RPI::FeatureProcessor overrides...
        void Activate() override;
        void Deactivate() override;
        void Render(const AZ::RPI::FeatureProcessor::RenderPacket& packet) override;

        void SetDetailMaterialConfiguration(const DetailMaterialConfiguration& config);
        void SetMeshConfiguration(const MeshConfiguration& config);

        const AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> GetTerrainShaderResourceGroup() const;
        const AZ::Data::Instance<AZ::RPI::Material> GetMaterial() const;
        const TerrainClipmapManager& GetClipmapManager() const;
    private:

        static constexpr auto InvalidImageIndex = AZ::Render::BindlessImageArrayHandler::InvalidImageIndex;
        using MaterialInstance = AZ::Data::Instance<AZ::RPI::Material>;
        
        struct WorldShaderData
        {
            float m_zMin;
            float m_zMax;
            float m_zExtents;
            float m_padding;
        };

        // AZ::RPI::MaterialReloadNotificationBus::Handler overrides...
        void OnMaterialReinitialized(const MaterialInstance& material) override;

        // AzFramework::Terrain::TerrainDataNotificationBus overrides...
        void OnTerrainDataDestroyBegin() override;
        void OnTerrainDataChanged(const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask) override;

        // AZ::RPI::SceneNotificationBus overrides...
        void OnRenderPipelineAdded(AZ::RPI::RenderPipelinePtr pipeline) override;
        void OnRenderPipelinePassesChanged(AZ::RPI::RenderPipeline* renderPipeline) override;
        
        // AZ::RPI::FeatureProcessor overrides...
        void ApplyRenderPipelineChange(AZ::RPI::RenderPipeline* renderPipeline) override;

        void Initialize();

        void PrepareMaterialData();

        void TerrainHeightOrSettingsUpdated(const AZ::Aabb& dirtyRegion);

        void ProcessSurfaces(const FeatureProcessor::RenderPacket& process);

        void CachePasses();

        TerrainMeshManager m_meshManager;
        TerrainMacroMaterialManager m_macroMaterialManager;
        TerrainDetailMaterialManager m_detailMaterialManager;
        TerrainClipmapManager m_clipmapManager;

        AZStd::shared_ptr<AZ::Render::BindlessImageArrayHandler> m_imageArrayHandler;

        AZStd::unique_ptr<AZ::RPI::AssetUtils::AsyncAssetLoader> m_materialAssetLoader;
        MaterialInstance m_materialInstance;

        AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> m_terrainSrg;

        AZ::RHI::ShaderInputNameIndex m_worldDataIndex = "m_terrainWorldData";

        AZ::Vector2 m_zBounds{ AZ::Vector2::CreateZero() };
        AZ::Aabb m_dirtyRegion{ AZ::Aabb::CreateNull() };
        
        float m_sampleSpacing{ 0.0f };
        bool m_terrainBoundsNeedUpdate{ false };

        AZStd::vector<AZ::RPI::RenderPass*> m_passes;
    };
}
