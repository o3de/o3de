/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Scene/PhysXSceneInterface.h>

#include <AzFramework/Physics/Common/PhysicsJoint.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/Configuration/SceneConfiguration.h>
#include <System/PhysXSystem.h>

namespace PhysX
{
    namespace Internal
    {
        template<class Handler, class Function>
        void EventRegisterHelper(PhysXSystem* physxSystem, AzPhysics::SceneHandle sceneHandle, Handler& handler, Function registerFunc)
        {
            if (AzPhysics::Scene* scene = physxSystem->GetScene(sceneHandle))
            {
                auto func = AZStd::bind(registerFunc, scene, AZStd::placeholders::_1);
                func(handler);
            }
        }
    }

    PhysXSceneInterface::PhysXSceneInterface(PhysXSystem* physxSystem)
        : m_physxSystem(physxSystem)
    {

    }

    // AzPhysics::SceneInterface ...
    AzPhysics::SceneHandle PhysXSceneInterface::GetSceneHandle(const AZStd::string& sceneName)
    {
        return m_physxSystem->GetSceneHandle(sceneName);
    }

    AzPhysics::Scene* PhysXSceneInterface::GetScene(AzPhysics::SceneHandle handle)
    {
        return m_physxSystem->GetScene(handle);
    }

    void PhysXSceneInterface::StartSimulation(AzPhysics::SceneHandle sceneHandle, float deltatime)
    {
        if (AzPhysics::Scene* scene = m_physxSystem->GetScene(sceneHandle))
        {
            scene->StartSimulation(deltatime);
        }
    }

    void PhysXSceneInterface::FinishSimulation(AzPhysics::SceneHandle sceneHandle)
    {
        if (AzPhysics::Scene* scene = m_physxSystem->GetScene(sceneHandle))
        {
            scene->FinishSimulation();
        }
    }

    void PhysXSceneInterface::SetEnabled(AzPhysics::SceneHandle sceneHandle, bool enable)
    {
        if (AzPhysics::Scene* scene = m_physxSystem->GetScene(sceneHandle))
        {
            scene->SetEnabled(enable);
        }
    }

    bool PhysXSceneInterface::IsEnabled(AzPhysics::SceneHandle sceneHandle) const
    {
        if (AzPhysics::Scene* scene = m_physxSystem->GetScene(sceneHandle))
        {
            return scene->IsEnabled();
        }
        return false;
    }

    AzPhysics::SimulatedBodyHandle PhysXSceneInterface::AddSimulatedBody(AzPhysics::SceneHandle sceneHandle, const AzPhysics::SimulatedBodyConfiguration* simulatedBodyConfig)
    {
        if (AzPhysics::Scene* scene = m_physxSystem->GetScene(sceneHandle))
        {
            return scene->AddSimulatedBody(simulatedBodyConfig);
        }
        return AzPhysics::InvalidSimulatedBodyHandle;
    }

    AzPhysics::SimulatedBodyHandleList PhysXSceneInterface::AddSimulatedBodies(AzPhysics::SceneHandle sceneHandle, const AzPhysics::SimulatedBodyConfigurationList& simulatedBodyConfigs)
    {
        if (AzPhysics::Scene* scene = m_physxSystem->GetScene(sceneHandle))
        {
            return scene->AddSimulatedBodies(simulatedBodyConfigs);
        }

        return {}; //return an empty list
    }

    AzPhysics::SimulatedBody* PhysXSceneInterface::GetSimulatedBodyFromHandle(AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle bodyHandle)
    {
        if (AzPhysics::Scene* scene = m_physxSystem->GetScene(sceneHandle))
        {
            return scene->GetSimulatedBodyFromHandle(bodyHandle);
        }
        return nullptr;
    }

    AzPhysics::SimulatedBodyList PhysXSceneInterface::GetSimulatedBodiesFromHandle(AzPhysics::SceneHandle sceneHandle, const AzPhysics::SimulatedBodyHandleList& bodyHandles)
    {
        if (AzPhysics::Scene* scene = m_physxSystem->GetScene(sceneHandle))
        {
            return scene->GetSimulatedBodiesFromHandle(bodyHandles);
        }
        return {}; //return an empty list
    }

    void PhysXSceneInterface::RemoveSimulatedBody(AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle& bodyHandle)
    {
        if (AzPhysics::Scene* scene = m_physxSystem->GetScene(sceneHandle))
        {
            scene->RemoveSimulatedBody(bodyHandle);
        }
    }

    void PhysXSceneInterface::RemoveSimulatedBodies(AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandleList& bodyHandles)
    {
        if (AzPhysics::Scene* scene = m_physxSystem->GetScene(sceneHandle))
        {
            scene->RemoveSimulatedBodies(bodyHandles);
        }
    }

    void PhysXSceneInterface::EnableSimulationOfBody(AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle bodyHandle)
    {
        if (AzPhysics::Scene* scene = m_physxSystem->GetScene(sceneHandle))
        {
            scene->EnableSimulationOfBody(bodyHandle);
        }
    }

