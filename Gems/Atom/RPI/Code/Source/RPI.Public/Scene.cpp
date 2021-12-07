/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Culling.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawSystem.h>
#include <Atom/RPI.Public/FeatureProcessorFactory.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Pass/RasterPass.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/View.h>

#include <AzCore/Debug/EventTrace.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobEmpty.h>

#include <AzCore/Task/TaskGraph.h>

#include <AzFramework/Entity/EntityContext.h>

namespace AZ
{
    namespace RPI
    {
        ScenePtr Scene::CreateScene(const SceneDescriptor& sceneDescriptor)
        {
            Scene* scene = aznew Scene();
            for (const auto& fpId : sceneDescriptor.m_featureProcessorNames)
            {
                scene->EnableFeatureProcessor(FeatureProcessorId{ fpId });
            }

            auto sceneSrgLayout = RPISystemInterface::Get()->GetSceneSrgLayout();
            if (sceneSrgLayout)
            {
                auto shaderAsset = RPISystemInterface::Get()->GetCommonShaderAssetForSrgs();
                scene->m_srg = ShaderResourceGroup::Create(shaderAsset, sceneSrgLayout->GetName());
                
                // Set value for constants defined in SceneTimeSrg.azsli
                scene->m_timeInputIndex = scene->m_srg->FindShaderInputConstantIndex(Name{ "m_time" });
            }

            scene->m_name = sceneDescriptor.m_nameId;

            return ScenePtr(scene);
        }

        ScenePtr Scene::CreateSceneFromAsset(Data::Asset<AnyAsset> sceneAsset)
        {
            const SceneDescriptor* sceneDescriptor = GetDataFromAnyAsset<SceneDescriptor>(sceneAsset);
            if (sceneDescriptor == nullptr)
            {
                return nullptr;
            }

            ScenePtr scene = Scene::CreateScene(*sceneDescriptor);
            if (scene == nullptr)
            {
                AZ_Error("RPISystem", false, "Failed to create scene from asset %s", sceneAsset.GetHint().c_str());
                return nullptr;
            }

            return scene;
        }

        Scene* Scene::GetSceneForEntityContextId(AzFramework::EntityContextId entityContextId)
        {
            // Find the scene for this entity context.
            AZStd::shared_ptr<AzFramework::Scene> scene = AzFramework::EntityContext::FindContainingScene(entityContextId);
            if (scene)
            {
                // Get the RPI::Scene subsystem from the AZFramework Scene.
                RPI::ScenePtr* scenePtr = scene->FindSubsystem<RPI::ScenePtr>();
                if (scenePtr)
                {
                    return scenePtr->get();
                }
            }
            return nullptr;
        }

        Scene* Scene::GetSceneForEntityId(AZ::EntityId entityId)
        {
            // Find the entity context for the entity ID.
            AzFramework::EntityContextId entityContextId = AzFramework::EntityContextId::CreateNull();
            AzFramework::EntityIdContextQueryBus::EventResult(entityContextId, entityId, &AzFramework::EntityIdContextQueryBus::Events::GetOwningContextId);

            if (!entityContextId.IsNull())
            {
                return GetSceneForEntityContextId(entityContextId);
            }
            return nullptr;
        }


        Scene::Scene()
        {
            m_id = AZ::Uuid::CreateRandom();
            m_cullingScene = aznew CullingScene();
            SceneRequestBus::Handler::BusConnect(m_id);
            m_drawFilterTagRegistry = RHI::DrawFilterTagRegistry::Create();
        }

        Scene::~Scene()
        {
            if (m_taskGraphActive)
            {
                WaitAndCleanTGEvent(AZStd::move(m_simulationFinishedTGEvent));
            }
            else
            {
                WaitAndCleanCompletionJob(m_simulationCompletion);
            }
            SceneRequestBus::Handler::BusDisconnect();

            // Remove all the render pipelines. Need to process queued changes with pass system before and after remove render pipelines
            AZ::RPI::PassSystemInterface::Get()->ProcessQueuedChanges();
            for (auto it = m_pipelines.begin(); it != m_pipelines.end(); ++it)
            {
                RenderPipelinePtr pipelineToRemove = (*it);
                pipelineToRemove->OnRemovedFromScene(this);
            }
            m_pipelines.clear();
            AZ::RPI::PassSystemInterface::Get()->ProcessQueuedChanges();

            Deactivate();

            delete m_cullingScene;
        }

