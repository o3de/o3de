
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/RigidBodyBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Physics/Utils.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>

#include <Source/BaseColliderComponent.h>
#include <Source/RigidBodyComponent.h>
#include <Source/StaticRigidBodyComponent.h>
#include <Source/SystemComponent.h>
#include <Source/Utils.h>
#include <PhysX/MathConversion.h>
#include <PhysX/PhysXLocks.h>
#include <Scene/PhysXScene.h>

namespace PhysX
{
    // ShapeInfoCache
    AZ::Aabb BaseColliderComponent::ShapeInfoCache::GetAabb(const AZStd::vector<AZStd::shared_ptr<Physics::Shape>>& shapes)
    { 
        if (m_cacheOutdated)
        {
            UpdateCache(shapes);
        }
        return m_aabb; 
    }

    void BaseColliderComponent::ShapeInfoCache::InvalidateCache()
    { 
        m_cacheOutdated = true; 
    }

    const AZ::Transform& BaseColliderComponent::ShapeInfoCache::GetWorldTransform()
    {
        return m_worldTransform;
    }

    void BaseColliderComponent::ShapeInfoCache::SetWorldTransform(const AZ::Transform& worldTransform)
    {
        m_worldTransform = worldTransform;
    }

    void BaseColliderComponent::ShapeInfoCache::UpdateCache(const AZStd::vector<AZStd::shared_ptr<Physics::Shape>>& shapes)
    {
        size_t numShapes = shapes.size();

        if (numShapes > 0)
        {
            auto* scene = Utils::GetDefaultScene();
            auto* pxScene = static_cast<physx::PxScene*>(scene->GetNativePointer());
            PHYSX_SCENE_READ_LOCK(pxScene);

            auto pxShape = static_cast<physx::PxShape*>(shapes[0]->GetNativePointer());
            physx::PxTransform pxWorldTransform = PxMathConvert(m_worldTransform);
            physx::PxBounds3 bounds = physx::PxGeometryQuery::getWorldBounds(pxShape->getGeometry().any(),
                pxWorldTransform * pxShape->getLocalPose(), 1.0f);

            for (size_t shapeIndex = 1; shapeIndex < numShapes; ++shapeIndex)
            {
                pxShape = static_cast<physx::PxShape*>(shapes[0]->GetNativePointer());
                bounds.include(physx::PxGeometryQuery::getWorldBounds(pxShape->getGeometry().any(),
                    pxWorldTransform * pxShape->getLocalPose(), 1.0f));
            }

            m_aabb = PxMathConvert(bounds);
        }

        else
        {
            m_aabb = AZ::Aabb::CreateFromPoint(m_worldTransform.GetTranslation());
        }

        m_cacheOutdated = false;
    }

