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

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Material/MaterialAssetCreator.h>

namespace Terrain
{
    namespace
    {
        [[maybe_unused]] const char* TerrainFPName = "TerrainFeatureProcessor";
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
    }

    void TerrainFeatureProcessor::Initialize()
    {
        auto sceneSrgLayout = AZ::RPI::RPISystemInterface::Get()->GetSceneSrgLayout();

        // Load the terrain material asynchronously
        static constexpr const char* materialFilePath = "Materials/Terrain/DefaultPbrTerrain.azmaterial";
        const AZ::Data::AssetId materialAssetId =
            AZ::RPI::AssetUtils::GetAssetIdForProductPath(materialFilePath, AZ::RPI::AssetUtils::TraceLevel::Error);
        if (materialAssetId.IsValid())
        {
            m_materialAsset.Create(materialAssetId);
            m_materialAsset.QueueLoad();
            AZ::Data::AssetBus::Handler::BusConnect(materialAssetId);
        }

        OnTerrainDataChanged(
            AZ::Aabb::CreateNull(), TerrainDataChangedMask::HeightData | TerrainDataChangedMask::Settings);
        m_meshManager.Initialize(*GetParentScene());
    }

    void TerrainFeatureProcessor::Deactivate()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusDisconnect();
        
        DisableSceneNotification();
        OnTerrainDataDestroyBegin();

        m_materialAsset = {};
        m_materialInstance = {};