        void Scene::Activate()
        {
            AZ_Assert(!m_activated, "Already activated");

            m_activated = true;

            m_cullingScene->Activate(this);

            // We have to tick the PassSystem in order for all the pass attachments to get created. 
            // This has to be done before FeatureProcessors are activated, because they may try to
            // create PipelineState objects (PSOs) which would require data from attachments in the
            // the pass tree.
            AZ::RPI::PassSystemInterface::Get()->ProcessQueuedChanges();
            
            for (auto& fp : m_featureProcessors)
            {
                fp->Activate();
            }

            m_dynamicDrawSystem = static_cast<DynamicDrawSystem*>(RPI::DynamicDrawInterface::Get());
        }

        void Scene::Deactivate()
        {
            if (!m_activated)
            {
                return;
            }
            
            for (auto& fp : m_featureProcessors)
            {
                fp->Deactivate();
            }

            m_cullingScene->Deactivate();

            m_activated = false;
            m_pipelineStatesLookup.clear();
            m_dynamicDrawSystem = nullptr;
        }

        FeatureProcessor* Scene::EnableFeatureProcessor(const FeatureProcessorId& featureProcessorId)
        {
            FeatureProcessor* foundFeatureProcessor = GetFeatureProcessor(featureProcessorId);

            if (foundFeatureProcessor)
            {
                AZ_Warning("Scene", false, "FeatureProcessor '%s' is already enabled for this scene. Will not re-enable.", featureProcessorId.GetCStr());
                return foundFeatureProcessor;
            }

            // Check to make sure there aren't multiple different implementations of the same interface enabled for the scene
            // otherwise it would be ambiguous which feature processor is returned when GetFeatureProcessor is called with the interface.
            AZ::TypeId interfaceTypeId = FeatureProcessorFactory::Get()->GetFeatureProcessorInterfaceTypeId(featureProcessorId);
            if (!interfaceTypeId.IsNull())
            {
                foundFeatureProcessor = GetFeatureProcessor(interfaceTypeId);

                if (foundFeatureProcessor)
                {
                    AZ_Error("Scene", false, "FeatureProcessor '%s' is already enabled for this scene, which implements the same interface as %s. You cannot have more than one implementation of the same feature processor interface in a scene.", foundFeatureProcessor->RTTI_GetTypeName(), featureProcessorId.GetCStr());
                    return foundFeatureProcessor;
                }
            }

            // The feature processor was not found, so create it
            FeatureProcessorPtr createdFeatureProcessor = FeatureProcessorFactory::Get()->CreateFeatureProcessor(featureProcessorId);
            if (!createdFeatureProcessor)
            {
                AZ_Error("Scene", false, "FeatureProcessor '%s' could not be created. Check to make sure it has been registered with the FeatureProcessorFactory.", featureProcessorId.GetCStr());
                return nullptr;
            }

            foundFeatureProcessor = createdFeatureProcessor.get();

            AddFeatureProcessor(AZStd::move(createdFeatureProcessor));

            return foundFeatureProcessor;
        }
        
        void Scene::AddFeatureProcessor(FeatureProcessorPtr fp)
        {
            fp->m_parentScene = this;

            // If the Scene is not active then we should not activate the new FeatureProcessor either. 
            // In this case, the FeatureProcessor will be activated when Scene::Activate() is called.
            if (m_activated)
            {
                fp->Activate();
            }

            m_featureProcessors.emplace_back(AZStd::move(fp));
        }

        void Scene::EnableAllFeatureProcessors()
        {
            FeatureProcessorFactory::Get()->EnableAllForScene(this);
        }

        void Scene::DisableFeatureProcessor(const FeatureProcessorId& featureProcessorId)
        {
            auto foundFeatureProcessor = AZStd::find_if(
                AZStd::begin(m_featureProcessors),
                AZStd::end(m_featureProcessors),
                [featureProcessorId](const FeatureProcessorPtr& fp) { return FeatureProcessorId{ fp->RTTI_GetTypeName() } == featureProcessorId; });

            if (foundFeatureProcessor != AZStd::end(m_featureProcessors))
            {
                // If the Scene is not active then the removed FeatureProcessor is not active either, so no need to deactivate it.
                if (m_activated)
                {
                    (*foundFeatureProcessor)->Deactivate();
                }

                m_featureProcessors.erase(foundFeatureProcessor);
            }
            else
            {
                AZ_Warning("Scene", false, "FeatureProcessor '%s' is already disabled for this scene. Will not re-disable.", featureProcessorId.GetCStr());
            }
        }

