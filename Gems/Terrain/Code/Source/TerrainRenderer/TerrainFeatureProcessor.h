/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <TerrainRenderer/TerrainMacroMaterialBus.h>
#include <TerrainRenderer/TerrainAreaMaterialRequestBus.h>
#include <TerrainRenderer/TerrainMeshManager.h>

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/MeshDrawPacket.h>
#include <Atom/RPI.Public/Material/MaterialReloadNotificationBus.h>
#include <Atom/RPI.Public/Shader/ShaderSystemInterface.h>
#include <Atom/Feature/Utils/GpuBufferHandler.h>
#include <Atom/Feature/Utils/IndexedDataVector.h>
#include <Atom/Feature/Utils/SparseVector.h>

namespace AZ::RPI
{
    namespace AssetUtils
    {
        class AsyncAssetLoader;
    }
    class Material;
    class Model;
    class RenderPass;
    class StreamingImage;
}

namespace Terrain
{
    class TerrainFeatureProcessor final
        : public AZ::RPI::FeatureProcessor
        , private AZ::RPI::MaterialReloadNotificationBus::Handler
        , private AzFramework::Terrain::TerrainDataNotificationBus::Handler
        , private TerrainMacroMaterialNotificationBus::Handler
        , private TerrainAreaMaterialNotificationBus::Handler
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

        void SetWorldSize(AZ::Vector2 sizeInMeters);

    private:
        
        using MaterialInstance = AZ::Data::Instance<AZ::RPI::Material>;
        
        static constexpr uint16_t InvalidImageIndex = 0xFFFF;

        enum MacroMaterialShaderFlags : uint32_t
        {
            IsUsed           = 0b0000'0000'0000'0000'0000'0000'0000'0001,
            FlipMacroNormalX = 0b0000'0000'0000'0000'0000'0000'0000'0010,
            FlipMacroNormalY = 0b0000'0000'0000'0000'0000'0000'0000'0100,
        };

        enum DetailTextureFlags : uint32_t
        {
            UseTextureBaseColor =  0b0000'0000'0000'0000'0000'0000'0000'0001,
            UseTextureNormal =     0b0000'0000'0000'0000'0000'0000'0000'0010,
            UseTextureMetallic =   0b0000'0000'0000'0000'0000'0000'0000'0100,
            UseTextureRoughness =  0b0000'0000'0000'0000'0000'0000'0000'1000,
            UseTextureOcclusion =  0b0000'0000'0000'0000'0000'0000'0001'0000,
            UseTextureHeight =     0b0000'0000'0000'0000'0000'0000'0010'0000,
            UseTextureSpecularF0 = 0b0000'0000'0000'0000'0000'0000'0100'0000,

            FlipNormalX =          0b0000'0000'0000'0001'0000'0000'0000'0000,
            FlipNormalY =          0b0000'0000'0000'0010'0000'0000'0000'0000,

            BlendModeMask =        0b0000'0000'0000'1100'0000'0000'0000'0000,
            BlendModeLerp =        0b0000'0000'0000'0000'0000'0000'0000'0000,
            BlendModeLinearLight = 0b0000'0000'0000'0100'0000'0000'0000'0000,
            BlendModeMultiply =    0b0000'0000'0000'1000'0000'0000'0000'0000,
            BlendModeOverlay =     0b0000'0000'0000'1100'0000'0000'0000'0000,
        };

        struct WorldShaderData
        {
            AZStd::array<float, 3> m_min{ 0.0f, 0.0f, 0.0f };
            float padding1{ 0.0f };
            AZStd::array<float, 3> m_max{ 0.0f, 0.0f, 0.0f };
            float padding2{ 0.0f };
        };

        struct MacroMaterial
        {
            MacroMaterialData m_data;
            uint16_t m_colorIndex{ 0xFFFF };
            uint16_t m_normalIndex{ 0xFFFF };
        };

        struct MacroMaterialGridShaderData
        {
            uint32_t m_resolution; // How many x/y tiles in grid. x & y stored in 16 bits each. Total number of entries in m_macroMaterialData will be x * y
            float m_tileSize; // Size of a tile in meters.
            AZStd::array<float, 2> m_offset; // x/y offset of min x/y corner of grid.
        };