    void BaseColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BaseColliderComponent, AZ::Component>()
                ->Version(1)
                ->Field("ShapeConfigList", &BaseColliderComponent::m_shapeConfigList)
                ;
        }
    }

    void BaseColliderComponent::SetShapeConfigurationList(const AzPhysics::ShapeColliderPairList& shapeConfigList)
    {
        if (GetEntity()->GetState() == AZ::Entity::State::Active)
        {
            AZ_Warning("PhysX", false, "Trying to call SetShapeConfigurationList for entity \"%s\" while entity is active.",
                GetEntity()->GetName().c_str());
            return;
        }
        m_shapeConfigList = shapeConfigList;
    }

    AzPhysics::ShapeColliderPairList BaseColliderComponent::GetShapeConfigurations()
    {
        return m_shapeConfigList;
    }

    AZStd::vector<AZStd::shared_ptr<Physics::Shape>> BaseColliderComponent::GetShapes()
    {
        return m_shapes;
    }

    // TransformNotificationsBus
    void BaseColliderComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_shapeInfoCache.SetWorldTransform(world);
        m_shapeInfoCache.InvalidateCache();
    }

    // PhysX::ColliderShapeBus
    AZ::Aabb BaseColliderComponent::GetColliderShapeAabb()
    {
        if (m_shapes.empty())
        {
            return AZ::Aabb::CreateFromPoint(m_shapeInfoCache.GetWorldTransform().GetTranslation());
        }

        return m_shapeInfoCache.GetAabb(m_shapes);
    }

    bool BaseColliderComponent::IsTrigger()
    {
        AZ_Error("PhysX", !m_shapes.empty(), "Tried to call IsTrigger before any shapes were initialized for entity %s.",
            GetEntity()->GetName().c_str());

        // Colliders now support multiple shapes. This will return true if any of the shapes is a trigger.
        for (const auto& shapeConfigPair : m_shapeConfigList)
        {
            if (shapeConfigPair.first->m_isTrigger)
            {
                return true;
            }
        }

        return false;
    }

    void BaseColliderComponent::SetCollisionLayer(const AZStd::string& layerName, AZ::Crc32 colliderTag)
    {
        for(auto& shape : m_shapes) 
        {
            if (Physics::Utils::FilterTag(shape->GetTag(), colliderTag))
            {
                bool success = false;
                AzPhysics::CollisionLayer layer;
                Physics::CollisionRequestBus::BroadcastResult(success, &Physics::CollisionRequests::TryGetCollisionLayerByName, layerName, layer);
                if (success)
                {
                    shape->SetCollisionLayer(layer);
                }
            }
        }
    }

    AZStd::string BaseColliderComponent::GetCollisionLayerName()
    {
        AZStd::string layerName;
        if (!m_shapes.empty())
        {
            Physics::CollisionRequestBus::BroadcastResult(layerName, &Physics::CollisionRequests::GetCollisionLayerName, m_shapes[0]->GetCollisionLayer());
        }
        return layerName;
    }

    void BaseColliderComponent::SetCollisionGroup(const AZStd::string& groupName, AZ::Crc32 colliderTag)
    {
        for (auto& shape : m_shapes)
        {
            if (Physics::Utils::FilterTag(shape->GetTag(), colliderTag))
            {
                bool success = false;
                AzPhysics::CollisionGroup group;
                Physics::CollisionRequestBus::BroadcastResult(success, &Physics::CollisionRequests::TryGetCollisionGroupByName, groupName, group);
                if (success)
                {
                    shape->SetCollisionGroup(group);
                }
            }
        }
    }

    AZStd::string BaseColliderComponent::GetCollisionGroupName()
    {
        AZStd::string groupName;
        if (!m_shapes.empty())
        {
            Physics::CollisionRequestBus::BroadcastResult(groupName, &Physics::CollisionRequests::GetCollisionGroupName, m_shapes[0]->GetCollisionGroup());
        }
        return groupName;
    }

    void BaseColliderComponent::ToggleCollisionLayer(const AZStd::string& layerName, AZ::Crc32 colliderTag, bool enabled)
    {
        for (const auto& shape : m_shapes)
        {
            if (Physics::Utils::FilterTag(shape->GetTag(), colliderTag))
            {
                bool success = false;
                AzPhysics::CollisionLayer layer;
                Physics::CollisionRequestBus::BroadcastResult(success, &Physics::CollisionRequests::TryGetCollisionLayerByName, layerName, layer);
                if (success)
                {
                    auto group = shape->GetCollisionGroup();
                    group.SetLayer(layer, enabled);
                    shape->SetCollisionGroup(group);
                }
            }
        }
    }


    // AZ::Component
    void BaseColliderComponent::Activate()
    {
        const AZ::EntityId entityId = GetEntityId();

        ColliderComponentRequestBus::Handler::BusConnect(entityId);
        AZ::TransformNotificationBus::Handler::BusConnect(entityId);
        ColliderShapeRequestBus::Handler::BusConnect(GetEntityId());
        Physics::CollisionFilteringRequestBus::Handler::BusConnect(GetEntityId());

        AZ::Transform worldTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldTransform, entityId, &AZ::TransformBus::Events::GetWorldTM);
        m_shapeInfoCache.SetWorldTransform(worldTransform);

        InitShapes();
    }

    void BaseColliderComponent::Deactivate()
    {
        m_shapes.clear();

        Physics::CollisionFilteringRequestBus::Handler::BusDisconnect();
        ColliderShapeRequestBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        ColliderComponentRequestBus::Handler::BusDisconnect();
    }

    void BaseColliderComponent::UpdateScaleForShapeConfigs()
    {
        // Overridden by each collider component
    }

    bool BaseColliderComponent::InitShapes()
    {
        UpdateScaleForShapeConfigs();
        
        if (IsMeshCollider())
        {
            return InitMeshCollider();
        }
        else
        {
            const float transformScale = Utils::GetTransformScale(GetEntityId());

            m_shapes.reserve(m_shapeConfigList.size());

            for (const auto& shapeConfigPair : m_shapeConfigList)
            {
                const AZStd::shared_ptr<Physics::ShapeConfiguration>& shapeConfiguration = shapeConfigPair.second;
                if (!shapeConfiguration)
                {
                    AZ_Error("PhysX", false, "Unable to create a physics shape because shape configuration is null. Entity: %s",
                        GetEntity()->GetName().c_str());
                    return false;
                }

                Physics::ColliderConfiguration colliderConfiguration = *shapeConfigPair.first;
                colliderConfiguration.m_position *= transformScale;

                AZStd::shared_ptr<Physics::Shape> shape;
                Physics::SystemRequestBus::BroadcastResult(shape, &Physics::SystemRequests::CreateShape, colliderConfiguration, *shapeConfiguration);
                
                if (!shape)
                {
                    AZ_Error("PhysX", false, "Failed to create a PhysX shape. Entity: %s", GetEntity()->GetName().c_str());
                    return false;
                }

                m_shapes.push_back(shape);
            }

            return true;
        }
    }
    
    bool BaseColliderComponent::IsMeshCollider() const
    {
        return m_shapeConfigList.size() == 1 && m_shapeConfigList.begin()->second &&
            m_shapeConfigList.begin()->second->GetShapeType() == Physics::ShapeType::PhysicsAsset;
    }

    bool BaseColliderComponent::InitMeshCollider()
    {
        AZ_Assert(IsMeshCollider(), "InitMeshCollider called for a non-mesh collider.");

        const AzPhysics::ShapeColliderPair& shapeConfigurationPair = *(m_shapeConfigList.begin());
        const Physics::ColliderConfiguration& componentColliderConfiguration = *(shapeConfigurationPair.first.get());
        const Physics::PhysicsAssetShapeConfiguration& physicsAssetConfiguration = 
            *(static_cast<const Physics::PhysicsAssetShapeConfiguration*>(shapeConfigurationPair.second.get()));

        if (!physicsAssetConfiguration.m_asset.IsReady())
        {
            return false;
        }

        const bool hasNonUniformScale = (AZ::NonUniformScaleRequestBus::FindFirstHandler(GetEntityId()) != nullptr);
        Utils::GetShapesFromAsset(physicsAssetConfiguration, componentColliderConfiguration, hasNonUniformScale,
            physicsAssetConfiguration.m_subdivisionLevel, m_shapes);

        return true;
    }

}