        void Scene::DisableAllFeatureProcessors()
        {
            for (auto& fp : m_featureProcessors)
            {
                fp->Deactivate();
            }
            m_featureProcessors.clear();
        }
        
        FeatureProcessor* Scene::GetFeatureProcessor(const FeatureProcessorId& featureProcessorId) const
        {
            AZ::TypeId featureProcessorTypeId = FeatureProcessorFactory::Get()->GetFeatureProcessorTypeId(featureProcessorId);

            return GetFeatureProcessor(featureProcessorTypeId);
        }
        
        FeatureProcessor* Scene::GetFeatureProcessor(const TypeId& featureProcessorTypeId) const
        {
            auto foundFP = AZStd::find_if(
                AZStd::begin(m_featureProcessors),
                AZStd::end(m_featureProcessors),
                [featureProcessorTypeId](const FeatureProcessorPtr& fp) { return fp->RTTI_IsTypeOf(featureProcessorTypeId); });

            return foundFP == AZStd::end(m_featureProcessors) ? nullptr : (*foundFP).get();
        }

        void Scene::AddRenderPipeline(RenderPipelinePtr pipeline)
        {
            if (pipeline->m_scene != nullptr)
            {
                AZ_Assert(false, "Pipeline was added to another scene");
                return;
            }

            auto pipelineId = pipeline->GetId();
            if (GetRenderPipeline(pipelineId))
            {
                AZ_Assert(false, "Pipeline with same name id is already added to this scene. Please set the pipeline with a different id");
                return;
            }

            pipeline->SetDrawFilterTag(m_drawFilterTagRegistry->AcquireTag(pipelineId));

            m_pipelines.push_back(pipeline);

            // Set this pipeline as default if the default pipeline was empty. This pipeline should be the first pipeline be added to the scene
            if (m_defaultPipeline == nullptr)
            {
                m_defaultPipeline = pipeline;
            }

            pipeline->OnAddedToScene(this);
            PassSystemInterface::Get()->ProcessQueuedChanges();
            pipeline->BuildPipelineViews();

            // Force to update the lookup table since adding render pipeline would effect any pipeline states created before pass system tick
            RebuildPipelineStatesLookup();

            SceneNotificationBus::Event(m_id, &SceneNotification::OnRenderPipelineAdded, pipeline);
        }
        
        void Scene::RemoveRenderPipeline(const RenderPipelineId& pipelineId)
        {
            bool removed = false;
            for (auto it = m_pipelines.begin(); it != m_pipelines.end(); ++it)
            {
                if (pipelineId == (*it)->GetId())
                {
                    // process queued changes first before remove pipeline passes
                    AZ::RPI::PassSystemInterface::Get()->ProcessQueuedChanges();
                    RenderPipelinePtr pipelineToRemove = (*it);

                    if (m_defaultPipeline == pipelineToRemove)
                    {
                        m_defaultPipeline = nullptr;
                    }

                    m_drawFilterTagRegistry->ReleaseTag(pipelineToRemove->GetDrawFilterTag());

                    pipelineToRemove->OnRemovedFromScene(this);
                    m_pipelines.erase(it);
                    
                    SceneNotificationBus::Event(m_id, &SceneNotification::OnRenderPipelineRemoved, pipelineToRemove.get());

                    // If the default pipeline was removed, set to the first one in the list
                    if (m_defaultPipeline == nullptr && m_pipelines.size() > 0)
                    {
                        m_defaultPipeline = m_pipelines[0];
                    }

                    AZ::RPI::PassSystemInterface::Get()->ProcessQueuedChanges();
                    RebuildPipelineStatesLookup();

                    removed = true;
                    break;
                }
            }
            
            AZ_Assert(removed, "Pipeline %s is not added to this Scene", pipelineId.GetCStr());
        }

        RenderPipelinePtr Scene::GetRenderPipeline(const RenderPipelineId& pipelineId) const
        {
            for (auto& pipeline : m_pipelines)
            {
                if (pipeline->GetId() == pipelineId)
                {
                    return pipeline;
                }
            }
            return nullptr;
        }

