/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Configuration.h>

namespace AZ
{
    namespace RPI
    {
        //! Ebus to receive scene's notifications
        class ATOM_RPI_PUBLIC_API SceneNotification
            : public AZ::EBusTraits
        {
        public:
            // EBus Configuration
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            using BusIdType = SceneId;

            //! Custom connection policy to make sure events are fully in sync
            template <class Bus>
            struct SceneConnectionPolicy
                : public EBusConnectionPolicy<Bus>
            {
                static void Connect(typename Bus::BusPtr& busPtr, typename Bus::Context& context, 
                    typename Bus::HandlerNode& handler, typename Bus::Context::ConnectLockGuard& connectLock,
                    const typename Bus::BusIdType& id = 0);
            };
            template<typename Bus>
            using ConnectionPolicy = SceneConnectionPolicy<Bus>;
            //////////////////////////////////////////////////////////////////////////

            enum class RenderPipelineChangeType
            {
                Added,              //The render pipeline was added to this scene
                PassChanged,      //Any passes of this render pipeline are modified before a render tick
                Removed             //The render pipeline was removed from this scene
            };
                        
            //! O3DE_DEPRECATION_NOTICE(GHI-12687)
            //! @deprecated use OnRenderPipelineChanged(RenderPipeline*, RenderPipelineChangeType::Added)
            //! Notifies when a render pipeline is added to this scene. 
            //! @param pipeline The render pipeline which was added
            virtual void OnRenderPipelineAdded(RenderPipelinePtr pipeline) {};
                        
            //! O3DE_DEPRECATION_NOTICE(GHI-12687)
            //! @deprecated use OnRenderPipelineChanged(RenderPipeline*, RenderPipelineChangeType::PassChanged)
            //! Notifies when any passes of this render pipeline are modified before a render tick
            //! This includes adding a pass, removing a pass, or if pass data changed (such as attachments, draw list tags, etc.)
            //! Feature processors may need to use it to update their cached pipeline states
            //! @param pipeline The render pipeline which was modified
            virtual void OnRenderPipelinePassesChanged([[maybe_unused]] RenderPipeline* renderPipeline) {}

            //! O3DE_DEPRECATION_NOTICE(GHI-12687)
            //! @deprecated use OnRenderPipelineChanged(RenderPipeline*, RenderPipelineChangeType::Removed)
            //! Notifies when a render pipeline is removed from this scene.
            //! @param pipeline The render pipeline which was removed
            virtual void OnRenderPipelineRemoved([[maybe_unused]] RenderPipeline* pipeline) {};

            //! Notifies when a render pipeline was added, removed or changed
            //! @param pipeline The render pipeline which was added
            virtual void OnRenderPipelineChanged(RenderPipeline* , RenderPipelineChangeType) {};
            
            //! Notifies when a persistent view is set/changed (for a particular RenderPipeline + ViewTag)
            //! @param pipeline The render pipeline which was modified
            //! @param viewTag The viewTag in this render pipeline which the new view was set to
            //! @param newView The view which was set to the render pipeline's view tag
            //! @param previousView The previous view associates to render pipeline's view tag before the new view was set
            virtual void OnRenderPipelinePersistentViewChanged([[maybe_unused]] RenderPipeline* renderPipeline, PipelineViewTag viewTag, ViewPtr newView, ViewPtr previousView) {}

            //! Notifies that the pipeline state lookup table has been rebuilt, so the pipeline state data (multisample state,
            //! render attachment configuration, etc) for a DrawListTag may have changed. 
            virtual void OnPipelineStateLookupRebuilt() {};

            //! Notifies when the PrepareRender phase is beginning
            //! This phase is when data is read from the FeatureProcessors and written to the draw lists.
            virtual void OnBeginPrepareRender() {}

            //! Notifies when the PrepareRender phase is ending
            virtual void OnEndPrepareRender() {}
        };

        using SceneNotificationBus = AZ::EBus<SceneNotification>;
        
        //! Ebus to handle requests sent to scene
        class ATOM_RPI_PUBLIC_API SceneRequest
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            using BusIdType = SceneId;

            virtual void OnSceneNotificationHandlerConnected(SceneNotification* handler) = 0;

            //! Causes an update of the PipelineStateLookup during the next render tick,
            //! after queued Pipeline changes are executed.
            virtual void PipelineStateLookupNeedsRebuild() = 0;
        };

        using SceneRequestBus = AZ::EBus<SceneRequest>;

        template <class Bus>
        void SceneNotification::SceneConnectionPolicy<Bus>::Connect(typename Bus::BusPtr& busPtr, typename Bus::Context& context, 
            typename Bus::HandlerNode& handler, typename Bus::Context::ConnectLockGuard& connectLock, const typename Bus::BusIdType& id)
        {
            EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, connectLock, id);
            SceneRequestBus::Event(id, &SceneRequestBus::Events::OnSceneNotificationHandlerConnected, handler);
        }
    } // namespace RPI
} // namespace AZ

DECLARE_EBUS_EXTERN_DLL_MULTI_ADDRESS(RPI::SceneNotification);
