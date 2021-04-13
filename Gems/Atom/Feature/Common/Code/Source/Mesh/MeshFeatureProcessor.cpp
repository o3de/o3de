/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <RenderCommon.h>

#include <Atom/RHI/CpuProfiler.h>
#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>
#include <Atom/Feature/Mesh/MeshFeatureProcessor.h>
#include <Atom/Feature/ReflectionProbe/ReflectionProbeFeatureProcessor.h>
#include <Atom/RPI.Public/Model/ModelLodUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Culling.h>
#include <Atom/Utils/StableDynamicArray.h>

#include <AtomCore/Instance/InstanceDatabase.h>

#include <AzCore/Console/IConsole.h>
#include <AzCore/Debug/EventTrace.h>
#include <AzCore/Jobs/Algorithms.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Math/ShapeIntersection.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Asset/AssetCommon.h>

namespace AZ
{
    namespace Render
    {
        void MeshFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<MeshFeatureProcessor, FeatureProcessor>()
                    ->Version(0);
            }
        }

        void MeshFeatureProcessor::Activate()
        {
            m_transformService = GetParentScene()->GetFeatureProcessor<TransformServiceFeatureProcessor>();
            AZ_Assert(m_transformService, "MeshFeatureProcessor requires a TransformServiceFeatureProcessor on its parent scene.");

            m_rayTracingFeatureProcessor = GetParentScene()->GetFeatureProcessor<RayTracingFeatureProcessor>();

            m_handleGlobalShaderOptionUpdate = RPI::ShaderSystemInterface::GlobalShaderOptionUpdatedEvent::Handler
            {
                [this](const AZ::Name&, RPI::ShaderOptionValue) { m_forceRebuildDrawPackets = true; }
            };
            RPI::ShaderSystemInterface::Get()->Connect(m_handleGlobalShaderOptionUpdate);
            EnableSceneNotification();
        }

        void MeshFeatureProcessor::Deactivate()
        {
            m_handleGlobalShaderOptionUpdate.Disconnect();

            DisableSceneNotification();
            AZ_Warning("MeshFeatureProcessor", m_meshData.size() == 0,
                "Deactivaing the MeshFeatureProcessor, but there are still outstanding mesh handles.\n"
            );
            m_transformService = nullptr;
            m_forceRebuildDrawPackets = false;
        }

        void MeshFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);
            AZ_ATOM_PROFILE_FUNCTION("RPI", "MeshFeatureProcessor: Simulate");
            AZ_UNUSED(packet);

            AZStd::concurrency_check_scope scopeCheck(m_meshDataChecker);

            const auto iteratorRanges = m_meshData.GetParallelRanges();
            AZ::JobCompletion jobCompletion;
            for (const auto& iteratorRange : iteratorRanges)
            {
                const auto jobLambda = [&]() -> void
                {
                    AZ_PROFILE_SCOPE(Debug::ProfileCategory::AzRender, "MeshFP::Simulate() Lambda");
                    for (auto meshDataIter = iteratorRange.first; meshDataIter != iteratorRange.second; ++meshDataIter)
                    {
                        if (!meshDataIter->m_model)
                        {
                            continue;   // model not loaded yet
                        }

                        if (!meshDataIter->m_visible)
                        {
                            continue;
                        }

                        if (meshDataIter->m_objectSrgNeedsUpdate)
                        {
                            meshDataIter->UpdateObjectSrg();
                        }

                        // [GFX TODO] [ATOM-1357] Currently all of the draw packets have to be checked for material ID changes because
                        // material properties can impact which actual shader is used, which impacts the SRG in the draw packet.
                        // This is scheduled to be optimized so the work is only done on draw packets that need it instead of having
                        // to check every one.
                        meshDataIter->UpdateDrawPackets(m_forceRebuildDrawPackets);

                        if (meshDataIter->m_cullableNeedsRebuild)
                        {
                            meshDataIter->BuildCullable();
                        }
                    }
                };
                Job* executeGroupJob = aznew JobFunction<decltype(jobLambda)>(jobLambda, true, nullptr); // Auto-deletes
                executeGroupJob->SetDependent(&jobCompletion);
                executeGroupJob->Start();
            }
            jobCompletion.StartAndWaitForCompletion();

            m_forceRebuildDrawPackets = false;

            // CullingSystem::RegisterOrUpdateCullable() is not threadsafe, so need to do those updates in a single thread
            for (MeshDataInstance& meshDataInstance : m_meshData)
            {
                if (meshDataInstance.m_model && meshDataInstance.m_cullBoundsNeedsUpdate)
                {
                    meshDataInstance.UpdateCullBounds(m_transformService);
                }
            }
        }

        void MeshFeatureProcessor::OnBeginPrepareRender()
        {
            m_meshDataChecker.soft_lock();
        }

        void MeshFeatureProcessor::OnEndPrepareRender()
        {
            m_meshDataChecker.soft_unlock();
        }

        MeshFeatureProcessor::MeshHandle MeshFeatureProcessor::AcquireMesh(
            const Data::Asset<RPI::ModelAsset>& modelAsset,
            const MaterialAssignmentMap& materials,
            bool skinnedMeshWithMotion,
            bool rayTracingEnabled)
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);

            // don't need to check the concurrency during emplace() because the StableDynamicArray won't move the other elements during insertion
            MeshHandle meshDataHandle = m_meshData.emplace();

            // Mark skinned meshes to enable special processes to generate motion vector 
            meshDataHandle->m_skinnedMeshWithMotion = skinnedMeshWithMotion;

            // set ray tracing flag, but always disable on skinned meshes
            // [GFX TODO][ATOM-13067] Enable raytracing on skinned meshes
            meshDataHandle->m_rayTracingEnabled = rayTracingEnabled && (skinnedMeshWithMotion == false);

            meshDataHandle->m_scene = GetParentScene();
            meshDataHandle->m_materialAssignments = materials;

            meshDataHandle->m_objectId = m_transformService->ReserveObjectId();
            meshDataHandle->m_meshLoader = AZStd::make_unique<MeshDataInstance::MeshLoader>(modelAsset, &*meshDataHandle);

            return meshDataHandle;
        }

        MeshFeatureProcessor::MeshHandle MeshFeatureProcessor::AcquireMesh(
            const Data::Asset<RPI::ModelAsset>& modelAsset,
            const Data::Instance<RPI::Material>& material,
            bool skinnedMeshWithMotion,
            bool rayTracingEnabled)
        {
            Render::MaterialAssignmentMap materials;
            Render::MaterialAssignment& defaultMaterial = materials[AZ::Render::DefaultMaterialAssignmentId];
            defaultMaterial.m_materialInstance = material;

            return AcquireMesh(modelAsset, materials, skinnedMeshWithMotion, rayTracingEnabled);
        }

        bool MeshFeatureProcessor::ReleaseMesh(MeshHandle& meshHandle)
        {
            if (meshHandle.IsValid())
            {
                meshHandle->DeInit();
                m_transformService->ReleaseObjectId(meshHandle->m_objectId);

                AZStd::concurrency_check_scope scopeCheck(m_meshDataChecker);
                m_meshData.erase(meshHandle);

                return true;
            }
            return false;
        }

        MeshFeatureProcessor::MeshHandle MeshFeatureProcessor::CloneMesh(const MeshHandle& meshHandle)
        {
            if (meshHandle.IsValid())
            {
                MeshHandle clone = AcquireMesh(meshHandle->m_model->GetModelAsset(), meshHandle->m_materialAssignments);
                return clone;
            }
            return MeshFeatureProcessor::MeshHandle();
        }

        Data::Instance<RPI::Model> MeshFeatureProcessor::GetModel(const MeshHandle& meshHandle) const
        {
            return meshHandle.IsValid() ? meshHandle->m_model : nullptr;
        }

        void MeshFeatureProcessor::SetMaterialAssignmentMap(const MeshHandle& meshHandle, const Data::Instance<RPI::Material>& material)
        {
            Render::MaterialAssignmentMap materials;
            Render::MaterialAssignment& defaultMaterial = materials[AZ::Render::DefaultMaterialAssignmentId];
            defaultMaterial.m_materialInstance = material;

            return SetMaterialAssignmentMap(meshHandle, materials);
        }

        void MeshFeatureProcessor::SetMaterialAssignmentMap(const MeshHandle& meshHandle, const MaterialAssignmentMap& materials)
        {
            if (meshHandle.IsValid())
            {
                if (meshHandle->m_model)
                {
                    Data::Instance<RPI::Model> model = meshHandle->m_model;
                    meshHandle->DeInit();
                    meshHandle->m_materialAssignments = materials;
                    meshHandle->Init(model);
                }
                else
                {
                    meshHandle->m_materialAssignments = materials;
                }
            }
        }

        const MaterialAssignmentMap& MeshFeatureProcessor::GetMaterialAssignmentMap(const MeshHandle& meshHandle) const
        {
            return meshHandle.IsValid() ? meshHandle->m_materialAssignments : DefaultMaterialAssignmentMap;
        }

        void MeshFeatureProcessor::ConnectModelChangeEventHandler(const MeshHandle& meshHandle, ModelChangedEvent::Handler& handler)
        {
            if (meshHandle.IsValid())
            {
                handler.Connect(meshHandle->m_meshLoader->GetModelChangedEvent());
            }
        }

        void MeshFeatureProcessor::SetTransform(const MeshHandle& meshHandle, const AZ::Transform& transform)
        {
            if (meshHandle.IsValid())
            {
                MeshDataInstance& meshData = *meshHandle;
                meshData.m_cullBoundsNeedsUpdate = true;
                meshData.m_objectSrgNeedsUpdate = true;

                m_transformService->SetTransformForId(meshHandle->m_objectId, transform);

                // ray tracing data needs to be updated with the new transform
                if (m_rayTracingFeatureProcessor)
                {
                    m_rayTracingFeatureProcessor->SetMeshTransform(meshHandle->m_objectId, transform);
                }
            }
        }

        Transform MeshFeatureProcessor::GetTransform(const MeshHandle& meshHandle)
        {
            if (meshHandle.IsValid())
            {
                return m_transformService->GetTransformForId(meshHandle->m_objectId);
            }
            else
            {
                AZ_Assert(false, "Invalid mesh handle");
                return Transform::CreateIdentity();
            }
        }

        void MeshFeatureProcessor::SetSortKey(const MeshHandle& meshHandle, RHI::DrawItemSortKey sortKey)
        {
            if (meshHandle.IsValid())
            {
                meshHandle->SetSortKey(sortKey);
            }
        }

        RHI::DrawItemSortKey MeshFeatureProcessor::GetSortKey(const MeshHandle& meshHandle)
        {
            if (meshHandle.IsValid())
            {
                return meshHandle->GetSortKey();
            }
            else
            {
                AZ_Assert(false, "Invalid mesh handle");
                return 0;
            }
        }

        void MeshFeatureProcessor::SetLodOverride(const MeshHandle& meshHandle, RPI::Cullable::LodOverride lodOverride)
        {
            if (meshHandle.IsValid())
            {
                meshHandle->SetLodOverride(lodOverride);
            }
        }

        RPI::Cullable::LodOverride MeshFeatureProcessor::GetLodOverride(const MeshHandle& meshHandle)
        {
            if (meshHandle.IsValid())
            {
                return meshHandle->GetLodOverride();
            }
            else
            {
                AZ_Assert(false, "Invalid mesh handle");
                return 0;
            }
        }

        void MeshFeatureProcessor::SetExcludeFromReflectionCubeMaps(const MeshHandle& meshHandle, bool excludeFromReflectionCubeMaps)
        {
            if (meshHandle.IsValid())
            {
                meshHandle->m_excludeFromReflectionCubeMaps = excludeFromReflectionCubeMaps;
            }
        }

        void MeshFeatureProcessor::SetRayTracingEnabled(const MeshHandle& meshHandle, bool rayTracingEnabled)
        {
            if (meshHandle.IsValid())
            {
                // update the ray tracing data based on the current state and the new state
                if (rayTracingEnabled && !meshHandle->m_rayTracingEnabled)
                {
                    // add to ray tracing
                    meshHandle->SetRayTracingData();
                }
                else if (!rayTracingEnabled && meshHandle->m_rayTracingEnabled)
                {
                    // remove from ray tracing
                    if (m_rayTracingFeatureProcessor)
                    {
                        m_rayTracingFeatureProcessor->RemoveMesh(meshHandle->m_objectId);
                    }
                }

                // set new state
                meshHandle->m_rayTracingEnabled = rayTracingEnabled;
            }
        }

        void MeshFeatureProcessor::SetVisible(const MeshHandle& meshHandle, bool visible)
        {
            if (meshHandle.IsValid())
            {
                meshHandle->m_visible = visible;
            }
        }

        void MeshFeatureProcessor::SetUseForwardPassIblSpecular(const MeshHandle& meshHandle, bool useForwardPassIblSpecular)
        {
            if (meshHandle.IsValid())
            {
                meshHandle->m_useForwardPassIblSpecular = useForwardPassIblSpecular;
                meshHandle->m_objectSrgNeedsUpdate = true;

                if (meshHandle->m_model)
                {
                    const size_t modelLodCount = meshHandle->m_model->GetLodCount();
                    for (size_t modelLodIndex = 0; modelLodIndex < modelLodCount; ++modelLodIndex)
                    {
                        meshHandle->BuildDrawPacketList(modelLodIndex);
                    }
                }
            }
        }

        void MeshFeatureProcessor::OnRenderPipelineAdded(RPI::RenderPipelinePtr pipeline)
        {
            m_forceRebuildDrawPackets = true;;
        }

        void MeshFeatureProcessor::OnRenderPipelineRemoved([[maybe_unused]] RPI::RenderPipeline* pipeline)
        {
            m_forceRebuildDrawPackets = true;
        }

        void MeshFeatureProcessor::UpdateMeshReflectionProbes()
        {
            // we need to rebuild the Srg for any meshes that are using the forward pass IBL specular option
            for (auto& meshInstance : m_meshData)
            {
                if (meshInstance.m_useForwardPassIblSpecular)
                {
                    meshInstance.m_objectSrgNeedsUpdate = true;
                }
            }
        }

        // MeshDataInstance::MeshLoader...

        MeshDataInstance::MeshLoader::MeshLoader(const Data::Asset<RPI::ModelAsset>& modelAsset, MeshDataInstance* parent)
            : m_modelAsset(modelAsset)
            , m_parent(parent)
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);

            if (!m_modelAsset.GetId().IsValid())
            {
                AZ_Error("MeshDataInstance::MeshLoader", false, "Invalid model asset Id.");
                return;
            }

            // Check if the model is in the instance database
            Data::Instance<RPI::Model> model = Data::InstanceDatabase<RPI::Model>::Instance().Find(Data::InstanceId::CreateFromAssetId(m_modelAsset.GetId()));
            if (model)
            {
                m_parent->Init(model);
                m_modelChangedEvent.Signal(AZStd::move(model));
                return;
            }

            m_modelAsset.QueueLoad();
            Data::AssetBus::Handler::BusConnect(modelAsset.GetId());
        }

        MeshDataInstance::MeshLoader::~MeshLoader()
        {
            Data::AssetBus::Handler::BusDisconnect();
        }

        MeshFeatureProcessorInterface::ModelChangedEvent& MeshDataInstance::MeshLoader::GetModelChangedEvent()
        {
            return m_modelChangedEvent;
        }

        //! AssetBus::Handler overrides...
        void MeshDataInstance::MeshLoader::OnAssetReady(Data::Asset<Data::AssetData> asset)
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);

            Data::Instance<RPI::Model> model = RPI::Model::FindOrCreate(asset);
            if (model)
            {
                m_parent->Init(model);
                m_modelChangedEvent.Signal(AZStd::move(model));
            }
            else
            {
                AZ_Error("MeshDataInstance::OnAssetReady", false, "Failed to create model instance for '%s'", asset.GetHint().c_str());
            }
        }

        void MeshDataInstance::MeshLoader::OnAssetError(Data::Asset<Data::AssetData> asset)
        {
            // Note: m_modelAsset and asset represents same asset, but only m_modelAsset contains the file path in its hint from serialization
            AZ_Error("MeshDataInstance::MeshLoader", false, "Failed to load asset %s.", m_modelAsset.GetHint().c_str()); 
        }

        // MeshDataInstance...

        void MeshDataInstance::DeInit()
        {
            m_scene->GetCullingSystem()->UnregisterCullable(m_cullable);

            // remove from ray tracing
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = m_scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            if (rayTracingFeatureProcessor)
            {
                rayTracingFeatureProcessor->RemoveMesh(m_objectId);
            }

            m_meshLoader.reset();
            m_drawPacketListsByLod.clear();
            m_materialAssignments.clear();
            m_shaderResourceGroup = {};
            m_model = {};
        }

        void MeshDataInstance::Init(Data::Instance<RPI::Model> model)
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);

            auto modelAsset = model->GetModelAsset();
            for (const auto& modelLodAsset : modelAsset->GetLodAssets())
            {
                for (const auto& mesh : modelLodAsset->GetMeshes())
                {
                    if (mesh.GetMaterialAsset().GetStatus() != Data::AssetData::AssetStatus::Ready)
                    {

                    }
                }
            }

            m_model = model;
            const size_t modelLodCount = m_model->GetLodCount();
            m_drawPacketListsByLod.resize(modelLodCount);
            for (size_t modelLodIndex = 0; modelLodIndex < modelLodCount; ++modelLodIndex)
            {
                BuildDrawPacketList(modelLodIndex);
            }

            if (m_shaderResourceGroup)
            {
                // Set object Id once since it never changes
                RHI::ShaderInputNameIndex objectIdIndex = "m_objectId";
                m_shaderResourceGroup->SetConstant(objectIdIndex, m_objectId.GetIndex());
                objectIdIndex.AssertValid();
            }

            if (m_rayTracingEnabled)
            {
                SetRayTracingData();
            }

            m_cullableNeedsRebuild = true;
            m_cullBoundsNeedsUpdate = true;
            m_objectSrgNeedsUpdate = true;
        }

        void MeshDataInstance::BuildDrawPacketList(size_t modelLodIndex)
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);

            RPI::ModelLod& modelLod = *m_model->GetLods()[modelLodIndex];
            const size_t meshCount = modelLod.GetMeshes().size();

            MeshDataInstance::DrawPacketList& drawPacketListOut = m_drawPacketListsByLod[modelLodIndex];
            drawPacketListOut.clear();
            drawPacketListOut.reserve(meshCount);

            for (size_t meshIndex = 0; meshIndex < meshCount; ++meshIndex)
            {
                Data::Instance<RPI::Material> material = modelLod.GetMeshes()[meshIndex].m_material;

                // Determine if there is a material override specified for this sub mesh
                const MaterialAssignmentId materialAssignmentId(modelLodIndex, material ? material->GetAssetId() : AZ::Data::AssetId());
                const MaterialAssignment& materialAssignment = GetMaterialAssignmentFromMapWithFallback(m_materialAssignments, materialAssignmentId);
                if (materialAssignment.m_materialInstance.get())
                {
                    material = materialAssignment.m_materialInstance;
                }

                if (!material)
                {
                    AZ_Warning("MeshFeatureProcessor", false, "No material provided for mesh. Skipping.");
                    continue;
                }

                auto& objectSrgAsset = material->GetAsset()->GetObjectSrgAsset();

                if (!objectSrgAsset)
                {
                    AZ_Warning("MeshFeatureProcessor", false, "No per-object ShaderResourceGroup found.");
                    continue;
                }

                if (m_shaderResourceGroup && m_shaderResourceGroup->GetAsset() != objectSrgAsset)
                {
                    AZ_Warning("MeshFeatureProcessor", false, "All materials on a model must use the same per-object ShaderResourceGroup. Skipping.");
                    continue;
                }

                // The first time we find the per-surface SRG asset we create an instance and store it
                // in shaderResourceGroupInOut. All of the Model's draw packets will use this same instance.
                if (!m_shaderResourceGroup)
                {
                    m_shaderResourceGroup = RPI::ShaderResourceGroup::Create(objectSrgAsset);
                    if (!m_shaderResourceGroup)
                    {
                        AZ_Warning("MeshFeatureProcessor", false, "Failed to create a new shader resource group, skipping.");
                        continue;
                    }
                }

                SelectMotionVectorShader(material);

                // setup the mesh draw packet
                RPI::MeshDrawPacket drawPacket(modelLod, meshIndex, material, m_shaderResourceGroup, materialAssignment.m_matModUvOverrides);

                // set the shader option to select forward pass IBL specular if necessary
                if (!drawPacket.SetShaderOption(AZ::Name("o_meshUseForwardPassIBLSpecular"), AZ::RPI::ShaderOptionValue{ m_useForwardPassIblSpecular }))
                {
                    AZ_Warning("MeshDrawPacket", false, "Failed to set o_meshUseForwardPassIBLSpecular on mesh draw packet");
                }

                drawPacket.SetStencilRef(m_useForwardPassIblSpecular || MaterialRequiresForwardPassIblSpecular(material) ? Render::StencilRefs::None : Render::StencilRefs::UseIBLSpecularPass);
                drawPacket.SetSortKey(m_sortKey);
                drawPacket.Update(*m_scene, false);
                drawPacketListOut.emplace_back(AZStd::move(drawPacket));
            }
        }

        void MeshDataInstance::SetRayTracingData()
        {
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = m_scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            if (rayTracingFeatureProcessor == nullptr)
            {
                return;
            }

            const AZStd::array_view<Data::Instance<RPI::ModelLod>>& modelLods = m_model->GetLods();
            if (modelLods.empty())
            {
                return;
            }

            // use the lowest LOD for raytracing
            uint32_t rayTracingLod = aznumeric_cast<uint32_t>(modelLods.size() - 1);
            const Data::Instance<RPI::ModelLod>& modelLod = modelLods[rayTracingLod];

            // setup a stream layout and shader input contract for the position and normal streams
            static const char* PositionSemantic = "POSITION";
            static const char* NormalSemantic = "NORMAL";
            static const RHI::Format StreamFormat = RHI::Format::R32G32B32_FLOAT;

            RHI::InputStreamLayoutBuilder layoutBuilder;
            layoutBuilder.AddBuffer()->Channel(PositionSemantic, StreamFormat);
            layoutBuilder.AddBuffer()->Channel(NormalSemantic, StreamFormat);
            RHI::InputStreamLayout inputStreamLayout = layoutBuilder.End();

            RPI::ShaderInputContract::StreamChannelInfo positionStreamChannelInfo;
            positionStreamChannelInfo.m_semantic = RHI::ShaderSemantic(AZ::Name(PositionSemantic));
            positionStreamChannelInfo.m_componentCount = RHI::GetFormatComponentCount(StreamFormat);

            RPI::ShaderInputContract::StreamChannelInfo normalStreamChannelInfo;
            normalStreamChannelInfo.m_semantic = RHI::ShaderSemantic(AZ::Name(NormalSemantic));
            normalStreamChannelInfo.m_componentCount = RHI::GetFormatComponentCount(StreamFormat);

            RPI::ShaderInputContract shaderInputContract;
            shaderInputContract.m_streamChannels.emplace_back(positionStreamChannelInfo);
            shaderInputContract.m_streamChannels.emplace_back(normalStreamChannelInfo);

            // setup the raytracing data for each sub-mesh 
            const size_t meshCount = modelLod->GetMeshes().size();
            RayTracingFeatureProcessor::SubMeshVector subMeshes;
            for (uint32_t meshIndex = 0; meshIndex < meshCount; ++meshIndex)
            {
                const RPI::ModelLod::Mesh& mesh = modelLod->GetMeshes()[meshIndex];

                // retrieve vertex/index buffers
                RPI::ModelLod::StreamBufferViewList streamBufferViews;
                [[maybe_unused]] bool result = modelLod->GetStreamsForMesh(inputStreamLayout, streamBufferViews, shaderInputContract, meshIndex);
                AZ_Assert(result, "Failed to retrieve mesh stream buffer views");

                // note that the element count is the size of the entire buffer, even though this mesh may only
                // occupy a portion of the vertex buffer.  This is necessary since we are accessing it using
                // a ByteAddressBuffer in the raytracing shaders and passing the byte offset to the shader in a constant buffer.
                uint32_t vertexBufferByteCount = const_cast<RHI::Buffer*>(streamBufferViews[0].GetBuffer())->GetDescriptor().m_byteCount;
                RHI::BufferViewDescriptor vertexBufferDescriptor = RHI::BufferViewDescriptor::CreateRaw(0, vertexBufferByteCount);

                const RHI::IndexBufferView& indexBufferView = mesh.m_indexBufferView;
                uint32_t indexElementSize = indexBufferView.GetIndexFormat() == RHI::IndexFormat::Uint16 ? 2 : 4;
                uint32_t indexElementCount = (uint32_t)indexBufferView.GetBuffer()->GetDescriptor().m_byteCount / indexElementSize;
                RHI::BufferViewDescriptor indexBufferDescriptor;
                indexBufferDescriptor.m_elementOffset = 0;
                indexBufferDescriptor.m_elementCount = indexElementCount;
                indexBufferDescriptor.m_elementSize = indexElementSize;
                indexBufferDescriptor.m_elementFormat = indexBufferView.GetIndexFormat() == RHI::IndexFormat::Uint16 ? RHI::Format::R16_UINT : RHI::Format::R32_UINT;

                // retrieve the material
                Data::Instance<RPI::Material> material = mesh.m_material;

                const MaterialAssignmentId materialAssignmentId(rayTracingLod, material ? material->GetAssetId() : AZ::Data::AssetId());
                const MaterialAssignment& materialAssignment = GetMaterialAssignmentFromMapWithFallback(m_materialAssignments, materialAssignmentId);
                if (materialAssignment.m_materialInstance.get())
                {
                    material = materialAssignment.m_materialInstance;
                }

                AZ::Color irradianceColor(1.0f, 1.0f, 1.0f, 1.0f);
                if (material)
                {
                    RPI::MaterialPropertyIndex propertyIndex = material->FindPropertyIndex(AZ::Name("irradiance.color"));
                    if (propertyIndex.IsValid())
                    {
                        irradianceColor = material->GetPropertyValue<AZ::Color>(propertyIndex);
                    }

                    propertyIndex = material->FindPropertyIndex(AZ::Name("irradiance.factor"));
                    if (propertyIndex.IsValid())
                    {
                        irradianceColor *= material->GetPropertyValue<float>(propertyIndex);
                    }
                }

                RayTracingFeatureProcessor::SubMesh subMesh;
                subMesh.m_vertexFormat = StreamFormat;
                subMesh.m_positionVertexBufferView = streamBufferViews[0];
                subMesh.m_positionShaderBufferView = const_cast<RHI::Buffer*>(streamBufferViews[0].GetBuffer())->GetBufferView(vertexBufferDescriptor);
                subMesh.m_normalVertexBufferView = streamBufferViews[1];
                subMesh.m_normalShaderBufferView = const_cast<RHI::Buffer*>(streamBufferViews[1].GetBuffer())->GetBufferView(vertexBufferDescriptor);
                subMesh.m_indexBufferView = mesh.m_indexBufferView;
                subMesh.m_indexShaderBufferView = const_cast<RHI::Buffer*>(mesh.m_indexBufferView.GetBuffer())->GetBufferView(indexBufferDescriptor);
                subMesh.m_irradianceColor = irradianceColor;
                subMeshes.push_back(subMesh);
            }

            rayTracingFeatureProcessor->SetMesh(m_objectId, subMeshes);
        }

        void MeshDataInstance::SetSortKey(RHI::DrawItemSortKey sortKey)
        {
            m_sortKey = sortKey;
            for (auto& drawPacketList : m_drawPacketListsByLod)
            {
                for (auto& drawPacket : drawPacketList)
                {
                    drawPacket.SetSortKey(sortKey);
                }
            }
        }

        RHI::DrawItemSortKey MeshDataInstance::GetSortKey()
        {
            return m_sortKey;
        }

        void MeshDataInstance::SetLodOverride(RPI::Cullable::LodOverride lodOverride)
        {
            m_cullable.m_lodData.m_lodOverride = lodOverride;
        }

        RPI::Cullable::LodOverride MeshDataInstance::GetLodOverride()
        {
            return m_cullable.m_lodData.m_lodOverride;
        }

        void MeshDataInstance::UpdateDrawPackets(bool forceUpdate /*= false*/)
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);
            for (auto& drawPacketList : m_drawPacketListsByLod)
            {
                for (auto& drawPacket : drawPacketList)
                {
                    if (drawPacket.Update(*m_scene, forceUpdate))
                    {
                        m_cullableNeedsRebuild = true;
                    }
                }
            }
        }

        void MeshDataInstance::BuildCullable()
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);
            AZ_Assert(m_cullableNeedsRebuild, "This function only needs to be called if the cullable to be rebuilt");
            AZ_Assert(m_model, "The model has not finished loading yet");

            RPI::Cullable::CullData& cullData = m_cullable.m_cullData;
            RPI::Cullable::LodData& lodData = m_cullable.m_lodData;

            const Aabb& localAabb = m_model->GetAabb();
            lodData.m_lodSelectionRadius = 0.5f*localAabb.GetExtents().GetMaxElement();

            const size_t modelLodCount = m_model->GetLodCount();
            const auto& lodAssets = m_model->GetModelAsset()->GetLodAssets();
            AZ_Assert(lodAssets.size() == modelLodCount, "Number of asset lods must match number of model lods");

            lodData.m_lods.resize(modelLodCount);
            cullData.m_drawListMask.reset();

            const size_t lodCount = lodAssets.size();
            for (size_t lodIndex = 0; lodIndex < lodCount; ++lodIndex)
            {
                //initialize the lod
                RPI::Cullable::LodData::Lod& lod = lodData.m_lods[lodIndex];
                //[GFX TODO][ATOM-5562] - Level of detail: override lod distances and add global lod multiplier(s)
                static const float MinimumScreenCoverage = 1.0f/1080.0f;        //mesh should cover at least a screen pixel at 1080p to be drawn
                static const float ReductionFactor = 0.5f;
                if (lodIndex == 0)
                {
                    //first lod
                    lod.m_screenCoverageMax = 1.0f;
                }
                else
                {
                    //every other lod: use the previous lod's min
                    lod.m_screenCoverageMax = AZStd::GetMax(lodData.m_lods[lodIndex-1].m_screenCoverageMin, MinimumScreenCoverage);
                }
                if (lodIndex < lodAssets.size() - 1)
                {
                    //first and middle lods: compute a stepdown value for the min
                    lod.m_screenCoverageMin = AZStd::GetMax(ReductionFactor * lod.m_screenCoverageMax, MinimumScreenCoverage);
                }
                else
                {
                    //last lod: use MinimumScreenCoverage for the min
                    lod.m_screenCoverageMin = MinimumScreenCoverage;
                }

                lod.m_drawPackets.clear();
                for (const RPI::MeshDrawPacket& meshDrawPacket : m_drawPacketListsByLod[lodIndex])
                {
                    const RHI::DrawPacket* rhiDrawPacket = meshDrawPacket.GetRHIDrawPacket();

                    if (rhiDrawPacket)
                    {
                        //OR-together all the drawListMasks (so we know which views to cull against)
                        cullData.m_drawListMask |= rhiDrawPacket->GetDrawListMask();

                        lod.m_drawPackets.push_back(rhiDrawPacket);
                    }
                }
            }

            cullData.m_hideFlags = RPI::View::UsageNone;
            if (m_excludeFromReflectionCubeMaps)
            {
                cullData.m_hideFlags |= RPI::View::UsageReflectiveCubeMap;
            }

            cullData.m_scene = m_scene;     //[GFX_TODO][ATOM-13796] once the IVisibilitySystem supports multiple octree scenes, remove this

