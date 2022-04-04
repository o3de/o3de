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

        TerrainClipmapManager() = default;
        virtual ~TerrainClipmapManager() = default;

        void Initialize(AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg);
        bool IsInitialized() const;
        void Reset();
        bool UpdateSrgIndices(AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& srg);

        void Update(const AZ::Vector3& cameraPosition, AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg);

        AZ::Data::Instance<AZ::RPI::AttachmentImage> GetClipmapImage(const AZ::Name& clipmapName) const;
    private:
        void UpdateClipmapData(const AZ::Vector3& cameraPosition);
        void InitializeClipmapData();
        void InitializeClipmapImages();

        static constexpr uint32_t ClipmapStackSize = 5;
        static constexpr uint32_t ClipmapSizeWidth = 1024;
        static constexpr uint32_t ClipmapSizeHeight = 1024;

        struct ClipmapData
        {
            //! The 2D xy-plane view position where the main camera is.
            //! 0,1: previous; 2,3: current.
            AZStd::array<float, 4> m_viewPosition;

            // 2D xy-plane world bounds defined by the terrain.
            // 0,1: min; 2,3: max.
            AZStd::array<float, 4> m_worldBounds;

            //! The max range that the clipmap is covering.
            AZStd::array<float, 2> m_maxRenderSize;

            //! The size of a single clipmap.
            AZStd::array<float, 2> m_clipmapSize;

            //! Clipmap centers in normalized UV coordinates [0, 1].
            // 0,1: previous clipmap centers; 2,3: current clipmap centers.
            // They are used for toroidal addressing and may move each frame based on the view point movement.
            // The move distance is scaled differently in each layer.
            AZStd::array<AZStd::array<float, 4>, ClipmapStackSize> m_clipmapCenters;

            //! A list of reciprocal the clipmap scale [s],
            //! where 1 pixel in the current layer of clipmap represents s meters.
            //! Fast lookup list to avoid redundant calculation in shaders.
            AZStd::array<AZStd::array<float, 4>, ClipmapStackSize> m_clipmapScaleInv;
        };

        ClipmapData m_clipmapData;

        AZ::RHI::ShaderInputConstantIndex m_clipmapDataIndex;
        AZ::RHI::ShaderInputImageIndex m_macroColorClipmapsIndex;
        AZ::RHI::ShaderInputImageIndex m_macroNormalClipmapsIndex;
        AZ::RHI::ShaderInputImageIndex m_detailColorClipmapsIndex;
        AZ::RHI::ShaderInputImageIndex m_detailNormalClipmapsIndex;
        AZ::RHI::ShaderInputImageIndex m_detailHeightClipmapsIndex;
        AZ::RHI::ShaderInputImageIndex m_detailMiscClipmapsIndex;

        AZStd::unordered_map<AZ::Name, AZ::Data::Instance<AZ::RPI::AttachmentImage>> m_clipmaps;

        bool m_isInitialized = false;

        bool m_clipmapsNeedUpdate = false;
    };
}