        void Scene::SimulateTaskGraph()
        {
            static const AZ::TaskDescriptor simulationTGDesc{"RPI::Scene::Simulate", "Graphics"};
            AZ::TaskGraph simulationTG;

            for (FeatureProcessorPtr& fp : m_featureProcessors)
            {
                FeatureProcessor* featureProcessor = fp.get();
                simulationTG.AddTask(
                    simulationTGDesc,
                    [this, featureProcessor]()
                    {
                        featureProcessor->Simulate(m_simulatePacket);
                    });
            }
            simulationTG.Detach();
            m_simulationFinishedTGEvent = AZStd::make_unique<TaskGraphEvent>();
            simulationTG.Submit(m_simulationFinishedTGEvent.get());
        }

        void Scene::SimulateJobs()
        {
            // Create a new job to track completion.
            m_simulationCompletion = aznew AZ::JobCompletion();

            for (FeatureProcessorPtr& fp : m_featureProcessors)
            {
                FeatureProcessor* featureProcessor = fp.get();
                const auto jobLambda = [this, featureProcessor](AZ::Job& owner)
                {
                    FeatureProcessor::SimulatePacket jobPacket = m_simulatePacket;
                    jobPacket.m_parentJob = &owner;
                    featureProcessor->Simulate(jobPacket);
                };

                AZ::Job* simulationJob = AZ::CreateJobFunction(AZStd::move(jobLambda), true, nullptr);  //auto-deletes
                simulationJob->SetDependent(m_simulationCompletion);
                simulationJob->Start();
            }
            //[GFX TODO]: the completion job should start here
        }

        void Scene::Simulate(RHI::JobPolicy jobPolicy, float simulationTime)
        {
            AZ_PROFILE_SCOPE(RPI, "Scene: Simulate");

            m_simulationTime = simulationTime;

            // If previous simulation job wasn't done, wait for it to finish.
            if (m_taskGraphActive)
            {
                WaitAndCleanTGEvent(AZStd::move(m_simulationFinishedTGEvent));
            }
            else
            {
                WaitAndCleanCompletionJob(m_simulationCompletion);
            }

            auto taskGraphActiveInterface = AZ::Interface<AZ::TaskGraphActiveInterface>::Get();
            m_taskGraphActive = taskGraphActiveInterface && taskGraphActiveInterface->IsTaskGraphActive();

            if (jobPolicy == RHI::JobPolicy::Serial)
            {
                for (auto& fp : m_featureProcessors)
                {
                    fp->Simulate(m_simulatePacket);
                }
            }
            else
            {
                if (m_taskGraphActive)
                {
                    SimulateTaskGraph();
                }
                else
                {
                    SimulateJobs();
                }
            }
        }

        void Scene::WaitAndCleanTGEvent(AZStd::unique_ptr<AZ::TaskGraphEvent>&&  completionTGEvent)
        {
            AZ_PROFILE_SCOPE(RPI, "Scene: WaitAndCleanTGEvent");
            if (completionTGEvent)
            {
                completionTGEvent->Wait();
            }
            // allow completionTGEvent to go out of scope and be deleted
        }

        void Scene::WaitAndCleanCompletionJob(AZ::JobCompletion*& completionJob)
        {
            if (completionJob)
            {
                AZ_PROFILE_SCOPE(RPI, "Scene: WaitAndCleanCompletionJob");
                //[GFX TODO]: the completion job should start earlier and wait for completion here
                completionJob->StartAndWaitForCompletion();
                delete completionJob;
                completionJob = nullptr;
            }
        }
        
        void Scene::ConnectEvent(PrepareSceneSrgEvent::Handler& handler)
        {
            handler.Connect(m_prepareSrgEvent);
        }

        void Scene::PrepareSceneSrg()
        {
            if (m_srg)
            {
                if (m_timeInputIndex.IsValid())
                {
                    m_srg->SetConstant(m_timeInputIndex, m_simulationTime);
                }

                // signal any handlers to update values for their partial scene srg
                m_prepareSrgEvent.Signal(m_srg.get());

                m_srg->Compile();
            }
        }