    void PhysXSceneInterface::DisableSimulationOfBody(AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle bodyHandle)
    {
        if (AzPhysics::Scene* scene = m_physxSystem->GetScene(sceneHandle))
        {
            scene->DisableSimulationOfBody(bodyHandle);
        }
    }

    AzPhysics::JointHandle PhysXSceneInterface::AddJoint(
        AzPhysics::SceneHandle sceneHandle, const AzPhysics::JointConfiguration* jointConfig, 
        AzPhysics::SimulatedBodyHandle parentBody, AzPhysics::SimulatedBodyHandle childBody) 
    {
        if (AzPhysics::Scene* scene = m_physxSystem->GetScene(sceneHandle))
        {
            return scene->AddJoint(jointConfig, parentBody, childBody);
        }

        return AzPhysics::InvalidJointHandle;
    }

    AzPhysics::Joint* PhysXSceneInterface::GetJointFromHandle(AzPhysics::SceneHandle sceneHandle, AzPhysics::JointHandle jointHandle) 
    {
        if (AzPhysics::Scene* scene = m_physxSystem->GetScene(sceneHandle))
        {
            return scene->GetJointFromHandle(jointHandle);
        }

        return nullptr;
    }

    void PhysXSceneInterface::RemoveJoint(AzPhysics::SceneHandle sceneHandle, AzPhysics::JointHandle jointHandle) 
    {
        if (AzPhysics::Scene* scene = m_physxSystem->GetScene(sceneHandle))
        {
            scene->RemoveJoint(jointHandle);
        }
    }

    AzPhysics::SceneQueryHits PhysXSceneInterface::QueryScene(
        AzPhysics::SceneHandle sceneHandle, const AzPhysics::SceneQueryRequest* request)
    {
        if (AzPhysics::Scene* scene = m_physxSystem->GetScene(sceneHandle))
        {
            return scene->QueryScene(request);
        }
        return AzPhysics::SceneQueryHits();
    }

    bool PhysXSceneInterface::QueryScene(
        AzPhysics::SceneHandle sceneHandle, const AzPhysics::SceneQueryRequest* request, AzPhysics::SceneQueryHits& result)
    {
        if (AzPhysics::Scene* scene = m_physxSystem->GetScene(sceneHandle))
        {
            return scene->QueryScene(request, result);
        }
        return false;
    }

    AzPhysics::SceneQueryHitsList PhysXSceneInterface::QuerySceneBatch(
        AzPhysics::SceneHandle sceneHandle, const AzPhysics::SceneQueryRequests& requests)
    {
        if (AzPhysics::Scene* scene = m_physxSystem->GetScene(sceneHandle))
        {
            return scene->QuerySceneBatch(requests);
        }
        return {}; //return an empty list
    }

    bool PhysXSceneInterface::QuerySceneAsync(
        AzPhysics::SceneHandle sceneHandle, AzPhysics::SceneQuery::AsyncRequestId requestId,
        const AzPhysics::SceneQueryRequest* request, AzPhysics::SceneQuery::AsyncCallback callback)
    {
        if (AzPhysics::Scene* scene = m_physxSystem->GetScene(sceneHandle))
        {
            return scene->QuerySceneAsync(requestId, request, callback);
        }
        return false;
    }

    bool PhysXSceneInterface::QuerySceneAsyncBatch(
        AzPhysics::SceneHandle sceneHandle, AzPhysics::SceneQuery::AsyncRequestId requestId,
        const AzPhysics::SceneQueryRequests& requests, AzPhysics::SceneQuery::AsyncBatchCallback callback)
    {
        if (AzPhysics::Scene* scene = m_physxSystem->GetScene(sceneHandle))
        {
            return scene->QuerySceneAsyncBatch(requestId, requests, callback);
        }
        return false;
    }

    void PhysXSceneInterface::SuppressCollisionEvents(AzPhysics::SceneHandle sceneHandle,
        const AzPhysics::SimulatedBodyHandle& bodyHandleA,
        const AzPhysics::SimulatedBodyHandle& bodyHandleB)
    {
        if (AzPhysics::Scene* scene = m_physxSystem->GetScene(sceneHandle))
        {
            scene->SuppressCollisionEvents(bodyHandleA, bodyHandleB);
        }

    }

    void PhysXSceneInterface::UnsuppressCollisionEvents(AzPhysics::SceneHandle sceneHandle,
        const AzPhysics::SimulatedBodyHandle& bodyHandleA,
        const AzPhysics::SimulatedBodyHandle& bodyHandleB)
    {
        if (AzPhysics::Scene* scene = m_physxSystem->GetScene(sceneHandle))
        {
            scene->UnsuppressCollisionEvents(bodyHandleA, bodyHandleB);
        }
    }