        struct MacroMaterialShaderData
        {
            MacroMaterialShaderFlags m_flags;
            uint32_t m_colorMapId{InvalidImageIndex};
            uint32_t m_normalMapId{InvalidImageIndex};
            float m_normalFactor;

            // macro material bounds in world space
            AZStd::array<float, 2> m_boundsMin{ 0.0f, 0.0f };
            AZStd::array<float, 2> m_boundsMax{ 0.0f, 0.0f };
        };

        struct DetailMaterialShaderData
        {
            // Uv
            AZStd::array<float, 12> m_uvTransform
            {
                1.0, 0.0, 0.0, 0.0,
                0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
            };

            float m_baseColorRed{ 1.0f };
            float m_baseColorGreen{ 1.0f };
            float m_baseColorBlue{ 1.0f };

            // Factor / Scale / Bias for input textures
            float m_baseColorFactor{ 1.0f };

            float m_normalFactor{ 1.0f };
            float m_metalFactor{ 1.0f };
            float m_roughnessScale{ 1.0f };
            float m_roughnessBias{ 0.0f };

            float m_specularF0Factor{ 1.0f };
            float m_occlusionFactor{ 1.0f };
            float m_heightFactor{ 1.0f };
            float m_heightOffset{ 0.0f };

            float m_heightBlendFactor{ 0.5f };

            // Flags
            DetailTextureFlags m_flags{ 0 };

            // Image indices
            uint16_t m_colorImageIndex{ InvalidImageIndex };
            uint16_t m_normalImageIndex{ InvalidImageIndex };
            uint16_t m_roughnessImageIndex{ InvalidImageIndex };
            uint16_t m_metalnessImageIndex{ InvalidImageIndex };

            uint16_t m_specularF0ImageIndex{ InvalidImageIndex };
            uint16_t m_occlusionImageIndex{ InvalidImageIndex };
            uint16_t m_heightImageIndex{ InvalidImageIndex };

            // 16 byte aligned
            uint16_t m_padding1;
            uint32_t m_padding2;
            uint32_t m_padding3;
        };

        struct DetailMaterialData
        {
            AZ::Data::AssetId m_assetId;
            AZ::RPI::Material::ChangeId m_materialChangeId{AZ::RPI::Material::DEFAULT_CHANGE_ID};
            uint32_t refCount = 0;
            uint16_t m_detailMaterialBufferIndex{ 0xFFFF };

            AZ::Data::Instance<AZ::RPI::Image> m_colorImage;
            AZ::Data::Instance<AZ::RPI::Image> m_normalImage;
            AZ::Data::Instance<AZ::RPI::Image> m_roughnessImage;
            AZ::Data::Instance<AZ::RPI::Image> m_metalnessImage;
            AZ::Data::Instance<AZ::RPI::Image> m_specularF0Image;
            AZ::Data::Instance<AZ::RPI::Image> m_occlusionImage;
            AZ::Data::Instance<AZ::RPI::Image> m_heightImage;
        };

        struct DetailMaterialSurface
        {
            AZ::Crc32 m_surfaceTag;
            uint16_t m_detailMaterialId;
        };

        struct DetailMaterialListRegion
        {
            AZ::EntityId m_entityId;
            AZ::Aabb m_region{AZ::Aabb::CreateNull()};
            AZStd::vector<DetailMaterialSurface> m_materialsForSurfaces;
        };
        
        struct Vector2i
        {
            int32_t m_x{ 0 };
            int32_t m_y{ 0 };

            Vector2i operator+(const Vector2i& rhs) const;
            Vector2i& operator+=(const Vector2i& rhs);
            Vector2i operator-(const Vector2i& rhs) const;
            Vector2i& operator-=(const Vector2i& rhs);
            Vector2i operator-() const;
        };

        struct Aabb2i
        {
            Vector2i m_min;
            Vector2i m_max;

            Aabb2i operator+(const Vector2i& offset) const;
            Aabb2i operator-(const Vector2i& offset) const;

            Aabb2i GetClamped(Aabb2i rhs) const;
            bool IsValid() const;
        };
        
