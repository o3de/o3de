/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Source/HeightfieldColliderComponent.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>
#include <AzFramework/Physics/Utils.h>

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
                ->Version(2)
                ->Field("ColliderConfiguration", &HeightfieldColliderComponent::m_colliderConfig)
                ->Field("BakedHeightfieldAsset", &HeightfieldColliderComponent::m_bakedHeightfieldAsset)
                ;
        }
    }

    void HeightfieldColliderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysicsWorldBodyService"));
        provided.push_back(AZ_CRC_CE("PhysicsColliderService"));
        provided.push_back(AZ_CRC_CE("PhysicsHeightfieldColliderService"));
    }

    void HeightfieldColliderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("PhysicsHeightfieldProviderService"));
    }

    void HeightfieldColliderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PhysicsColliderService"));
        // Incompatible with other rigid bodies because it handles its own rigid body
        // internally and it would conflict if another rigid body is added to the entity.
        incompatible.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
    }

    HeightfieldColliderComponent::~HeightfieldColliderComponent()
    {
    }


    void HeightfieldColliderComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_bakedHeightfieldAsset)
        {
            m_bakedHeightfieldAsset = asset;

            Pipeline::HeightFieldAsset* heightfieldAsset = m_bakedHeightfieldAsset.Get();
            
            bool minMaxHeightsMatch = AZ::IsClose(m_shapeConfig->GetMinHeightBounds(), heightfieldAsset->GetMinHeight()) &&
                AZ::IsClose(m_shapeConfig->GetMaxHeightBounds(), heightfieldAsset->GetMaxHeight());

            if (!minMaxHeightsMatch)
            {
                AZ_Warning(
                    "PhysX",
                    false,
                    "MinMax heights mismatch between baked heightfield and heightfield provider. Entity [%s]. "
                    "Terrain [%0.2f, %0.2f], Asset [%0.2f, %0.2f]",
                    GetEntity()->GetName().c_str(),
                    m_shapeConfig->GetMinHeightBounds(),
                    m_shapeConfig->GetMaxHeightBounds(),
                    heightfieldAsset->GetMinHeight(),
                    heightfieldAsset->GetMaxHeight());
            }

            physx::PxHeightField* pxHeightfield = heightfieldAsset->GetHeightField();

            // Since PxHeightfield will have shared ownership in both HeightfieldAsset and HeightfieldShapeConfiguration,
            // we need to increment the reference counter here. Both of these places call release() in destructors,
            // so we need to avoid double deletion this way.
            pxHeightfield->acquireReference();
            m_shapeConfig->SetCachedNativeHeightfield(pxHeightfield);

            InitHeightfieldCollider(HeightfieldCollider::DataSource::UseCachedHeightfield);
        }
    }

    void HeightfieldColliderComponent::OnAssetReload(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_bakedHeightfieldAsset)
        {
            m_heightfieldCollider.reset();
            OnAssetReady(asset);
        }
    }

    void HeightfieldColliderComponent::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        InitHeightfieldCollider(HeightfieldCollider::DataSource::GenerateNewHeightfield);
    }

    void HeightfieldColliderComponent::Activate()
    {
        *m_shapeConfig = Utils::CreateBaseHeightfieldShapeConfiguration(GetEntityId());

        AZ::Data::AssetId assetId = m_bakedHeightfieldAsset.GetId();
        AZ::Data::AssetData::AssetStatus assetStatus = m_bakedHeightfieldAsset.GetStatus();

        if (assetId.IsValid() && assetStatus != AZ::Data::AssetData::AssetStatus::Error)
        {
            if (m_bakedHeightfieldAsset.GetStatus() == AZ::Data::AssetData::AssetStatus::NotLoaded)
            {
                m_bakedHeightfieldAsset.QueueLoad();
            }

            AZ::Data::AssetBus::Handler::BusConnect(assetId);
        }
        else
        {
            InitHeightfieldCollider(HeightfieldCollider::DataSource::GenerateNewHeightfield);
        }
    }

    void HeightfieldColliderComponent::Deactivate()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
        Physics::CollisionFilteringRequestBus::Handler::BusDisconnect();
        ColliderComponentRequestBus::Handler::BusDisconnect();

        m_heightfieldCollider.reset();
    }

    void HeightfieldColliderComponent::BlockOnPendingJobs()
    {
        if (m_heightfieldCollider)
        {
            m_heightfieldCollider->BlockOnPendingJobs();
        }
    }

    void HeightfieldColliderComponent::SetColliderConfiguration(const Physics::ColliderConfiguration& colliderConfig)
    {
        if (GetEntity()->GetState() == AZ::Entity::State::Active)
        {
            AZ_Warning(
                "PhysX", false, "Trying to call SetShapeConfiguration for entity \"%s\" while entity is active.",
                GetEntity()->GetName().c_str());
            return;
        }
        *m_colliderConfig = colliderConfig;
    }

    void HeightfieldColliderComponent::SetBakedHeightfieldAsset(const AZ::Data::Asset<Pipeline::HeightFieldAsset>& heightfieldAsset)
    {
        m_bakedHeightfieldAsset = heightfieldAsset;
    }

    // ColliderComponentRequestBus
    AzPhysics::ShapeColliderPairList HeightfieldColliderComponent::GetShapeConfigurations()
    {
        AzPhysics::ShapeColliderPairList shapeConfigurationList({ AzPhysics::ShapeColliderPair(m_colliderConfig, m_shapeConfig) });
        return shapeConfigurationList;
    }

    AZStd::shared_ptr<Physics::Shape> HeightfieldColliderComponent::GetHeightfieldShape()
    {
        return m_heightfieldCollider->GetHeightfieldShape();
    }

    // ColliderComponentRequestBus
    AZStd::vector<AZStd::shared_ptr<Physics::Shape>> HeightfieldColliderComponent::GetShapes()
    {
        return { GetHeightfieldShape() };
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

    void HeightfieldColliderComponent::InitHeightfieldCollider(HeightfieldCollider::DataSource heightfieldDataSource)
    {
        const AZ::EntityId entityId = GetEntityId();

        AzPhysics::SceneHandle sceneHandle = AzPhysics::InvalidSceneHandle;
        Physics::DefaultWorldBus::BroadcastResult(sceneHandle, &Physics::DefaultWorldRequests::GetDefaultSceneHandle);
        
        m_heightfieldCollider = AZStd::make_unique<HeightfieldCollider>(
            entityId, GetEntity()->GetName(), sceneHandle, m_colliderConfig, m_shapeConfig, heightfieldDataSource);

        ColliderComponentRequestBus::Handler::BusConnect(entityId);
        Physics::CollisionFilteringRequestBus::Handler::BusConnect(entityId);
    }

} // namespace PhysX