#ifdef AZ_CULL_DEBUG_ENABLED
            m_cullable.SetDebugName(AZ::Name(AZStd::string::format("%s - objectId: %u", m_model->GetModelAsset()->GetName().GetCStr(), m_objectId.GetIndex())));
#endif

            m_cullableNeedsRebuild = false;
            m_cullBoundsNeedsUpdate = true;
        }

        void MeshDataInstance::UpdateCullBounds(const TransformServiceFeatureProcessor* transformService)
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);
            AZ_Assert(m_cullBoundsNeedsUpdate, "This function only needs to be called if the culling bounds need to be rebuilt");
            AZ_Assert(m_model, "The model has not finished loading yet");

            Transform localToWorld = transformService->GetTransformForId(m_objectId);

            Vector3 center;
            float radius;
            Aabb localAabb = m_model->GetAabb();
            localAabb.GetTransformedAabb(localToWorld).GetAsSphere(center, radius);

            m_cullable.m_cullData.m_boundingSphere = Sphere(center, radius);
            m_cullable.m_cullData.m_boundingObb = localAabb.GetTransformedObb(localToWorld);
            m_cullable.m_cullData.m_visibilityEntry.m_boundingVolume = localAabb.GetTransformedAabb(localToWorld);
            m_cullable.m_cullData.m_visibilityEntry.m_userData = &m_cullable;
            m_cullable.m_cullData.m_visibilityEntry.m_typeFlags = AzFramework::VisibilityEntry::TYPE_RPI_Cullable;
            m_scene->GetCullingSystem()->RegisterOrUpdateCullable(m_cullable);

            m_cullBoundsNeedsUpdate = false;
        }

        void MeshDataInstance::SelectMotionVectorShader(Data::Instance<RPI::Material> material)
        {
            // Two motion vector shaders are defined in the material for static mesh (only animated by transform matrix)
            // and skinned mesh (per vertex animation) respectively, it's because they have different input signatures
            // (skinned mesh needs two streaming channels while static mesh only needs one) that cannot be addressed by shader option
            // itself. Therefore this function is used to pick one to use and disable the other one depending on the type of the mesh
            // so it won't cause errors due to missing input streaming channel.

            //[GFX TODO][ATOM-4726] Replace this with a "isSkinnedMesh" external material property and a functor that enables/disables the appropriate shader
            for (auto& shaderItem : material->GetShaderCollection())
            {
                if (shaderItem.GetShaderAsset()->GetName() == Name{ "StaticMeshMotionVector" } && m_skinnedMeshWithMotion)
                {
                    shaderItem.SetEnabled(false);
                }
                 
                if (shaderItem.GetShaderAsset()->GetName() == Name{ "SkinnedMeshMotionVector" } && (!m_skinnedMeshWithMotion))
                {
                    shaderItem.SetEnabled(false);
                }
            }
        }

        void MeshDataInstance::UpdateObjectSrg()
        {
            if (!m_shaderResourceGroup)
            {
                return;
            }

            if (m_useForwardPassIblSpecular)
            {
                // retrieve probe constant indices
                AZ::RHI::ShaderInputConstantIndex posConstantIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name("m_reflectionProbeData.m_aabbPos"));
                AZ_Error("MeshDataInstance", posConstantIndex.IsValid(), "Failed to find ReflectionProbe constant index");

                AZ::RHI::ShaderInputConstantIndex outerAabbMinConstantIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name("m_reflectionProbeData.m_outerAabbMin"));
                AZ_Error("MeshDataInstance", outerAabbMinConstantIndex.IsValid(), "Failed to find ReflectionProbe constant index");

                AZ::RHI::ShaderInputConstantIndex outerAabbMaxConstantIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name("m_reflectionProbeData.m_outerAabbMax"));
                AZ_Error("MeshDataInstance", outerAabbMaxConstantIndex.IsValid(), "Failed to find ReflectionProbe constant index");

                AZ::RHI::ShaderInputConstantIndex innerAabbMinConstantIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name("m_reflectionProbeData.m_innerAabbMin"));
                AZ_Error("MeshDataInstance", innerAabbMinConstantIndex.IsValid(), "Failed to find ReflectionProbe constant index");

                AZ::RHI::ShaderInputConstantIndex innerAabbMaxConstantIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name("m_reflectionProbeData.m_innerAabbMax"));
                AZ_Error("MeshDataInstance", innerAabbMaxConstantIndex.IsValid(), "Failed to find ReflectionProbe constant index");

                AZ::RHI::ShaderInputConstantIndex useReflectionProbeConstantIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name("m_reflectionProbeData.m_useReflectionProbe"));
                AZ_Error("MeshDataInstance", useReflectionProbeConstantIndex.IsValid(), "Failed to find ReflectionProbe constant index");

                AZ::RHI::ShaderInputConstantIndex useParallaxCorrectionConstantIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name("m_reflectionProbeData.m_useParallaxCorrection"));
                AZ_Error("MeshDataInstance", useParallaxCorrectionConstantIndex.IsValid(), "Failed to find ReflectionProbe constant index");

                // retrieve probe cubemap index
                Name reflectionCubeMapImageName = Name("m_reflectionProbeCubeMap");
                RHI::ShaderInputImageIndex reflectionCubeMapImageIndex = m_shaderResourceGroup->FindShaderInputImageIndex(reflectionCubeMapImageName);
                AZ_Error("MeshDataInstance", reflectionCubeMapImageIndex.IsValid(), "Failed to find shader image index [%s]", reflectionCubeMapImageName.GetCStr());

                // retrieve the list of probes that contain the centerpoint of the mesh
                TransformServiceFeatureProcessor* transformServiceFeatureProcessor = m_scene->GetFeatureProcessor<TransformServiceFeatureProcessor>();
                Transform transform = transformServiceFeatureProcessor->GetTransformForId(m_objectId);

                ReflectionProbeFeatureProcessor* reflectionProbeFeatureProcessor = m_scene->GetFeatureProcessor<ReflectionProbeFeatureProcessor>();
                ReflectionProbeFeatureProcessor::ReflectionProbeVector reflectionProbes;
                reflectionProbeFeatureProcessor->FindReflectionProbes(transform.GetTranslation(), reflectionProbes);

                if (!reflectionProbes.empty() && reflectionProbes[0])
                {
                    m_shaderResourceGroup->SetConstant(posConstantIndex, reflectionProbes[0]->GetPosition());
                    m_shaderResourceGroup->SetConstant(outerAabbMinConstantIndex, reflectionProbes[0]->GetOuterAabbWs().GetMin());
                    m_shaderResourceGroup->SetConstant(outerAabbMaxConstantIndex, reflectionProbes[0]->GetOuterAabbWs().GetMax());
                    m_shaderResourceGroup->SetConstant(innerAabbMinConstantIndex, reflectionProbes[0]->GetInnerAabbWs().GetMin());
                    m_shaderResourceGroup->SetConstant(innerAabbMaxConstantIndex, reflectionProbes[0]->GetInnerAabbWs().GetMax());
                    m_shaderResourceGroup->SetConstant(useReflectionProbeConstantIndex, true);
                    m_shaderResourceGroup->SetConstant(useParallaxCorrectionConstantIndex, reflectionProbes[0]->GetUseParallaxCorrection());

                    m_shaderResourceGroup->SetImage(reflectionCubeMapImageIndex, reflectionProbes[0]->GetCubeMapImage());
                }
                else
                {
                    m_shaderResourceGroup->SetConstant(useReflectionProbeConstantIndex, false);
                }
            }

            m_shaderResourceGroup->Compile();
            m_objectSrgNeedsUpdate = false;
        }

        bool MeshDataInstance::MaterialRequiresForwardPassIblSpecular(Data::Instance<RPI::Material> material) const
        {
            RPI::MaterialPropertyIndex propertyIndex = material->FindPropertyIndex(AZ::Name("general.forwardPassIBLSpecular"));
            if (propertyIndex.IsValid())
            {
               return material->GetPropertyValue<bool>(propertyIndex);
            }

            return false;
        }
    } // namespace Render
} // namespace AZ
