/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/TerrainFeatureProcessor.h>

#include <Atom/Utils/Utils.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/RasterPass.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/Feature/RenderCommon.h>

#include <SurfaceData/SurfaceDataSystemRequestBus.h>

#include <Atom/RPI.Reflect/Material/MaterialAssetCreator.h>

namespace Terrain
{
    namespace
    {
        [[maybe_unused]] const char* TerrainFPName = "TerrainFeatureProcessor";
        const char* TerrainHeightmapChars = "TerrainHeightmap";
    }

    namespace SceneSrgInputs
    {
        static const char* const HeightmapImage("m_heightmapImage");
        static const char* const TerrainWorldData("m_terrainWorldData");
    }

    namespace TerrainSrgInputs
    {
        static const char* const Textures("m_textures");
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
        EnableSceneNotification();

        Initialize();
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusConnect();
        
        m_handleGlobalShaderOptionUpdate = AZ::RPI::ShaderSystemInterface::GlobalShaderOptionUpdatedEvent::Handler
        {
            [this](const AZ::Name&, AZ::RPI::ShaderOptionValue) { m_forceRebuildDrawPackets = true; }
        };
        AZ::RPI::ShaderSystemInterface::Get()->Connect(m_handleGlobalShaderOptionUpdate);
    }

    void TerrainFeatureProcessor::Initialize()
    {
        m_imageArrayHandler = AZStd::make_shared<AZ::Render::BindlessImageArrayHandler>();

        auto sceneSrgLayout = AZ::RPI::RPISystemInterface::Get()->GetSceneSrgLayout();
        
        m_heightmapPropertyIndex = sceneSrgLayout->FindShaderInputImageIndex(AZ::Name(SceneSrgInputs::HeightmapImage));
        AZ_Error(TerrainFPName, m_heightmapPropertyIndex.IsValid(), "Failed to find scene srg input constant %s.", SceneSrgInputs::HeightmapImage);
        
        m_worldDataIndex = sceneSrgLayout->FindShaderInputConstantIndex(AZ::Name(SceneSrgInputs::TerrainWorldData));
        AZ_Error(TerrainFPName, m_worldDataIndex.IsValid(), "Failed to find scene srg input constant %s.", SceneSrgInputs::TerrainWorldData);

        // Load the terrain material asynchronously
        const AZStd::string materialFilePath = "Materials/Terrain/DefaultPbrTerrain.azmaterial";
        m_materialAssetLoader = AZStd::make_unique<AZ::RPI::AssetUtils::AsyncAssetLoader>();
        *m_materialAssetLoader = AZ::RPI::AssetUtils::AsyncAssetLoader::Create<AZ::RPI::MaterialAsset>(materialFilePath, 0u,
            [&](AZ::Data::Asset<AZ::Data::AssetData> assetData, bool success) -> void
            {
                const AZ::Data::Asset<AZ::RPI::MaterialAsset>& materialAsset = static_cast<AZ::Data::Asset<AZ::RPI::MaterialAsset>>(assetData);
                if (success)
                {
                    m_materialInstance = AZ::RPI::Material::FindOrCreate(assetData);
                    AZ::RPI::MaterialReloadNotificationBus::Handler::BusConnect(materialAsset->GetId());
                    if (!materialAsset->GetObjectSrgLayout())
                    {
                        AZ_Error("TerrainFeatureProcessor", false, "No per-object ShaderResourceGroup found on terrain material.");
                    }
                    else
                    {
                        PrepareMaterialData();
                    }
                }
            }
        );
        OnTerrainDataChanged(AZ::Aabb::CreateNull(), TerrainDataChangedMask::HeightData);

    }

    void TerrainFeatureProcessor::Deactivate()
    {
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusDisconnect();
        AZ::RPI::MaterialReloadNotificationBus::Handler::BusDisconnect();
        
        DisableSceneNotification();
        OnTerrainDataDestroyBegin();

        m_materialAssetLoader = {};
        m_materialInstance = {};

        m_meshManager.Reset();
        m_macroMaterialManager.Reset();
        m_detailMaterialManager.Reset();
        m_clipmapManager.Reset();
    }

