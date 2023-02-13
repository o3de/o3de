/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
        provided.push_back(AZ_CRC_CE("PhysicsWorldBodyService"));
        provided.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
        provided.push_back(AZ_CRC_CE("PhysicsStaticRigidBodyService"));
    }

    void StaticRigidBodyComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void StaticRigidBodyComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
    }

    void StaticRigidBodyComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void StaticRigidBodyComponent::CreateRigidBody()
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

        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            configuration.m_startSimulationEnabled = false; // enable physics will enable this when called.
            m_staticRigidBodyHandle = sceneInterface->AddSimulatedBody(m_attachedSceneHandle, &configuration);
        }

        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        Physics::RigidBodyRequestBus::Handler::BusConnect(GetEntityId());
        AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusConnect(GetEntityId());
    }

    void StaticRigidBodyComponent::DestroyRigidBody()
    {
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneInterface->RemoveSimulatedBody(m_attachedSceneHandle, m_staticRigidBodyHandle);
            m_staticRigidBodyHandle = AzPhysics::InvalidSimulatedBodyHandle;
        }

        Physics::RigidBodyRequestBus::Handler::BusDisconnect();
        AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void StaticRigidBodyComponent::Activate()
    {
        if (m_attachedSceneHandle == AzPhysics::InvalidSceneHandle)
        {
            Physics::DefaultWorldBus::BroadcastResult(m_attachedSceneHandle, &Physics::DefaultWorldRequests::GetDefaultSceneHandle);
        }

        if (m_attachedSceneHandle == AzPhysics::InvalidSceneHandle)
        {
            // Early out if there's no relevant physics world present.
            // It may be a valid case when we have game-time components assigned to editor entities via a script
            // so no need to print a warning here.
            return;
        }

        // During activation all the collider components will create their physics shapes.
        // Delaying the creation of the rigid body to OnEntityActivated so all the shapes are ready.
        AZ::EntityBus::Handler::BusConnect(GetEntityId());
    }

    void StaticRigidBodyComponent::OnEntityActivated([[maybe_unused]] const AZ::EntityId& entityId)
    {
        AZ::EntityBus::Handler::BusDisconnect();

        CreateRigidBody();

        EnablePhysics();
    }

    void StaticRigidBodyComponent::Deactivate()
    {
        if (m_attachedSceneHandle == AzPhysics::InvalidSceneHandle)
        {
            return;
        }

        DisablePhysics();

        DestroyRigidBody();

        AZ::EntityBus::Handler::BusDisconnect();
    }

    void StaticRigidBodyComponent::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
    {
        if (AzPhysics::SimulatedBody* body = GetSimulatedBody())
        {
            body->SetTransform(world);
        }
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

        Physics::RigidBodyNotificationBus::Event(GetEntityId(), &Physics::RigidBodyNotificationBus::Events::OnPhysicsEnabled, GetEntityId());
    }

    void StaticRigidBodyComponent::DisablePhysics()
    {
        if (!IsPhysicsEnabled())
        {
            return;
        }
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneInterface->DisableSimulationOfBody(m_attachedSceneHandle, m_staticRigidBodyHandle);
        }

        Physics::RigidBodyNotificationBus::Event(GetEntityId(), &Physics::RigidBodyNotificationBus::Events::OnPhysicsDisabled, GetEntityId());
    }

    bool StaticRigidBodyComponent::IsPhysicsEnabled() const
    {
        if (m_staticRigidBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
                sceneInterface != nullptr &&
                sceneInterface->IsEnabled(m_attachedSceneHandle))//check if the scene is enabled
            {
                if (AzPhysics::SimulatedBody* body = sceneInterface->GetSimulatedBodyFromHandle(m_attachedSceneHandle, m_staticRigidBodyHandle))
                {
                    return body->m_simulating;
                }
            }
        }
        return false;
    }

    AZ::Aabb StaticRigidBodyComponent::GetAabb() const
    {
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            if (AzPhysics::SimulatedBody* body = sceneInterface->GetSimulatedBodyFromHandle(m_attachedSceneHandle, m_staticRigidBodyHandle))
            {
                return body->GetAabb();
            }
        }
        return AZ::Aabb::CreateNull();
    }

    AzPhysics::SimulatedBodyHandle StaticRigidBodyComponent::GetSimulatedBodyHandle() const
    {
        return m_staticRigidBodyHandle;
    }

    AzPhysics::SimulatedBody* StaticRigidBodyComponent::GetSimulatedBody()
    {
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            return sceneInterface->GetSimulatedBodyFromHandle(m_attachedSceneHandle, m_staticRigidBodyHandle);
        }
        return nullptr;
    }

    AzPhysics::SceneQueryHit StaticRigidBodyComponent::RayCast(const AzPhysics::RayCastRequest& request)
    {
        if (auto* body = azdynamic_cast<PhysX::StaticRigidBody*>(GetSimulatedBody()))
        {
            return body->RayCast(request);
        }
        return AzPhysics::SceneQueryHit();
    }

    AZ::Vector3 StaticRigidBodyComponent::GetCenterOfMassWorld() const
    {
        return AZ::Vector3::CreateZero();
    }

    AZ::Vector3 StaticRigidBodyComponent::GetCenterOfMassLocal() const
    {
        return AZ::Vector3::CreateZero();
    }

    AZ::Matrix3x3 StaticRigidBodyComponent::GetInertiaWorld() const
    {
        return AZ::Matrix3x3::CreateIdentity();
    }

    AZ::Matrix3x3 StaticRigidBodyComponent::GetInertiaLocal() const
    {
        return AZ::Matrix3x3::CreateIdentity();
    }

    AZ::Matrix3x3 StaticRigidBodyComponent::GetInverseInertiaWorld() const
    {
        return AZ::Matrix3x3::CreateIdentity();
    }

    AZ::Matrix3x3 StaticRigidBodyComponent::GetInverseInertiaLocal() const
    {
        return AZ::Matrix3x3::CreateIdentity();
    }

    float StaticRigidBodyComponent::GetMass() const
    {
        return 0.0f;
    }

    float StaticRigidBodyComponent::GetInverseMass() const
    {
        return 0.0f;
    }

    void StaticRigidBodyComponent::SetMass(float)
    {
    }

    void StaticRigidBodyComponent::SetCenterOfMassOffset(const AZ::Vector3&)
    {
    }

    AZ::Vector3 StaticRigidBodyComponent::GetLinearVelocity() const
    {
        return AZ::Vector3::CreateZero();
    }

    void StaticRigidBodyComponent::SetLinearVelocity(const AZ::Vector3&)
    {
    }

    AZ::Vector3 StaticRigidBodyComponent::GetAngularVelocity() const
    {
        return AZ::Vector3::CreateZero();
    }

    void StaticRigidBodyComponent::SetAngularVelocity(const AZ::Vector3&)
    {
    }

    AZ::Vector3 StaticRigidBodyComponent::GetLinearVelocityAtWorldPoint(const AZ::Vector3&) const
    {
        return AZ::Vector3::CreateZero();
    }

    void StaticRigidBodyComponent::ApplyLinearImpulse(const AZ::Vector3&)
    {
    }

    void StaticRigidBodyComponent::ApplyLinearImpulseAtWorldPoint(const AZ::Vector3&, const AZ::Vector3&)
    {
    }

    void StaticRigidBodyComponent::ApplyAngularImpulse(const AZ::Vector3&)
    {
    }

    float StaticRigidBodyComponent::GetLinearDamping() const
    {
        return 0.0f;
    }

    void StaticRigidBodyComponent::SetLinearDamping(float)
    {
    }

    float StaticRigidBodyComponent::GetAngularDamping() const
    {
        return 0.0f;
    }

    void StaticRigidBodyComponent::SetAngularDamping(float)
    {
    }

    bool StaticRigidBodyComponent::IsAwake() const
    {
        return false;
    }

    void StaticRigidBodyComponent::ForceAsleep()
    {
    }

    void StaticRigidBodyComponent::ForceAwake()
    {
    }

    bool StaticRigidBodyComponent::IsKinematic() const
    {
        return false;
    }

    void StaticRigidBodyComponent::SetKinematic(bool)
    {
    }

    void StaticRigidBodyComponent::SetKinematicTarget(const AZ::Transform&)
    {
    }

    bool StaticRigidBodyComponent::IsGravityEnabled() const
    {
        return false;
    }

    void StaticRigidBodyComponent::SetGravityEnabled(bool)
    {
    }

    void StaticRigidBodyComponent::SetSimulationEnabled(bool)
    {
    }

    float StaticRigidBodyComponent::GetSleepThreshold() const
    {
        return 0.0f;
    }

    void StaticRigidBodyComponent::SetSleepThreshold(float)
    {
    }

    AzPhysics::RigidBody* StaticRigidBodyComponent::GetRigidBody()
    {
        return nullptr;
    }
} // namespace PhysX
