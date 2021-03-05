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

#include "WhiteBox_precompiled.h"

#include "WhiteBoxColliderComponent.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/RigidBody.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/World.h>

namespace WhiteBox
{
    void WhiteBoxColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        WhiteBoxColliderConfiguration::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WhiteBoxColliderComponent, AZ::Component>()
                ->Version(1)
                ->Field("MeshData", &WhiteBoxColliderComponent::m_shapeConfiguration)
                ->Field("Configuration", &WhiteBoxColliderComponent::m_physicsColliderConfiguration)
                ->Field("WhiteBoxConfiguration", &WhiteBoxColliderComponent::m_whiteBoxColliderConfiguration);
        }
    }

    void WhiteBoxColliderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    WhiteBoxColliderComponent::WhiteBoxColliderComponent(
        const Physics::CookedMeshShapeConfiguration& shapeConfiguration,
        const Physics::ColliderConfiguration& physicsColliderConfiguration,
        const WhiteBoxColliderConfiguration& whiteBoxColliderConfiguration)
        : m_shapeConfiguration(shapeConfiguration)
        , m_physicsColliderConfiguration(physicsColliderConfiguration)
        , m_whiteBoxColliderConfiguration(whiteBoxColliderConfiguration)
    {
    }

    void WhiteBoxColliderComponent::Activate()
    {
        const AZ::EntityId entityId = GetEntityId();

        AZ::Transform worldTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldTransform, entityId, &AZ::TransformInterface::GetWorldTM);

        Physics::RigidBodyConfiguration bodyConfiguration;
        bodyConfiguration.m_debugName = GetEntity()->GetName().c_str();
        bodyConfiguration.m_entityId = entityId;
        bodyConfiguration.m_orientation = worldTransform.GetRotation();
        bodyConfiguration.m_position = worldTransform.GetTranslation();
        bodyConfiguration.m_kinematic = true; // note: this field is ignored in the WhiteBoxBodyType::Static case

        // create rigid body
        switch (m_whiteBoxColliderConfiguration.m_bodyType)
        {
        case WhiteBoxBodyType::Kinematic:
            {
                Physics::SystemRequestBus::BroadcastResult(
                    m_rigidBody, &Physics::SystemRequests::CreateRigidBody, bodyConfiguration);
            }
            break;
        case WhiteBoxBodyType::Static:
            {
                Physics::SystemRequestBus::BroadcastResult(
                    m_rigidBody, &Physics::SystemRequests::CreateStaticRigidBody, bodyConfiguration);
            }
            break;
        default:
            AZ_Assert(
                false, "WhiteBoxBodyType %d not handled", static_cast<int>(m_whiteBoxColliderConfiguration.m_bodyType));
            break;
        }

        // create shape
        AZStd::shared_ptr<Physics::Shape> shape;
        Physics::SystemRequestBus::BroadcastResult(
            shape, &Physics::SystemRequests::CreateShape, m_physicsColliderConfiguration, m_shapeConfiguration);

        AZStd::visit(
            [&shape](auto& rigidBody)
            {
                if (rigidBody)
                {
                    // attach shape
                    rigidBody->AddShape(shape);

                    // add body to the world
                    Physics::WorldRequestBus::Event(
                        Physics::DefaultPhysicsWorldId, &Physics::World::AddBody, *rigidBody);
                }
            },
            m_rigidBody);

        AZ::TransformNotificationBus::Handler::BusConnect(entityId);
    }

    void WhiteBoxColliderComponent::Deactivate()
    {
        AZ::TransformNotificationBus::Handler::BusDisconnect();

        AZStd::visit(
            [](auto& rigidBody)
            {
                rigidBody.reset();
            },
            m_rigidBody);
    }

    void WhiteBoxColliderComponent::OnTransformChanged(
        [[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
    {
        const AZ::Transform worldTransformWithoutScale = [worldTransform = world]() mutable
        {
            worldTransform.SetScale(AZ::Vector3::CreateOne());
            return worldTransform;
        }();

        if (auto rigidBody = AZStd::get_if<AZStd::unique_ptr<Physics::RigidBody>>(&m_rigidBody))
        {
            // move the rigid body kinematically so dynamic rigid bodies will be affected by the movement
            (*rigidBody)->SetKinematicTarget(worldTransformWithoutScale);
        }
        else if (AZStd::get_if<AZStd::unique_ptr<Physics::RigidBodyStatic>>(&m_rigidBody))
        {
            AZ_WarningOnce(
                "WhiteBox", false,
                "The White Box Collider must be made Kinematic to respond to OnTransformChanged events");
        }
    }
} // namespace WhiteBox
