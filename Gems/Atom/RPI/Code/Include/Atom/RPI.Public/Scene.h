/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/DrawList.h>
#include <Atom/RHI/PipelineStateDescriptor.h>
#include <Atom/RHI/DrawFilterTagRegistry.h>
#include <Atom/RHI.Reflect/FrameSchedulerEnums.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>
#include <Atom/RPI.Reflect/System/SceneDescriptor.h>
#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Public/FeatureProcessorFactory.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/SceneBus.h>
#include <Atom/RPI.Public/ViewProviderBus.h>

#include <AtomCore/Instance/Instance.h>

#include <AzCore/Component/TickBus.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Script/ScriptTimePoint.h>
#include <AzCore/Task/TaskGraph.h>

#include <AzFramework/Scene/Scene.h>
#include <AzFramework/Scene/SceneSystemInterface.h>

namespace AzFramework
{
    class IVisibilityScene;
}

namespace AZ
{
    namespace RPI
    {
        class View;
        class FeatureProcessor;
        class ShaderResourceGroup;
        class ShaderResourceGroupAsset;
        class CullingScene;
        class DynamicDrawSystem;

        // Callback function to modify values of a ShaderResourceGroup
        using ShaderResourceGroupCallback = AZStd::function<void(ShaderResourceGroup*)>;

