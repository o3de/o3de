/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <DiffuseGlobalIllumination/DiffuseProbeGridFeatureProcessor.h>
#include <Atom/Feature/TransformService/TransformServiceFeatureProcessor.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/PipelineState.h>
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
                    ->Version(0);
            }
        }

        void DiffuseProbeGridFeatureProcessor::Activate()
        {
            RHI::RHISystemInterface* rhiSystem = RHI::RHISystemInterface::Get();

            m_diffuseProbeGrids.reserve(InitialProbeGridAllocationSize);
            m_realTimeDiffuseProbeGrids.reserve(InitialProbeGridAllocationSize);

            RHI::BufferPoolDescriptor desc;
            desc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
            desc.m_bindFlags = RHI::BufferBindFlags::InputAssembly;

            m_bufferPool = RHI::Factory::Get().CreateBufferPool();
            m_bufferPool->SetName(Name("DiffuseProbeGridBoxBufferPool"));
            [[maybe_unused]] RHI::ResultCode resultCode = m_bufferPool->Init(*rhiSystem->GetDevice(), desc);
            AZ_Error("DiffuseProbeGridFeatureProcessor", resultCode == RHI::ResultCode::Success, "Failed to initialize buffer pool");

            // create box mesh vertices and indices
            CreateBoxMesh();

            // image pool
            {
                RHI::ImagePoolDescriptor imagePoolDesc;
                imagePoolDesc.m_bindFlags = RHI::ImageBindFlags::ShaderReadWrite | RHI::ImageBindFlags::CopyRead;

                m_probeGridRenderData.m_imagePool = RHI::Factory::Get().CreateImagePool();
                [[maybe_unused]] RHI::ResultCode result = m_probeGridRenderData.m_imagePool->Init(*rhiSystem->GetDevice(), imagePoolDesc);
                AZ_Assert(result == RHI::ResultCode::Success, "Failed to initialize output image pool");
            }

            // create image view descriptors
            m_probeGridRenderData.m_probeRayTraceImageViewDescriptor = RHI::ImageViewDescriptor::Create(DiffuseProbeGridRenderData::RayTraceImageFormat, 0, 0);
            m_probeGridRenderData.m_probeIrradianceImageViewDescriptor = RHI::ImageViewDescriptor::Create(DiffuseProbeGridRenderData::IrradianceImageFormat, 0, 0);
            m_probeGridRenderData.m_probeDistanceImageViewDescriptor = RHI::ImageViewDescriptor::Create(DiffuseProbeGridRenderData::DistanceImageFormat, 0, 0);
            m_probeGridRenderData.m_probeRelocationImageViewDescriptor = RHI::ImageViewDescriptor::Create(DiffuseProbeGridRenderData::RelocationImageFormat, 0, 0);
            m_probeGridRenderData.m_probeClassificationImageViewDescriptor = RHI::ImageViewDescriptor::Create(DiffuseProbeGridRenderData::ClassificationImageFormat, 0, 0);

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

            EnableSceneNotification();
        }

        void DiffuseProbeGridFeatureProcessor::Deactivate()
        {
            AZ_Warning("DiffuseProbeGridFeatureProcessor", m_diffuseProbeGrids.size() == 0, 
                "Deactivating the DiffuseProbeGridFeatureProcessor, but there are still outstanding probe grids probes. Components\n"
                "using DiffuseProbeGridHandles should free them before the DiffuseProbeGridFeatureProcessor is deactivated.\n"
            );

            DisableSceneNotification();

            if (m_bufferPool)
            {
                m_bufferPool.reset();
            }
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
        }

        void DiffuseProbeGridFeatureProcessor::OnBeginPrepareRender()
        {
            for (auto& diffuseProbeGrid : m_realTimeDiffuseProbeGrids)
            {
                diffuseProbeGrid->ResetCullingVisibility();
            }
        }

        void DiffuseProbeGridFeatureProcessor::OnEndPrepareRender()
        {
            // re-build the list of visible real-time diffuse probe grids
            m_visibleRealTimeDiffuseProbeGrids.clear();
            for (auto& diffuseProbeGrid : m_realTimeDiffuseProbeGrids)
            {
                if (diffuseProbeGrid->GetIsVisible())
                {
                    m_visibleRealTimeDiffuseProbeGrids.push_back(diffuseProbeGrid);
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

        void DiffuseProbeGridFeatureProcessor::BakeTextures(
            const DiffuseProbeGridHandle& probeGrid,
            DiffuseProbeGridBakeTexturesCallback callback,
            const AZStd::string& irradianceTextureRelativePath,
            const AZStd::string& distanceTextureRelativePath,
            const AZStd::string& relocationTextureRelativePath,
            const AZStd::string& classificationTextureRelativePath)
        {
            AZ_Assert(probeGrid.get(), "BakeTextures called with an invalid handle");

            AddNotificationEntry(irradianceTextureRelativePath);
            AddNotificationEntry(distanceTextureRelativePath);
            AddNotificationEntry(relocationTextureRelativePath);
            AddNotificationEntry(classificationTextureRelativePath);

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
            const AZStd::string& relocationTextureRelativePath,
            const AZStd::string& classificationTextureRelativePath)
        {
            for (auto& diffuseProbeGrid : m_diffuseProbeGrids)
            {
                if ((diffuseProbeGrid->GetBakedIrradianceRelativePath() == irradianceTextureRelativePath) ||
                    (diffuseProbeGrid->GetBakedDistanceRelativePath() == distanceTextureRelativePath) ||
                    (diffuseProbeGrid->GetBakedRelocationRelativePath() == relocationTextureRelativePath) ||
                    (diffuseProbeGrid->GetBakedClassificationRelativePath() == classificationTextureRelativePath))
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

        void DiffuseProbeGridFeatureProcessor::SetBakedTextures(const DiffuseProbeGridHandle& probeGrid, const DiffuseProbeGridBakedTextures& bakedTextures)
        {
            AZ_Assert(probeGrid.get(), "SetBakedTextures called with an invalid handle");
            probeGrid->SetBakedTextures(bakedTextures);
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
            m_boxIndexBuffer = AZ::RHI::Factory::Get().CreateBuffer();
            request.m_buffer = m_boxIndexBuffer.get();
            request.m_descriptor = AZ::RHI::BufferDescriptor{ AZ::RHI::BufferBindFlags::InputAssembly, m_boxIndices.size() * sizeof(uint16_t) };
            request.m_initialData = m_boxIndices.data();
            AZ::RHI::ResultCode result = m_bufferPool->InitBuffer(request);
            AZ_Error("DiffuseProbeGridFeatureProcessor", result == RHI::ResultCode::Success, "Failed to initialize box index buffer - error [%d]", result);

            // create index buffer view
            AZ::RHI::IndexBufferView indexBufferView =
            {
                *m_boxIndexBuffer,
                0,
                sizeof(indices),
                AZ::RHI::IndexFormat::Uint16,
            };
            m_probeGridRenderData.m_boxIndexBufferView = indexBufferView;
            m_probeGridRenderData.m_boxIndexCount = numIndices;

            // create position buffer
            m_boxPositionBuffer = AZ::RHI::Factory::Get().CreateBuffer();
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
            m_probeGridRenderData.m_boxPositionBufferView = { { positionBufferView } };

            AZ::RHI::ValidateStreamBufferViews(m_boxStreamLayout, m_probeGridRenderData.m_boxPositionBufferView);
        }

        void DiffuseProbeGridFeatureProcessor::OnRenderPipelinePassesChanged([[maybe_unused]] RPI::RenderPipeline* renderPipeline)
        {
            UpdatePasses();
            m_needUpdatePipelineStates = true;
        }

        void DiffuseProbeGridFeatureProcessor::OnRenderPipelineAdded([[maybe_unused]] RPI::RenderPipelinePtr pipeline)
        {
            UpdatePasses();
            m_needUpdatePipelineStates = true;
        }

        void DiffuseProbeGridFeatureProcessor::OnRenderPipelineRemoved([[maybe_unused]] RPI::RenderPipeline* pipeline)
        {
            m_needUpdatePipelineStates = true;
        }

        void DiffuseProbeGridFeatureProcessor::UpdatePipelineStates()
        {
            if(m_probeGridRenderData.m_pipelineState)
            {
                m_probeGridRenderData.m_pipelineState->SetOutputFromScene(GetParentScene());
                m_probeGridRenderData.m_pipelineState->Finalize();
            }
        }

        void DiffuseProbeGridFeatureProcessor::UpdatePasses()
        {
            // disable the DiffuseProbeGridUpdatePass if the platform does not support raytracing
            RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();
            if (device->GetFeatures().m_rayTracing == false)
            {
                RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassName(AZ::Name("DiffuseProbeGridUpdatePass"), GetParentScene());
                RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [](RPI::Pass* pass) -> RPI::PassFilterExecutionFlow
                    {
                        pass->SetEnabled(false);
                         return RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                    });
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
            HandleAssetNotification(asset, DiffuseProbeGridTextureNotificationType::Ready);
        }

        void DiffuseProbeGridFeatureProcessor::OnAssetError(Data::Asset<Data::AssetData> asset)
        {
            AZ_Error("ReflectionProbeFeatureProcessor", false, "Failed to load cubemap [%s]", asset.GetHint().c_str());

            HandleAssetNotification(asset, DiffuseProbeGridTextureNotificationType::Error);
        }
    } // namespace Render
} // namespace AZ
