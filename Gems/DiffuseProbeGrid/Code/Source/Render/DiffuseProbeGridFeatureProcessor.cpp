/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Render/DiffuseProbeGridFeatureProcessor.h>
#include <DiffuseProbeGrid_Traits_Platform.h>
#include <Atom/Feature/SpecularReflections/SpecularReflectionsFeatureProcessorInterface.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/DevicePipelineState.h>
#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>

// This component invokes shaders based on Nvidia's RTX-GI SDK.
// Please refer to "Shaders/DiffuseGlobalIllumination/Nvidia RTX-GI License.txt" for license information.

namespace AZ
{
    namespace Render
    {
        void DiffuseProbeGridFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<DiffuseProbeGridFeatureProcessor, FeatureProcessor>()
                    ->Version(1);
            }
        }

        void DiffuseProbeGridFeatureProcessor::Activate()
        {
            if (!AZ_TRAIT_DIFFUSE_GI_PASSES_SUPPORTED)
            {
                // GI is not supported on this platform
                return;
            }

            auto rayTracingDeviceMask = RHI::RHISystemInterface::Get()->GetRayTracingSupport();

            m_diffuseProbeGrids.reserve(InitialProbeGridAllocationSize);
            m_realTimeDiffuseProbeGrids.reserve(InitialProbeGridAllocationSize);

            RHI::BufferPoolDescriptor desc;
            desc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
            desc.m_bindFlags = RHI::BufferBindFlags::InputAssembly;

            m_bufferPool = aznew RHI::BufferPool;
            m_bufferPool->SetName(Name("DiffuseProbeGridBoxBufferPool"));
            [[maybe_unused]] RHI::ResultCode resultCode = m_bufferPool->Init(desc);
            AZ_Error("DiffuseProbeGridFeatureProcessor", resultCode == RHI::ResultCode::Success, "Failed to initialize buffer pool");

            // create box mesh vertices and indices
            CreateBoxMesh();

            // image pool
            {
                RHI::ImagePoolDescriptor imagePoolDesc;
                imagePoolDesc.m_bindFlags = RHI::ImageBindFlags::ShaderReadWrite | RHI::ImageBindFlags::CopyRead;

                m_probeGridRenderData.m_imagePool = aznew RHI::ImagePool;
                m_probeGridRenderData.m_imagePool->SetName(Name("DiffuseProbeGridRenderImageData"));
                [[maybe_unused]] RHI::ResultCode result = m_probeGridRenderData.m_imagePool->Init(imagePoolDesc);
                AZ_Assert(result == RHI::ResultCode::Success, "Failed to initialize output image pool");
            }

            // buffer pool
            {
                RHI::BufferPoolDescriptor bufferPoolDesc;
                bufferPoolDesc.m_bindFlags = RHI::BufferBindFlags::ShaderReadWrite;

                m_probeGridRenderData.m_bufferPool = aznew RHI::BufferPool;
                m_probeGridRenderData.m_bufferPool->SetName(Name("DiffuseProbeGridRenderBufferData"));
                [[maybe_unused]] RHI::ResultCode result = m_probeGridRenderData.m_bufferPool->Init(bufferPoolDesc);
                AZ_Assert(result == RHI::ResultCode::Success, "Failed to initialize output buffer pool");
            }

            // create image view descriptors
            m_probeGridRenderData.m_probeRayTraceImageViewDescriptor = RHI::ImageViewDescriptor::Create(DiffuseProbeGridRenderData::RayTraceImageFormat, 0, 0);
            m_probeGridRenderData.m_probeIrradianceImageViewDescriptor = RHI::ImageViewDescriptor::Create(DiffuseProbeGridRenderData::IrradianceImageFormat, 0, 0);
            m_probeGridRenderData.m_probeDistanceImageViewDescriptor = RHI::ImageViewDescriptor::Create(DiffuseProbeGridRenderData::DistanceImageFormat, 0, 0);
            m_probeGridRenderData.m_probeDataImageViewDescriptor = RHI::ImageViewDescriptor::Create(DiffuseProbeGridRenderData::ProbeDataImageFormat, 0, 0);

            // create grid data buffer descriptor
            m_probeGridRenderData.m_gridDataBufferViewDescriptor = RHI::BufferViewDescriptor::CreateStructured(0, 1, DiffuseProbeGridRenderData::GridDataBufferSize);

            // load shader
            // Note: the shader may not be available on all platforms
            Data::Instance<RPI::Shader> shader = RPI::LoadCriticalShader("Shaders/DiffuseGlobalIllumination/DiffuseProbeGridRender.azshader");
            if (shader)
            {
                m_probeGridRenderData.m_drawListTag = shader->GetDrawListTag();

                m_probeGridRenderData.m_pipelineState = aznew RPI::PipelineStateForDraw;
                m_probeGridRenderData.m_pipelineState->Init(shader); // uses default shader variant
                m_probeGridRenderData.m_pipelineState->SetInputStreamLayout(m_boxStreamLayout);
                m_probeGridRenderData.m_pipelineState->SetOutputFromScene(GetParentScene());
                m_probeGridRenderData.m_pipelineState->Finalize();

                // load object shader resource group
                m_probeGridRenderData.m_shader = shader;
                m_probeGridRenderData.m_srgLayout = shader->FindShaderResourceGroupLayout(RPI::SrgBindingSlot::Object);
                AZ_Error("DiffuseProbeGridFeatureProcessor", m_probeGridRenderData.m_srgLayout != nullptr, "Failed to find ObjectSrg layout");
                

            }

            // Load the shader that contains the scene and view SRG layout that was used by the precompiled shaders.
            // Since View and Scene can be modified by projects, we may need to copy the content to the scene and view SRGs
            // that were used when creating the precompiled shaders (to avoid a layout mismatch).
            m_sceneAndViewShader = RPI::LoadCriticalShader("Shaders/DiffuseGlobalIllumination/SceneAndViewSrgs.azshader");
            if (m_sceneAndViewShader)
            {
                if (auto sceneSrgLayout = m_sceneAndViewShader->FindShaderResourceGroupLayout(RPI::SrgBindingSlot::Scene))
                {
                    // No need to copy SRG if layout is the same
                    const RHI::ShaderResourceGroupLayout* layout = RPI::RPISystemInterface::Get()->GetSceneSrgLayout().get();
                    if (layout->GetHash() != sceneSrgLayout->GetHash())
                    {
                        m_sceneShaderResourceGroup = RPI::ShaderResourceGroup::Create(
                            m_sceneAndViewShader->GetAsset(), m_sceneAndViewShader->GetSupervariantIndex(), sceneSrgLayout->GetName());
                    }
                }
            }

            if (rayTracingDeviceMask != RHI::MultiDevice::NoDevices)
            {
                // initialize the buffer pools for the DiffuseProbeGrid visualization
                m_visualizationBufferPools = aznew RHI::RayTracingBufferPools;
                m_visualizationBufferPools->Init(rayTracingDeviceMask);

                // load probe visualization model, the BLAS will be created in OnAssetReady()

                // The asset ID for our visualization model has the ID from the lowercased relative path of the source asset
                // and a sub ID that's generated based on the asset name.
                // The asset sub id is hardcoded here because the sub id is generated based on the asset name
                // and the generation method for models currently only exists in ModelAssetBuilderComponent::CreateAssetId().
                // It isn't exposed to the engine.
                // Note that there's technically a bug where if the DiffuseProbeSphere asset hasn't been processed by the Asset
                // Processor by the time this loads, it will load the default missing asset (a cube) instead of the sphere asset
                // until the next run of the Editor. This could be fixed by using the MeshFeatureProcessor to load the asset and
                // using ConnectModelChangeEventHandler() to listen for model changes to refresh the visualization.
                // However, since that will just cause the visualization to change from a cube to a sphere on the first run of the
                // Editor, handling the edge case might be overkill.
                Data::AssetId modelAssetId = Data::AssetId(AZ::Uuid::CreateName("models/diffuseprobesphere.fbx"), 268692035);
                m_visualizationModelAsset =
                    Data::AssetManager::Instance().GetAsset<AZ::RPI::ModelAsset>(modelAssetId, Data::AssetLoadBehavior::PreLoad);

                if (m_visualizationModelAsset.GetId().IsValid())
                {
                    if (!m_visualizationModelAsset.IsReady())
                    {
                        m_visualizationModelAsset.QueueLoad();
                    }

                    Data::AssetBus::MultiHandler::BusConnect(m_visualizationModelAsset.GetId());
                }
            }

            // query buffer attachmentId
            AZStd::string uuidString = AZ::Uuid::CreateRandom().ToString<AZStd::string>();
            m_queryBufferAttachmentId = AZStd::string::format("DiffuseProbeGridQueryBuffer_%s", uuidString.c_str());

            // cache the SpecularReflectionsFeatureProcessor and SSR RayTracing state
            m_specularReflectionsFeatureProcessor = GetParentScene()->GetFeatureProcessor<SpecularReflectionsFeatureProcessorInterface>();
            if (m_specularReflectionsFeatureProcessor)
            {
                const SSROptions& ssrOptions = m_specularReflectionsFeatureProcessor->GetSSROptions();
                m_ssrRayTracingEnabled = ssrOptions.IsRayTracingEnabled();
            }

            EnableSceneNotification();
        }

        void DiffuseProbeGridFeatureProcessor::Deactivate()
        {
            if (!AZ_TRAIT_DIFFUSE_GI_PASSES_SUPPORTED)
            {
                // GI is not supported on this platform
                return;
            }

            AZ_Warning("DiffuseProbeGridFeatureProcessor", m_diffuseProbeGrids.size() == 0,
                "Deactivating the DiffuseProbeGridFeatureProcessor, but there are still outstanding probe grids probes. Components\n"
                "using DiffuseProbeGridHandles should free them before the DiffuseProbeGridFeatureProcessor is deactivated.\n"
            );

            DisableSceneNotification();

            if (m_bufferPool)
            {
                m_bufferPool.reset();
            }

            m_sceneShaderResourceGroup = nullptr;
            m_viewShaderResourceGroups.clear();
            m_sceneAndViewShader = nullptr;

            Data::AssetBus::MultiHandler::BusDisconnect();
        }

        void DiffuseProbeGridFeatureProcessor::Simulate([[maybe_unused]] const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_SCOPE(AzRender, "DiffuseProbeGridFeatureProcessor: Simulate");

            // update pipeline states
            if (m_needUpdatePipelineStates)
            {
                UpdatePipelineStates();
                m_needUpdatePipelineStates = false;
            }

            // check pending textures and connect bus for notifications
            for (auto& notificationEntry : m_notifyTextureAssets)
            {
                if (notificationEntry.m_assetId.IsValid())
                {
                    // asset already has an assetId
                    continue;
                }

                // query for the assetId
                AZ::Data::AssetId assetId;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                    assetId,
                    &AZ::Data::AssetCatalogRequests::GetAssetIdByPath,
                    notificationEntry.m_relativePath.c_str(),
                    azrtti_typeid<AZ::RPI::StreamingImageAsset>(),
                    false);

                if (assetId.IsValid())
                {
                    notificationEntry.m_assetId = assetId;
                    notificationEntry.m_asset.Create(assetId, true);
                    Data::AssetBus::MultiHandler::BusConnect(assetId);
                }
            }

            // if the volumes changed we need to re-sort the probe list
            if (m_probeGridSortRequired)
            {
                AZ_PROFILE_SCOPE(AzRender, "Sort diffuse probe grids");

                // sort the probes by descending inner volume size, so the smallest volumes are rendered last
                auto sortFn = [](AZStd::shared_ptr<DiffuseProbeGrid> const& probe1, AZStd::shared_ptr<DiffuseProbeGrid> const& probe2) -> bool
                {
                    const Obb& obb1 = probe1->GetObbWs();
                    const Obb& obb2 = probe2->GetObbWs();
                    float size1 = obb1.GetHalfLengthX() * obb1.GetHalfLengthZ() * obb1.GetHalfLengthY();
                    float size2 = obb2.GetHalfLengthX() * obb2.GetHalfLengthZ() * obb2.GetHalfLengthY();

                    return (size1 > size2);
                };

                AZStd::sort(m_diffuseProbeGrids.begin(), m_diffuseProbeGrids.end(), sortFn);
                AZStd::sort(m_realTimeDiffuseProbeGrids.begin(), m_realTimeDiffuseProbeGrids.end(), sortFn);
                m_probeGridSortRequired = false;
            }

            // call Simulate on all diffuse probe grids
            for (uint32_t probeGridIndex = 0; probeGridIndex < m_diffuseProbeGrids.size(); ++probeGridIndex)
            {
                AZStd::shared_ptr<DiffuseProbeGrid>& diffuseProbeGrid = m_diffuseProbeGrids[probeGridIndex];
                AZ_Assert(diffuseProbeGrid.use_count() > 1, "DiffuseProbeGrid found with no corresponding owner, ensure that RemoveProbe() is called before releasing probe handles");

                diffuseProbeGrid->Simulate(probeGridIndex);
            }

            if (m_specularReflectionsFeatureProcessor)
            {
                const SSROptions& ssrOptions = m_specularReflectionsFeatureProcessor->GetSSROptions();
                if (m_ssrRayTracingEnabled != ssrOptions.IsRayTracingEnabled())
                {
                    m_ssrRayTracingEnabled = ssrOptions.IsRayTracingEnabled();

                    AZStd::vector<Name> passHierarchy = { Name("ReflectionScreenSpacePass"), Name("DiffuseProbeGridQueryFullscreenWithAlbedoPass") };
                    RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassHierarchy(passHierarchy);
                    RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [this](RPI::Pass* pass) -> RPI::PassFilterExecutionFlow
                        {
                            pass->SetEnabled(m_ssrRayTracingEnabled);
                            return RPI::PassFilterExecutionFlow::StopVisitingPasses;
                        });
                }
            }
        }

        void DiffuseProbeGridFeatureProcessor::OnBeginPrepareRender()
        {
            for (auto& diffuseProbeGrid : m_realTimeDiffuseProbeGrids)
            {
                diffuseProbeGrid->ResetCullingVisibility();
            }

            // build the query buffer for the irradiance queries (if any)
            if (m_irradianceQueries.size())
            {
                m_queryBuffer.AdvanceCurrentBufferAndUpdateData(m_irradianceQueries);

                // create the bufferview descriptor with the new number of elements
                m_queryBufferViewDescriptor = m_queryBuffer.GetCurrentBuffer()->GetBufferViewDescriptor();
            }

            // The passes in the DiffuseProbeGrid use precompiled shaders, so we can't use the View or Scene SRG directly because the layout
            // may not match with the layout used when creating the precompiled shaders. We need to copy the shader inputs
            // from the view/scene SRG into the SRG that was created from the shader asset.
            if (m_sceneShaderResourceGroup)
            {
                const auto sceneSrg = GetParentScene()->GetShaderResourceGroup();
                m_sceneShaderResourceGroup->CopyShaderResourceGroupData(*sceneSrg);
                m_sceneShaderResourceGroup->Compile();
            }

            // Copy the content from the view SRGs
            for (const auto& pipelineEntry : m_viewShaderResourceGroups)
            {
                RPI::RenderPipeline* pipeline = pipelineEntry.first;
                for (const auto& viewEntry : pipelineEntry.second)
                {
                    Data::Instance<RPI::ShaderResourceGroup> viewSRG = viewEntry.second;
                    RPI::ViewPtr view = pipeline->GetFirstView(viewEntry.first);
                    if (view)
                    {
                        viewSRG->CopyShaderResourceGroupData(*view->GetShaderResourceGroup());
                        viewSRG->Compile();
                    }
                }
            }
        }

        void DiffuseProbeGridFeatureProcessor::OnEndPrepareRender()
        {
            // re-build the list of visible diffuse probe grids
            m_visibleDiffuseProbeGrids.clear();
            m_visibleRealTimeDiffuseProbeGrids.clear();
            for (auto& diffuseProbeGrid : m_diffuseProbeGrids)
            {
                if (diffuseProbeGrid->GetIsVisible())
                {
                    if (diffuseProbeGrid->GetMode() == DiffuseProbeGridMode::RealTime)
                    {
                        m_visibleRealTimeDiffuseProbeGrids.push_back(diffuseProbeGrid);
                    }

                    m_visibleDiffuseProbeGrids.push_back(diffuseProbeGrid);
                }
            }
        }

        DiffuseProbeGridHandle DiffuseProbeGridFeatureProcessor::AddProbeGrid(const AZ::Transform& transform, const AZ::Vector3& extents, const AZ::Vector3& probeSpacing)
        {
            AZStd::shared_ptr<DiffuseProbeGrid> diffuseProbeGrid = AZStd::make_shared<DiffuseProbeGrid>();
            diffuseProbeGrid->Init(GetParentScene(), &m_probeGridRenderData);
            diffuseProbeGrid->SetTransform(transform);
            diffuseProbeGrid->SetExtents(extents);
            diffuseProbeGrid->SetProbeSpacing(probeSpacing);
            m_diffuseProbeGrids.push_back(diffuseProbeGrid);

            UpdateRealTimeList(diffuseProbeGrid);

            m_probeGridSortRequired = true;

            return diffuseProbeGrid;
        }

        void DiffuseProbeGridFeatureProcessor::RemoveProbeGrid(DiffuseProbeGridHandle& probeGrid)
        {
            AZ_Assert(probeGrid.get(), "RemoveProbeGrid called with an invalid handle");

            // remove from main list
            auto itEntry = AZStd::find_if(m_diffuseProbeGrids.begin(), m_diffuseProbeGrids.end(), [&](AZStd::shared_ptr<DiffuseProbeGrid> const& entry)
            {
                return (entry == probeGrid);
            });

            AZ_Assert(itEntry != m_diffuseProbeGrids.end(), "RemoveProbeGrid called with a probe grid that is not in the probe list");
            m_diffuseProbeGrids.erase(itEntry);

            // remove from side list of real-time grids
            itEntry = AZStd::find_if(m_realTimeDiffuseProbeGrids.begin(), m_realTimeDiffuseProbeGrids.end(), [&](AZStd::shared_ptr<DiffuseProbeGrid> const& entry)
            {
                return (entry == probeGrid);
            });

            if (itEntry != m_realTimeDiffuseProbeGrids.end())
            {
                m_realTimeDiffuseProbeGrids.erase(itEntry);
            }

            // remove from side list of visible grids
            itEntry = AZStd::find_if(m_visibleDiffuseProbeGrids.begin(), m_visibleDiffuseProbeGrids.end(), [&](AZStd::shared_ptr<DiffuseProbeGrid> const& entry)
            {
                return (entry == probeGrid);
            });

            if (itEntry != m_visibleDiffuseProbeGrids.end())
            {
                m_visibleDiffuseProbeGrids.erase(itEntry);
            }

            // remove from side list of visible real-time grids
            itEntry = AZStd::find_if(m_visibleRealTimeDiffuseProbeGrids.begin(), m_visibleRealTimeDiffuseProbeGrids.end(), [&](AZStd::shared_ptr<DiffuseProbeGrid> const& entry)
            {
                return (entry == probeGrid);
            });

            if (itEntry != m_visibleRealTimeDiffuseProbeGrids.end())
            {
                m_visibleRealTimeDiffuseProbeGrids.erase(itEntry);
            }

            probeGrid = nullptr;
        }

        bool DiffuseProbeGridFeatureProcessor::ValidateExtents(const DiffuseProbeGridHandle& probeGrid, const AZ::Vector3& newExtents)
        {
            AZ_Assert(probeGrid.get(), "SetTransform called with an invalid handle");
            return probeGrid->ValidateExtents(newExtents);
        }

        void DiffuseProbeGridFeatureProcessor::SetExtents(const DiffuseProbeGridHandle& probeGrid, const AZ::Vector3& extents)
        {
            AZ_Assert(probeGrid.get(), "SetExtents called with an invalid handle");
            probeGrid->SetExtents(extents);
            m_probeGridSortRequired = true;
        }

        void DiffuseProbeGridFeatureProcessor::SetTransform(const DiffuseProbeGridHandle& probeGrid, const AZ::Transform& transform)
        {
            AZ_Assert(probeGrid.get(), "SetTransform called with an invalid handle");
            probeGrid->SetTransform(transform);
            m_probeGridSortRequired = true;
        }

        bool DiffuseProbeGridFeatureProcessor::ValidateProbeSpacing(const DiffuseProbeGridHandle& probeGrid, const AZ::Vector3& newSpacing)
        {
            AZ_Assert(probeGrid.get(), "SetTransform called with an invalid handle");
            return probeGrid->ValidateProbeSpacing(newSpacing);
        }

        void DiffuseProbeGridFeatureProcessor::SetProbeSpacing(const DiffuseProbeGridHandle& probeGrid, const AZ::Vector3& probeSpacing)
        {
            AZ_Assert(probeGrid.get(), "SetProbeSpacing called with an invalid handle");
            probeGrid->SetProbeSpacing(probeSpacing);
        }

        void DiffuseProbeGridFeatureProcessor::SetViewBias(const DiffuseProbeGridHandle& probeGrid, float viewBias)
        {
            AZ_Assert(probeGrid.get(), "SetViewBias called with an invalid handle");
            probeGrid->SetViewBias(viewBias);
        }

        void DiffuseProbeGridFeatureProcessor::SetNormalBias(const DiffuseProbeGridHandle& probeGrid, float normalBias)
        {
            AZ_Assert(probeGrid.get(), "SetNormalBias called with an invalid handle");
            probeGrid->SetNormalBias(normalBias);
        }

        void DiffuseProbeGridFeatureProcessor::SetNumRaysPerProbe(const DiffuseProbeGridHandle& probeGrid, DiffuseProbeGridNumRaysPerProbe numRaysPerProbe)
        {
            AZ_Assert(probeGrid.get(), "SetNumRaysPerProbe called with an invalid handle");
            probeGrid->SetNumRaysPerProbe(numRaysPerProbe);
        }

        void DiffuseProbeGridFeatureProcessor::SetAmbientMultiplier(const DiffuseProbeGridHandle& probeGrid, float ambientMultiplier)
        {
            AZ_Assert(probeGrid.get(), "SetAmbientMultiplier called with an invalid handle");
            probeGrid->SetAmbientMultiplier(ambientMultiplier);
        }

        void DiffuseProbeGridFeatureProcessor::Enable(const DiffuseProbeGridHandle& probeGrid, bool enable)
        {
            AZ_Assert(probeGrid.get(), "Enable called with an invalid handle");
            probeGrid->Enable(enable);
        }

        void DiffuseProbeGridFeatureProcessor::SetGIShadows(const DiffuseProbeGridHandle& probeGrid, bool giShadows)
        {
            AZ_Assert(probeGrid.get(), "SetGIShadows called with an invalid handle");
            probeGrid->SetGIShadows(giShadows);
        }

        void DiffuseProbeGridFeatureProcessor::SetUseDiffuseIbl(const DiffuseProbeGridHandle& probeGrid, bool useDiffuseIbl)
        {
            AZ_Assert(probeGrid.get(), "SetUseDiffuseIbl called with an invalid handle");
            probeGrid->SetUseDiffuseIbl(useDiffuseIbl);
        }

        bool DiffuseProbeGridFeatureProcessor::CanBakeTextures()
        {
            return RHI::RHISystemInterface::Get()->GetRayTracingSupport() != RHI::MultiDevice::NoDevices;
        }

        void DiffuseProbeGridFeatureProcessor::BakeTextures(
            const DiffuseProbeGridHandle& probeGrid,
            DiffuseProbeGridBakeTexturesCallback callback,
            const AZStd::string& irradianceTextureRelativePath,
            const AZStd::string& distanceTextureRelativePath,
            const AZStd::string& probeDataTextureRelativePath)
        {
            AZ_Assert(probeGrid.get(), "BakeTextures called with an invalid handle");

            AddNotificationEntry(irradianceTextureRelativePath);
            AddNotificationEntry(distanceTextureRelativePath);
            AddNotificationEntry(probeDataTextureRelativePath);

            probeGrid->GetTextureReadback().BeginTextureReadback(callback);
        }

        void DiffuseProbeGridFeatureProcessor::UpdateRealTimeList(const DiffuseProbeGridHandle& diffuseProbeGrid)
        {
            if (diffuseProbeGrid->GetMode() == DiffuseProbeGridMode::RealTime)
            {
                // add to side list of real-time grids
                auto itEntry = AZStd::find_if(m_realTimeDiffuseProbeGrids.begin(), m_realTimeDiffuseProbeGrids.end(), [&](AZStd::shared_ptr<DiffuseProbeGrid> const& entry)
                {
                    return (entry == diffuseProbeGrid);
                });

                if (itEntry == m_realTimeDiffuseProbeGrids.end())
                {
                    m_realTimeDiffuseProbeGrids.push_back(diffuseProbeGrid);
                }
            }
            else
            {
                // remove from side list of real-time grids
                auto itEntry = AZStd::find_if(m_realTimeDiffuseProbeGrids.begin(), m_realTimeDiffuseProbeGrids.end(), [&](AZStd::shared_ptr<DiffuseProbeGrid> const& entry)
                {
                    return (entry == diffuseProbeGrid);
                });

                if (itEntry != m_realTimeDiffuseProbeGrids.end())
                {
                    m_realTimeDiffuseProbeGrids.erase(itEntry);
                }
            }
        }

        void DiffuseProbeGridFeatureProcessor::AddNotificationEntry(const AZStd::string& relativePath)
        {
            AZStd::string assetPath = relativePath + ".streamingimage";

            // check to see if this is an existing asset
            AZ::Data::AssetId assetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                assetId,
                &AZ::Data::AssetCatalogRequests::GetAssetIdByPath,
                assetPath.c_str(),
                azrtti_typeid<AZ::RPI::StreamingImageAsset>(),
                false);

            // We only track notifications for new texture assets, meaning assets that are created the first time a DiffuseProbeGrid is baked.
            // On subsequent bakes the existing assets are automatically reloaded by the RPI since they are already known by the asset system.
            if (!assetId.IsValid())
            {
                m_notifyTextureAssets.push_back({ assetPath, assetId });
            }
        }

        bool DiffuseProbeGridFeatureProcessor::CheckTextureAssetNotification(
            const AZStd::string& relativePath,
            Data::Asset<RPI::StreamingImageAsset>& outTextureAsset,
            DiffuseProbeGridTextureNotificationType& outNotificationType)
        {
            for (NotifyTextureAssetVector::iterator itNotification = m_notifyTextureAssets.begin(); itNotification != m_notifyTextureAssets.end(); ++itNotification)
            {
                if (itNotification->m_relativePath == relativePath)
                {
                    outNotificationType = itNotification->m_notificationType;
                    if (outNotificationType != DiffuseProbeGridTextureNotificationType::None)
                    {
                        outTextureAsset = itNotification->m_asset;
                        m_notifyTextureAssets.erase(itNotification);
                    }

                    return true;
                }
            }

            return false;
        }

        bool DiffuseProbeGridFeatureProcessor::AreBakedTexturesReferenced(
            const AZStd::string& irradianceTextureRelativePath,
            const AZStd::string& distanceTextureRelativePath,
            const AZStd::string& probeDataTextureRelativePath)
        {
            for (auto& diffuseProbeGrid : m_diffuseProbeGrids)
            {
                if ((diffuseProbeGrid->GetBakedIrradianceRelativePath() == irradianceTextureRelativePath) ||
                    (diffuseProbeGrid->GetBakedDistanceRelativePath() == distanceTextureRelativePath) ||
                    (diffuseProbeGrid->GetBakedProbeDataRelativePath() == probeDataTextureRelativePath))
                {
                    return true;
                }
            }

            return false;
        }

        void DiffuseProbeGridFeatureProcessor::SetMode(const DiffuseProbeGridHandle& probeGrid, DiffuseProbeGridMode mode)
        {
            AZ_Assert(probeGrid.get(), "SetMode called with an invalid handle");
            probeGrid->SetMode(mode);

            UpdateRealTimeList(probeGrid);

            m_probeGridSortRequired = true;
        }

        void DiffuseProbeGridFeatureProcessor::SetScrolling(const DiffuseProbeGridHandle& probeGrid, bool scrolling)
        {
            AZ_Assert(probeGrid.get(), "SetScrolling called with an invalid handle");
            probeGrid->SetScrolling(scrolling);
        }

        void DiffuseProbeGridFeatureProcessor::SetEdgeBlendIbl(const DiffuseProbeGridHandle& probeGrid, bool edgeBlendIbl)
        {
            AZ_Assert(probeGrid.get(), "SetEdgeBlendIbl called with an invalid handle");
            probeGrid->SetEdgeBlendIbl(edgeBlendIbl);
        }

        void DiffuseProbeGridFeatureProcessor::SetFrameUpdateCount(const DiffuseProbeGridHandle& probeGrid, uint32_t frameUpdateCount)
        {
            AZ_Assert(probeGrid.get(), "SetFrameUpdateCount called with an invalid handle");
            probeGrid->SetFrameUpdateCount(frameUpdateCount);
        }

        void DiffuseProbeGridFeatureProcessor::SetTransparencyMode(const DiffuseProbeGridHandle& probeGrid, DiffuseProbeGridTransparencyMode transparencyMode)
        {
            AZ_Assert(probeGrid.get(), "SetTransparencyMode called with an invalid handle");
            probeGrid->SetTransparencyMode(transparencyMode);
        }

        void DiffuseProbeGridFeatureProcessor::SetEmissiveMultiplier(const DiffuseProbeGridHandle& probeGrid, float emissiveMultiplier)
        {
            AZ_Assert(probeGrid.get(), "SetEmissiveMultiplier called with an invalid handle");
            probeGrid->SetEmissiveMultiplier(emissiveMultiplier);
        }

        void DiffuseProbeGridFeatureProcessor::SetBakedTextures(const DiffuseProbeGridHandle& probeGrid, const DiffuseProbeGridBakedTextures& bakedTextures)
        {
            AZ_Assert(probeGrid.get(), "SetBakedTextures called with an invalid handle");
            probeGrid->SetBakedTextures(bakedTextures);
        }

        void DiffuseProbeGridFeatureProcessor::SetVisualizationEnabled(const DiffuseProbeGridHandle& probeGrid, bool visualizationEnabled)
        {
            AZ_Assert(probeGrid.get(), "SetVisualizationEnabled called with an invalid handle");
            probeGrid->SetVisualizationEnabled(visualizationEnabled);
        }

        void DiffuseProbeGridFeatureProcessor::SetVisualizationShowInactiveProbes(const DiffuseProbeGridHandle& probeGrid, bool visualizationShowInactiveProbes)
        {
            AZ_Assert(probeGrid.get(), "SetVisualizationShowInactiveProbes called with an invalid handle");
            probeGrid->SetVisualizationShowInactiveProbes(visualizationShowInactiveProbes);
        }

        void DiffuseProbeGridFeatureProcessor::SetVisualizationSphereRadius(const DiffuseProbeGridHandle& probeGrid, float visualizationSphereRadius)
        {
            AZ_Assert(probeGrid.get(), "SetVisualizationSphereRadius called with an invalid handle");
            probeGrid->SetVisualizationSphereRadius(visualizationSphereRadius);
        }

        uint32_t DiffuseProbeGridFeatureProcessor::AddIrradianceQuery(const AZ::Vector3& position, const AZ::Vector3& direction)
        {
            m_irradianceQueries.push_back({ position, direction });
            return aznumeric_cast<uint32_t>(m_irradianceQueries.size()) - 1;
        }

        void DiffuseProbeGridFeatureProcessor::ClearIrradianceQueries()
        {
            m_irradianceQueries.clear();
        }

        RPI::ShaderResourceGroup* DiffuseProbeGridFeatureProcessor::GetSceneSrg() const
        {
            return m_sceneShaderResourceGroup.get();
        }

        RPI::ShaderResourceGroup* DiffuseProbeGridFeatureProcessor::GetViewSrg(
            RPI::RenderPipeline* pipeline, RPI::PipelineViewTag viewTag) const
        {
            auto findPipelineIter = m_viewShaderResourceGroups.find(pipeline);
            if (findPipelineIter != m_viewShaderResourceGroups.end())
            {
                auto findViewTagIter = findPipelineIter->second.find(viewTag);
                if (findViewTagIter != findPipelineIter->second.end())
                {
                    return findViewTagIter->second.get();
                }
            }
            return nullptr;
        }

        void DiffuseProbeGridFeatureProcessor::CreateBoxMesh()
        {
            // vertex positions
            static const Position positions[] =
            {
                // front
                { -0.5f, -0.5f,  0.5f },
                {  0.5f, -0.5f,  0.5f },
                {  0.5f,  0.5f,  0.5f },
                { -0.5f,  0.5f,  0.5f },

                // back
                { -0.5f, -0.5f, -0.5f },
                {  0.5f, -0.5f, -0.5f },
                {  0.5f,  0.5f, -0.5f },
                { -0.5f,  0.5f, -0.5f },

                // left
                { -0.5f, -0.5f,  0.5f },
                { -0.5f,  0.5f,  0.5f },
                { -0.5f,  0.5f, -0.5f },
                { -0.5f, -0.5f, -0.5f },

                // right
                {  0.5f, -0.5f,  0.5f },
                {  0.5f,  0.5f,  0.5f },
                {  0.5f,  0.5f, -0.5f },
                {  0.5f, -0.5f, -0.5f },

                // bottom
                { -0.5f, -0.5f,  0.5f },
                {  0.5f, -0.5f,  0.5f },
                {  0.5f, -0.5f, -0.5f },
                { -0.5f, -0.5f, -0.5f },

                // top
                { -0.5f,  0.5f,  0.5f },
                {  0.5f,  0.5f,  0.5f },
                {  0.5f,  0.5f, -0.5f },
                { -0.5f,  0.5f, -0.5f },
            };
            static const u32 numPositions = sizeof(positions) / sizeof(positions[0]);

            for (u32 i = 0; i < numPositions; ++i)
            {
                m_boxPositions.push_back(positions[i]);
            }

            // indices
            static const uint16_t indices[] =
            {
                // front
                0, 1, 2, 2, 3, 0,

                // back
                5, 4, 7, 7, 6, 5,

                // left
                8, 9, 10, 10, 11, 8,

                // right
                14, 13, 12, 12, 15, 14,

                // bottom
                18, 17, 16, 16, 19, 18,

                // top
                23, 20, 21, 21, 22, 23
            };
            static const u32 numIndices = sizeof(indices) / sizeof(indices[0]);

            for (u32 i = 0; i < numIndices; ++i)
            {
                m_boxIndices.push_back(indices[i]);
            }

            // create stream layout
            RHI::InputStreamLayoutBuilder layoutBuilder;
            layoutBuilder.AddBuffer()->Channel("POSITION", RHI::Format::R32G32B32_FLOAT);
            layoutBuilder.SetTopology(RHI::PrimitiveTopology::TriangleList);
            m_boxStreamLayout = layoutBuilder.End();

            // create index buffer
            AZ::RHI::BufferInitRequest request;
            m_boxIndexBuffer = aznew RHI::Buffer;
            request.m_buffer = m_boxIndexBuffer.get();
            request.m_descriptor = AZ::RHI::BufferDescriptor{ AZ::RHI::BufferBindFlags::InputAssembly, m_boxIndices.size() * sizeof(uint16_t) };
            request.m_initialData = m_boxIndices.data();
            [[maybe_unused]] AZ::RHI::ResultCode result = m_bufferPool->InitBuffer(request);
            AZ_Error("DiffuseProbeGridFeatureProcessor", result == RHI::ResultCode::Success, "Failed to initialize box index buffer - error [%d]", result);

            // create index buffer view
            AZ::RHI::IndexBufferView indexBufferView =
            {
                *m_boxIndexBuffer,
                0,
                sizeof(indices),
                AZ::RHI::IndexFormat::Uint16,
            };
            m_probeGridRenderData.m_geometryView.SetIndexBufferView(indexBufferView);
            m_probeGridRenderData.m_geometryView.SetDrawArguments(RHI::DrawIndexed{ 0, numIndices, 0 });

            // create position buffer
            m_boxPositionBuffer = aznew RHI::Buffer;
            request.m_buffer = m_boxPositionBuffer.get();
            request.m_descriptor = AZ::RHI::BufferDescriptor{ AZ::RHI::BufferBindFlags::InputAssembly, m_boxPositions.size() * sizeof(Position) };
            request.m_initialData = m_boxPositions.data();
            result = m_bufferPool->InitBuffer(request);
            AZ_Error("DiffuseProbeGridFeatureProcessor", result == RHI::ResultCode::Success, "Failed to initialize box index buffer - error [%d]", result);

            // create position buffer view
            RHI::StreamBufferView positionBufferView =
            {
                *m_boxPositionBuffer,
                0,
                (uint32_t)(m_boxPositions.size() * sizeof(Position)),
                sizeof(Position),
            };
            m_probeGridRenderData.m_geometryView.ClearStreamBufferViews();
            m_probeGridRenderData.m_geometryView.AddStreamBufferView(positionBufferView);

            AZ::RHI::ValidateStreamBufferViews(m_boxStreamLayout, m_probeGridRenderData.m_geometryView.GetStreamBufferViews());
        }

        void DiffuseProbeGridFeatureProcessor::OnRenderPipelineChanged(RPI::RenderPipeline* renderPipeline,
                RPI::SceneNotification::RenderPipelineChangeType changeType)
        {
            if (changeType == RPI::SceneNotification::RenderPipelineChangeType::PassChanged)
            {
                // change the attachment on the AuxGeom pass to use the output of the visualization pass
                RPI::PassFilter auxGeomPassFilter = RPI::PassFilter::CreateWithPassName(AZ::Name("AuxGeomPass"), renderPipeline);
                RPI::Pass* auxGeomPass = RPI::PassSystemInterface::Get()->FindFirstPass(auxGeomPassFilter);
                RPI::PassFilter visualizationPassFilter = RPI::PassFilter::CreateWithPassName(AZ::Name("DiffuseProbeGridVisualizationPass"), renderPipeline);
                RPI::Pass* visualizationPass = RPI::PassSystemInterface::Get()->FindFirstPass(visualizationPassFilter);

                if (auxGeomPass && visualizationPass && visualizationPass->GetInputOutputCount())
                {
                    RPI::PassAttachmentBinding& visualizationBinding = visualizationPass->GetInputOutputBinding(0);
                    RPI::PassAttachmentBinding* auxGeomBinding = auxGeomPass->FindAttachmentBinding(AZ::Name("ColorInputOutput"));
                    if (auxGeomBinding)
                    {
                        auxGeomBinding->SetAttachment(visualizationBinding.GetAttachment());
                    }
                }

                UpdatePasses();
            }
            else if (changeType == RPI::SceneNotification::RenderPipelineChangeType::Removed)
            {
                m_viewShaderResourceGroups.erase(renderPipeline);
            }
            m_needUpdatePipelineStates = true;
        }

        void DiffuseProbeGridFeatureProcessor::OnRenderPipelinePersistentViewChanged(
            RPI::RenderPipeline* renderPipeline, RPI::PipelineViewTag viewTag, RPI::ViewPtr newView, RPI::ViewPtr previousView)
        {
            if (m_sceneAndViewShader)
            {
                if (auto viewSrgLayout = m_sceneAndViewShader->FindShaderResourceGroupLayout(RPI::SrgBindingSlot::View))
                {
                    // No need to copy view SRG data if the layout is the same
                    const RHI::ShaderResourceGroupLayout* layout = RPI::RPISystemInterface::Get()->GetViewSrgLayout().get();
                    if (layout->GetHash() != viewSrgLayout->GetHash())
                    {
                        auto& viewSRGs = m_viewShaderResourceGroups[renderPipeline];
                        if (newView)
                        {
                            // Create a new SRG for the viewTag that is being added
                            auto viewSRG = RPI::ShaderResourceGroup::Create(
                                m_sceneAndViewShader->GetAsset(), m_sceneAndViewShader->GetSupervariantIndex(), viewSrgLayout->GetName());
                            viewSRGs[viewTag] = viewSRG;
                        }
                        else
                        {
                            // Remove the SRG since the view is being removed
                            viewSRGs.erase(viewTag);
                        }
                    }
                }
            }
        }

        void DiffuseProbeGridFeatureProcessor::AddRenderPasses(AZ::RPI::RenderPipeline* renderPipeline)
        {
            // only add to this pipeline if it contains the DiffuseGlobalFullscreen pass
            RPI::PassFilter diffuseGlobalFullscreenPassFilter = RPI::PassFilter::CreateWithPassName(AZ::Name("DiffuseGlobalFullscreenPass"), renderPipeline);
            RPI::Pass* diffuseGlobalFullscreenPass = RPI::PassSystemInterface::Get()->FindFirstPass(diffuseGlobalFullscreenPassFilter);
            if (!diffuseGlobalFullscreenPass)
            {
                return;
            }

            // check to see if the DiffuseProbeGrid passes were already added
            RPI::PassFilter diffuseProbeGridUpdatePassFilter = RPI::PassFilter::CreateWithPassName(AZ::Name("DiffuseProbeGridUpdatePass"), renderPipeline);
            RPI::Pass* diffuseProbeGridUpdatePass = RPI::PassSystemInterface::Get()->FindFirstPass(diffuseProbeGridUpdatePassFilter);

            if (!diffuseProbeGridUpdatePass)
            {
                AddPassRequest(renderPipeline, "Passes/DiffuseProbeGridPreparePassRequest.azasset", "DepthPrePass");
                AddPassRequest(renderPipeline, "Passes/DiffuseProbeGridUpdatePassRequest.azasset", "DiffuseProbeGridPreparePass");
                AddPassRequest(renderPipeline, "Passes/DiffuseProbeGridRenderPassRequest.azasset", "ForwardSubsurface");

                // add the fullscreen query pass for SSR raytracing fallback color
                AddPassRequest(renderPipeline, "Passes/DiffuseProbeGridScreenSpaceReflectionsQueryPassRequest.azasset", "ReflectionScreenSpaceRayTracingPass");

                // only add the visualization pass if there's an AuxGeom pass in the pipeline
                RPI::PassFilter auxGeomPassFilter = RPI::PassFilter::CreateWithPassName(AZ::Name("AuxGeomPass"), renderPipeline);
                RPI::Pass* auxGeomPass = RPI::PassSystemInterface::Get()->FindFirstPass(auxGeomPassFilter);
                if (auxGeomPass)
                {
                    AddPassRequest(renderPipeline, "Passes/DiffuseProbeGridVisualizationPassRequest.azasset", "PostProcessPass");
                }

                // disable the DiffuseGlobalFullscreenPass if it exists, since it is replaced with a DiffuseProbeGrid composite pass
                diffuseGlobalFullscreenPass->SetEnabled(false);
            }

            UpdatePasses();
            m_needUpdatePipelineStates = true;
        }

        void DiffuseProbeGridFeatureProcessor::AddPassRequest(RPI::RenderPipeline* renderPipeline, const char* passRequestAssetFilePath, const char* insertionPointPassName)
        {
            auto passRequestAsset = RPI::AssetUtils::LoadAssetByProductPath<RPI::AnyAsset>(passRequestAssetFilePath, RPI::AssetUtils::TraceLevel::Warning);

            // load pass request from the asset
            const RPI::PassRequest* passRequest = nullptr;
            if (passRequestAsset->IsReady())
            {
                passRequest = passRequestAsset->GetDataAs<RPI::PassRequest>();
            }

            if (!passRequest)
            {
                AZ_Error("DiffuseProbeGridFeatureProcessor", false, "Failed to load PassRequest asset [%s]", passRequestAssetFilePath);
                return;
            }

            // check to see if the pass already exists
            RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassName(passRequest->m_passName, renderPipeline);
            RPI::Pass* existingPass = RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);
            if (existingPass)
            {
                return;
            }

            // create tha pass from the request
            RPI::Ptr<RPI::Pass> newPass = RPI::PassSystemInterface::Get()->CreatePassFromRequest(passRequest);
            if (!newPass)
            {
                AZ_Error("DiffuseProbeGridFeatureProcessor", false, "Failed to create pass from pass request [%s]", passRequest->m_passName.GetCStr());
                return;
            }

            // Add the pass to render pipeline
            bool success = renderPipeline->AddPassAfter(newPass, AZ::Name(insertionPointPassName));
            if (!success)
            {
                AZ_Warning("DiffuseProbeGridFeatureProcessor", false, "Failed to add pass [%s] to render pipeline [%s]", newPass->GetName().GetCStr(), renderPipeline->GetId().GetCStr());
            }
        }

        void DiffuseProbeGridFeatureProcessor::UpdatePipelineStates()
        {
            if (m_probeGridRenderData.m_pipelineState)
            {
                m_probeGridRenderData.m_pipelineState->SetOutputFromScene(GetParentScene());
                m_probeGridRenderData.m_pipelineState->Finalize();
            }
        }

        void DiffuseProbeGridFeatureProcessor::UpdatePasses()
        {
            // disable the DiffuseProbeGridUpdatePass if the platform does not support raytracing
            if (RHI::RHISystemInterface::Get()->GetRayTracingSupport() == RHI::MultiDevice::NoDevices)
            {
                RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassName(AZ::Name("DiffuseProbeGridUpdatePass"), GetParentScene());
                RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [](RPI::Pass* pass) -> RPI::PassFilterExecutionFlow
                    {
                        pass->SetEnabled(false);
                         return RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                    });
            }
        }

        void DiffuseProbeGridFeatureProcessor::OnVisualizationModelAssetReady(Data::Asset<Data::AssetData> asset)
        {
            m_visualizationModel = RPI::Model::FindOrCreate(asset);
            AZ_Assert(m_visualizationModel.get(), "Failed to load DiffuseProbeGrid visualization model");
            if (!m_visualizationModel)
            {
                return;
            }

            const AZStd::span<const Data::Instance<RPI::ModelLod>>& modelLods = m_visualizationModel->GetLods();
            AZ_Assert(!modelLods.empty(), "Invalid DiffuseProbeGrid visualization model");
            if (modelLods.empty())
            {
                return;
            }

            const Data::Instance<RPI::ModelLod>& modelLod = modelLods[0];
            AZ_Assert(!modelLod->GetMeshes().empty(), "Invalid DiffuseProbeGrid visualization model asset");
            if (modelLod->GetMeshes().empty())
            {
                return;
            }

            const auto meshes = modelLod->GetMeshes();
            const RPI::ModelLod::Mesh& mesh = meshes[0];

            // setup a stream layout and shader input contract for the position vertex stream
            static const char* PositionSemantic = "POSITION";
            static const RHI::Format PositionStreamFormat = RHI::Format::R32G32B32_FLOAT;

            RPI::ShaderInputContract::StreamChannelInfo positionStreamChannelInfo;
            positionStreamChannelInfo.m_semantic = RHI::ShaderSemantic(AZ::Name(PositionSemantic));
            positionStreamChannelInfo.m_componentCount = RHI::GetFormatComponentCount(PositionStreamFormat);

            RPI::ShaderInputContract shaderInputContract;
            shaderInputContract.m_streamChannels.emplace_back(positionStreamChannelInfo);

            // retrieve vertex/index buffers
            RHI::InputStreamLayout inputStreamLayout;
            RHI::StreamBufferIndices streamIndices;
            [[maybe_unused]] bool result = modelLod->GetStreamsForMesh(
                inputStreamLayout,
                streamIndices,
                nullptr,
                shaderInputContract,
                0);
            AZ_Assert(result, "Failed to retrieve DiffuseProbeGrid visualization mesh stream buffer views");

            auto streamIter = mesh.CreateStreamIterator(streamIndices);

            m_visualizationVB = streamIter[0];
            m_visualizationIB = mesh.GetIndexBufferView();

            // create the BLAS object
            RHI::RayTracingBlasDescriptor blasDescriptor;
            blasDescriptor.Build()
                ->Geometry()
                ->VertexFormat(PositionStreamFormat)
                ->VertexBuffer(m_visualizationVB)
                ->IndexBuffer(m_visualizationIB)
            ;

            m_visualizationBlas = aznew RHI::RayTracingBlas;
            auto deviceMask = RHI::RHISystemInterface::Get()->GetRayTracingSupport();
            if (deviceMask != RHI::MultiDevice::NoDevices)
            {
                m_visualizationBlas->CreateBuffers(deviceMask, &blasDescriptor, *m_visualizationBufferPools);
            }
        }

        void DiffuseProbeGridFeatureProcessor::HandleAssetNotification(Data::Asset<Data::AssetData> asset, DiffuseProbeGridTextureNotificationType notificationType)
        {
            for (NotifyTextureAssetVector::iterator itNotification = m_notifyTextureAssets.begin(); itNotification != m_notifyTextureAssets.end(); ++itNotification)
            {
                if (itNotification->m_assetId == asset.GetId())
                {
                    // store the texture asset
                    itNotification->m_asset = Data::static_pointer_cast<RPI::StreamingImageAsset>(asset);
                    itNotification->m_notificationType = notificationType;

                    // stop notifications on this asset
                    Data::AssetBus::MultiHandler::BusDisconnect(itNotification->m_assetId);

                    break;
                }
            }
        }

        void DiffuseProbeGridFeatureProcessor::OnAssetReady(Data::Asset<Data::AssetData> asset)
        {
            if (asset.GetId() == m_visualizationModelAsset.GetId())
            {
                OnVisualizationModelAssetReady(asset);
            }
            else
            {
                HandleAssetNotification(asset, DiffuseProbeGridTextureNotificationType::Ready);
            }
        }

        void DiffuseProbeGridFeatureProcessor::OnAssetError(Data::Asset<Data::AssetData> asset)
        {
            if (asset.GetId() == m_visualizationModelAsset.GetId())
            {
                AZ_Error("DiffuseProbeGridFeatureProcessor", false, "Failed to load probe visualization model asset [%s]", asset.GetHint().c_str());
            }
            else
            {
                AZ_Error("DiffuseProbeGridFeatureProcessor", false, "Failed to load cubemap [%s]", asset.GetHint().c_str());

                HandleAssetNotification(asset, DiffuseProbeGridTextureNotificationType::Error);
            }
        }
    } // namespace Render
} // namespace AZ
