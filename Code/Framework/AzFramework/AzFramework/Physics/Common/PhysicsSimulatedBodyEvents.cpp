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

#include <AzFramework/Physics/Common/PhysicsSimulatedBodyEvents.h>

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
    } // namespace SimulatedBodyEvents
}