        void Scene::CollectDrawPacketsTaskGraph()
        {
                AZ_PROFILE_SCOPE(RPI, "CollectDrawPacketsTaskGraph");
                AZ::TaskGraphEvent collectDrawPacketsTGEvent;
                static const AZ::TaskDescriptor collectDrawPacketsTGDesc{"RPI_Scene_PrepareRender_CollectDrawPackets", "Graphics"};
                AZ::TaskGraph collectDrawPacketsTG;

                // Launch FeatureProcessor::Render() taskgraphs
                for (auto& fp : m_featureProcessors)
                {
                    collectDrawPacketsTG.AddTask( 
                        collectDrawPacketsTGDesc,
                        [this, &fp]()
                        {
                            fp->Render(m_renderPacket);
                        });

                }
                collectDrawPacketsTG.Submit(&collectDrawPacketsTGEvent);

                // Launch CullingSystem::ProcessCullables() jobs (will run concurrently with FeatureProcessor::Render() jobs if m_parallelOctreeTraversal)
                const bool parallelOctreeTraversal = m_cullingScene->GetDebugContext().m_parallelOctreeTraversal;
                m_cullingScene->BeginCulling(m_renderPacket.m_views);
                static const AZ::TaskDescriptor processCullablesDescriptor{"AZ::RPI::Scene::ProcessCullables", "Graphics"};
                AZ::TaskGraphEvent processCullablesTGEvent;
                AZ::TaskGraph processCullablesTG;
                if (parallelOctreeTraversal)
                {
                    for (ViewPtr& viewPtr : m_renderPacket.m_views)
                    {
                        processCullablesTG.AddTask(processCullablesDescriptor, [this, &viewPtr, &processCullablesTGEvent]()
                            {
                                AZ::TaskGraph subTaskGraph;
                                m_cullingScene->ProcessCullablesTG(*this, *viewPtr, subTaskGraph);
                                if (!subTaskGraph.IsEmpty())
                                {
                                    subTaskGraph.Detach();
                                    subTaskGraph.Submit(&processCullablesTGEvent);
                                }
                            });
                    }
                }
                else
                {
                    for (ViewPtr& viewPtr : m_renderPacket.m_views)
                    {
                        m_cullingScene->ProcessCullablesTG(*this, *viewPtr, processCullablesTG);
                    }
                }
                bool processCullablesHasWork = !processCullablesTG.IsEmpty();
                if (processCullablesHasWork)
                {
                    processCullablesTG.Submit(&processCullablesTGEvent);
                }

                collectDrawPacketsTGEvent.Wait();
                if (processCullablesHasWork) // skip the wait if there is no work to do
                {
                    processCullablesTGEvent.Wait();
                }
        }

        void Scene::CollectDrawPacketsJobs()
        {
            AZ_PROFILE_SCOPE(RPI, "CollectDrawPacketsJobs");
            AZ::JobCompletion* collectDrawPacketsCompletion = aznew AZ::JobCompletion();

            // Launch FeatureProcessor::Render() jobs
            for (auto& fp : m_featureProcessors)
            {
                const auto renderLambda = [this, &fp]()
                {
                    fp->Render(m_renderPacket);
                };

                AZ::Job* renderJob = AZ::CreateJobFunction(AZStd::move(renderLambda), true, nullptr);    //auto-deletes
                renderJob->SetDependent(collectDrawPacketsCompletion);
                renderJob->Start();
            }

            // Launch CullingSystem::ProcessCullables() jobs (will run concurrently with FeatureProcessor::Render() jobs)
            const bool parallelOctreeTraversal = m_cullingScene->GetDebugContext().m_parallelOctreeTraversal;
            m_cullingScene->BeginCulling(m_renderPacket.m_views);
            for (ViewPtr& viewPtr : m_renderPacket.m_views)
            {
                AZ::Job* processCullablesJob = AZ::CreateJobFunction([this, &viewPtr](AZ::Job& thisJob)
                    {
                        m_cullingScene->ProcessCullablesJobs(*this, *viewPtr, thisJob); // can't call directly because ProcessCullables needs a parent job
                    },
                    true, nullptr); //auto-deletes
                if (parallelOctreeTraversal)
                {
                    processCullablesJob->SetDependent(collectDrawPacketsCompletion);
                    processCullablesJob->Start();
                }
                else
                {
                    processCullablesJob->StartAndWaitForCompletion();
                }
            }

            WaitAndCleanCompletionJob(collectDrawPacketsCompletion);
        }

        void Scene::FinalizeDrawListsTaskGraph()
        {
            AZ::TaskGraphEvent finalizeDrawListsTGEvent;
            static const AZ::TaskDescriptor finalizeDrawListsTGDesc{"RPI_Scene_PrepareRender_FinalizeDrawLists", "Graphics"};

            AZ::TaskGraph finalizeDrawListsTG;
            for (auto& view : m_renderPacket.m_views)
            {
                finalizeDrawListsTG.AddTask(
                    finalizeDrawListsTGDesc,
                    [view]()
                    {
                        view->FinalizeDrawLists();
                    });
            }
            finalizeDrawListsTG.Submit(&finalizeDrawListsTGEvent);
            finalizeDrawListsTGEvent.Wait();
        }