        class Scene final
            : public SceneRequestBus::Handler
        {
            friend class FeatureProcessorFactory;
            friend class RPISystem;
        public:
            AZ_CLASS_ALLOCATOR(Scene, AZ::SystemAllocator, 0);
            AZ_RTTI(Scene, "{29860D3E-D57E-41D9-8624-C39604EF2973}");

            // Pipeline states info built from scene's render pipeline passes
            struct PipelineStateData
            {
                RHI::RenderAttachmentConfiguration m_renderAttachmentConfiguration;
                RHI::MultisampleState m_multisampleState;
            };
            typedef AZStd::vector<PipelineStateData> PipelineStateList;

            static ScenePtr CreateScene(const SceneDescriptor& sceneDescriptor);

            static ScenePtr CreateSceneFromAsset(Data::Asset<AnyAsset> sceneAsset);

            //! Gets the RPI::Scene for a given entityContextId.
            //! May return nullptr if there is no RPI::Scene created for that entityContext.
            static Scene* GetSceneForEntityContextId(AzFramework::EntityContextId entityContextId);
            
            //! Gets the RPI::Scene for a given entityId.
            static Scene* GetSceneForEntityId(AZ::EntityId entityId);

            ~Scene();

            void Activate();
            void Deactivate();

            //! Enables a feature processor type for this scene.
            //! Only a single instance of feature processor type is allowed to be active per scene.
            //! The order in which feature processors are enabled is the order in which
            //! feature processors will be updated when updating is single threaded.
            template<typename FeatureProcessorType>
            FeatureProcessorType* EnableFeatureProcessor();

            FeatureProcessor* EnableFeatureProcessor(const FeatureProcessorId& featureProcessorId);

            //! Enable all feature processors which were available (registered in feature processor factory) for this scene.
            void EnableAllFeatureProcessors();

            //! Disables a feature processor type from the scene, only if was previously
            //! enabled.
            template<typename FeatureProcessorType>
            void DisableFeatureProcessor();

            void DisableFeatureProcessor(const FeatureProcessorId& featureProcessorId);

            void DisableAllFeatureProcessors();

            //! Callback function that will be invoked with each non-pointer FeatureProcessor
            //! return true to continue visiting or false to halt
            using FeatureProcessorVisitCallback = AZStd::function<bool(FeatureProcessor&)>;
            void VisitFeatureProcessor(FeatureProcessorVisitCallback callback) const;

            //! Linear search to retrieve specific class of a feature processor.
            //! Returns nullptr if a feature processor with the specified id is
            //! not found.
            template<typename FeatureProcessorType>
            FeatureProcessorType* GetFeatureProcessor() const;

            FeatureProcessor* GetFeatureProcessor(const FeatureProcessorId& featureProcessorId) const;

            FeatureProcessor* GetFeatureProcessor(const TypeId& featureProcessorTypeId) const;

            template<typename FeatureProcessorType>
            static FeatureProcessorType* GetFeatureProcessorForEntity(AZ::EntityId entityId);

            template<typename FeatureProcessorType>
            static FeatureProcessorType* GetFeatureProcessorForEntityContextId(AzFramework::EntityContextId entityContextId);

            //! Get pipeline by name id
            RenderPipelinePtr GetRenderPipeline(const RenderPipelineId& pipelineId) const;

            void AddRenderPipeline(RenderPipelinePtr pipeline);

            void RemoveRenderPipeline(const RenderPipelineId& pipelineId);

            const RHI::ShaderResourceGroup* GetRHIShaderResourceGroup() const;
            Data::Instance<ShaderResourceGroup> GetShaderResourceGroup() const;

            const SceneId& GetId() const;

            AZ::Name GetName() const;

            //! Set default pipeline by render pipeline ID.
            //! It returns true if the default render pipeline was set from the input ID.
            //! If the specified render pipeline doesn't exist in this scene then it won't do anything and returns false.
            bool SetDefaultRenderPipeline(const RenderPipelineId& pipelineId);

            //! Return default pipeline. If the default pipeline wasn't set, then it would return nullptr.
            RenderPipelinePtr GetDefaultRenderPipeline() const;

            //! Return all added render pipelines in this scene
            const AZStd::vector<RenderPipelinePtr>& GetRenderPipelines() const;

            //! Configure some pipeline state data from scene's passes associated with specified DrawListTag.
            //! The pipeline states which will be set may include: OutputAttachmentLayout; MultisampleState.
            //! If the current scene's render pipeline doesn't contain the DrawListTag, it returns false and failed to configure the pipeline state
            //! And the caller shouldn't need to continue creating draw data with this pipeline state.
            bool ConfigurePipelineState(RHI::DrawListTag drawListTag, RHI::PipelineStateDescriptorForDraw& outPipelineState) const;

            const PipelineStateList& GetPipelineStates(RHI::DrawListTag drawListTag) const;

            bool HasOutputForPipelineState(RHI::DrawListTag drawListTag) const;

            AzFramework::IVisibilityScene* GetVisibilityScene() const { return m_visibilityScene; }

            AZ::RPI::CullingScene* GetCullingScene() const { return m_cullingScene; }

            RenderPipelinePtr FindRenderPipelineForWindow(AzFramework::NativeWindowHandle windowHandle, ViewType viewType = ViewType::Default);

            using PrepareSceneSrgEvent = AZ::Event<RPI::ShaderResourceGroup*>;
            //! Connect a handler to listen to the event that the Scene is ready to update and compile its scene srg
            //! User should use this event to update the part scene srg they know of
            void ConnectEvent(PrepareSceneSrgEvent::Handler& handler);

            //! Rebuild pipeline states lookup table.
            //! This function is called every time scene's render pipelines change.
            //! User may call this function explicitly if render pipelines were changed
            void RebuildPipelineStatesLookup();
                        
            //! Try apply render pipeline changes from each feature processors if the pipeline allows modification and wasn't modified.
            void TryApplyRenderPipelineChanges(RenderPipeline* pipeline);

        protected:
            // SceneFinder overrides...
            void OnSceneNotifictaionHandlerConnected(SceneNotification* handler);
            void PipelineStateLookupNeedsRebuild() override;

            // Cpu simulation which runs all active FeatureProcessor Simulate() functions.
            // @param jobPolicy if it's JobPolicy::Parallel, the function will spawn a job thread for each FeatureProcessor's simulation.
            // @param simulationTime the number of seconds since the application started
            void Simulate(RHI::JobPolicy jobPolicy, float simulationTime);

            // Collect DrawPackets from FeatureProcessors
            // @param jobPolicy if it's JobPolicy::Parallel, the function will spawn a job thread for each FeatureProcessor's
            // PrepareRender.
            // @param simulationTime the number of seconds since the application started; this is the same time value that was passed to Simulate()
            void PrepareRender(RHI::JobPolicy jobPolicy, float simulationTime);

            // Function called when the current frame is finished rendering.
            void OnFrameEnd();

            // Update and compile scene and view srgs
            // This is called after PassSystem's FramePrepare so passes can still modify view srgs in its FramePrepareIntenal function before they are submitted to command list
            void UpdateSrgs();


        private:
            Scene();


            // Helper function to wait for end of TaskGraph and then delete the TaskGraphEvent
            void WaitAndCleanTGEvent();

            // Helper function for wait and clean up a completion job
            void WaitAndCleanCompletionJob(AZ::JobCompletion*& completionJob);

            // Add a created feature processor to this scene
            void AddFeatureProcessor(FeatureProcessorPtr fp);

            // Check each of the added render pipelines and set its recreate flag if it's allowed to be modified by any feature processors
            // This is usually called when a feature processor was added and removed after scene was activated
            void CheckRecreateRenderPipeline();

            // Send out event to PrepareSceneSrgEvent::Handlers so they can update scene srg as needed
            // This happens in UpdateSrgs()
            void PrepareSceneSrg();

            // Implementation functions that allow scene to switch between using Jobs or TaskGraphs
            void SimulateTaskGraph();
            void SimulateJobs();

            void CollectDrawPacketsTaskGraph();
            void CollectDrawPacketsJobs();

            void FinalizeDrawListsTaskGraph();
            void FinalizeDrawListsJobs();

            // List of feature processors that are active for this scene
            AZStd::vector<FeatureProcessorPtr> m_featureProcessors;

            // List of pipelines of this scene. Each pipeline has an unique pipeline Id.
            AZStd::vector<RenderPipelinePtr> m_pipelines;

            // CPU simulation TaskGraphEvent to wait for completion of all the simulation tasks
            AZStd::unique_ptr<AZ::TaskGraphEvent> m_simulationFinishedTGEvent;

            // CPU simulation job completion for track all feature processors' simulation jobs
            AZ::JobCompletion* m_simulationCompletion = nullptr;

            AzFramework::IVisibilityScene* m_visibilityScene;
            AZ::RPI::CullingScene* m_cullingScene;

            // Cached views for current rendering frame. It gets re-built every frame.
            AZ::RPI::FeatureProcessor::SimulatePacket m_simulatePacket;
            AZ::RPI::FeatureProcessor::RenderPacket m_renderPacket;

            // Scene's srg
            Data::Instance<ShaderResourceGroup> m_srg;
            // Event to for prepare scene srg
            PrepareSceneSrgEvent m_prepareSrgEvent;

            // The uuid to identify this scene.
            SceneId m_id;

            // Scene's name which is set at initialization. Can be empty
            AZ::Name m_name;

            bool m_activated = false;
            bool m_taskGraphActive = false; // update during tick, to ensure it only changes on frame boundaries

            RenderPipelinePtr m_defaultPipeline;

            // Rebuild the m_pipelineStatesLookup after queued Pipeline changes have been applied.
            bool m_pipelineStatesLookupNeedsRebuild = false;

            // Mapping of draw list tag and a group of pipeline states info built from scene's render pipeline passes
            AZStd::map<RHI::DrawListTag, PipelineStateList> m_pipelineStatesLookup;

            // reference of dynamic draw system (from RPISystem)
            DynamicDrawSystem* m_dynamicDrawSystem = nullptr;

            // Registry which allocates draw filter tag for RenderPipeline
            RHI::Ptr<RHI::DrawFilterTagRegistry> m_drawFilterTagRegistry;

            RHI::ShaderInputNameIndex m_timeInputIndex = "m_time";
            float m_simulationTime = 0.0;
            RHI::ShaderInputNameIndex m_prevTimeInputIndex = "m_prevTime";
            float m_prevSimulationTime = 0.0;
        };

