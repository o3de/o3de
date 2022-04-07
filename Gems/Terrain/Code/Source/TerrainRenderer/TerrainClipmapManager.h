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

namespace Terrain
{
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
            // Miscellany clipmap combining:
            // roughness, specularF0, metalness, occlusion
            DetailMisc,

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
            "m_detailMiscClipmaps"
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
        void InitializeClipmapData();
        void InitializeClipmapImages();

        static constexpr uint32_t MacroClipmapStackSize = 3;
        static constexpr uint32_t DetailClipmapStackSize = 5;
        static constexpr uint32_t ClipmapSizeWidth = 1024;
        static constexpr uint32_t ClipmapSizeHeight = 1024;

        static_assert(DetailClipmapStackSize >= MacroClipmapStackSize,
            "Macro clipmaps use lower resolutions. Array construction will use"
            "max(DetailClipmapStackSize, MacroClipmapStackSize).");

        struct ClipmapData
        {
            //! The 2D xy-plane view position where the main camera is.
            AZStd::array<float, 2> m_previousViewPosition;
            AZStd::array<float, 2> m_currentViewPosition;

            // 2D xy-plane world bounds defined by the terrain.
            AZStd::array<float, 2> m_worldBoundsMin;
            AZStd::array<float, 2> m_worldBoundsMax;

            //! The max range that the clipmap is covering.
            AZStd::array<float, 2> m_maxRenderSize;

            //! The size of a single clipmap.
            AZStd::array<float, 2> m_clipmapSize;

            //! Clipmap centers in normalized UV coordinates [0, 1].
            // 0,1: previous clipmap centers; 2,3: current clipmap centers.
            // They are used for toroidal addressing and may move each frame based on the view point movement.
            // The move distance is scaled differently in each layer.
            AZStd::array<AZStd::array<float, 4>, DetailClipmapStackSize> m_clipmapCenters;

            //! A list of reciprocal the clipmap scale [s],
            //! where 1 pixel in the current layer of clipmap represents s meters.
            //! Fast lookup list to avoid redundant calculation in shaders.
            AZStd::array<AZStd::array<float, 4>, DetailClipmapStackSize> m_clipmapScaleInv;
        };

        ClipmapData m_clipmapData;

        AZ::RHI::ShaderInputNameIndex m_terrainSrgClipmapDataIndex = ClipmapDataShaderInput;
        AZ::RHI::ShaderInputNameIndex m_terrainSrgClipmapImageIndex[ClipmapName::Count];

        AZ::Data::Instance<AZ::RPI::AttachmentImage> m_clipmaps[ClipmapName::Count];

        bool m_isInitialized = false;
    };
}
