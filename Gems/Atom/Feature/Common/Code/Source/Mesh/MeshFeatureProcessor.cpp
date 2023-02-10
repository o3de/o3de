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
#include <Atom/Feature/Mesh/MeshCommon.h>
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
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Name/NameDictionary.h>
#include <algorithm>
#include <execution>

namespace AZ
{
    namespace Render
    {
        static AZ::Name s_o_meshUseForwardPassIBLSpecular_Name =
            AZ::Name::FromStringLiteral("o_meshUseForwardPassIBLSpecular", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_Manual_Name = AZ::Name::FromStringLiteral("Manual", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_Multiply_Name = AZ::Name::FromStringLiteral("Multiply", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_BaseColorTint_Name = AZ::Name::FromStringLiteral("BaseColorTint", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_BaseColor_Name = AZ::Name::FromStringLiteral("BaseColor", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_baseColor_color_Name = AZ::Name::FromStringLiteral("baseColor.color", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_baseColor_factor_Name = AZ::Name::FromStringLiteral("baseColor.factor", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_baseColor_useTexture_Name =
            AZ::Name::FromStringLiteral("baseColor.useTexture", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_metallic_factor_Name = AZ::Name::FromStringLiteral("metallic.factor", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_roughness_factor_Name = AZ::Name::FromStringLiteral("roughness.factor", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_emissive_enable_Name = AZ::Name::FromStringLiteral("emissive.enable", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_emissive_color_Name = AZ::Name::FromStringLiteral("emissive.color", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_emissive_intensity_Name =
            AZ::Name::FromStringLiteral("emissive.intensity", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_baseColor_textureMap_Name =
            AZ::Name::FromStringLiteral("baseColor.textureMap", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_normal_textureMap_Name =
            AZ::Name::FromStringLiteral("normal.textureMap", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_metallic_textureMap_Name =
            AZ::Name::FromStringLiteral("metallic.textureMap", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_roughness_textureMap_Name =
            AZ::Name::FromStringLiteral("roughness.textureMap", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_irradiance_irradianceColorSource_Name =
            AZ::Name::FromStringLiteral("irradiance.irradianceColorSource", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_emissive_textureMap_Name =
            AZ::Name::FromStringLiteral("emissive.textureMap", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_irradiance_manualColor_Name =
            AZ::Name::FromStringLiteral("irradiance.manualColor", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_irradiance_color_Name = AZ::Name::FromStringLiteral("irradiance.color", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_baseColor_textureBlendMode_Name =
            AZ::Name::FromStringLiteral("baseColor.textureBlendMode", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_irradiance_factor_Name =
            AZ::Name::FromStringLiteral("irradiance.factor", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_opacity_mode_Name = AZ::Name::FromStringLiteral("opacity.mode", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_opacity_factor_Name = AZ::Name::FromStringLiteral("opacity.factor", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_m_rootConstantInstanceDataOffset_Name =
            AZ::Name::FromStringLiteral("m_rootConstantInstanceDataOffset", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_m_instanceDataOffset_Name =
            AZ::Name::FromStringLiteral("m_instanceDataOffset", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_m_instanceData_Name =
            AZ::Name::FromStringLiteral("m_instanceData", AZ::Interface<AZ::NameDictionary>::Get());

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
            m_reflectionProbeFeatureProcessor = GetParentScene()->GetFeatureProcessor<ReflectionProbeFeatureProcessor>();
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

            m_meshMovedFlag = GetParentScene()->GetViewTagBitRegistry().AcquireTag(MeshCommon::MeshMovedName);
            
            bool enableHardwareInstancingFlagsCvar = false;
            if (auto* console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
            {
                console->GetCvarValue("r_enableHardwareInstancing", enableHardwareInstancingFlagsCvar);

                // push the cvars value so anything in this dll can access it directly.
                console->PerformCommand(
                    AZStd::string::format("r_enableHardwareInstancing %s", enableHardwareInstancingFlagsCvar ? "true" : "false")
                        .c_str());
            }

            RHI::FreeListAllocator::Descriptor allocatorDescriptor;
            allocatorDescriptor.m_alignmentInBytes = 1;
            // TODO: make this variable in some way
            allocatorDescriptor.m_capacityInBytes = 100000;
            allocatorDescriptor.m_policy = RHI::FreeListAllocatorPolicy::BestFit;
            allocatorDescriptor.m_garbageCollectLatency = 60;
            m_meshDataAllocator.Init(allocatorDescriptor);

            // Pre-allocate memory for mesh data
            m_meshData.resize(allocatorDescriptor.m_capacityInBytes);

            m_meshInstanceManager.SetAllocator(&m_postCullingPoolAllocator);
        }

        void MeshFeatureProcessor::Deactivate()
        {
            for (auto& bufferHandler : m_perViewInstanceDataBufferHandlers)
            {
                bufferHandler.Release();
            }
            m_perViewInstanceDataBufferHandlers.clear();
            m_perViewInstanceData.clear();

            m_flagRegistry.reset();

            m_handleGlobalShaderOptionUpdate.Disconnect();

            DisableSceneNotification();
            AZ_Warning("MeshFeatureProcessor", m_modelData.size() == 0,
                "Deactivating the MeshFeatureProcessor, but there are still outstanding mesh handles.\n"
            );
            m_transformService = nullptr;
            m_rayTracingFeatureProcessor = nullptr;
            m_reflectionProbeFeatureProcessor = nullptr;
            m_forceRebuildDrawPackets = false;

            GetParentScene()->GetViewTagBitRegistry().ReleaseTag(m_meshMovedFlag);
        }

        TransformServiceFeatureProcessorInterface::ObjectId MeshFeatureProcessor::GetObjectId(const MeshHandle& meshHandle) const
        {
            if (meshHandle.IsValid())
            {
                return m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_objectId;
            }

            return TransformServiceFeatureProcessorInterface::ObjectId::Null;
        }

        void MeshFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "MeshFeatureProcessor: Simulate");

            AZ::Job* parentJob = packet.m_parentJob;
            AZStd::concurrency_check_scope scopeCheck(m_meshDataChecker);

            // This is a really awful way to iterate over instance groups...
            const auto perInstanceGroupJobLambda = [&]() -> void
            {
                m_instanceGroupIndices.clear();
                RPI::Scene* scene = GetParentScene();
                for (auto& modelDataIter : m_modelData)
                {
                    MeshFP::EndCullingData* endCullingData = modelDataIter.GetItem<ModelDataIndex::EndCullingData>();
                    //ModelDataInstance* modelData = modelDataIter.GetItem<ModelDataIndex::Instance>();
                    //auto model = modelData->GetModel();
                    for (auto& meshDataIndices : endCullingData->m_instanceIndicesByLod)
                    {
                        for (uint32_t meshDataIndex = meshDataIndices.m_startIndex;
                             meshDataIndex < meshDataIndices.m_startIndex + meshDataIndices.m_count;
                             ++meshDataIndex)
                        {
                            uint32_t instanceIndex = m_meshData[meshDataIndex].m_instanceGroupHandle_metaDataMeshOffset;
                            m_instanceGroupIndices.insert(instanceIndex);
                        }
                    }
                }

                for (MeshInstanceManager::Handle instanceIndex : m_instanceGroupIndices)
                {
                    MeshInstanceData& instanceData = m_meshInstanceManager[instanceIndex];
                    RPI::MeshDrawPacket& drawPacket = instanceData.m_drawPacket;
                    if (drawPacket.Update(*scene, m_forceRebuildDrawPackets))
                    {
                        // Clear any cached draw packets, since they need to be re-created
                        instanceData.m_perViewDrawPackets.clear();
                        instanceData.m_drawSrgInstanceDataIndices.clear();
                        // We're going to need an index for m_instanceOffset every frame for each draw item, so cache those here
                        for (auto& drawSrg : drawPacket.GetDrawSrgs())
                        {
                            RHI::ShaderInputConstantIndex drawSrgIndexInstanceOffsetIndex =
                                drawSrg->FindShaderInputConstantIndex(s_m_instanceDataOffset_Name);

                            instanceData.m_drawSrgInstanceDataIndices.push_back(drawSrgIndexInstanceOffsetIndex);
                        }
                        const RPI::MeshDrawPacket::RootConstantsLayoutList& rootConstantsLayouts =
                            instanceData.m_drawPacket.GetRootConstantsLayouts();

                        if (!rootConstantsLayouts.empty())
                        {
                            // Get the root constant layout
                            RHI::ShaderInputConstantIndex shaderInputIndex =
                                rootConstantsLayouts[0]->FindShaderInputIndex(s_m_rootConstantInstanceDataOffset_Name);

                            if (shaderInputIndex.IsValid())
                            {
                                RHI::Interval interval = rootConstantsLayouts[0]->GetInterval(shaderInputIndex);
                                instanceData.m_drawRootConstantInterval = interval;
                            }
                            else
                            {
                                instanceData.m_drawRootConstantInterval = RHI::Interval{};
                            }
                        }
                    }
                }
            };

            const auto iteratorRanges = m_modelData.GetParallelRanges();
            AZ::JobCompletion jobCompletion;
            AZStd::vector<Job*> updateJobQueue;
            for (const auto& iteratorRange : iteratorRanges)
            {
                const auto initJobLambda = [&]() -> void
                {
                    AZ_PROFILE_SCOPE(AzRender, "MeshFeatureProcessor: Simulate: Job");

                    for (auto meshDataIter = iteratorRange.first; meshDataIter != iteratorRange.second; ++meshDataIter)
                    {
                        ModelDataInstance* modelDataInstance = meshDataIter.GetItem<ModelDataIndex::Instance>();
                        MeshFP::EndCullingData* endCullingData = meshDataIter.GetItem<ModelDataIndex::EndCullingData>();
                        if (!modelDataInstance->m_model)
                        {
                            continue; // model not loaded yet
                        }

                        if (!modelDataInstance->m_visible)
                        {
                            continue;
                        }

                        if (modelDataInstance->m_needsInit)
                        {
                            modelDataInstance->Init(this, endCullingData, m_meshInstanceManager);
                        }

                        if (modelDataInstance->m_objectSrgNeedsUpdate)
                        {
                            modelDataInstance->UpdateObjectSrg(m_reflectionProbeFeatureProcessor, m_transformService);
                        }

                        if (modelDataInstance->m_needsSetRayTracingData)
                        {
                            modelDataInstance->SetRayTracingData(m_rayTracingFeatureProcessor, m_transformService);
                        }

                        if (!r_enableHardwareInstancing)
                        {
                            // [GFX TODO] [ATOM-1357] Currently all of the draw packets have to be checked for material ID changes because
                            // material properties can impact which actual shader is used, which impacts the SRG in the draw packet.
                            // This is scheduled to be optimized so the work is only done on draw packets that need it instead of having
                            // to check every one.
                            //modelDataInstance->UpdateDrawPackets(m_forceRebuildDrawPackets);
                        }
                    }
                };

                const auto updateJobLambda = [&]() -> void
                {
                    AZ_PROFILE_SCOPE(AzRender, "MeshFeatureProcessor: Simulate: Job");

                    for (auto meshDataIter = iteratorRange.first; meshDataIter != iteratorRange.second; ++meshDataIter)
                    {
                        ModelDataInstance* modelDataInstance = meshDataIter.GetItem<ModelDataIndex::Instance>();
                        MeshFP::EndCullingData* endCullingData = meshDataIter.GetItem<ModelDataIndex::EndCullingData>();

                        if (!modelDataInstance->m_model)
                        {
                            continue; // model not loaded yet
                        }

                        if (modelDataInstance->m_cullableNeedsRebuild)
                        {
                            modelDataInstance->BuildCullable(this, endCullingData, m_meshInstanceManager);
                        }

                        if (modelDataInstance->m_cullBoundsNeedsUpdate)
                        {
                            modelDataInstance->UpdateCullBounds(this, endCullingData, m_transformService);
                        }
                    }
                };
                Job* executeInitGroupJob = aznew JobFunction<decltype(initJobLambda)>(initJobLambda, true, nullptr); // Auto-deletes
                Job* executeUpdateGroupJob = aznew JobFunction<decltype(updateJobLambda)>(updateJobLambda, true, nullptr); // Auto-deletes
                updateJobQueue.push_back(executeUpdateGroupJob);
                if (parentJob)
                {
                    parentJob->StartAsChild(executeInitGroupJob);
                }
                else
                {
                    executeUpdateGroupJob->SetDependent(&jobCompletion);
                    executeUpdateGroupJob->Start();
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

                // Now that the init stage is complete, run the instance draw packet update
                // TODO: this needs to be going the parent job. Simulate is being called in a job already. If we create a new job here, and call StartAndWaitForCompletion, then this thread might try to steal work, and end up excuting something that is trying to do a blocking asset load.
                // And that blocking load will wait for the main thread to tick to load the asset. But the main thread is in RenderTick and waiting for this job to complete.
                // Although I'm not certain if utilizing the parent job will fix the problem. 
                Job* perInstanceGroupJob =
                    aznew JobFunction<decltype(perInstanceGroupJobLambda)>(perInstanceGroupJobLambda, true, nullptr); // Auto-deletes
                perInstanceGroupJob->StartAndWaitForCompletion();

                // Now that that is done, execute the update jobs
                AZ::JobCompletion updateJobCompletion;
                for (Job* updateJob : updateJobQueue)
                {
                    updateJob->SetDependent(&updateJobCompletion);
                    updateJob->Start();
                }
                updateJobCompletion.StartAndWaitForCompletion();
            }

            m_forceRebuildDrawPackets = false;
        }

        
        void MeshFeatureProcessor::OnEndCulling(const MeshFeatureProcessor::RenderPacket& packet)
        {
            if (r_enableHardwareInstancing)
            {
                AZ_PROFILE_SCOPE(RPI, "MeshFeatureProcessor: OnEndCulling");
                size_t viewIndex = 0;
                for (const RPI::ViewPtr& view : packet.m_views)
                {
                    // Update the buffer that stores the instance data for this frames visible instances
                    // This needs to happen after the mesh handles have been updated, but before culling, since we have to update the
                    // cullables to use the new draw packets
                    //
                    // Eventually there will be a unique buffer for each view storing just the instance data for
                    // the instances that are visible in that view, so things that are culled on the CPU side can be skipped
                    // For now, we're assuming that there is only one instance for any given object, so we'll create just one
                    // buffer and share it between all of the views. For culling, that draw call will just be omitted entirely for now,
                    // but objects will eventually be culled by omitting their instance data from the view's instance data buffer
                    RPI::VisibilityListView visibilityList = view->GetVisibilityList();
                    size_t instanceBufferCount = visibilityList.size();

                    // Initialize the instance data if it hasn't been created yet
                    if (m_perViewInstanceData.size() <= viewIndex)
                    {
                        m_perViewInstanceData.push_back(
                            AZStd::vector<uint32_t, AZStdIAllocator>(AZStdIAllocator(&m_postCullingPoolAllocator)));
                    }

                    AZStd::vector<uint32_t, AZStdIAllocator>& perViewInstanceData = m_perViewInstanceData[viewIndex];

                    // Initialize the buffer handler if it hasn't been created yet
                    if (m_perViewInstanceDataBufferHandlers.size() <= viewIndex)
                    {
                        GpuBufferHandler::Descriptor desc;
                        desc.m_bufferName = "MeshInstanceDataBuffer";
                        desc.m_bufferSrgName = "m_instanceData";
                        desc.m_elementSize = sizeof(uint32_t);
                        desc.m_srgLayout = RPI::RPISystemInterface::Get()->GetViewSrgLayout().get();

                        m_perViewInstanceDataBufferHandlers.push_back(GpuBufferHandler(desc));
                    }

                    GpuBufferHandler& instanceDataBufferHandler = m_perViewInstanceDataBufferHandlers[viewIndex];

                    if (instanceBufferCount > 0)
                    {
                        //perViewInstanceData.reserve(instanceBufferCount);
                        perViewInstanceData.clear();
                    }
                    m_perViewSortInstanceData.clear();
                    AZStd::unordered_set<uint32_t> visibleInstanceIndices;
                    // Even though this is larger than actual number of visible instance groups, reserve memory here to avoid several allocations later
                    visibleInstanceIndices.reserve(instanceBufferCount);
                    for (const RPI::VisiblityEntryProperties& visibilityEntry : visibilityList)
                    {
                        const MeshData* meshData = reinterpret_cast<const MeshData*>(visibilityEntry.m_userData);
                        // The metadata store offset in m_instanceGroupHandle_metaDataMeshOffset and count in m_objectId_metaDataMeshCount
                        uint32_t meshOffset = (meshData + visibilityEntry.m_lodIndex)->m_instanceGroupHandle_metaDataMeshOffset;
                        uint32_t meshCount = (meshData + visibilityEntry.m_lodIndex)->m_objectId_metaDataMeshCount;
                        {
                            for (uint32_t meshDataIndex = meshOffset; meshDataIndex < meshOffset + meshCount;
                                 ++meshDataIndex)
                            {
                                SortInstanceData instanceData;
                                instanceData.m_instanceIndex = m_meshData[meshDataIndex].m_instanceGroupHandle_metaDataMeshOffset;
                                instanceData.m_objectId = m_meshData[meshDataIndex].m_objectId_metaDataMeshCount;
                                instanceData.m_depth = visibilityEntry.m_depth;

                                /* MeshInstanceData& instanceData = m_meshInstanceManager[instanceIndex];

                                // The per-instance data is just the objectId, which is used to look up the object transform
                                // After this first pass, we will accumulate all the per-instance data into one buffer for the view,
                                // sorted by the instance key so that all data for any objects instanced together is contiguous
                                if (instanceData.m_perViewInstanceData.size() <= viewIndex)
                                {
                                    instanceData.m_perViewInstanceData.resize(viewIndex + 1);
                                    instanceData.m_perViewDrawPackets.resize(viewIndex + 1);
                                }
                                instanceData.m_perViewInstanceData[viewIndex].push_back(objectId);
                                visibleInstanceIndices.insert(instanceIndex);*/
                                m_perViewSortInstanceData.push_back(instanceData);
                            }
                        }
                    }

                    std::sort(std::execution::par_unseq, m_perViewSortInstanceData.begin(), m_perViewSortInstanceData.end());
                    uint32_t totalVisibleInstanceCount =
                        static_cast<uint32_t>(m_perViewSortInstanceData.size());
                    if (totalVisibleInstanceCount > 0)
                    {
                        /// Serial iteration over visible data
                        ///
                        ///
                        /// 
                        /*uint32_t instanceDataOffset = 0;
                        uint32_t currentInstanceGroup = m_perViewSortInstanceData[0].m_instanceIndex;
                        perViewInstanceData.reserve(m_perViewSortInstanceData.size());
                        perViewInstanceData.push_back(m_perViewSortInstanceData[0].m_objectId);
                        for (uint32_t i = 1; i < totalVisibleInstanceCount; ++i)
                        {
                            perViewInstanceData.push_back(m_perViewSortInstanceData[i].m_objectId);
                            // Anytime the instance group changes, submit a draw
                            if (m_perViewSortInstanceData[i].m_instanceIndex != currentInstanceGroup)
                            {
                                // Submit a draw
                                MeshInstanceData& instanceData = m_meshInstanceManager[currentInstanceGroup];

                                if (instanceData.m_perViewDrawPackets.size() <= viewIndex)
                                {
                                    instanceData.m_perViewDrawPackets.resize(viewIndex + 1);
                                }

                                // Cache a cloned drawpacket here
                                if (!instanceData.m_perViewDrawPackets[viewIndex])
                                {
                                    // Clone the draw packet
                                    RHI::DrawPacketBuilder drawPacketBuilder;
                                    instanceData.m_perViewDrawPackets[viewIndex] =
                                        const_cast<RHI::DrawPacket*>(drawPacketBuilder.Clone(instanceData.m_drawPacket.GetRHIDrawPacket()));
                                }

                                RHI::Ptr<RHI::DrawPacket> clonedDrawPacket = instanceData.m_perViewDrawPackets[viewIndex];

                                // Get the root constant layout
                                if (instanceData.m_drawRootConstantInterval.IsValid())
                                {
                                    // Set the instance data offset
                                    for (size_t drawItemIndex = 0; drawItemIndex < clonedDrawPacket->GetDrawItemCount(); ++drawItemIndex)
                                    {
                                        AZStd::span<uint8_t> data{ reinterpret_cast<uint8_t*>(&instanceDataOffset), sizeof(uint32_t) };
                                        clonedDrawPacket->SetRootConstant(
                                            drawItemIndex, instanceData.m_drawRootConstantInterval, data);
                                    }
                                }
                                else
                                {
                                    AZ_Error(
                                        "MeshFeatureProcessor",
                                        false,
                                        "Trying to instance something that is missing s_m_rootConstantInstanceDataOffset_Name from its "
                                        "root constant layout.");
                                }

                                // This is the number of visible instances for this view
                                uint32_t instanceCount = i - instanceDataOffset;

                                // Set the cloned draw packet instance count
                                clonedDrawPacket->SetInstanceCount(instanceCount);

                                // Submit the draw packet
                                view->AddDrawPacket(clonedDrawPacket.get(), 0.0f);

                                // Update the loop trackers
                                instanceDataOffset = i;
                                currentInstanceGroup = m_perViewSortInstanceData[i].m_instanceIndex;
                            }
                        }

                        // submit the last instance group
                        {
                            // Submit a draw
                            MeshInstanceData& instanceData = m_meshInstanceManager[currentInstanceGroup];

                            if (instanceData.m_perViewDrawPackets.size() <= viewIndex)
                            {
                                instanceData.m_perViewDrawPackets.resize(viewIndex + 1);
                            }

                            // Cache a cloned drawpacket here
                            if (!instanceData.m_perViewDrawPackets[viewIndex])
                            {
                                // Clone the draw packet
                                RHI::DrawPacketBuilder drawPacketBuilder;
                                instanceData.m_perViewDrawPackets[viewIndex] =
                                    const_cast<RHI::DrawPacket*>(drawPacketBuilder.Clone(instanceData.m_drawPacket.GetRHIDrawPacket()));
                            }

                            RHI::Ptr<RHI::DrawPacket> clonedDrawPacket = instanceData.m_perViewDrawPackets[viewIndex];

                            // Set the instance data offset
                            for (size_t i = 0; i < clonedDrawPacket->GetDrawItemCount(); ++i)
                            {
                                // Get the root constant layout
                                if (instanceData.m_drawRootConstantInterval.IsValid())
                                {
                                    AZStd::span<uint8_t> data{ reinterpret_cast<uint8_t*>(&instanceDataOffset), sizeof(uint32_t) };
                                    clonedDrawPacket->SetRootConstant(i, instanceData.m_drawRootConstantInterval, data);
                                }
                                else
                                {
                                    AZ_Error(
                                        "MeshFeatureProcessor",
                                        false,
                                        "Trying to instance something that is missing s_m_rootConstantInstanceDataOffset_Name from its "
                                        "root constant layout.");
                                }
                            }

                            // This is the number of visible instances for this view
                            uint32_t instanceCount = static_cast<uint32_t>(perViewInstanceData.size()) - instanceDataOffset;
                            // Set the cloned draw packet instance count
                            clonedDrawPacket->SetInstanceCount(instanceCount);

                            // Submit the draw packet
                            view->AddDrawPacket(clonedDrawPacket.get(), 0.0f);
                        }*/



                        /// Parallel iteration over instance data
                        ///
                        ///
                        ///
                        ///
                        ///
                        ///

                        // Make space for the final data
                        perViewInstanceData.resize_no_construct(m_perViewSortInstanceData.size());

                        // Create the task graph
                        static const AZ::TaskDescriptor buildInstanceBufferTaskDescriptor{ "AZ::Render::MeshFeatureProcessor::OnEndCulling - process instance data", "Graphics" };
                        AZ::TaskGraphEvent buildInstanceBufferTGEvent{ "BuildInstanceBuffer Wait" };
                        AZ::TaskGraph buildInstanceBufferTG{ "BuildInstanceBuffer" };
                        
                        // Divy up the work into tasks
                        constexpr uint32_t approximateBatchSize = 256;
                        uint32_t taskCount = totalVisibleInstanceCount / approximateBatchSize;
                        uint32_t currentBatchStart = 0;
                        uint32_t currentBatchEndNonInclusive = 0;
                        // For each task, find the next boundary where the work needs to be split
                        // TODO: find the boundary in the task itself? Would need to also find the correct starting point, instead of basing start on the previous end
                        for (uint32_t taskIndex = 0; taskIndex < taskCount; ++taskIndex)
                        {
                            if (taskIndex < taskCount - 1)
                            {
                                // End location is roughly here
                                uint32_t approximateEndOffset = taskIndex * approximateBatchSize + approximateBatchSize;
                                uint32_t batchEndInstanceGroup = m_perViewSortInstanceData[approximateEndOffset].m_instanceIndex;
                                // TODO: validate what happens whenthe offset overruns the start of the next batch
                                for (uint32_t actualEndOffset = approximateEndOffset;
                                     actualEndOffset < approximateEndOffset + approximateBatchSize;
                                     ++actualEndOffset)
                                {
                                    if (m_perViewSortInstanceData[actualEndOffset].m_instanceIndex != batchEndInstanceGroup)
                                    {
                                        // We've found where the current batch ends
                                        currentBatchEndNonInclusive = actualEndOffset;
                                        break;
                                    }
                                }
                            }
                            else
                            {
                                currentBatchEndNonInclusive = totalVisibleInstanceCount;
                            }

                            // Process data up to but not including actualEndOffset
                            buildInstanceBufferTG.AddTask(
                                buildInstanceBufferTaskDescriptor,
                                [this, currentBatchStart, currentBatchEndNonInclusive, viewIndex, &view, &perViewInstanceData]()
                                {
                                    uint32_t currentInstanceGroup = m_perViewSortInstanceData[currentBatchStart].m_instanceIndex;
                                    uint32_t instanceDataOffset = currentBatchStart;
                                    float depth = 0.0f;
                                    for (uint32_t i = currentBatchStart; i < currentBatchEndNonInclusive; ++i)
                                    {
                                        perViewInstanceData[i] = m_perViewSortInstanceData[i].m_objectId;
                                        depth += m_perViewSortInstanceData[i].m_depth;
                                        // Anytime the instance group changes, submit a draw
                                        if (m_perViewSortInstanceData[i].m_instanceIndex != currentInstanceGroup)
                                        {
                                            // Submit a draw
                                            MeshInstanceData& instanceData = m_meshInstanceManager[currentInstanceGroup];

                                            if (instanceData.m_perViewDrawPackets.size() <= viewIndex)
                                            {
                                                AZStd::scoped_lock meshDataLock(m_meshDataMutex);
                                                instanceData.m_perViewDrawPackets.resize(viewIndex + 1);
                                            }

                                            // Cache a cloned drawpacket here
                                            if (!instanceData.m_perViewDrawPackets[viewIndex])
                                            {
                                                // Clone the draw packet
                                                RHI::DrawPacketBuilder drawPacketBuilder;
                                                instanceData.m_perViewDrawPackets[viewIndex] = const_cast<RHI::DrawPacket*>(
                                                    drawPacketBuilder.Clone(instanceData.m_drawPacket.GetRHIDrawPacket()));
                                            }

                                            RHI::Ptr<RHI::DrawPacket> clonedDrawPacket = instanceData.m_perViewDrawPackets[viewIndex];

                                            // Get the root constant layout
                                            // TODO: Verify the validity of the root constant interval elsewhere
                                            //if (instanceData.m_drawRootConstantInterval.IsValid())
                                            {
                                                // Set the instance data offset
                                                AZStd::span<uint8_t> data{ reinterpret_cast<uint8_t*>(&instanceDataOffset),
                                                                            sizeof(uint32_t) };
                                                clonedDrawPacket->SetRootConstant(instanceData.m_drawRootConstantInterval, data);
                                            }
                                            /* else
                                            {
                                                AZ_Error(
                                                    "MeshFeatureProcessor",
                                                    false,
                                                    "Trying to instance something that is missing s_m_rootConstantInstanceDataOffset_Name "
                                                    "from its "
                                                    "root constant layout.");
                                            }*/

                                            // This is the number of visible instances for this view
                                            uint32_t instanceCount = i - instanceDataOffset;

                                            // Set the cloned draw packet instance count
                                            clonedDrawPacket->SetInstanceCount(instanceCount);

                                            float averageDepth = depth / static_cast<float>(instanceCount);
                                            depth = 0;

                                            // Submit the draw packet
                                            // TODO: accumulate average depth and submit here
                                            view->AddDrawPacket(clonedDrawPacket.get(), averageDepth);

                                            // Update the loop trackers
                                            instanceDataOffset = i;
                                            currentInstanceGroup = m_perViewSortInstanceData[i].m_instanceIndex;
                                        }
                                    }

                                    // submit the last instance group
                                    {
                                        // Submit a draw
                                        MeshInstanceData& instanceData = m_meshInstanceManager[currentInstanceGroup];

                                        if (instanceData.m_perViewDrawPackets.size() <= viewIndex)
                                        {
                                            AZStd::scoped_lock meshDataLock(m_meshDataMutex);
                                            instanceData.m_perViewDrawPackets.resize(viewIndex + 1);
                                        }

                                        // Cache a cloned drawpacket here
                                        if (!instanceData.m_perViewDrawPackets[viewIndex])
                                        {
                                            // Clone the draw packet
                                            RHI::DrawPacketBuilder drawPacketBuilder;
                                            instanceData.m_perViewDrawPackets[viewIndex] = const_cast<RHI::DrawPacket*>(
                                                drawPacketBuilder.Clone(instanceData.m_drawPacket.GetRHIDrawPacket()));
                                        }

                                        RHI::Ptr<RHI::DrawPacket> clonedDrawPacket = instanceData.m_perViewDrawPackets[viewIndex];

                                        // Get the root constant layout
                                        // TODO: Verify the validity of the root constant interval elsewhere
                                        // if (instanceData.m_drawRootConstantInterval.IsValid())
                                        {
                                            // Set the instance data offset
                                            AZStd::span<uint8_t> data{ reinterpret_cast<uint8_t*>(&instanceDataOffset), sizeof(uint32_t) };
                                            clonedDrawPacket->SetRootConstant(instanceData.m_drawRootConstantInterval, data);
                                        }
                                        /* else
                                        {
                                            AZ_Error(
                                                "MeshFeatureProcessor",
                                                false,
                                                "Trying to instance something that is missing s_m_rootConstantInstanceDataOffset_Name "
                                                "from its "
                                                "root constant layout.");
                                        }*/

                                        // This is the number of visible instances for this view
                                        uint32_t instanceCount = currentBatchEndNonInclusive - instanceDataOffset;
                                        // Set the cloned draw packet instance count
                                        clonedDrawPacket->SetInstanceCount(instanceCount);

                                        // Submit the draw packet
                                        view->AddDrawPacket(clonedDrawPacket.get(), 0.0f);
                                    }
                                });

                                currentBatchStart = currentBatchEndNonInclusive;
                        }

                        // submit the tasks
                        buildInstanceBufferTG.Submit(&buildInstanceBufferTGEvent);
                        buildInstanceBufferTGEvent.Wait();
                    }





                    /// Mesh instance manager owns partial lists of instance data
                    ///
                    ///
                    /// 
                    /* for (uint32_t instanceIndex : visibleInstanceIndices)
                    {
                        MeshInstanceData& instanceData = m_meshInstanceManager[instanceIndex];

                        if (instanceData.m_perViewDrawPackets.size() <= viewIndex)
                        {
                            instanceData.m_perViewDrawPackets.resize(viewIndex + 1);
                        }

                        // Cache a cloned drawpacket here
                        if (!instanceData.m_perViewDrawPackets[viewIndex])
                        {
                            // Clone the draw packet
                            RHI::DrawPacketBuilder drawPacketBuilder;
                            instanceData.m_perViewDrawPackets[viewIndex] =
                                const_cast<RHI::DrawPacket*>(drawPacketBuilder.Clone(instanceData.m_drawPacket.GetRHIDrawPacket()));
                        }

                        RHI::Ptr<RHI::DrawPacket> clonedDrawPacket = instanceData.m_perViewDrawPackets[viewIndex];

                        // Get the instanceData offset
                        // The offset into the per-instance data is part of the instanced draw call. This can change every frame,
                        // and will differ between views
                        [[maybe_unused]] uint32_t instanceDataOffset = static_cast<uint32_t>(perViewInstanceData.size());

                        // Set the instance data offset
                        for (size_t i = 0; i < clonedDrawPacket->GetDrawItemCount(); ++i)
                        {
                            // Get the root constant layout
                            if (instanceData.m_drawRootConstantIntervals[i].IsValid())
                            {
                                AZStd::span<uint8_t> data{ reinterpret_cast<uint8_t*>(&instanceDataOffset), sizeof(uint32_t) };
                                clonedDrawPacket->SetRootConstant(i, instanceData.m_drawRootConstantIntervals[i], data);
                            }
                            else
                            {
                                AZ_Error(
                                    "MeshFeatureProcessor",
                                    false,
                                    "Trying to instance something that is missing s_m_rootConstantInstanceDataOffset_Name from its "
                                    "root constant layout.");
                            }
                        }

                        // These are the object Id's (instance data) for all the instances visible by this view
                        AZStd::vector<uint32_t>& instanceDataForCurrentIndexAndView = instanceData.m_perViewInstanceData[viewIndex];
                        // This is the number of visible instances for this view
                        uint32_t instanceCount = static_cast<uint32_t>(instanceDataForCurrentIndexAndView.size());
                        // Set the cloned draw packet instance count
                        clonedDrawPacket->SetInstanceCount(instanceCount);

                        // Add the actual instance data for this view
                        perViewInstanceData.insert(
                            perViewInstanceData.end(),
                            instanceDataForCurrentIndexAndView.begin(),
                            instanceDataForCurrentIndexAndView.end());
                        instanceDataForCurrentIndexAndView.clear();

                        // Submit the draw packet
                        view->AddDrawPacket(clonedDrawPacket.get(), 0.0f);
                    }*/

                    // Use the correct srg for the view
                    instanceDataBufferHandler.UpdateSrg(view->GetShaderResourceGroup().get());

                    // Now that we have all of our instance data, we need to create the buffer and bind it to the view srgs
                    // Eventually, this could be a transient buffer

                    // create output buffer descriptors
                    instanceDataBufferHandler.UpdateBuffer(perViewInstanceData.data(), static_cast<uint32_t>(perViewInstanceData.size()));
                    viewIndex++;
                }
            }
        }

        void MeshFeatureProcessor::OnBeginPrepareRender()
        {
            m_meshDataChecker.soft_lock();
            AZ_Error("MeshFeatureProcessor::OnBeginPrepareRender", !(r_enablePerMeshShaderOptionFlags && r_enableHardwareInstancing),
                "r_enablePerMeshShaderOptionFlags and r_enableHardwareInstancing are incompatible at this time. r_enablePerMeshShaderOptionFlags results "
                "in a unique shader permutation for a given object depending on which light types are in range of the object. This isn't known until "
                "immediately before rendering. Determining whether or not two meshes can be instanced happens when the object is first set up, and we don't "
                "want to update that instance map every frame, so if instancing is enabled we treat r_enablePerMeshShaderOptionFlags as disabled. "
                "This can be relaxed for static meshes in the future when we know they won't be moving. ");
            if (!r_enablePerMeshShaderOptionFlags && m_enablePerMeshShaderOptionFlags && !r_enableHardwareInstancing)
            {
                // Per mesh shader option flags was on, but now turned off, so reset all the shader options.
                for (auto modelHandle : m_modelData)
                {
                    ModelDataInstance* model = modelHandle.GetItem<ModelDataIndex::Instance>();
                    for (RPI::MeshDrawPacketList& drawPacketList : model->m_drawPacketListsByLod)
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
                    model->m_cullable.m_shaderOptionFlags = 0;
                    model->m_cullable.m_prevShaderOptionFlags = 0;
                    model->m_cullableNeedsRebuild = true;

                    // [GHI-13619]
                    // Update the draw packets on the cullable, since we just set a shader item.
                    // BuildCullable is a bit overkill here, this could be reduced to just updating the drawPacket specific info
                    // It's also going to cause m_cullableNeedsUpdate to be set, which will execute next frame, which we don't need
                    MeshFP::EndCullingData* endCullingData = modelHandle.GetItem<ModelDataIndex::EndCullingData>();
                    model->BuildCullable(this, endCullingData,m_meshInstanceManager);
                }
            }

            m_enablePerMeshShaderOptionFlags = r_enablePerMeshShaderOptionFlags && !r_enableHardwareInstancing;

            if (m_enablePerMeshShaderOptionFlags)
            {
                for (auto modelHandle : m_modelData)
                {
                    ModelDataInstance* model = modelHandle.GetItem<ModelDataIndex::Instance>();
                    if (model->m_cullable.m_prevShaderOptionFlags != model->m_cullable.m_shaderOptionFlags)
                    {
                        // Per mesh shader option flags have changed, so rebuild the draw packet with the new shader options.
                        for (RPI::MeshDrawPacketList& drawPacketList : model->m_drawPacketListsByLod)
                        {
                            for (RPI::MeshDrawPacket& drawPacket : drawPacketList)
                            {
                                m_flagRegistry->VisitTags(
                                    [&](AZ::Name shaderOption, FlagRegistry::TagType tag)
                                    {
                                        bool shaderOptionValue = (model->m_cullable.m_shaderOptionFlags & tag.GetIndex()) > 0;
                                        drawPacket.SetShaderOption(shaderOption, AZ::RPI::ShaderOptionValue(shaderOptionValue));
                                    }
                                );
                                drawPacket.Update(*GetParentScene(), true);
                            }
                        }
                        model->m_cullableNeedsRebuild = true;

                        // [GHI-13619]
                        // Update the draw packets on the cullable, since we just set a shader item.
                        // BuildCullable is a bit overkill here, this could be reduced to just updating the drawPacket specific info
                        // It's also going to cause m_cullableNeedsUpdate to be set, which will execute next frame, which we don't need
                        MeshFP::EndCullingData* endCullingData = modelHandle.GetItem<ModelDataIndex::EndCullingData>();
                        model->BuildCullable(this, endCullingData, m_meshInstanceManager);
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
            for (auto modelHandle : m_modelData)
            {
                ModelDataInstance* model = modelHandle.GetItem<ModelDataIndex::Instance>();
                model->m_cullable.m_prevShaderOptionFlags = model->m_cullable.m_shaderOptionFlags.exchange(0);
                model->m_cullable.m_flags = model->m_isAlwaysDynamic ? m_meshMovedFlag.GetIndex() : 0;
            }
            m_meshDataAllocator.GarbageCollect();
        }


        MeshFeatureProcessor::MeshHandle MeshFeatureProcessor::AcquireMesh(
            const MeshHandleDescriptor& descriptor,
            const MaterialAssignmentMap& materials)
        {
            AZ_PROFILE_SCOPE(AzRender, "MeshFeatureProcessor: AcquireMesh");

            // don't need to check the concurrency during emplace() because the StableDynamicArray won't move the other elements during insertion
            MeshHandle meshDataHandle = m_modelData.emplace((int)0);
            ModelDataInstance& modelDataInstance = m_modelData.GetData<ModelDataIndex::Instance>(meshDataHandle);
            modelDataInstance.m_descriptor = descriptor;
            modelDataInstance.m_scene = GetParentScene();
            modelDataInstance.m_materialAssignments = materials;
            modelDataInstance.m_objectId = m_transformService->ReserveObjectId();
            modelDataInstance.m_rayTracingUuid = AZ::Uuid::CreateRandom();
            modelDataInstance.m_originalModelAsset = descriptor.m_modelAsset;
            modelDataInstance.m_meshLoader = AZStd::make_unique<ModelDataInstance::MeshLoader>(descriptor.m_modelAsset, &modelDataInstance);
            modelDataInstance.m_isAlwaysDynamic = descriptor.m_isAlwaysDynamic;

            modelDataInstance.UpdateMaterialChangeIds();

            MeshFP::EndCullingData& endCullingData = m_modelData.GetData<ModelDataIndex::EndCullingData>(meshDataHandle);
            endCullingData.m_objectId = modelDataInstance.m_objectId;

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
                m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_meshLoader.reset();
                m_modelData.GetData<ModelDataIndex::Instance>(meshHandle)
                    .DeInit(this, &m_modelData.GetData<ModelDataIndex::EndCullingData>(meshHandle), m_meshInstanceManager, m_rayTracingFeatureProcessor);
                m_transformService->ReleaseObjectId(m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_objectId);

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
                MeshHandle clone = AcquireMesh(m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_descriptor, m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_materialAssignments);
                return clone;
            }
            return MeshFeatureProcessor::MeshHandle();
        }

        Data::Instance<RPI::Model> MeshFeatureProcessor::GetModel(const MeshHandle& meshHandle) const
        {
            return meshHandle.IsValid() ? m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_model : nullptr;
        }

        Data::Asset<RPI::ModelAsset> MeshFeatureProcessor::GetModelAsset(const MeshHandle& meshHandle) const
        {
            if (meshHandle.IsValid())
            {
                return m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_originalModelAsset;
            }

            return {};
        }
        
        const RPI::MeshDrawPacketLods& MeshFeatureProcessor::GetDrawPackets(const MeshHandle& meshHandle) const
        {
            return meshHandle.IsValid() ? m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_drawPacketListsByLod : m_emptyDrawPacketLods;
        }

        const AZStd::vector<Data::Instance<RPI::ShaderResourceGroup>>& MeshFeatureProcessor::GetObjectSrgs(const MeshHandle& meshHandle) const
        {
            static AZStd::vector<Data::Instance<RPI::ShaderResourceGroup>> staticEmptyList;
            return meshHandle.IsValid() ? m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_objectSrgList : staticEmptyList;
        }

        void MeshFeatureProcessor::QueueObjectSrgForCompile(const MeshHandle& meshHandle) const
        {
            if (meshHandle.IsValid())
            {
                m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_objectSrgNeedsUpdate = true;
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
                if (m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_model)
                {
                    Data::Instance<RPI::Model> model = m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_model;
                    m_modelData.GetData<ModelDataIndex::Instance>(meshHandle)
                        .DeInit(this,
                            &m_modelData.GetData<ModelDataIndex::EndCullingData>(meshHandle),
                            m_meshInstanceManager,
                            m_rayTracingFeatureProcessor);
                    m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_materialAssignments = materials;
                    m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).QueueInit(model);
                }
                else
                {
                    m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_materialAssignments = materials;
                }

                m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).UpdateMaterialChangeIds();

                m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_objectSrgNeedsUpdate = true;
            }
        }

        const MaterialAssignmentMap& MeshFeatureProcessor::GetMaterialAssignmentMap(const MeshHandle& meshHandle) const
        {
            return meshHandle.IsValid() ? m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_materialAssignments : DefaultMaterialAssignmentMap;
        }

        void MeshFeatureProcessor::ConnectModelChangeEventHandler(const MeshHandle& meshHandle, ModelChangedEvent::Handler& handler)
        {
            if (meshHandle.IsValid())
            {
                handler.Connect(m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_meshLoader->GetModelChangedEvent());
            }
        }

        void MeshFeatureProcessor::SetTransform(const MeshHandle& meshHandle, const AZ::Transform& transform, const AZ::Vector3& nonUniformScale)
        {
            if (meshHandle.IsValid())
            {
                ModelDataInstance& modelData = m_modelData.GetData<ModelDataIndex::Instance>(meshHandle);
                modelData.m_cullBoundsNeedsUpdate = true;
                modelData.m_objectSrgNeedsUpdate = true;
                modelData.m_cullable.m_flags = modelData.m_cullable.m_flags | m_meshMovedFlag.GetIndex();

                m_transformService->SetTransformForId(m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_objectId, transform, nonUniformScale);

                // ray tracing data needs to be updated with the new transform
                if (m_rayTracingFeatureProcessor)
                {
                    m_rayTracingFeatureProcessor->SetMeshTransform(m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_rayTracingUuid, transform, nonUniformScale);
                }
            }
        }

        void MeshFeatureProcessor::SetLocalAabb(const MeshHandle& meshHandle, const AZ::Aabb& localAabb)
        {
            if (meshHandle.IsValid())
            {
                ModelDataInstance& modelData = m_modelData.GetData<ModelDataIndex::Instance>(meshHandle);
                modelData.m_aabb = localAabb;
                modelData.m_cullBoundsNeedsUpdate = true;
                modelData.m_objectSrgNeedsUpdate = true;
            }
        };

        AZ::Aabb MeshFeatureProcessor::GetLocalAabb(const MeshHandle& meshHandle) const
        {
            if (meshHandle.IsValid())
            {
                return m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_aabb;
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
                return m_transformService->GetTransformForId(m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_objectId);
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
                return m_transformService->GetNonUniformScaleForId(m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_objectId);
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
                m_modelData.GetData<ModelDataIndex::Instance>(meshHandle)
                    .SetSortKey(this,
                        &m_modelData.GetData<ModelDataIndex::EndCullingData>(meshHandle),
                        m_meshInstanceManager,
                        m_rayTracingFeatureProcessor,
                        sortKey);
            }
        }

        RHI::DrawItemSortKey MeshFeatureProcessor::GetSortKey(const MeshHandle& meshHandle) const
        {
            if (meshHandle.IsValid())
            {
                return m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).GetSortKey();
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
                m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).SetMeshLodConfiguration(meshLodConfig);
            }
        }

        RPI::Cullable::LodConfiguration MeshFeatureProcessor::GetMeshLodConfiguration(const MeshHandle& meshHandle) const
        {
            if (meshHandle.IsValid())
            {
                return m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).GetMeshLodConfiguration();
            }
            else
            {
                AZ_Assert(false, "Invalid mesh handle");
                return {RPI::Cullable::LodType::Default, 0, 0.0f, 0.0f };
            }
        }

        void MeshFeatureProcessor::SetIsAlwaysDynamic(const MeshHandle & meshHandle, bool isAlwaysDynamic)
        {
            if (meshHandle.IsValid())
            {
                m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_isAlwaysDynamic = isAlwaysDynamic;
            }
        }

        bool MeshFeatureProcessor::GetIsAlwaysDynamic(const MeshHandle& meshHandle) const
        {
            if (!meshHandle.IsValid())
            {
                AZ_Assert(false, "Invalid mesh handle");
                return false;
            }
            return m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_isAlwaysDynamic;
        }

        void MeshFeatureProcessor::SetExcludeFromReflectionCubeMaps(const MeshHandle& meshHandle, bool excludeFromReflectionCubeMaps)
        {
            if (meshHandle.IsValid())
            {
                m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_excludeFromReflectionCubeMaps = excludeFromReflectionCubeMaps;
                if (excludeFromReflectionCubeMaps)
                {
                    m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_cullable.m_cullData.m_hideFlags |= RPI::View::UsageReflectiveCubeMap;
                }
                else
                {
                    m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_cullable.m_cullData.m_hideFlags &= ~RPI::View::UsageReflectiveCubeMap;
                }
            }
        }

        void MeshFeatureProcessor::SetRayTracingEnabled(const MeshHandle& meshHandle, bool rayTracingEnabled)
        {
            if (meshHandle.IsValid())
            {
                // update the ray tracing data based on the current state and the new state
                if (rayTracingEnabled && !m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_descriptor.m_isRayTracingEnabled)
                {
                    // add to ray tracing
                    m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_needsSetRayTracingData = true;
                }
                else if (!rayTracingEnabled && m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_descriptor.m_isRayTracingEnabled)
                {
                    // remove from ray tracing
                    if (m_rayTracingFeatureProcessor)
                    {
                        m_rayTracingFeatureProcessor->RemoveMesh(m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_rayTracingUuid);
                    }
                }

                // set new state
                m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_descriptor.m_isRayTracingEnabled = rayTracingEnabled;
            }
        }

        bool MeshFeatureProcessor::GetRayTracingEnabled(const MeshHandle& meshHandle) const
        {
            if (meshHandle.IsValid())
            {
                return m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_descriptor.m_isRayTracingEnabled;
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
                return m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_visible;
            }
            return false;
        }

        void MeshFeatureProcessor::SetVisible(const MeshHandle& meshHandle, bool visible)
        {
            if (meshHandle.IsValid())
            {
                m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).SetVisible(visible);

                if (m_rayTracingFeatureProcessor && m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_descriptor.m_isRayTracingEnabled)
                {
                    // always remove from ray tracing first
                    m_rayTracingFeatureProcessor->RemoveMesh(m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_rayTracingUuid);

                    // now add if it's visible
                    if (visible)
                    {
                        m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_needsSetRayTracingData = true;
                    }
                }
            }
        }

        void MeshFeatureProcessor::SetUseForwardPassIblSpecular(const MeshHandle& meshHandle, bool useForwardPassIblSpecular)
        {
            if (meshHandle.IsValid())
            {
                m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_descriptor.m_useForwardPassIblSpecular = useForwardPassIblSpecular;
                m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_objectSrgNeedsUpdate = true;

                if (m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_model)
                {
                    const size_t modelLodCount = m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_model->GetLodCount();
                    for (size_t modelLodIndex = 0; modelLodIndex < modelLodCount; ++modelLodIndex)
                    {
                        m_modelData.GetData<ModelDataIndex::Instance>(meshHandle)
                            .BuildDrawPacketList(this,
                                &m_modelData.GetData<ModelDataIndex::EndCullingData>(meshHandle),
                                m_meshInstanceManager, modelLodIndex);
                    }
                }
            }
        }
        
        const RPI::Cullable* MeshFeatureProcessor::GetCullable(const MeshHandle& meshHandle)
        {
            if (meshHandle.IsValid())
            {
                return &m_modelData.GetData<ModelDataIndex::Instance>(meshHandle).m_cullable;
            }
            return nullptr;
        }

        RHI::Ptr<MeshFeatureProcessor::FlagRegistry> MeshFeatureProcessor::GetShaderOptionFlagRegistry()
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
            for (auto meshHandle : m_modelData)
            {
                ModelDataInstance* meshInstance = meshHandle.GetItem<ModelDataIndex::Instance>();
                if (meshInstance->m_descriptor.m_useForwardPassIblSpecular)
                {
                    meshInstance->m_objectSrgNeedsUpdate = true;
                }
            }
        }

        void MeshFeatureProcessor::ReportShaderOptionFlags([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
        {
            m_reportShaderOptionFlags = true;
        }

        MeshFP::MeshDataIndicesForLod MeshFeatureProcessor::AcquireMeshIndices(uint32_t lodCount, uint32_t meshCount)
        {
            AZStd::lock_guard<AZStd::mutex> guard(m_meshDataMutex);
            MeshFP::MeshDataIndicesForLod result;
            // We use lodCount + meshCount so that we can store the metadata for each lod before the mesh data
            result.m_startIndex = static_cast<uint32_t>(m_meshDataAllocator.Allocate(lodCount + meshCount, 1).m_ptr);
            result.m_count = meshCount;
            return result;
        }

        void MeshFeatureProcessor::ReleaseMeshIndices(MeshFP::MeshDataIndicesForLod meshDataIndices)
        {
            AZStd::lock_guard<AZStd::mutex> guard(m_meshDataMutex);
            m_meshDataAllocator.DeAllocate(RHI::VirtualAddress(static_cast<uintptr_t>(meshDataIndices.m_startIndex)));
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

            for (auto modelHandle : m_modelData)
            {
                ModelDataInstance* model = modelHandle.GetItem<ModelDataIndex::Instance>();
                ++flagStats[model->m_cullable.m_shaderOptionFlags.load()];
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
                // Assume cloned models will involve some kind of geometry deformation
                m_parent->m_isAlwaysDynamic = true;
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
                RayTracingFeatureProcessor* rayTracingFeatureProcessor = m_parent->m_scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
                m_parent->RemoveRayTracingData(rayTracingFeatureProcessor);
                m_parent->QueueInit(model);
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

        void ModelDataInstance::DeInit(MeshFeatureProcessor* meshFeatureProcessor, MeshFP::EndCullingData* endCullingData, MeshInstanceManager& meshInstanceManager, RayTracingFeatureProcessor* rayTracingFeatureProcessor)
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

            RemoveRayTracingData(rayTracingFeatureProcessor);

            if (!r_enableHardwareInstancing)
            {
                m_drawPacketListsByLod.clear();
            }
            else
            {
                // Release the instance indices
                for (auto& meshDataIndices : endCullingData->m_instanceIndicesByLod)
                {
                    {
                        for (uint32_t meshDataIndex = meshDataIndices.m_startIndex;
                             meshDataIndex < meshDataIndices.m_startIndex + meshDataIndices.m_count;
                             ++meshDataIndex)
                        {
                            meshInstanceManager.RemoveInstance(
                                meshFeatureProcessor->m_meshData[meshDataIndex].m_instanceGroupHandle_metaDataMeshOffset);
                        }
                    }
                }
                endCullingData->m_instanceIndicesByLod.clear();
                // Need to verify that this thing has actually been initialized with valid indices
                // TODO: Maybe skip the whole function if !m_needsInit
                if (!m_needsInit)
                {
                    meshFeatureProcessor->ReleaseMeshIndices(m_meshDataIndices);
                }
            }
            m_materialAssignments.clear();
            m_materialChangeIds.clear();
            m_objectSrgList = {};
            m_model = {};
        }

        void ModelDataInstance::QueueInit(const Data::Instance<RPI::Model>& model)
        {
            m_model = model;
            m_needsInit = true;
            m_aabb = m_model->GetModelAsset()->GetAabb();
        }

        void ModelDataInstance::Init(
            MeshFeatureProcessor* meshFeatureProcessor, MeshFP::EndCullingData* endCullingData, MeshInstanceManager& meshInstanceManager)
        {
            const size_t modelLodCount = m_model->GetLodCount();

            // If HW Instancing is enabled, the draw packets will reside in the MeshInstanceManager
            if (!r_enableHardwareInstancing)
            {
                m_drawPacketListsByLod.resize(modelLodCount);
            }
            else
            {
                endCullingData->m_instanceIndicesByLod.resize(modelLodCount);
            }

            uint32_t totalMeshCount = 0;
            for (size_t modelLodIndex = 0; modelLodIndex < modelLodCount; ++modelLodIndex)
            {
                RPI::ModelLod& modelLod = *m_model->GetLods()[modelLodIndex];
                totalMeshCount += static_cast<uint32_t>(modelLod.GetMeshes().size());
            }
            m_meshDataIndices = meshFeatureProcessor->AcquireMeshIndices(static_cast<uint32_t>(modelLodCount), totalMeshCount);
            uint32_t meshOffset = m_meshDataIndices.m_startIndex + static_cast<uint32_t>(modelLodCount);
            for (size_t modelLodIndex = 0; modelLodIndex < modelLodCount; ++modelLodIndex)
            {
                // We're going to encode two things into the lod entry from the mesh data
                // The offset to the start of the lod, and the mesh count for that lod
                // This way, culling can know where the data is and how many meshes there are, without
                // needing to fetch any data from the model data instance
                RPI::ModelLod& modelLod = *m_model->GetLods()[modelLodIndex];

                // Store the metadata for the lod
                uint32_t meshCount = static_cast<uint32_t>(modelLod.GetMeshes().size());
                meshFeatureProcessor->m_meshData[m_meshDataIndices.m_startIndex + modelLodIndex].m_instanceGroupHandle_metaDataMeshOffset =
                    meshOffset;
                meshFeatureProcessor->m_meshData[m_meshDataIndices.m_startIndex + modelLodIndex].m_objectId_metaDataMeshCount = meshCount;
                    
                meshOffset += meshCount;
                BuildDrawPacketList(meshFeatureProcessor, endCullingData, meshInstanceManager, modelLodIndex);
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
                m_needsSetRayTracingData = true;
            }

            m_cullableNeedsRebuild = true;
            m_cullBoundsNeedsUpdate = true;
            m_objectSrgNeedsUpdate = true;
            m_needsInit = false;
        }

        void ModelDataInstance::BuildDrawPacketList(
            MeshFeatureProcessor* meshFeatureProcessor,
            MeshFP::EndCullingData* endCullingData, MeshInstanceManager& meshInstanceManager, size_t modelLodIndex)
        {
            RPI::ModelLod& modelLod = *m_model->GetLods()[modelLodIndex];
            const size_t meshCount = modelLod.GetMeshes().size();

            if (!r_enableHardwareInstancing)
            {
                RPI::MeshDrawPacketList& drawPacketListOut = m_drawPacketListsByLod[modelLodIndex];
                drawPacketListOut.clear();
                drawPacketListOut.reserve(meshCount);
            }
            else
            {
                MeshFP::MeshDataIndicesForLod& meshInstanceIndices = endCullingData->m_instanceIndicesByLod[modelLodIndex];
                meshInstanceIndices.m_startIndex = meshFeatureProcessor->m_meshData[m_meshDataIndices.m_startIndex + modelLodIndex].m_instanceGroupHandle_metaDataMeshOffset;
                meshInstanceIndices.m_count = static_cast<uint32_t>(meshCount);
            }

            uint32_t meshDataStartForLod =
                meshFeatureProcessor->m_meshData[m_meshDataIndices.m_startIndex + modelLodIndex].m_instanceGroupHandle_metaDataMeshOffset;
            // Get the offset for the lod
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

                
                bool materialRequiresForwardPassIblSpecular = MaterialRequiresForwardPassIblSpecular(material);

                // Track whether any materials in this mesh require ForwardPassIblSpecular, we need this information when the ObjectSrg is
                // updated
                m_hasForwardPassIblSpecularMaterial |= materialRequiresForwardPassIblSpecular;

                // stencil bits
                uint8_t stencilRef = m_descriptor.m_useForwardPassIblSpecular || materialRequiresForwardPassIblSpecular
                    ? Render::StencilRefs::None
                    : Render::StencilRefs::UseIBLSpecularPass;
                stencilRef |= Render::StencilRefs::UseDiffuseGIPass;

                InsertResult index{ InvalidIndex, false };

                if (r_enableHardwareInstancing)
                {
                    // Get thi instance index for referencing the draw packet
                    MeshInstanceKey key{};
                    key.m_modelId = m_model->GetId();
                    key.m_lodIndex = static_cast<uint32_t>(modelLodIndex);
                    key.m_meshIndex = static_cast<uint32_t>(meshIndex);
                    key.m_materialId = material->GetId();
                    key.m_forceInstancingOff = Uuid::CreateRandom();
                    key.m_stencilRef = stencilRef;
                    key.m_sortKey = m_sortKey;
                    index = meshInstanceManager.AddInstance(key);
                    [[maybe_unused]]MeshFP::MeshDataIndicesForLod& meshInstanceIndices = endCullingData->m_instanceIndicesByLod[modelLodIndex];
                    meshFeatureProcessor->m_meshData[meshDataStartForLod + meshIndex].m_instanceGroupHandle_metaDataMeshOffset =
                        index.m_handle.GetUint();
                    meshFeatureProcessor->m_meshData[meshDataStartForLod + meshIndex].m_objectId_metaDataMeshCount = m_objectId.GetIndex();
                }

                // We're dealing with a new, uninitialized draw packet, so we need to initialize it
                // Ideally we defer the actual draw packet creation until later, but for now, just build it here
                if (!r_enableHardwareInstancing || index.m_wasInserted)
                {
                    // setup the mesh draw packet
                    RPI::MeshDrawPacket drawPacket(
                        modelLod,
                        meshIndex,
                        material,
                        meshObjectSrg,
                        materialAssignment.m_matModUvOverrides,
                        nullptr);

                    // set the shader option to select forward pass IBL specular if necessary
                    if (!drawPacket.SetShaderOption(s_o_meshUseForwardPassIBLSpecular_Name, AZ::RPI::ShaderOptionValue{ m_descriptor.m_useForwardPassIblSpecular }))
                    {
                        AZ_Warning("MeshDrawPacket", false, "Failed to set o_meshUseForwardPassIBLSpecular on mesh draw packet");
                    }

                    drawPacket.SetStencilRef(stencilRef);
                    drawPacket.SetSortKey(m_sortKey);
                    drawPacket.Update(*m_scene, false);

                    if (!r_enableHardwareInstancing)
                    {
                        m_drawPacketListsByLod[modelLodIndex].emplace_back(AZStd::move(drawPacket));
                    }
                    else
                    {
                        MeshInstanceData& instanceData = meshInstanceManager[index.m_handle];
                        instanceData.m_drawPacket = drawPacket;
                        // We're going to need an index for m_instanceOffset every frame for each draw item, so cache those here
                        for (auto& drawSrg : instanceData.m_drawPacket.GetDrawSrgs())
                        {
                            RHI::ShaderInputConstantIndex drawSrgIndexInstanceOffsetIndex =
                                drawSrg->FindShaderInputConstantIndex(s_m_instanceDataOffset_Name);

                            instanceData.m_drawSrgInstanceDataIndices.push_back(drawSrgIndexInstanceOffsetIndex);
                        }

                        const RPI::MeshDrawPacket::RootConstantsLayoutList& rootConstantsLayouts =
                            instanceData.m_drawPacket.GetRootConstantsLayouts();

                        if (!rootConstantsLayouts.empty())
                        {
                            // Get the root constant layout
                            RHI::ShaderInputConstantIndex shaderInputIndex =
                                rootConstantsLayouts[0]->FindShaderInputIndex(s_m_rootConstantInstanceDataOffset_Name);

                            if (shaderInputIndex.IsValid())
                            {
                                RHI::Interval interval = rootConstantsLayouts[0]->GetInterval(shaderInputIndex);
                                instanceData.m_drawRootConstantInterval = interval;
                            }
                            else
                            {
                                instanceData.m_drawRootConstantInterval = RHI::Interval{};
                            }
                        }
                    }
                }
            }
        }

        void ModelDataInstance::SetRayTracingData(RayTracingFeatureProcessor* rayTracingFeatureProcessor, TransformServiceFeatureProcessor* transformServiceFeatureProcessor)
        {
            RemoveRayTracingData(rayTracingFeatureProcessor);

            if (!m_model)
            {
                return;
            }

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
                    propertyIndex = material->FindPropertyIndex(s_baseColor_color_Name);
                    if (propertyIndex.IsValid())
                    {
                        subMesh.m_baseColor = material->GetPropertyValue<AZ::Color>(propertyIndex);
                    }

                    propertyIndex = material->FindPropertyIndex(s_baseColor_factor_Name);
                    if (propertyIndex.IsValid())
                    {
                        subMesh.m_baseColor *= material->GetPropertyValue<float>(propertyIndex);
                    }

                    // metallic
                    propertyIndex = material->FindPropertyIndex(s_metallic_factor_Name);
                    if (propertyIndex.IsValid())
                    {
                        subMesh.m_metallicFactor = material->GetPropertyValue<float>(propertyIndex);
                    }

                    // roughness
                    propertyIndex = material->FindPropertyIndex(s_roughness_factor_Name);
                    if (propertyIndex.IsValid())
                    {
                        subMesh.m_roughnessFactor = material->GetPropertyValue<float>(propertyIndex);
                    }

                    // emissive color
                    propertyIndex = material->FindPropertyIndex(s_emissive_enable_Name);
                    if (propertyIndex.IsValid())
                    {
                        if (material->GetPropertyValue<bool>(propertyIndex))
                        {
                            propertyIndex = material->FindPropertyIndex(s_emissive_color_Name);
                            if (propertyIndex.IsValid())
                            {
                                subMesh.m_emissiveColor = material->GetPropertyValue<AZ::Color>(propertyIndex);
                            }

                            propertyIndex = material->FindPropertyIndex(s_emissive_intensity_Name);
                            if (propertyIndex.IsValid())
                            {
                                subMesh.m_emissiveColor *= material->GetPropertyValue<float>(propertyIndex);
                            }
                        }
                    }

                    // textures
                    Data::Instance<RPI::Image> baseColorImage; // can be used for irradiance color below
                    propertyIndex = material->FindPropertyIndex(s_baseColor_textureMap_Name);
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

                    propertyIndex = material->FindPropertyIndex(s_normal_textureMap_Name);
                    if (propertyIndex.IsValid())
                    {
                        Data::Instance<RPI::Image> image = material->GetPropertyValue<Data::Instance<RPI::Image>>(propertyIndex);
                        if (image.get())
                        {
                            subMesh.m_textureFlags |= RayTracingSubMeshTextureFlags::Normal;
                            subMesh.m_normalImageView = image->GetImageView();
                        }
                    }

                    propertyIndex = material->FindPropertyIndex(s_metallic_textureMap_Name);
                    if (propertyIndex.IsValid())
                    {
                        Data::Instance<RPI::Image> image = material->GetPropertyValue<Data::Instance<RPI::Image>>(propertyIndex);
                        if (image.get())
                        {
                            subMesh.m_textureFlags |= RayTracingSubMeshTextureFlags::Metallic;
                            subMesh.m_metallicImageView = image->GetImageView();
                        }
                    }

                    propertyIndex = material->FindPropertyIndex(s_roughness_textureMap_Name);
                    if (propertyIndex.IsValid())
                    {
                        Data::Instance<RPI::Image> image = material->GetPropertyValue<Data::Instance<RPI::Image>>(propertyIndex);
                        if (image.get())
                        {
                            subMesh.m_textureFlags |= RayTracingSubMeshTextureFlags::Roughness;
                            subMesh.m_roughnessImageView = image->GetImageView();
                        }
                    }

                    propertyIndex = material->FindPropertyIndex(s_emissive_textureMap_Name);
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

            AZ::Transform transform = transformServiceFeatureProcessor->GetTransformForId(m_objectId);
            AZ::Vector3 nonUniformScale = transformServiceFeatureProcessor->GetNonUniformScaleForId(m_objectId);

            rayTracingFeatureProcessor->AddMesh(m_rayTracingUuid, m_model->GetModelAsset()->GetId(), subMeshes, transform, nonUniformScale);
            m_needsSetRayTracingData = false;
        }

        void ModelDataInstance::SetIrradianceData(
            RayTracingFeatureProcessor::SubMesh& subMesh,
            const Data::Instance<RPI::Material> material,
            const Data::Instance<RPI::Image> baseColorImage)
        {
            RPI::MaterialPropertyIndex propertyIndex = material->FindPropertyIndex(s_irradiance_irradianceColorSource_Name);
            if (!propertyIndex.IsValid())
            {
                return;
            }

            uint32_t enumVal = material->GetPropertyValue<uint32_t>(propertyIndex);
            AZ::Name irradianceColorSource = material->GetMaterialPropertiesLayout()->GetPropertyDescriptor(propertyIndex)->GetEnumName(enumVal);

            if (irradianceColorSource.IsEmpty() || irradianceColorSource == s_Manual_Name)
            {
                propertyIndex = material->FindPropertyIndex(s_irradiance_manualColor_Name);
                if (propertyIndex.IsValid())
                {
                    subMesh.m_irradianceColor = material->GetPropertyValue<AZ::Color>(propertyIndex);
                }
                else
                {
                    // Couldn't find irradiance.manualColor -> check for an irradiance.color in case the material type
                    // doesn't have the concept of manual vs. automatic irradiance color, allow a simpler property name
                    propertyIndex = material->FindPropertyIndex(s_irradiance_color_Name);
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
            else if (irradianceColorSource == s_BaseColorTint_Name)
            {
                // Use only the baseColor, no texture on top of it
                subMesh.m_irradianceColor = subMesh.m_baseColor;
            }
            else if (irradianceColorSource == s_BaseColor_Name)
            {
                // Check if texturing is enabled
                bool useTexture;
                propertyIndex = material->FindPropertyIndex(s_baseColor_useTexture_Name);
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
                        propertyIndex = material->FindPropertyIndex(s_baseColor_textureBlendMode_Name);
                        if (propertyIndex.IsValid())
                        {
                            AZ::Name textureBlendMode = material->GetMaterialPropertiesLayout()
                                     ->GetPropertyDescriptor(propertyIndex)
                                     ->GetEnumName(material->GetPropertyValue<uint32_t>(propertyIndex));
                            if (textureBlendMode != s_Multiply_Name)
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
            propertyIndex = material->FindPropertyIndex(s_irradiance_factor_Name);
            if (propertyIndex.IsValid())
            {
                subMesh.m_irradianceColor *= material->GetPropertyValue<float>(propertyIndex);
            }

            // set the raytracing transparency from the material opacity factor
            float opacity = 1.0f;
            propertyIndex = material->FindPropertyIndex(s_opacity_mode_Name);
            if (propertyIndex.IsValid())
            {
                // only query the opacity factor if it's a non-Opaque mode
                uint32_t mode = material->GetPropertyValue<uint32_t>(propertyIndex);
                if (mode > 0)
                {
                    propertyIndex = material->FindPropertyIndex(s_opacity_factor_Name);
                    if (propertyIndex.IsValid())
                    {
                        opacity = material->GetPropertyValue<float>(propertyIndex);
                    }
                }
            }

            subMesh.m_irradianceColor.SetA(opacity);
        }

        void ModelDataInstance::RemoveRayTracingData(RayTracingFeatureProcessor* rayTracingFeatureProcessor)
        {
            // remove from ray tracing
            if (rayTracingFeatureProcessor)
            {
                rayTracingFeatureProcessor->RemoveMesh(m_rayTracingUuid);
            }
        }

        void ModelDataInstance::SetSortKey(
            MeshFeatureProcessor* meshFeatureProcessor,
            MeshFP::EndCullingData* endCullingData,
            MeshInstanceManager& meshInstanceManager, RayTracingFeatureProcessor* rayTracingFeatureProcessor, RHI::DrawItemSortKey sortKey)
        {
            RHI::DrawItemSortKey previousSortKey = m_sortKey;
            m_sortKey = sortKey;
            if (previousSortKey != m_sortKey)
            {
                if (!r_enableHardwareInstancing)
                {
                    for (auto& drawPacketList : m_drawPacketListsByLod)
                    {
                        for (auto& drawPacket : drawPacketList)
                        {
                            drawPacket.SetSortKey(sortKey);
                        }
                    }
                }
                else
                {
                    // If the ModelDataInstance has already been initialized
                    if (m_model && !m_needsInit)
                    {
                        // The sort key is part of the instance key, so we need to update the
                        // instance map since it has changed
                        Data::Instance<RPI::Model> model = m_model;

                        // DeInit/ReInit is overkill (destroys and re-creates ray-tracing data)
                        // but it works for now since SetSortKey is infrequent
                        DeInit(meshFeatureProcessor, endCullingData, meshInstanceManager, rayTracingFeatureProcessor);
                        QueueInit(model);
                    }
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

        void ModelDataInstance::BuildCullable(
            MeshFeatureProcessor* meshFeatureProcessor, MeshFP::EndCullingData* endCullingData, MeshInstanceManager& meshInstanceManager)
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
                size_t meshCount = lodAssets[lodIndex]->GetMeshes().size();
                for (size_t meshIndex = 0; meshIndex < meshCount; ++meshIndex)
                {
                    const RHI::DrawPacket* rhiDrawPacket = nullptr;
                    if (!r_enableHardwareInstancing)
                    {
                        rhiDrawPacket = m_drawPacketListsByLod[lodIndex][meshIndex].GetRHIDrawPacket();
                    }
                    else
                    {
                        uint32_t meshDataIndex = endCullingData->m_instanceIndicesByLod[lodIndex].m_startIndex + static_cast<uint32_t>(meshIndex);
                        rhiDrawPacket = meshInstanceManager[meshFeatureProcessor->m_meshData[meshDataIndex].m_instanceGroupHandle_metaDataMeshOffset].m_drawPacket.GetRHIDrawPacket();
                    }

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

        void ModelDataInstance::UpdateCullBounds(
            MeshFeatureProcessor* meshFeatureProcessor,
            [[maybe_unused]] MeshFP::EndCullingData* endCullingData,
            const TransformServiceFeatureProcessor* transformService)
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
            if (!r_enableHardwareInstancing)
            {
                // Let the culling system submit the work
                m_cullable.m_cullData.m_visibilityEntry.m_typeFlags = AzFramework::VisibilityEntry::TYPE_RPI_Cullable;
            }
            else
            {
                // Let the MeshFeatureProcessor submit the work
                m_cullable.m_cullData.m_visibilityEntry.m_typeFlags = AzFramework::VisibilityEntry::TYPE_RPI_Visibility_List;
                m_cullable.m_cullData.m_optionalMeshFeatureProcessorData = &meshFeatureProcessor->m_meshData[m_meshDataIndices.m_startIndex];
            }
            m_scene->GetCullingScene()->RegisterOrUpdateCullable(m_cullable);

            m_cullBoundsNeedsUpdate = false;
        }

        void ModelDataInstance::UpdateObjectSrg(ReflectionProbeFeatureProcessor* reflectionProbeFeatureProcessor, TransformServiceFeatureProcessor* transformServiceFeatureProcessor)
        {
            for (auto& objectSrg : m_objectSrgList)
            {
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
            bool requiresForwardPassIbl = false;

            // look for a shader that has the o_materialUseForwardPassIBLSpecular option set
            // Note: this should be changed to have the material automatically set the forwardPassIBLSpecular
            // property and look for that instead of the shader option.
            // [GFX TODO][ATOM-5040] Address Property Metadata Feedback Loop
            material->ForAllShaderItems(
                [&](const Name&, const RPI::ShaderCollection::Item& shaderItem)
                {
                    if (shaderItem.IsEnabled())
                    {
                        RPI::ShaderOptionIndex index = shaderItem.GetShaderOptionGroup().GetShaderOptionLayout()->FindShaderOptionIndex(Name{"o_materialUseForwardPassIBLSpecular"});
                        if (index.IsValid())
                        {
                            RPI::ShaderOptionValue value = shaderItem.GetShaderOptionGroup().GetValue(Name{"o_materialUseForwardPassIBLSpecular"});
                            if (value.GetIndex() == 1)
                            {
                                requiresForwardPassIbl = true;
                                return false; // break
                            }
                        }
                    }

                    return true; // continue
                });

            return requiresForwardPassIbl;
        }

        void ModelDataInstance::SetVisible(bool isVisible)
        {
            m_visible = isVisible;
            m_cullable.m_isHidden = !isVisible;
        }

        void ModelDataInstance::UpdateMaterialChangeIds()
        {
            // update the material changeId list with the current material assignments
            m_materialChangeIds.clear();

            for (const auto& materialAssignment : m_materialAssignments)
            {
                const AZ::Data::Instance<RPI::Material>& materialInstance = materialAssignment.second.m_materialInstance;
                if (materialInstance.get())
                {
                    m_materialChangeIds[materialInstance] = materialInstance->GetCurrentChangeId();
                }
            }
        }

        bool ModelDataInstance::CheckForMaterialChanges() const
        {
            // check for the same number of materials
            if (m_materialChangeIds.size() != m_materialAssignments.size())
            {
                return true;
            }

            // check for material changes using the changeId
            for (const auto& materialAssignment : m_materialAssignments)
            {
                const AZ::Data::Instance<RPI::Material>& materialInstance = materialAssignment.second.m_materialInstance;

                MaterialChangeIdMap::const_iterator it = m_materialChangeIds.find(materialInstance);
                if (it == m_materialChangeIds.end() || it->second != materialInstance->GetCurrentChangeId())
                {
                    return true;
                }
            }

            return false;
        }

        void ModelDataInstance::OnRebuildMaterialInstance()
        {
            if (m_visible && m_descriptor.m_isRayTracingEnabled)
            {
                // If we already know we need an update, don't bother checking for material changes
                // This way we're not calling CheckForMaterialChanges multiple times
                // in the same frame for the same object
                if (m_needsSetRayTracingData || CheckForMaterialChanges())
                {
                    m_needsSetRayTracingData = true;

                    // update the material changeId list with the latest materials
                    UpdateMaterialChangeIds();
                }
            }
        }
    } // namespace Render
} // namespace AZ
