/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Mesh/MeshFeatureProcessor.h>
#include <Mesh/StreamBufferViewsBuilder.h>
#include <Atom/Feature/CoreLights/PhotometricValue.h>
#include <Atom/Feature/Material/ConvertEmissiveUnitFunctor.h>
#include <Atom/Feature/Mesh/MeshCommon.h>
#include <Atom/Feature/Mesh/ModelReloaderSystemInterface.h>
#include <Atom/Feature/RenderCommon.h>
#include <Atom/Feature/Utils/GpuBufferHandler.h>
#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/RHIUtils.h>
#include <Atom/RPI.Public/AssetQuality.h>
#include <Atom/RPI.Public/Culling.h>
#include <Atom/RPI.Public/Model/ModelLodUtils.h>
#include <Atom/RPI.Public/Model/ModelTagSystemComponent.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>

#include <Atom/Utils/StableDynamicArray.h>
#include <ReflectionProbe/ReflectionProbeFeatureProcessor.h>

#include <Atom/RPI.Reflect/Model/ModelAssetCreator.h>

#include <AzFramework/Asset/AssetSystemBus.h>

#include <AtomCore/Instance/InstanceDatabase.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Jobs/Algorithms.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Math/ShapeIntersection.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <algorithm>

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
        static AZ::Name s_emissive_unit_Name = AZ::Name::FromStringLiteral("emissive.unit", AZ::Interface<AZ::NameDictionary>::Get());
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
        static AZ::Name s_o_meshInstancingIsEnabled_Name =
            AZ::Name::FromStringLiteral("o_meshInstancingIsEnabled", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_transparent_Name = AZ::Name::FromStringLiteral("transparent", AZ::Interface<AZ::NameDictionary>::Get());
        static AZ::Name s_block_silhouette_Name = AZ::Name::FromStringLiteral("silhouette.blockSilhouette", AZ::Interface<AZ::NameDictionary>::Get());

        static ModelDataInstance& ToModelDataInstance(const MeshFeatureProcessorInterface::MeshHandle& meshHandle)
        {
            return *azrtti_cast<ModelDataInstance*>(&*meshHandle);
        }

        static void CacheRootConstantInterval(MeshInstanceGroupData& meshInstanceGroupData)
        {
            meshInstanceGroupData.m_drawRootConstantOffset = 0;

            RHI::ConstPtr<RHI::ConstantsLayout> rootConstantsLayout = meshInstanceGroupData.m_drawPacket.GetRootConstantsLayout();
            if (rootConstantsLayout)
            {
                // Get the root constant layout
                RHI::ShaderInputConstantIndex shaderInputIndex =
                    rootConstantsLayout->FindShaderInputIndex(s_m_rootConstantInstanceDataOffset_Name);

                if (shaderInputIndex.IsValid())
                {
                    RHI::Interval interval = rootConstantsLayout->GetInterval(shaderInputIndex);
                    meshInstanceGroupData.m_drawRootConstantOffset = interval.m_min;
                }
            }
        }

        void MeshFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<MeshFeatureProcessor, FeatureProcessor>()->Version(1);
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
            m_meshMotionDrawListTag = AZ::RHI::RHISystemInterface::Get()->GetDrawListTagRegistry()->AcquireTag(MeshCommon::MotionDrawListTagName);
            m_transparentDrawListTag = AZ::RHI::RHISystemInterface::Get()->GetDrawListTagRegistry()->AcquireTag(s_transparent_Name);

            if (auto* console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
            {
                console->GetCvarValue("r_meshInstancingEnabled", m_enableMeshInstancing);

                // push the cvars value so anything in this dll can access it directly.
                console->PerformCommand(
                    AZStd::string::format("r_meshInstancingEnabled %s", m_enableMeshInstancing ? "true" : "false")
                        .c_str());

                console->GetCvarValue("r_meshInstancingEnabledForTransparentObjects", m_enableMeshInstancingForTransparentObjects);

                // push the cvars value so anything in this dll can access it directly.
                console->PerformCommand(
                    AZStd::string::format(
                        "r_meshInstancingEnabledForTransparentObjects %s", m_enableMeshInstancingForTransparentObjects ? "true" : "false")
                        .c_str());

                size_t meshInstancingBucketSortScatterBatchSize;
                console->GetCvarValue("r_meshInstancingBucketSortScatterBatchSize", meshInstancingBucketSortScatterBatchSize);

                // push the cvars value so anything in this dll can access it directly.
                console->PerformCommand(
                    AZStd::string::format(
                        "r_meshInstancingBucketSortScatterBatchSize %zu", meshInstancingBucketSortScatterBatchSize)
                        .c_str());
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
            m_rayTracingFeatureProcessor = nullptr;
            m_reflectionProbeFeatureProcessor = nullptr;
            m_forceRebuildDrawPackets = false;

            GetParentScene()->GetViewTagBitRegistry().ReleaseTag(m_meshMovedFlag);
            RHI::RHISystemInterface::Get()->GetDrawListTagRegistry()->ReleaseTag(m_meshMotionDrawListTag);
            RHI::RHISystemInterface::Get()->GetDrawListTagRegistry()->ReleaseTag(m_transparentDrawListTag);
        }

        TransformServiceFeatureProcessorInterface::ObjectId MeshFeatureProcessor::GetObjectId(const MeshHandle& meshHandle) const
        {
            if (meshHandle.IsValid())
            {
                return ToModelDataInstance(meshHandle).m_objectId;
            }

            return TransformServiceFeatureProcessorInterface::ObjectId::Null;
        }

        void MeshFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "MeshFeatureProcessor: Simulate");

            AZ::Job* parentJob = packet.m_parentJob;
            AZStd::concurrency_check_scope scopeCheck(m_meshDataChecker);

            // If the instancing cvar has changed, we need to re-initalize the ModelDataInstances
            CheckForInstancingCVarChange();

            AZStd::vector<Job*> initJobQueue = CreateInitJobQueue();
            AZStd::vector<Job*> updateCullingJobQueue = CreateUpdateCullingJobQueue();

            if (!r_meshInstancingEnabled)
            {
                // There's no need for all the init jobs to finish before any of the update culling jobs are run.
                // Any update culling job can run once it's corresponding init job is done. So instead of separating the jobs
                // entirely, use individual job dependencies to synchronize them. This performs better than having a big sync between them
                ExecuteCombinedJobQueue(initJobQueue, updateCullingJobQueue, parentJob);
            }
            else
            {

                ExecuteSimulateJobQueue(initJobQueue, parentJob);
                // Per-InstanceGroup work must be done after the Init jobs are complete, because the init jobs will determine which instance
                // group each mesh belongs to and populate those instance groups
                // Note: the Per-InstanceGroup jobs need to be created after init jobs because it's possible new instance groups are created in init jobs
                AZStd::vector<Job*> perInstanceGroupJobQueue = CreatePerInstanceGroupJobQueue();
                ExecuteSimulateJobQueue(perInstanceGroupJobQueue, parentJob);
                // Updating the culling scene must happen after the per-instance group work is done
                // because the per-instance group work will update the draw packets.
                ExecuteSimulateJobQueue(updateCullingJobQueue, parentJob);
            }

            m_forceRebuildDrawPackets = false;
        }

        void MeshFeatureProcessor::CheckForInstancingCVarChange()
        {
            if (m_enableMeshInstancing != r_meshInstancingEnabled || m_enableMeshInstancingForTransparentObjects != r_meshInstancingEnabledForTransparentObjects)
            {
                // DeInit and re-init every object
                for (auto& modelDataInstance : m_modelData)
                {
                    modelDataInstance.ReInit(this);
                }
                m_enableMeshInstancing = r_meshInstancingEnabled;
                m_enableMeshInstancingForTransparentObjects = r_meshInstancingEnabledForTransparentObjects;
            }
        }

        AZStd::vector<Job*> MeshFeatureProcessor::CreatePerInstanceGroupJobQueue()
        {
            const auto instanceManagerRanges = m_meshInstanceManager.GetParallelRanges();
            AZStd::vector<Job*> perInstanceGroupJobQueue;
            perInstanceGroupJobQueue.reserve(instanceManagerRanges.size());
            RPI::Scene* scene = GetParentScene();
            for (const auto& iteratorRange : instanceManagerRanges)
            {
                const auto perInstanceGroupJobLambda = [this, scene, iteratorRange]() -> void
                {
                    AZ_PROFILE_SCOPE(AzRender, "MeshFeatureProcessor: Simulate: PerInstanceGroupUpdate");
                    for (auto instanceGroupDataIter = iteratorRange.m_begin; instanceGroupDataIter != iteratorRange.m_end;
                         ++instanceGroupDataIter)
                    {
                        if (instanceGroupDataIter->UpdateDrawPacket(*scene, m_forceRebuildDrawPackets))
                        {
                            // We're going to need an interval for the root constant data that we update every frame for each draw item, so
                            // cache that here
                            CacheRootConstantInterval(*instanceGroupDataIter);
                        }
                    }
                };
                Job* executePerInstanceGroupJob =
                    aznew JobFunction<decltype(perInstanceGroupJobLambda)>(perInstanceGroupJobLambda, true, nullptr); // Auto-deletes
                perInstanceGroupJobQueue.push_back(executePerInstanceGroupJob);
            }
            return perInstanceGroupJobQueue;
        }

        AZStd::vector<Job*> MeshFeatureProcessor::CreateInitJobQueue()
        {
            const auto iteratorRanges = m_modelData.GetParallelRanges();
            AZStd::vector<Job*> initJobQueue;
            initJobQueue.reserve(iteratorRanges.size());
            bool removePerMeshShaderOptionFlags = !r_enablePerMeshShaderOptionFlags && m_enablePerMeshShaderOptionFlags;
            for (const auto& iteratorRange : iteratorRanges)
            {
                const auto initJobLambda = [this, iteratorRange, removePerMeshShaderOptionFlags]() -> void
                {
                    AZ_PROFILE_SCOPE(AzRender, "MeshFeatureProcessor: Simulate: Init");

                    for (auto modelDataIter = iteratorRange.m_begin; modelDataIter != iteratorRange.m_end; ++modelDataIter)
                    {
                        if (!modelDataIter->m_model)
                        {
                            continue; // model not loaded yet
                        }

                        if (!modelDataIter->m_flags.m_visible)
                        {
                            continue;
                        }

                        if (modelDataIter->m_flags.m_needsInit)
                        {
                            modelDataIter->Init(this);
                        }

                        if (modelDataIter->m_flags.m_objectSrgNeedsUpdate)
                        {
                            modelDataIter->UpdateObjectSrg(this);
                        }

                        if (modelDataIter->m_flags.m_needsSetRayTracingData)
                        {
                            modelDataIter->SetRayTracingData(this);
                        }

                        // If instancing is enabled, the draw packets will be updated by the per-instance group jobs,
                        // so they don't need to be updated here
                        if (!r_meshInstancingEnabled)
                        {
                            // Unset per mesh shader options
                            if (removePerMeshShaderOptionFlags)
                            {
                                for (RPI::MeshDrawPacketList& drawPacketList : modelDataIter->m_meshDrawPacketListsByLod)
                                {
                                    for (RPI::MeshDrawPacket& drawPacket : drawPacketList)
                                    {
                                        m_flagRegistry->VisitTags(
                                            [&](AZ::Name shaderOption, [[maybe_unused]] FlagRegistry::TagType tag)
                                            {
                                                drawPacket.UnsetShaderOption(shaderOption);
                                            });
                                    }
                                }

                                modelDataIter->m_cullable.m_shaderOptionFlags = 0;
                                modelDataIter->m_cullable.m_prevShaderOptionFlags = 0;
                            }

                            // [GFX TODO] [ATOM-1357] Currently all of the draw packets have to be checked for material ID changes because
                            // material properties can impact which actual shader is used, which impacts the SRG in the draw packet.
                            // This is scheduled to be optimized so the work is only done on draw packets that need it instead of having
                            // to check every one.
                            modelDataIter->UpdateDrawPackets(m_forceRebuildDrawPackets);
                        }
                    }
                };
                Job* executeInitJob = aznew JobFunction<decltype(initJobLambda)>(initJobLambda, true, nullptr); // Auto-deletes
                initJobQueue.push_back(executeInitJob);
            }
            return initJobQueue;
        }

        AZStd::vector<Job*> MeshFeatureProcessor::CreateUpdateCullingJobQueue()
        {
            const auto iteratorRanges = m_modelData.GetParallelRanges();
            AZStd::vector<Job*> updateCullingJobQueue;
            updateCullingJobQueue.reserve(iteratorRanges.size());

            for (const auto& iteratorRange : iteratorRanges)
            {
                const auto updateCullingJobLambda = [this, iteratorRange]() -> void
                {
                    AZ_PROFILE_SCOPE(AzRender, "MeshFeatureProcessor: Simulate: UpdateCulling");

                    for (auto meshDataIter = iteratorRange.m_begin; meshDataIter != iteratorRange.m_end; ++meshDataIter)
                    {
                        if (!meshDataIter->m_model)
                        {
                            continue; // model not loaded yet
                        }

                        if (meshDataIter->m_flags.m_cullableNeedsRebuild)
                        {
                            meshDataIter->BuildCullable();
                        }

                        if (meshDataIter->m_flags.m_cullBoundsNeedsUpdate)
                        {
                            meshDataIter->UpdateCullBounds(this);
                        }
                    }
                };
                Job* executeUpdateGroupJob =
                    aznew JobFunction<decltype(updateCullingJobLambda)>(updateCullingJobLambda, true, nullptr); // Auto-deletes
                updateCullingJobQueue.push_back(executeUpdateGroupJob);
            }
            return updateCullingJobQueue;
        }

        void MeshFeatureProcessor::ExecuteCombinedJobQueue(AZStd::span<Job*> initQueue, AZStd::span<Job*> updateCullingQueue, Job* parentJob)
        {
            AZ::JobCompletion jobCompletion;
            for (size_t i = 0; i < initQueue.size(); ++i)
            {
                // Update Culling work should happen after Init is done
                initQueue[i]->SetDependent(updateCullingQueue[i]);

                // FeatureProcessor::Simulate is optionally run with a parent job.
                if (parentJob)
                {
                    // When a parent job is used, we set dependencies on it and use WaitForChildren to wait for them to finish executing
                    parentJob->StartAsChild(updateCullingQueue[i]);
                    initQueue[i]->Start();
                }
                else
                {
                    // When a parent job is not used, we use a job completion to synchronize
                    updateCullingQueue[i]->SetDependent(&jobCompletion);
                    initQueue[i]->Start();
                    updateCullingQueue[i]->Start();
                }
            }

            if (parentJob)
            {
                parentJob->WaitForChildren();
            }
            else
            {
                jobCompletion.StartAndWaitForCompletion();
            }
        }

        void MeshFeatureProcessor::ExecuteSimulateJobQueue(AZStd::span<Job*> jobQueue, Job* parentJob)
        {
            AZ::JobCompletion jobCompletion;
            for (Job* childJob : jobQueue)
            {
                // FeatureProcessor::Simulate is optionally run with a parent job.
                if (parentJob)
                {
                    // When a parent job is used, we set dependencies on it and use WaitForChildren to wait for them to finish executing
                    parentJob->StartAsChild(childJob);
                }
                else
                {
                    // When a parent job is not used, we use a job completion to synchronize
                    childJob->SetDependent(&jobCompletion);
                    childJob->Start();
                }
            }

            if (parentJob)
            {
                parentJob->WaitForChildren();
            }
            else
            {
                jobCompletion.StartAndWaitForCompletion();
            }
        }

        void MeshFeatureProcessor::OnEndCulling(const MeshFeatureProcessor::RenderPacket& packet)
        {
            if (r_meshInstancingEnabled)
            {
                AZ_PROFILE_SCOPE(RPI, "MeshFeatureProcessor: OnEndCulling");

                // If necessary, allocate memory up front for the work that needs to be done this frame
                ResizePerViewInstanceVectors(packet.m_views.size());

                {
                    // Iterate over all of the visible objects for each view, and perform the first stage of the bucket sort
                    // where each visible object is sorted into its bucket
                    AZ_PROFILE_SCOPE(RPI, "MeshFeatureProcessor: Add Visible Objects to Buckets");
                    AZ::TaskGraphEvent addVisibleObjectsToBucketsTGEvent{ "AddVisibleObjectsToBuckets Wait" };
                    AZ::TaskGraph addVisibleObjectsToBucketsTG{ "AddVisibleObjectsToBuckets" };
                    for (size_t viewIndex = 0; viewIndex < packet.m_views.size(); ++viewIndex)
                    {
                        AddVisibleObjectsToBuckets(addVisibleObjectsToBucketsTG, viewIndex, packet.m_views[viewIndex]);
                    }

                    addVisibleObjectsToBucketsTG.Submit(&addVisibleObjectsToBucketsTGEvent);
                    addVisibleObjectsToBucketsTGEvent.Wait();
                }

                {
                    // Now that the buckets have been filled, create a task for each bucket to sort each individual bucket in parallel
                    AZ_PROFILE_SCOPE(RPI, "MeshFeatureProcessor: Sort Buckets");
                    AZ::TaskGraphEvent sortInstanceBufferBucketsTGEvent{ "SortInstanceBufferBuckets Wait" };
                    AZ::TaskGraph sortInstanceBufferBucketsTG{ "SortInstanceBufferBuckets" };
                    for (size_t viewIndex = 0; viewIndex < packet.m_views.size(); ++viewIndex)
                    {
                        SortInstanceBufferBuckets(sortInstanceBufferBucketsTG, viewIndex);
                    }

                    // submit the tasks
                    sortInstanceBufferBucketsTG.Submit(&sortInstanceBufferBucketsTGEvent);
                    sortInstanceBufferBucketsTGEvent.Wait();
                }

                {
                    // For each bucket, create a task to iterate over the instance buffer to calculate the offset and count
                    // to use with each instanced draw call, and add the draw calls to the view.
                    AZ_PROFILE_SCOPE(RPI, "MeshFeatureProcessor: Build Instance Buffer and Draw Calls");
                    AZ::TaskGraphEvent buildInstanceBufferTGEvent{ "BuildInstanceBuffer Wait" };
                    AZ::TaskGraph buildInstanceBufferTG{ "BuildInstanceBuffer" };
                    for (size_t viewIndex = 0; viewIndex < packet.m_views.size(); ++viewIndex)
                    {
                        BuildInstanceBufferAndDrawCalls(buildInstanceBufferTG, viewIndex, packet.m_views[viewIndex]);
                    }

                    // submit the tasks
                    buildInstanceBufferTG.Submit(&buildInstanceBufferTGEvent);
                    buildInstanceBufferTGEvent.Wait();
                }

                for (size_t viewIndex = 0; viewIndex < packet.m_views.size(); ++viewIndex)
                {
                    // Now that the per-view instance buffers are up to date on the CPU, update them on the GPU
                    UpdateGPUInstanceBufferForView(viewIndex, packet.m_views[viewIndex]);
                }
            }
        }

        void MeshFeatureProcessor::ResizePerViewInstanceVectors(size_t viewCount)
        {
            AZ_PROFILE_SCOPE(RPI, "MeshFeatureProcessor: ResizePerInstanceVectors");
            // Initialize the instance data if it hasn't been created yet
            if (m_perViewInstanceData.size() <= viewCount)
            {
                m_perViewInstanceData.resize(
                    viewCount, AZStd::vector<TransformServiceFeatureProcessorInterface::ObjectId>());
            }

            if (m_perViewInstanceGroupBuckets.size() <= viewCount)
            {
                m_perViewInstanceGroupBuckets.resize(viewCount, AZStd::vector<InstanceGroupBucket>());
            }

            // Initialize the buffer handler if it hasn't been created yet
            if (m_perViewInstanceDataBufferHandlers.size() <= viewCount)
            {
                GpuBufferHandler::Descriptor desc;
                desc.m_bufferName = "MeshInstanceDataBuffer";
                desc.m_bufferSrgName = "m_instanceData";
                desc.m_elementSize = sizeof(uint32_t);
                desc.m_srgLayout = RPI::RPISystemInterface::Get()->GetViewSrgLayout().get();

                m_perViewInstanceDataBufferHandlers.reserve(viewCount);
                while (m_perViewInstanceDataBufferHandlers.size() < viewCount)
                {
                    // We construct and add these one at a time instead of a single call to resize
                    // because copying a GpuBufferHandler will result in a new one that refers to the same buffer,
                    // and we want a unique GpuBufferHandler referring to a unique buffer for each view.
                    m_perViewInstanceDataBufferHandlers.push_back(GpuBufferHandler(desc));
                }
            }

            AZStd::vector<uint32_t> perBucketInstanceCounts;
            const auto instanceManagerRanges = m_meshInstanceManager.GetParallelRanges();
            if (instanceManagerRanges.size() > 0)
            {
                // Resize the per-bucket data vectors for every view
                for (AZStd::vector<InstanceGroupBucket>& perViewInstanceGroupBuckets : m_perViewInstanceGroupBuckets)
                {
                    // Get the max page index (bucket count) by looking at the index of the very last page
                    // This is slightly conservative, as the StableDynamicArray in the MeshInstanceManager will always
                    // increment the page count to get the index of a new page, but it will never decrement the page count
                    // it or re-use the index of and existing page after it is freed, so that could result in some extra buckets
                    // here that are ultimately unused. But the MeshInstanceManager is never releasing unused pages, so that won't
                    // be an issue.
                    uint32_t bucketCount = static_cast<uint32_t>(instanceManagerRanges.back().m_begin.GetPageIndex()) + 1;
                    perViewInstanceGroupBuckets.resize(bucketCount);
                    perBucketInstanceCounts.resize(bucketCount, 0);
                    for (InstanceGroupBucket& instanceGroupBucket : perViewInstanceGroupBuckets)
                    {
                        instanceGroupBucket.m_currentElementIndex = 0;
                        instanceGroupBucket.m_sortInstanceData.clear();
                    }
                }
            }
            else
            {
                // If there are no buckets, clear them
                for (AZStd::vector<InstanceGroupBucket>& perViewInstanceGroupBuckets : m_perViewInstanceGroupBuckets)
                {
                    perViewInstanceGroupBuckets.clear();
                }
            }

            for (const auto& iteratorRange : instanceManagerRanges)
            {
                uint32_t maxPossibleInstanceCountForGroup = 0;
                for (auto instanceGroupDataIter = iteratorRange.m_begin; instanceGroupDataIter != iteratorRange.m_end;
                     ++instanceGroupDataIter)
                {
                    // Resize the cloned draw packet vector so that there is a unique drawItem for each view
                    instanceGroupDataIter->m_perViewDrawPackets.resize(viewCount);
                    maxPossibleInstanceCountForGroup += instanceGroupDataIter->m_count;
                }
                perBucketInstanceCounts[iteratorRange.m_begin.GetPageIndex()] = maxPossibleInstanceCountForGroup;
            }

            // Resize the per-bucket data vectors for every view to allow for all possible objects to be visible
            for (size_t viewIndex = 0; viewIndex < viewCount; ++viewIndex)
            {
                AZStd::vector<InstanceGroupBucket>& currentViewInstanceGroupBuckets = m_perViewInstanceGroupBuckets[viewIndex];
                for (size_t bucketIndex = 0; bucketIndex < currentViewInstanceGroupBuckets.size(); ++bucketIndex)
                {
                    // Reserve enough memory to handle the case where all of the objects are visible
                    // We use resize_no_construct instead of reserve + push_back so that we can use an
                    // atomic index to insert the data lock-free from multiple threads.
                    uint32_t maxPossibleObjects = perBucketInstanceCounts[bucketIndex];
                    currentViewInstanceGroupBuckets[bucketIndex].m_sortInstanceData.resize_no_construct(maxPossibleObjects);
                }
            }
        }


        void MeshFeatureProcessor::SetLightingChannelMask(const MeshHandle& meshHandle, uint32_t lightingChannelMask)
        {
            if (meshHandle.IsValid())
            {
                ToModelDataInstance(meshHandle).SetLightingChannelMask(lightingChannelMask);
            }
        }

        uint32_t MeshFeatureProcessor::GetLightingChannelMask(const MeshHandle& meshHandle) const
        {
            if (meshHandle.IsValid())
            {
                return meshHandle->GetLightingChannelMask();
            }
            else
            {
                AZ_Assert(false, "Invalid mesh handle");
                return 1;
            }
        }


        void MeshFeatureProcessor::AddVisibleObjectsToBuckets(
            TaskGraph& addVisibleObjectsToBucketsTG, size_t viewIndex, const RPI::ViewPtr& view)
        {
            AZ_PROFILE_SCOPE(RPI, "MeshFeatureProcessor: AddVisibleObjectsToBuckets");
            size_t visibleObjectCount = view->GetVisibleObjectList().size();

            AZStd::vector<TransformServiceFeatureProcessorInterface::ObjectId>& perViewInstanceData = m_perViewInstanceData[viewIndex];
            if (visibleObjectCount > 0)
            {
                perViewInstanceData.clear();

                static const AZ::TaskDescriptor addVisibleObjectsToBucketsTaskDescriptor{
                    "AZ::Render::MeshFeatureProcessor::OnEndCulling - AddVisibleObjectsToBuckets", "Graphics"
                };

                size_t batchSize = r_meshInstancingBucketSortScatterBatchSize;
                size_t batchCount = AZ::DivideAndRoundUp(visibleObjectCount, batchSize);

                for (size_t batchIndex = 0; batchIndex < batchCount; ++batchIndex)
                {
                    size_t batchStart = batchIndex * batchSize;
                    // If we're in the last batch, we just get the remaining objects
                    size_t currentBatchCount = batchIndex == batchCount - 1 ? visibleObjectCount - batchStart : batchSize;

                    addVisibleObjectsToBucketsTG.AddTask(
                        addVisibleObjectsToBucketsTaskDescriptor,
                        [this, view, viewIndex, batchStart, currentBatchCount]()
                        {
                            RPI::VisibleObjectListView visibilityList = view->GetVisibleObjectList();
                            AZStd::vector<InstanceGroupBucket>& currentViewInstanceGroupBuckets = m_perViewInstanceGroupBuckets[viewIndex];
                            for (size_t i = batchStart; i < batchStart + currentBatchCount; ++i)
                            {
                                const RPI::VisibleObjectProperties& visibleObject = visibilityList[i];
                                const ModelDataInstance::PostCullingInstanceDataList* postCullingInstanceDataList =
                                    static_cast<const ModelDataInstance::PostCullingInstanceDataList*>(visibleObject.m_userData);

                                for (const ModelDataInstance::PostCullingInstanceData& postCullingData : *postCullingInstanceDataList)
                                {
                                    SortInstanceData instanceData;
                                    instanceData.m_instanceGroupHandle = postCullingData.m_instanceGroupHandle;
                                    instanceData.m_objectId = postCullingData.m_objectId;
                                    instanceData.m_depth = visibleObject.m_depth;

                                    // Sort transparent objects in reverse by making their depths negative.
                                    if (instanceData.m_instanceGroupHandle->m_isTransparent)
                                    {
                                        instanceData.m_depth *= -1.0f;
                                    }

                                    // Add the sort data to the bucket
                                    InstanceGroupBucket& instanceGroupBucket =
                                        currentViewInstanceGroupBuckets[postCullingData.m_instanceGroupPageIndex];
                                    // Use an atomic operation to determine where to insert this sort data
                                    uint32_t currentIndex = instanceGroupBucket.m_currentElementIndex++;
                                    instanceGroupBucket.m_sortInstanceData[currentIndex] = instanceData;
                                }
                            }
                        });
                }
            }
        }

        void MeshFeatureProcessor::SortInstanceBufferBuckets(TaskGraph& sortInstanceBufferBucketsTG, size_t viewIndex)
        {
            AZ_PROFILE_SCOPE(RPI, "MeshFeatureProcessor: SortInstanceBufferBuckets");
            AZStd::vector<InstanceGroupBucket>& currentViewInstanceGroupBuckets = m_perViewInstanceGroupBuckets[viewIndex];

            // Populate a task graph where each task is responsible for sorting a bucket.
            static const AZ::TaskDescriptor sortInstanceBufferBucketsTaskDescriptor{
                "AZ::Render::MeshFeatureProcessor::OnEndCulling - sort instance data buckets", "Graphics"
            };

            for (InstanceGroupBucket& instanceGroupBucket : currentViewInstanceGroupBuckets)
            {
                // We're creating one task per bucket here. That is ideal when the buckets are all close to the same size,
                // but it can lead to an imperfect distribution of work if one bucket has more objects than any of the others.
                // If this becomes a performance bottleneck, it could be alleviated by adding an heuristic to sort any overfull
                // buckets using a parallel std sort rather than using a single task, or by breaking it up into smaller buckets.
                sortInstanceBufferBucketsTG.AddTask(
                    sortInstanceBufferBucketsTaskDescriptor,
                    [&instanceGroupBucket]()
                    {
                        // Note: we've previously resized m_sortInstanceData to conservatively fit all possible visible meshes for the bucket,
                        // which allowed us to use an atomic index for parallel lock free insertion.
                        // As a result, m_sortInstanceData it has a greater size than the actual count.
                        // We only care about the real visible objects, so cut off the last unused elements here
                        instanceGroupBucket.m_sortInstanceData.resize(instanceGroupBucket.m_currentElementIndex);

                        // Sort within the bucket
                        std::sort(instanceGroupBucket.m_sortInstanceData.begin(), instanceGroupBucket.m_sortInstanceData.end());
                    });
            }
        }

        static void AddInstancedDrawPacketToView(
            const RPI::ViewPtr& view,
            size_t viewIndex,
            ModelDataInstance::InstanceGroupHandle instanceGroupHandle,
            float accumulatedDepth,
            uint32_t instanceGroupBeginIndex,
            uint32_t instanceGroupEndNonInclusiveIndex)
        {
            MeshInstanceGroupData& instanceGroup = *instanceGroupHandle;

            // Each task is working on a page of instance groups, but
            // there is also one task per-view. So there may be multiple
            // threads accessing the intance group here, so we must use a lock to protect it.
            // We could potentially handle all views for a given bucket of instance groups
            // In a single task, which would negate the need to lock here
            if (instanceGroup.m_perViewDrawPackets.size() <= viewIndex)
            {
                AZStd::scoped_lock meshDataLock(instanceGroup.m_eventLock);
                instanceGroup.m_perViewDrawPackets.resize(viewIndex + 1);
            }

            // Cache a cloned drawpacket here
            if (!instanceGroup.m_perViewDrawPackets[viewIndex])
            {
                // Since there is only one task that will operate both on this view index and on the bucket with this instance group,
                // there is no need to lock here.
                RHI::DrawPacketBuilder drawPacketBuilder{RHI::MultiDevice::AllDevices};
                instanceGroup.m_perViewDrawPackets[viewIndex] = drawPacketBuilder.Clone(instanceGroup.m_drawPacket.GetRHIDrawPacket());
            }

            // Now that we have a valid cloned draw packet, update it with the latest offset + count
            RHI::Ptr<RHI::DrawPacket> clonedDrawPacket = instanceGroup.m_perViewDrawPackets[viewIndex];

            // Set the instance data offset
            AZStd::span<uint8_t> data{ reinterpret_cast<uint8_t*>(&instanceGroupBeginIndex), sizeof(uint32_t) };
            clonedDrawPacket->SetRootConstant(instanceGroup.m_drawRootConstantOffset, data);

            // instanceGroupEndNonInclusiveIndex is the first index after the current group ends.
            uint32_t instanceCount = instanceGroupEndNonInclusiveIndex - instanceGroupBeginIndex;

            // Set the cloned draw packet instance count
            clonedDrawPacket->SetInstanceCount(instanceCount);

            float averageDepth = accumulatedDepth / static_cast<float>(instanceCount);

            // Depth values from the camera are always positive.
            // However, we use negative values to sort by reverse depth for transparent objects
            // If the average depth is negative (this instance group is transparent), make it positive to get the real average depth for the group
            averageDepth = AZStd::abs(averageDepth);

            // Submit the draw packet
            view->AddDrawPacket(clonedDrawPacket.get(), averageDepth);
        }

        void MeshFeatureProcessor::BuildInstanceBufferAndDrawCalls(
            TaskGraph& buildInstanceBufferTG, size_t viewIndex, const RPI::ViewPtr& view)
        {
            AZStd::vector<TransformServiceFeatureProcessorInterface::ObjectId>& perViewInstanceData = m_perViewInstanceData[viewIndex];
            AZStd::vector<InstanceGroupBucket>& currentViewInstanceGroupBuckets = m_perViewInstanceGroupBuckets[viewIndex];

            uint32_t currentBatchStart = 0;
            for (InstanceGroupBucket& instanceGroupBucket : currentViewInstanceGroupBuckets)
            {
                if (instanceGroupBucket.m_currentElementIndex > 0)
                {
                    static const AZ::TaskDescriptor buildInstanceBufferTaskDescriptor{
                        "AZ::Render::MeshFeatureProcessor::OnEndCulling - process instance data", "Graphics"
                    };
                    // Process data up to but not including actualEndOffset
                    buildInstanceBufferTG.AddTask(
                        buildInstanceBufferTaskDescriptor,
                        [currentBatchStart,
                        viewIndex,
                        &view,
                        &perViewInstanceData, &instanceGroupBucket]()
                        {
                            ModelDataInstance::InstanceGroupHandle currentInstanceGroup =
                                instanceGroupBucket.m_sortInstanceData.begin()->m_instanceGroupHandle;
                            uint32_t instanceDataOffset = currentBatchStart;
                            float accumulatedDepth = 0.0f;
                            uint32_t instanceDataIndex = currentBatchStart;
                            for (SortInstanceData& sortInstanceData : instanceGroupBucket.m_sortInstanceData)
                            {
                                // Anytime the instance group changes, submit a draw for the previous group
                                if (sortInstanceData.m_instanceGroupHandle != currentInstanceGroup)
                                {
                                    AddInstancedDrawPacketToView(
                                        view, viewIndex, currentInstanceGroup, accumulatedDepth, instanceDataOffset, instanceDataIndex);

                                    // Update the loop trackers
                                    accumulatedDepth = 0.0f;
                                    instanceDataOffset = instanceDataIndex;
                                    currentInstanceGroup = sortInstanceData.m_instanceGroupHandle;
                                }
                                perViewInstanceData[instanceDataIndex] = sortInstanceData.m_objectId;
                                accumulatedDepth += sortInstanceData.m_depth;
                                instanceDataIndex++;
                            }

                            // Submit the last instance group
                            {
                                AddInstancedDrawPacketToView(
                                    view, viewIndex, currentInstanceGroup, accumulatedDepth, instanceDataOffset, instanceDataIndex);
                            }
                        });

                    // At this point, inserting into the bucket is already complete, so m_currentElementIndex represents the count of all visible meshes in this bucket.
                    currentBatchStart += instanceGroupBucket.m_currentElementIndex;
                }
            }

            // currentBatchStart now represents the total count of visible instances in this view.
            // Re-size the instance data buffer so that we can fill it with the tasks created above
            perViewInstanceData.resize_no_construct(currentBatchStart);
        }

        void MeshFeatureProcessor::UpdateGPUInstanceBufferForView(size_t viewIndex, const RPI::ViewPtr& view)
        {
            AZ_PROFILE_SCOPE(RPI, "MeshFeatureProcessor: UpdateGPUInstanceBufferForView");
            // Use the correct srg for the view
            GpuBufferHandler& instanceDataBufferHandler = m_perViewInstanceDataBufferHandlers[viewIndex];
            instanceDataBufferHandler.UpdateSrg(view->GetShaderResourceGroup().get());

            // Now that we have all of our instance data, we need to create the buffer and bind it to the view srgs
            // Eventually, this could be a transient buffer

            // create output buffer descriptors
            AZStd::vector<TransformServiceFeatureProcessorInterface::ObjectId>& perViewInstanceData = m_perViewInstanceData[viewIndex];
            instanceDataBufferHandler.UpdateBuffer(perViewInstanceData.data(), static_cast<uint32_t>(perViewInstanceData.size()));
        }

        void MeshFeatureProcessor::OnBeginPrepareRender()
        {
            m_meshDataChecker.soft_lock();

            // The per-mesh shader option flags are set in feature processors' simulate function
            // So we want to process the flags here to update the draw packets if needed.
            // Update MeshDrawPacket's shader options if PerMeshShaderOption is enabled
            if (r_enablePerMeshShaderOptionFlags || m_enablePerMeshShaderOptionFlags)
            {
                // For mesh instance groups when r_meshInstancingEnabled is enabled
                AZStd::vector<ModelDataInstance::InstanceGroupHandle> instanceGroupsNeedUpdate;

                // Per mesh shader option flags was on, but now turned off, so reset all the shader options.
                for (auto& modelHandle : m_modelData)
                {
                    if (modelHandle.m_cullable.m_prevShaderOptionFlags != modelHandle.m_cullable.m_shaderOptionFlags)
                    {
                        // skip if the model need to be initialized
                        if (modelHandle.m_flags.m_needsInit)
                        {
                            continue;
                        }

                        if (!r_meshInstancingEnabled)
                        {
                            for (RPI::MeshDrawPacketList& drawPacketList : modelHandle.m_meshDrawPacketListsByLod)
                            {
                                for (RPI::MeshDrawPacket& drawPacket : drawPacketList)
                                {
                                    m_flagRegistry->VisitTags(
                                        [&](AZ::Name shaderOption, FlagRegistry::TagType tag)
                                        {
                                            bool shaderOptionValue = (modelHandle.m_cullable.m_shaderOptionFlags & tag.GetIndex()) > 0;
                                            drawPacket.SetShaderOption(shaderOption, AZ::RPI::ShaderOptionValue(shaderOptionValue));
                                        });
                                    drawPacket.Update(*GetParentScene(), true);
                                }
                            }

                            modelHandle.m_flags.m_cullableNeedsRebuild = true;
                            // [GHI-13619]
                            // Update the draw packets on the cullable, since we just set a shader item.
                            // BuildCullable is a bit overkill here, this could be reduced to just updating the drawPacket specific info
                            // It's also going to cause m_cullableNeedsUpdate to be set, which will execute next frame, which we don't need
                            modelHandle.BuildCullable();
                        }
                        else
                        {
                            // mark the instance groups which need to update their shader options
                            for (size_t lodIndex = 0; lodIndex < modelHandle.m_postCullingInstanceDataByLod.size(); ++lodIndex)
                            {
                                ModelDataInstance::PostCullingInstanceDataList& postCullingInstanceDataList =
                                    modelHandle.m_postCullingInstanceDataByLod[lodIndex];
                                for (const ModelDataInstance::PostCullingInstanceData& postCullingData : postCullingInstanceDataList)
                                {
                                    instanceGroupsNeedUpdate.push_back(postCullingData.m_instanceGroupHandle);
                                }
                            }
                        }
                    }
                }

                if (r_meshInstancingEnabled)
                {
                    for (auto& instanceGroupDataIter : instanceGroupsNeedUpdate)
                    {
                        // default values for when r_enablePerMeshShaderOptionFlags was set from true to false
                        bool shaderOptionFlagsChanged = true;
                        uint32_t shaderOptionFlagMask = 0; // 0 means disable all shader options

                        if (r_enablePerMeshShaderOptionFlags)
                        {
                            shaderOptionFlagsChanged = instanceGroupDataIter->UpdateShaderOptionFlags();
                            shaderOptionFlagMask = instanceGroupDataIter->m_shaderOptionFlagMask;
                        }

                        if (shaderOptionFlagsChanged)
                        {
                            // Set shader options here
                            m_flagRegistry->VisitTags(
                                [&](AZ::Name shaderOption, FlagRegistry::TagType tag)
                                {
                                    if ((shaderOptionFlagMask & tag.GetIndex()) > 0)
                                    {
                                        bool shaderOptionValue = (instanceGroupDataIter->m_shaderOptionFlags & tag.GetIndex()) > 0;
                                        instanceGroupDataIter->m_drawPacket.SetShaderOption(
                                            shaderOption, AZ::RPI::ShaderOptionValue(shaderOptionValue));
                                    }
                                    else
                                    {
                                        instanceGroupDataIter->m_drawPacket.UnsetShaderOption(shaderOption);
                                    }
                                });
                            instanceGroupDataIter->UpdateDrawPacket(*GetParentScene(), true);

                            // Note, we don't need to call CacheRootConstantInterval() here because the root constant layout won't change
                            // when we switch shader variants.
                        }
                    }
                }
            }

            m_enablePerMeshShaderOptionFlags = r_enablePerMeshShaderOptionFlags;
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
                model.m_cullable.m_prevShaderOptionFlags = model.m_cullable.m_shaderOptionFlags.exchange(0);
                model.m_cullable.m_flags = model.m_flags.m_isAlwaysDynamic ? m_meshMovedFlag.GetIndex() : 0;
            }
        }

        MeshFeatureProcessor::MeshHandle MeshFeatureProcessor::AcquireMesh(const MeshHandleDescriptor& descriptor)
        {
            AZ_PROFILE_SCOPE(AzRender, "MeshFeatureProcessor: AcquireMesh");

            // don't need to check the concurrency during emplace() because the StableDynamicArray won't move the other elements during
            // insertion
            StableDynamicArrayHandle<ModelDataInstance> meshDataHandle = m_modelData.emplace();

            meshDataHandle->m_descriptor = descriptor;
            meshDataHandle->m_descriptor.m_modelChangedEventHandler.Connect(meshDataHandle->m_modelChangedEvent);
            meshDataHandle->m_descriptor.m_objectSrgCreatedHandler.Connect(meshDataHandle->m_objectSrgCreatedEvent);
            meshDataHandle->m_scene = GetParentScene();
            meshDataHandle->m_objectId = m_transformService->ReserveObjectId();
            meshDataHandle->m_rayTracingUuid = AZ::Uuid::CreateRandom();
            meshDataHandle->m_originalModelAsset = descriptor.m_modelAsset;
            meshDataHandle->m_flags.m_keepBufferAssetsInMemory = descriptor.m_supportRayIntersection; // Note: the MeshLoader may need to read this flag. so it needs to be assigned because meshloader is created
            meshDataHandle->m_meshLoader = AZStd::make_shared<ModelDataInstance::MeshLoader>(descriptor.m_modelAsset, &*meshDataHandle);
            meshDataHandle->m_flags.m_isAlwaysDynamic = descriptor.m_isAlwaysDynamic;
            meshDataHandle->m_flags.m_isDrawMotion = descriptor.m_isAlwaysDynamic;

            if (descriptor.m_excludeFromReflectionCubeMaps)
            {
                meshDataHandle->m_cullable.m_cullData.m_hideFlags |= RPI::View::UsageReflectiveCubeMap;
            }

            return meshDataHandle;
        }

        bool MeshFeatureProcessor::ReleaseMesh(MeshHandle& meshHandle)
        {
            if (meshHandle.IsValid())
            {
                auto converted = StableDynamicArrayHandle<ModelDataInstance>(std::move(meshHandle));
                converted->m_meshLoader.reset();
                converted->DeInit(this);
                m_transformService->ReleaseObjectId(converted->m_objectId);

                AZStd::concurrency_check_scope scopeCheck(m_meshDataChecker);
                m_modelData.erase(converted);

                return true;
            }
            return false;
        }

        void MeshFeatureProcessor::SetDrawItemEnabled(const MeshHandle& meshHandle, RHI::DrawListTag drawListTag, bool enabled)
        {
            AZ::RPI::MeshDrawPacketLods& drawPacketListByLod = meshHandle.IsValid() && !r_meshInstancingEnabled
                ? ToModelDataInstance(meshHandle).m_meshDrawPacketListsByLod
                : m_emptyDrawPacketLods;

            for (AZ::RPI::MeshDrawPacketList& drawPacketList : drawPacketListByLod)
            {
                for (AZ::RPI::MeshDrawPacket& meshDrawPacket : drawPacketList)
                {
                    RHI::DrawPacket* drawPacket = meshDrawPacket.GetRHIDrawPacket();

                    if (drawPacket != nullptr)
                    {
                        size_t drawItemCount = drawPacket->GetDrawItemCount();

                        for (size_t idx = 0; idx < drawItemCount; ++idx)
                        {
                            // Ensure that the draw item belongs to the specified tag
                            if (drawPacket->GetDrawListTag(idx) == drawListTag)
                            {
                                drawPacket->GetDrawItem(idx)->SetEnabled(enabled);
                            }
                        }
                    }
                }
            }
        }

        void MeshFeatureProcessor::PrintDrawPacketInfo(const MeshHandle& meshHandle)
        {
            AZStd::string stringOutput = "\n------- MESH INFO -------\n";

            AZ::RPI::MeshDrawPacketLods& drawPacketListByLod = meshHandle.IsValid() && !r_meshInstancingEnabled
                ? ToModelDataInstance(meshHandle).m_meshDrawPacketListsByLod
                : m_emptyDrawPacketLods;

            u32 lodCounter = 0;
            for (AZ::RPI::MeshDrawPacketList& drawPacketList : drawPacketListByLod)
            {
                stringOutput += AZStd::string::format("--- Mesh Lod %u ---\n", lodCounter++);
                u32 drawPacketCounter = 0;
                for (AZ::RPI::MeshDrawPacket& meshDrawPacket : drawPacketList)
                {
                    RHI::DrawPacket* drawPacket = meshDrawPacket.GetRHIDrawPacket();
                    if (drawPacket)
                    {
                        size_t numDrawItems = drawPacket->GetDrawItemCount();
                        stringOutput += AZStd::string::format("-- Draw Packet %u (%zu Draw Items) --\n", drawPacketCounter++, numDrawItems);

                        for (size_t drawItemIdx = 0; drawItemIdx < numDrawItems; ++drawItemIdx)
                        {
                            RHI::DrawItem* drawItem = drawPacket->GetDrawItem(drawItemIdx);
                            RHI::DrawListTag tag = drawPacket->GetDrawListTag(drawItemIdx);
                            stringOutput += AZStd::string::format("Item %zu | ", drawItemIdx);
                            stringOutput += drawItem->GetEnabled() ? "Enabled  | " : "Disabled | ";
                            stringOutput += AZStd::string::format("%s Tag\n", RHI::GetDrawListName(tag).GetCStr());
                        }

                    }
                }
            }
            stringOutput += "\n";
            AZ_Printf("MeshFeatureProcessor", stringOutput.c_str());
        }

        MeshFeatureProcessor::MeshHandle MeshFeatureProcessor::CloneMesh(const MeshHandle& meshHandle)
        {
            if (meshHandle.IsValid())
            {
                return AcquireMesh(ToModelDataInstance(meshHandle).m_descriptor);
            }
            return MeshFeatureProcessor::MeshHandle();
        }

        Data::Instance<RPI::Model> MeshFeatureProcessor::GetModel(const MeshHandle& meshHandle) const
        {
            return meshHandle.IsValid() ? ToModelDataInstance(meshHandle).m_model : nullptr;
        }

        Data::Asset<RPI::ModelAsset> MeshFeatureProcessor::GetModelAsset(const MeshHandle& meshHandle) const
        {
            if (meshHandle.IsValid())
            {
                return ToModelDataInstance(meshHandle).m_originalModelAsset;
            }

            return {};
        }

        const RPI::MeshDrawPacketLods& MeshFeatureProcessor::GetDrawPackets(const MeshHandle& meshHandle) const
        {
            // This function is being deprecated. It's currently used to get draw packets so that we can print some
            // debug information about the draw packets in an imgui menu. But the ownership model for draw packets is changing.
            // We can no longer assume a meshHandle directly keeps a copy of all of its draw packets.

            return meshHandle.IsValid() && !r_meshInstancingEnabled ? ToModelDataInstance(meshHandle).m_meshDrawPacketListsByLod
                                                                    : m_emptyDrawPacketLods;
        }

        const AZStd::vector<Data::Instance<RPI::ShaderResourceGroup>>& MeshFeatureProcessor::GetObjectSrgs(const MeshHandle& meshHandle) const
        {
            static AZStd::vector<Data::Instance<RPI::ShaderResourceGroup>> staticEmptyList;
            return meshHandle.IsValid() ? ToModelDataInstance(meshHandle).m_objectSrgList : staticEmptyList;
        }

        void MeshFeatureProcessor::QueueObjectSrgForCompile(const MeshHandle& meshHandle) const
        {
            if (meshHandle.IsValid())
            {
                ToModelDataInstance(meshHandle).m_flags.m_objectSrgNeedsUpdate = true;
            }
        }

        void MeshFeatureProcessor::SetCustomMaterials(const MeshHandle& meshHandle, const Data::Instance<RPI::Material>& material)
        {
            Render::CustomMaterialMap materials;
            materials[AZ::Render::DefaultCustomMaterialId] = { material };
            return SetCustomMaterials(meshHandle, materials);
        }

        void MeshFeatureProcessor::SetCustomMaterials(const MeshHandle& meshHandle, const CustomMaterialMap& materials)
        {
            if (meshHandle.IsValid())
            {
                auto& modelData = ToModelDataInstance(meshHandle);
                modelData.m_descriptor.m_customMaterials = materials;
                if (modelData.m_model)
                {
                    modelData.ReInit(this);
                }

                modelData.m_flags.m_objectSrgNeedsUpdate = true;
            }
        }

        const CustomMaterialMap& MeshFeatureProcessor::GetCustomMaterials(const MeshHandle& meshHandle) const
        {
            return meshHandle.IsValid() ? ToModelDataInstance(meshHandle).m_descriptor.m_customMaterials : DefaultCustomMaterialMap;
        }

        AZStd::unique_ptr<StreamBufferViewsBuilderInterface> MeshFeatureProcessor::CreateStreamBufferViewsBuilder(
            const MeshHandle& meshHandle) const
        {
            return AZStd::make_unique<ShaderStreamBufferViewsBuilder>(meshHandle);
        }

        DispatchDrawItemList MeshFeatureProcessor::BuildDispatchDrawItemList(
            const MeshHandle& meshHandle,
            const uint32_t lodIndex,
            const uint32_t meshIndex,
            const RHI::DrawListMask drawListTagsFilter,
            const RHI::DrawFilterMask materialPipelineFilter,
            DispatchArgumentsSetupCB dispatchArgumentsSetupCB) const
        {
            DispatchDrawItemList retList;

            const AZ::RPI::MeshDrawPacketLods& drawPacketListByLod = GetDrawPackets(meshHandle);
            const uint32_t lodCount = aznumeric_caster(drawPacketListByLod.size());
            if (lodIndex >= lodCount)
            {
                // This is normal. May happen if a caller got a valid MeshHandle before
                // a mesh is fully loaded from assets.
                return retList;
            }
            const AZ::RPI::MeshDrawPacketList& drawPacketList = drawPacketListByLod[lodIndex];
            const uint32_t meshCount = aznumeric_caster(drawPacketList.size());
            if (meshIndex >= meshCount)
            {
                AZ_Error("MeshFeatureProcessor", false,
                    "For lodIndex=%u, got invalid meshIndex=%u, maxMeshCount=%u",
                    lodIndex, meshIndex, meshCount);
                return retList;
            }
            const AZ::RPI::MeshDrawPacket& meshDrawPacket = drawPacketList[meshIndex];
            const RHI::DrawPacket* drawPacket = meshDrawPacket.GetRHIDrawPacket();
            const auto& shadersList = meshDrawPacket.GetActiveShaderList();
            if (drawPacket)
            {
                const uint32_t drawItemCount = aznumeric_caster(drawPacket->GetDrawItemCount());
                for (uint32_t drawItemIdx = 0; drawItemIdx < drawItemCount; ++drawItemIdx)
                {
                    const RHI::DrawItem* drawItem = drawPacket->GetDrawItem(drawItemIdx);
                    if (drawItem->GetPipelineStateType() != RHI::PipelineStateType::Dispatch)
                    {
                        continue;
                    }
                    // Only create the DispatchItems for DrawItems whose DrawListTag is included
                    // in @drawListTagsFilter AND their DrawFilterMask is included in @materialPipelineFilter.
                    RHI::DrawListTag tag = drawPacket->GetDrawListTag(drawItemIdx);
                    RHI::DrawFilterMask drawItemPipelineFilter = drawPacket->GetDrawFilterMask(drawItemIdx);
                    if (
                        drawListTagsFilter.test(tag.GetIndex()) &&
                        (materialPipelineFilter & drawItemPipelineFilter)
                        )
                    {
                        retList.emplace_back(DispatchDrawItem(drawItem));
                        auto& dispatchDrawItem = retList.back();
                        const auto& shaderAsset = shadersList[drawItemIdx].m_shader->GetAsset();
                        RHI::DispatchDirect dispatchDirect;
                        RPI::GetComputeShaderNumThreads(shaderAsset, dispatchDirect);
                        dispatchArgumentsSetupCB(lodIndex, meshIndex, drawItemIdx, drawItem, dispatchDirect);
                        InitializeDispatchItemFromDrawItem(dispatchDrawItem.m_distpatchItem, drawItem, dispatchDirect);
                    }
                }
            }

            return retList;
        }

        void MeshFeatureProcessor::InitializeDispatchItemFromDrawItem(
            RHI::DispatchItem& dstDispatchItem, const RHI::DrawItem* srcDrawItem, const RHI::DispatchDirect& dispatchDirect) const
        {
            RHI::DispatchArguments dispatchArguments(dispatchDirect);
            dstDispatchItem.SetArguments(dispatchArguments);

            bool deviceCommonDataIsSet = false;
            RHI::MultiDeviceObject::IterateDevices(
                RHI::MultiDevice::AllDevices,
                [&](int deviceIndex)
                {
                    const auto& deviceDrawItem = srcDrawItem->GetDeviceDrawItem(deviceIndex);
                    dstDispatchItem.SetDeviceShaderResourceGroups(
                        deviceIndex, deviceDrawItem.m_shaderResourceGroups, deviceDrawItem.m_shaderResourceGroupCount);
                    dstDispatchItem.SetUniqueDeviceShaderResourceGroup(deviceIndex, deviceDrawItem.m_uniqueShaderResourceGroup);
                    dstDispatchItem.SetDevicePipelineState(deviceIndex, deviceDrawItem.m_pipelineState);
                    if (!deviceCommonDataIsSet)
                    {
                        dstDispatchItem.SetRootConstantSize(deviceDrawItem.m_rootConstantSize);
                        dstDispatchItem.SetRootConstants(deviceDrawItem.m_rootConstants);
                        deviceCommonDataIsSet = true;
                    }
                    return true;
                });
        }

        void MeshFeatureProcessor::SetTransform(const MeshHandle& meshHandle, const AZ::Transform& transform, const AZ::Vector3& nonUniformScale)
        {
            if (meshHandle.IsValid())
            {
                auto& modelData = ToModelDataInstance(meshHandle);
                modelData.m_flags.m_cullBoundsNeedsUpdate = true;
                modelData.m_flags.m_objectSrgNeedsUpdate = true;
                modelData.m_cullable.m_flags = modelData.m_cullable.m_flags | m_meshMovedFlag.GetIndex();

                // Only set m_dynamic flag if the model instance is initialized.
                if (!modelData.m_flags.m_dynamic)
                {
                    modelData.m_flags.m_dynamic = (modelData.m_model && !modelData.m_flags.m_needsInit) ? true : false;

                    // Enable draw motion for all the DrawPacket referenced by this model
                    if (r_meshInstancingEnabled && modelData.m_flags.m_dynamic)
                    {
                        for (size_t lodIndex = 0; lodIndex < modelData.m_postCullingInstanceDataByLod.size(); ++lodIndex)
                        {
                            ModelDataInstance::PostCullingInstanceDataList& postCullingInstanceDataList =
                                modelData.m_postCullingInstanceDataByLod[lodIndex];
                            for (const ModelDataInstance::PostCullingInstanceData& postCullingData : postCullingInstanceDataList)
                            {
                                AZStd::scoped_lock<AZStd::mutex> scopedLock(postCullingData.m_instanceGroupHandle->m_eventLock);
                                if (!postCullingData.m_instanceGroupHandle->m_isDrawMotion)
                                {
                                    postCullingData.m_instanceGroupHandle->m_isDrawMotion = true;
                                    postCullingData.m_instanceGroupHandle->m_drawPacket.SetEnableDraw(m_meshMotionDrawListTag, true);
                                }
                            }
                        }
                    }
                }

                m_transformService->SetTransformForId(modelData.m_objectId, transform, nonUniformScale);

                // ray tracing data needs to be updated with the new transform
                if (m_rayTracingFeatureProcessor)
                {
                    m_rayTracingFeatureProcessor->SetMeshTransform(modelData.m_rayTracingUuid, transform, nonUniformScale);
                }
            }
        }

        void MeshFeatureProcessor::SetLocalAabb(const MeshHandle& meshHandle, const AZ::Aabb& localAabb)
        {
            if (meshHandle.IsValid())
            {
                ModelDataInstance& modelData = ToModelDataInstance(meshHandle);
                modelData.m_aabb = localAabb;
                modelData.m_flags.m_cullBoundsNeedsUpdate = true;
                modelData.m_flags.m_objectSrgNeedsUpdate = true;
            }
        };

        AZ::Aabb MeshFeatureProcessor::GetLocalAabb(const MeshHandle& meshHandle) const
        {
            if (meshHandle.IsValid())
            {
                return ToModelDataInstance(meshHandle).m_aabb;
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
                return m_transformService->GetTransformForId(ToModelDataInstance(meshHandle).m_objectId);
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
                return m_transformService->GetNonUniformScaleForId(ToModelDataInstance(meshHandle).m_objectId);
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
                ToModelDataInstance(meshHandle).SetSortKey(this, sortKey);
            }
        }

        RHI::DrawItemSortKey MeshFeatureProcessor::GetSortKey(const MeshHandle& meshHandle) const
        {
            if (meshHandle.IsValid())
            {
                return ToModelDataInstance(meshHandle).GetSortKey();
            }
            else
            {
                AZ_Assert(false, "Invalid mesh handle");
                return 0;
            }
        }

        void ModelDataInstance::SetLightingChannelMask(uint32_t lightingChannelMask)
        {
            m_lightingChannelMask = lightingChannelMask;
        }

        void MeshFeatureProcessor::SetMeshLodConfiguration(const MeshHandle& meshHandle, const RPI::Cullable::LodConfiguration& meshLodConfig)
        {
            if (meshHandle.IsValid())
            {
                ToModelDataInstance(meshHandle).SetMeshLodConfiguration(meshLodConfig);
            }
        }

        RPI::Cullable::LodConfiguration MeshFeatureProcessor::GetMeshLodConfiguration(const MeshHandle& meshHandle) const
        {
            if (meshHandle.IsValid())
            {
                return ToModelDataInstance(meshHandle).GetMeshLodConfiguration();
            }
            else
            {
                AZ_Assert(false, "Invalid mesh handle");
                return { RPI::Cullable::LodType::Default, 0, 0.0f, 0.0f };
            }
        }

        void MeshFeatureProcessor::SetIsAlwaysDynamic(const MeshHandle & meshHandle, bool isAlwaysDynamic)
        {
            if (meshHandle.IsValid())
            {
                ToModelDataInstance(meshHandle).m_flags.m_isAlwaysDynamic = isAlwaysDynamic;
            }
        }

        bool MeshFeatureProcessor::GetIsAlwaysDynamic(const MeshHandle& meshHandle) const
        {
            if (!meshHandle.IsValid())
            {
                AZ_Assert(false, "Invalid mesh handle");
                return false;
            }
            return ToModelDataInstance(meshHandle).m_flags.m_isAlwaysDynamic;
        }

        void MeshFeatureProcessor::SetExcludeFromReflectionCubeMaps(const MeshHandle& meshHandle, bool excludeFromReflectionCubeMaps)
        {
            if (meshHandle.IsValid())
            {
                auto& modelData = ToModelDataInstance(meshHandle);
                modelData.m_descriptor.m_excludeFromReflectionCubeMaps = excludeFromReflectionCubeMaps;
                if (excludeFromReflectionCubeMaps)
                {
                    modelData.m_cullable.m_cullData.m_hideFlags |= RPI::View::UsageReflectiveCubeMap;
                }
                else
                {
                    modelData.m_cullable.m_cullData.m_hideFlags &= ~RPI::View::UsageReflectiveCubeMap;
                }
            }
        }

        bool MeshFeatureProcessor::GetExcludeFromReflectionCubeMaps(const MeshHandle& meshHandle) const
        {
            if (meshHandle.IsValid())
            {
                return ToModelDataInstance(meshHandle).m_descriptor.m_excludeFromReflectionCubeMaps;
            }
            return false;
        }

        void MeshFeatureProcessor::SetRayTracingEnabled(const MeshHandle& meshHandle, bool enabled)
        {
            if (meshHandle.IsValid())
            {
                auto& modelData = ToModelDataInstance(meshHandle);
                // update the ray tracing data based on the current state and the new state
                if (enabled && !modelData.m_descriptor.m_isRayTracingEnabled)
                {
                    // add to ray tracing
                    modelData.m_flags.m_needsSetRayTracingData = true;
                }
                else if (!enabled && modelData.m_descriptor.m_isRayTracingEnabled)
                {
                    // remove from ray tracing
                    if (m_rayTracingFeatureProcessor)
                    {
                        m_rayTracingFeatureProcessor->RemoveMesh(modelData.m_rayTracingUuid);
                    }
                }

                // set new state
                modelData.m_descriptor.m_isRayTracingEnabled = enabled;
            }
        }

        bool MeshFeatureProcessor::GetRayTracingEnabled(const MeshHandle& meshHandle) const
        {
            if (meshHandle.IsValid())
            {
                return ToModelDataInstance(meshHandle).m_descriptor.m_isRayTracingEnabled;
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
                return ToModelDataInstance(meshHandle).m_flags.m_visible;
            }
            return false;
        }

        void MeshFeatureProcessor::SetVisible(const MeshHandle& meshHandle, bool visible)
        {
            if (meshHandle.IsValid())
            {
                auto& modelData = ToModelDataInstance(meshHandle);
                modelData.SetVisible(visible);

                if (m_rayTracingFeatureProcessor && modelData.m_descriptor.m_isRayTracingEnabled)
                {
                    // always remove from ray tracing first
                    m_rayTracingFeatureProcessor->RemoveMesh(modelData.m_rayTracingUuid);

                    // now add if it's visible
                    if (visible)
                    {
                        modelData.m_flags.m_needsSetRayTracingData = true;
                    }
                }
            }
        }

        void MeshFeatureProcessor::SetUseForwardPassIblSpecular(const MeshHandle& meshHandle, bool useForwardPassIblSpecular)
        {
            if (meshHandle.IsValid())
            {
                auto& modelData = ToModelDataInstance(meshHandle);
                modelData.m_descriptor.m_useForwardPassIblSpecular = useForwardPassIblSpecular;
                modelData.m_flags.m_objectSrgNeedsUpdate = true;

                if (modelData.m_model)
                {
                    const size_t modelLodCount = modelData.m_model->GetLodCount();
                    for (size_t modelLodIndex = 0; modelLodIndex < modelLodCount; ++modelLodIndex)
                    {
                        modelData.BuildDrawPacketList(this, modelLodIndex);
                    }
                }
            }
        }

        void MeshFeatureProcessor::SetRayTracingDirty(const MeshHandle& meshHandle)
        {
            if (meshHandle.IsValid())
            {
                ToModelDataInstance(meshHandle).m_flags.m_needsSetRayTracingData = true;
            }
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
            for (auto& meshInstance : m_modelData)
            {
                // we need to rebuild the Srg for any meshes that are using the forward pass IBL specular option
                if (meshInstance.m_descriptor.m_useForwardPassIblSpecular)
                {
                    meshInstance.m_flags.m_objectSrgNeedsUpdate = true;
                }

                // update the raytracing reflection probe data if necessary
                RayTracingFeatureProcessor::Mesh::ReflectionProbe reflectionProbe;
                bool currentHasRayTracingReflectionProbe = meshInstance.m_flags.m_hasRayTracingReflectionProbe;
                meshInstance.SetRayTracingReflectionProbeData(this, reflectionProbe);

                if (meshInstance.m_flags.m_hasRayTracingReflectionProbe ||
                    (currentHasRayTracingReflectionProbe != meshInstance.m_flags.m_hasRayTracingReflectionProbe))
                {
                    m_rayTracingFeatureProcessor->SetMeshReflectionProbe(meshInstance.m_rayTracingUuid, reflectionProbe);
                }
            }
        }

        void MeshFeatureProcessor::ReportShaderOptionFlags([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
        {
            m_reportShaderOptionFlags = true;
        }

        RayTracingFeatureProcessor* MeshFeatureProcessor::GetRayTracingFeatureProcessor() const
        {
            return m_rayTracingFeatureProcessor;
        }

        ReflectionProbeFeatureProcessor* MeshFeatureProcessor::GetReflectionProbeFeatureProcessor() const
        {
            return m_reflectionProbeFeatureProcessor;
        }

        TransformServiceFeatureProcessor* MeshFeatureProcessor::GetTransformServiceFeatureProcessor() const
        {
            return m_transformService;
        }

        RHI::DrawListTag MeshFeatureProcessor::GetTransparentDrawListTag() const
        {
            return m_transparentDrawListTag;
        }

        MeshInstanceManager& MeshFeatureProcessor::GetMeshInstanceManager()
        {
            return m_meshInstanceManager;
        }

        bool MeshFeatureProcessor::IsMeshInstancingEnabled() const
        {
            return m_enableMeshInstancing;
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
                ++flagStats[model.m_cullable.m_shaderOptionFlags.load()];
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

            m_modelAsset.QueueLoad();
            Data::AssetBus::Handler::BusConnect(m_modelAsset.GetId());
            AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        }

        ModelDataInstance::MeshLoader::~MeshLoader()
        {
            AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
            Data::AssetBus::Handler::BusDisconnect();
            SystemTickBus::Handler::BusDisconnect();
        }

        void ModelDataInstance::MeshLoader::OnSystemTick()
        {
            SystemTickBus::Handler::BusDisconnect();

            // Assign the fully loaded asset back to the mesh handle to not only hold asset id, but the actual data as well.
            m_parent->m_originalModelAsset = m_modelAsset;

            if (const auto& modelTags = m_modelAsset->GetTags(); !modelTags.empty())
            {
                RPI::AssetQuality highestLodBias = RPI::AssetQualityLowest;
                for (const AZ::Name& tag : modelTags)
                {
                    RPI::AssetQuality tagQuality = RPI::AssetQualityHighest;
                    RPI::ModelTagBus::BroadcastResult(tagQuality, &RPI::ModelTagBus::Events::GetQuality, tag);

                    highestLodBias = AZStd::min(highestLodBias, tagQuality);
                }

                if (highestLodBias >= m_modelAsset->GetLodCount())
                {
                    highestLodBias = aznumeric_caster(m_modelAsset->GetLodCount() - 1);
                }

                m_parent->m_lodBias = highestLodBias;

                for (const AZ::Name& tag : modelTags)
                {
                    RPI::ModelTagBus::Broadcast(&RPI::ModelTagBus::Events::RegisterAsset, tag, m_modelAsset->GetId());
                }
            }
            else
            {
                m_parent->m_lodBias = 0;
            }

            Data::Instance<RPI::Model> model;
            // Check if a requires cloning callback got set and if so check if cloning the model asset is requested.
            if (m_parent->m_descriptor.m_requiresCloneCallback && m_parent->m_descriptor.m_requiresCloneCallback(m_modelAsset))
            {
                // Clone the model asset to force create another model instance.
                AZ::Data::AssetId newId(AZ::Uuid::CreateRandom(), /*subId=*/0);
                Data::Asset<RPI::ModelAsset> clonedAsset;
                // Assume cloned models will involve some kind of geometry deformation
                m_parent->m_flags.m_isAlwaysDynamic = true;
                if (AZ::RPI::ModelAssetCreator::Clone(m_modelAsset, clonedAsset, newId))
                {
                    model = RPI::Model::FindOrCreate(clonedAsset);
                }
                else
                {
                    AZ_Error("ModelDataInstance", false, "Cannot clone model for '%s'. Cloth simulation results won't be individual per entity.", m_modelAsset->GetName().GetCStr());
                    model = RPI::Model::FindOrCreate(m_modelAsset);
                }
            }
            else
            {
                // Static mesh, no cloth buffer present.
                model = RPI::Model::FindOrCreate(m_modelAsset);
            }

            if (model)
            {
                RayTracingFeatureProcessor* rayTracingFeatureProcessor =
                    m_parent->m_scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
                m_parent->RemoveRayTracingData(rayTracingFeatureProcessor);
                m_parent->QueueInit(model);
                m_parent->m_modelChangedEvent.Signal(AZStd::move(model));

                // we always start out with a refcount of 1
                model->GetModelAsset()->AddRefBufferAssets();

                // if we don't want to keep them, this will drop the refcount to 0.
                if (!m_parent->m_flags.m_keepBufferAssetsInMemory)
                {
                    model->GetModelAsset()->ReleaseRefBufferAssets();
                }
            }
            else
            {
                // when running with null renderer, the RPI::Model::FindOrCreate(...) is expected to return nullptr, so suppress this error.
                AZ_Error("ModelDataInstance::OnAssetReady", RHI::IsNullRHI(), "Failed to create model instance for '%s'", m_modelAsset.GetHint().c_str());
            }
        }

        //! AssetBus::Handler overrides...
        void ModelDataInstance::MeshLoader::OnAssetReady(Data::Asset<Data::AssetData> asset)
        {
            // Update our model asset reference to contain the latest loaded version.
            m_modelAsset = asset;

            // The mesh loader queues the model asset to be loaded then connects to the asset bus. If the asset is already loaded
            // OnAssetReady will be called before returning from the acquire function. Many callers connect handlers for model change
            // events. Some of the handlers attempt to access their stored mesh handle member, which will not be up to date if the acquire
            // function hasn't returned. This postpones sending the event until the next tick, allowing the acquire function to return and
            // update and he stored mesh handles.
            SystemTickBus::Handler::BusConnect();
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

        void ModelDataInstance::MeshLoader::OnCatalogAssetRemoved(
            const AZ::Data::AssetId& assetId, [[maybe_unused]] const AZ::Data::AssetInfo& assetInfo)
        {
            OnCatalogAssetChanged(assetId);
        }

        void ModelDataInstance::MeshLoader::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
        {
            // If the asset didn't exist in the catalog when it first attempted to load, we need to try loading it again
            OnCatalogAssetChanged(assetId);
        }

        void ModelDataInstance::MeshLoader::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
        {
            if (assetId == m_modelAsset.GetId())
            {
                Data::Asset<RPI::ModelAsset> modelAssetReference = m_modelAsset;

                // If the asset was modified, reload it. This will also cause a model to change back to the default missing
                // asset if it was removed, and it will replace the default missing asset with the real asset if it was added.
                AZ::SystemTickBus::QueueFunction(
                    [=, meshLoader = m_parent->m_meshLoader]() mutable
                    {
                        // Only trigger the reload if the meshLoader is still being used by something other than the lambda.
                        // If the lambda is the only owner, it will get destroyed after this queued call, so there's no point
                        // in reloading the model.
                        if (meshLoader.use_count() > 1)
                        {
                            ModelReloaderSystemInterface::Get()->ReloadModel(modelAssetReference, m_modelReloadedEventHandler);
                        }
                    });
            }
        }

        ModelDataInstance::ModelDataInstance()
        {
            m_flags.m_cullBoundsNeedsUpdate = false;
            m_flags.m_cullableNeedsRebuild = false;
            m_flags.m_needsInit = false;
            m_flags.m_objectSrgNeedsUpdate = true;
            m_flags.m_isAlwaysDynamic = false;
            m_flags.m_dynamic = false;
            m_flags.m_isDrawMotion = false;
            m_flags.m_visible = true;
            m_flags.m_useForwardPassIblSpecular = false;
            m_flags.m_hasForwardPassIblSpecularMaterial = false;
            m_flags.m_needsSetRayTracingData = false;
            m_flags.m_hasRayTracingReflectionProbe = false;
        }

        void ModelDataInstance::DeInit(MeshFeatureProcessor* meshFeatureProcessor)
        {
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = meshFeatureProcessor->GetRayTracingFeatureProcessor();
            m_scene->GetCullingScene()->UnregisterCullable(m_cullable);

            RemoveRayTracingData(rayTracingFeatureProcessor);

            // We're intentionally using the MeshFeatureProcessor's value instead of using the cvar directly here,
            // because DeInit might be called after the cvar changes, but we want to do the de-initialization based
            // on what the setting was before (when the resources were initialized). The MeshFeatureProcessor will still have the cached
            // value in that case
            if (!meshFeatureProcessor->IsMeshInstancingEnabled())
            {
                m_meshDrawPacketListsByLod.clear();
            }
            else
            {
                // Remove all the meshes from the MeshInstanceManager
                MeshInstanceManager& meshInstanceManager = meshFeatureProcessor->GetMeshInstanceManager();

                for (size_t lodIndex = 0; lodIndex < m_postCullingInstanceDataByLod.size(); ++lodIndex)
                {
                    PostCullingInstanceDataList& postCullingInstanceDataList = m_postCullingInstanceDataByLod[lodIndex];
                    for (PostCullingInstanceData& postCullingData : postCullingInstanceDataList)
                    {
                        postCullingData.m_instanceGroupHandle->RemoveAssociatedInstance(this);

                        // Remove instance will decrement the use-count of the instance group, and only release the instance group
                        // if nothing else is referring to it.
                        meshInstanceManager.RemoveInstance(postCullingData.m_instanceGroupHandle);
                    }
                    postCullingInstanceDataList.clear();
                }
                m_postCullingInstanceDataByLod.clear();
            }

            m_descriptor.m_customMaterials.clear();
            m_objectSrgList = {};
            m_model = {};
        }

        void ModelDataInstance::ReInit(MeshFeatureProcessor* meshFeatureProcessor)
        {
            CustomMaterialMap customMaterials = m_descriptor.m_customMaterials;
            const Data::Instance<RPI::Model> model = m_model;
            DeInit(meshFeatureProcessor);
            m_descriptor.m_customMaterials = customMaterials;
            m_model = model;
            QueueInit(m_model);
        }

        void ModelDataInstance::QueueInit(const Data::Instance<RPI::Model>& model)
        {
            m_model = model;
            m_flags.m_needsInit = true;
            m_aabb = m_model->GetModelAsset()->GetAabb();
        }

        void ModelDataInstance::Init(MeshFeatureProcessor* meshFeatureProcessor)
        {
            const size_t modelLodCount = m_model->GetLodCount();

            if (!r_meshInstancingEnabled)
            {
                m_meshDrawPacketListsByLod.resize(modelLodCount);
            }
            else
            {
                m_postCullingInstanceDataByLod.resize(modelLodCount);
            }

            for (size_t modelLodIndex = 0; modelLodIndex < modelLodCount; ++modelLodIndex)
            {
                BuildDrawPacketList(meshFeatureProcessor, modelLodIndex);
            }

            for (auto& objectSrg : m_objectSrgList)
            {
                // Set object Id once since it never changes
                RHI::ShaderInputNameIndex objectIdIndex = "m_objectId";
                objectSrg->SetConstant(objectIdIndex, m_objectId.GetIndex());
                objectIdIndex.AssertValid();
            }

            if (m_flags.m_visible && m_descriptor.m_isRayTracingEnabled)
            {
                m_flags.m_needsSetRayTracingData = true;
            }

            m_flags.m_cullableNeedsRebuild = true;
            m_flags.m_cullBoundsNeedsUpdate = true;
            m_flags.m_objectSrgNeedsUpdate = true;
            m_flags.m_needsInit = false;
        }

        struct MeshInstancingSupport
        {
            bool m_canSupportInstancing = false;
            bool m_isTransparent = false;
        };

        static MeshInstancingSupport CanSupportInstancing(
            Data::Instance<RPI::Material> material, bool useForwardPassIbleSpecular, const RHI::DrawListTag& transparentDrawListTag)
        {
            MeshInstancingSupport result;
            if (useForwardPassIbleSpecular)
            {
                // Forward pass ibl specular uses the ObjectSrg to set the closest reflection probe data
                // Since all instances from a single instanced draw call share a single ObjectSrg, this
                // will not work with instancing unless they happen to all share the same closes probe.
                // In the future, we could make that part of the MeshInstanceGroupKey, but that impacts
                // the initalization logic since at Init time we don't yet know the closest reflection probe.
                // So initially we treat that case as not supporting instancing, and eventually we can re-order
                // the logic in MeshFeatureProcessor::Simulate such that we know the up-to-date ObjectSrg data
                // before this point
                result.m_canSupportInstancing = false;
                return result;
            }

            bool shadersSupportInstancing = true;
            bool isTransparent = false;
            material->ForAllShaderItems(
                [&](const Name&, const RPI::ShaderCollection::Item& shaderItem)
                {
                    if (shaderItem.IsEnabled())
                    {
                        // Check to see if the shaderItem has the o_meshInstancingEnabled option. All shader items in the draw packet must
                        // support this option
                        RPI::ShaderOptionIndex index = shaderItem.GetShaderOptionGroup().GetShaderOptionLayout()->FindShaderOptionIndex(
                            s_o_meshInstancingIsEnabled_Name);
                        if (!index.IsValid())
                        {
                            shadersSupportInstancing = false;
                            return false; // break
                        }

                        // Get the DrawListTag. Use the explicit draw list override if exists.
                        AZ::RHI::DrawListTag drawListTag = shaderItem.GetDrawListTagOverride();

                        if (drawListTag.IsNull())
                        {
                            drawListTag = RHI::RHISystemInterface::Get()->GetDrawListTagRegistry()->FindTag(
                                shaderItem.GetShaderAsset()->GetDrawListName());
                        }

                        // Check to see if the shaderItem is for a transparent pass. If any of the active shader items
                        // are for a transparent pass, we still support instancing, but we mark it as transparent so that
                        // we can sort by reverse-depth
                        if (drawListTag == transparentDrawListTag)
                        {
                            isTransparent = true;
                            if (!r_meshInstancingEnabledForTransparentObjects)
                            {
                                shadersSupportInstancing = false;
                                return false; // break
                            }
                        }
                    }

                    return true; // continue
                });

            result.m_canSupportInstancing = shadersSupportInstancing;
            result.m_isTransparent = isTransparent;
            return result;
        }

        void ModelDataInstance::BuildDrawPacketList(MeshFeatureProcessor* meshFeatureProcessor, size_t modelLodIndex)
        {
            RPI::ModelLod& modelLod = *m_model->GetLods()[modelLodIndex];
            const size_t meshCount = modelLod.GetMeshes().size();
            MeshInstanceManager& meshInstanceManager = meshFeatureProcessor->GetMeshInstanceManager();

            if (!r_meshInstancingEnabled)
            {
                RPI::MeshDrawPacketList& drawPacketListOut = m_meshDrawPacketListsByLod[modelLodIndex];
                drawPacketListOut.clear();
                drawPacketListOut.reserve(meshCount);
            }

            auto meshMotionDrawListTag = AZ::RHI::RHISystemInterface::Get()->GetDrawListTagRegistry()->FindTag(MeshCommon::MotionDrawListTagName);

            for (size_t meshIndex = 0; meshIndex < meshCount; ++meshIndex)
            {
                const auto meshes = modelLod.GetMeshes();
                const RPI::ModelLod::Mesh& mesh = meshes[meshIndex];

                // Determine if there is a custom material specified for this submission
                const CustomMaterialId customMaterialId(aznumeric_cast<AZ::u64>(modelLodIndex), mesh.m_materialSlotStableId);
                const auto& customMaterialInfo = GetCustomMaterialWithFallback(customMaterialId);
                const auto& material = customMaterialInfo.m_material ? customMaterialInfo.m_material : mesh.m_material;

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
                    m_objectSrgCreatedEvent.Signal(meshObjectSrg);
                    m_objectSrgList.push_back(meshObjectSrg);
                }

                bool materialRequiresForwardPassIblSpecular = MaterialRequiresForwardPassIblSpecular(material);

                // Track whether any materials in this mesh require ForwardPassIblSpecular, we need this information when the ObjectSrg is
                // updated
                m_flags.m_hasForwardPassIblSpecularMaterial |= materialRequiresForwardPassIblSpecular;

                MeshInstanceManager::InsertResult instanceGroupInsertResult{ MeshInstanceManager::Handle{}, 0 };

                MeshInstancingSupport instancingSupport;
                if (r_meshInstancingEnabled)
                {
                    // Get the instance index for referencing the draw packet
                    MeshInstanceGroupKey key{};

                    // Only meshes from the same model and lod with a matching material instance can be instanced
                    key.m_modelId = m_model->GetId();
                    key.m_lodIndex = static_cast<uint32_t>(modelLodIndex);
                    key.m_meshIndex = static_cast<uint32_t>(meshIndex);
                    key.m_materialId = material->GetId();

                    // Two meshes that could otherwise be instanced but have manually specified sort keys will not be instanced together
                    key.m_sortKey = m_sortKey;

                    instancingSupport = CanSupportInstancing(
                        material, m_flags.m_hasForwardPassIblSpecularMaterial, meshFeatureProcessor->GetTransparentDrawListTag());

                    if (instancingSupport.m_canSupportInstancing && !r_meshInstancingDebugForceUniqueObjectsForProfiling)
                    {
                        // If this object can be instanced, it gets a null uuid that will match other objects that can be instanced with it
                        key.m_forceInstancingOff = Uuid::CreateNull();
                    }
                    else
                    {
                        // When instancing is enabled, everything goes down the instancing path, including this object
                        // However, using a random uuid here will give it its own unique instance group, with it's own unique ObjectSrg,
                        // so it will end up as an instanced draw call with a count of 1

                        // We also use this path when r_meshInstancingDebugForceUniqueObjectsForProfiling is true, which makes meshes that
                        // would otherwise be instanced end up in a unique group. This is helpful for performance profiling to test the
                        // worst case scenario of lots of objects that don't actually end up getting instanced but still go down the
                        // instancing path
                        key.m_forceInstancingOff = Uuid::CreateRandom();
                    }

                    instanceGroupInsertResult = meshInstanceManager.AddInstance(key);
                    PostCullingInstanceData postCullingData;
                    postCullingData.m_instanceGroupHandle = instanceGroupInsertResult.m_handle;
                    postCullingData.m_instanceGroupPageIndex = instanceGroupInsertResult.m_pageIndex;
                    postCullingData.m_objectId = m_objectId;
                    // Mark the group as transparent so that the depth can be sorted in reverse
                    postCullingData.m_instanceGroupHandle->m_isTransparent = instancingSupport.m_isTransparent;
                    m_postCullingInstanceDataByLod[modelLodIndex].push_back(postCullingData);

                    // The instaceGroup needs to keep a reference of this ModelDataInstance so it can
                    // notify the ModelDataInstance when the MeshDrawPacket is changed or get the cullable's flags
                    instanceGroupInsertResult.m_handle->AddAssociatedInstance(this);
                }

                // If this condition is true, we're dealing with a new, uninitialized draw packet, either because instancing is disabled
                // or because this was the first object in the instance group. So we need to initialize it
                if (!r_meshInstancingEnabled || instanceGroupInsertResult.m_instanceCount == 1)
                {
                    // setup the mesh draw packet
                    RPI::MeshDrawPacket drawPacket(modelLod, meshIndex, material, meshObjectSrg, customMaterialInfo.m_uvMapping);

                    // set the shader option to select forward pass IBL specular if necessary
                    if (!drawPacket.SetShaderOption(s_o_meshUseForwardPassIBLSpecular_Name, AZ::RPI::ShaderOptionValue{ m_descriptor.m_useForwardPassIblSpecular }))
                    {
                        AZ_Warning("MeshDrawPacket", false, "Failed to set o_meshUseForwardPassIBLSpecular on mesh draw packet");
                    }

                    if (instancingSupport.m_canSupportInstancing)
                    {
                        drawPacket.SetShaderOption(s_o_meshInstancingIsEnabled_Name, AZ::RPI::ShaderOptionValue{ true });
                    }

                    bool blockSilhouettes = false;
                    if (auto index = material->FindPropertyIndex(s_block_silhouette_Name); index.IsValid())
                    {
                        blockSilhouettes = material->GetPropertyValue<bool>(index);
                    }

                    // stencil bits
                    uint8_t stencilRef = m_descriptor.m_useForwardPassIblSpecular || materialRequiresForwardPassIblSpecular
                        ? Render::StencilRefs::None
                        : Render::StencilRefs::UseIBLSpecularPass;
                    stencilRef |= Render::StencilRefs::UseDiffuseGIPass;
                    stencilRef |= blockSilhouettes ? Render::StencilRefs::BlockSilhouettes : 0;

                    drawPacket.SetStencilRef(stencilRef);
                    drawPacket.SetSortKey(m_sortKey);
                    drawPacket.SetEnableDraw(meshMotionDrawListTag, m_flags.m_isDrawMotion);
                    // Note: do not add drawPacket.Update() here. It's not needed.It may cause issue with m_shaderVariantHandler which captures 'this' pointer.

                    if (!r_meshInstancingEnabled)
                    {
                        m_meshDrawPacketListsByLod[modelLodIndex].emplace_back(AZStd::move(drawPacket));
                    }
                    else
                    {
                        MeshInstanceGroupData& instanceGroupData = meshInstanceManager[instanceGroupInsertResult.m_handle];
                        instanceGroupData.m_drawPacket = AZStd::move(drawPacket);
                        instanceGroupData.m_isDrawMotion = m_flags.m_isDrawMotion;

                        // We're going to need an interval for the root constant data that we update every frame for each draw item, so cache that here
                        CacheRootConstantInterval(instanceGroupData);
                    }
                }

                // For mesh instancing only
                // If this model needs to draw motion, enable draw motion vector for the DrawPacket.
                // This means any mesh instances which are using this draw packet would draw motion vector too. This is fine, just not optimized.
                if (r_meshInstancingEnabled && m_flags.m_isDrawMotion)
                {
                    MeshInstanceGroupData& instanceGroupData = meshInstanceManager[instanceGroupInsertResult.m_handle];
                    if (!instanceGroupData.m_isDrawMotion)
                    {
                        instanceGroupData.m_isDrawMotion = true;
                        instanceGroupData.m_drawPacket.SetEnableDraw(meshMotionDrawListTag, true);
                    }
                }
            }
        }

        void ModelDataInstance::SetRayTracingData(MeshFeatureProcessor* meshFeatureProcessor)
        {
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = meshFeatureProcessor->GetRayTracingFeatureProcessor();
            TransformServiceFeatureProcessor* transformServiceFeatureProcessor =
                meshFeatureProcessor->GetTransformServiceFeatureProcessor();
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
                const auto meshes = modelLod->GetMeshes();
                const RPI::ModelLod::Mesh& mesh = meshes[meshIndex];

                // retrieve the material
                const CustomMaterialId customMaterialId(rayTracingLod, mesh.m_materialSlotStableId);
                const auto& customMaterialInfo = GetCustomMaterialWithFallback(customMaterialId);
                const auto& material = customMaterialInfo.m_material ? customMaterialInfo.m_material : mesh.m_material;

                if (!material)
                {
                    AZ_Warning("MeshFeatureProcessor", false, "No material provided for mesh. Skipping.");
                    continue;
                }

                // retrieve vertex/index buffers
                RHI::InputStreamLayout inputStreamLayout;
                RHI::StreamBufferIndices streamIndices;

                [[maybe_unused]] bool result = modelLod->GetStreamsForMesh(
                    inputStreamLayout,
                    streamIndices,
                    nullptr,
                    shaderInputContract,
                    meshIndex,
                    customMaterialInfo.m_uvMapping,
                    material->GetAsset()->GetMaterialTypeAsset()->GetUvNameMap());
                AZ_Assert(result, "Failed to retrieve mesh stream buffer views");

                // The code below expects streams for positions, normals, tangents, bitangents, and uvs.
                constexpr size_t NumExpectedStreams = 5;
                if (streamIndices.Size() < NumExpectedStreams)
                {
                    AZ_Warning("MeshFeatureProcessor", false, "Model is missing one or more expected streams "
                        "(positions, normals, tangents, bitangents, uvs), skipping the raytracing data generation.");
                    continue;
                }

                auto streamIter = mesh.CreateStreamIterator(streamIndices);

                // note that the element count is the size of the entire buffer, even though this mesh may only
                // occupy a portion of the vertex buffer.  This is necessary since we are accessing it using
                // a ByteAddressBuffer in the raytracing shaders and passing the byte offset to the shader in a constant buffer.
                uint32_t positionBufferByteCount = static_cast<uint32_t>(const_cast<RHI::Buffer*>(streamIter[0].GetBuffer())->GetDescriptor().m_byteCount);
                RHI::BufferViewDescriptor positionBufferDescriptor = RHI::BufferViewDescriptor::CreateRaw(0, positionBufferByteCount);

                uint32_t normalBufferByteCount = static_cast<uint32_t>(const_cast<RHI::Buffer*>(streamIter[1].GetBuffer())->GetDescriptor().m_byteCount);
                RHI::BufferViewDescriptor normalBufferDescriptor = RHI::BufferViewDescriptor::CreateRaw(0, normalBufferByteCount);

                uint32_t tangentBufferByteCount = static_cast<uint32_t>(const_cast<RHI::Buffer*>(streamIter[2].GetBuffer())->GetDescriptor().m_byteCount);
                RHI::BufferViewDescriptor tangentBufferDescriptor = RHI::BufferViewDescriptor::CreateRaw(0, tangentBufferByteCount);

                uint32_t bitangentBufferByteCount = static_cast<uint32_t>(const_cast<RHI::Buffer*>(streamIter[3].GetBuffer())->GetDescriptor().m_byteCount);
                RHI::BufferViewDescriptor bitangentBufferDescriptor = RHI::BufferViewDescriptor::CreateRaw(0, bitangentBufferByteCount);

                uint32_t uvBufferByteCount = static_cast<uint32_t>(const_cast<RHI::Buffer*>(streamIter[4].GetBuffer())->GetDescriptor().m_byteCount);
                RHI::BufferViewDescriptor uvBufferDescriptor = RHI::BufferViewDescriptor::CreateRaw(0, uvBufferByteCount);

                const RHI::IndexBufferView& indexBufferView = mesh.GetIndexBufferView();
                uint32_t indexElementSize = indexBufferView.GetIndexFormat() == RHI::IndexFormat::Uint16 ? 2 : 4;
                uint32_t indexElementCount = (uint32_t)indexBufferView.GetBuffer()->GetDescriptor().m_byteCount / indexElementSize;
                RHI::BufferViewDescriptor indexBufferDescriptor;
                indexBufferDescriptor.m_elementOffset = 0;
                indexBufferDescriptor.m_elementCount = indexElementCount;
                indexBufferDescriptor.m_elementSize = indexElementSize;
                indexBufferDescriptor.m_elementFormat = indexBufferView.GetIndexFormat() == RHI::IndexFormat::Uint16 ? RHI::Format::R16_UINT : RHI::Format::R32_UINT;

                // set the SubMesh data to pass to the RayTracingFeatureProcessor, starting with vertex/index data
                RayTracingFeatureProcessor::SubMesh subMesh;
                RayTracingFeatureProcessor::SubMeshMaterial& subMeshMaterial = subMesh.m_material;
                subMesh.m_positionFormat = PositionStreamFormat;
                subMesh.m_positionVertexBufferView = streamIter[0];
                subMesh.m_positionShaderBufferView = const_cast<RHI::Buffer*>(streamIter[0].GetBuffer())->BuildBufferView(positionBufferDescriptor);

                subMesh.m_normalFormat = NormalStreamFormat;
                subMesh.m_normalVertexBufferView = streamIter[1];
                subMesh.m_normalShaderBufferView = const_cast<RHI::Buffer*>(streamIter[1].GetBuffer())->BuildBufferView(normalBufferDescriptor);

                if (tangentBufferByteCount > 0)
                {
                    subMesh.m_bufferFlags |= RayTracingSubMeshBufferFlags::Tangent;
                    subMesh.m_tangentFormat = TangentStreamFormat;
                    subMesh.m_tangentVertexBufferView = streamIter[2];
                    subMesh.m_tangentShaderBufferView = const_cast<RHI::Buffer*>(streamIter[2].GetBuffer())->BuildBufferView(tangentBufferDescriptor);
                }

                if (bitangentBufferByteCount > 0)
                {
                    subMesh.m_bufferFlags |= RayTracingSubMeshBufferFlags::Bitangent;
                    subMesh.m_bitangentFormat = BitangentStreamFormat;
                    subMesh.m_bitangentVertexBufferView = streamIter[3];
                    subMesh.m_bitangentShaderBufferView = const_cast<RHI::Buffer*>(streamIter[3].GetBuffer())->BuildBufferView(bitangentBufferDescriptor);
                }

                if (uvBufferByteCount > 0)
                {
                    subMesh.m_bufferFlags |= RayTracingSubMeshBufferFlags::UV;
                    subMesh.m_uvFormat = UVStreamFormat;
                    subMesh.m_uvVertexBufferView = streamIter[4];
                    subMesh.m_uvShaderBufferView = const_cast<RHI::Buffer*>(streamIter[4].GetBuffer())->BuildBufferView(uvBufferDescriptor);
                }

                subMesh.m_indexBufferView = mesh.GetIndexBufferView();
                subMesh.m_indexShaderBufferView = const_cast<RHI::Buffer*>(mesh.GetIndexBufferView().GetBuffer())->BuildBufferView(indexBufferDescriptor);

                // add material data
                if (material)
                {
                    RPI::MaterialPropertyIndex propertyIndex;

                    // base color
                    propertyIndex = material->FindPropertyIndex(s_baseColor_color_Name);
                    if (propertyIndex.IsValid())
                    {
                        subMeshMaterial.m_baseColor = material->GetPropertyValue<AZ::Color>(propertyIndex);
                    }

                    propertyIndex = material->FindPropertyIndex(s_baseColor_factor_Name);
                    if (propertyIndex.IsValid())
                    {
                        subMeshMaterial.m_baseColor *= material->GetPropertyValue<float>(propertyIndex);
                    }

                    // metallic
                    propertyIndex = material->FindPropertyIndex(s_metallic_factor_Name);
                    if (propertyIndex.IsValid())
                    {
                        subMeshMaterial.m_metallicFactor = material->GetPropertyValue<float>(propertyIndex);
                    }

                    // roughness
                    propertyIndex = material->FindPropertyIndex(s_roughness_factor_Name);
                    if (propertyIndex.IsValid())
                    {
                        subMeshMaterial.m_roughnessFactor = material->GetPropertyValue<float>(propertyIndex);
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
                                subMeshMaterial.m_emissiveColor = material->GetPropertyValue<AZ::Color>(propertyIndex);
                            }

                            // When we have an emissive intensity, the unit of the intensity is defined in the material settings.
                            // For non-raytracing materials, the intensity is converted, and set in the shader, by a Functor.
                            // This (and the other) Functors are normally called in the Compile function of the Material
                            // We can't use the Compile function here, because the raytracing material behaves bit differently
                            // Therefor we need to look for the right Functor to convert the intensity here
                            propertyIndex = material->FindPropertyIndex(s_emissive_intensity_Name);
                            if (propertyIndex.IsValid())
                            {
                                auto unitPropertyIndex = material->FindPropertyIndex(s_emissive_unit_Name);
                                AZ_WarningOnce(
                                    "MeshFeatureProcessor",
                                    propertyIndex.IsValid(),
                                    "Emissive intensity property missing in material %s. Materials with an emissive intensity need a unit for the intensity.",
                                    material->GetAsset()->GetId().ToFixedString().c_str());
                                if (unitPropertyIndex.IsValid())
                                {
                                    auto intensity = material->GetPropertyValue<float>(propertyIndex);
                                    auto unit = material->GetPropertyValue<uint32_t>(unitPropertyIndex);
                                    bool foundEmissiveUnitFunctor = false;
                                    for (const auto& functor : material->GetAsset()->GetMaterialFunctors())
                                    {
                                        auto emissiveFunctor = azdynamic_cast<ConvertEmissiveUnitFunctor*>(functor);
                                        if (emissiveFunctor != nullptr)
                                        {
                                            intensity = emissiveFunctor->GetProcessedValue(intensity, unit);
                                            foundEmissiveUnitFunctor = true;
                                            break;
                                        }
                                    }
                                    AZ_WarningOnce(
                                        "MeshFeatureProcessor",
                                        foundEmissiveUnitFunctor,
                                        "Could not find ConvertEmissiveUnitFunctor for material %s",
                                        material->GetAsset()->GetId().ToFixedString().c_str());
                                    if (foundEmissiveUnitFunctor)
                                    {
                                        subMeshMaterial.m_emissiveColor *= intensity;
                                    }
                                }
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
                            subMeshMaterial.m_textureFlags |= RayTracingSubMeshTextureFlags::BaseColor;
                            subMeshMaterial.m_baseColorImageView = image->GetImageView();
                            baseColorImage = image;
                        }
                    }

                    propertyIndex = material->FindPropertyIndex(s_normal_textureMap_Name);
                    if (propertyIndex.IsValid())
                    {
                        Data::Instance<RPI::Image> image = material->GetPropertyValue<Data::Instance<RPI::Image>>(propertyIndex);
                        if (image.get())
                        {
                            subMeshMaterial.m_textureFlags |= RayTracingSubMeshTextureFlags::Normal;
                            subMeshMaterial.m_normalImageView = image->GetImageView();
                        }
                    }

                    propertyIndex = material->FindPropertyIndex(s_metallic_textureMap_Name);
                    if (propertyIndex.IsValid())
                    {
                        Data::Instance<RPI::Image> image = material->GetPropertyValue<Data::Instance<RPI::Image>>(propertyIndex);
                        if (image.get())
                        {
                            subMeshMaterial.m_textureFlags |= RayTracingSubMeshTextureFlags::Metallic;
                            subMeshMaterial.m_metallicImageView = image->GetImageView();
                        }
                    }

                    propertyIndex = material->FindPropertyIndex(s_roughness_textureMap_Name);
                    if (propertyIndex.IsValid())
                    {
                        Data::Instance<RPI::Image> image = material->GetPropertyValue<Data::Instance<RPI::Image>>(propertyIndex);
                        if (image.get())
                        {
                            subMeshMaterial.m_textureFlags |= RayTracingSubMeshTextureFlags::Roughness;
                            subMeshMaterial.m_roughnessImageView = image->GetImageView();
                        }
                    }

                    propertyIndex = material->FindPropertyIndex(s_emissive_textureMap_Name);
                    if (propertyIndex.IsValid())
                    {
                        Data::Instance<RPI::Image> image = material->GetPropertyValue<Data::Instance<RPI::Image>>(propertyIndex);
                        if (image.get())
                        {
                            subMeshMaterial.m_textureFlags |= RayTracingSubMeshTextureFlags::Emissive;
                            subMeshMaterial.m_emissiveImageView = image->GetImageView();
                        }
                    }

                    // irradiance color
                    SetIrradianceData(subMesh, material, baseColorImage);
                }

                subMeshes.push_back(subMesh);
            }

            // setup the RayTracing Mesh
            RayTracingFeatureProcessor::Mesh rayTracingMesh;
            rayTracingMesh.m_assetId = m_model->GetModelAsset()->GetId();
            rayTracingMesh.m_transform = transformServiceFeatureProcessor->GetTransformForId(m_objectId);
            rayTracingMesh.m_nonUniformScale = transformServiceFeatureProcessor->GetNonUniformScaleForId(m_objectId);
            rayTracingMesh.m_isSkinnedMesh = m_descriptor.m_isSkinnedMesh;
            rayTracingMesh.m_instanceMask |= (rayTracingMesh.m_isSkinnedMesh)
                ? static_cast<uint32_t>(AZ::RHI::RayTracingAccelerationStructureInstanceInclusionMask::SKINNED_MESH)
                : static_cast<uint32_t>(AZ::RHI::RayTracingAccelerationStructureInstanceInclusionMask::STATIC_MESH);

            // setup the reflection probe data, and track if this mesh is currently affected by a reflection probe
            SetRayTracingReflectionProbeData(meshFeatureProcessor, rayTracingMesh.m_reflectionProbe);

            // add the mesh
            rayTracingFeatureProcessor->AddMesh(m_rayTracingUuid, rayTracingMesh, subMeshes);
            m_flags.m_needsSetRayTracingData = false;
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
            RayTracingFeatureProcessor::SubMeshMaterial& subMeshMaterial = subMesh.m_material;

            if (irradianceColorSource.IsEmpty() || irradianceColorSource == s_Manual_Name)
            {
                propertyIndex = material->FindPropertyIndex(s_irradiance_manualColor_Name);
                if (propertyIndex.IsValid())
                {
                    subMeshMaterial.m_irradianceColor = material->GetPropertyValue<AZ::Color>(propertyIndex);
                }
                else
                {
                    // Couldn't find irradiance.manualColor -> check for an irradiance.color in case the material type
                    // doesn't have the concept of manual vs. automatic irradiance color, allow a simpler property name
                    propertyIndex = material->FindPropertyIndex(s_irradiance_color_Name);
                    if (propertyIndex.IsValid())
                    {
                        subMeshMaterial.m_irradianceColor = material->GetPropertyValue<AZ::Color>(propertyIndex);
                    }
                    else
                    {
                        AZ_Warning(
                            "MeshFeatureProcessor", false,
                            "No irradiance.manualColor or irradiance.color field found. Defaulting to 1.0f.");
                        subMeshMaterial.m_irradianceColor = AZ::Colors::White;
                    }
                }
            }
            else if (irradianceColorSource == s_BaseColorTint_Name)
            {
                // Use only the baseColor, no texture on top of it
                subMeshMaterial.m_irradianceColor = subMeshMaterial.m_baseColor;
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

                        // We do a simple 'multiply' blend with the base color
                        // Note: other blend modes are currently not supported
                        subMeshMaterial.m_irradianceColor = avgColor * subMeshMaterial.m_baseColor;
                    }
                    else
                    {
                        AZ_Warning("MeshFeatureProcessor", false, "Using BaseColor as irradianceColorSource "
                                "is currently only supported for textures of type StreamingImage");
                        // Default to the flat base color
                        subMeshMaterial.m_irradianceColor = subMeshMaterial.m_baseColor;
                    }
                }
                else
                {
                    // No texture, simply copy the baseColor
                    subMeshMaterial.m_irradianceColor = subMeshMaterial.m_baseColor;
                }
            }
            else
            {
                AZ_Warning("MeshFeatureProcessor", false, "Unknown irradianceColorSource value: %s, "
                        "defaulting to 1.0f.", irradianceColorSource.GetCStr());
                subMeshMaterial.m_irradianceColor = AZ::Colors::White;
            }


            // Overall scale factor
            propertyIndex = material->FindPropertyIndex(s_irradiance_factor_Name);
            if (propertyIndex.IsValid())
            {
                subMeshMaterial.m_irradianceColor *= material->GetPropertyValue<float>(propertyIndex);
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

            subMeshMaterial.m_irradianceColor.SetA(opacity);
        }

        void ModelDataInstance::SetRayTracingReflectionProbeData(
            MeshFeatureProcessor* meshFeatureProcessor,
            RayTracingFeatureProcessor::Mesh::ReflectionProbe& reflectionProbe)
        {
            TransformServiceFeatureProcessor* transformServiceFeatureProcessor = meshFeatureProcessor->GetTransformServiceFeatureProcessor();
            ReflectionProbeFeatureProcessor* reflectionProbeFeatureProcessor = meshFeatureProcessor->GetReflectionProbeFeatureProcessor();
            AZ::Transform transform = transformServiceFeatureProcessor->GetTransformForId(m_objectId);

            // retrieve reflection probes
            Aabb aabbWS = m_aabb;
            aabbWS.ApplyTransform(transform);

            ReflectionProbeHandleVector reflectionProbeHandles;
            reflectionProbeFeatureProcessor->FindReflectionProbes(aabbWS, reflectionProbeHandles);

            m_flags.m_hasRayTracingReflectionProbe = !reflectionProbeHandles.empty();
            if (m_flags.m_hasRayTracingReflectionProbe)
            {
                // take the last handle from the list, which will be the smallest (most influential) probe
                ReflectionProbeHandle handle = reflectionProbeHandles.back();
                reflectionProbe.m_modelToWorld = reflectionProbeFeatureProcessor->GetTransform(handle);
                reflectionProbe.m_outerObbHalfLengths = reflectionProbeFeatureProcessor->GetOuterObbWs(handle).GetHalfLengths();
                reflectionProbe.m_innerObbHalfLengths = reflectionProbeFeatureProcessor->GetInnerObbWs(handle).GetHalfLengths();
                reflectionProbe.m_useParallaxCorrection = reflectionProbeFeatureProcessor->GetUseParallaxCorrection(handle);
                reflectionProbe.m_exposure = reflectionProbeFeatureProcessor->GetRenderExposure(handle);
                reflectionProbe.m_reflectionProbeCubeMap = reflectionProbeFeatureProcessor->GetCubeMap(handle);
            }
        }

        void ModelDataInstance::RemoveRayTracingData(RayTracingFeatureProcessor* rayTracingFeatureProcessor)
        {
            // remove from ray tracing
            if (rayTracingFeatureProcessor)
            {
                rayTracingFeatureProcessor->RemoveMesh(m_rayTracingUuid);
            }
        }

        void ModelDataInstance::SetSortKey(MeshFeatureProcessor* meshFeatureProcessor, RHI::DrawItemSortKey sortKey)
        {
            RHI::DrawItemSortKey previousSortKey = m_sortKey;
            m_sortKey = sortKey;
            if (previousSortKey != m_sortKey)
            {
                if (!r_meshInstancingEnabled)
                {
                    for (auto& drawPacketList : m_meshDrawPacketListsByLod)
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
                    if (m_model && !m_flags.m_needsInit)
                    {
                        // DeInit/ReInit is overkill (destroys and re-creates ray-tracing data)
                        // but it works for now since SetSortKey is infrequent
                        // Init needs to be called because that is where we determine what can be part of the same instance group,
                        // and the sort key is part of that.
                        ReInit(meshFeatureProcessor);
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
            AZ_Assert(!r_meshInstancingEnabled, "If mesh instancing is enabled, the draw packet update should be going through the MeshInstanceManager.");

            // Only enable draw motion if model is dynamic and draw motion was disabled
            bool enableDrawMotion = !m_flags.m_isDrawMotion && m_flags.m_dynamic;
            RHI::DrawListTag meshMotionDrawListTag;
            if (enableDrawMotion)
            {
                meshMotionDrawListTag = AZ::RHI::RHISystemInterface::Get()->GetDrawListTagRegistry()->FindTag(MeshCommon::MotionDrawListTagName);
            }

            uint32_t lodIndex = 0;
            for (auto& meshDrawPacketList : m_meshDrawPacketListsByLod)
            {
                uint32_t meshIndex = 0;
                for (auto& meshDrawPacket : meshDrawPacketList)
                {
                    if (enableDrawMotion)
                    {
                        meshDrawPacket.SetEnableDraw(meshMotionDrawListTag, true);
                    }
                    if (meshDrawPacket.Update(*m_scene, forceUpdate))
                    {
                        HandleDrawPacketUpdate(lodIndex, meshIndex, meshDrawPacket);
                    }
                    meshIndex++;
                }
                lodIndex++;
            }
        }

        void ModelDataInstance::BuildCullable()
        {
            AZ_Assert(m_flags.m_cullableNeedsRebuild, "This function only needs to be called if the cullable to be rebuilt");
            AZ_Assert(m_model, "The model has not finished loading yet");

            RPI::Cullable::CullData& cullData = m_cullable.m_cullData;
            RPI::Cullable::LodData& lodData = m_cullable.m_lodData;

            const Aabb& localAabb = m_aabb;
            lodData.m_lodSelectionRadius = 0.5f * localAabb.GetExtents().GetMaxElement();

            const size_t modelLodCount = m_model->GetLodCount();
            const auto& lodAssets = m_model->GetModelAsset()->GetLodAssets();
            AZ_Assert(lodAssets.size() == modelLodCount, "Number of asset lods must match number of model lods");
            AZ_Assert(m_lodBias <= modelLodCount - 1, "Incorrect lod bias");

            lodData.m_lods.resize(modelLodCount);
            cullData.m_drawListMask.reset();

            const size_t lodCount = lodAssets.size();

            for (size_t lodIndex = 0; lodIndex < lodCount; ++lodIndex)
            {
                //initialize the lod
                RPI::Cullable::LodData::Lod& lod = lodData.m_lods[lodIndex];
                // non-used lod (except if forced)
                if (lodIndex < m_lodBias)
                {
                    // set impossible screen coverage to disable it
                    lod.m_screenCoverageMax = 0.0f;
                    lod.m_screenCoverageMin = 1.0f;
                }
                else
                {
                    if (lodIndex == m_lodBias)
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
                }

                lod.m_drawPackets.clear();
                if (!r_meshInstancingEnabled)
                {
                    const RPI::MeshDrawPacketList& drawPacketList = m_meshDrawPacketListsByLod[lodIndex + m_lodBias];
                    for (const RPI::MeshDrawPacket& drawPacket : drawPacketList)
                    {
                        // If mesh instancing is disabled, get the draw packets directly from this ModelDataInstance
                        const RHI::DrawPacket* rhiDrawPacket = drawPacket.GetRHIDrawPacket();

                        if (rhiDrawPacket)
                        {
                            // OR-together all the drawListMasks (so we know which views to cull against)
                            cullData.m_drawListMask |= rhiDrawPacket->GetDrawListMask();

                            lod.m_drawPackets.push_back(rhiDrawPacket);
                        }
                    }
                }
                else
                {
                    const PostCullingInstanceDataList& postCullingInstanceDataList = m_postCullingInstanceDataByLod[lodIndex + m_lodBias];
                    for (const ModelDataInstance::PostCullingInstanceData& postCullingData : postCullingInstanceDataList)
                    {
                        // If mesh instancing is enabled, get the draw packet from the MeshInstanceManager
                        const RHI::DrawPacket* rhiDrawPacket = postCullingData.m_instanceGroupHandle->m_drawPacket.GetRHIDrawPacket();

                        if (rhiDrawPacket)
                        {
                            // OR-together all the drawListMasks (so we know which views to cull against)
                            cullData.m_drawListMask |= rhiDrawPacket->GetDrawListMask();
                        }

                        // Set the user data for the cullable lod to reference the intance group handles for the lod
                        lod.m_visibleObjectUserData = static_cast<void*>(&m_postCullingInstanceDataByLod[lodIndex + m_lodBias]);
                    }
                }
            }

            cullData.m_hideFlags = RPI::View::UsageNone;
            if (m_descriptor.m_excludeFromReflectionCubeMaps)
            {
                cullData.m_hideFlags |= RPI::View::UsageReflectiveCubeMap;
            }

#ifdef AZ_CULL_DEBUG_ENABLED
            m_cullable.SetDebugName(AZ::Name(AZStd::string::format("%s - objectId: %u", m_model->GetModelAsset()->GetName().GetCStr(), m_objectId.GetIndex())));
#endif

            m_flags.m_cullableNeedsRebuild = false;
            m_flags.m_cullBoundsNeedsUpdate = true;
        }

        void ModelDataInstance::UpdateCullBounds(const MeshFeatureProcessor* meshFeatureProcessor)
        {
            AZ_Assert(m_flags.m_cullBoundsNeedsUpdate, "This function only needs to be called if the culling bounds need to be rebuilt");
            AZ_Assert(m_model, "The model has not finished loading yet");
            const TransformServiceFeatureProcessor* transformService = meshFeatureProcessor->GetTransformServiceFeatureProcessor();
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
            m_cullable.m_cullData.m_entityId = m_descriptor.m_entityId;
            if (!r_meshInstancingEnabled)
            {
                m_cullable.m_cullData.m_visibilityEntry.m_typeFlags = AzFramework::VisibilityEntry::TYPE_RPI_Cullable;
            }
            else
            {
                m_cullable.m_cullData.m_visibilityEntry.m_typeFlags = AzFramework::VisibilityEntry::TYPE_RPI_VisibleObjectList;
            }
            m_scene->GetCullingScene()->RegisterOrUpdateCullable(m_cullable);

            m_flags.m_cullBoundsNeedsUpdate = false;
        }

        void ModelDataInstance::UpdateObjectSrg(MeshFeatureProcessor* meshFeatureProcessor)
        {
            ReflectionProbeFeatureProcessor* reflectionProbeFeatureProcessor = meshFeatureProcessor->GetReflectionProbeFeatureProcessor();
            TransformServiceFeatureProcessor* transformServiceFeatureProcessor = meshFeatureProcessor->GetTransformServiceFeatureProcessor();
            for (auto& objectSrg : m_objectSrgList)
            {
                if (reflectionProbeFeatureProcessor && (m_descriptor.m_useForwardPassIblSpecular || m_flags.m_hasForwardPassIblSpecularMaterial))
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

                RHI::ShaderInputConstantIndex lightingChannelMaskIndex = objectSrg->FindShaderInputConstantIndex(AZ::Name("m_lightingChannelMask"));
                if (lightingChannelMaskIndex.IsValid())
                {
                    objectSrg->SetConstant(lightingChannelMaskIndex, m_lightingChannelMask);
                }

                objectSrg->Compile();
            }

            // Set m_objectSrgNeedsUpdate to false if there are object SRGs in the list
            m_flags.m_objectSrgNeedsUpdate = m_flags.m_objectSrgNeedsUpdate && (m_objectSrgList.size() == 0);
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
            m_flags.m_visible = isVisible;
            m_cullable.m_isHidden = !isVisible;
        }

        CustomMaterialInfo ModelDataInstance::GetCustomMaterialWithFallback(const CustomMaterialId& id) const
        {
            const CustomMaterialId ignoreLodId(DefaultCustomMaterialLodIndex, id.second);
            for (const auto& currentId : { id, ignoreLodId, DefaultCustomMaterialId })
            {
                if (auto itr = m_descriptor.m_customMaterials.find(currentId); itr != m_descriptor.m_customMaterials.end() && itr->second.m_material)
                {
                    return itr->second;
                }
            }
            return CustomMaterialInfo{};
        }

        void ModelDataInstance::HandleDrawPacketUpdate(uint32_t lodIndex, uint32_t meshIndex, RPI::MeshDrawPacket& meshDrawPacket)
        {
            // When the drawpacket is updated, the cullable must be rebuilt to use the latest draw packet
            m_flags.m_cullableNeedsRebuild = true;
            m_meshDrawPacketUpdatedEvent.Signal(*this, lodIndex, meshIndex, meshDrawPacket);
        }

        void ModelDataInstance::ConnectMeshDrawPacketUpdatedHandler(MeshDrawPacketUpdatedEvent::Handler& handler)
        {
            if (handler.IsConnected())
            {
                handler.Disconnect();
            }
            handler.Connect(m_meshDrawPacketUpdatedEvent);
        }

    } // namespace Render
} // namespace AZ
