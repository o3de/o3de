/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/array.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <Atom/Feature/Utils/GpuBufferHandler.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <TerrainRenderer/ClipmapBounds.h>
#include <TerrainRenderer/TerrainAreaMaterialRequestBus.h>
#include <TerrainRenderer/TerrainMacroMaterialBus.h>

namespace Terrain
{
    //! Clipmap configuration to set basic properties of the clipmaps.
    //! Derived properties will be automatically calculated based on the given data, including:
    //! Precision of each clipmap level, depth of the clipmap stack.
    struct ClipmapConfiguration
    {
        AZ_CLASS_ALLOCATOR(ClipmapConfiguration, AZ::SystemAllocator);
        AZ_RTTI(ClipmapConfiguration, "{5CC8A81E-B850-46BA-A577-E21530D9ED04}");

        ClipmapConfiguration() = default;
        virtual ~ClipmapConfiguration() = default;

        //! Max clipmap number that can have. Used to initialize fixed arrays.
        static constexpr uint32_t MacroClipmapStackSizeMax = 16;
        static constexpr uint32_t DetailClipmapStackSizeMax = 16;
        static constexpr uint32_t SharedClipmapStackSizeMax = AZStd::max(ClipmapConfiguration::MacroClipmapStackSizeMax, ClipmapConfiguration::DetailClipmapStackSizeMax);

        enum ClipmapSize : uint32_t
        {
            ClipmapSize512 = 512u,
            ClipmapSize1024 = 1024u,
            ClipmapSize2048 = 2048u
        };

        //! Enable clipmap feature for rendering
        bool m_clipmapEnabled = false;

        //! The size of the clipmap image in each layer.
        ClipmapSize m_clipmapSize = ClipmapSize1024;

        //! Max render radius that the lowest resolution clipmap can cover.
        //! Radius in: meters.
        //! For example, 1000 means the clipmaps render 1000 meters at most from the view position.
        float m_macroClipmapMaxRenderRadius = 2048.0f;
        float m_detailClipmapMaxRenderRadius = 256.0f;

        //! The resolution of the highest resolution clipmap in the stack.
        //! The actual max resolution may be higher due to matching max render radius.
        //! Resolution in: texels per meter.
        float m_macroClipmapMaxResolution = 2.0f;
        float m_detailClipmapMaxResolution = 2048.0f;

        //! The scale base between two adjacent clipmap layers.
        //! For example, 3 means the (n+1)th clipmap covers 3^2 = 9 times
        //! to what is covered by the nth clipmap.
        float m_macroClipmapScaleBase = 2.0f;
        float m_detailClipmapScaleBase = 2.0f;

        //! The margin of the clipmap where the data won't be used, so that bigger margin results in less frequent update.
        //! But bigger margin also means it tends to use lower resolution clipmap.
        //! Size in: texels.
        uint32_t m_macroClipmapMarginSize = 4;
        uint32_t m_detailClipmapMarginSize = 4;
        //! In addition to the above margin size used for updating,
        //! this margin is a safety margin to avoid edge cases when blending or sampling.
        //! For example, due to toroidal addressing, 2 physical adjacent texels can locate
        //! on the opposite edges in the clipmap in logical space. They may be accidentally blended
        //! by float rounding errors. Size in texels.
        uint32_t m_extendedClipmapMarginSize = 2;

        //! The size of the blending area between each clipmap level.
        //! Size in texels.
        uint32_t m_clipmapBlendSize = 256;

        //! Calculate how many layers of clipmap is needed.
        //! Final result must be less or equal than the MacroClipmapStackSizeMax/DetailClipmapStackSizeMax.
        uint32_t CalculateMacroClipmapStackSize() const;
        uint32_t CalculateDetailClipmapStackSize() const;
    };

