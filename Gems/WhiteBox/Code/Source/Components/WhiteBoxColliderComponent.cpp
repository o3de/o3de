/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxColliderComponent.h"
#include "WhiteBoxComponent.h"

#include <Rendering/WhiteBoxRenderData.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/MathUtils.h>
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
                ->Version(3)
                ->Field("MeshData", &WhiteBoxColliderComponent::m_shapeConfiguration)
                ->Field("BooleanMeshData", &WhiteBoxColliderComponent::m_booleanShapeConfiguration)
                ->Field("HasBooleanMesh", &WhiteBoxColliderComponent::m_hasBooleanMesh)
                ->Field("UseBooleanMesh", &WhiteBoxColliderComponent::m_useBooleanMesh)
                ->Field("DrawCollider", &WhiteBoxColliderComponent::m_drawCollider)
                ->Field("Configuration", &WhiteBoxColliderComponent::m_physicsColliderConfiguration)
                ->Field("WhiteBoxConfiguration", &WhiteBoxColliderComponent::m_whiteBoxColliderConfiguration);
        }
    }

    void WhiteBoxColliderComponent::GetProvidedServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
    }

    void WhiteBoxColliderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void WhiteBoxColliderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
        // Incompatible with other rigid bodies because it handles its own rigid body
        // internally and it would conflict if another rigid body is added to the entity.
        incompatible.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
    }

    WhiteBoxColliderComponent::WhiteBoxColliderComponent(
        const Physics::CookedMeshShapeConfiguration& shapeConfiguration,
        const Physics::ColliderConfiguration& physicsColliderConfiguration,
        const WhiteBoxColliderConfiguration& whiteBoxColliderConfiguration,
        const Physics::CookedMeshShapeConfiguration& booleanMeshShape,
        const bool hasBooleanMesh,
        const bool useBooleanMesh)
        : m_shapeConfiguration(shapeConfiguration)
        , m_booleanShapeConfiguration(booleanMeshShape)
        , m_hasBooleanMesh(hasBooleanMesh)
        , m_useBooleanMesh(hasBooleanMesh && useBooleanMesh)
        , m_physicsColliderConfiguration(physicsColliderConfiguration)
        , m_whiteBoxColliderConfiguration(whiteBoxColliderConfiguration)
    {
    }

    const Physics::CookedMeshShapeConfiguration& WhiteBoxColliderComponent::ActiveShapeConfiguration() const
    {
        return (m_useBooleanMesh && m_hasBooleanMesh) ? m_booleanShapeConfiguration : m_shapeConfiguration;
    }

    void WhiteBoxColliderComponent::Activate()
    {
        RebuildBody();

        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        WhiteBoxColliderRequestBus::Handler::BusConnect(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
    }

    void WhiteBoxColliderComponent::RebuildBody()
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

        // remove any existing body so this can be called again to swap the collider mesh
        DestroyBody();

        const AZ::EntityId entityId = GetEntityId();

        AZ::Transform worldTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldTransform, entityId, &AZ::TransformInterface::GetWorldTM);

        // The mesh is cooked unscaled and the body carries no scale, so apply the entity scale to the shape.
        Physics::CookedMeshShapeConfiguration shapeConfiguration = ActiveShapeConfiguration();
        m_builtScale = worldTransform.GetUniformScale();
        shapeConfiguration.m_scale = AZ::Vector3(m_builtScale);

        if (shapeConfiguration.GetCookedMeshData().empty())
        {
            // nothing cooked for the selected variant - skip creating a shape rather than
            // letting PhysX fail on an empty buffer.
            AZ_Warning(
                "WhiteBox", false,
                "WhiteBoxColliderComponent has no cooked mesh data for the %s collider; skipping physics body creation",
                m_useBooleanMesh ? "boolean" : "base");
            return;
        }

        // create shape from the currently selected (base or boolean-evaluated) cooked mesh
        AZStd::shared_ptr<Physics::Shape> shape;
        Physics::SystemRequestBus::BroadcastResult(
            shape, &Physics::SystemRequests::CreateShape, m_physicsColliderConfiguration, shapeConfiguration);

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
                // Since the shape used is a triangle mesh the COM, Mass and Inertia
                // cannot be computed. Disable them to use default values.
                bodyConfiguration.m_computeCenterOfMass = false;
                bodyConfiguration.m_computeMass = false;
                bodyConfiguration.m_computeInertiaTensor = false;
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
    }

    void WhiteBoxColliderComponent::DestroyBody()
    {
        if (m_simulatedBodyHandle == AzPhysics::InvalidSimulatedBodyHandle)
        {
            return;
        }

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

    void WhiteBoxColliderComponent::BakeCollider(const bool useBooleanMesh)
    {
        // ignore requests for the boolean mesh when one was not baked
        const bool desired = useBooleanMesh && m_hasBooleanMesh;
        if (desired == m_useBooleanMesh)
        {
            return; // already using the requested collider
        }

        m_useBooleanMesh = desired;
        RebuildBody();
    }

    void WhiteBoxColliderComponent::Deactivate()
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        WhiteBoxColliderRequestBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();

        DestroyBody();
    }

    void WhiteBoxColliderComponent::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (!m_drawCollider)
        {
            return;
        }

        // The collider is cooked from the same mesh the render component draws, so use the
        // render component's currently active faces (base or boolean) as the wireframe. This
        // shows what the physics shape actually is at runtime.
        const auto* whiteBoxComponent = GetEntity()->FindComponent<WhiteBoxComponent>();
        if (whiteBoxComponent == nullptr)
        {
            return;
        }
        const WhiteBoxRenderData& renderData = whiteBoxComponent->GetActiveRenderData();
        if (renderData.m_faces.empty())
        {
            return;
        }

        AZ::Transform worldTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        debugDisplay.DepthTestOn();
        debugDisplay.SetColor(AZ::Color(1.0f, 0.25f, 0.1f, 1.0f)); // orange wireframe (game mode)

        for (const WhiteBoxFace& face : renderData.m_faces)
        {
            const AZ::Vector3 a = worldTransform.TransformPoint(face.m_v1.m_position);
            const AZ::Vector3 b = worldTransform.TransformPoint(face.m_v2.m_position);
            const AZ::Vector3 c = worldTransform.TransformPoint(face.m_v3.m_position);
            debugDisplay.DrawLine(a, b);
            debugDisplay.DrawLine(b, c);
            debugDisplay.DrawLine(c, a);
        }
    }

    void WhiteBoxColliderComponent::OnTransformChanged(
        [[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
    {
        // The shape scale is baked at build time, so a uniform scale change needs a rebuild.
        if (!AZ::IsClose(world.GetUniformScale(), m_builtScale))
        {
            RebuildBody();
            return;
        }

        const AZ::Transform worldTransformWithoutScale = [worldTransform = world]() mutable
        {
            worldTransform.SetUniformScale(1.0f);
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
