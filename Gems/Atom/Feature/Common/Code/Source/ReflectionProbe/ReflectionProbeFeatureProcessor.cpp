/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ReflectionProbe/ReflectionProbeFeatureProcessor.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/Feature/Mesh/MeshFeatureProcessor.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/PipelineState.h>
#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>
#include <Atom/Feature/RenderCommon.h>

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
                    ->Version(1);
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
                m_reflectionRenderData.m_stencilShader,
                m_reflectionRenderData.m_stencilSrgLayout,
                m_reflectionRenderData.m_stencilDrawListTag);

            LoadShader("shaders/reflections/reflectionprobeblendweight.azshader",
                m_reflectionRenderData.m_blendWeightPipelineState,
                m_reflectionRenderData.m_blendWeightShader,
                m_reflectionRenderData.m_blendWeightSrgLayout,
                m_reflectionRenderData.m_blendWeightDrawListTag);

            LoadShader("shaders/reflections/reflectionproberenderouter.azshader",
                m_reflectionRenderData.m_renderOuterPipelineState,
                m_reflectionRenderData.m_renderOuterShader,
                m_reflectionRenderData.m_renderOuterSrgLayout,
                m_reflectionRenderData.m_renderOuterDrawListTag);

            LoadShader("shaders/reflections/reflectionproberenderinner.azshader",
                m_reflectionRenderData.m_renderInnerPipelineState,
                m_reflectionRenderData.m_renderInnerShader,
                m_reflectionRenderData.m_renderInnerSrgLayout,
                m_reflectionRenderData.m_renderInnerDrawListTag);

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
            AZ_PROFILE_SCOPE(AzRender, "ReflectionProbeFeatureProcessor: Simulate");

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
                AZ_PROFILE_SCOPE(AzRender, "Sort reflection probes");

                // sort the probes by descending inner volume size, so the smallest volumes are rendered last
                auto sortFn = [](ReflectionProbePtr const& probe1, ReflectionProbePtr const& probe2) -> bool
                {
                    const Obb& obb1 = probe1->GetInnerObbWs();
                    const Obb& obb2 = probe2->GetInnerObbWs();
                    float size1 = obb1.GetHalfLengthX() * obb1.GetHalfLengthZ() * obb1.GetHalfLengthY();
                    float size2 = obb2.GetHalfLengthX() * obb2.GetHalfLengthZ() * obb2.GetHalfLengthY();

                    return (size1 > size2);
                };

                AZStd::sort(m_reflectionProbes.begin(), m_reflectionProbes.end(), sortFn);
                m_probeSortRequired = false;
                m_meshFeatureProcessorUpdateRequired = true;
            }

            // call Simulate on all reflection probes
            for (uint32_t probeIndex = 0; probeIndex < m_reflectionProbes.size(); ++probeIndex)
            {
                ReflectionProbePtr& reflectionProbe = m_reflectionProbes[probeIndex];
                AZ_Assert(reflectionProbe.use_count() > 1, "ReflectionProbe found with no corresponding owner, ensure that RemoveProbe() is called before releasing probe handles");

                reflectionProbe->Simulate(probeIndex);
            }
        }

        void ReflectionProbeFeatureProcessor::OnRenderEnd()
        {
            // call OnRenderEnd on all reflection probes
            for (auto& reflectionProbe : m_reflectionProbes)
            {
                AZ_Assert(reflectionProbe.use_count() > 1, "ReflectionProbe found with no corresponding owner, ensure that RemoveProbe() is called before releasing probe handles");

                reflectionProbe->OnRenderEnd();
            }

            // notify the MeshFeatureProcessor if there were changes to the ReflectionProbes
            // Note: This is done in OnRenderEnd to avoid a race between the two feature processors in Simulate, and any changes
            // will be applied on the next frame by the MeshFeatureProcessor.
            if (m_meshFeatureProcessorUpdateRequired)
            {
                MeshFeatureProcessor* meshFeatureProcessor = GetParentScene()->GetFeatureProcessor<MeshFeatureProcessor>();
                meshFeatureProcessor->UpdateMeshReflectionProbes();

                m_meshFeatureProcessorUpdateRequired = false;
            }
        }

        ReflectionProbeHandle ReflectionProbeFeatureProcessor::AddReflectionProbe(const AZ::Transform& transform, bool useParallaxCorrection)
        {
            ReflectionProbePtr reflectionProbe = AZStd::make_shared<ReflectionProbe>();
            reflectionProbe->Init(GetParentScene(), &m_reflectionRenderData);
            reflectionProbe->SetTransform(transform);
            reflectionProbe->SetUseParallaxCorrection(useParallaxCorrection);

            m_reflectionProbes.push_back(reflectionProbe);
            m_reflectionProbeMap[reflectionProbe->GetUuid()] = reflectionProbe;

            m_probeSortRequired = true;

            return reflectionProbe->GetUuid();
        }

        void ReflectionProbeFeatureProcessor::RemoveReflectionProbe(ReflectionProbeHandle& handle)
        {
            if (!ValidateHandle(handle))
            {
                return;
            }

            ReflectionProbePtr reflectionProbe = m_reflectionProbeMap[handle];
            auto itEntry = AZStd::find_if(m_reflectionProbes.begin(), m_reflectionProbes.end(), [&](ReflectionProbePtr const& entry)
            {
                return (entry == reflectionProbe);
            });

            AZ_Assert(itEntry != m_reflectionProbes.end(), "RemoveProbe called with a probe that is not in the probe list");

            m_reflectionProbes.erase(itEntry);
            m_reflectionProbeMap.erase(handle);

            m_meshFeatureProcessorUpdateRequired = true;
        }

        void ReflectionProbeFeatureProcessor::SetOuterExtents(const ReflectionProbeHandle& handle, const Vector3& outerExtents)
        {
            if (!ValidateHandle(handle))
            {
                return;
            }

            m_reflectionProbeMap[handle]->SetOuterExtents(outerExtents);
            m_probeSortRequired = true;
        }

        AZ::Vector3 ReflectionProbeFeatureProcessor::GetOuterExtents(const ReflectionProbeHandle& handle) const
        {
            if (!ValidateHandle(handle))
            {
                return AZ::Vector3::CreateZero();
            }

            ReflectionProbeMap::const_iterator it = m_reflectionProbeMap.find(handle);
            return it->second->GetOuterExtents();
        }

        void ReflectionProbeFeatureProcessor::SetInnerExtents(const ReflectionProbeHandle& handle, const Vector3& innerExtents)
        {
            if (!ValidateHandle(handle))
            {
                return;
            }

            m_reflectionProbeMap[handle]->SetInnerExtents(innerExtents);
            m_probeSortRequired = true;
        }

        AZ::Vector3 ReflectionProbeFeatureProcessor::GetInnerExtents(const ReflectionProbeHandle& handle) const
        {
            if (!ValidateHandle(handle))
            {
                return AZ::Vector3::CreateZero();
            }

            ReflectionProbeMap::const_iterator it = m_reflectionProbeMap.find(handle);
            return it->second->GetInnerExtents();
        }

        AZ::Obb ReflectionProbeFeatureProcessor::GetOuterObbWs(const ReflectionProbeHandle& handle) const
        {
            if (!ValidateHandle(handle))
            {
                return AZ::Obb();
            }

            ReflectionProbeMap::const_iterator it = m_reflectionProbeMap.find(handle);
            return it->second->GetOuterObbWs();
        }

        AZ::Obb ReflectionProbeFeatureProcessor::GetInnerObbWs(const ReflectionProbeHandle& handle) const
        {
            if (!ValidateHandle(handle))
            {
                return AZ::Obb();
            }

            ReflectionProbeMap::const_iterator it = m_reflectionProbeMap.find(handle);
            return it->second->GetInnerObbWs();
        }

        void ReflectionProbeFeatureProcessor::SetTransform(const ReflectionProbeHandle& handle, const AZ::Transform& transform)
        {
            if (!ValidateHandle(handle))
            {
                return;
            }

            m_reflectionProbeMap[handle]->SetTransform(transform);
            m_probeSortRequired = true;
        }

        AZ::Transform ReflectionProbeFeatureProcessor::GetTransform(const ReflectionProbeHandle& handle) const
        {
            if (!ValidateHandle(handle))
            {
                return AZ::Transform::CreateIdentity();
            }

            ReflectionProbeMap::const_iterator it = m_reflectionProbeMap.find(handle);
            return it->second->GetTransform();
        }

        void ReflectionProbeFeatureProcessor::SetCubeMap(const ReflectionProbeHandle& handle, Data::Instance<RPI::Image>& cubeMapImage, const AZStd::string& relativePath)
        {
            if (!ValidateHandle(handle))
            {
                return;
            }

            m_reflectionProbeMap[handle]->SetCubeMapImage(cubeMapImage, relativePath);

            m_meshFeatureProcessorUpdateRequired = true;
        }

        Data::Instance<RPI::Image> ReflectionProbeFeatureProcessor::GetCubeMap(const ReflectionProbeHandle& handle) const
        {
            if (!ValidateHandle(handle))
            {
                return nullptr;
            }

            ReflectionProbeMap::const_iterator it = m_reflectionProbeMap.find(handle);
            return it->second->GetCubeMapImage();
        }

        void ReflectionProbeFeatureProcessor::SetRenderExposure(const ReflectionProbeHandle& handle, float renderExposure)
        {
            if (!ValidateHandle(handle))
            {
                return;
            }

            m_reflectionProbeMap[handle]->SetRenderExposure(renderExposure);
        }

        float ReflectionProbeFeatureProcessor::GetRenderExposure(const ReflectionProbeHandle& handle) const
        {
            if (!ValidateHandle(handle))
            {
                return 0.0f;
            }

            ReflectionProbeMap::const_iterator it = m_reflectionProbeMap.find(handle);
            return it->second->GetRenderExposure();
        }

        void ReflectionProbeFeatureProcessor::SetBakeExposure(const ReflectionProbeHandle& handle, float bakeExposure)
        {
            if (!ValidateHandle(handle))
            {
                return;
            }

            m_reflectionProbeMap[handle]->SetBakeExposure(bakeExposure);
        }

        float ReflectionProbeFeatureProcessor::GetBakeExposure(const ReflectionProbeHandle& handle) const
        {
            if (!ValidateHandle(handle))
            {
                return 0.0f;
            }

            ReflectionProbeMap::const_iterator it = m_reflectionProbeMap.find(handle);
            return it->second->GetBakeExposure();
        }

        bool ReflectionProbeFeatureProcessor::GetUseParallaxCorrection(const ReflectionProbeHandle& handle) const
        {
            if (!ValidateHandle(handle))
            {
                return false;
            }

            ReflectionProbeMap::const_iterator it = m_reflectionProbeMap.find(handle);
            return it->second->GetUseParallaxCorrection();
        }

        void ReflectionProbeFeatureProcessor::Bake(const ReflectionProbeHandle& handle, RenderCubeMapCallback callback, const AZStd::string& relativePath)
        {
            if (!ValidateHandle(handle))
            {
                return;
            }

            m_reflectionProbeMap[handle]->Bake(callback);

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

        void ReflectionProbeFeatureProcessor::ShowVisualization(const ReflectionProbeHandle& handle, bool showVisualization)
        {
            if (!ValidateHandle(handle))
            {
                return;
            }

            m_reflectionProbeMap[handle]->ShowVisualization(showVisualization);
        }

        void ReflectionProbeFeatureProcessor::FindReflectionProbes(const AZ::Vector3& position, ReflectionProbeHandleVector& reflectionProbeHandles)
        {
            FindReflectionProbesInternal(Aabb::CreateCenterRadius(position, 0.5f), reflectionProbeHandles, [&](const ReflectionProbe* reflectionProbe) -> bool
            {
                return reflectionProbe->GetOuterObbWs().Contains(position);
            });
        }

        void ReflectionProbeFeatureProcessor::FindReflectionProbes(const AZ::Aabb& aabb, ReflectionProbeHandleVector& reflectionProbeHandles)
        {
            FindReflectionProbesInternal(aabb, reflectionProbeHandles, [&](const ReflectionProbe* reflectionProbe) -> bool
            {
                // [GFX TODO] Implement Obb-Aabb intersection test in ShapeIntersectionTests (AzCore)
                AZ::Aabb outerAabb = AZ::Aabb::CreateFromObb(reflectionProbe->GetOuterObbWs());
                return outerAabb.Overlaps(aabb);
            });
        }

        void ReflectionProbeFeatureProcessor::FindReflectionProbesInternal(const AZ::Aabb& aabb, ReflectionProbeHandleVector& reflectionProbeHandles, AZStd::function<bool(const ReflectionProbe*)> filter)
        {
            reflectionProbeHandles.clear();

            AZStd::vector<AzFramework::VisibilityEntry*> visibilityEntries;
            GetParentScene()->GetCullingScene()->GetVisibilityScene()->Enumerate(
                aabb,
                [&](const AzFramework::IVisibilityScene::NodeData& nodeData)
                {
                    for (AzFramework::VisibilityEntry* entry : nodeData.m_entries)
                    {
                        if (entry->m_typeFlags & AzFramework::VisibilityEntry::TypeFlags::TYPE_RPI_Cullable)
                        {
                            if (RPI::Cullable* cullable = static_cast<RPI::Cullable*>(entry->m_userData);
                                cullable && cullable->m_cullData.m_componentType == Culling::ComponentType::ReflectionProbe)
                            {
                                AZ::Uuid uuid = cullable->m_cullData.m_componentUuid;
                                ReflectionProbePtr reflectionProbe = m_reflectionProbeMap[cullable->m_cullData.m_componentUuid];
                                AZ_Assert(reflectionProbe, "Unable to find reflection probe by Uuid");

                                if (reflectionProbe
                                    && reflectionProbe->GetCubeMapImage()
                                    && reflectionProbe->GetCubeMapImage()->IsInitialized())
                                {
                                    if (!filter || filter(reflectionProbe.get()))
                                    {
                                        reflectionProbeHandles.push_back(uuid);
                                    }
                                }
                            }
                        }
                    }
                }
            );

            // sort the probes by descending inner volume size
            auto sortFn = [&](const ReflectionProbeHandle& handle1, const ReflectionProbeHandle& handle2) -> bool
            {
                const Obb& obb1 = m_reflectionProbeMap[handle1]->GetInnerObbWs();
                const Obb& obb2 = m_reflectionProbeMap[handle2]->GetInnerObbWs();
                float size1 = obb1.GetHalfLengthX() * obb1.GetHalfLengthZ() * obb1.GetHalfLengthY();
                float size2 = obb2.GetHalfLengthX() * obb2.GetHalfLengthZ() * obb2.GetHalfLengthY();

                return (size1 > size2);
            };

            AZStd::sort(reflectionProbeHandles.begin(), reflectionProbeHandles.end(), sortFn);
        }

        bool ReflectionProbeFeatureProcessor::ValidateHandle(const ReflectionProbeHandle& handle) const
        {
            if (handle.IsNull() || m_reflectionProbeMap.find(handle) == m_reflectionProbeMap.end())
            {
                AZ_Assert(false, "Invalid ReflectionProbeHandle passed to the ReflectionProbeFeatureProcessor");
                return false;
            }

            return true;
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
            [[maybe_unused]] AZ::RHI::ResultCode result = m_bufferPool->InitBuffer(request);
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
            Data::Instance<RPI::Shader>& shader,
            RHI::Ptr<RHI::ShaderResourceGroupLayout>& srgLayout,
            RHI::DrawListTag& drawListTag)
        {
            // load shader
            shader = RPI::LoadCriticalShader(filePath);

            if (shader == nullptr)
            {
                AZ_Error("ReflectionProbeFeatureProcessor", false, "Failed to find asset for shader [%s]", filePath);
                return;
            }

            // store drawlist tag
            drawListTag = shader->GetDrawListTag();

            // create pipeline state
            pipelineState = aznew RPI::PipelineStateForDraw;
            pipelineState->Init(shader); // uses default shader variant
            pipelineState->SetInputStreamLayout(m_boxStreamLayout);
            pipelineState->SetOutputFromScene(GetParentScene());
            pipelineState->Finalize();

            // load object shader resource group
            srgLayout = shader->FindShaderResourceGroupLayout(RPI::SrgBindingSlot::Object);
            AZ_Error("ReflectionProbeFeatureProcessor", srgLayout != nullptr, "Failed to find ObjectSrg layout from shader [%s]", filePath);
        }

        void ReflectionProbeFeatureProcessor::OnRenderPipelineChanged(RPI::RenderPipeline* renderPipeline,
            RPI::SceneNotification::RenderPipelineChangeType changeType)
        {
            if (changeType == RPI::SceneNotification::RenderPipelineChangeType::PassChanged)
            {
                for (auto& reflectionProbe : m_reflectionProbes)
                {
                    reflectionProbe->OnRenderPipelinePassesChanged(renderPipeline);
                }
            }
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