    void PhysXSceneInterface::SetGravity(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& gravity)
    {
        if (AzPhysics::Scene* scene = m_physxSystem->GetScene(sceneHandle))
        {
            scene->SetGravity(gravity);
        }
    }

    AZ::Vector3 PhysXSceneInterface::GetGravity(AzPhysics::SceneHandle sceneHandle) const
    {
        if (AzPhysics::Scene* scene = m_physxSystem->GetScene(sceneHandle))
        {
            return scene->GetGravity();
        }
        return AZ::Vector3::CreateZero();
    }

    void PhysXSceneInterface::RegisterSceneConfigurationChangedEventHandler(AzPhysics::SceneHandle sceneHandle,
        AzPhysics::SceneEvents::OnSceneConfigurationChanged::Handler& handler)
    {
        Internal::EventRegisterHelper(m_physxSystem, sceneHandle, handler, &AzPhysics::Scene::RegisterSceneConfigurationChangedEventHandler);
    }
    
    void PhysXSceneInterface::RegisterSimulationBodyAddedHandler(AzPhysics::SceneHandle sceneHandle,
        AzPhysics::SceneEvents::OnSimulationBodyAdded::Handler& handler)
    {
        Internal::EventRegisterHelper(m_physxSystem, sceneHandle, handler, &AzPhysics::Scene::RegisterSimulationBodyAddedHandler);
    }

    void PhysXSceneInterface::RegisterSimulationBodyRemovedHandler(AzPhysics::SceneHandle sceneHandle,
        AzPhysics::SceneEvents::OnSimulationBodyRemoved::Handler& handler)
    {
        Internal::EventRegisterHelper(m_physxSystem, sceneHandle, handler, &AzPhysics::Scene::RegisterSimulationBodyRemovedHandler);
    }

    void PhysXSceneInterface::RegisterSimulationBodySimulationEnabledHandler(AzPhysics::SceneHandle sceneHandle,
        AzPhysics::SceneEvents::OnSimulationBodySimulationEnabled::Handler& handler)
    {
        Internal::EventRegisterHelper(m_physxSystem, sceneHandle, handler, &AzPhysics::Scene::RegisterSimulationBodySimulationEnabledHandler);
    }

    void PhysXSceneInterface::RegisterSimulationBodySimulationDisabledHandler(AzPhysics::SceneHandle sceneHandle,
        AzPhysics::SceneEvents::OnSimulationBodySimulationDisabled::Handler& handler)
    {
        Internal::EventRegisterHelper(m_physxSystem, sceneHandle, handler, &AzPhysics::Scene::RegisterSimulationBodySimulationDisabledHandler);
    }

    void PhysXSceneInterface::RegisterSceneSimulationStartHandler(AzPhysics::SceneHandle sceneHandle, AzPhysics::SceneEvents::OnSceneSimulationStartHandler& handler)
    {
        Internal::EventRegisterHelper(m_physxSystem, sceneHandle, handler, &AzPhysics::Scene::RegisterSceneSimulationStartHandler);
    }

    void PhysXSceneInterface::RegisterSceneSimulationFinishHandler(AzPhysics::SceneHandle sceneHandle, AzPhysics::SceneEvents::OnSceneSimulationFinishHandler& handler)
    {
        Internal::EventRegisterHelper(m_physxSystem, sceneHandle, handler, &AzPhysics::Scene::RegisterSceneSimulationFinishHandler);
    }

    void PhysXSceneInterface::RegisterSceneActiveSimulatedBodiesHandler(AzPhysics::SceneHandle sceneHandle, AzPhysics::SceneEvents::OnSceneActiveSimulatedBodiesEvent::Handler& handler)
    {
        Internal::EventRegisterHelper(m_physxSystem, sceneHandle, handler, &AzPhysics::Scene::RegisterSceneActiveSimulatedBodiesHandler);
    }

    void PhysXSceneInterface::RegisterSceneCollisionEventHandler(AzPhysics::SceneHandle sceneHandle,
        AzPhysics::SceneEvents::OnSceneCollisionsEvent::Handler& handler)
    {
        Internal::EventRegisterHelper(m_physxSystem, sceneHandle, handler, &AzPhysics::Scene::RegisterSceneCollisionEventHandler);
    }

    void PhysXSceneInterface::RegisterSceneTriggersEventHandler(AzPhysics::SceneHandle sceneHandle,
        AzPhysics::SceneEvents::OnSceneTriggersEvent::Handler& handler)
    {
        Internal::EventRegisterHelper(m_physxSystem, sceneHandle, handler, &AzPhysics::Scene::RegisterSceneTriggersEventHandler);
    }

    void PhysXSceneInterface::RegisterSceneGravityChangedEvent(AzPhysics::SceneHandle sceneHandle, AzPhysics::SceneEvents::OnSceneGravityChangedEvent::Handler& handler)
    {
        Internal::EventRegisterHelper(m_physxSystem, sceneHandle, handler, &AzPhysics::Scene::RegisterSceneGravityChangedEvent);
    }
}
