/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

#include <AzFramework/Scene/SceneSystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
namespace AZ
{
    namespace Render
    {
        namespace Bootstrap
        {
            class Notification
                : public AZ::EBusTraits
            {
            public:
                //////////////////////////////////////////////////////////////////////////
                // EBusTraits overrides
                static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
                static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

                /**
                 * Custom connection policy to make sure all we are fully in sync
                 */
                template <class Bus>
                struct NotificationConnectionPolicy
                    : public EBusConnectionPolicy<Bus>
                {
                    static void Connect(typename Bus::BusPtr& busPtr, typename Bus::Context& context, typename Bus::HandlerNode& handler, typename Bus::Context::ConnectLockGuard& connectLock, const typename Bus::BusIdType& id = 0)
                    {
                        // connect
                        EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, connectLock, id);

                        // Check if bootstrap scene already exists and fire notifications if it does
                        auto sceneSystem = AzFramework::SceneSystemInterface::Get();
                        AZ_Assert(sceneSystem, "Notification bus called before the scene system has been initialized.");
                        AZStd::shared_ptr<AzFramework::Scene> mainScene = sceneSystem->GetScene(AzFramework::Scene::MainSceneName);
                        AZ_Assert(mainScene, "AzFramework didn't set up any scenes.");

                        AZ::RPI::ScenePtr* defaultScene = mainScene->FindSubsystem<AZ::RPI::ScenePtr>();
                        if (defaultScene && *defaultScene && (*defaultScene)->GetDefaultRenderPipeline())
                        {
                            handler->OnBootstrapSceneReady(defaultScene->get());
                        }
                    }
                };
                template<typename Bus>
                using ConnectionPolicy = NotificationConnectionPolicy<Bus>;

                //////////////////////////////////////////////////////////////////////////

                virtual void OnBootstrapSceneReady(AZ::RPI::Scene* bootstrapScene) = 0;
            };
            using NotificationBus = AZ::EBus<Notification>;
        } // namespace Bootstrap
    } // namespace Render
} // namespace AZ
