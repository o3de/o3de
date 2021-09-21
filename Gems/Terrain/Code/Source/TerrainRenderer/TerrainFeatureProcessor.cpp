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
#include <Atom/RPI.Public/MeshDrawPacket.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomDraw.h>
#include <Atom/RPI.Public/Buffer/BufferSystem.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/StreamingImagePool.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Buffer/BufferAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelLodAssetCreator.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/Image/ImageMipChainAssetCreator.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAssetCreator.h>
#include <Atom/RHI.Reflect/InputStreamLayout.h>
#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>
#include <Atom/Feature/RenderCommon.h>

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

            // Load the terrain material asynchronously
            const AZStd::string materialFilePath = "Materials/Terrain/DefaultPbrTerrain.azmaterial";
            m_materialAssetLoader = AZStd::make_unique<AZ::RPI::AssetUtils::AsyncAssetLoader>();
            *m_materialAssetLoader = AZ::RPI::AssetUtils::AsyncAssetLoader::Create<AZ::RPI::MaterialAsset>(materialFilePath, 0u,
                [&](AZ::Data::Asset<AZ::Data::AssetData> assetData, bool success) -> void
                {
                    const AZ::Data::Asset<AZ::RPI::MaterialAsset>& materialAsset = static_cast<AZ::Data::Asset<AZ::RPI::MaterialAsset>>(assetData);
                    m_materialInstance = success ? AZ::RPI::Material::FindOrCreate(assetData) : nullptr;
                    
                    if (!materialAsset->GetObjectSrgLayout())
                    {
                        AZ_Error("TerrainFeatureProcessor", false, "No per-object ShaderResourceGroup found on terrain material.");
                    }
                    m_materialAssetLoader.reset();
                }
            );

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

        if (!InitializePatchModel())
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

        m_patchModel = {};
        m_areaData = {};

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

            float xFirstPatchStart =
                m_areaData.m_terrainBounds.GetMin().GetX() - fmod(m_areaData.m_terrainBounds.GetMin().GetX(), GridMeters);
            float xLastPatchStart = m_areaData.m_terrainBounds.GetMax().GetX() - fmod(m_areaData.m_terrainBounds.GetMax().GetX(), GridMeters);
            float yFirstPatchStart =
                m_areaData.m_terrainBounds.GetMin().GetY() - fmod(m_areaData.m_terrainBounds.GetMin().GetY(), GridMeters);
            float yLastPatchStart = m_areaData.m_terrainBounds.GetMax().GetY() - fmod(m_areaData.m_terrainBounds.GetMax().GetY(), GridMeters);

            for (float yPatch = yFirstPatchStart; yPatch <= yLastPatchStart; yPatch += GridMeters)
            {
                for (float xPatch = xFirstPatchStart; xPatch <= xLastPatchStart; xPatch += GridMeters)
                {
                    const auto& materialAsset = m_materialInstance->GetAsset();
                    auto& shaderAsset = materialAsset->GetMaterialTypeAsset()->GetShaderAssetForObjectSrg();
                    auto objectSrg = AZ::RPI::ShaderResourceGroup::Create(shaderAsset, materialAsset->GetObjectSrgLayout()->GetName());
                    if (!objectSrg)
                    {
                        AZ_Warning("TerrainFeatureProcessor", false, "Failed to create a new shader resource group, skipping.");
                        continue;
                    }

                    { // Update SRG
                        
                        AZStd::array<float, 2> uvMin = { 0.0f, 0.0f };
                        AZStd::array<float, 2> uvMax = { 1.0f, 1.0f };

                        uvMin[0] = (float)((xPatch - m_areaData.m_terrainBounds.GetMin().GetX()) / m_areaData.m_terrainBounds.GetXExtent());
                        uvMin[1] = (float)((yPatch - m_areaData.m_terrainBounds.GetMin().GetY()) / m_areaData.m_terrainBounds.GetYExtent());

                        uvMax[0] =
                            (float)(((xPatch + GridMeters) - m_areaData.m_terrainBounds.GetMin().GetX()) / m_areaData.m_terrainBounds.GetXExtent());
                        uvMax[1] =
                            (float)(((yPatch + GridMeters) - m_areaData.m_terrainBounds.GetMin().GetY()) / m_areaData.m_terrainBounds.GetYExtent());

                        AZStd::array<float, 2> uvStep =
                        {
                            1.0f / m_areaData.m_heightmapImageWidth, 1.0f / m_areaData.m_heightmapImageHeight,
                        };

                        AZ::Transform transform = m_areaData.m_transform;
                        transform.SetTranslation(xPatch, yPatch, m_areaData.m_transform.GetTranslation().GetZ());

                        AZ::Matrix3x4 matrix3x4 = AZ::Matrix3x4::CreateFromTransform(transform);

                        objectSrg->SetImage(m_heightmapImageIndex, m_areaData.m_heightmapImage);
                        objectSrg->SetConstant(m_modelToWorldIndex, matrix3x4);

                        ShaderTerrainData terrainDataForSrg;
                        terrainDataForSrg.m_sampleSpacing = m_areaData.m_sampleSpacing;
                        terrainDataForSrg.m_heightScale = m_areaData.m_heightScale;
                        terrainDataForSrg.m_uvMin = uvMin;
                        terrainDataForSrg.m_uvMax = uvMax;
                        terrainDataForSrg.m_uvStep = uvStep;
                        objectSrg->SetConstant(m_terrainDataIndex, terrainDataForSrg);

                        objectSrg->Compile();
                    }

                    AZ::RPI::ModelLod& modelLod = *m_patchModel->GetLods()[0].get();
                    AZ::RPI::MeshDrawPacket drawPacket(modelLod, 0, m_materialInstance, objectSrg);

                    // set the shader option to select forward pass IBL specular if necessary
                    if (!drawPacket.SetShaderOption(AZ::Name("o_meshUseForwardPassIBLSpecular"), AZ::RPI::ShaderOptionValue{ false }))
                    {
                        AZ_Warning("MeshDrawPacket", false, "Failed to set o_meshUseForwardPassIBLSpecular on mesh draw packet");
                    }
                    uint8_t stencilRef = AZ::Render::StencilRefs::UseDiffuseGIPass | AZ::Render::StencilRefs::UseIBLSpecularPass;
                    drawPacket.SetStencilRef(stencilRef);
                    drawPacket.Update(*GetParentScene(), false);

                    m_sectorData.emplace_back(
                        drawPacket,
                        AZ::Aabb::CreateFromMinMax(
                            AZ::Vector3(xPatch, yPatch, m_areaData.m_terrainBounds.GetMin().GetZ()),
                            AZ::Vector3(xPatch + GridMeters, yPatch + GridMeters, m_areaData.m_terrainBounds.GetMax().GetZ())
                        ),
                        objectSrg
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
                    view->AddDrawPacket(sectorData.m_drawPacket.GetRHIDrawPacket());
                }
            }
        }
    }

    void TerrainFeatureProcessor::InitializeTerrainPatch(PatchData& patchdata)
    {
        patchdata.m_positions.clear();
        patchdata.m_uvs.clear();
        patchdata.m_indices.clear();

        uint16_t gridVertices = GridSize + 1; // For m_gridSize quads, (m_gridSize + 1) vertices are needed.
        size_t size = gridVertices * gridVertices;
        size *= size;

        patchdata.m_positions.reserve(size);
        patchdata.m_uvs.reserve(size);

        for (uint16_t y = 0; y < gridVertices; ++y)
        {
            for (uint16_t x = 0; x < gridVertices; ++x)
            {
                patchdata.m_positions.push_back({ aznumeric_cast<float>(x) * GridSpacing, aznumeric_cast<float>(y) * GridSpacing });
                patchdata.m_uvs.push_back({ aznumeric_cast<float>(x) / GridSize, aznumeric_cast<float>(y) / GridSize });
            }
        }

        patchdata.m_indices.reserve(GridSize * GridSize * 6); // total number of quads, 2 triangles with 6 indices per quad.
        
        for (uint16_t y = 0; y < GridSize; ++y)
        {
            for (uint16_t x = 0; x < GridSize; ++x)
            {
                uint16_t topLeft = y * gridVertices + x;
                uint16_t topRight = topLeft + 1;
                uint16_t bottomLeft = (y + 1) * gridVertices + x;
                uint16_t bottomRight = bottomLeft + 1;

                constexpr uint16_t one = 1;

                patchdata.m_indices.emplace_back(topLeft);
                patchdata.m_indices.emplace_back(topRight);
                patchdata.m_indices.emplace_back(bottomLeft);
                patchdata.m_indices.emplace_back(bottomLeft);
                patchdata.m_indices.emplace_back(topRight);
                patchdata.m_indices.emplace_back(bottomRight);
            }
        }
    }
    
    AZ::Outcome<AZ::Data::Asset<AZ::RPI::BufferAsset>> TerrainFeatureProcessor::CreateBufferAsset(
        const void* data, const AZ::RHI::BufferViewDescriptor& bufferViewDescriptor, const AZStd::string& bufferName)
    {
        AZ::RPI::BufferAssetCreator creator;
        creator.Begin(AZ::Uuid::CreateRandom());

        AZ::RHI::BufferDescriptor bufferDescriptor;
        bufferDescriptor.m_bindFlags = AZ::RHI::BufferBindFlags::InputAssembly | AZ::RHI::BufferBindFlags::ShaderRead;
        bufferDescriptor.m_byteCount = static_cast<uint64_t>(bufferViewDescriptor.m_elementSize) * static_cast<uint64_t>(bufferViewDescriptor.m_elementCount);

        creator.SetBuffer(data, bufferDescriptor.m_byteCount, bufferDescriptor);
        creator.SetBufferViewDescriptor(bufferViewDescriptor);
        creator.SetUseCommonPool(AZ::RPI::CommonBufferPoolType::StaticInputAssembly);

        AZ::Data::Asset<AZ::RPI::BufferAsset> bufferAsset;
        if (creator.End(bufferAsset))
        {
            bufferAsset.SetHint(bufferName);
            return AZ::Success(bufferAsset);
        }

        return AZ::Failure();
    }

    bool TerrainFeatureProcessor::InitializePatchModel()
    {
        PatchData patchData;
        InitializeTerrainPatch(patchData);

        auto positionBufferViewDesc = AZ::RHI::BufferViewDescriptor::CreateTyped(0, aznumeric_cast<uint32_t>(patchData.m_positions.size()), AZ::RHI::Format::R32G32_FLOAT);
        auto positionsOutcome = CreateBufferAsset(patchData.m_positions.data(), positionBufferViewDesc, "TerrainPatchPositions");
        
        auto uvBufferViewDesc = AZ::RHI::BufferViewDescriptor::CreateTyped(0, aznumeric_cast<uint32_t>(patchData.m_uvs.size()), AZ::RHI::Format::R32G32_FLOAT);
        auto uvsOutcome = CreateBufferAsset(patchData.m_uvs.data(), uvBufferViewDesc, "TerrainPatchUvs");
        
        auto indexBufferViewDesc = AZ::RHI::BufferViewDescriptor::CreateTyped(0, aznumeric_cast<uint32_t>(patchData.m_indices.size()), AZ::RHI::Format::R16_UINT);
        auto indicesOutcome = CreateBufferAsset(patchData.m_indices.data(), indexBufferViewDesc, "TerrainPatchIndices");

        if (!positionsOutcome.IsSuccess() || !uvsOutcome.IsSuccess() || !indicesOutcome.IsSuccess())
        {
            AZ_Error(TerrainFPName, false, "Failed to create GPU buffers for Terrain");
            return false;
        }

        AZ::RPI::ModelLodAssetCreator modelLodAssetCreator;
        modelLodAssetCreator.Begin(AZ::Uuid::CreateRandom());

        modelLodAssetCreator.BeginMesh();
        modelLodAssetCreator.AddMeshStreamBuffer(AZ::RHI::ShaderSemantic{ "POSITION" }, AZ::Name(), {positionsOutcome.GetValue(), positionBufferViewDesc});
        modelLodAssetCreator.AddMeshStreamBuffer(AZ::RHI::ShaderSemantic{ "UV" }, AZ::Name(), {uvsOutcome.GetValue(), uvBufferViewDesc});
        modelLodAssetCreator.SetMeshIndexBuffer({indicesOutcome.GetValue(), indexBufferViewDesc});

        AZ::Aabb aabb = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0, 0.0, 0.0), AZ::Vector3(GridMeters, GridMeters, 0.0));
        modelLodAssetCreator.SetMeshAabb(AZStd::move(aabb));
        modelLodAssetCreator.SetMeshName(AZ::Name("Terrain Patch"));
        modelLodAssetCreator.EndMesh();

        AZ::Data::Asset<AZ::RPI::ModelLodAsset> modelLodAsset;
        modelLodAssetCreator.End(modelLodAsset);
        
        AZ::RPI::ModelAssetCreator modelAssetCreator;
        modelAssetCreator.Begin(AZ::Uuid::CreateRandom());
        modelAssetCreator.AddLodAsset(AZStd::move(modelLodAsset));

        AZ::Data::Asset<AZ::RPI::ModelAsset> modelAsset;
        bool success = modelAssetCreator.End(modelAsset);

        m_patchModel = AZ::RPI::Model::FindOrCreate(modelAsset);

        return success;
    }
}
