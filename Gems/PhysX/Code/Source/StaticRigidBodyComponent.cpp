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

#include <PhysX_precompiled.h>

#include <Source/StaticRigidBodyComponent.h>
#include <Source/RigidBodyStatic.h>
#include <Source/Utils.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Utils.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>
#include <AzCore/Component/Entity.h>

#include <PhysX/ColliderComponentBus.h>

namespace PhysX
{
    // Definitions are put in .cpp so we can have AZStd::unique_ptr<T> member with forward declared T in the header
    // This causes AZStd::unique_ptr<T> ctor/dtor to be generated when full type info is available
    StaticRigidBodyComponent::StaticRigidBodyComponent() = default;
    StaticRigidBodyComponent::~StaticRigidBodyComponent() = default;

    StaticRigidBodyComponent::StaticRigidBodyComponent(AzPhysics::SceneHandle sceneHandle)
        : m_attachedSceneHandle(sceneHandle)
    {

    }

    void StaticRigidBodyComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<StaticRigidBodyComponent, AZ::Component>()
                ->Version(1)
                ;
        }
    }

    void StaticRigidBodyComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("PhysicsWorldBodyService", 0x944da0cc));
        provided.push_back(AZ_CRC("PhysXStaticRigidBodyService", 0xaae8973b));
    }

    void StaticRigidBodyComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    void StaticRigidBodyComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        // There can be only one StaticRigidBodyComponent per entity
        incompatible.push_back(AZ_CRC("PhysXStaticRigidBodyService", 0xaae8973b));
        // Cannot have both StaticRigidBodyComponent and RigidBodyComponent
        incompatible.push_back(AZ_CRC("PhysXRigidBodyService", 0x1d4c64a8));
    }

    void StaticRigidBodyComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC("PhysXColliderService", 0x4ff43f7c));
    }

    void StaticRigidBodyComponent::InitStaticRigidBody()
    {
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformInterface::GetWorldTM);

        AzPhysics::StaticRigidBodyConfiguration configuration;
        configuration.m_orientation = transform.GetRotation();
        configuration.m_position = transform.GetTranslation();
        configuration.m_entityId = GetEntityId();
        configuration.m_debugName = GetEntity()->GetName();

        AZStd::vector<AZStd::shared_ptr<Physics::Shape>> allshapes;
        ColliderComponentRequestBus::EnumerateHandlersId(GetEntityId(), [&allshapes](ColliderComponentRequests* handler)
        {
            const AZStd::vector<AZStd::shared_ptr<Physics::Shape>>& shapes = handler->GetShapes();
            allshapes.insert(allshapes.end(), shapes.begin(), shapes.end());
            return true;
        });
        configuration.m_colliderAndShapeData = allshapes;

        if (m_attachedSceneHandle == AzPhysics::InvalidSceneHandle)
        {
            Physics::DefaultWorldBus::BroadcastResult(m_attachedSceneHandle, &Physics::DefaultWorldRequests::GetDefaultSceneHandle);
        }
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            m_staticRigidBodyHandle = sceneInterface->AddSimulatedBody(m_attachedSceneHandle, &configuration);
            m_staticRigidBody = azdynamic_cast<PhysX::StaticRigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(m_attachedSceneHandle, m_staticRigidBodyHandle));
        }
    }

    void StaticRigidBodyComponent::Activate()
    {
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());

        InitStaticRigidBody();

        AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusConnect(GetEntityId());
    }

    void StaticRigidBodyComponent::Deactivate()
    {
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneInterface->RemoveSimulatedBody(m_attachedSceneHandle, m_staticRigidBodyHandle);
            m_staticRigidBodyHandle = AzPhysics::InvalidSceneHandle;
            m_staticRigidBody = nullptr;
        }

        AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void StaticRigidBodyComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_staticRigidBody->SetTransform(world);
    }

    void StaticRigidBodyComponent::EnablePhysics()
    {
        if (IsPhysicsEnabled())
        {
            return;
        }
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneInterface->EnableSimulationOfBody(m_attachedSceneHandle, m_staticRigidBodyHandle);
        }
    }

    void StaticRigidBodyComponent::DisablePhysics()
    {
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneInterface->DisableSimulationOfBody(m_attachedSceneHandle, m_staticRigidBodyHandle);
        }
    }

    bool StaticRigidBodyComponent::IsPhysicsEnabled() const
    {
        return m_staticRigidBody != nullptr && m_staticRigidBody->m_simulating;
    }

    AZ::Aabb StaticRigidBodyComponent::GetAabb() const
    {
        return m_staticRigidBody->GetAabb();
    }

    AzPhysics::SimulatedBodyHandle StaticRigidBodyComponent::GetSimulatedBodyHandle() const
    {
        return m_staticRigidBodyHandle;
    }

    AzPhysics::SimulatedBody* StaticRigidBodyComponent::GetSimulatedBody()
    {
        return m_staticRigidBody;
    }

    AzPhysics::SceneQueryHit StaticRigidBodyComponent::RayCast(const AzPhysics::RayCastRequest& request)
    {
        if (m_staticRigidBody)
        {
            return m_staticRigidBody->RayCast(request);
        }
        return AzPhysics::SceneQueryHit();
    }

} // namespace PhysX