        static constexpr float MacroMaterialGridSize = 64.0f;
        static constexpr uint16_t MacroMaterialsPerTile = 4;

        // AZ::RPI::MaterialReloadNotificationBus::Handler overrides...
        void OnMaterialReinitialized(const MaterialInstance& material) override;

        // AzFramework::Terrain::TerrainDataNotificationBus overrides...
        void OnTerrainDataDestroyBegin() override;
        void OnTerrainDataChanged(const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask) override;

        // TerrainMacroMaterialNotificationBus overrides...
        void OnTerrainMacroMaterialCreated(AZ::EntityId entityId, const MacroMaterialData& material) override;
        void OnTerrainMacroMaterialChanged(AZ::EntityId entityId, const MacroMaterialData& material) override;
        void OnTerrainMacroMaterialRegionChanged(AZ::EntityId entityId, const AZ::Aabb& oldRegion, const AZ::Aabb& newRegion) override;
        void OnTerrainMacroMaterialDestroyed(AZ::EntityId entityId) override;
        
        // TerrainAreaMaterialNotificationBus overrides...
        void OnTerrainSurfaceMaterialMappingCreated(AZ::EntityId entityId, SurfaceData::SurfaceTag surfaceTag, MaterialInstance material) override;
        void OnTerrainSurfaceMaterialMappingDestroyed(AZ::EntityId entityId, SurfaceData::SurfaceTag surfaceTag) override;
        void OnTerrainSurfaceMaterialMappingChanged(AZ::EntityId entityId, SurfaceData::SurfaceTag surfaceTag, MaterialInstance material) override;
        void OnTerrainSurfaceMaterialMappingRegionChanged(AZ::EntityId entityId, const AZ::Aabb& oldRegion, const AZ::Aabb& newRegion) override;

        // AZ::RPI::SceneNotificationBus overrides...
        void OnRenderPipelinePassesChanged(AZ::RPI::RenderPipeline* renderPipeline) override;

        void Initialize();

        void UpdateHeightmapImage();
        void PrepareMaterialData();

        void TerrainHeightOrSettingsUpdated(const AZ::Aabb& dirtyRegion);
        void TerrainSurfaceDataUpdated(const AZ::Aabb& dirtyRegion);

        void UpdateMacroMaterialShaderEntry(uint16_t shaderDataIdx, const MacroMaterial& macroMaterialData);
        void RemoveMacroMaterialShaderEntry(uint16_t shaderDataIdx);

        uint16_t CreateOrUpdateDetailMaterial(MaterialInstance material);
        void CheckDetailMaterialForDeletion(uint16_t detailMaterialId);
        void UpdateDetailMaterialData(uint16_t detailMaterialIndex, MaterialInstance material);
        void CheckUpdateDetailTexture(const Aabb2i& newBounds, const Vector2i& newCenter);
        void UpdateDetailTexture(const Aabb2i& updateArea, const Aabb2i& textureBounds, const Vector2i& centerPixel);
        uint16_t GetDetailMaterialForSurfaceTypeAndPosition(AZ::Crc32 surfaceType, const AZ::Vector2& position);
        uint8_t CalculateUpdateRegions(const Aabb2i& updateArea, const Aabb2i& textureBounds, const Vector2i& centerPixel,
            AZStd::array<Aabb2i, 4>& textureSpaceAreas, AZStd::array<Aabb2i, 4>& scaledWorldSpaceAreas);

        void ProcessSurfaces(const FeatureProcessor::RenderPacket& process);

        template <typename T>
        T* FindByEntityId(AZ::EntityId entityId, AZ::Render::IndexedDataVector<T>& container);
        template <typename T>
        T& FindOrCreateByEntityId(AZ::EntityId entityId, AZ::Render::IndexedDataVector<T>& container);
        template <typename T>
        void RemoveByEntityId(AZ::EntityId entityId, AZ::Render::IndexedDataVector<T>& container);

        void CacheForwardPass();

        template<typename Callback>
        void ForMacroMaterialsInBounds(const AZ::Aabb& bounds, Callback callback);

        uint16_t AppendBindlessImage(const AZ::RHI::ImageView* imageView);
        void UpdateBindlessImage(uint16_t index, const AZ::RHI::ImageView* imageView);
        void RemoveBindlessImage(uint16_t index);

