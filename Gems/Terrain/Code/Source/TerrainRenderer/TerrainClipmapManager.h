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
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <TerrainRenderer/ClipmapBounds.h>

namespace Terrain
{
    //! Clipmap configuration to set basic properties of the clipmaps.
    //! Derived properties will be automatically calculated based on the given data, including:
    //! Precision of each clipmap level, depth of the clipmap stack.
    struct ClipmapConfiguration
    {
        AZ_CLASS_ALLOCATOR(ClipmapConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(ClipmapConfiguration, "{5CC8A81E-B850-46BA-A577-E21530D9ED04}");

        ClipmapConfiguration() = default;
        virtual ~ClipmapConfiguration() = default;

        //! Max clipmap number that can have. Used to initialize fixed arrays.
        static constexpr uint32_t MacroClipmapStackSizeMax = 16;
        static constexpr uint32_t DetailClipmapStackSizeMax = 16;

        //! The size of the clipmap image in each layer.
        uint32_t m_clipmapSize = 1024u;

        //! Max render radius that the lowest resolution clipmap can cover.
        //! Radius in: meters.
        float m_macroClipmapMaxRenderRadius = 2048.0f;
        float m_detailClipmapMaxRenderRadius = 256.0f;

        //! Max resolution of the clipmap stack.
        //! The actual max resolution may be bigger due to rounding.
        //! Resolution in: texels per meter.
        float m_macroClipmapMaxResolution = 2.0f;
        float m_detailClipmapMaxResolution = 64.0f;

        //! The scale base between two adjacent clipmap layers.
        //! For example, 3 means the (n+1)th clipmap covers 3^2 = 9 times
        //! to what is covered by the nth clipmap.
        float m_macroClipmapScaleBase = 2.0f;
        float m_detailClipmapScaleBase = 2.0f;

        //! Calculate how many layers of clipmap is needed.
        //! Final result must be less or equal than the MacroClipmapStackSizeMax/DetailClipmapStackSizeMax.
        uint32_t CalculateMacroClipmapStackSize() const;
        uint32_t CalculateDetailClipmapStackSize() const;
    };

    //! This class manages all terrain clipmaps' creation and update.
    //! It should be owned by the TerrianFeature Processor and provide data and images
    //! for the ClipmapGenerationPass and terrain forward rendering pass.
    class TerrainClipmapManager
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

        void Initialize(AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg);
        bool IsInitialized() const;
        void Reset();
        bool UpdateSrgIndices(AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& srg);

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
        void UpdateClipmapData(const AZ::Vector3& cameraPosition, const AZ::RPI::Scene* scene);

        //! Initialzation functions.
        void InitializeClipmapBounds(const AZ::Vector2& center);
        void InitializeClipmapData();
        void InitializeClipmapImages();

        //! Data to be passed to shaders
        struct ClipmapData
        {
            //! The 2D xy-plane view position where the main camera is.
            AZStd::array<float, 2> m_previousViewPosition;
            AZStd::array<float, 2> m_currentViewPosition;

            // 2D xy-plane world bounds defined by the terrain.
            AZStd::array<float, 2> m_worldBoundsMin;
            AZStd::array<float, 2> m_worldBoundsMax;

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

            //! The size of the clipmap image in each layer.
            //! Given 2 copies in different types to save casting.
            float m_clipmapSizeFloat;
            uint32_t m_clipmapSizeUint;

            //! Clipmap centers in texel coordinates ranging [0, size).
            //! 0,1: previous clipmap centers; 2,3: current clipmap centers.
            //! They are used for toroidal addressing and may move each frame based on the view point movement.
            //! The move distance is scaled differently in each layer.
            AZStd::array<AZStd::array<uint32_t, 4>, ClipmapConfiguration::MacroClipmapStackSizeMax> m_macroClipmapCenters;
            AZStd::array<AZStd::array<uint32_t, 4>, ClipmapConfiguration::DetailClipmapStackSizeMax> m_detailClipmapCenters;

            //! A list of reciprocal the clipmap scale [s],
            //! where 1 pixel in the current layer of clipmap represents s meters.
            //! Fast lookup list to avoid redundant calculation in shaders.
            //! x: macro; y: detail
            AZStd::array<AZStd::array<float, 4>, AZStd::max(ClipmapConfiguration::MacroClipmapStackSizeMax, ClipmapConfiguration::DetailClipmapStackSizeMax)> m_clipmapScaleInv;

            //! The region of the clipmap that needs update.
            //! Each clipmap can have 0-6 regions to update each frame.
            AZStd::array<AZStd::array<uint32_t, 4>, ClipmapConfiguration::MacroClipmapStackSizeMax * ClipmapBounds::MaxUpdateRegions> m_macroClipmapBoundsRegions;
            AZStd::array<AZStd::array<uint32_t, 4>, ClipmapConfiguration::DetailClipmapStackSizeMax * ClipmapBounds::MaxUpdateRegions> m_detailClipmapBoundsRegions;

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
            uint32_t m_debugClipmapId;

            // Which clipmap level to sample from, or texture array index.
            float m_debugClipmapLevel; // cast to float in CPU

            // Current viewport size.
            AZStd::array<float, 2> m_viewportSize;

            // How big the clipmap should appear on the screen.
            float m_debugScale;

            // Multiplier adjustment for final color output.
            float m_debugBrightness;
        };

        ClipmapData m_clipmapData;

        //! They describe how clipmaps are initilized and updated.
        //! Data will be gathered from them when camera moves.
        AZStd::vector<ClipmapBounds> m_macroClipmapBounds;
        AZStd::vector<ClipmapBounds> m_detailClipmapBounds;

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
        bool m_firstTimeUpdate = true;

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
    };
}
