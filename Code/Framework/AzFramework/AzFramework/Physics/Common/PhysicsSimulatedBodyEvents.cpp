/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Common/PhysicsSimulatedBodyEvents.h>

#include <AzCore/Interface/Interface.h>
#include <AzCore/std/functional.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/Collision/CollisionEvents.h>

namespace AzPhysics
{
    namespace SimulatedBodyEvents
    {
        namespace Internal
        {
            //helper to register a handler
            template<typename Handler, class Function>
            void RegisterHandler(AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle bodyHandle,
                Handler& handler, Function registerFunc)
            {
                if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
                {
                    if (AzPhysics::SimulatedBody* body = sceneInterface->GetSimulatedBodyFromHandle(sceneHandle, bodyHandle))
                    {
                        auto func = AZStd::bind(registerFunc, body, AZStd::placeholders::_1);
                        func(handler);
                    }
                }
            }
        }

        void RegisterOnCollisionBeginHandler(AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle bodyHandle,
            OnCollisionBegin::Handler& handler)
        {
            Internal::RegisterHandler(sceneHandle, bodyHandle, handler, &SimulatedBody::RegisterOnCollisionBeginHandler);
        }

        void RegisterOnCollisionPersistHandler(AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle bodyHandle,
            OnCollisionPersist::Handler& handler)
        {
            Internal::RegisterHandler(sceneHandle, bodyHandle, handler, &SimulatedBody::RegisterOnCollisionPersistHandler);
        }

        void RegisterOnCollisionEndHandler(AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle bodyHandle,
            OnCollisionEnd::Handler& handler)
        {
            Internal::RegisterHandler(sceneHandle, bodyHandle, handler, &SimulatedBody::RegisterOnCollisionEndHandler);
        }

        void RegisterOnTriggerEnterHandler(AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle bodyHandle,
            OnTriggerEnter::Handler& handler)
        {
            Internal::RegisterHandler(sceneHandle, bodyHandle, handler, &SimulatedBody::RegisterOnTriggerEnterHandler);
        }

        void RegisterOnTriggerExitHandler(AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle bodyHandle,
            OnTriggerExit::Handler& handler)
        {
            Internal::RegisterHandler(sceneHandle, bodyHandle, handler, &SimulatedBody::RegisterOnTriggerExitHandler);
        }

        void RegisterOnSyncTransformHandler(
            AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle bodyHandle, OnSyncTransform::Handler& handler)
        {
            Internal::RegisterHandler(sceneHandle, bodyHandle, handler, &SimulatedBody::RegisterOnSyncTransformHandler);
        }

    } // namespace SimulatedBodyEvents
}