        void Scene::FinalizeDrawListsJobs()
        {
            AZ::JobCompletion* finalizeDrawListsCompletion = aznew AZ::JobCompletion();
            for (auto& view : m_renderPacket.m_views)
            {
                const auto finalizeDrawListsLambda = [view]()
                {
                    view->FinalizeDrawLists();
                };

                AZ::Job* finalizeDrawListsJob = AZ::CreateJobFunction(AZStd::move(finalizeDrawListsLambda), true, nullptr);     //auto-deletes
                finalizeDrawListsJob->SetDependent(finalizeDrawListsCompletion);
                finalizeDrawListsJob->Start();
            }
            WaitAndCleanCompletionJob(finalizeDrawListsCompletion);
        }

        void Scene::PrepareRender(RHI::JobPolicy jobPolicy, float simulationTime)
        {
            AZ_PROFILE_SCOPE(RPI, "Scene: PrepareRender");

            if (m_taskGraphActive)
            {
                WaitAndCleanTGEvent(AZStd::move(m_simulationFinishedTGEvent));
            }
            else
            {
                WaitAndCleanCompletionJob(m_simulationCompletion);
            }

            SceneNotificationBus::Event(GetId(), &SceneNotification::OnBeginPrepareRender);

            // Get active pipelines which need to be rendered and notify them frame started
            AZStd::vector<RenderPipelinePtr> activePipelines;
            {
                AZ_PROFILE_SCOPE(RPI, "Scene: OnStartFrame");
                for (auto& pipeline : m_pipelines)
                {
                    if (pipeline->NeedsRender())
                    {
                        activePipelines.push_back(pipeline);
                        pipeline->OnStartFrame(simulationTime);
                    }
                }
            }

            // Return if there is no active render pipeline
            if (activePipelines.empty())
            {
                SceneNotificationBus::Event(GetId(), &SceneNotification::OnEndPrepareRender);
                return;
            }

            // Init render packet
            m_renderPacket.m_views.clear();
            AZ_Assert(m_cullingScene, "m_cullingScene is not initialized");
            m_renderPacket.m_cullingScene = m_cullingScene;
            m_renderPacket.m_jobPolicy = jobPolicy;
            

            {
                AZ_PROFILE_SCOPE(RPI, "Setup Views");

                // Collect persistent views from all pipelines to be rendered
                AZStd::map<ViewPtr, RHI::DrawListMask> persistentViews; 
                for (const auto& pipeline : activePipelines)
                {
                    pipeline->CollectPersistentViews(persistentViews);
                }

                // Set combined draw list mask for each persistent view and get a global draw list mask for late use
                for (auto& viewInfo : persistentViews)
                {
                    viewInfo.first->SetDrawListMask(viewInfo.second);
                    m_renderPacket.m_views.push_back(viewInfo.first);
                    m_renderPacket.m_drawListMask |= viewInfo.second;
                }
            
                // Collect transient views from each feature processor
                FeatureProcessor::PrepareViewsPacket prepareViewPacket;
                AZStd::vector<AZStd::pair<PipelineViewTag, ViewPtr>> transientViews;
                for (auto& fp : m_featureProcessors)
                {
                    fp->PrepareViews(prepareViewPacket, transientViews);
                }
            
                // Save transient views to RenderPacket and add them to each pipeline. 
                for (auto view : transientViews)
                {
                    m_renderPacket.m_views.push_back(view.second);
                    m_renderPacket.m_drawListMask |= view.second->GetDrawListMask();
                    for (auto& itr : activePipelines)
                    {
                        itr->AddTransientView(view.first, view.second);
                    }
                }
            }

            {

                if (m_taskGraphActive)
                {
                    CollectDrawPacketsTaskGraph();
                }
                else
                {
                    CollectDrawPacketsJobs();
                }

                m_cullingScene->EndCulling();

                // Add dynamic draw data for all the views
                if (m_dynamicDrawSystem)
                {
                    m_dynamicDrawSystem->SubmitDrawData(this, m_renderPacket.m_views);
                }
            }

            {
                AZ_PROFILE_SCOPE(RPI, "FinalizeDrawLists");
                if (jobPolicy == RHI::JobPolicy::Serial || 
                    m_renderPacket.m_views.size() <= 1) // FinalizeDrawListsX both immediately wait for the job to complete, skip job if only 1 job would be generated
                {
                    for (auto& view : m_renderPacket.m_views)
                    {
                        view->FinalizeDrawLists();
                    }
                }
                else
                {
                    if (m_taskGraphActive)
                    {
                        FinalizeDrawListsTaskGraph();
                    }
                    else
                    {
                        FinalizeDrawListsJobs();
                    }
                }
            }

            {
                AZ_PROFILE_SCOPE(RPI, "Scene OnEndPrepareRender");
                SceneNotificationBus::Event(GetId(), &SceneNotification::OnEndPrepareRender);
            }
        }

