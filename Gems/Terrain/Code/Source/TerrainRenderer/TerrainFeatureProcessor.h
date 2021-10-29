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

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/MeshDrawPacket.h>
#include <Atom/RPI.Public/Material/MaterialReloadNotificationBus.h>
#include <Atom/RPI.Public/Shader/ShaderSystemInterface.h>
#include <Atom/Feature/Utils/IndexedDataVector.h>

namespace AZ::RPI
{
    namespace AssetUtils
    {
        class AsyncAssetLoader;
    }
    class Material;
    class Model;
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
        static constexpr uint32_t MaxMaterialsPerSector = 4;

        enum MacroMaterialFlags
        {
            ColorImageUsed = 0b01,
            NormalImageUsed = 0b10,
        };

        struct ShaderTerrainData // Must align with struct in Object Srg
        {
            AZStd::array<float, 2> m_uvMin;
            AZStd::array<float, 2> m_uvMax;
            AZStd::array<float, 2> m_uvStep;
            float m_sampleSpacing;
            float m_heightScale;
        };

        struct ShaderMacroMaterialData
        {
            AZStd::array<float, 2> m_uvMin;
            AZStd::array<float, 2> m_uvMax;
            float m_normalFactor;
            uint32_t m_flipNormalX{ 0 }; // bool in shader
            uint32_t m_flipNormalY{ 0 }; // bool in shader
            uint32_t m_mapsInUse{ 0b00 }; // 0b01 = color, 0b10 = normal
        };

        struct VertexPosition
        {
            float m_posx;
            float m_posy;
        };

        struct VertexUv
        {
            float m_u;
            float m_v;
        };

        struct PatchData
        {
            AZStd::vector<VertexPosition> m_positions;
            AZStd::vector<VertexUv> m_uvs;
            AZStd::vector<uint16_t> m_indices;
        };
        
        struct SectorData
        {
            AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> m_srg; // Hold on to ref so it's not dropped
            AZ::Aabb m_aabb;
            AZStd::fixed_vector<AZ::RPI::MeshDrawPacket, AZ::RPI::ModelLodAsset::LodCountMax> m_drawPackets;
            AZStd::fixed_vector<uint16_t, MaxMaterialsPerSector> m_macroMaterials;
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

            FlipNormalX =          0b0000'0000'0000'0000'0000'0000'1000'0000,
            FlipNormalY =          0b0000'0000'0000'0000'0000'0001'0000'0000,

            BlendModeMask =        0b0000'0000'0000'0000'0000'0110'0000'0000,
            BlendModeLerp =        0b0000'0000'0000'0000'0000'0000'0000'0000,
            BlendModeLinearLight = 0b0000'0000'0000'0000'0000'0010'0000'0000,
            BlendModeMultiply =    0b0000'0000'0000'0000'0000'0100'0000'0000,
            BlendModeOverlay =     0b0000'0000'0000'0000'0000'0110'0000'0000,
        };

        struct DetailMaterialShaderProperties
        {
            // Uv
            AZStd::array<float, 12> m_uvTransform
            {
                1.0, 0.0, 0.0, 0.0,
                0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
            };

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

            float m_padding; // 16 byte aligned
        };

        struct DetailMaterialData
        {
            AZ::Data::AssetId m_assetId;
            AZ::RPI::Material::ChangeId m_materialChangeId{AZ::RPI::Material::DEFAULT_CHANGE_ID};

            AZ::Data::Instance<AZ::RPI::Image> m_colorImage;
            AZ::Data::Instance<AZ::RPI::Image> m_normalImage;
            AZ::Data::Instance<AZ::RPI::Image> m_roughnessImage;
            AZ::Data::Instance<AZ::RPI::Image> m_metalnessImage;
            AZ::Data::Instance<AZ::RPI::Image> m_specularF0Image;
            AZ::Data::Instance<AZ::RPI::Image> m_occlusionImage;
            AZ::Data::Instance<AZ::RPI::Image> m_heightImage;

            DetailMaterialShaderProperties m_properties; // maps directly to shader
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

        void Initialize();
        void InitializeTerrainPatch(uint16_t gridSize, float gridSpacing, PatchData& patchdata);
        bool InitializePatchModel();

