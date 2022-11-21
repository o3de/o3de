/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/RHIUtils.h>
#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>
#include <Atom/Feature/RenderCommon.h>
#include <Atom/Feature/Mesh/MeshFeatureProcessor.h>
#include <Atom/Feature/Mesh/ModelReloaderSystemInterface.h>
#include <Atom/RPI.Public/Model/ModelLodUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Culling.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/Utils/StableDynamicArray.h>
#include <ReflectionProbe/ReflectionProbeFeatureProcessor.h>

#include <Atom/RPI.Reflect/Model/ModelAssetCreator.h>

#include <AzFramework/Asset/AssetSystemBus.h>

#include <AtomCore/Instance/InstanceDatabase.h>

#include <AzCore/Console/IConsole.h>
#include <AzCore/Jobs/Algorithms.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Math/ShapeIntersection.h>
#include <AzCore/RTTI/RTTI.h>
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
                    ->Version(1);
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

            // Must read cvar from AZ::Console due to static variable in multiple libraries, see ghi-5537
            bool enablePerMeshShaderOptionFlagsCvar = false;
            if (auto* console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
            {
                console->GetCvarValue("r_enablePerMeshShaderOptionFlags", enablePerMeshShaderOptionFlagsCvar);

                // push the cvars value so anything in this dll can access it directly.
                console->PerformCommand(AZStd::string::format("r_enablePerMeshShaderOptionFlags %s", enablePerMeshShaderOptionFlagsCvar ? "true" : "false").c_str());
            }
        }

        void MeshFeatureProcessor::Deactivate()
        {
            m_flagRegistry.reset();

            m_handleGlobalShaderOptionUpdate.Disconnect();

            DisableSceneNotification();
            AZ_Warning("MeshFeatureProcessor", m_modelData.size() == 0,
                "Deactivating the MeshFeatureProcessor, but there are still outstanding mesh handles.\n"
            );
            m_transformService = nullptr;
            m_forceRebuildDrawPackets = false;
        }

        TransformServiceFeatureProcessorInterface::ObjectId MeshFeatureProcessor::GetObjectId(const MeshHandle& meshHandle) const
        {
            if (meshHandle.IsValid())
            {
                return meshHandle->m_objectId;
            }

            return TransformServiceFeatureProcessorInterface::ObjectId::Null;
        }

        void MeshFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "MeshFeatureProcessor: Simulate");

            AZ::Job* parentJob = packet.m_parentJob;
            AZStd::concurrency_check_scope scopeCheck(m_meshDataChecker);

            const auto iteratorRanges = m_modelData.GetParallelRanges();
            AZ::JobCompletion jobCompletion;
            for (const auto& iteratorRange : iteratorRanges)
            {
                const auto jobLambda = [&]() -> void
                {
                    AZ_PROFILE_SCOPE(AzRender, "MeshFeatureProcessor: Simulate: Job");

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

                        if (meshDataIter->m_cullBoundsNeedsUpdate)
                        {
                            meshDataIter->UpdateCullBounds(m_transformService);
                        }
                    }
                };
                Job* executeGroupJob = aznew JobFunction<decltype(jobLambda)>(jobLambda, true, nullptr); // Auto-deletes
                if (parentJob)
                {
                    parentJob->StartAsChild(executeGroupJob);
                }
                else
                {
                    executeGroupJob->SetDependent(&jobCompletion);
                    executeGroupJob->Start();
                }
            }
            {
                AZ_PROFILE_SCOPE(AzRender, "MeshFeatureProcessor: Simulate: WaitForChildren");
                if (parentJob)
                {
                    parentJob->WaitForChildren();
                }
                else
                {
                    jobCompletion.StartAndWaitForCompletion();
                }
            }

            m_forceRebuildDrawPackets = false;
        }

        void MeshFeatureProcessor::OnBeginPrepareRender()
        {
            m_meshDataChecker.soft_lock();

            if (!r_enablePerMeshShaderOptionFlags && m_enablePerMeshShaderOptionFlags)
            {
                // Per mesh shader option flags was on, but now turned off, so reset all the shader options.
                for (auto& model : m_modelData)
                {
                    for (RPI::MeshDrawPacketList& drawPacketList : model.m_drawPacketListsByLod)
                    {
                        for (RPI::MeshDrawPacket& drawPacket : drawPacketList)
                        {
                            m_flagRegistry->VisitTags(
                                [&](AZ::Name shaderOption, [[maybe_unused]] FlagRegistry::TagType tag)
                                {
                                    drawPacket.UnsetShaderOption(shaderOption);
                                }
                            );
                            drawPacket.Update(*GetParentScene(), true);
                        }
                    }
                    model.m_cullable.m_flags = 0;
                    model.m_cullable.m_prevFlags = 0;
                    model.m_cullableNeedsRebuild = true;
                    model.BuildCullable();
                }
            }

            m_enablePerMeshShaderOptionFlags = r_enablePerMeshShaderOptionFlags;

            if (m_enablePerMeshShaderOptionFlags)
            {
                for (auto& model : m_modelData)
                {
                    if (model.m_cullable.m_prevFlags != model.m_cullable.m_flags)
                    {
                        // Per mesh shader option flags have changed, so rebuild the draw packet with the new shader options.
                        for (RPI::MeshDrawPacketList& drawPacketList : model.m_drawPacketListsByLod)
                        {
                            for (RPI::MeshDrawPacket& drawPacket : drawPacketList)
                            {
                                m_flagRegistry->VisitTags(
                                    [&](AZ::Name shaderOption, FlagRegistry::TagType tag)
                                    {
                                        bool shaderOptionValue = (model.m_cullable.m_flags & tag.GetIndex()) > 0;
                                        drawPacket.SetShaderOption(shaderOption, AZ::RPI::ShaderOptionValue(shaderOptionValue));
                                    }
                                );
                                drawPacket.Update(*GetParentScene(), true);
                            }
                        }
                        model.m_cullableNeedsRebuild = true;
                        model.BuildCullable();
                    }
                }
            }

        }

        void MeshFeatureProcessor::OnEndPrepareRender()
        {
            m_meshDataChecker.soft_unlock();

            if (m_reportShaderOptionFlags)
            {
                m_reportShaderOptionFlags = false;
                PrintShaderOptionFlags();
            }
            for (auto& model : m_modelData)
            {
                model.m_cullable.m_prevFlags = model.m_cullable.m_flags.exchange(0);
            }
        }

        MeshFeatureProcessor::MeshHandle MeshFeatureProcessor::AcquireMesh(
            const MeshHandleDescriptor& descriptor,
            const MaterialAssignmentMap& materials)
        {
            AZ_PROFILE_SCOPE(AzRender, "MeshFeatureProcessor: AcquireMesh");

            // don't need to check the concurrency during emplace() because the StableDynamicArray won't move the other elements during insertion
            MeshHandle meshDataHandle = m_modelData.emplace();

            meshDataHandle->m_descriptor = descriptor;
            meshDataHandle->m_scene = GetParentScene();
            meshDataHandle->m_materialAssignments = materials;
            meshDataHandle->m_objectId = m_transformService->ReserveObjectId();
            meshDataHandle->m_rayTracingUuid = AZ::Uuid::CreateRandom();
            meshDataHandle->m_originalModelAsset = descriptor.m_modelAsset;
            meshDataHandle->m_meshLoader = AZStd::make_unique<ModelDataInstance::MeshLoader>(descriptor.m_modelAsset, &*meshDataHandle);

            return meshDataHandle;
        }

        MeshFeatureProcessor::MeshHandle MeshFeatureProcessor::AcquireMesh(
            const MeshHandleDescriptor& descriptor,
            const Data::Instance<RPI::Material>& material)
        {
            Render::MaterialAssignmentMap materials;
            Render::MaterialAssignment& defaultMaterial = materials[AZ::Render::DefaultMaterialAssignmentId];
            defaultMaterial.m_materialInstance = material;

            return AcquireMesh(descriptor, materials);
        }

        bool MeshFeatureProcessor::ReleaseMesh(MeshHandle& meshHandle)
        {
            if (meshHandle.IsValid())
            {
                meshHandle->m_meshLoader.reset();
                meshHandle->DeInit();
                m_transformService->ReleaseObjectId(meshHandle->m_objectId);

                AZStd::concurrency_check_scope scopeCheck(m_meshDataChecker);
                m_modelData.erase(meshHandle);

                return true;
            }
            return false;
        }

        MeshFeatureProcessor::MeshHandle MeshFeatureProcessor::CloneMesh(const MeshHandle& meshHandle)
        {
            if (meshHandle.IsValid())
            {
                MeshHandle clone = AcquireMesh(meshHandle->m_descriptor, meshHandle->m_materialAssignments);
                return clone;
            }
            return MeshFeatureProcessor::MeshHandle();
        }

        Data::Instance<RPI::Model> MeshFeatureProcessor::GetModel(const MeshHandle& meshHandle) const
        {
            return meshHandle.IsValid() ? meshHandle->m_model : nullptr;
        }

        Data::Asset<RPI::ModelAsset> MeshFeatureProcessor::GetModelAsset(const MeshHandle& meshHandle) const
        {
            if (meshHandle.IsValid())
            {
                return meshHandle->m_originalModelAsset;
            }

            return {};
        }
        
        const RPI::MeshDrawPacketLods& MeshFeatureProcessor::GetDrawPackets(const MeshHandle& meshHandle) const
        {
            return meshHandle.IsValid() ? meshHandle->m_drawPacketListsByLod : m_emptyDrawPacketLods;
        }

        const AZStd::vector<Data::Instance<RPI::ShaderResourceGroup>>& MeshFeatureProcessor::GetObjectSrgs(const MeshHandle& meshHandle) const
        {
            static AZStd::vector<Data::Instance<RPI::ShaderResourceGroup>> staticEmptyList;
            return meshHandle.IsValid() ? meshHandle->m_objectSrgList : staticEmptyList;
        }

        void MeshFeatureProcessor::QueueObjectSrgForCompile(const MeshHandle& meshHandle) const
        {
            if (meshHandle.IsValid())
            {
                meshHandle->m_objectSrgNeedsUpdate = true;
            }
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

                meshHandle->m_objectSrgNeedsUpdate = true;
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

        void MeshFeatureProcessor::SetTransform(const MeshHandle& meshHandle, const AZ::Transform& transform, const AZ::Vector3& nonUniformScale)
        {
            if (meshHandle.IsValid())
            {
                ModelDataInstance& modelData = *meshHandle;
                modelData.m_cullBoundsNeedsUpdate = true;
                modelData.m_objectSrgNeedsUpdate = true;

                m_transformService->SetTransformForId(meshHandle->m_objectId, transform, nonUniformScale);

                // ray tracing data needs to be updated with the new transform
                if (m_rayTracingFeatureProcessor)
                {
                    m_rayTracingFeatureProcessor->SetMeshTransform(meshHandle->m_rayTracingUuid, transform, nonUniformScale);
                }
            }
        }

        void MeshFeatureProcessor::SetLocalAabb(const MeshHandle& meshHandle, const AZ::Aabb& localAabb)
        {
            if (meshHandle.IsValid())
            {
                ModelDataInstance& modelData = *meshHandle;
                modelData.m_aabb = localAabb;
                modelData.m_cullBoundsNeedsUpdate = true;
                modelData.m_objectSrgNeedsUpdate = true;
            }
        };

        AZ::Aabb MeshFeatureProcessor::GetLocalAabb(const MeshHandle& meshHandle) const
        {
            if (meshHandle.IsValid())
            {
                return meshHandle->m_aabb;
            }
            else
            {
                AZ_Assert(false, "Invalid mesh handle");
                return Aabb::CreateNull();
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

        Vector3 MeshFeatureProcessor::GetNonUniformScale(const MeshHandle& meshHandle)
        {
            if (meshHandle.IsValid())
            {
                return m_transformService->GetNonUniformScaleForId(meshHandle->m_objectId);
            }
            else
            {
                AZ_Assert(false, "Invalid mesh handle");
                return Vector3::CreateOne();
            }
        }

        void MeshFeatureProcessor::SetSortKey(const MeshHandle& meshHandle, RHI::DrawItemSortKey sortKey)
        {
            if (meshHandle.IsValid())
            {
                meshHandle->SetSortKey(sortKey);
            }
        }

        RHI::DrawItemSortKey MeshFeatureProcessor::GetSortKey(const MeshHandle& meshHandle) const
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

        void MeshFeatureProcessor::SetMeshLodConfiguration(const MeshHandle& meshHandle, const RPI::Cullable::LodConfiguration& meshLodConfig)
        {
            if (meshHandle.IsValid())
            {
                meshHandle->SetMeshLodConfiguration(meshLodConfig);
            }
        }

        RPI::Cullable::LodConfiguration MeshFeatureProcessor::GetMeshLodConfiguration(const MeshHandle& meshHandle) const
        {
            if (meshHandle.IsValid())
            {
                return meshHandle->GetMeshLodConfiguration();
            }
            else
            {
                AZ_Assert(false, "Invalid mesh handle");
                return {RPI::Cullable::LodType::Default, 0, 0.0f, 0.0f };
            }
        }

        void MeshFeatureProcessor::SetExcludeFromReflectionCubeMaps(const MeshHandle& meshHandle, bool excludeFromReflectionCubeMaps)
        {
            if (meshHandle.IsValid())
            {
                meshHandle->m_excludeFromReflectionCubeMaps = excludeFromReflectionCubeMaps;
                if (excludeFromReflectionCubeMaps)
                {
                    meshHandle->m_cullable.m_cullData.m_hideFlags |= RPI::View::UsageReflectiveCubeMap;
                }
                else
                {
                    meshHandle->m_cullable.m_cullData.m_hideFlags &= ~RPI::View::UsageReflectiveCubeMap;
                }
            }
        }

        void MeshFeatureProcessor::SetRayTracingEnabled(const MeshHandle& meshHandle, bool rayTracingEnabled)
        {
            if (meshHandle.IsValid())
            {
                // update the ray tracing data based on the current state and the new state
                if (rayTracingEnabled && !meshHandle->m_descriptor.m_isRayTracingEnabled)
                {
                    // add to ray tracing
                    meshHandle->SetRayTracingData();
                }
                else if (!rayTracingEnabled && meshHandle->m_descriptor.m_isRayTracingEnabled)
                {
                    // remove from ray tracing
                    if (m_rayTracingFeatureProcessor)
                    {
                        m_rayTracingFeatureProcessor->RemoveMesh(meshHandle->m_rayTracingUuid);
                    }
                }

                // set new state
                meshHandle->m_descriptor.m_isRayTracingEnabled = rayTracingEnabled;
            }
        }

        bool MeshFeatureProcessor::GetRayTracingEnabled(const MeshHandle& meshHandle) const
        {
            if (meshHandle.IsValid())
            {
                return meshHandle->m_descriptor.m_isRayTracingEnabled;
            }
            else
            {
                AZ_Assert(false, "Invalid mesh handle");
                return false;
            }
        }

        bool MeshFeatureProcessor::GetVisible(const MeshHandle& meshHandle) const
        {
            if (meshHandle.IsValid())
            {
                return meshHandle->m_visible;
            }
            return false;
        }

        void MeshFeatureProcessor::SetVisible(const MeshHandle& meshHandle, bool visible)
        {
            if (meshHandle.IsValid())
            {
                meshHandle->SetVisible(visible);

                if (m_rayTracingFeatureProcessor && meshHandle->m_descriptor.m_isRayTracingEnabled)
                {
                    // always remove from ray tracing first
                    m_rayTracingFeatureProcessor->RemoveMesh(meshHandle->m_rayTracingUuid);

                    // now add if it's visible
                    if (visible)
                    {
                        meshHandle->SetRayTracingData();
                    }
                }
            }
        }

        void MeshFeatureProcessor::SetUseForwardPassIblSpecular(const MeshHandle& meshHandle, bool useForwardPassIblSpecular)
        {
            if (meshHandle.IsValid())
            {
                meshHandle->m_descriptor.m_useForwardPassIblSpecular = useForwardPassIblSpecular;
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


        RHI::Ptr<MeshFeatureProcessor::FlagRegistry> MeshFeatureProcessor::GetFlagRegistry()
        {
            if (m_flagRegistry == nullptr)
            {
                m_flagRegistry = FlagRegistry::Create();
            }
            return m_flagRegistry;
        };

        void MeshFeatureProcessor::ForceRebuildDrawPackets([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
        {
            m_forceRebuildDrawPackets = true;
        }

        void MeshFeatureProcessor::OnRenderPipelineChanged([[maybe_unused]] RPI::RenderPipeline* pipeline,
            [[maybe_unused]] RPI::SceneNotification::RenderPipelineChangeType changeType)
        {
            m_forceRebuildDrawPackets = true;
        }

        void MeshFeatureProcessor::UpdateMeshReflectionProbes()
        {
            // we need to rebuild the Srg for any meshes that are using the forward pass IBL specular option
            for (auto& meshInstance : m_modelData)
            {
                if (meshInstance.m_descriptor.m_useForwardPassIblSpecular)
                {
                    meshInstance.m_objectSrgNeedsUpdate = true;
                }
            }
        }

        void MeshFeatureProcessor::ReportShaderOptionFlags([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
        {
            m_reportShaderOptionFlags = true;
        }

        void MeshFeatureProcessor::PrintShaderOptionFlags()
        {
            AZStd::map<FlagRegistry::TagType, AZ::Name> tags;
            AZStd::string registeredFoundMessage = "Registered flags: ";

            auto gatherTags = [&](const Name& name, FlagRegistry::TagType tag)
            {
                tags[tag] = name;
                registeredFoundMessage.append(name.GetCStr() + AZStd::string(", "));
            };

            m_flagRegistry->VisitTags(gatherTags);

            registeredFoundMessage.erase(registeredFoundMessage.end() - 2);

            AZ_Printf("MeshFeatureProcessor", registeredFoundMessage.c_str());

            AZStd::map<uint32_t, uint32_t> flagStats;

            for (auto& model : m_modelData)
            {
                ++flagStats[model.m_cullable.m_flags.load()];
            }

            for (auto [flag, references] : flagStats)
            {
                AZStd::string flagList;

                if (flag == 0)
                {
                    flagList = "(None)";
                }
                else
                {
                    for (auto [tag, name] : tags)
                    {
                        if ((tag.GetIndex() & flag) > 0)
                        {
                            flagList.append(name.GetCStr());
                            flagList.append(", ");
                        }
                    }
                    flagList.erase(flagList.end() - 2);
                }

                AZ_Printf("MeshFeatureProcessor", "Found %u references to [%s]", references, flagList.c_str());
            }
        }

        MeshFeatureProcessorInterface::ModelChangedEvent& ModelDataInstance::MeshLoader::GetModelChangedEvent()
        {
            return m_modelChangedEvent;
        }

        // ModelDataInstance::MeshLoader...

        ModelDataInstance::MeshLoader::MeshLoader(const Data::Asset<RPI::ModelAsset>& modelAsset, ModelDataInstance* parent)
            : m_modelAsset(modelAsset)
            , m_parent(parent)
        {
            if (!m_modelAsset.GetId().IsValid())
            {
                AZ_Error("ModelDataInstance::MeshLoader", false, "Invalid model asset Id.");
                return;
            }
            
            if (!m_modelAsset.IsReady())
            {
                m_modelAsset.QueueLoad();
            }

            Data::AssetBus::Handler::BusConnect(modelAsset.GetId());
            AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        }

        ModelDataInstance::MeshLoader::~MeshLoader()
        {
            AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
            Data::AssetBus::Handler::BusDisconnect();
        }

        // ModelDataInstance...

        //! AssetBus::Handler overrides...
        void ModelDataInstance::MeshLoader::OnAssetReady(Data::Asset<Data::AssetData> asset)
        {
            Data::Asset<RPI::ModelAsset> modelAsset = asset;

            // Assign the fully loaded asset back to the mesh handle to not only hold asset id, but the actual data as well.
            m_parent->m_originalModelAsset = asset;

            Data::Instance<RPI::Model> model;
            // Check if a requires cloning callback got set and if so check if cloning the model asset is requested.
            if (m_parent->m_descriptor.m_requiresCloneCallback &&
                m_parent->m_descriptor.m_requiresCloneCallback(modelAsset))
            {
                // Clone the model asset to force create another model instance.
                AZ::Data::AssetId newId(AZ::Uuid::CreateRandom(), /*subId=*/0);
                Data::Asset<RPI::ModelAsset> clonedAsset;
                if (AZ::RPI::ModelAssetCreator::Clone(modelAsset, clonedAsset, newId))
                {
                    model = RPI::Model::FindOrCreate(clonedAsset);
                }
                else
                {
                    AZ_Error("ModelDataInstance", false, "Cannot clone model for '%s'. Cloth simulation results won't be individual per entity.", modelAsset->GetName().GetCStr());
                    model = RPI::Model::FindOrCreate(modelAsset);
                }
            }
            else
            {
                // Static mesh, no cloth buffer present.
                model = RPI::Model::FindOrCreate(modelAsset);
            }
            
            if (model)
            {
                m_parent->RemoveRayTracingData();
                m_parent->Init(model);
                m_modelChangedEvent.Signal(AZStd::move(model));
            }
            else
            {
                //when running with null renderer, the RPI::Model::FindOrCreate(...) is expected to return nullptr, so suppress this error.
                AZ_Error(
                    "ModelDataInstance::OnAssetReady", RHI::IsNullRHI(), "Failed to create model instance for '%s'",
                    asset.GetHint().c_str());
            }
        }

        
        void ModelDataInstance::MeshLoader::OnModelReloaded(Data::Asset<Data::AssetData> asset)
        {
            OnAssetReady(asset);
        }

        void ModelDataInstance::MeshLoader::OnAssetError(Data::Asset<Data::AssetData> asset)
        {
            // Note: m_modelAsset and asset represents same asset, but only m_modelAsset contains the file path in its hint from serialization
            AZ_Error(
                "ModelDataInstance::MeshLoader", false, "Failed to load asset %s. It may be missing, or not be finished processing",
                m_modelAsset.GetHint().c_str());

            AzFramework::AssetSystemRequestBus::Broadcast(
                &AzFramework::AssetSystem::AssetSystemRequests::EscalateAssetByUuid, m_modelAsset.GetId().m_guid);
        }
        
        void ModelDataInstance::MeshLoader::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
        {
            if (assetId == m_modelAsset.GetId())
            {
                Data::Asset<RPI::ModelAsset> modelAssetReference = m_modelAsset;

                // If the asset was modified, reload it
                AZ::SystemTickBus::QueueFunction(
                    [=]() mutable
                    {
                        ModelReloaderSystemInterface::Get()->ReloadModel(modelAssetReference, m_modelReloadedEventHandler);
                    });
            }
        }

        void ModelDataInstance::MeshLoader::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
        {
            if (assetId == m_modelAsset.GetId())
            {
                Data::Asset<RPI::ModelAsset> modelAssetReference = m_modelAsset;
                
                // If the asset didn't exist in the catalog when it first attempted to load, we need to try loading it again
                AZ::SystemTickBus::QueueFunction(
                    [=]() mutable
                    {
                        ModelReloaderSystemInterface::Get()->ReloadModel(modelAssetReference, m_modelReloadedEventHandler);
                    });
            }
        }

        void ModelDataInstance::DeInit()
        {
            m_scene->GetCullingScene()->UnregisterCullable(m_cullable);

            for (const auto& materialAssignment : m_materialAssignments)
            {
                const AZ::Data::Instance<RPI::Material>& materialInstance = materialAssignment.second.m_materialInstance;
                if (materialInstance.get())
                {
                    MaterialAssignmentNotificationBus::MultiHandler::BusDisconnect(materialInstance->GetAssetId());
                }
            }

            RemoveRayTracingData();

            m_drawPacketListsByLod.clear();
            m_materialAssignments.clear();
            m_objectSrgList = {};
            m_model = {};
        }

        void ModelDataInstance::Init(Data::Instance<RPI::Model> model)
        {
            m_model = model;
            const size_t modelLodCount = m_model->GetLodCount();
            m_drawPacketListsByLod.resize(modelLodCount);
            for (size_t modelLodIndex = 0; modelLodIndex < modelLodCount; ++modelLodIndex)
            {
                BuildDrawPacketList(modelLodIndex);
            }

            for(auto& objectSrg : m_objectSrgList)
            {
                // Set object Id once since it never changes
                RHI::ShaderInputNameIndex objectIdIndex = "m_objectId";
                objectSrg->SetConstant(objectIdIndex, m_objectId.GetIndex());
                objectIdIndex.AssertValid();
            }

            for (const auto& materialAssignment : m_materialAssignments)
            {
                const AZ::Data::Instance<RPI::Material>& materialInstance = materialAssignment.second.m_materialInstance;
                if (materialInstance.get())
                {
                    MaterialAssignmentNotificationBus::MultiHandler::BusConnect(materialInstance->GetAssetId());
                }
            }

            if (m_visible && m_descriptor.m_isRayTracingEnabled)
            {
                SetRayTracingData();
            }

            m_aabb = model->GetModelAsset()->GetAabb();

            m_cullableNeedsRebuild = true;
            m_cullBoundsNeedsUpdate = true;
            m_objectSrgNeedsUpdate = true;
        }

        void ModelDataInstance::BuildDrawPacketList(size_t modelLodIndex)
        {
            RPI::ModelLod& modelLod = *m_model->GetLods()[modelLodIndex];
            const size_t meshCount = modelLod.GetMeshes().size();
            
            RPI::MeshDrawPacketList& drawPacketListOut = m_drawPacketListsByLod[modelLodIndex];
            drawPacketListOut.clear();
            drawPacketListOut.reserve(meshCount);

            m_hasForwardPassIblSpecularMaterial = false;

            for (size_t meshIndex = 0; meshIndex < meshCount; ++meshIndex)
            {
                const RPI::ModelLod::Mesh& mesh = modelLod.GetMeshes()[meshIndex];

                Data::Instance<RPI::Material> material = mesh.m_material;

                // Determine if there is a material override specified for this sub mesh
                const MaterialAssignmentId materialAssignmentId(modelLodIndex, mesh.m_materialSlotStableId);
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

                auto& objectSrgLayout = material->GetAsset()->GetObjectSrgLayout();

                if (!objectSrgLayout)
                {
                    AZ_Warning("MeshFeatureProcessor", false, "No per-object ShaderResourceGroup found.");
                    continue;
                }

                Data::Instance<RPI::ShaderResourceGroup> meshObjectSrg;

                // See if the object SRG for this mesh is already in our list of object SRGs
                for (auto& objectSrgIter : m_objectSrgList)
                {
                    if (objectSrgIter->GetLayout()->GetHash() == objectSrgLayout->GetHash())
                    {
                        meshObjectSrg = objectSrgIter;
                    }
                }

                // If the object SRG for this mesh was not already in the list, create it and add it to the list
                if (!meshObjectSrg)
                {
                    auto& shaderAsset = material->GetAsset()->GetMaterialTypeAsset()->GetShaderAssetForObjectSrg();
                    meshObjectSrg = RPI::ShaderResourceGroup::Create(shaderAsset, objectSrgLayout->GetName());
                    if (!meshObjectSrg)
                    {
                        AZ_Warning("MeshFeatureProcessor", false, "Failed to create a new shader resource group, skipping.");
                        continue;
                    }
                    m_objectSrgList.push_back(meshObjectSrg);
                }

                // setup the mesh draw packet
                RPI::MeshDrawPacket drawPacket(modelLod, meshIndex, material, meshObjectSrg, materialAssignment.m_matModUvOverrides);

                // set the shader option to select forward pass IBL specular if necessary
                if (!drawPacket.SetShaderOption(AZ::Name("o_meshUseForwardPassIBLSpecular"), AZ::RPI::ShaderOptionValue{ m_descriptor.m_useForwardPassIblSpecular }))
                {
                    AZ_Warning("MeshDrawPacket", false, "Failed to set o_meshUseForwardPassIBLSpecular on mesh draw packet");
                }

                bool materialRequiresForwardPassIblSpecular = MaterialRequiresForwardPassIblSpecular(material);

                // track whether any materials in this mesh require ForwardPassIblSpecular, we need this information when the ObjectSrg is updated
                m_hasForwardPassIblSpecularMaterial |= materialRequiresForwardPassIblSpecular;

                // stencil bits
                uint8_t stencilRef = m_descriptor.m_useForwardPassIblSpecular || materialRequiresForwardPassIblSpecular ? Render::StencilRefs::None : Render::StencilRefs::UseIBLSpecularPass;
                stencilRef |= Render::StencilRefs::UseDiffuseGIPass;

                drawPacket.SetStencilRef(stencilRef);
                drawPacket.SetSortKey(m_sortKey);
                drawPacket.Update(*m_scene, false);
                drawPacketListOut.emplace_back(AZStd::move(drawPacket));
            }
        }

        void ModelDataInstance::SetRayTracingData()
        {
            RemoveRayTracingData();

            if (!m_model)
            {
                return;
            }

            RayTracingFeatureProcessor* rayTracingFeatureProcessor = m_scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            if (!rayTracingFeatureProcessor)
            {
                return;
            }

            const AZStd::span<const Data::Instance<RPI::ModelLod>>& modelLods = m_model->GetLods();
            if (modelLods.empty())
            {
                return;
            }

            // use the lowest LOD for raytracing
            uint32_t rayTracingLod = aznumeric_cast<uint32_t>(modelLods.size() - 1);
            const Data::Instance<RPI::ModelLod>& modelLod = modelLods[rayTracingLod];

            // setup a stream layout and shader input contract for the vertex streams
            static const char* PositionSemantic = "POSITION";
            static const char* NormalSemantic = "NORMAL";
            static const char* TangentSemantic = "TANGENT";
            static const char* BitangentSemantic = "BITANGENT";
            static const char* UVSemantic = "UV";
            static const RHI::Format PositionStreamFormat = RHI::Format::R32G32B32_FLOAT;
            static const RHI::Format NormalStreamFormat = RHI::Format::R32G32B32_FLOAT;
            static const RHI::Format TangentStreamFormat = RHI::Format::R32G32B32A32_FLOAT;
            static const RHI::Format BitangentStreamFormat = RHI::Format::R32G32B32_FLOAT;
            static const RHI::Format UVStreamFormat = RHI::Format::R32G32_FLOAT;

            RHI::InputStreamLayoutBuilder layoutBuilder;
            layoutBuilder.AddBuffer()->Channel(PositionSemantic, PositionStreamFormat);
            layoutBuilder.AddBuffer()->Channel(NormalSemantic, NormalStreamFormat);
            layoutBuilder.AddBuffer()->Channel(UVSemantic, UVStreamFormat);
            layoutBuilder.AddBuffer()->Channel(TangentSemantic, TangentStreamFormat);
            layoutBuilder.AddBuffer()->Channel(BitangentSemantic, BitangentStreamFormat);
            RHI::InputStreamLayout inputStreamLayout = layoutBuilder.End();

            RPI::ShaderInputContract::StreamChannelInfo positionStreamChannelInfo;
            positionStreamChannelInfo.m_semantic = RHI::ShaderSemantic(AZ::Name(PositionSemantic));
            positionStreamChannelInfo.m_componentCount = RHI::GetFormatComponentCount(PositionStreamFormat);

            RPI::ShaderInputContract::StreamChannelInfo normalStreamChannelInfo;
            normalStreamChannelInfo.m_semantic = RHI::ShaderSemantic(AZ::Name(NormalSemantic));
            normalStreamChannelInfo.m_componentCount = RHI::GetFormatComponentCount(NormalStreamFormat);

            RPI::ShaderInputContract::StreamChannelInfo tangentStreamChannelInfo;
            tangentStreamChannelInfo.m_semantic = RHI::ShaderSemantic(AZ::Name(TangentSemantic));
            tangentStreamChannelInfo.m_componentCount = RHI::GetFormatComponentCount(TangentStreamFormat);
            tangentStreamChannelInfo.m_isOptional = true;

            RPI::ShaderInputContract::StreamChannelInfo bitangentStreamChannelInfo;
            bitangentStreamChannelInfo.m_semantic = RHI::ShaderSemantic(AZ::Name(BitangentSemantic));
            bitangentStreamChannelInfo.m_componentCount = RHI::GetFormatComponentCount(BitangentStreamFormat);
            bitangentStreamChannelInfo.m_isOptional = true;

            RPI::ShaderInputContract::StreamChannelInfo uvStreamChannelInfo;
            uvStreamChannelInfo.m_semantic = RHI::ShaderSemantic(AZ::Name(UVSemantic));
            uvStreamChannelInfo.m_componentCount = RHI::GetFormatComponentCount(UVStreamFormat);
            uvStreamChannelInfo.m_isOptional = true;

            RPI::ShaderInputContract shaderInputContract;
            shaderInputContract.m_streamChannels.emplace_back(positionStreamChannelInfo);
            shaderInputContract.m_streamChannels.emplace_back(normalStreamChannelInfo);
            shaderInputContract.m_streamChannels.emplace_back(tangentStreamChannelInfo);
            shaderInputContract.m_streamChannels.emplace_back(bitangentStreamChannelInfo);
            shaderInputContract.m_streamChannels.emplace_back(uvStreamChannelInfo);

            // setup the raytracing data for each sub-mesh 
            const size_t meshCount = modelLod->GetMeshes().size();
            RayTracingFeatureProcessor::SubMeshVector subMeshes;
            for (uint32_t meshIndex = 0; meshIndex < meshCount; ++meshIndex)
            {
                const RPI::ModelLod::Mesh& mesh = modelLod->GetMeshes()[meshIndex];

                // retrieve the material
                Data::Instance<RPI::Material> material = mesh.m_material;

                const MaterialAssignmentId materialAssignmentId(rayTracingLod, mesh.m_materialSlotStableId);
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

                // retrieve vertex/index buffers
                RPI::ModelLod::StreamBufferViewList streamBufferViews;
                [[maybe_unused]] bool result = modelLod->GetStreamsForMesh(
                    inputStreamLayout,
                    streamBufferViews,
                    nullptr,
                    shaderInputContract,
                    meshIndex,
                    materialAssignment.m_matModUvOverrides,
                    material->GetAsset()->GetMaterialTypeAsset()->GetUvNameMap());
                AZ_Assert(result, "Failed to retrieve mesh stream buffer views");

                // note that the element count is the size of the entire buffer, even though this mesh may only
                // occupy a portion of the vertex buffer.  This is necessary since we are accessing it using
                // a ByteAddressBuffer in the raytracing shaders and passing the byte offset to the shader in a constant buffer.
                uint32_t positionBufferByteCount = static_cast<uint32_t>(const_cast<RHI::Buffer*>(streamBufferViews[0].GetBuffer())->GetDescriptor().m_byteCount);
                RHI::BufferViewDescriptor positionBufferDescriptor = RHI::BufferViewDescriptor::CreateRaw(0, positionBufferByteCount);

                uint32_t normalBufferByteCount = static_cast<uint32_t>(const_cast<RHI::Buffer*>(streamBufferViews[1].GetBuffer())->GetDescriptor().m_byteCount);
                RHI::BufferViewDescriptor normalBufferDescriptor = RHI::BufferViewDescriptor::CreateRaw(0, normalBufferByteCount);

                uint32_t tangentBufferByteCount = static_cast<uint32_t>(const_cast<RHI::Buffer*>(streamBufferViews[2].GetBuffer())->GetDescriptor().m_byteCount);
                RHI::BufferViewDescriptor tangentBufferDescriptor = RHI::BufferViewDescriptor::CreateRaw(0, tangentBufferByteCount);

                uint32_t bitangentBufferByteCount = static_cast<uint32_t>(const_cast<RHI::Buffer*>(streamBufferViews[3].GetBuffer())->GetDescriptor().m_byteCount);
                RHI::BufferViewDescriptor bitangentBufferDescriptor = RHI::BufferViewDescriptor::CreateRaw(0, bitangentBufferByteCount);

                uint32_t uvBufferByteCount = static_cast<uint32_t>(const_cast<RHI::Buffer*>(streamBufferViews[4].GetBuffer())->GetDescriptor().m_byteCount);
                RHI::BufferViewDescriptor uvBufferDescriptor = RHI::BufferViewDescriptor::CreateRaw(0, uvBufferByteCount);

                const RHI::IndexBufferView& indexBufferView = mesh.m_indexBufferView;
                uint32_t indexElementSize = indexBufferView.GetIndexFormat() == RHI::IndexFormat::Uint16 ? 2 : 4;
                uint32_t indexElementCount = (uint32_t)indexBufferView.GetBuffer()->GetDescriptor().m_byteCount / indexElementSize;
                RHI::BufferViewDescriptor indexBufferDescriptor;
                indexBufferDescriptor.m_elementOffset = 0;
                indexBufferDescriptor.m_elementCount = indexElementCount;
                indexBufferDescriptor.m_elementSize = indexElementSize;
                indexBufferDescriptor.m_elementFormat = indexBufferView.GetIndexFormat() == RHI::IndexFormat::Uint16 ? RHI::Format::R16_UINT : RHI::Format::R32_UINT;

                // set the SubMesh data to pass to the RayTracingFeatureProcessor, starting with vertex/index data
                RayTracingFeatureProcessor::SubMesh subMesh;
                subMesh.m_positionFormat = PositionStreamFormat;
                subMesh.m_positionVertexBufferView = streamBufferViews[0];
                subMesh.m_positionShaderBufferView = const_cast<RHI::Buffer*>(streamBufferViews[0].GetBuffer())->GetBufferView(positionBufferDescriptor);

                subMesh.m_normalFormat = NormalStreamFormat;
                subMesh.m_normalVertexBufferView = streamBufferViews[1];
                subMesh.m_normalShaderBufferView = const_cast<RHI::Buffer*>(streamBufferViews[1].GetBuffer())->GetBufferView(normalBufferDescriptor);

                if (tangentBufferByteCount > 0)
                {
                    subMesh.m_bufferFlags |= RayTracingSubMeshBufferFlags::Tangent;
                    subMesh.m_tangentFormat = TangentStreamFormat;
                    subMesh.m_tangentVertexBufferView = streamBufferViews[2];
                    subMesh.m_tangentShaderBufferView = const_cast<RHI::Buffer*>(streamBufferViews[2].GetBuffer())->GetBufferView(tangentBufferDescriptor);
                }

                if (bitangentBufferByteCount > 0)
                {
                    subMesh.m_bufferFlags |= RayTracingSubMeshBufferFlags::Bitangent;
                    subMesh.m_bitangentFormat = BitangentStreamFormat;
                    subMesh.m_bitangentVertexBufferView = streamBufferViews[3];
                    subMesh.m_bitangentShaderBufferView = const_cast<RHI::Buffer*>(streamBufferViews[3].GetBuffer())->GetBufferView(bitangentBufferDescriptor);
                }

                if (uvBufferByteCount > 0)
                {
                    subMesh.m_bufferFlags |= RayTracingSubMeshBufferFlags::UV;
                    subMesh.m_uvFormat = UVStreamFormat;
                    subMesh.m_uvVertexBufferView = streamBufferViews[4];
                    subMesh.m_uvShaderBufferView = const_cast<RHI::Buffer*>(streamBufferViews[4].GetBuffer())->GetBufferView(uvBufferDescriptor);
                }

                subMesh.m_indexBufferView = mesh.m_indexBufferView;
                subMesh.m_indexShaderBufferView = const_cast<RHI::Buffer*>(mesh.m_indexBufferView.GetBuffer())->GetBufferView(indexBufferDescriptor);

                // add material data
                if (material)
                {
                    RPI::MaterialPropertyIndex propertyIndex;

                    // base color
                    propertyIndex = material->FindPropertyIndex(AZ::Name("baseColor.color"));
                    if (propertyIndex.IsValid())
                    {
                        subMesh.m_baseColor = material->GetPropertyValue<AZ::Color>(propertyIndex);
                    }

                    propertyIndex = material->FindPropertyIndex(AZ::Name("baseColor.factor"));
                    if (propertyIndex.IsValid())
                    {
                        subMesh.m_baseColor *= material->GetPropertyValue<float>(propertyIndex);
                    }

                    // metallic
                    propertyIndex = material->FindPropertyIndex(AZ::Name("metallic.factor"));
                    if (propertyIndex.IsValid())
                    {
                        subMesh.m_metallicFactor = material->GetPropertyValue<float>(propertyIndex);
                    }

                    // roughness
                    propertyIndex = material->FindPropertyIndex(AZ::Name("roughness.factor"));
                    if (propertyIndex.IsValid())
                    {
                        subMesh.m_roughnessFactor = material->GetPropertyValue<float>(propertyIndex);
                    }

                    // emissive color
                    propertyIndex = material->FindPropertyIndex(AZ::Name("emissive.enable"));
                    if (propertyIndex.IsValid())
                    {
                        if (material->GetPropertyValue<bool>(propertyIndex))
                        {
                            propertyIndex = material->FindPropertyIndex(AZ::Name("emissive.color"));
                            if (propertyIndex.IsValid())
                            {
                                subMesh.m_emissiveColor = material->GetPropertyValue<AZ::Color>(propertyIndex);
                            }

                            propertyIndex = material->FindPropertyIndex(AZ::Name("emissive.intensity"));
                            if (propertyIndex.IsValid())
                            {
                                subMesh.m_emissiveColor *= material->GetPropertyValue<float>(propertyIndex);
                            }
                        }
                    }

                    // textures
                    Data::Instance<RPI::Image> baseColorImage; // can be used for irradiance color below
                    propertyIndex = material->FindPropertyIndex(AZ::Name("baseColor.textureMap"));
                    if (propertyIndex.IsValid())
                    {
                        Data::Instance<RPI::Image> image = material->GetPropertyValue<Data::Instance<RPI::Image>>(propertyIndex);
                        if (image.get())
                        {
                            subMesh.m_textureFlags |= RayTracingSubMeshTextureFlags::BaseColor;
                            subMesh.m_baseColorImageView = image->GetImageView();
                            baseColorImage = image;
                        }
                    }

                    propertyIndex = material->FindPropertyIndex(AZ::Name("normal.textureMap"));
                    if (propertyIndex.IsValid())
                    {
                        Data::Instance<RPI::Image> image = material->GetPropertyValue<Data::Instance<RPI::Image>>(propertyIndex);
                        if (image.get())
                        {
                            subMesh.m_textureFlags |= RayTracingSubMeshTextureFlags::Normal;
                            subMesh.m_normalImageView = image->GetImageView();
                        }
                    }

                    propertyIndex = material->FindPropertyIndex(AZ::Name("metallic.textureMap"));
                    if (propertyIndex.IsValid())
                    {
                        Data::Instance<RPI::Image> image = material->GetPropertyValue<Data::Instance<RPI::Image>>(propertyIndex);
                        if (image.get())
                        {
                            subMesh.m_textureFlags |= RayTracingSubMeshTextureFlags::Metallic;
                            subMesh.m_metallicImageView = image->GetImageView();
                        }
                    }

                    propertyIndex = material->FindPropertyIndex(AZ::Name("roughness.textureMap"));
                    if (propertyIndex.IsValid())
                    {
                        Data::Instance<RPI::Image> image = material->GetPropertyValue<Data::Instance<RPI::Image>>(propertyIndex);
                        if (image.get())
                        {
                            subMesh.m_textureFlags |= RayTracingSubMeshTextureFlags::Roughness;
                            subMesh.m_roughnessImageView = image->GetImageView();
                        }
                    }

                    propertyIndex = material->FindPropertyIndex(AZ::Name("emissive.textureMap"));
                    if (propertyIndex.IsValid())
                    {
                        Data::Instance<RPI::Image> image = material->GetPropertyValue<Data::Instance<RPI::Image>>(propertyIndex);
                        if (image.get())
                        {
                            subMesh.m_textureFlags |= RayTracingSubMeshTextureFlags::Emissive;
                            subMesh.m_emissiveImageView = image->GetImageView();
                        }
                    }

                    // irradiance color
                    SetIrradianceData(subMesh, material, baseColorImage);
                }

                subMeshes.push_back(subMesh);
            }

            TransformServiceFeatureProcessor* transformServiceFeatureProcessor = m_scene->GetFeatureProcessor<TransformServiceFeatureProcessor>();
            AZ::Transform transform = transformServiceFeatureProcessor->GetTransformForId(m_objectId);
            AZ::Vector3 nonUniformScale = transformServiceFeatureProcessor->GetNonUniformScaleForId(m_objectId);

            rayTracingFeatureProcessor->AddMesh(m_rayTracingUuid, m_model->GetModelAsset()->GetId(), subMeshes, transform, nonUniformScale);
        }

        void ModelDataInstance::SetIrradianceData(
            RayTracingFeatureProcessor::SubMesh& subMesh,
            const Data::Instance<RPI::Material> material,
            const Data::Instance<RPI::Image> baseColorImage)
        {
            RPI::MaterialPropertyIndex propertyIndex = material->FindPropertyIndex(AZ::Name("irradiance.irradianceColorSource"));
            if (!propertyIndex.IsValid())
            {
                return;
            }

            uint32_t enumVal = material->GetPropertyValue<uint32_t>(propertyIndex);
            AZ::Name irradianceColorSource = material->GetMaterialPropertiesLayout()->GetPropertyDescriptor(propertyIndex)->GetEnumName(enumVal);

            if (irradianceColorSource.IsEmpty() || irradianceColorSource == AZ::Name("Manual"))
            {
                propertyIndex = material->FindPropertyIndex(AZ::Name("irradiance.manualColor"));
                if (propertyIndex.IsValid())
                {
                    subMesh.m_irradianceColor = material->GetPropertyValue<AZ::Color>(propertyIndex);
                }
                else
                {
                    // Couldn't find irradiance.manualColor -> check for an irradiance.color in case the material type
                    // doesn't have the concept of manual vs. automatic irradiance color, allow a simpler property name
                    propertyIndex = material->FindPropertyIndex(AZ::Name("irradiance.color"));
                    if (propertyIndex.IsValid())
                    {
                        subMesh.m_irradianceColor = material->GetPropertyValue<AZ::Color>(propertyIndex);
                    }
                    else
                    {
                        AZ_Warning(
                            "MeshFeatureProcessor", false,
                            "No irradiance.manualColor or irradiance.color field found. Defaulting to 1.0f.");
                        subMesh.m_irradianceColor = AZ::Colors::White;
                    }
                }
            }
            else if (irradianceColorSource == AZ::Name("BaseColorTint"))
            {
                // Use only the baseColor, no texture on top of it
                subMesh.m_irradianceColor = subMesh.m_baseColor;
            }
            else if (irradianceColorSource == AZ::Name("BaseColor"))
            {
                // Check if texturing is enabled
                bool useTexture;
                propertyIndex = material->FindPropertyIndex(AZ::Name("baseColor.useTexture"));
                if (propertyIndex.IsValid())
                {
                    useTexture = material->GetPropertyValue<bool>(propertyIndex);
                }
                else
                {
                    // No explicit baseColor.useTexture switch found, assuming the user wants to use
                    // a texture if a texture was found.
                    useTexture = true;
                }

                // If texturing was requested: check if we found a texture and use it
                if (useTexture && baseColorImage.get())
                {
                    // Currently GetAverageColor() is only implemented for a StreamingImage
                    auto baseColorStreamingImg = azdynamic_cast<RPI::StreamingImage*>(baseColorImage.get());
                    if (baseColorStreamingImg)
                    {
                        // Note: there are quite a few hidden assumptions in using the average
                        // texture color. For instance, (1) it assumes that every texel in the
                        // texture actually gets mapped to the surface (or non-mapped regions are
                        // colored with a meaningful 'average' color, or have zero opacity); (2) it
                        // assumes that the mapping from uv space to the mesh surface is
                        // (approximately) area-preserving to get a properly weighted average; and
                        // mostly, (3) it assumes that a single 'average color' is a meaningful
                        // characterisation of the full material.
                        Color avgColor = baseColorStreamingImg->GetAverageColor();

                        // We do a simple 'multiply' blend with the base color for now. Warn
                        // the user if something else was intended.
                        propertyIndex = material->FindPropertyIndex(AZ::Name("baseColor.textureBlendMode"));
                        if (propertyIndex.IsValid())
                        {
                            AZ::Name textureBlendMode = material->GetMaterialPropertiesLayout()
                                     ->GetPropertyDescriptor(propertyIndex)
                                     ->GetEnumName(material->GetPropertyValue<uint32_t>(propertyIndex));
                            if (textureBlendMode != AZ::Name("Multiply"))
                            {
                                AZ_Warning("MeshFeatureProcessor", false, "textureBlendMode '%s' is not "
                                        "yet supported when requesting BaseColor irradiance source, "
                                        "using 'Multiply' for deriving the irradiance color.",
                                        textureBlendMode.GetCStr());
                            }
                        }
                        // 'Multiply' blend mode:
                        subMesh.m_irradianceColor = avgColor * subMesh.m_baseColor;
                    }
                    else
                    {
                        AZ_Warning("MeshFeatureProcessor", false, "Using BaseColor as irradianceColorSource "
                                "is currently only supported for textures of type StreamingImage");
                        // Default to the flat base color
                        subMesh.m_irradianceColor = subMesh.m_baseColor;
                    }
                }
                else
                {
                    // No texture, simply copy the baseColor
                    subMesh.m_irradianceColor = subMesh.m_baseColor;
                }
            }
            else
            {
                AZ_Warning("MeshFeatureProcessor", false, "Unknown irradianceColorSource value: %s, "
                        "defaulting to 1.0f.", irradianceColorSource.GetCStr());
                subMesh.m_irradianceColor = AZ::Colors::White;
            }


            // Overall scale factor
            propertyIndex = material->FindPropertyIndex(AZ::Name("irradiance.factor"));
            if (propertyIndex.IsValid())
            {
                subMesh.m_irradianceColor *= material->GetPropertyValue<float>(propertyIndex);
            }

            // set the raytracing transparency from the material opacity factor
            float opacity = 1.0f;
            propertyIndex = material->FindPropertyIndex(AZ::Name("opacity.mode"));
            if (propertyIndex.IsValid())
            {
                // only query the opacity factor if it's a non-Opaque mode
                uint32_t mode = material->GetPropertyValue<uint32_t>(propertyIndex);
                if (mode > 0)
                {
                    propertyIndex = material->FindPropertyIndex(AZ::Name("opacity.factor"));
                    if (propertyIndex.IsValid())
                    {
                        opacity = material->GetPropertyValue<float>(propertyIndex);
                    }
                }
            }

            subMesh.m_irradianceColor.SetA(opacity);
        }

        void ModelDataInstance::RemoveRayTracingData()
        {
            // remove from ray tracing
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = m_scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            if (rayTracingFeatureProcessor)
            {
                rayTracingFeatureProcessor->RemoveMesh(m_rayTracingUuid);
            }
        }

        void ModelDataInstance::SetSortKey(RHI::DrawItemSortKey sortKey)
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

        RHI::DrawItemSortKey ModelDataInstance::GetSortKey() const
        {
            return m_sortKey;
        }

        void ModelDataInstance::SetMeshLodConfiguration(RPI::Cullable::LodConfiguration meshLodConfig)
        {
            m_cullable.m_lodData.m_lodConfiguration = meshLodConfig;
        }

        RPI::Cullable::LodConfiguration ModelDataInstance::GetMeshLodConfiguration() const
        {
            return m_cullable.m_lodData.m_lodConfiguration;
        }

        void ModelDataInstance::UpdateDrawPackets(bool forceUpdate /*= false*/)
        {
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

        void ModelDataInstance::BuildCullable()
        {
            AZ_Assert(m_cullableNeedsRebuild, "This function only needs to be called if the cullable to be rebuilt");
            AZ_Assert(m_model, "The model has not finished loading yet");

            RPI::Cullable::CullData& cullData = m_cullable.m_cullData;
            RPI::Cullable::LodData& lodData = m_cullable.m_lodData;

            const Aabb& localAabb = m_aabb;
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
                if (lodIndex == 0)
                {
                    //first lod
                    lod.m_screenCoverageMax = 1.0f;
                }
                else
                {
                    //every other lod: use the previous lod's min
                    lod.m_screenCoverageMax = AZStd::GetMax(lodData.m_lods[lodIndex - 1].m_screenCoverageMin, lodData.m_lodConfiguration.m_minimumScreenCoverage);
                }

                if (lodIndex < lodAssets.size() - 1)
                {
                    //first and middle lods: compute a stepdown value for the min
                    lod.m_screenCoverageMin = AZStd::GetMax(lodData.m_lodConfiguration.m_qualityDecayRate * lod.m_screenCoverageMax, lodData.m_lodConfiguration.m_minimumScreenCoverage);
                }
                else
                {
                    //last lod: use MinimumScreenCoverage for the min
                    lod.m_screenCoverageMin = lodData.m_lodConfiguration.m_minimumScreenCoverage;
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

#ifdef AZ_CULL_DEBUG_ENABLED
            m_cullable.SetDebugName(AZ::Name(AZStd::string::format("%s - objectId: %u", m_model->GetModelAsset()->GetName().GetCStr(), m_objectId.GetIndex())));
#endif

            m_cullableNeedsRebuild = false;
            m_cullBoundsNeedsUpdate = true;
        }

        void ModelDataInstance::UpdateCullBounds(const TransformServiceFeatureProcessor* transformService)
        {
            AZ_Assert(m_cullBoundsNeedsUpdate, "This function only needs to be called if the culling bounds need to be rebuilt");
            AZ_Assert(m_model, "The model has not finished loading yet");

            Transform localToWorld = transformService->GetTransformForId(m_objectId);
            Vector3 nonUniformScale = transformService->GetNonUniformScaleForId(m_objectId);

            Vector3 center;
            float radius;
            Aabb localAabb = m_aabb;
            localAabb.MultiplyByScale(nonUniformScale);

            localAabb.GetTransformedAabb(localToWorld).GetAsSphere(center, radius);

            m_cullable.m_lodData.m_lodSelectionRadius = 0.5f*localAabb.GetExtents().GetMaxElement();

            m_cullable.m_cullData.m_boundingSphere = Sphere(center, radius);
            m_cullable.m_cullData.m_boundingObb = localAabb.GetTransformedObb(localToWorld);
            m_cullable.m_cullData.m_visibilityEntry.m_boundingVolume = localAabb.GetTransformedAabb(localToWorld);
            m_cullable.m_cullData.m_visibilityEntry.m_userData = &m_cullable;
            m_cullable.m_cullData.m_visibilityEntry.m_typeFlags = AzFramework::VisibilityEntry::TYPE_RPI_Cullable;
            m_scene->GetCullingScene()->RegisterOrUpdateCullable(m_cullable);

            m_cullBoundsNeedsUpdate = false;
        }

        void ModelDataInstance::UpdateObjectSrg()
        {
            for (auto& objectSrg : m_objectSrgList)
            {
                ReflectionProbeFeatureProcessor* reflectionProbeFeatureProcessor = m_scene->GetFeatureProcessor<ReflectionProbeFeatureProcessor>();

                if (reflectionProbeFeatureProcessor && (m_descriptor.m_useForwardPassIblSpecular || m_hasForwardPassIblSpecularMaterial))
                {
                    // retrieve probe constant indices
                    AZ::RHI::ShaderInputConstantIndex modelToWorldConstantIndex = objectSrg->FindShaderInputConstantIndex(Name("m_reflectionProbeData.m_modelToWorld"));
                    AZ_Error("ModelDataInstance", modelToWorldConstantIndex.IsValid(), "Failed to find ReflectionProbe constant index");

                    AZ::RHI::ShaderInputConstantIndex modelToWorldInverseConstantIndex = objectSrg->FindShaderInputConstantIndex(Name("m_reflectionProbeData.m_modelToWorldInverse"));
                    AZ_Error("ModelDataInstance", modelToWorldInverseConstantIndex.IsValid(), "Failed to find ReflectionProbe constant index");

                    AZ::RHI::ShaderInputConstantIndex outerObbHalfLengthsConstantIndex = objectSrg->FindShaderInputConstantIndex(Name("m_reflectionProbeData.m_outerObbHalfLengths"));
                    AZ_Error("ModelDataInstance", outerObbHalfLengthsConstantIndex.IsValid(), "Failed to find ReflectionProbe constant index");

                    AZ::RHI::ShaderInputConstantIndex innerObbHalfLengthsConstantIndex = objectSrg->FindShaderInputConstantIndex(Name("m_reflectionProbeData.m_innerObbHalfLengths"));
                    AZ_Error("ModelDataInstance", innerObbHalfLengthsConstantIndex.IsValid(), "Failed to find ReflectionProbe constant index");

                    AZ::RHI::ShaderInputConstantIndex useReflectionProbeConstantIndex = objectSrg->FindShaderInputConstantIndex(Name("m_reflectionProbeData.m_useReflectionProbe"));
                    AZ_Error("ModelDataInstance", useReflectionProbeConstantIndex.IsValid(), "Failed to find ReflectionProbe constant index");

                    AZ::RHI::ShaderInputConstantIndex useParallaxCorrectionConstantIndex = objectSrg->FindShaderInputConstantIndex(Name("m_reflectionProbeData.m_useParallaxCorrection"));
                    AZ_Error("ModelDataInstance", useParallaxCorrectionConstantIndex.IsValid(), "Failed to find ReflectionProbe constant index");

                    AZ::RHI::ShaderInputConstantIndex exposureConstantIndex = objectSrg->FindShaderInputConstantIndex(Name("m_reflectionProbeData.m_exposure"));
                    AZ_Error("ModelDataInstance", exposureConstantIndex.IsValid(), "Failed to find ReflectionProbe constant index");

                    // retrieve probe cubemap index
                    Name reflectionCubeMapImageName = Name("m_reflectionProbeCubeMap");
                    RHI::ShaderInputImageIndex reflectionCubeMapImageIndex = objectSrg->FindShaderInputImageIndex(reflectionCubeMapImageName);
                    AZ_Error("ModelDataInstance", reflectionCubeMapImageIndex.IsValid(), "Failed to find shader image index [%s]", reflectionCubeMapImageName.GetCStr());

                    // retrieve the list of probes that overlap the mesh bounds
                    TransformServiceFeatureProcessor* transformServiceFeatureProcessor = m_scene->GetFeatureProcessor<TransformServiceFeatureProcessor>();
                    Transform transform = transformServiceFeatureProcessor->GetTransformForId(m_objectId);

                    Aabb aabbWS = m_aabb;
                    aabbWS.ApplyTransform(transform);

                    ReflectionProbeHandleVector reflectionProbeHandles;
                    reflectionProbeFeatureProcessor->FindReflectionProbes(aabbWS, reflectionProbeHandles);

                    if (!reflectionProbeHandles.empty())
                    {
                        // take the last handle from the list, which will be the smallest (most influential) probe
                        ReflectionProbeHandle handle = reflectionProbeHandles.back();

                        objectSrg->SetConstant(modelToWorldConstantIndex, Matrix3x4::CreateFromTransform(reflectionProbeFeatureProcessor->GetTransform(handle)));
                        objectSrg->SetConstant(modelToWorldInverseConstantIndex, Matrix3x4::CreateFromTransform(reflectionProbeFeatureProcessor->GetTransform(handle)).GetInverseFull());
                        objectSrg->SetConstant(outerObbHalfLengthsConstantIndex, reflectionProbeFeatureProcessor->GetOuterObbWs(handle).GetHalfLengths());
                        objectSrg->SetConstant(innerObbHalfLengthsConstantIndex, reflectionProbeFeatureProcessor->GetInnerObbWs(handle).GetHalfLengths());
                        objectSrg->SetConstant(useReflectionProbeConstantIndex, true);
                        objectSrg->SetConstant(useParallaxCorrectionConstantIndex, reflectionProbeFeatureProcessor->GetUseParallaxCorrection(handle));
                        objectSrg->SetConstant(exposureConstantIndex, reflectionProbeFeatureProcessor->GetRenderExposure(handle));

                        objectSrg->SetImage(reflectionCubeMapImageIndex, reflectionProbeFeatureProcessor->GetCubeMap(handle));
                    }
                    else
                    {
                        objectSrg->SetConstant(useReflectionProbeConstantIndex, false);
                    }
                }

                objectSrg->Compile();
            }

            // Set m_objectSrgNeedsUpdate to false if there are object SRGs in the list
            m_objectSrgNeedsUpdate = m_objectSrgNeedsUpdate && (m_objectSrgList.size() == 0);
        }

        bool ModelDataInstance::MaterialRequiresForwardPassIblSpecular(Data::Instance<RPI::Material> material) const
        {
            // look for a shader that has the o_materialUseForwardPassIBLSpecular option set
            // Note: this should be changed to have the material automatically set the forwardPassIBLSpecular
            // property and look for that instead of the shader option.
            // [GFX TODO][ATOM-5040] Address Property Metadata Feedback Loop
            for (auto& shaderItem : material->GetShaderCollection())
            {
                if (shaderItem.IsEnabled())
                {
                    RPI::ShaderOptionIndex index = shaderItem.GetShaderOptionGroup().GetShaderOptionLayout()->FindShaderOptionIndex(Name{ "o_materialUseForwardPassIBLSpecular" });
                    if (index.IsValid())
                    {
                        RPI::ShaderOptionValue value = shaderItem.GetShaderOptionGroup().GetValue(Name{ "o_materialUseForwardPassIBLSpecular" });
                        if (value.GetIndex() == 1)
                        {
                            return true;
                        }
                    }

                }
            }

            return false;
        }

        void ModelDataInstance::SetVisible(bool isVisible)
        {
            m_visible = isVisible;
            m_cullable.m_isHidden = !isVisible;
        }

        void ModelDataInstance::OnRebuildMaterialInstance()
        {
            if (m_visible && m_descriptor.m_isRayTracingEnabled)
            {
                SetRayTracingData();
            }
        }
    } // namespace Render
} // namespace AZ