        void Scene::OnFrameEnd()
        {
            AZ_PROFILE_SCOPE(RPI, "Scene: OnFrameEnd");
            for (auto& pipeline : m_pipelines)
            {
                if (pipeline->NeedsRender())
                {
                    pipeline->OnFrameEnd();
                }
            }
            for (auto& fp : m_featureProcessors)
            {
                fp->OnRenderEnd();
            }
        }

        void Scene::UpdateSrgs()
        {
            PrepareSceneSrg();

            for (auto& view : m_renderPacket.m_views)
            {
                view->UpdateSrg();
            }
        }

        const RHI::ShaderResourceGroup* Scene::GetRHIShaderResourceGroup() const
        {
            if (m_srg.get())
            {
                return m_srg.get()->GetRHIShaderResourceGroup();
            }
            else
            {
                return nullptr;
            }
        }

        Data::Instance<ShaderResourceGroup> Scene::GetShaderResourceGroup() const
        {
            return m_srg;
        }

        const SceneId& Scene::GetId() const
        {
            return m_id;
        }

        AZ::Name Scene::GetName() const
        {
            return m_name;
        }
                
        bool Scene::SetDefaultRenderPipeline(const RenderPipelineId& pipelineId)
        {
            RenderPipelinePtr newPipeline = GetRenderPipeline(pipelineId);
            if (newPipeline)
            {
                m_defaultPipeline = newPipeline;
                return true;
            }
            return false;
        }

        RenderPipelinePtr Scene::GetDefaultRenderPipeline() const
        {
            return m_defaultPipeline;
        }

        const AZStd::vector<RenderPipelinePtr>& Scene::GetRenderPipelines() const
        {
            return m_pipelines;
        }
        
        void Scene::OnSceneNotifictaionHandlerConnected(SceneNotification* handler)
        {
            for (auto renderPipeline : m_pipelines)
            {
                handler->OnRenderPipelineAdded(renderPipeline);
                const RenderPipeline::PipelineViewMap& viewsInfo = renderPipeline->GetPipelineViews();
                for (RenderPipeline::PipelineViewMap::const_iterator itr = viewsInfo.begin(); itr != viewsInfo.end(); itr++)
                {
                    if (itr->second.m_type == PipelineViewType::Persistent && itr->second.m_views.size() == 1)
                    {
                        handler->OnRenderPipelinePersistentViewChanged(renderPipeline.get(), itr->first, itr->second.m_views[0], nullptr);
                    }
                }
            }
        }
        
        bool Scene::ConfigurePipelineState(RHI::DrawListTag drawListTag, RHI::PipelineStateDescriptorForDraw& outPipelineState) const
        {
            auto pipelineStatesItr = m_pipelineStatesLookup.find(drawListTag);

            if (pipelineStatesItr != m_pipelineStatesLookup.end())
            {
                const PipelineStateList& pipelineStateList = pipelineStatesItr->second;

                AZ_Error("RPI", pipelineStateList.size() == 1, "Scene::ConfigurePipelineState called for drawListTag [%s] which has [%zu] different pipeline states."
                    "Using first pipeline state by default.",
                    RHI::RHISystemInterface::Get()->GetDrawListTagRegistry()->GetName(drawListTag).GetCStr(),
                    pipelineStateList.size()
                );

                AZ_Assert(pipelineStateList.size() > 0, "Scene::ConfigurePipelineState called for drawListTag [%s] which has [%zu] pipeline states.",
                    RHI::RHISystemInterface::Get()->GetDrawListTagRegistry()->GetName(drawListTag).GetCStr(),
                    pipelineStateList.size()
                );

                outPipelineState.m_renderAttachmentConfiguration = pipelineStateList[0].m_renderAttachmentConfiguration;
                outPipelineState.m_renderStates.m_multisampleState = pipelineStateList[0].m_multisampleState;
                return true;
            }

            return false;
        }