    //! This class manages all terrain clipmaps' creation and update.
    //! It should be owned by the TerrianFeature Processor and provide data and images
    //! for the ClipmapGenerationPass and terrain forward rendering pass.
    class TerrainClipmapManager
        : private TerrainMacroMaterialNotificationBus::Handler
        , private TerrainAreaMaterialNotificationBus::Handler
        , private AzFramework::Terrain::TerrainDataNotificationBus::Handler
    {
        friend class TerrainClipmapGenerationPass;
    public:
        AZ_RTTI(TerrainClipmapManager, "{5892AEE6-F3FA-4DFC-BBEC-77E1B91506A2}");
        AZ_DISABLE_COPY_MOVE(TerrainClipmapManager);

        TerrainClipmapManager();
        virtual ~TerrainClipmapManager() = default;

        //! Name for each clipmap image.
        enum ClipmapName : uint32_t
        {
            MacroColor = 0,
            MacroNormal,
            DetailColor,
            DetailNormal,
            DetailHeight,
            DetailRoughness,
            DetailSpecularF0,
            DetailMetalness,
            DetailOcclusion,

            Count
        };

        //! Shader input names
        static constexpr const char* ClipmapDataShaderInput = "m_clipmapData";
        static constexpr const char* ClipmapImageShaderInput[ClipmapName::Count] = {
            "m_macroColorClipmaps",
            "m_macroNormalClipmaps",
            "m_detailColorClipmaps",
            "m_detailNormalClipmaps",
            "m_detailHeightClipmaps",
            "m_detailRoughnessClipmaps",
            "m_detailSpecularF0Clipmaps",
            "m_detailMetalnessClipmaps",
            "m_detailOcclusionClipmaps",
        };

        void SetConfiguration(const ClipmapConfiguration& config);
        bool IsClipmapEnabled() const;

        void Initialize(AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg);
        bool IsInitialized() const;
        void Reset();
        //! Reset full refresh flag so that all the clipmap image will be repainted.
        void TriggerFullRefresh();
        void UpdateSrgIndices(AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& srg);

        void Update(const AZ::Vector3& cameraPosition, const AZ::RPI::Scene* scene, AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg);

        //! Import the clipmap to the frame graph and set scope attachment access,
        //! so that the compute pass can build dependencies accordingly.
        void ImportClipmap(ClipmapName clipmapName, AZ::RHI::FrameGraphAttachmentInterface attachmentDatabase) const;
        void UseClipmap(ClipmapName clipmapName, AZ::RHI::ScopeAttachmentAccess access, AZ::RHI::FrameGraphInterface frameGraph) const;

        //! Get the clipmap images for passes using them.
        AZ::Data::Instance<AZ::RPI::AttachmentImage> GetClipmapImage(ClipmapName clipmapName) const;

        //! Get the dispatch thread num for the clipmap compute shader based on the current frame.
        void GetMacroDispatchThreadNum(uint32_t& outThreadX, uint32_t& outThreadY, uint32_t& outThreadZ) const;
        void GetDetailDispatchThreadNum(uint32_t& outThreadX, uint32_t& outThreadY, uint32_t& outThreadZ) const;

        //! Get the size of the clipmap from config.
        uint32_t GetClipmapSize() const;

        //! Returns if there are clipmap regions requiring update.
        bool HasMacroClipmapUpdate() const;
        bool HasDetailClipmapUpdate() const;
    private:
        //! Update the C++ copy of the clipmap data. And will later be bound to the terrain SRG.
        void UpdateClipmapData(
            const AZ::Vector3& cameraPosition,
            const AZ::RPI::Scene* scene,
            AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg);

        //! Initialzation functions.
        void QueryMacroClipmapStackSize();
        void QueryDetailClipmapStackSize();
        void InitializeMacroClipmapBounds(const AZ::Vector2& center);
        void InitializeMacroClipmapData();
        void InitializeMacroClipmapImages();
        void InitializeMacroClipmapGpuBuffer();
        void InitializeDetailClipmapBounds(const AZ::Vector2& center);
        void InitializeDetailClipmapData();
        void InitializeDetailClipmapImages();
        void InitializeDetailClipmapGpuBuffer();

        //! Clear functions.
        void ClearMacroClipmapImages();
        void ClearMacroClipmapGpuBuffer();
        void ClearDetailClipmapImages();
        void ClearDetailClipmapGpuBuffer();

        //! Cached terrain SRG for configuration update.
        AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> m_terrainSrg;

        using RawVector2f = AZStd::array<float, 2>;
        using RawVector4f = AZStd::array<float, 4>;
        using RawVector2u = AZStd::array<uint32_t, 2>;
        using RawVector4u = AZStd::array<uint32_t, 4>;

        //! Data to be passed to shaders
        struct alignas(16) ClipmapData
        {
            //! Current viewport size.
            RawVector2f m_viewportSize;

            //! The max range that the clipmap is covering.
            float m_macroClipmapMaxRenderRadius;
            float m_detailClipmapMaxRenderRadius;

            //! The scale base between two adjacent clipmap layers.
            //! For example, 3 means the (n+1)th clipmap covers 3^2 = 9 times
            //! to what is covered by the nth clipmap.
            float m_macroClipmapScaleBase;
            float m_detailClipmapScaleBase;

            //! Size of the clipmap stack.
            uint32_t m_macroClipmapStackSize;
            uint32_t m_detailClipmapStackSize;

            //! The margin size of the edge of the clipmap where the data won't be used.
            //! Use float to reduce frequent casting in shaders.
            float m_macroClipmapMarginSize;
            float m_detailClipmapMarginSize;

            //! In addition to the above margin size used for updating,
            //! this margin is a safety margin to avoid edge cases when blending or sampling.
            float m_extendedClipmapMarginSize;

            //! The size of the clipmap image in each layer.
            //! Given 2 copies in different types to save casting.
            float m_clipmapSizeFloat;
            uint32_t m_clipmapSizeUint;

            //! The texel position where blending to the next level should start. Equivalent to:
            //! m_clipmapSizeFloat / 2.0 - m_macroClipmapMarginSize - m_extendedClipmapMarginSize
            //! Cached for frequent access.
            float m_validMacroClipmapRadius;
            //! Same as above, equivalent to:
            //! m_clipmapSizeFloat / 2.0 - m_detailClipmapMarginSize - m_extendedClipmapMarginSize
            float m_validDetailClipmapRadius;

            //! The size of the blending area between each clipmap level.
            float m_clipmapBlendSize;

            //! The number of regions to be updated during the current frame.
            uint32_t m_macroClipmapUpdateRegionCount = 0;
            uint32_t m_detailClipmapUpdateRegionCount = 0;

            //! Numbers match the compute shader invoking call dispatch(X, Y, 1).
            uint32_t m_macroDispatchGroupCountX = 1;
            uint32_t m_macroDispatchGroupCountY = 1;
            uint32_t m_detailDispatchGroupCountX = 1;
            uint32_t m_detailDispatchGroupCountY = 1;

            //! Debug data
            //! Enables debug overlay to indicate clipmap levels.
            float m_macroClipmapOverlayFactor = 0.0f;
            float m_detailClipmapOverlayFactor = 0.0f;

            // 0: macro color clipmap
            // 1: macro normal clipmap
            // 2: detail color clipmap
            // 3: detail normal clipmap
            // 4: detail height clipmap
            // 5: detail roughness clipmap
            // 6: detail specularF0 clipmap
            // 7: detail metalness clipmap
            // 8: detail occlusion clipmap
            uint32_t m_debugClipmapId = 0;

            // Which clipmap level to sample from, or texture array index.
            float m_debugClipmapLevel = 0; // cast to float in CPU

            // How big the clipmap should appear on the screen.
            float m_debugScale = 0.5f;

            // Multiplier adjustment for final color output.
            float m_debugBrightness = 1.0f;

            //! Clipmap centers in texel coordinates ranging [0, size).
            //! Clipmap centers are the logical center of the texture, based on toroidal addressing.
            struct ClipmapCenter
            {
                RawVector2u m_macro;
                RawVector2u m_detail;
            };
            AZStd::array<ClipmapCenter, ClipmapConfiguration::SharedClipmapStackSizeMax> m_clipmapCenters;

            //! Clipmap centers in world coordinates.
            struct ClipmapWorldCenter
            {
                RawVector2f m_macro;
                RawVector2f m_detail;
            };
            AZStd::array<ClipmapWorldCenter, ClipmapConfiguration::SharedClipmapStackSizeMax> m_clipmapWorldCenters;

            //! A scale converting the length from the texture space to the world space.
            //! For example: given texel (u0, v0) and (u1, v1), dtexel = sqrt((u0 - u1)^2, (v0 - v1)^2)
            //!              dworld = dtexel * clipmapToWorldScale.
            struct ClipmapToWorldScale
            {
                float m_macro;
                float m_detail;
                RawVector2f m_padding;
            };
            AZStd::array<ClipmapToWorldScale, ClipmapConfiguration::SharedClipmapStackSizeMax> m_clipmapToWorldScale;
        };

        ClipmapData m_clipmapData;

        //! They describe how clipmaps are initilized and updated.
        //! Data will be gathered from them when camera moves.
        AZStd::vector<ClipmapBounds> m_macroClipmapBounds;
        AZStd::vector<ClipmapBounds> m_detailClipmapBounds;

        //! GPU buffer containing the region aabbs to be updated during this frame.
        AZ::Render::GpuBufferHandler m_macroClipmapUpdateRegionsBuffer;
        AZ::Render::GpuBufferHandler m_detailClipmapUpdateRegionsBuffer;

        struct ClipmapUpdateRegion
        {
            RawVector4u m_updateRegion;
            uint32_t m_clipmapLevel;
            // 16 bytes alignment padding
            AZStd::array<uint32_t, 3> m_padding;

            ClipmapUpdateRegion(uint32_t clipmapLevel, RawVector4u updateRegion)
                : m_clipmapLevel(clipmapLevel)
                , m_updateRegion(updateRegion)
            {
            }
        };
        AZStd::vector<ClipmapUpdateRegion> m_macroClipmapUpdateRegions;
        AZStd::vector<ClipmapUpdateRegion> m_detailClipmapUpdateRegions;

        //! Terrain SRG input.
        AZ::RHI::ShaderInputNameIndex m_terrainSrgClipmapDataIndex = ClipmapDataShaderInput;
        AZ::RHI::ShaderInputNameIndex m_terrainSrgClipmapImageIndex[ClipmapName::Count];

        //! Clipmap images.
        AZ::Data::Instance<AZ::RPI::AttachmentImage> m_clipmaps[ClipmapName::Count];

        //! The actual stack size calculated from the configuration.
        uint32_t m_macroClipmapStackSize;
        uint32_t m_detailClipmapStackSize;

        //! Clipmap config that sets basic properties of the clipmaps.
        ClipmapConfiguration m_config;

        //! Flag used to refresh the class and prevent doule initialization.
        bool m_isInitialized = false;
        //! Flag to generate the full clipmap in situation such as first frame and material update.
        bool m_fullRefreshClipmaps = true;

        //! Dispatch threads for the compute pass.
        uint32_t m_macroTotalDispatchThreadX = 0;
        uint32_t m_macroTotalDispatchThreadY = 0;
        uint32_t m_detailTotalDispatchThreadX = 0;
        uint32_t m_detailTotalDispatchThreadY = 0;

        //! Group threads defined in the compute shader.
        static constexpr uint32_t MacroGroupThreadX = 8;
        static constexpr uint32_t MacroGroupThreadY = 8;
        static constexpr uint32_t DetailGroupThreadX = 8;
        static constexpr uint32_t DetailGroupThreadY = 8;

        // AzFramework::Terrain::TerrainDataNotificationBus overrides...
        void OnTerrainDataChanged(const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask) override;

        // TerrainMacroMaterialNotificationBus overrides...
        void OnTerrainMacroMaterialCreated(AZ::EntityId entityId, const MacroMaterialData& material) override;
        void OnTerrainMacroMaterialChanged(AZ::EntityId entityId, const MacroMaterialData& material) override;
        void OnTerrainMacroMaterialRegionChanged(AZ::EntityId entityId, const AZ::Aabb& oldRegion, const AZ::Aabb& newRegion) override;
        void OnTerrainMacroMaterialDestroyed(AZ::EntityId entityId) override;

        // TerrainAreaMaterialNotificationBus overrides...
        void OnTerrainDefaultSurfaceMaterialCreated(AZ::EntityId entityId, AZ::Data::Instance<AZ::RPI::Material> material) override;
        void OnTerrainDefaultSurfaceMaterialDestroyed(AZ::EntityId entityId) override;
        void OnTerrainDefaultSurfaceMaterialChanged(AZ::EntityId entityId, AZ::Data::Instance<AZ::RPI::Material> newMaterial) override;
        void OnTerrainSurfaceMaterialMappingCreated(AZ::EntityId entityId, SurfaceData::SurfaceTag surfaceTag, AZ::Data::Instance<AZ::RPI::Material> material) override;
        void OnTerrainSurfaceMaterialMappingDestroyed(AZ::EntityId entityId, SurfaceData::SurfaceTag surfaceTag) override;
        void OnTerrainSurfaceMaterialMappingMaterialChanged(AZ::EntityId entityId, SurfaceData::SurfaceTag surfaceTag, AZ::Data::Instance<AZ::RPI::Material> material) override;
        void OnTerrainSurfaceMaterialMappingTagChanged(AZ::EntityId entityId, SurfaceData::SurfaceTag oldSurfaceTag, SurfaceData::SurfaceTag newSurfaceTag) override;
        void OnTerrainSurfaceMaterialMappingRegionCreated(AZ::EntityId entityId, const AZ::Aabb& region) override;
        void OnTerrainSurfaceMaterialMappingRegionDestroyed(AZ::EntityId entityId, const AZ::Aabb& oldRegion) override;
        void OnTerrainSurfaceMaterialMappingRegionChanged(AZ::EntityId entityId, const AZ::Aabb& oldRegion, const AZ::Aabb& newRegion) override;
    };
}