        void UpdateTerrainData();
        void PrepareMaterialData();
        void UpdateMacroMaterialData(MacroMaterialData& macroMaterialData, const MacroMaterialData& newMaterialData);
        
        void TerrainHeightOrSettingsUpdated(const AZ::Aabb& dirtyRegion);
        void TerrainSurfaceDataUpdated(const AZ::Aabb& dirtyRegion);

        uint16_t CreateOrUpdateDetailMaterial(MaterialInstance material);
        void UpdateDetailMaterialData(DetailMaterialData& materialData, MaterialInstance material);
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

        template<typename Callback>
        void ForOverlappingSectors(const AZ::Aabb& bounds, Callback callback);

        AZ::Outcome<AZ::Data::Asset<AZ::RPI::BufferAsset>> CreateBufferAsset(
            const void* data, const AZ::RHI::BufferViewDescriptor& bufferViewDescriptor, const AZStd::string& bufferName);

        // System-level parameters
        static constexpr float GridSpacing{ 1.0f };
        static constexpr int32_t GridSize{ 64 }; // number of terrain quads (vertices are m_gridSize + 1)
        static constexpr float GridMeters{ GridSpacing * GridSize };
        static constexpr int32_t DetailTextureSize{ 1024 };
        static constexpr int32_t DetailTextureSizeHalf{ DetailTextureSize / 2 };
        static constexpr float DetailTextureScale{ 0.5f };

        AZStd::unique_ptr<AZ::RPI::AssetUtils::AsyncAssetLoader> m_materialAssetLoader;
        MaterialInstance m_materialInstance;

        AZ::RHI::ShaderInputConstantIndex m_modelToWorldIndex;
        AZ::RHI::ShaderInputConstantIndex m_terrainDataIndex;
        AZ::RHI::ShaderInputConstantIndex m_macroMaterialDataIndex;
        AZ::RHI::ShaderInputConstantIndex m_macroMaterialCountIndex;
        AZ::RHI::ShaderInputImageIndex m_macroColorMapIndex;
        AZ::RHI::ShaderInputImageIndex m_macroNormalMapIndex;
        AZ::RPI::MaterialPropertyIndex m_heightmapPropertyIndex;
        AZ::RPI::MaterialPropertyIndex m_detailMaterialIdPropertyIndex;
        AZ::RPI::MaterialPropertyIndex m_detailCenterPropertyIndex;
        AZ::RPI::MaterialPropertyIndex m_detailAabbPropertyIndex;
        AZ::RPI::MaterialPropertyIndex m_detailHalfPixelUvPropertyIndex;

        AZ::Data::Instance<AZ::RPI::Model> m_patchModel;
        AZ::Vector3 m_previousCameraPosition = AZ::Vector3(AZStd::numeric_limits<float>::max(), 0.0, 0.0);

        // Per-area data
        struct TerrainAreaData
        {
            AZ::Transform m_transform{ AZ::Transform::CreateIdentity() };
            AZ::Aabb m_terrainBounds{ AZ::Aabb::CreateNull() };
            AZ::Data::Instance<AZ::RPI::AttachmentImage> m_heightmapImage;
            float m_sampleSpacing{ 0.0f };
            bool m_heightmapUpdated{ true };
            bool m_macroMaterialsUpdated{ true };
            bool m_rebuildSectors{ true };
        };
        
        TerrainAreaData m_areaData;
        AZ::Aabb m_dirtyRegion{ AZ::Aabb::CreateNull() };
        AZ::Aabb m_dirtyDetailRegion{ AZ::Aabb::CreateNull() };

        Aabb2i m_detailTextureBounds;
        Vector2i m_detailTextureCenter;
        AZ::Data::Instance<AZ::RPI::AttachmentImage> m_detailTextureImage;
        AZ::RPI::ShaderSystemInterface::GlobalShaderOptionUpdatedEvent::Handler m_handleGlobalShaderOptionUpdate;
        bool m_forceRebuildDrawPackets = false;

        AZStd::vector<SectorData> m_sectorData;

        AZ::Render::IndexedDataVector<MacroMaterialData> m_macroMaterials;
        AZ::Render::IndexedDataVector<DetailMaterialData> m_detailMaterials;
        AZ::Render::IndexedDataVector<DetailMaterialListRegion> m_detailMaterialRegions;
    };
}