        const Scene::PipelineStateList& Scene::GetPipelineStates(RHI::DrawListTag drawListTag) const
        {
            static const PipelineStateList emptyList;

            auto pipelineStatesItr = m_pipelineStatesLookup.find(drawListTag);

            if (pipelineStatesItr != m_pipelineStatesLookup.end())
            {
                return pipelineStatesItr->second;
            }

            return emptyList;
        }

        bool Scene::HasOutputForPipelineState(RHI::DrawListTag drawListTag) const
        {
            return m_pipelineStatesLookup.find(drawListTag) != m_pipelineStatesLookup.end();
        }

        void Scene::RebuildPipelineStatesLookup()
        {
            AZ_PROFILE_SCOPE(RPI, "Scene: RebuildPipelineStatesLookup");
            m_pipelineStatesLookup.clear();

            AZStd::queue<ParentPass*> parents;
            for (auto renderPipeline : m_pipelines)
            {
                parents.push(renderPipeline->GetRootPass().get());

                // Visit all the passes under this root pass
                while (!parents.empty())
                {
                    // Visit the first parent pass in the parents queue then remove it from the queue
                    ParentPass* parent = parents.front();
                    parents.pop();

                    for (const Ptr<Pass>& child : parent->GetChildren())
                    {
                        ParentPass* asParent = child->AsParent();
                        if (asParent)
                        {
                            // Add to parent queue for late visiting
                            parents.push(asParent);
                        }
                        else if (child->HasDrawListTag())
                        {
                            // only need to process RasterPass since it and its derived classes need to use draw list tag to 
                            // acquire OutputAttachmentLayout and MultisampleState
                            RasterPass* rasterPass = azrtti_cast<RasterPass*>(child.get());
                            if (rasterPass == nullptr)
                            {
                                continue;
                            }

                            RHI::DrawListTag drawListTag = child->GetDrawListTag();
                            auto pipelineStatesItr = m_pipelineStatesLookup.find(drawListTag);

                            if (pipelineStatesItr == m_pipelineStatesLookup.end())
                            {
                                m_pipelineStatesLookup[drawListTag].push_back();
                                m_pipelineStatesLookup[drawListTag][0].m_multisampleState = rasterPass->GetMultisampleState();
                                m_pipelineStatesLookup[drawListTag][0].m_renderAttachmentConfiguration = rasterPass->GetRenderAttachmentConfiguration();
                                rasterPass->SetPipelineStateDataIndex(0);
                            }
                            else
                            {
                                PipelineStateList& pipelineStateList = pipelineStatesItr->second;
                                size_t size = pipelineStateList.size();

                                u32 index = 0;

                                auto multisampleState = rasterPass->GetMultisampleState();
                                auto renderAttachmentConfg = rasterPass->GetRenderAttachmentConfiguration();

                                for (; index < size; ++index)
                                {
                                    PipelineStateData stateData = pipelineStateList[index];
                                    if (stateData.m_multisampleState == multisampleState
                                        && stateData.m_renderAttachmentConfiguration == renderAttachmentConfg)
                                    {
                                        break;
                                    }
                                }

                                if (index < size)
                                {
                                    // Found matching pipeline state data, set index
                                    rasterPass->SetPipelineStateDataIndex(index);
                                }
                                else
                                {
                                    // No match found, add new pipeline state data
                                    pipelineStateList.push_back();
                                    pipelineStateList[size].m_multisampleState = rasterPass->GetMultisampleState();
                                    pipelineStateList[size].m_renderAttachmentConfiguration = rasterPass->GetRenderAttachmentConfiguration();
                                    rasterPass->SetPipelineStateDataIndex(static_cast<AZ::u32>(size));
                                }
                            }
                        }
                    }
                }
            }
        }

        RenderPipelinePtr Scene::FindRenderPipelineForWindow(AzFramework::NativeWindowHandle windowHandle)
        {
            for (auto renderPipeline : m_pipelines)
            {
                if (renderPipeline->GetWindowHandle() == windowHandle)
                {
                    return renderPipeline;
                }
            }
            return nullptr;
        }
    }
}
