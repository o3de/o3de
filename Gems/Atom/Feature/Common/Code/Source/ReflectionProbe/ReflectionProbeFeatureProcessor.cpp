/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/Feature/ReflectionProbe/ReflectionProbeFeatureProcessor.h>
#include <Atom/Feature/Mesh/MeshFeatureProcessor.h>
#include <Atom/RHI/CpuProfiler.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/PipelineState.h>
#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>
#include <AzCore/Debug/EventTrace.h>

namespace AZ
{
    namespace Render
    {
        void ReflectionProbeFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<ReflectionProbeFeatureProcessor, FeatureProcessor>()
                    ->Version(0);
            }
        }

        void ReflectionProbeFeatureProcessor::Activate()
        {
            RHI::RHISystemInterface* rhiSystem = RHI::RHISystemInterface::Get();

            m_reflectionProbes.reserve(InitialProbeAllocationSize);

            RHI::BufferPoolDescriptor desc;
            desc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
            desc.m_bindFlags = RHI::BufferBindFlags::InputAssembly;

            m_bufferPool = RHI::Factory::Get().CreateBufferPool();
            m_bufferPool->SetName(Name("ReflectionProbeBoxBufferPool"));
            [[maybe_unused]] RHI::ResultCode resultCode = m_bufferPool->Init(*rhiSystem->GetDevice(), desc);
            AZ_Error("ReflectionProbeFeatureProcessor", resultCode == RHI::ResultCode::Success, "Failed to initialize buffer pool");

            // create box mesh vertices and indices
            CreateBoxMesh();

            // load shaders for stencil, blend weight, and render passes
            LoadShader("shaders/reflections/reflectionprobestencil.azshader",
                m_reflectionRenderData.m_stencilPipelineState,
                m_reflectionRenderData.m_stencilSrgAsset,
                m_reflectionRenderData.m_stencilDrawListTag);

            LoadShader("shaders/reflections/reflectionprobeblendweight.azshader",
                m_reflectionRenderData.m_blendWeightPipelineState,
                m_reflectionRenderData.m_blendWeightSrgAsset,
                m_reflectionRenderData.m_blendWeightDrawListTag);

            LoadShader("shaders/reflections/reflectionproberenderouter.azshader",
                m_reflectionRenderData.m_renderOuterPipelineState,
                m_reflectionRenderData.m_renderOuterSrgAsset,
                m_reflectionRenderData.m_renderOuterDrawListTag);

            LoadShader("shaders/reflections/reflectionproberenderinner.azshader",
                m_reflectionRenderData.m_renderInnerPipelineState,
                m_reflectionRenderData.m_renderInnerSrgAsset,
                m_reflectionRenderData.m_renderInnerDrawListTag);

            // create ShaderResourceGroups here so we can get the layout and cache the indices
            // Note: the SRGs are not needed beyond this method since each probe creates its own SRGs, we are just interested in the indices

            // cache probe stencil shader indices 
            Data::Instance<RPI::ShaderResourceGroup> stencilSrg = RPI::ShaderResourceGroup::Create(m_reflectionRenderData.m_stencilSrgAsset);
            AZ_Error("ReflectionProbeFeatureProcessor", stencilSrg.get(), "Failed to create stencil back face shader resource group");

            const RHI::ShaderResourceGroupLayout* stencilSrgLayout = stencilSrg->GetLayout();
            Name modelToWorldConstantName = Name("m_modelToWorld");
            m_reflectionRenderData.m_modelToWorldStencilConstantIndex = stencilSrgLayout->FindShaderInputConstantIndex(modelToWorldConstantName);
            AZ_Error("ReflectionProbeFeatureProcessor", m_reflectionRenderData.m_modelToWorldStencilConstantIndex.IsValid(), "Failed to find stencil shader input constant [%s]", modelToWorldConstantName.GetCStr());

            // cache probe render shader indices
            // Note: the outer and inner render shaders use the same Srg
            Data::Instance<RPI::ShaderResourceGroup> renderReflectionSrg = RPI::ShaderResourceGroup::Create(m_reflectionRenderData.m_renderOuterSrgAsset);
            AZ_Error("ReflectionProbeFeatureProcessor", renderReflectionSrg.get(), "Failed to create render reflection shader resource group");

            const RHI::ShaderResourceGroupLayout* renderReflectionSrgLayout = renderReflectionSrg->GetLayout();
            m_reflectionRenderData.m_modelToWorldRenderConstantIndex = renderReflectionSrgLayout->FindShaderInputConstantIndex(modelToWorldConstantName);
            AZ_Error("ReflectionProbeFeatureProcessor", m_reflectionRenderData.m_modelToWorldRenderConstantIndex.IsValid(), "Failed to find render shader input constant [%s]", modelToWorldConstantName.GetCStr());

            Name aabbPosConstantName = Name("m_aabbPos");
            m_reflectionRenderData.m_aabbPosRenderConstantIndex = renderReflectionSrgLayout->FindShaderInputConstantIndex(aabbPosConstantName);
            AZ_Error("ReflectionProbeFeatureProcessor", m_reflectionRenderData.m_aabbPosRenderConstantIndex.IsValid(), "Failed to find render shader input constant [%s]", aabbPosConstantName.GetCStr());

            Name outerAabbMinConstantName = Name("m_outerAabbMin");
            m_reflectionRenderData.m_outerAabbMinRenderConstantIndex = renderReflectionSrgLayout->FindShaderInputConstantIndex(outerAabbMinConstantName);
            AZ_Error("ReflectionProbeFeatureProcessor", m_reflectionRenderData.m_outerAabbMinRenderConstantIndex.IsValid(), "Failed to find render shader input constant [%s]", outerAabbMinConstantName.GetCStr());

            Name outerAabbMaxConstantName = Name("m_outerAabbMax");
            m_reflectionRenderData.m_outerAabbMaxRenderConstantIndex = renderReflectionSrgLayout->FindShaderInputConstantIndex(outerAabbMaxConstantName);
            AZ_Error("ReflectionProbeFeatureProcessor", m_reflectionRenderData.m_outerAabbMaxRenderConstantIndex.IsValid(), "Failed to find render shader input constant [%s]", outerAabbMaxConstantName.GetCStr());

            Name innerAabbMinConstantName = Name("m_innerAabbMin");
            m_reflectionRenderData.m_innerAabbMinRenderConstantIndex = renderReflectionSrgLayout->FindShaderInputConstantIndex(innerAabbMinConstantName);
            AZ_Error("ReflectionProbeFeatureProcessor", m_reflectionRenderData.m_innerAabbMinRenderConstantIndex.IsValid(), "Failed to find render shader input constant [%s]", innerAabbMinConstantName.GetCStr());

            Name innerAabbMaxConstantName = Name("m_innerAabbMax");
            m_reflectionRenderData.m_innerAabbMaxRenderConstantIndex = renderReflectionSrgLayout->FindShaderInputConstantIndex(innerAabbMaxConstantName);
            AZ_Error("ReflectionProbeFeatureProcessor", m_reflectionRenderData.m_innerAabbMaxRenderConstantIndex.IsValid(), "Failed to find render shader input constant [%s]", innerAabbMaxConstantName.GetCStr());

            Name useParallaxCorrectionConstantName = Name("m_useParallaxCorrection");
            m_reflectionRenderData.m_useParallaxCorrectionRenderConstantIndex = renderReflectionSrgLayout->FindShaderInputConstantIndex(useParallaxCorrectionConstantName);
            AZ_Error("ReflectionProbeFeatureProcessor", m_reflectionRenderData.m_useParallaxCorrectionRenderConstantIndex.IsValid(), "Failed to find render shader input constant [%s]", useParallaxCorrectionConstantName.GetCStr());

            Name reflectionCubeMapImageName = Name("m_reflectionCubeMap");
            m_reflectionRenderData.m_reflectionCubeMapRenderImageIndex = renderReflectionSrgLayout->FindShaderInputImageIndex(reflectionCubeMapImageName);
            AZ_Error("ReflectionProbeFeatureProcessor", m_reflectionRenderData.m_reflectionCubeMapRenderImageIndex.IsValid(), "Failed to find render shader input image [%s]", reflectionCubeMapImageName.GetCStr());

            EnableSceneNotification();
        }

        void ReflectionProbeFeatureProcessor::Deactivate()
        {
            AZ_Warning("ReflectionProbeFeatureProcessor", m_reflectionProbes.size() == 0, 
                "Deactivating the ReflectionProbeFeatureProcessor, but there are still outstanding reflection probes. Components\n"
                "using ReflectionProbeHandles should free them before the ReflectionProbeFeatureProcessor is deactivated.\n"
            );

            DisableSceneNotification();

            if (m_bufferPool)
            {
                m_bufferPool.reset();
            }

            Data::AssetBus::MultiHandler::BusDisconnect();
        }

        void ReflectionProbeFeatureProcessor::Simulate([[maybe_unused]] const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);
            AZ_ATOM_PROFILE_FUNCTION("ReflectionProbe", "ReflectionProbeFeatureProcessor: Simulate");

            // update pipeline states
            if (m_needUpdatePipelineStates)
            {
                UpdatePipelineStates();
                m_needUpdatePipelineStates = false;
            }

            // check pending cubemaps and notify that the asset is ready
            for (auto& notificationEntry : m_notifyCubeMapAssets)
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
            if (m_probeSortRequired)
            {
                AZ_PROFILE_SCOPE(Debug::ProfileCategory::AzRender, "Sort reflection probes");
                AZ_ATOM_PROFILE_FUNCTION("ReflectionProbe", "ReflectionProbeFeatureProcessor: Sort reflection probes");

                // sort the probes by descending inner volume size, so the smallest volumes are rendered last
                auto sortFn = [](AZStd::shared_ptr<ReflectionProbe> const& probe1, AZStd::shared_ptr<ReflectionProbe> const& probe2) -> bool
                {
                    const Aabb& aabb1 = probe1->GetInnerAabbWs();
                    const Aabb& aabb2 = probe2->GetInnerAabbWs();
                    float size1 = aabb1.GetXExtent() * aabb1.GetZExtent() * aabb1.GetYExtent();
                    float size2 = aabb2.GetXExtent() * aabb2.GetZExtent() * aabb2.GetYExtent();
                    return (size1 > size2);
                };

                AZStd::sort(m_reflectionProbes.begin(), m_reflectionProbes.end(), sortFn);
                m_probeSortRequired = false;

                // notify the MeshFeatureProcessor that the reflection probes changed
                MeshFeatureProcessor* meshFeatureProcessor = GetParentScene()->GetFeatureProcessor<MeshFeatureProcessor>();
                meshFeatureProcessor->UpdateMeshReflectionProbes();
            }

            // call Simulate on all reflection probes
            for (uint32_t probeIndex = 0; probeIndex < m_reflectionProbes.size(); ++probeIndex)
            {
                AZStd::shared_ptr<ReflectionProbe>& reflectionProbe = m_reflectionProbes[probeIndex];
                AZ_Assert(reflectionProbe.use_count() > 1, "ReflectionProbe found with no corresponding owner, ensure that RemoveProbe() is called before releasing probe handles");

                reflectionProbe->Simulate(probeIndex);
            }
        }

        ReflectionProbeHandle ReflectionProbeFeatureProcessor::AddProbe(const AZ::Transform& transform, bool useParallaxCorrection)
        {
            AZStd::shared_ptr<ReflectionProbe> reflectionProbe = AZStd::make_shared<ReflectionProbe>();
            reflectionProbe->Init(GetParentScene(), &m_reflectionRenderData);
            reflectionProbe->SetTransform(transform);
            reflectionProbe->SetUseParallaxCorrection(useParallaxCorrection);
            m_reflectionProbes.push_back(reflectionProbe);
            m_probeSortRequired = true;

            return reflectionProbe;
        }

        void ReflectionProbeFeatureProcessor::RemoveProbe(ReflectionProbeHandle& probe)
        {
            AZ_Assert(probe.get(), "RemoveProbe called with an invalid handle");

            auto itEntry = AZStd::find_if(m_reflectionProbes.begin(), m_reflectionProbes.end(), [&](AZStd::shared_ptr<ReflectionProbe> const& entry)
            {
                return (entry == probe);
            });

            AZ_Assert(itEntry != m_reflectionProbes.end(), "RemoveProbe called with a probe that is not in the probe list");
            m_reflectionProbes.erase(itEntry);
        }

        void ReflectionProbeFeatureProcessor::SetProbeOuterExtents(const ReflectionProbeHandle& probe, const Vector3& outerExtents)
        {
            AZ_Assert(probe.get(), "SetProbeOuterExtents called with an invalid handle");
            probe->SetOuterExtents(outerExtents);
            m_probeSortRequired = true;
        }

        void ReflectionProbeFeatureProcessor::SetProbeInnerExtents(const ReflectionProbeHandle& probe, const Vector3& innerExtents)
        {
            AZ_Assert(probe.get(), "SetProbeInnerExtents called with an invalid handle");
            probe->SetInnerExtents(innerExtents);
            m_probeSortRequired = true;
        }

        void ReflectionProbeFeatureProcessor::SetProbeCubeMap(const ReflectionProbeHandle& probe, Data::Instance<RPI::Image>& cubeMapImage, const AZStd::string& relativePath)
        {
            AZ_Assert(probe.get(), "SetProbeCubeMap called with an invalid handle");
            probe->SetCubeMapImage(cubeMapImage, relativePath);
        }

        void ReflectionProbeFeatureProcessor::SetProbeTransform(const ReflectionProbeHandle& probe, const AZ::Transform& transform)
        {
            AZ_Assert(probe.get(), "SetProbeTransform called with an invalid handle");
            probe->SetTransform(transform);
            m_probeSortRequired = true;
        }

        void ReflectionProbeFeatureProcessor::BakeProbe(const ReflectionProbeHandle& probe, BuildCubeMapCallback callback, const AZStd::string& relativePath)
        {
            AZ_Assert(probe.get(), "BakeProbe called with an invalid handle");
            probe->BuildCubeMap(callback);

            // check to see if this is an existing asset
            AZ::Data::AssetId assetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                assetId,
                &AZ::Data::AssetCatalogRequests::GetAssetIdByPath,
                relativePath.c_str(),
                azrtti_typeid<AZ::RPI::StreamingImageAsset>(),
                false);

            // we only track notifications for new cubemap assets, existing assets are automatically reloaded by the RPI
            if (!assetId.IsValid())
            {
                m_notifyCubeMapAssets.push_back({ relativePath, assetId });
            }
        }

        bool ReflectionProbeFeatureProcessor::CheckCubeMapAssetNotification(const AZStd::string& relativePath, Data::Asset<RPI::StreamingImageAsset>& outCubeMapAsset, CubeMapAssetNotificationType& outNotificationType)
        {
            for (NotifyCubeMapAssetVector::iterator itNotification = m_notifyCubeMapAssets.begin(); itNotification != m_notifyCubeMapAssets.end(); ++itNotification)
            {
                if (itNotification->m_relativePath == relativePath)
                {
                    outNotificationType = itNotification->m_notificationType;
                    if (outNotificationType != CubeMapAssetNotificationType::None)
                    {
                        outCubeMapAsset = itNotification->m_asset;
                        m_notifyCubeMapAssets.erase(itNotification);
                    }

                    return true;
                }
            }

            return false;
        }

        bool ReflectionProbeFeatureProcessor::IsCubeMapReferenced(const AZStd::string& relativePath)
        {
            for (auto& reflectionProbe : m_reflectionProbes)
            {
                if (reflectionProbe->GetCubeMapRelativePath() == relativePath)
                {
                    return true;
                }
            }

            return false;
        }

        void ReflectionProbeFeatureProcessor::ShowProbeVisualization(const ReflectionProbeHandle& probe, bool showVisualization)
        {
            AZ_Assert(probe.get(), "ShowProbeVisualization called with an invalid handle");
            probe->ShowVisualization(showVisualization);
        }

        void ReflectionProbeFeatureProcessor::FindReflectionProbes(const Vector3& position, ReflectionProbeVector& reflectionProbes)
        {
            reflectionProbes.clear();

            // simple AABB check to find the reflection probes that contain the position
            for (auto& reflectionProbe : m_reflectionProbes)
            {
                if (reflectionProbe->GetOuterAabbWs().Contains(position)
                    && reflectionProbe->GetCubeMapImage()
                    && reflectionProbe->GetCubeMapImage()->IsInitialized())
                {
                    reflectionProbes.push_back(reflectionProbe);
                }
            }
        }

        void ReflectionProbeFeatureProcessor::CreateBoxMesh()
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
            AZ_Error("ReflectionProbeFeatureProcessor", result == RHI::ResultCode::Success, "Failed to initialize box index buffer - error [%d]", result);

            // create index buffer view
            AZ::RHI::IndexBufferView indexBufferView =
            {
                *m_boxIndexBuffer,
                0,
                sizeof(indices),
                AZ::RHI::IndexFormat::Uint16,
            };
            m_reflectionRenderData.m_boxIndexBufferView = indexBufferView;
            m_reflectionRenderData.m_boxIndexCount = numIndices;

            // create position buffer
            m_boxPositionBuffer = AZ::RHI::Factory::Get().CreateBuffer();
            request.m_buffer = m_boxPositionBuffer.get();
            request.m_descriptor = AZ::RHI::BufferDescriptor{ AZ::RHI::BufferBindFlags::InputAssembly, m_boxPositions.size() * sizeof(Position) };
            request.m_initialData = m_boxPositions.data();
            result = m_bufferPool->InitBuffer(request);
            AZ_Error("ReflectionProbeFeatureProcessor", result == RHI::ResultCode::Success, "Failed to initialize box index buffer - error [%d]", result);

            // create position buffer view
            RHI::StreamBufferView positionBufferView =
            {
                *m_boxPositionBuffer,
                0,
                (uint32_t)(m_boxPositions.size() * sizeof(Position)),
                sizeof(Position),
            };
            m_reflectionRenderData.m_boxPositionBufferView = { { positionBufferView } };

            AZ::RHI::ValidateStreamBufferViews(m_boxStreamLayout, m_reflectionRenderData.m_boxPositionBufferView);
        }

        void ReflectionProbeFeatureProcessor::LoadShader(
            const char* filePath,
            RPI::Ptr<RPI::PipelineStateForDraw>& pipelineState,
            Data::Asset<RPI::ShaderResourceGroupAsset>& srgAsset,
            RHI::DrawListTag& drawListTag)
        {
            // load shader
            Data::Instance<RPI::Shader> shader = RPI::LoadShader(filePath);
            AZ_Error("ReflectionProbeFeatureProcessor", shader, "Failed to find asset for shader [%s]", filePath);

            // store drawlist tag
            drawListTag = shader->GetDrawListTag();

            // create pipeline state
            pipelineState = aznew RPI::PipelineStateForDraw;
            pipelineState->Init(shader); // uses default shader variant
            pipelineState->SetInputStreamLayout(m_boxStreamLayout);
            pipelineState->SetOutputFromScene(GetParentScene());
            pipelineState->Finalize();

            // load object shader resource group
            srgAsset = shader->FindShaderResourceGroupAsset(Name{ "ObjectSrg" });
            AZ_Error("ReflectionProbeFeatureProcessor", srgAsset.GetId().IsValid(), "Failed to find ObjectSrg asset for shader [%s]", filePath);
            AZ_Error("ReflectionProbeFeatureProcessor", srgAsset.IsReady(), "ObjectSrg asset is not loaded for shader [%s]", filePath);
        }

        void ReflectionProbeFeatureProcessor::OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline)
        {
            for (auto& reflectionProbe : m_reflectionProbes)
            {
                reflectionProbe->OnRenderPipelinePassesChanged(renderPipeline);
            }
            m_needUpdatePipelineStates = true;
        }

        void ReflectionProbeFeatureProcessor::OnRenderPipelineAdded(RPI::RenderPipelinePtr pipeline)
        {
            m_needUpdatePipelineStates = true;
        }

        void ReflectionProbeFeatureProcessor::OnRenderPipelineRemoved([[maybe_unused]] RPI::RenderPipeline* pipeline)
        {
            m_needUpdatePipelineStates = true;
        }

        void ReflectionProbeFeatureProcessor::UpdatePipelineStates()
        {
            auto scene = GetParentScene();

            m_reflectionRenderData.m_stencilPipelineState->SetOutputFromScene(scene);
            m_reflectionRenderData.m_stencilPipelineState->Finalize();

            m_reflectionRenderData.m_blendWeightPipelineState->SetOutputFromScene(scene);
            m_reflectionRenderData.m_blendWeightPipelineState->Finalize();

            m_reflectionRenderData.m_renderOuterPipelineState->SetOutputFromScene(scene);
            m_reflectionRenderData.m_renderOuterPipelineState->Finalize();

            m_reflectionRenderData.m_renderInnerPipelineState->SetOutputFromScene(scene);
            m_reflectionRenderData.m_renderInnerPipelineState->Finalize();
        }

        void ReflectionProbeFeatureProcessor::HandleAssetNotification(Data::Asset<Data::AssetData> asset, CubeMapAssetNotificationType notificationType)
        {
            for (NotifyCubeMapAssetVector::iterator itNotification = m_notifyCubeMapAssets.begin(); itNotification != m_notifyCubeMapAssets.end(); ++itNotification)
            {
                if (itNotification->m_assetId == asset.GetId())
                {
                    // store the cubemap asset
                    itNotification->m_asset = Data::static_pointer_cast<RPI::StreamingImageAsset>(asset);
                    itNotification->m_notificationType = notificationType;

                    // stop notifications on this asset
                    Data::AssetBus::MultiHandler::BusDisconnect(itNotification->m_assetId);

                    break;
                }
            }
        }

        void ReflectionProbeFeatureProcessor::OnAssetReady(Data::Asset<Data::AssetData> asset)
        {
            HandleAssetNotification(asset, CubeMapAssetNotificationType::Ready);
        }

        void ReflectionProbeFeatureProcessor::OnAssetError(Data::Asset<Data::AssetData> asset)
        {
            AZ_Error("ReflectionProbeFeatureProcessor", false, "Failed to load cubemap [%s]", asset.GetHint().c_str());

            HandleAssetNotification(asset, CubeMapAssetNotificationType::Error);
        }
    } // namespace Render
} // namespace AZ
