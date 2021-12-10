/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/std/containers/array.h>

#include <AzFramework/Terrain/TerrainDataRequestBus.h>

#include <TerrainRenderer/Aabb2i.h>
#include <TerrainRenderer/BindlessImageArrayHandler.h>
#include <TerrainRenderer/TerrainAreaMaterialRequestBus.h>
#include <TerrainRenderer/Vector2i.h>

#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Reflect/Image/Image.h>
#include <Atom/Feature/Utils/IndexedDataVector.h>
#include <Atom/Feature/Utils/SparseVector.h>
#include <Atom/Feature/Utils/GpuBufferHandler.h>

namespace Terrain
{
    class TerrainDetailMaterialManager
        : private AzFramework::Terrain::TerrainDataNotificationBus::Handler
        , private TerrainAreaMaterialNotificationBus::Handler
    {
    public:

        AZ_RTTI(TerrainDetailMaterialManager, "{3CBAF88F-E3B1-43B8-97A5-999133188BCC}");
        AZ_DISABLE_COPY_MOVE(TerrainDetailMaterialManager);

        TerrainDetailMaterialManager() = default;
        ~TerrainDetailMaterialManager() = default;
        
        void Initialize(
            const AZStd::shared_ptr<AZ::Render::BindlessImageArrayHandler>& bindlessImageHandler,
            AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg);
        bool IsInitialized() const;
        void Reset();
        bool UpdateSrgIndices(AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& srg);

        void Update(const AZ::Vector3& cameraPosition, AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg);

    private:
        
        using MaterialInstance = AZ::Data::Instance<AZ::RPI::Material>;
        static constexpr auto InvalidImageIndex = AZ::Render::BindlessImageArrayHandler::InvalidImageIndex;

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
        
        // System-level parameters
        static constexpr int32_t DetailTextureSize{ 1024 };
        static constexpr int32_t DetailTextureSizeHalf{ DetailTextureSize / 2 };
        static constexpr float DetailTextureScale{ 0.5f };
        
        // AzFramework::Terrain::TerrainDataNotificationBus overrides...
        void OnTerrainDataChanged(const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask) override;
        
        // TerrainAreaMaterialNotificationBus overrides...
        void OnTerrainSurfaceMaterialMappingCreated(AZ::EntityId entityId, SurfaceData::SurfaceTag surfaceTag, MaterialInstance material) override;
        void OnTerrainSurfaceMaterialMappingDestroyed(AZ::EntityId entityId, SurfaceData::SurfaceTag surfaceTag) override;
        void OnTerrainSurfaceMaterialMappingChanged(AZ::EntityId entityId, SurfaceData::SurfaceTag surfaceTag, MaterialInstance material) override;
        void OnTerrainSurfaceMaterialMappingRegionChanged(AZ::EntityId entityId, const AZ::Aabb& oldRegion, const AZ::Aabb& newRegion) override;

        void RemoveAllImages();

        uint16_t CreateOrUpdateDetailMaterial(MaterialInstance material);
        void CheckDetailMaterialForDeletion(uint16_t detailMaterialId);
        void UpdateDetailMaterialData(uint16_t detailMaterialIndex, MaterialInstance material);
        void CheckUpdateDetailTexture(const Aabb2i& newBounds, const Vector2i& newCenter);
        void UpdateDetailTexture(const Aabb2i& updateArea, const Aabb2i& textureBounds, const Vector2i& centerPixel);
        uint16_t GetDetailMaterialForSurfaceTypeAndPosition(AZ::Crc32 surfaceType, const AZ::Vector2& position);
        uint8_t CalculateUpdateRegions(const Aabb2i& updateArea, const Aabb2i& textureBounds, const Vector2i& centerPixel,
            AZStd::array<Aabb2i, 4>& textureSpaceAreas, AZStd::array<Aabb2i, 4>& scaledWorldSpaceAreas);

        DetailMaterialListRegion* FindByEntityId(AZ::EntityId entityId, AZ::Render::IndexedDataVector<DetailMaterialListRegion>& container);
        DetailMaterialListRegion& FindOrCreateByEntityId(AZ::EntityId entityId, AZ::Render::IndexedDataVector<DetailMaterialListRegion>& container);
        void RemoveByEntityId(AZ::EntityId entityId, AZ::Render::IndexedDataVector<DetailMaterialListRegion>& container);
        
        AZStd::shared_ptr<AZ::Render::BindlessImageArrayHandler> m_bindlessImageHandler;
        
        AZ::Data::Instance<AZ::RPI::AttachmentImage> m_detailTextureImage;

        AZ::Render::IndexedDataVector<DetailMaterialData> m_detailMaterials;
        AZ::Render::IndexedDataVector<DetailMaterialListRegion> m_detailMaterialRegions;
        AZ::Render::SparseVector<DetailMaterialShaderData> m_detailMaterialShaderData;
        AZ::Render::GpuBufferHandler m_detailMaterialDataBuffer;
        
        AZ::Aabb m_dirtyDetailRegion{ AZ::Aabb::CreateNull() };
        AZ::Vector3 m_previousCameraPosition = AZ::Vector3(AZStd::numeric_limits<float>::max(), 0.0, 0.0);
        Aabb2i m_detailTextureBounds;
        Vector2i m_detailTextureCenter;

        AZ::RHI::ShaderInputImageIndex m_detailMaterialIdPropertyIndex;
        AZ::RHI::ShaderInputBufferIndex m_detailMaterialDataIndex;
        AZ::RHI::ShaderInputConstantIndex m_detailCenterPropertyIndex;
        AZ::RHI::ShaderInputConstantIndex m_detailAabbPropertyIndex;
        AZ::RHI::ShaderInputConstantIndex m_detailHalfPixelUvPropertyIndex;

        bool m_isInitialized{ false };
        bool m_detailMaterialBufferNeedsUpdate{ false };
        bool m_detailImageNeedsUpdate{ false };

    };
}