        // System-level parameters
        static constexpr int32_t DetailTextureSize{ 1024 };
        static constexpr int32_t DetailTextureSizeHalf{ DetailTextureSize / 2 };
        static constexpr float DetailTextureScale{ 0.5f };
        
        TerrainMeshManager m_meshManager;

        AZStd::unique_ptr<AZ::RPI::AssetUtils::AsyncAssetLoader> m_materialAssetLoader;
        MaterialInstance m_materialInstance;

        AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> m_terrainSrg;
        AZ::Data::Instance<AZ::RPI::AttachmentImage> m_heightmapImage;
        AZ::Data::Instance<AZ::RPI::AttachmentImage> m_detailTextureImage;

        AZ::RHI::ShaderInputImageIndex m_heightmapPropertyIndex;
        AZ::RHI::ShaderInputConstantIndex m_worldDataIndex;
        AZ::RHI::ShaderInputImageUnboundedArrayIndex m_texturesIndex;
        AZ::RHI::ShaderInputConstantIndex m_macroMaterialGridIndex;
        AZ::RHI::ShaderInputImageIndex m_detailMaterialIdPropertyIndex;
        AZ::RHI::ShaderInputBufferIndex m_detailMaterialDataIndex;
        AZ::RHI::ShaderInputConstantIndex m_detailCenterPropertyIndex;
        AZ::RHI::ShaderInputConstantIndex m_detailAabbPropertyIndex;
        AZ::RHI::ShaderInputConstantIndex m_detailHalfPixelUvPropertyIndex;

        AZ::Vector3 m_previousCameraPosition = AZ::Vector3(AZStd::numeric_limits<float>::max(), 0.0, 0.0);

        AZ::Aabb m_terrainBounds{ AZ::Aabb::CreateNull() };
        AZ::Aabb m_dirtyRegion{ AZ::Aabb::CreateNull() };
        AZ::Aabb m_dirtyDetailRegion{ AZ::Aabb::CreateNull() };
        
        float m_sampleSpacing{ 0.0f };
        
        bool m_heightmapNeedsUpdate{ false };
        bool m_macroMaterialBufferNeedsUpdate{ false };
        bool m_detailMaterialBufferNeedsUpdate{ false };
        bool m_forceRebuildDrawPackets{ false };
        bool m_imageBindingsNeedUpdate{ false };
        bool m_detailImagesNeedUpdate{ false };

        Aabb2i m_detailTextureBounds;
        Vector2i m_detailTextureCenter;
        AZ::RPI::ShaderSystemInterface::GlobalShaderOptionUpdatedEvent::Handler m_handleGlobalShaderOptionUpdate;

        // Macro materials stored in a grid of (MacroMaterialGridCount * MacroMaterialGridCount) where each tile in the grid covers
        // an area of (MacroMaterialGridSize * MacroMaterialGridSize) and each tile can hold MacroMaterialsPerTile macro materials
        AZStd::vector<MacroMaterialShaderData> m_macroMaterialShaderData;
        AZStd::vector<AZ::EntityId> m_macroMaterialEntities; // Same as above, but used to track entity ids which aren't needed by the shader.
        AZStd::map<AZ::EntityId, MacroMaterial> m_macroMaterial; // Used for looking up macro materials by entity id when the data isn't provided by a bus.
        AZ::Render::GpuBufferHandler m_macroMaterialDataBuffer;
        uint16_t m_macroMaterialTilesX{ 0 };
        uint16_t m_macroMaterialTilesY{ 0 };

        AZ::Render::IndexedDataVector<DetailMaterialData> m_detailMaterials;
        AZ::Render::IndexedDataVector<DetailMaterialListRegion> m_detailMaterialRegions;
        AZ::Render::SparseVector<DetailMaterialShaderData> m_detailMaterialShaderData;
        AZ::Render::GpuBufferHandler m_detailMaterialDataBuffer;

        AZ::RPI::RenderPass* m_forwardPass;

        AZStd::vector<const AZ::RHI::ImageView*> m_bindlessImageViews;
        AZStd::vector<uint16_t> m_bindlessImageViewFreeList;
    };
}
