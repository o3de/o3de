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
#include <Atom/RPI.Public/Shader/Shader.h>

#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RHI/ShaderResourceGroup.h>
#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/DrawPacket.h>
#include <Atom/RHI/IndexBufferView.h>
#include <Atom/RHI/PipelineState.h>
#include <Atom/RHI/StreamBufferView.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

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
        // RPI::SceneNotificationBus overrides ...
        void OnRenderPipelineAdded(AZ::RPI::RenderPipelinePtr pipeline) override;
        void OnRenderPipelineRemoved(AZ::RPI::RenderPipeline* pipeline) override;
        void OnRenderPipelinePassesChanged(AZ::RPI::RenderPipeline* renderPipeline) override;

        void InitializeAtomStuff();

        void InitializeTerrainPatch();

        bool InitializeRenderBuffers();
        void DestroyRenderBuffers();

        void ProcessSurfaces(const FeatureProcessor::RenderPacket& process);

        // System-level parameters
        const float m_gridSpacing{ 1.0f };
        const float m_gridMeters{ 32.0f };

        // System-level cached reference to the Atom RHI
        AZ::RHI::RHISystemInterface* m_rhiSystem = nullptr;

        // System-level references to the shader, pipeline, and shader-related information
        AZ::Data::Instance<AZ::RPI::Shader> m_shader{};
        AZ::RHI::PipelineStateDescriptorForDraw m_pipelineStateDescriptor;
        AZ::RHI::ConstPtr<AZ::RHI::PipelineState> m_pipelineState = nullptr;
        AZ::RHI::DrawListTag m_drawListTag;
        AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> m_perObjectSrgAsset;

        AZ::RHI::ShaderInputImageIndex m_heightmapImageIndex;
        AZ::RHI::ShaderInputConstantIndex m_modelToWorldIndex;
        AZ::RHI::ShaderInputConstantIndex m_heightScaleIndex;
        AZ::RHI::ShaderInputConstantIndex m_uvMinIndex;
        AZ::RHI::ShaderInputConstantIndex m_uvMaxIndex;
        AZ::RHI::ShaderInputConstantIndex m_uvStepIndex;


        // Pos_float_2 + UV_float_2
        struct Vertex
        {
            float m_posx;
            float m_posy;
            float m_u;
            float m_v;

            Vertex(float posx, float posy, float u, float v)
                : m_posx(posx)
                , m_posy(posy)
                , m_u(u)
                , m_v(v)
            {
            }
        };

        // System-level definition of a grid patch.  (ex: 32m x 32m)
        AZStd::vector<Vertex> m_gridVertices;
        AZStd::vector<uint16_t> m_gridIndices;

        // System-level data related to the grid patch
        AZ::RHI::Ptr<AZ::RHI::BufferPool> m_hostPool = nullptr;
        AZ::RHI::Ptr<AZ::RHI::Buffer> m_indexBuffer;
        AZ::RHI::Ptr<AZ::RHI::Buffer> m_vertexBuffer;
        AZ::RHI::IndexBufferView m_indexBufferView;
        AZ::RHI::StreamBufferView m_vertexBufferView;

        // Per-area data
        struct TerrainAreaData
        {
            AZ::Transform m_transform;
            AZ::Aabb m_terrainBounds{ AZ::Aabb::CreateNull() };
            float m_heightScale;
            AZ::Data::Instance<AZ::RPI::StreamingImage> m_heightmapImage;
            uint32_t m_heightmapImageWidth;
            uint32_t m_heightmapImageHeight;
            bool m_propertiesDirty{ true };
        };

        TerrainAreaData m_areaData;

        struct SectorData
        {
            AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> m_srg;
            AZ::Aabb m_aabb;
            AZStd::unique_ptr<const AZ::RHI::DrawPacket> m_drawPacket;

            SectorData(const AZ::RHI::DrawPacket* drawPacket, AZ::Aabb aabb, AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> srg)
                : m_srg(srg)
                , m_aabb(aabb)
                , m_drawPacket(drawPacket)
            {}
        };

        AZStd::vector<SectorData> m_sectorData;
    };
}
