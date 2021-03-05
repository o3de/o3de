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
#include <Source/World.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/Utils.h>

#include <PhysX/ColliderComponentBus.h>

namespace PhysX
{
    // Definitions are put in .cpp so we can have AZStd::unique_ptr<T> member with forward declared T in the header
    // This causes AZStd::unique_ptr<T> ctor/dtor to be generated when full type info is available
    StaticRigidBodyComponent::StaticRigidBodyComponent() = default;
    StaticRigidBodyComponent::~StaticRigidBodyComponent() = default;

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
        // Not compatible with cry engine colliders
        incompatible.push_back(AZ_CRC("ColliderService", 0x902d4e93));
        // There can be only one StaticRigidBodyComponent per entity
        incompatible.push_back(AZ_CRC("PhysXStaticRigidBodyService", 0xaae8973b));
        // Cannot have both StaticRigidBodyComponent and RigidBodyComponent
        incompatible.push_back(AZ_CRC("PhysXRigidBodyService", 0x1d4c64a8));
    }

    void StaticRigidBodyComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC("PhysXColliderService", 0x4ff43f7c));
    }

    PhysX::RigidBodyStatic* StaticRigidBodyComponent::GetStaticRigidBody()
    {
        return m_staticRigidBody.get();
    }

    void StaticRigidBodyComponent::InitStaticRigidBody()
    {
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformInterface::GetWorldTM);

        Physics::WorldBodyConfiguration configuration;
        configuration.m_orientation = transform.GetRotation();
        configuration.m_position = transform.GetTranslation();
        configuration.m_entityId = GetEntityId();
        configuration.m_debugName = GetEntity()->GetName();

        m_staticRigidBody = AZStd::make_unique<PhysX::RigidBodyStatic>(configuration);

        ColliderComponentRequestBus::EnumerateHandlersId(GetEntityId(), [this](ColliderComponentRequests* handler)
        {
            const AZStd::vector<AZStd::shared_ptr<Physics::Shape>>& shapes = handler->GetShapes();
            for (auto& shape : shapes)
            {
                m_staticRigidBody->AddShape(shape);
            }
            return true;
        });

        World* world = Utils::GetDefaultWorld();
        world->AddBody(*m_staticRigidBody);
    }

    void StaticRigidBodyComponent::Activate()
    {
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());

        InitStaticRigidBody();

        Physics::WorldBodyRequestBus::Handler::BusConnect(GetEntityId());
        Physics::WorldBodyNotificationBus::Event(GetEntityId(), &Physics::WorldBodyNotifications::OnPhysicsEnabled);
    }

    void StaticRigidBodyComponent::Deactivate()
    {
        Physics::Utils::DeferDelete(AZStd::move(m_staticRigidBody));

        Physics::WorldBodyRequestBus::Handler::BusDisconnect();
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
        World* world = Utils::GetDefaultWorld();
        world->AddBody(*m_staticRigidBody);

        Physics::WorldBodyNotificationBus::Event(GetEntityId(), &Physics::WorldBodyNotifications::OnPhysicsEnabled);
    }

    void StaticRigidBodyComponent::DisablePhysics()
    {
        if (Physics::World* world = m_staticRigidBody->GetWorld())
        {
            world->RemoveBody(*m_staticRigidBody);
        }

        Physics::WorldBodyNotificationBus::Event(GetEntityId(), &Physics::WorldBodyNotifications::OnPhysicsDisabled);
    }

    bool StaticRigidBodyComponent::IsPhysicsEnabled() const
    {
        return m_staticRigidBody != nullptr && m_staticRigidBody->GetWorld() != nullptr;
    }

    AZ::Aabb StaticRigidBodyComponent::GetAabb() const
    {
        return m_staticRigidBody->GetAabb();
    }

    Physics::WorldBody* StaticRigidBodyComponent::GetWorldBody()
    {
        return m_staticRigidBody.get();
    }

    Physics::RayCastHit StaticRigidBodyComponent::RayCast(const Physics::RayCastRequest& request)
    {
        return m_staticRigidBody->RayCast(request);
    }

} // namespace PhysX
