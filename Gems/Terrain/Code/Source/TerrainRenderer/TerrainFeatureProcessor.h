/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Shader/Shader.h>

#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/MeshDrawPacket.h>
#include <Atom/RHI/ShaderResourceGroup.h>
#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/DrawPacket.h>
#include <Atom/RHI/IndexBufferView.h>
#include <Atom/RHI/PipelineState.h>
#include <Atom/RHI/StreamBufferView.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ::RPI
{
    namespace AssetUtils
    {
        class AsyncAssetLoader;
    }
    class Model;
}

namespace Terrain
{
    class TerrainFeatureProcessor final
        : public AZ::RPI::FeatureProcessor
    {
    public:
        AZ_RTTI(TerrainFeatureProcessor, "{D7DAC1F9-4A9F-4D3C-80AE-99579BF8AB1C}", AZ::RPI::FeatureProcessor);
        AZ_DISABLE_COPY_MOVE(TerrainFeatureProcessor);
        AZ_FEATURE_PROCESSOR(TerrainFeatureProcessor);

        static void Reflect(AZ::ReflectContext* context);

        TerrainFeatureProcessor() = default;
        ~TerrainFeatureProcessor() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        void Render(const AZ::RPI::FeatureProcessor::RenderPacket& packet) override;

        void UpdateTerrainData(const AZ::Transform& transform, const AZ::Aabb& worldBounds, float sampleSpacing,
                               uint32_t width, uint32_t height, const AZStd::vector<float>& heightData);

        void RemoveTerrainData()
        {
            m_areaData = {};
        }

    private:

        struct ShaderTerrainData // Must align with struct in Object Srg
        {
            AZStd::array<float, 2> m_uvMin;
            AZStd::array<float, 2> m_uvMax;
            AZStd::array<float, 2> m_uvStep;
            float m_sampleSpacing;
            float m_heightScale;
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
        
        void Initialize();
        void InitializeTerrainPatch(uint16_t gridSize, float gridSpacing, PatchData& patchdata);
        bool InitializePatchModel();

        void ProcessSurfaces(const FeatureProcessor::RenderPacket& process);
        
        AZ::Outcome<AZ::Data::Asset<AZ::RPI::BufferAsset>> CreateBufferAsset(
            const void* data, const AZ::RHI::BufferViewDescriptor& bufferViewDescriptor, const AZStd::string& bufferName);

        // System-level parameters
        static constexpr float GridSpacing{ 1.0f };
        static constexpr uint32_t GridSize{ 64 }; // number of terrain quads (vertices are m_gridSize + 1)
        static constexpr float GridMeters{ GridSpacing * GridSize };

        AZStd::unique_ptr<AZ::RPI::AssetUtils::AsyncAssetLoader> m_materialAssetLoader;
        AZ::Data::Instance<AZ::RPI::Material> m_materialInstance;

        AZ::RHI::ShaderInputConstantIndex m_modelToWorldIndex;
        AZ::RHI::ShaderInputConstantIndex m_terrainDataIndex;

        AZ::Data::Instance<AZ::RPI::Model> m_patchModel;

        // Per-area data
        struct TerrainAreaData
        {
            AZ::Transform m_transform{ AZ::Transform::CreateIdentity() };
            AZ::Aabb m_terrainBounds{ AZ::Aabb::CreateNull() };
            float m_heightScale{ 0.0f };
            AZ::Data::Instance<AZ::RPI::StreamingImage> m_heightmapImage;
            uint32_t m_heightmapImageWidth{ 0 };
            uint32_t m_heightmapImageHeight{ 0 };
            bool m_propertiesDirty{ true };
            float m_sampleSpacing{ 0.0f };
        };

        TerrainAreaData m_areaData;

        struct SectorData
        {
            AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> m_srg; // Hold on to ref so it's not dropped
            AZ::Aabb m_aabb;
            AZStd::fixed_vector<AZ::RPI::MeshDrawPacket, AZ::RPI::ModelLodAsset::LodCountMax> m_drawPackets;
        };

        AZStd::vector<SectorData> m_sectorData;
    };
}