    void TerrainFeatureProcessor::Render(const AZ::RPI::FeatureProcessor::RenderPacket& packet)
    {
        ProcessSurfaces(packet);
    }

    void TerrainFeatureProcessor::OnTerrainDataDestroyBegin()
    {
        m_heightmapImage = {};
        m_terrainBounds = AZ::Aabb::CreateNull();
        m_dirtyRegion = AZ::Aabb::CreateNull();
        m_heightmapNeedsUpdate = false;
    }
    
    void TerrainFeatureProcessor::OnTerrainDataChanged(const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask)
    {
        if ((dataChangedMask & (TerrainDataChangedMask::HeightData | TerrainDataChangedMask::Settings)) != 0)
        {
            TerrainHeightOrSettingsUpdated(dirtyRegion);
        }
    }

    void TerrainFeatureProcessor::TerrainHeightOrSettingsUpdated(const AZ::Aabb& dirtyRegion)
    {
        AZ::Aabb worldBounds = AZ::Aabb::CreateNull();
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            worldBounds, &AzFramework::Terrain::TerrainDataRequests::GetTerrainAabb);

        const AZ::Aabb& regionToUpdate = dirtyRegion.IsValid() ? dirtyRegion : worldBounds;

        m_dirtyRegion.AddAabb(regionToUpdate);
        m_dirtyRegion.Clamp(worldBounds);