        m_meshManager.Reset();
        m_macroMaterialManager.Reset();
        m_detailMaterialManager.Reset();
        if (m_clipmapManager.IsClipmapEnabled())
        {
            m_clipmapManager.Reset();
        }
    }

    void TerrainFeatureProcessor::Render(const AZ::RPI::FeatureProcessor::RenderPacket& packet)
    {
        ProcessSurfaces(packet);
    }

    void TerrainFeatureProcessor::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        m_materialAsset = asset;
        if (!m_materialAsset->GetObjectSrgLayout())
        {
            AZ_Error("TerrainFeatureProcessor", false, "No per-object ShaderResourceGroup found on terrain material.");
        }
        else
        {
            m_materialInstance = AZ::RPI::Material::FindOrCreate(m_materialAsset);
            PrepareMaterialData();
            m_terrainBoundsNeedUpdate = true;
        }
    }

    void TerrainFeatureProcessor::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        OnAssetReady(asset);
    }

    void TerrainFeatureProcessor::OnTerrainDataDestroyBegin()
    {
        m_zBounds = {};
    }
    
    void TerrainFeatureProcessor::OnTerrainDataChanged([[maybe_unused]] const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask)
    {
        if ((dataChangedMask & TerrainDataChangedMask::Settings) == TerrainDataChangedMask::Settings)
        {
            AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                m_zBounds, &AzFramework::Terrain::TerrainDataRequests::GetTerrainHeightBounds);

            m_terrainBoundsNeedUpdate = true;
        }
    }

    void TerrainFeatureProcessor::OnRenderPipelineChanged([[maybe_unused]] AZ::RPI::RenderPipeline* renderPipeline,
        [[maybe_unused]] AZ::RPI::SceneNotification::RenderPipelineChangeType changeType)
    {
        CachePasses();
        if (changeType == RenderPipelineChangeType::Added || changeType == RenderPipelineChangeType::PassChanged)
        {
            m_meshManager.SetRebuildDrawPackets();
        }
    }

    void TerrainFeatureProcessor::AddRenderPasses(AZ::RPI::RenderPipeline* renderPipeline)
    {
        // Get the pass requests to create passes from the asset
        AddPassRequestToRenderPipeline(renderPipeline, "Passes/TerrainPassRequest.azasset", "DepthPrePass", true);

        // Only add debug pass if the DebugOverlayPass exists
        if (renderPipeline->FindFirstPass(AZ::Name("DebugOverlayPass")))
        {
            AddPassRequestToRenderPipeline(renderPipeline, "Passes/TerrainDebugPassRequest.azasset", "DebugOverlayPass", false);
        }
    }

    void TerrainFeatureProcessor::PrepareMaterialData()
    {
        m_terrainSrg = {};

        m_materialInstance->ForAllShaderItems(
            [&](const AZ::Name&, const AZ::RPI::ShaderCollection::Item& shaderItem)
            {
                if (shaderItem.GetShaderAsset()->GetDrawListName() == AZ::Name("forward"))
                {
                    const auto& shaderAsset = shaderItem.GetShaderAsset();
                    m_terrainSrg = AZ::RPI::ShaderResourceGroup::Create(
                        shaderItem.GetShaderAsset(), shaderAsset->GetSupervariantIndex(AZ::Name()), AZ::Name{ "TerrainSrg" });
                    AZ_Error(TerrainFPName, m_terrainSrg, "Failed to create Terrain shader resource group");

                    // skip the rest of shader items
                    return false;
                }
                return true;                     
            });

        AZ_Error(TerrainFPName, m_terrainSrg, "Terrain Srg not found on any shader in the terrain material");

        if (m_terrainSrg)
        {
            if (m_macroMaterialManager.IsInitialized())
            {
                m_macroMaterialManager.UpdateSrgIndices(m_terrainSrg);
            }
            else
            {
                m_macroMaterialManager.Initialize(m_terrainSrg);
            }
            
            if (m_detailMaterialManager.IsInitialized())
            {
                m_detailMaterialManager.UpdateSrgIndices(m_terrainSrg);
            }
            else if(m_materialInstance)
            {
                m_detailMaterialManager.Initialize(m_terrainSrg, m_materialInstance);
            }

            if (m_clipmapManager.IsClipmapEnabled())
            {
                if (m_clipmapManager.IsInitialized())
                {
                    m_clipmapManager.UpdateSrgIndices(m_terrainSrg);
                }
                else
                {
                    m_clipmapManager.Initialize(m_terrainSrg);
                }
            }
            m_meshManager.SetMaterial(m_materialInstance);
        }
        else
        {
            m_macroMaterialManager.Reset();
            m_detailMaterialManager.Reset();
            if (m_clipmapManager.IsClipmapEnabled())
            {
                m_clipmapManager.Reset();
            }
        }
    }

    void TerrainFeatureProcessor::ProcessSurfaces(const FeatureProcessor::RenderPacket& process)
    {
        AZ_PROFILE_FUNCTION(AzRender);
        
        if (m_zBounds.m_min == 0.0f && m_zBounds.m_max == 0.0f)
        {
            return;
        }

        if (m_materialInstance && m_materialInstance->CanCompile())
        {
            AZ::Vector3 cameraPosition = AZ::Vector3::CreateZero();
            AZ::RPI::ViewPtr mainView;
            for (auto& view : process.m_views)
            {
                if ((view->GetUsageFlags() & AZ::RPI::View::UsageFlags::UsageCamera) > 0)
                {
                    mainView = view;
                    cameraPosition = view->GetCameraTransform().GetTranslation();
                    break;
                }
            }

            if (m_terrainSrg)
            {
                if (m_meshManager.IsInitialized())
                {
                    m_meshManager.Update(mainView, m_terrainSrg);
                }

                if (m_macroMaterialManager.IsInitialized())
                {
                    m_macroMaterialManager.Update(mainView, m_terrainSrg);
                }

                if (m_detailMaterialManager.IsInitialized())
                {
                    m_detailMaterialManager.Update(cameraPosition, m_terrainSrg);
                }

                if (m_clipmapManager.IsClipmapEnabled())
                {
                    if (m_clipmapManager.IsInitialized())
                    {
                        m_clipmapManager.Update(cameraPosition, GetParentScene(), m_terrainSrg);
                    }
                }
            }

            if (m_meshManager.IsInitialized())
            {
                m_meshManager.DrawMeshes(process, mainView);
            }
        }

        if (m_terrainBoundsNeedUpdate)
        {
            m_terrainBoundsNeedUpdate = false;

            WorldShaderData worldData;
            worldData.m_zMin = m_zBounds.m_min;
            worldData.m_zMax = m_zBounds.m_max;
            worldData.m_zExtents = worldData.m_zMax - worldData.m_zMin;

            auto sceneSrg = GetParentScene()->GetShaderResourceGroup();
            sceneSrg->SetConstant(m_worldDataIndex, worldData);
        }

        if (m_materialInstance)
        {
            m_materialInstance->Compile();
        }

        if (m_terrainSrg && m_passes.size() > 0)
        {
            m_terrainSrg->Compile();
            for (auto* pass : m_passes)
            {
                pass->BindSrg(m_terrainSrg->GetRHIShaderResourceGroup());
            }
        }
    }

    void TerrainFeatureProcessor::SetDetailMaterialConfiguration(const DetailMaterialConfiguration& config)
    {
        m_detailMaterialManager.SetDetailMaterialConfiguration(config);
    }
    
    void TerrainFeatureProcessor::SetMeshConfiguration(const MeshConfiguration& config)
    {
        m_meshManager.SetConfiguration(config);
        m_macroMaterialManager.SetRenderDistance(config.m_renderDistance);
    }

    void TerrainFeatureProcessor::SetClipmapConfiguration(const ClipmapConfiguration& config)
    {
        m_clipmapManager.SetConfiguration(config);
    }
    
    void TerrainFeatureProcessor::CachePasses()
    {
        m_passes.clear();

        auto rasterPassFilter = AZ::RPI::PassFilter::CreateWithPassClass<AZ::RPI::RasterPass>();
        rasterPassFilter.SetOwnerScene(GetParentScene());
        AZ::RHI::RHISystemInterface* rhiSystem = AZ::RHI::RHISystemInterface::Get();
        const AZ::RHI::DrawListTag forwardTag = rhiSystem->GetDrawListTagRegistry()->AcquireTag(AZ::Name("forward"));
        const AZ::RHI::DrawListTag depthTag = rhiSystem->GetDrawListTagRegistry()->AcquireTag(AZ::Name("depth"));
        const AZ::RHI::DrawListTag shadowTag = rhiSystem->GetDrawListTagRegistry()->AcquireTag(AZ::Name("shadow"));

        AZ::RPI::PassSystemInterface::Get()->ForEachPass(rasterPassFilter,
            [&](AZ::RPI::Pass* pass) -> AZ::RPI::PassFilterExecutionFlow
            {
                auto* rasterPass = azrtti_cast<AZ::RPI::RasterPass*>(pass);

                if (rasterPass && rasterPass->GetPassState() != AZ::RPI::PassState::Orphaned)
                {
                    if (rasterPass->GetDrawListTag() == forwardTag ||
                        rasterPass->GetDrawListTag() == depthTag ||
                        rasterPass->GetDrawListTag() == shadowTag)
                    {
                        m_passes.push_back(rasterPass);
                    }
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
