/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>

#include <TerrainRenderer/Passes/TerrainClipmapComputePass.h>
#include <TerrainRenderer/TerrainDetailMaterialManager.h>
#include <TerrainRenderer/TerrainMacroMaterialManager.h>
#include <TerrainRenderer/TerrainClipmapManager.h>
#include <TerrainRenderer/TerrainMeshManager.h>

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>

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
        , public AZ::Data::AssetBus::Handler
        , private AzFramework::Terrain::TerrainDataNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainFeatureProcessor, AZ::SystemAllocator)
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
        void SetClipmapConfiguration(const ClipmapConfiguration& config);

        const AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> GetTerrainShaderResourceGroup() const;
        const AZ::Data::Instance<AZ::RPI::Material> GetMaterial() const;
        const TerrainClipmapManager& GetClipmapManager() const;
    private:

        using MaterialInstance = AZ::Data::Instance<AZ::RPI::Material>;
        
        struct WorldShaderData
        {
            float m_zMin;
            float m_zMax;
            float m_zExtents;
            float m_padding;
        };

        //! AZ::Data::AssetBus overrides...
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        // AzFramework::Terrain::TerrainDataNotificationBus overrides...
        void OnTerrainDataDestroyBegin() override;
        void OnTerrainDataChanged(const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask) override;

        // AZ::RPI::SceneNotificationBus overrides...
        void OnRenderPipelineChanged(AZ::RPI::RenderPipeline* pipeline, AZ::RPI::SceneNotification::RenderPipelineChangeType changeType) override;
        
        // AZ::RPI::FeatureProcessor overrides...
        void AddRenderPasses(AZ::RPI::RenderPipeline* renderPipeline) override;

        void Initialize();

        void PrepareMaterialData();

        void ProcessSurfaces(const FeatureProcessor::RenderPacket& process);

        void CachePasses();

        TerrainMeshManager m_meshManager;
        TerrainMacroMaterialManager m_macroMaterialManager;
        TerrainDetailMaterialManager m_detailMaterialManager;
        TerrainClipmapManager m_clipmapManager;

        AZ::Data::Asset<AZ::RPI::MaterialAsset> m_materialAsset;
        MaterialInstance m_materialInstance;

        AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> m_terrainSrg;

        AZ::RHI::ShaderInputNameIndex m_worldDataIndex = "m_terrainWorldData";

        AzFramework::Terrain::FloatRange m_zBounds;
        AZ::Aabb m_dirtyRegion{ AZ::Aabb::CreateNull() };
        
        bool m_terrainBoundsNeedUpdate{ false };

        AZStd::vector<AZ::RPI::RenderPass*> m_passes;
    };
}
