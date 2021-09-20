/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/TerrainFeatureProcessor.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Frustum.h>

#include <Atom/Utils/Utils.h>

#include <Atom/RHI/DrawPacketBuilder.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomDraw.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/StreamingImagePool.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/Image/ImageMipChainAssetCreator.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAssetCreator.h>
#include <Atom/RHI.Reflect/InputStreamLayout.h>
#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>


namespace Terrain
{
    namespace
    {
        const uint32_t DEFAULT_UploadBufferSize = 512 * 1024; // 512k
        [[maybe_unused]] const char* TerrainFPName = "TerrainFeatureProcessor";
    }

    namespace ShaderInputs
    {
        static const char* const HeightmapImage("m_heightmapImage");
        static const char* const ModelToWorld("m_modelToWorld");
        static const char* const TerrainData("m_terrainData");
    }


    void TerrainFeatureProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TerrainFeatureProcessor, AZ::RPI::FeatureProcessor>()
                ->Version(0)
                ;
        }
    }

    void TerrainFeatureProcessor::Activate()
    {
        m_areaData = {};

        InitializeAtomStuff();
        EnableSceneNotification();
    }

    void TerrainFeatureProcessor::ConfigurePipelineState(ShaderState& shaderState, bool assertOnFail)
    {
        if (shaderState.m_shader == nullptr)
        {
            AZ_Assert(shaderState.m_shader || !assertOnFail, "Terrain shader failed to load correctly.");
            return;
        }

        bool success = GetParentScene()->ConfigurePipelineState(shaderState.m_shader->GetDrawListTag(), shaderState.m_pipelineStateDescriptor);
        AZ_Assert(success || !assertOnFail, "Couldn't configure the pipeline state.");
        if (success)
        {
            shaderState.m_pipelineState = shaderState.m_shader->AcquirePipelineState(shaderState.m_pipelineStateDescriptor);
            AZ_Assert(shaderState.m_pipelineState, "Failed to acquire default pipeline state.");
        }
    }

    void TerrainFeatureProcessor::InitializeAtomStuff()
    {
        m_rhiSystem = AZ::RHI::RHISystemInterface::Get();

        {       
            auto LoadShader = [this](const char* filePath, ShaderState& shaderState)
            {
                shaderState.m_shader = AZ::RPI::LoadShader(filePath);
                if (!shaderState.m_shader)
                {
                    AZ_Error(TerrainFPName, false, "Failed to find or create a shader instance from shader asset '%s'", filePath);
                    return;
                }
                
                // Create the data layout
                shaderState.m_pipelineStateDescriptor = AZ::RHI::PipelineStateDescriptorForDraw{};
                {
                    AZ::RHI::InputStreamLayoutBuilder layoutBuilder;

                    layoutBuilder.AddBuffer()
                        ->Channel("POSITION", AZ::RHI::Format::R32G32_FLOAT)
                        ->Channel("UV", AZ::RHI::Format::R32G32_FLOAT)
                        ;
                    shaderState.m_pipelineStateDescriptor.m_inputStreamLayout = layoutBuilder.End();
                }

                auto shaderVariant = shaderState.m_shader->GetVariant(AZ::RPI::ShaderAsset::RootShaderVariantStableId);
                shaderVariant.ConfigurePipelineState(shaderState.m_pipelineStateDescriptor);
                
                // If this fails to run now, it's ok, we'll initialize it in OnRenderPipelineAdded later.
                ConfigurePipelineState(shaderState, false);
            };
            
            LoadShader("Shaders/Terrain/Terrain.azshader", m_shaderStates[ShaderType::Forward]);
            LoadShader("Shaders/Terrain/Terrain_DepthPass.azshader", m_shaderStates[ShaderType::Depth]);

            // Forward and depth shader use same srg layout.
            AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> perObjectSrgLayout =
                m_shaderStates[ShaderType::Forward].m_shader->FindShaderResourceGroupLayout(AZ::Name{"ObjectSrg"});

            if (!perObjectSrgLayout)
            {
                AZ_Error(TerrainFPName, false, "Failed to get shader resource group layout");
                return;
            }
            else if (!perObjectSrgLayout->IsFinalized())
            {
                AZ_Error(TerrainFPName, false, "Shader resource group layout is not loaded");
                return;
            }

            m_heightmapImageIndex = perObjectSrgLayout->FindShaderInputImageIndex(AZ::Name(ShaderInputs::HeightmapImage));
            AZ_Error(TerrainFPName, m_heightmapImageIndex.IsValid(), "Failed to find shader input image %s.", ShaderInputs::HeightmapImage);

            m_modelToWorldIndex = perObjectSrgLayout->FindShaderInputConstantIndex(AZ::Name(ShaderInputs::ModelToWorld));
            AZ_Error(TerrainFPName, m_modelToWorldIndex.IsValid(), "Failed to find shader input constant %s.", ShaderInputs::ModelToWorld);

            m_terrainDataIndex = perObjectSrgLayout->FindShaderInputConstantIndex(AZ::Name(ShaderInputs::TerrainData));
            AZ_Error(TerrainFPName, m_terrainDataIndex.IsValid(), "Failed to find shader input constant %s.", ShaderInputs::TerrainData);
        }

        AZ::RHI::BufferPoolDescriptor dmaPoolDescriptor;
        dmaPoolDescriptor.m_heapMemoryLevel = AZ::RHI::HeapMemoryLevel::Host;
        dmaPoolDescriptor.m_bindFlags = AZ::RHI::BufferBindFlags::InputAssembly;

        m_hostPool = AZ::RHI::Factory::Get().CreateBufferPool();
        m_hostPool->SetName(AZ::Name("TerrainVertexPool"));
        AZ::RHI::ResultCode resultCode = m_hostPool->Init(*m_rhiSystem->GetDevice(), dmaPoolDescriptor);

        if (resultCode != AZ::RHI::ResultCode::Success)
        {
            AZ_Error(TerrainFPName, false, "Failed to create host buffer pool from RPI");
            return;
        }

        InitializeTerrainPatch();

        if (!InitializeRenderBuffers())
        {
            AZ_Error(TerrainFPName, false, "Failed to create Terrain render buffers!");
            return;
        }
    }

    void TerrainFeatureProcessor::OnRenderPipelineAdded([[maybe_unused]] AZ::RPI::RenderPipelinePtr pipeline)
    {
        for (ShaderState& shaderState: m_shaderStates)
        {
            ConfigurePipelineState(shaderState, true);
        }
    }

    void TerrainFeatureProcessor::OnRenderPipelineRemoved([[maybe_unused]] AZ::RPI::RenderPipeline* pipeline)
    {
    }

    void TerrainFeatureProcessor::OnRenderPipelinePassesChanged([[maybe_unused]] AZ::RPI::RenderPipeline* renderPipeline)
    {
    }


    void TerrainFeatureProcessor::Deactivate()
    {
        DisableSceneNotification();

        DestroyRenderBuffers();
        m_areaData = {};

        if (m_hostPool)
        {
            m_hostPool.reset();
        }

        m_rhiSystem = nullptr;
    }

    void TerrainFeatureProcessor::Render(const AZ::RPI::FeatureProcessor::RenderPacket& packet)
    {
        ProcessSurfaces(packet);
    }

    void TerrainFeatureProcessor::UpdateTerrainData(
        const AZ::Transform& transform,
        const AZ::Aabb& worldBounds,
        float sampleSpacing,
        uint32_t width, uint32_t height, const AZStd::vector<float>& heightData)
    {
        if (!worldBounds.IsValid())
        {
            return;
        }

        m_areaData.m_transform = transform;
        m_areaData.m_heightScale = worldBounds.GetZExtent();
        m_areaData.m_terrainBounds = worldBounds;
        m_areaData.m_heightmapImageHeight = height;
        m_areaData.m_heightmapImageWidth = width;
        m_areaData.m_sampleSpacing = sampleSpacing;

        // Create heightmap image data
        {
            m_areaData.m_propertiesDirty = true;

            AZ::RHI::Size imageSize;
            imageSize.m_width = width;
            imageSize.m_height = height;

            AZStd::vector<uint16_t> uint16Heights;
            uint16Heights.reserve(heightData.size());
            for (float sampleHeight : heightData)
            {
                float clampedSample = AZ::GetClamp(sampleHeight, 0.0f, 1.0f);
                constexpr uint16_t MaxUint16 = 0xFFFF;
                uint16Heights.push_back(aznumeric_cast<uint16_t>(clampedSample * MaxUint16));
            }

            AZ::Data::Instance<AZ::RPI::StreamingImagePool> streamingImagePool = AZ::RPI::ImageSystemInterface::Get()->GetSystemStreamingPool();
            m_areaData.m_heightmapImage = AZ::RPI::StreamingImage::CreateFromCpuData(*streamingImagePool,
                AZ::RHI::ImageDimension::Image2D,
                imageSize,
                AZ::RHI::Format::R16_UNORM,
                (uint8_t*)uint16Heights.data(),
                heightData.size() * sizeof(uint16_t));
            AZ_Error(TerrainFPName, m_areaData.m_heightmapImage, "Failed to initialize the heightmap image!");
        }

    }

    void TerrainFeatureProcessor::ProcessSurfaces(const FeatureProcessor::RenderPacket& process)
    {
        AZ_PROFILE_FUNCTION(AzRender);

        if ((m_shaderStates[ShaderType::Forward].m_shader == nullptr) ||
            (m_shaderStates[ShaderType::Depth].m_shader == nullptr) ||
            m_shaderStates[ShaderType::Forward].m_shader->GetDrawListTag().IsNull() ||
            m_shaderStates[ShaderType::Depth].m_shader->GetDrawListTag().IsNull())
        {
            return;
        }

        if (!m_areaData.m_terrainBounds.IsValid())
        {
            return;
        }
        
        if (m_areaData.m_propertiesDirty)
        {
            m_areaData.m_propertiesDirty = false;
            m_sectorData.clear();

            AZ::RHI::DrawPacketBuilder drawPacketBuilder;

            uint32_t numIndices = static_cast<uint32_t>(m_gridIndices.size());

            AZ::RHI::DrawIndexed drawIndexed;
            drawIndexed.m_indexCount = numIndices;
            drawIndexed.m_indexOffset = 0;
            drawIndexed.m_vertexOffset = 0;

            float xFirstPatchStart =
                m_areaData.m_terrainBounds.GetMin().GetX() - fmod(m_areaData.m_terrainBounds.GetMin().GetX(), m_gridMeters);
            float xLastPatchStart = m_areaData.m_terrainBounds.GetMax().GetX() - fmod(m_areaData.m_terrainBounds.GetMax().GetX(), m_gridMeters);
            float yFirstPatchStart =
                m_areaData.m_terrainBounds.GetMin().GetY() - fmod(m_areaData.m_terrainBounds.GetMin().GetY(), m_gridMeters);
            float yLastPatchStart = m_areaData.m_terrainBounds.GetMax().GetY() - fmod(m_areaData.m_terrainBounds.GetMax().GetY(), m_gridMeters);

            for (float yPatch = yFirstPatchStart; yPatch <= yLastPatchStart; yPatch += m_gridMeters)
            {
                for (float xPatch = xFirstPatchStart; xPatch <= xLastPatchStart; xPatch += m_gridMeters)
                {
                    drawPacketBuilder.Begin(nullptr);
                    drawPacketBuilder.SetDrawArguments(drawIndexed);
                    drawPacketBuilder.SetIndexBufferView(m_indexBufferView);
                    auto& forwardShader = m_shaderStates[ShaderType::Forward].m_shader;

                    auto resourceGroup = AZ::RPI::ShaderResourceGroup::Create(forwardShader->GetAsset(), forwardShader->GetSupervariantIndex(), AZ::Name("ObjectSrg"));
                    if (!resourceGroup)
                    {
                        AZ_Error(TerrainFPName, false, "Failed to create shader resource group");
                        return;
                    }

                    AZStd::array<float, 2> uvMin = { 0.0f, 0.0f };
                    AZStd::array<float, 2> uvMax = { 1.0f, 1.0f };

                    uvMin[0] = (float)((xPatch - m_areaData.m_terrainBounds.GetMin().GetX()) / m_areaData.m_terrainBounds.GetXExtent());
                    uvMin[1] = (float)((yPatch - m_areaData.m_terrainBounds.GetMin().GetY()) / m_areaData.m_terrainBounds.GetYExtent());

                    uvMax[0] =
                        (float)(((xPatch + m_gridMeters) - m_areaData.m_terrainBounds.GetMin().GetX()) / m_areaData.m_terrainBounds.GetXExtent());
                    uvMax[1] =
                        (float)(((yPatch + m_gridMeters) - m_areaData.m_terrainBounds.GetMin().GetY()) / m_areaData.m_terrainBounds.GetYExtent());

                    AZStd::array<float, 2> uvStep =
                    {
                        1.0f / m_areaData.m_heightmapImageWidth, 1.0f / m_areaData.m_heightmapImageHeight,
                    };

                    AZ::Transform transform = m_areaData.m_transform;
                    transform.SetTranslation(xPatch, yPatch, m_areaData.m_transform.GetTranslation().GetZ());

                    AZ::Matrix3x4 matrix3x4 = AZ::Matrix3x4::CreateFromTransform(transform);

                    resourceGroup->SetImage(m_heightmapImageIndex, m_areaData.m_heightmapImage);
                    resourceGroup->SetConstant(m_modelToWorldIndex, matrix3x4);

                    ShaderTerrainData terrainDataForSrg;
                    terrainDataForSrg.m_sampleSpacing = m_areaData.m_sampleSpacing;
                    terrainDataForSrg.m_heightScale = m_areaData.m_heightScale;
                    terrainDataForSrg.m_uvMin = uvMin;
                    terrainDataForSrg.m_uvMax = uvMax;
                    terrainDataForSrg.m_uvStep = uvStep;
                    resourceGroup->SetConstant(m_terrainDataIndex, terrainDataForSrg);

                    resourceGroup->Compile();
                    drawPacketBuilder.AddShaderResourceGroup(resourceGroup->GetRHIShaderResourceGroup());

                    auto addDrawItem = [&](ShaderState& shaderState)
                    {
                        AZ::RHI::DrawPacketBuilder::DrawRequest drawRequest;
                        drawRequest.m_listTag = shaderState.m_shader->GetDrawListTag();
                        drawRequest.m_pipelineState = shaderState.m_pipelineState.get();
                        drawRequest.m_streamBufferViews = AZStd::array_view<AZ::RHI::StreamBufferView>(&m_vertexBufferView, 1);
                        drawPacketBuilder.AddDrawItem(drawRequest);
                    };

                    for (ShaderState& shaderState : m_shaderStates)
                    {
                        addDrawItem(shaderState);
                    }
                    //addDrawItem(m_shaderStates[ShaderType::Forward]);
                    
                    m_sectorData.emplace_back(
                        drawPacketBuilder.End(),
                        AZ::Aabb::CreateFromMinMax(
                            AZ::Vector3(xPatch, yPatch, m_areaData.m_terrainBounds.GetMin().GetZ()),
                            AZ::Vector3(xPatch + m_gridMeters, yPatch + m_gridMeters, m_areaData.m_terrainBounds.GetMax().GetZ())
                        ),
                        resourceGroup
                    );
                }
            }
        }
        
        for (auto& view : process.m_views)
        {
            AZ::Frustum viewFrustum = AZ::Frustum::CreateFromMatrixColumnMajor(view->GetWorldToClipMatrix());
            for (auto& sectorData : m_sectorData)
            {
                if (viewFrustum.IntersectAabb(sectorData.m_aabb) != AZ::IntersectResult::Exterior)
                {
                    view->AddDrawPacket(sectorData.m_drawPacket.get());
                }
            }
        }
    }

    void TerrainFeatureProcessor::InitializeTerrainPatch()
    {
        m_gridVertices.clear();
        m_gridIndices.clear();

        for (float y = 0.0f; y < m_gridMeters; y += m_gridSpacing)
        {
            for (float x = 0.0f; x < m_gridMeters; x += m_gridSpacing)
            {
                float x0 = x;
                float x1 = x + m_gridSpacing;
                float y0 = y;
                float y1 = y + m_gridSpacing;

                uint16_t startIndex = (uint16_t)(m_gridVertices.size());

                m_gridVertices.emplace_back(x0, y0, x0 / m_gridMeters, y0 / m_gridMeters);
                m_gridVertices.emplace_back(x1, y0, x1 / m_gridMeters, y0 / m_gridMeters);
                m_gridVertices.emplace_back(x0, y1, x0 / m_gridMeters, y1 / m_gridMeters);
                m_gridVertices.emplace_back(x1, y1, x1 / m_gridMeters, y1 / m_gridMeters);

                m_gridIndices.emplace_back(startIndex);
                m_gridIndices.emplace_back(aznumeric_cast<uint16_t>(startIndex + 1));
                m_gridIndices.emplace_back(aznumeric_cast<uint16_t>(startIndex + 2));
                m_gridIndices.emplace_back(aznumeric_cast<uint16_t>(startIndex + 1));
                m_gridIndices.emplace_back(aznumeric_cast<uint16_t>(startIndex + 3));
                m_gridIndices.emplace_back(aznumeric_cast<uint16_t>(startIndex + 2));
            }
        }
    }

    bool TerrainFeatureProcessor::InitializeRenderBuffers()
    {
        AZ::RHI::ResultCode result = AZ::RHI::ResultCode::Fail;

        // Create geometry buffers
        m_indexBuffer = AZ::RHI::Factory::Get().CreateBuffer();
        m_vertexBuffer = AZ::RHI::Factory::Get().CreateBuffer();

        m_indexBuffer->SetName(AZ::Name("TerrainIndexBuffer"));
        m_vertexBuffer->SetName(AZ::Name("TerrainVertexBuffer"));

        AZStd::vector<AZ::RHI::Ptr<AZ::RHI::Buffer>> buffers = { m_indexBuffer , m_vertexBuffer };

        // Fill our buffers with the vertex/index data
        for (size_t bufferIndex = 0; bufferIndex < buffers.size(); ++bufferIndex)
        {
            AZ::RHI::Ptr<AZ::RHI::Buffer> buffer = buffers[bufferIndex];

            // Initialize the buffer

            AZ::RHI::BufferInitRequest bufferRequest;
            bufferRequest.m_descriptor = AZ::RHI::BufferDescriptor{ AZ::RHI::BufferBindFlags::InputAssembly, DEFAULT_UploadBufferSize };
            bufferRequest.m_buffer = buffer.get();

            result = m_hostPool->InitBuffer(bufferRequest);

            if (result != AZ::RHI::ResultCode::Success)
            {
                AZ_Error(TerrainFPName, false, "Failed to create GPU buffers for Terrain");
                return false;
            }

            // Grab a pointer to the buffer's data

            m_hostPool->OrphanBuffer(*buffer);

            AZ::RHI::BufferMapResponse mapResponse;
            m_hostPool->MapBuffer(AZ::RHI::BufferMapRequest(*buffer, 0, DEFAULT_UploadBufferSize), mapResponse);

            auto* mappedData = reinterpret_cast<uint8_t*>(mapResponse.m_data);

            //0th index should always be the index buffer
            if (bufferIndex == 0)
            {
                // Fill the index buffer with our terrain patch indices
                const uint64_t idxSize = m_gridIndices.size() * sizeof(uint16_t);
                memcpy(mappedData, m_gridIndices.data(), idxSize);

                m_indexBufferView = AZ::RHI::IndexBufferView(
                    *buffer, 0, static_cast<uint32_t>(idxSize), AZ::RHI::IndexFormat::Uint16);
            }
            else
            {
                // Fill the vertex buffer with our terrain patch vertices
                const uint64_t elementSize = m_gridVertices.size() * sizeof(Vertex);
                memcpy(mappedData, m_gridVertices.data(), elementSize);

                m_vertexBufferView = AZ::RHI::StreamBufferView(
                    *buffer, 0, static_cast<uint32_t>(elementSize), static_cast<uint32_t>(sizeof(Vertex)));
            }

            m_hostPool->UnmapBuffer(*buffer);
        }

        return true;
    }

    void TerrainFeatureProcessor::DestroyRenderBuffers()
    {
        m_indexBuffer.reset();
        m_vertexBuffer.reset();

        m_indexBufferView = {};
        m_vertexBufferView = {};

        for (ShaderState& shaderState : m_shaderStates)
        {
            shaderState.Reset();
        }
    }
}
