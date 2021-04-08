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

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Configuration/RigidBodyConfiguration.h>
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>

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
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        if (sceneInterface == nullptr)
        {
            AZ_Error("WhiteBox", false, "Missing Physics Scene Interface, unble to Activate WhiteBoxColliderComponent");
            return;
        }

        AzPhysics::SceneHandle defaultScene = sceneInterface->GetSceneHandle(AzPhysics::DefaultPhysicsSceneName);
        if (defaultScene == AzPhysics::InvalidSceneHandle)
        {
            AZ_Error("WhiteBox", false, "Missing Default Physics Scene, unble to Activate WhiteBoxColliderComponent");
            return;
        }

        const AZ::EntityId entityId = GetEntityId();

        AZ::Transform worldTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldTransform, entityId, &AZ::TransformInterface::GetWorldTM);

        // create shape
        AZStd::shared_ptr<Physics::Shape> shape;
        Physics::SystemRequestBus::BroadcastResult(
            shape, &Physics::SystemRequests::CreateShape, m_physicsColliderConfiguration, m_shapeConfiguration);

        // create rigid body
        switch (m_whiteBoxColliderConfiguration.m_bodyType)
        {
        case WhiteBoxBodyType::Kinematic:
            {
                AzPhysics::RigidBodyConfiguration bodyConfiguration;
                bodyConfiguration.m_debugName = GetEntity()->GetName().c_str();
                bodyConfiguration.m_entityId = entityId;
                bodyConfiguration.m_orientation = worldTransform.GetRotation();
                bodyConfiguration.m_position = worldTransform.GetTranslation();
                bodyConfiguration.m_kinematic = true; // note: this field is ignored in the WhiteBoxBodyType::Static case
                bodyConfiguration.m_colliderAndShapeData = shape;
                m_simulatedBodyHandle = sceneInterface->AddSimulatedBody(defaultScene, &bodyConfiguration);
            }
            break;
        case WhiteBoxBodyType::Static:
            {
                AzPhysics::StaticRigidBodyConfiguration staticBodyConfiguration;
                staticBodyConfiguration.m_debugName = GetEntity()->GetName().c_str();
                staticBodyConfiguration.m_entityId = entityId;
                staticBodyConfiguration.m_orientation = worldTransform.GetRotation();
                staticBodyConfiguration.m_position = worldTransform.GetTranslation();
                staticBodyConfiguration.m_colliderAndShapeData = shape;
                m_simulatedBodyHandle = sceneInterface->AddSimulatedBody(defaultScene, &staticBodyConfiguration);
            }
            break;
        default:
            AZ_Assert(
                false, "WhiteBoxBodyType %d not handled", static_cast<int>(m_whiteBoxColliderConfiguration.m_bodyType));
            break;
        }

        AZ::TransformNotificationBus::Handler::BusConnect(entityId);
    }

    void WhiteBoxColliderComponent::Deactivate()
    {
        AZ::TransformNotificationBus::Handler::BusDisconnect();

        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            if (AzPhysics::SceneHandle defaultScene = sceneInterface->GetSceneHandle(AzPhysics::DefaultPhysicsSceneName);
                defaultScene != AzPhysics::InvalidSceneHandle)
            {
                sceneInterface->RemoveSimulatedBody(defaultScene, m_simulatedBodyHandle);
            }
        }
        m_simulatedBodyHandle = AzPhysics::InvalidSimulatedBodyHandle;
    }

    void WhiteBoxColliderComponent::OnTransformChanged(
        [[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
    {
        const AZ::Transform worldTransformWithoutScale = [worldTransform = world]() mutable
        {
            worldTransform.SetScale(AZ::Vector3::CreateOne());
            return worldTransform;
        }();

        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            if (AzPhysics::SceneHandle defaultScene = sceneInterface->GetSceneHandle(AzPhysics::DefaultPhysicsSceneName);
                defaultScene != AzPhysics::InvalidSceneHandle)
            {
                //if this is a rigid body update the transform, otherwise its static for just warn
                if (auto* rigidBody = azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(defaultScene, m_simulatedBodyHandle))) 
                {
                    rigidBody->SetKinematicTarget(worldTransformWithoutScale);
                }
                else
                {
                    AZ_WarningOnce(
                        "WhiteBox", false,
                        "The White Box Collider must be made Kinematic to respond to OnTransformChanged events");
                }
            }
        }
    }
} // namespace WhiteBox
