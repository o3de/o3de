/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>
#include <AzFramework/Physics/Utils.h>

#include <Source/HeightfieldColliderComponent.h>
#include <Source/RigidBodyStatic.h>
#include <Source/SystemComponent.h>
#include <Source/Utils.h>

#include <PhysX/MathConversion.h>
#include <PhysX/PhysXLocks.h>
#include <Scene/PhysXScene.h>

namespace PhysX
{
    void HeightfieldColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<HeightfieldColliderComponent, AZ::Component>()
                ->Version(1)
                ->Field("ShapeConfig", &HeightfieldColliderComponent::m_shapeConfig)
                ;
        }
    }

    void HeightfieldColliderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysicsWorldBodyService"));
        provided.push_back(AZ_CRC_CE("PhysXColliderService"));
        provided.push_back(AZ_CRC_CE("PhysXHeightfieldColliderService"));
        provided.push_back(AZ_CRC_CE("PhysXStaticRigidBodyService"));
    }

    void HeightfieldColliderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("PhysicsHeightfieldProviderService"));
    }

    void HeightfieldColliderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PhysXColliderService"));
        incompatible.push_back(AZ_CRC_CE("PhysXStaticRigidBodyService"));
        incompatible.push_back(AZ_CRC_CE("PhysXRigidBodyService"));
    }

    HeightfieldColliderComponent::~HeightfieldColliderComponent()
    {
        ClearHeightfield();
    }

    void HeightfieldColliderComponent::Activate()
    {
        const AZ::EntityId entityId = GetEntityId();

        Physics::HeightfieldProviderNotificationBus::Handler::BusConnect(entityId);
        ColliderComponentRequestBus::Handler::BusConnect(entityId);
        ColliderShapeRequestBus::Handler::BusConnect(entityId);
        Physics::CollisionFilteringRequestBus::Handler::BusConnect(entityId);
        AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusConnect(entityId);

        RefreshHeightfield();
    }

    void HeightfieldColliderComponent::Deactivate()
    {
        AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusDisconnect();
        Physics::CollisionFilteringRequestBus::Handler::BusDisconnect();
        ColliderShapeRequestBus::Handler::BusDisconnect();
        ColliderComponentRequestBus::Handler::BusDisconnect();
        Physics::HeightfieldProviderNotificationBus::Handler::BusDisconnect();

        ClearHeightfield();
    }

    void HeightfieldColliderComponent::OnHeightfieldDataChanged([[maybe_unused]] const AZ::Aabb& dirtyRegion)
    {
        RefreshHeightfield();
    }

    void HeightfieldColliderComponent::ClearHeightfield()
    {
        // There are two references to the heightfield data, we need to clear both to make the heightfield clear out and deallocate:
        // - The simulated body has a pointer to the shape, which has a GeometryHolder, which has the Heightfield inside it
        // - The shape config is also holding onto a pointer to the Heightfield

        // We remove the simulated body first, since we don't want the heightfield to exist any more.
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
            sceneInterface && m_staticRigidBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            sceneInterface->RemoveSimulatedBody(m_attachedSceneHandle, m_staticRigidBodyHandle);
        }

        // Now we can safely clear out the cached heightfield pointer.
        Physics::HeightfieldShapeConfiguration& configuration = static_cast<Physics::HeightfieldShapeConfiguration&>(*m_shapeConfig.second);
        configuration.SetCachedNativeHeightfield(nullptr);
    }

    void HeightfieldColliderComponent::InitStaticRigidBody()
    {
        // Get the transform from the HeightfieldProvider.  Because rotation and scale can indirectly affect how the heightfield itself
        // is computed and the size of the heightfield, it's possible that the HeightfieldProvider will provide a different transform
        // back to us than the one that's directly on that entity.
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        Physics::HeightfieldProviderRequestsBus::EventResult(
            transform, GetEntityId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldTransform);

        AzPhysics::StaticRigidBodyConfiguration configuration;
        configuration.m_orientation = transform.GetRotation();
        configuration.m_position = transform.GetTranslation();
        configuration.m_entityId = GetEntityId();
        configuration.m_debugName = GetEntity()->GetName();
        configuration.m_colliderAndShapeData = GetShapeConfigurations();

        if (m_attachedSceneHandle == AzPhysics::InvalidSceneHandle)
        {
            Physics::DefaultWorldBus::BroadcastResult(m_attachedSceneHandle, &Physics::DefaultWorldRequests::GetDefaultSceneHandle);
        }
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            m_staticRigidBodyHandle = sceneInterface->AddSimulatedBody(m_attachedSceneHandle, &configuration);
        }
    }

    void HeightfieldColliderComponent::InitHeightfieldShapeConfiguration()
    {
        Physics::HeightfieldShapeConfiguration& configuration = static_cast<Physics::HeightfieldShapeConfiguration&>(*m_shapeConfig.second);

        configuration = Utils::CreateHeightfieldShapeConfiguration(GetEntityId());
    }

    void HeightfieldColliderComponent::RefreshHeightfield()
    {
        ClearHeightfield();
        InitHeightfieldShapeConfiguration();
        InitStaticRigidBody();
        Physics::ColliderComponentEventBus::Event(GetEntityId(), &Physics::ColliderComponentEvents::OnColliderChanged);
    }

    void HeightfieldColliderComponent::SetShapeConfiguration(const AzPhysics::ShapeColliderPair& shapeConfig)
    {
        if (GetEntity()->GetState() == AZ::Entity::State::Active)
        {
            AZ_Warning(
                "PhysX", false, "Trying to call SetShapeConfiguration for entity \"%s\" while entity is active.",
                GetEntity()->GetName().c_str());
            return;
        }
        m_shapeConfig = shapeConfig;
    }

    // SimulatedBodyComponentRequestsBus
    void HeightfieldColliderComponent::EnablePhysics()
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

    // SimulatedBodyComponentRequestsBus
    void HeightfieldColliderComponent::DisablePhysics()
    {
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneInterface->DisableSimulationOfBody(m_attachedSceneHandle, m_staticRigidBodyHandle);
        }
    }

    // SimulatedBodyComponentRequestsBus
    bool HeightfieldColliderComponent::IsPhysicsEnabled() const
    {
        if (m_staticRigidBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
                sceneInterface != nullptr && sceneInterface->IsEnabled(m_attachedSceneHandle)) // check if the scene is enabled
            {
                if (AzPhysics::SimulatedBody* body =
                        sceneInterface->GetSimulatedBodyFromHandle(m_attachedSceneHandle, m_staticRigidBodyHandle))
                {
                    return body->m_simulating;
                }
            }
        }
        return false;
    }

    // SimulatedBodyComponentRequestsBus
    AzPhysics::SimulatedBodyHandle HeightfieldColliderComponent::GetSimulatedBodyHandle() const
    {
        return m_staticRigidBodyHandle;
    }

    // SimulatedBodyComponentRequestsBus
    AzPhysics::SimulatedBody* HeightfieldColliderComponent::GetSimulatedBody()
    {
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            return sceneInterface->GetSimulatedBodyFromHandle(m_attachedSceneHandle, m_staticRigidBodyHandle);
        }
        return nullptr;
    }

    // SimulatedBodyComponentRequestsBus
    AzPhysics::SceneQueryHit HeightfieldColliderComponent::RayCast(const AzPhysics::RayCastRequest& request)
    {
        if (auto* body = azdynamic_cast<PhysX::StaticRigidBody*>(GetSimulatedBody()))
        {
            return body->RayCast(request);
        }
        return AzPhysics::SceneQueryHit();
    }

    // ColliderComponentRequestBus
    AzPhysics::ShapeColliderPairList HeightfieldColliderComponent::GetShapeConfigurations()
    {
        AzPhysics::ShapeColliderPairList shapeConfigurationList({ m_shapeConfig });
        return shapeConfigurationList;
    }

    AZStd::shared_ptr<Physics::Shape> HeightfieldColliderComponent::GetHeightfieldShape()
    {
        if (auto* body = azdynamic_cast<PhysX::StaticRigidBody*>(GetSimulatedBody()))
        {
            // Heightfields should only have one shape
            AZ_Assert(body->GetShapeCount() == 1, "Heightfield rigid body has the wrong number of shapes:  %zu", body->GetShapeCount());
            return body->GetShape(0);
        }

        return {};
    }

    // ColliderComponentRequestBus
    AZStd::vector<AZStd::shared_ptr<Physics::Shape>> HeightfieldColliderComponent::GetShapes()
    {
        return { GetHeightfieldShape() };
    }

    // PhysX::ColliderShapeBus
    AZ::Aabb HeightfieldColliderComponent::GetColliderShapeAabb()
    {
        // Get the Collider AABB directly from the heightfield provider.
        AZ::Aabb colliderAabb = AZ::Aabb::CreateNull();
        Physics::HeightfieldProviderRequestsBus::EventResult(
            colliderAabb, GetEntityId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldAabb);

        return colliderAabb;
    }

    // SimulatedBodyComponentRequestsBus
    AZ::Aabb HeightfieldColliderComponent::GetAabb() const
    {
        // On the SimulatedBodyComponentRequestsBus, get the AABB from the simulated body instead of the collider.
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            if (AzPhysics::SimulatedBody* body = sceneInterface->GetSimulatedBodyFromHandle(m_attachedSceneHandle, m_staticRigidBodyHandle))
            {
                return body->GetAabb();
            }
        }
        return AZ::Aabb::CreateNull();
    }

    // CollisionFilteringRequestBus
    void HeightfieldColliderComponent::SetCollisionLayer(const AZStd::string& layerName, AZ::Crc32 colliderTag)
    {
        if (auto heightfield = GetHeightfieldShape())
        {
            if (Physics::Utils::FilterTag(heightfield->GetTag(), colliderTag))
            {
                bool success = false;
                AzPhysics::CollisionLayer layer;
                Physics::CollisionRequestBus::BroadcastResult(
                    success, &Physics::CollisionRequests::TryGetCollisionLayerByName, layerName, layer);
                if (success)
                {
                    heightfield->SetCollisionLayer(layer);
                }
            }
        }
    }

    // CollisionFilteringRequestBus
    AZStd::string HeightfieldColliderComponent::GetCollisionLayerName()
    {
        AZStd::string layerName;
        if (auto heightfield = GetHeightfieldShape())
        {
            Physics::CollisionRequestBus::BroadcastResult(
                layerName, &Physics::CollisionRequests::GetCollisionLayerName, heightfield->GetCollisionLayer());
        }
        return layerName;
    }

    // CollisionFilteringRequestBus
    void HeightfieldColliderComponent::SetCollisionGroup(const AZStd::string& groupName, AZ::Crc32 colliderTag)
    {
        if (auto heightfield = GetHeightfieldShape())
        {
            if (Physics::Utils::FilterTag(heightfield->GetTag(), colliderTag))
            {
                bool success = false;
                AzPhysics::CollisionGroup group;
                Physics::CollisionRequestBus::BroadcastResult(
                    success, &Physics::CollisionRequests::TryGetCollisionGroupByName, groupName, group);
                if (success)
                {
                    heightfield->SetCollisionGroup(group);
                }
            }
        }
    }

    // CollisionFilteringRequestBus
    AZStd::string HeightfieldColliderComponent::GetCollisionGroupName()
    {
        AZStd::string groupName;
        if (auto heightfield = GetHeightfieldShape())
        {
            Physics::CollisionRequestBus::BroadcastResult(
                groupName, &Physics::CollisionRequests::GetCollisionGroupName, heightfield->GetCollisionGroup());
        }

        return groupName;
    }

    // CollisionFilteringRequestBus
    void HeightfieldColliderComponent::ToggleCollisionLayer(const AZStd::string& layerName, AZ::Crc32 colliderTag, bool enabled)
    {
        if (auto heightfield = GetHeightfieldShape())
        {
            if (Physics::Utils::FilterTag(heightfield->GetTag(), colliderTag))
            {
                bool success = false;
                AzPhysics::CollisionLayer layer;
                Physics::CollisionRequestBus::BroadcastResult(
                    success, &Physics::CollisionRequests::TryGetCollisionLayerByName, layerName, layer);
                if (success)
                {
                    auto group = heightfield->GetCollisionGroup();
                    group.SetLayer(layer, enabled);
                    heightfield->SetCollisionGroup(group);
                }
            }
        }
    }

} // namespace PhysX
