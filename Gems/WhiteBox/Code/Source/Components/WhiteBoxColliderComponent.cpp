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
#include <AzCore/Component/NonUniformScaleBus.h>
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
                ->Version(5)
                ->Field("MeshData", &WhiteBoxColliderComponent::m_shapeConfiguration)
                ->Field("PartData", &WhiteBoxColliderComponent::m_partShapeConfigurations)
                ->Field("BooleanPartData", &WhiteBoxColliderComponent::m_booleanPartShapeConfigurations)
                ->Field("BooleanMeshData", &WhiteBoxColliderComponent::m_booleanShapeConfiguration)
                ->Field("HasBooleanMesh", &WhiteBoxColliderComponent::m_hasBooleanMesh)
                ->Field("UseBooleanMesh", &WhiteBoxColliderComponent::m_useBooleanMesh)
                ->Field("DrawCollider", &WhiteBoxColliderComponent::m_drawCollider)
                ->Field("DebugVertices", &WhiteBoxColliderComponent::m_debugVertices)
                ->Field("DebugIndices", &WhiteBoxColliderComponent::m_debugIndices)
                ->Field("BakedNonUniformScale", &WhiteBoxColliderComponent::m_bakedNonUniformScale)
                ->Field("BakedBooleanNonUniformScale", &WhiteBoxColliderComponent::m_bakedBooleanNonUniformScale)
                ->Field("DebugBakedNonUniformScale", &WhiteBoxColliderComponent::m_debugBakedNonUniformScale)
                ->Field("BakedRenderDataNonUniformScale", &WhiteBoxColliderComponent::m_bakedRenderDataNonUniformScale)
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
        // Handles its own rigid body, so other rigid bodies would conflict.
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

    AZ::Vector3 WhiteBoxColliderComponent::RelativeScale(const AZ::Vector3& currentScale, const AZ::Vector3& bakedScale)
    {
        const auto divide = [](const float current, const float baked)
        {
            return AZ::IsClose(baked, 0.0f) ? 1.0f : current / baked;
        };
        return AZ::Vector3(
            divide(currentScale.GetX(), bakedScale.GetX()),
            divide(currentScale.GetY(), bakedScale.GetY()),
            divide(currentScale.GetZ(), bakedScale.GetZ()));
    }

    void WhiteBoxColliderComponent::Activate()
    {
        RebuildBody();

        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        WhiteBoxColliderRequestBus::Handler::BusConnect(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());

        // A non-uniform scale change does not fire a transform notification, so listen for it
        // explicitly and rebuild the body (its cooked shape scale is derived from it).
        m_nonUniformScaleChangedHandler = AZ::NonUniformScaleChangedEvent::Handler(
            [this]([[maybe_unused]] const AZ::Vector3& scale)
            {
                RebuildBody();
            });
        AZ::NonUniformScaleRequestBus::Event(
            GetEntityId(), &AZ::NonUniformScaleRequests::RegisterScaleChangedEvent, m_nonUniformScaleChangedHandler);
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

        // Also apply the current non-uniform scale, relative to the factor the editor baked into the
        // mesh, so a runtime resize is reflected without applying it twice.
        AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(
            nonUniformScale, entityId, &AZ::NonUniformScaleRequests::GetScale);
        m_builtNonUniformScale = nonUniformScale;
        const AZ::Vector3 bakedScale =
            (m_useBooleanMesh && m_hasBooleanMesh) ? m_bakedBooleanNonUniformScale : m_bakedNonUniformScale;
        shapeConfiguration.m_scale = AZ::Vector3(m_builtScale) * RelativeScale(nonUniformScale, bakedScale);

        // Convex Parts become one shape each; otherwise the single cooked mesh is the shape.
        const auto& parts = (m_useBooleanMesh && m_hasBooleanMesh) ? m_booleanPartShapeConfigurations : m_partShapeConfigurations;
        AzPhysics::ShapeVariantData shape;
        if (!parts.empty())
        {
            AZStd::vector<AZStd::shared_ptr<Physics::Shape>> shapes;
            for (const auto& part : parts)
            {
                Physics::CookedMeshShapeConfiguration scaledPart = part;
                scaledPart.m_scale = shapeConfiguration.m_scale;
                AZStd::shared_ptr<Physics::Shape> partShape;
                Physics::SystemRequestBus::BroadcastResult(
                    partShape, &Physics::SystemRequests::CreateShape, m_physicsColliderConfiguration, scaledPart);
                if (partShape)
                {
                    shapes.push_back(AZStd::move(partShape));
                }
            }
            shape = AZStd::move(shapes);
        }
        else if (shapeConfiguration.GetCookedMeshData().empty())
        {
            // nothing cooked for the selected variant - skip creating a shape rather than
            // letting PhysX fail on an empty buffer.
            AZ_Warning(
                "WhiteBox", false,
                "WhiteBoxColliderComponent has no cooked mesh data for the %s collider; skipping physics body creation",
                m_useBooleanMesh ? "boolean" : "base");
            return;
        }
        else
        {
            // create shape from the currently selected (base or boolean-evaluated) cooked mesh
            AZStd::shared_ptr<Physics::Shape> single;
            Physics::SystemRequestBus::BroadcastResult(
                single, &Physics::SystemRequests::CreateShape, m_physicsColliderConfiguration, shapeConfiguration);
            shape = AZStd::move(single);
        }

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

    void WhiteBoxColliderComponent::BakeCollider(const bool useBooleanMesh, const bool forceRebuild)
    {
        // ignore requests for the boolean mesh when one was not baked
        const bool desired = useBooleanMesh && m_hasBooleanMesh;
        if (desired == m_useBooleanMesh && !forceRebuild)
        {
            return; // already using the requested collider and no rebuild was forced
        }

        m_useBooleanMesh = desired;
        // Rebuilding re-reads the entity scale, re-baking the collider to the current size.
        RebuildBody();
    }

    void WhiteBoxColliderComponent::Deactivate()
    {
        m_nonUniformScaleChangedHandler.Disconnect();
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

        AZ::Transform worldTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        // The transform has no non-uniform scale; apply the part not already baked into the debug vertices.
        AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(
            nonUniformScale, GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);
        const auto toWorldScaled = [&worldTransform](const AZ::Vector3& scale, const AZ::Vector3& localPoint)
        {
            return worldTransform.TransformPoint(scale * localPoint);
        };
        const AZ::Vector3 debugScale = RelativeScale(nonUniformScale, m_debugBakedNonUniformScale);
        const auto toWorld = [&](const AZ::Vector3& localPoint)
        {
            return toWorldScaled(debugScale, localPoint);
        };

        debugDisplay.DepthTestOn();
        debugDisplay.SetColor(AZ::Color(1.0f, 0.25f, 0.1f, 1.0f)); // orange wireframe (game mode)

        // Prefer the cooked collider geometry; fall back to the render faces for older data.
        if (m_debugVertices.size() >= 3 && m_debugIndices.size() >= 3)
        {
            for (size_t i = 0; i + 2 < m_debugIndices.size(); i += 3)
            {
                const AZ::Vector3 a = toWorld(m_debugVertices[m_debugIndices[i]]);
                const AZ::Vector3 b = toWorld(m_debugVertices[m_debugIndices[i + 1]]);
                const AZ::Vector3 c = toWorld(m_debugVertices[m_debugIndices[i + 2]]);
                debugDisplay.DrawLine(a, b);
                debugDisplay.DrawLine(b, c);
                debugDisplay.DrawLine(c, a);
            }
            return;
        }

        const auto* whiteBoxComponent = GetEntity()->FindComponent<WhiteBoxComponent>();
        if (whiteBoxComponent == nullptr)
        {
            return;
        }
        const WhiteBoxRenderData& renderData = whiteBoxComponent->GetPhysicsRenderData();
        const AZ::Vector3 renderScale = RelativeScale(nonUniformScale, m_bakedRenderDataNonUniformScale);
        for (const WhiteBoxFace& face : renderData.m_faces)
        {
            const AZ::Vector3 a = toWorldScaled(renderScale, face.m_v1.m_position);
            const AZ::Vector3 b = toWorldScaled(renderScale, face.m_v2.m_position);
            const AZ::Vector3 c = toWorldScaled(renderScale, face.m_v3.m_position);
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