        // --- Template functions ---

        template<typename FeatureProcessorType>
        FeatureProcessorType* Scene::EnableFeatureProcessor()
        {
            static_assert(!AZStd::is_abstract<FeatureProcessorType>::value, "Cannot enable an abstract feature processor. Enable it via a specific implementation.");
            return static_cast<FeatureProcessorType*> (EnableFeatureProcessor(FeatureProcessorId{ FeatureProcessorType::RTTI_TypeName() }));
        }

        template<typename FeatureProcessorType>
        void Scene::DisableFeatureProcessor()
        {
            static_assert(!AZStd::is_abstract<FeatureProcessorType>::value, "Cannot disable an abstract feature processor. Disable it via a specific implementation.");
            DisableFeatureProcessor(FeatureProcessorId{ FeatureProcessorType::RTTI_TypeName() });
        }

        template<typename FeatureProcessorType>
        FeatureProcessorType* Scene::GetFeatureProcessor() const
        {
            return static_cast<FeatureProcessorType*> (GetFeatureProcessor(FeatureProcessorType::RTTI_Type()));
        }

        template<typename FeatureProcessorType>
        FeatureProcessorType* Scene::GetFeatureProcessorForEntity(AZ::EntityId entityId)
        {
            RPI::Scene* renderScene = GetSceneForEntityId(entityId);            
            if (renderScene)
            {
                return renderScene->GetFeatureProcessor<FeatureProcessorType>();
            }
            return nullptr;
        };

        template<typename FeatureProcessorType>
        FeatureProcessorType* Scene::GetFeatureProcessorForEntityContextId(AzFramework::EntityContextId entityContextId)
        {
            RPI::Scene* renderScene = GetSceneForEntityContextId(entityContextId);
            if (renderScene)
            {
                // Return the requested feature processor from the RPI::Scene.
                return renderScene->GetFeatureProcessor<FeatureProcessorType>();
            }
            return nullptr;
        };
    }
}
