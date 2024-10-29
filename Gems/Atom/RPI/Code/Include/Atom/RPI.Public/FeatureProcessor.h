/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/DrawListContext.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/SceneBus.h>

#include <Atom/RPI.Reflect/FeatureProcessorDescriptor.h>

#include <Atom/RHI.Reflect/FrameSchedulerEnums.h>

#include <AtomCore/Instance/InstanceData.h>

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Component/EntityId.h>

namespace AZ
{
    // forward declares
    class Job;

    namespace RPI
    {
        class RenderPipeline;

        //! @class FeatureProcessor
        //! @brief Interface that FeatureProcessors should derive from
        //! @detail FeatureProcceors will record simulation state from the simulation job graph into a buffer that is isolated from the asynchronous rendering graph.
        //!         Simulate() is called from the simulation graph preparing and publishing data for use by the asynchronous rendering execution graph.
        //!         Render() is called from the render graph, converting state data to GPU/rendering state and submitting to the pipeline coordinator.
        //!         Feature processors will contain or derive from listeners with a data buffers as needed to minimize contention.  FeatureProcesors will collate the
        //!         data from the listeners into a data packet/feature for submission to the render pipeline coordinator.
        //! 
        //!         It is recommended that each feature processor maintain a data buffer that is buffered N times for the data that is
        //!         expected to be delivered via an Ebus.
        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_PUBLIC_API FeatureProcessor
            : public SceneNotificationBus::Handler
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            friend class Scene;

        public:
            struct PrepareViewsPacket
            {
                AZStd::map<ViewPtr, RHI::DrawListMask> m_persistentViews;
            };

            struct SimulatePacket
            {
                AZ::Job* m_parentJob = nullptr;
            };
            
            struct RenderPacket
            {
                //! The views that are relevant for rendering this frame.
                AZStd::vector<ViewPtr> m_views;

                //! A global draw list mask which is a combined mask for all the views.
                //! Feature processors can utilize this mask to figure out if it may need to generate the DrawPackets upfront.
                //! For example, UI feature processor can skip further processing if it finds out there is no view has UI DrawListTag
                RHI::DrawListMask m_drawListMask;

                //! Whether to run jobs in parallel or not (for debugging)
                RHI::JobPolicy m_jobPolicy;

                class CullingScene* m_cullingScene;
            };            

            AZ_RTTI(FeatureProcessor, "{B8027170-C65C-4237-964D-B557FC9D7575}");
            AZ_CLASS_ALLOCATOR(FeatureProcessor, AZ::SystemAllocator);

            FeatureProcessor() = default;
            virtual ~FeatureProcessor() = default;

            Scene* GetParentScene() const { return m_parentScene; }

            // FeatureProcessor Interface
            //! Perform any necessary activation and gives access to owning Scene.
            virtual void Activate() {}

            //! Perform any necessary deactivation.
            virtual void Deactivate() {}
            
            //! O3DE_DEPRECATION_NOTICE(GHI-12687)
            //! @deprecated use AddRenderPasses(RenderPipeline*)
            //! Apply changes and add additional render passes to the render pipeline from the feature processors
            virtual void ApplyRenderPipelineChange([[maybe_unused]] RenderPipeline* pipeline) {}

            //! Add additional render passes to the render pipeline before it's finalized
            //! The render pipeline must have m_allowModification set to true (see Scene::TryApplyRenderPipelineChanges() function)
            //! This function is called when the render pipeline is added or rebuilt
            virtual void AddRenderPasses([[maybe_unused]] RenderPipeline* pipeline) {}

            //! Allows the feature processor to expose supporting views based on
            //! the main views passed in. Main views (persistent views) are views that must be 
            //! rendered and impacts the presentation of the application. Support
            //! views (transient views) are views that must be rendered only to correctly render 
            //! the main views. This function is called per frame and it happens on main thread.
            //! Support views should be added to outViews with their associated pipeline view tags.
            virtual void PrepareViews(
                const PrepareViewsPacket& /*prepareViewPacket*/, AZStd::vector<AZStd::pair<PipelineViewTag, ViewPtr>>& /*outViews*/) {}

            //! The feature processor should perform any internal simulation at this point - For 
            //! instance, updating a particle system or animation. Not every feature processor
            //! will need to implement this.
            //! 
            //!  - This may not be called every frame.
            //!  - This may be called in parallel with other feature processors.
            virtual void Simulate(const SimulatePacket&) {}

            //! The feature processor should enqueue draw packets to relevant draw lists.
            //! 
            //!  - This is called every frame.
            //!  - This may be called in parallel with other feature processors.
            //!  - This may be called in parallel with culling
            virtual void Render(const RenderPacket&) {}
            
            //! Notifies when culling is finished, but draw lists have not been finalized or sorted
            //! If a feature processor uses visibility lists instead of letting the culling system submit draw items
            //! it should access the visibility lists here
            virtual void OnEndCulling(const RenderPacket&) {}

            //! The feature processor may do clean up when the current render frame is finished
            //!  - This is called every RPI::RenderTick.
            virtual void OnRenderEnd() {}

        protected:
            // Functions to enable or disable SceneNotificationBus. Feature processors which need to handle these notifications may 
            // call EnableSceneNotification in its Activate function. 
            void EnableSceneNotification();
            void DisableSceneNotification();

        private:

            Scene* m_parentScene = nullptr;
        };

    } // namespace RPI
} // namespace AZ

#define AZ_FEATURE_PROCESSOR(TypeName)

