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
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <TerrainRenderer/ClipmapBounds.h>

namespace Terrain
{
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
        float m_detailClipmapMaxRenderRadius = 512.0f;

        //! Max resolution of the clipmap stack.
        //! The actual max resolution may be bigger due to rounding.
        //! Resolution in: texels per meter.
        float m_macroClipmapMaxResolution = 2.0f;
        float m_detailClipmapMaxResolution = 256.0f;

        //! The scale base between two adjacent clipmap layers.
        //! For example, 3 means the (n+1)th clipmap covers 3^2 = 9 times
        //! to what is covered by the nth clipmap.
        float m_macroClipmapScaleBase = 2.0f;
        float m_detailClipmapScaleBase = 3.0f;

        //! Calculate how many layers of clipmap is needed.
        //! Final result must be less or equal than the MacroClipmapStackSizeMax/DetailClipmapStackSizeMax.
        uint32_t CalculateMacroClipmapStackSize() const;
        uint32_t CalculateDetailClipmapStackSize() const;
    };

    class TerrainClipmapManager
    {
        friend class TerrainClipmapGenerationPass;
    public:
        AZ_RTTI(TerrainClipmapManager, "{5892AEE6-F3FA-4DFC-BBEC-77E1B91506A2}");
        AZ_DISABLE_COPY_MOVE(TerrainClipmapManager);

        TerrainClipmapManager();
        virtual ~TerrainClipmapManager() = default;

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

        // Shader input names
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

        void Update(const AZ::Vector3& cameraPosition, AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg);

        // Import the clipmap to the frame graph and set scope attachment access,
        // so that the compute pass can build dependencies accordingly.
        void ImportClipmap(ClipmapName clipmapName, AZ::RHI::FrameGraphAttachmentInterface attachmentDatabase) const;
        void UseClipmap(ClipmapName clipmapName, AZ::RHI::ScopeAttachmentAccess access, AZ::RHI::FrameGraphInterface frameGraph) const;

        AZ::Data::Instance<AZ::RPI::AttachmentImage> GetClipmapImage(ClipmapName clipmapName) const;
    private:
        void UpdateClipmapData(const AZ::Vector3& cameraPosition);
        void InitializeClipmapBounds();
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
            float m_clipmapSize;

            uint32_t m_padding;

            //! Clipmap centers in normalized UV coordinates [0, 1].
            //! 0,1: previous clipmap centers; 2,3: current clipmap centers.
            //! They are used for toroidal addressing and may move each frame based on the view point movement.
            //! The move distance is scaled differently in each layer.
            AZStd::array<AZStd::array<float, 4>, ClipmapConfiguration::MacroClipmapStackSizeMax> m_macroClipmapCenters;
            AZStd::array<AZStd::array<float, 4>, ClipmapConfiguration::DetailClipmapStackSizeMax> m_detailClipmapCenters;

            //! A list of reciprocal the clipmap scale [s],
            //! where 1 pixel in the current layer of clipmap represents s meters.
            //! Fast lookup list to avoid redundant calculation in shaders.
            //! x: macro; y: detail
            AZStd::array<AZStd::array<float, 4>, AZStd::max(ClipmapConfiguration::MacroClipmapStackSizeMax, ClipmapConfiguration::DetailClipmapStackSizeMax)> m_clipmapScaleInv;
        };

        ClipmapData m_clipmapData;

        AZStd::vector<ClipmapBounds> m_macroClipmapBounds;
        AZStd::vector<ClipmapBounds> m_detailClipmapBounds;

        AZ::RHI::ShaderInputNameIndex m_terrainSrgClipmapDataIndex = ClipmapDataShaderInput;
        AZ::RHI::ShaderInputNameIndex m_terrainSrgClipmapImageIndex[ClipmapName::Count];

        AZ::Data::Instance<AZ::RPI::AttachmentImage> m_clipmaps[ClipmapName::Count];

        uint32_t m_macroClipmapStackSize;
        uint32_t m_detailClipmapStackSize;

        ClipmapConfiguration m_config;
        bool m_isInitialized = false;
    };
}