        float queryResolution = 1.0f;
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            queryResolution, &AzFramework::Terrain::TerrainDataRequests::GetTerrainHeightQueryResolution);
        // Currently query resolution is multidimensional but the rendering system only supports this changing in one dimension.

        m_terrainBounds = worldBounds;
        m_sampleSpacing = queryResolution;
        m_heightmapNeedsUpdate = true;
    }

    void TerrainFeatureProcessor::OnRenderPipelineAdded([[maybe_unused]] AZ::RPI::RenderPipelinePtr pipeline)
    {
        CacheForwardPass();
    }

    void TerrainFeatureProcessor::OnRenderPipelinePassesChanged([[maybe_unused]] AZ::RPI::RenderPipeline* renderPipeline)
    {
        CacheForwardPass();
    }

    void TerrainFeatureProcessor::ApplyRenderPipelineChange(AZ::RPI::RenderPipeline* renderPipeline)
    {
        // Get the pass request to create terrain parent pass from the asset
        const char* passRequestAssetFilePath = "Passes/TerrainPassRequest.azasset";
        auto passRequestAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::AnyAsset>(
            passRequestAssetFilePath, AZ::RPI::AssetUtils::TraceLevel::Warning);
        const AZ::RPI::PassRequest* passRequest = nullptr;
        if (passRequestAsset->IsReady())
        {
            passRequest = passRequestAsset->GetDataAs<AZ::RPI::PassRequest>();
        }
        if (!passRequest)
        {
            AZ_Error("Terrain", false, "Failed to add terrain parent pass. Can't load PassRequest from %s", passRequestAssetFilePath);
            return;
        }

        // Return if the pass to be created already exists
        AZ::RPI::PassFilter passFilter = AZ::RPI::PassFilter::CreateWithPassName(passRequest->m_passName, renderPipeline);
        AZ::RPI::Pass* pass = AZ::RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);
        if (pass)
        {
            return;
        }

        // Create the pass
        AZ::RPI::Ptr<AZ::RPI::Pass> terrainParentPass  = AZ::RPI::PassSystemInterface::Get()->CreatePassFromRequest(passRequest);
        if (!terrainParentPass)
        {
            AZ_Error("Terrain", false, "Create terrain parent pass from pass request failed");
            return;
        }

        // Add the pass to render pipeline
        bool success = renderPipeline->AddPassBefore(terrainParentPass, AZ::Name("DepthPrePass"));
        // only create pass resources if it was success
        if (!success)
        {
            AZ_Error("Terrain", false, "Add the terrain parent pass to render pipeline [%s] failed",
                renderPipeline->GetId().GetCStr());
        }
    }

    void TerrainFeatureProcessor::UpdateHeightmapImage()
    {
        auto samplerType = AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP;
        const AZ::Vector2 stepSize(m_sampleSpacing);
        AZStd::pair<size_t, size_t> numSamples;
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            numSamples, &AzFramework::Terrain::TerrainDataRequests::GetNumSamplesFromRegion,
            m_terrainBounds, stepSize, samplerType);

        const AZ::RHI::Size heightmapSize = AZ::RHI::Size(
            aznumeric_cast<uint32_t>(numSamples.first),
            aznumeric_cast<uint32_t>(numSamples.second),
            1);

        if (!m_heightmapImage || m_heightmapImage->GetDescriptor().m_size != heightmapSize)
        {
            const AZ::Data::Instance<AZ::RPI::AttachmentImagePool> imagePool = AZ::RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();
            AZ::RHI::ImageDescriptor imageDescriptor = AZ::RHI::ImageDescriptor::Create2D(
                AZ::RHI::ImageBindFlags::ShaderRead, heightmapSize.m_width, heightmapSize.m_height, AZ::RHI::Format::R16_UNORM
            );

            const AZ::Name TerrainHeightmapName = AZ::Name(TerrainHeightmapChars);
            m_heightmapImage = AZ::RPI::AttachmentImage::Create(*imagePool.get(), imageDescriptor, TerrainHeightmapName, nullptr, nullptr);
            AZ_Error(TerrainFPName, m_heightmapImage, "Failed to initialize the heightmap image.");
            
            // World size changed, so the whole height map needs updating.
            m_dirtyRegion = m_terrainBounds;
            m_imageBindingsNeedUpdate = true;
        }

        if (!m_dirtyRegion.IsValid())
        {
            return;
        }
        
        if (m_heightmapImage)
        {
            AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                numSamples, &AzFramework::Terrain::TerrainDataRequests::GetNumSamplesFromRegion,
                m_dirtyRegion, stepSize, samplerType);
        
            const uint32_t updateWidth = aznumeric_cast<uint32_t>(numSamples.first);
            const uint32_t updateHeight = aznumeric_cast<uint32_t>(numSamples.second);

            // If there aren't any samples in the region, there's nothing to update, so just return.
            if ((updateWidth == 0) || (updateHeight == 0))
            {
                m_dirtyRegion = AZ::Aabb::CreateNull();
                return;
            }

            AZStd::vector<uint16_t> pixels;
            pixels.reserve(updateWidth * updateHeight);

            auto perPositionCallback = [this, &pixels]
                ([[maybe_unused]] size_t xIndex, [[maybe_unused]] size_t yIndex,
                const AzFramework::SurfaceData::SurfacePoint& surfacePoint,
                [[maybe_unused]] bool terrainExists)
            {
                const float clampedHeight = AZ::GetClamp((surfacePoint.m_position.GetZ() - m_terrainBounds.GetMin().GetZ()) / m_terrainBounds.GetExtents().GetZ(), 0.0f, 1.0f);
                const float expandedHeight = AZStd::roundf(clampedHeight * AZStd::numeric_limits<uint16_t>::max());
                const uint16_t uint16Height = aznumeric_cast<uint16_t>(expandedHeight);

                pixels.push_back(uint16Height);
            };

            AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                &AzFramework::Terrain::TerrainDataRequests::ProcessHeightsFromRegion,
                m_dirtyRegion, stepSize, perPositionCallback, samplerType);

            constexpr uint32_t BytesPerPixel = sizeof(uint16_t);
            const float left = AZStd::floorf(m_dirtyRegion.GetMin().GetX() / m_sampleSpacing) - AZStd::floorf(m_terrainBounds.GetMin().GetX() / m_sampleSpacing);
            const float top = AZStd::floorf(m_dirtyRegion.GetMin().GetY() / m_sampleSpacing) - AZStd::floorf(m_terrainBounds.GetMin().GetY() / m_sampleSpacing);

            AZ::RHI::ImageUpdateRequest imageUpdateRequest;
            imageUpdateRequest.m_imageSubresourcePixelOffset.m_left = aznumeric_cast<uint32_t>(left);
            imageUpdateRequest.m_imageSubresourcePixelOffset.m_top = aznumeric_cast<uint32_t>(top);
            imageUpdateRequest.m_sourceSubresourceLayout.m_bytesPerRow = updateWidth * BytesPerPixel;
            imageUpdateRequest.m_sourceSubresourceLayout.m_bytesPerImage = updateWidth * updateHeight * BytesPerPixel;
            imageUpdateRequest.m_sourceSubresourceLayout.m_rowCount = updateHeight;
            imageUpdateRequest.m_sourceSubresourceLayout.m_size = AZ::RHI::Size(updateWidth, updateHeight, 1);
            imageUpdateRequest.m_sourceData = pixels.data();
            imageUpdateRequest.m_image = m_heightmapImage->GetRHIImage();

            [[maybe_unused]] AZ::RHI::ResultCode result = m_heightmapImage->UpdateImageContents(imageUpdateRequest);
            AZ_Assert(result == AZ::RHI::ResultCode::Success, "TerrainFeatureProcessor failed to update heightmap image");
        }
        
        m_dirtyRegion = AZ::Aabb::CreateNull();
    }

    void TerrainFeatureProcessor::PrepareMaterialData()
    {
        m_terrainSrg = {};

        for (auto& shaderItem : m_materialInstance->GetShaderCollection())
        {
            if (shaderItem.GetShaderAsset()->GetDrawListName() == AZ::Name("forward"))
            {
                const auto& shaderAsset = shaderItem.GetShaderAsset();
                m_terrainSrg = AZ::RPI::ShaderResourceGroup::Create(shaderItem.GetShaderAsset(), shaderAsset->GetSupervariantIndex(AZ::Name()), AZ::Name{"TerrainSrg"});
                AZ_Error(TerrainFPName, m_terrainSrg, "Failed to create Terrain shader resource group");
                break;
            }
        }

        AZ_Error(TerrainFPName, m_terrainSrg, "Terrain Srg not found on any shader in the terrain material");

        if (m_terrainSrg)
        {
            if (m_imageArrayHandler->IsInitialized())
            {
                m_imageArrayHandler->UpdateSrgIndices(m_terrainSrg, AZ::Name(TerrainSrgInputs::Textures));
            }
            else
            {
                m_imageArrayHandler->Initialize(m_terrainSrg, AZ::Name(TerrainSrgInputs::Textures));
            }

            if (m_macroMaterialManager.IsInitialized())
            {
                m_macroMaterialManager.UpdateSrgIndices(m_terrainSrg);
            }
            else
            {
                m_macroMaterialManager.Initialize(m_imageArrayHandler, m_terrainSrg);
            }
            
            if (m_detailMaterialManager.IsInitialized())
            {
                m_detailMaterialManager.UpdateSrgIndices(m_terrainSrg);
            }
            else if(m_materialInstance)
            {
                m_detailMaterialManager.Initialize(m_imageArrayHandler, m_terrainSrg, m_materialInstance);
            }

            if (m_clipmapManager.IsInitialized())
            {
                m_clipmapManager.UpdateSrgIndices(m_terrainSrg);
            }
            else
            {
                m_clipmapManager.Initialize(m_terrainSrg);
            }
        }
        else
        {
            m_imageArrayHandler->Reset();
            m_macroMaterialManager.Reset();
            m_detailMaterialManager.Reset();
            m_clipmapManager.Reset();
        }
    }

    void TerrainFeatureProcessor::ProcessSurfaces(const FeatureProcessor::RenderPacket& process)
    {
        AZ_PROFILE_FUNCTION(AzRender);
        
        if (!m_terrainBounds.IsValid())
        {
            return;
        }

        if (m_materialInstance && m_materialInstance->CanCompile())
        {
            AZ::Vector3 cameraPosition = AZ::Vector3::CreateZero();
            for (auto& view : process.m_views)
            {
                if ((view->GetUsageFlags() & AZ::RPI::View::UsageFlags::UsageCamera) > 0)
                {
                    cameraPosition = view->GetCameraTransform().GetTranslation();
                    break;
                }
            }

            if (m_meshManager.IsInitialized())
            {
                bool surfacesRebuilt = false;
                surfacesRebuilt = m_meshManager.CheckRebuildSurfaces(m_materialInstance, *GetParentScene());
                if (m_forceRebuildDrawPackets && !surfacesRebuilt)
                {   
                    m_meshManager.RebuildDrawPackets(*GetParentScene());
                }
                m_forceRebuildDrawPackets = false;
            }

            if (m_terrainSrg)
            {
                if (m_macroMaterialManager.IsInitialized())
                {
                    m_macroMaterialManager.Update(m_terrainSrg);
                }

                if (m_detailMaterialManager.IsInitialized())
                {
                    m_detailMaterialManager.Update(cameraPosition, m_terrainSrg);
                }

                if (m_clipmapManager.IsInitialized())
                {
                    m_clipmapManager.Update(cameraPosition, m_terrainSrg);
                }
            }

            if (m_heightmapNeedsUpdate)
            {
                UpdateHeightmapImage();
                m_heightmapNeedsUpdate = false;
            }
            
            if (m_imageArrayHandler->IsInitialized())
            {
                bool result [[maybe_unused]] = m_imageArrayHandler->UpdateSrg(m_terrainSrg);
                AZ_Error(TerrainFPName, result, "Failed to set image view unbounded array into shader resource group.");
            }
        }
        
        if (m_meshManager.IsInitialized())
        {
            m_meshManager.DrawMeshes(process);
        }

        if (m_heightmapImage && m_imageBindingsNeedUpdate)
        {
            WorldShaderData worldData;
            m_terrainBounds.GetMin().StoreToFloat3(worldData.m_min.data());
            m_terrainBounds.GetMax().StoreToFloat3(worldData.m_max.data());

            m_imageBindingsNeedUpdate = false;

            auto sceneSrg = GetParentScene()->GetShaderResourceGroup();
            sceneSrg->SetImage(m_heightmapPropertyIndex, m_heightmapImage);
            sceneSrg->SetConstant(m_worldDataIndex, worldData);
        }

        if (m_materialInstance)
        {
            m_materialInstance->Compile();
        }

        if (m_terrainSrg && m_forwardPass)
        {
            m_terrainSrg->Compile();
            m_forwardPass->BindSrg(m_terrainSrg->GetRHIShaderResourceGroup());
        }
    }

    void TerrainFeatureProcessor::OnMaterialReinitialized([[maybe_unused]] const MaterialInstance& material)
    {
        PrepareMaterialData();
        m_forceRebuildDrawPackets = true;
        m_imageBindingsNeedUpdate = true;
    }

    void TerrainFeatureProcessor::SetDetailMaterialConfiguration(const DetailMaterialConfiguration& config)
    {
        m_detailMaterialManager.SetDetailMaterialConfiguration(config);
    }
    
    void TerrainFeatureProcessor::SetWorldSize([[maybe_unused]] AZ::Vector2 sizeInMeters)
    {
        // This will control the max rendering size. Actual terrain size can be much
        // larger but this will limit how much is rendered.
    }
    
    void TerrainFeatureProcessor::CacheForwardPass()
    {
        auto rasterPassFilter = AZ::RPI::PassFilter::CreateWithPassClass<AZ::RPI::RasterPass>();
        rasterPassFilter.SetOwnerScene(GetParentScene());
        AZ::RHI::RHISystemInterface* rhiSystem = AZ::RHI::RHISystemInterface::Get();
        AZ::RHI::DrawListTag forwardTag = rhiSystem->GetDrawListTagRegistry()->AcquireTag(AZ::Name("forward"));
        AZ::RPI::PassSystemInterface::Get()->ForEachPass(rasterPassFilter,
            [&](AZ::RPI::Pass* pass) -> AZ::RPI::PassFilterExecutionFlow
            {
                auto* rasterPass = azrtti_cast<AZ::RPI::RasterPass*>(pass);

                if (rasterPass && rasterPass->GetDrawListTag() == forwardTag)
                {
                    m_forwardPass = rasterPass;
                    return AZ::RPI::PassFilterExecutionFlow::StopVisitingPasses;
                }
                return AZ::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
            }
        );
    }

    const AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> TerrainFeatureProcessor::GetTerrainShaderResourceGroup() const
    {
        return m_terrainSrg;
    }

    const AZ::Data::Instance<AZ::RPI::Material> TerrainFeatureProcessor::GetMaterial() const
    {
        return m_materialInstance;
    }

    const TerrainClipmapManager& TerrainFeatureProcessor::GetClipmapManager() const
    {
        return m_clipmapManager;
    }
}
