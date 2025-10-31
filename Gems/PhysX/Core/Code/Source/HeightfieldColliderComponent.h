/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

#include <AzFramework/Physics/CollisionBus.h>
#include <AzFramework/Physics/Components/SimulatedBodyComponentBus.h>
#include <AzFramework/Physics/HeightfieldProviderBus.h>

#include <PhysX/ColliderComponentBus.h>
#include <PhysX/ColliderShapeBus.h>
#include <PhysX/HeightFieldAsset.h>
#include <AzCore/Asset/AssetCommon.h>

#include <Source/HeightfieldCollider.h>

namespace AzPhysics
{
    struct SimulatedBody;
}

namespace PhysX
{
    class StaticRigidBody;

    //! Component that provides a Heightfield Collider.
    //! It covers the static rigid body functionality as well, but it can be refactored out
    //! once EditorStaticRigidBodyComponent handles the creation of the simulated body.
    //! 
    //! The heightfield collider is a bit different from the other shape colliders in that it gets the heightfield data from a
    //! HeightfieldProvider, which can control position, rotation, size, and even change its data at runtime.
    //! Due to these differences, this component directly implements the collider instead of using BaseColliderComponent.
    class HeightfieldColliderComponent
        : public AZ::Component
        , public ColliderComponentRequestBus::Handler
        , protected Physics::CollisionFilteringRequestBus::Handler
        , private AZ::Data::AssetBus::Handler
    {
    public:
        using Configuration = Physics::HeightfieldShapeConfiguration;
        AZ_COMPONENT(HeightfieldColliderComponent, "{9A42672C-281A-4CE8-BFDD-EAA1E0FCED76}");
        static void Reflect(AZ::ReflectContext* context);

        HeightfieldColliderComponent() = default;
        ~HeightfieldColliderComponent() override;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        void Activate() override;

        void InitHeightfieldCollider(HeightfieldCollider::DataSource heightfieldDataSource);

        void Deactivate() override;

        void SetColliderConfiguration(const Physics::ColliderConfiguration& colliderConfig);
        void SetBakedHeightfieldAsset(const AZ::Data::Asset<Pipeline::HeightFieldAsset>& heightfieldAsset);

        void BlockOnPendingJobs();

    protected:
        // ColliderComponentRequestBus
        AzPhysics::ShapeColliderPairList GetShapeConfigurations() override;
        AZStd::vector<AZStd::shared_ptr<Physics::Shape>> GetShapes() override;

        // CollisionFilteringRequestBus
        void SetCollisionLayer(const AZStd::string& layerName, AZ::Crc32 filterTag) override;
        AZStd::string GetCollisionLayerName() override;
        void SetCollisionGroup(const AZStd::string& groupName, AZ::Crc32 filterTag) override;
        AZStd::string GetCollisionGroupName() override;
        void ToggleCollisionLayer(const AZStd::string& layerName, AZ::Crc32 filterTag, bool enabled) override;

        // AZ::Data::AssetBus::Handler overrides ...
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset);
        void OnAssetReload(AZ::Data::Asset<AZ::Data::AssetData> asset);
        void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset);

    private:
        AZStd::shared_ptr<Physics::Shape> GetHeightfieldShape();

        //! Stores collision layers, whether the collider is a trigger, etc.
        AZStd::shared_ptr<Physics::ColliderConfiguration> m_colliderConfig{ aznew Physics::ColliderConfiguration() };
        //! Stores all of the cached information for the heightfield shape.
        AZStd::shared_ptr<Physics::HeightfieldShapeConfiguration> m_shapeConfig{ aznew Physics::HeightfieldShapeConfiguration() };
        //! Contains all of the runtime logic for creating / updating / destroying the heightfield collider.
        AZStd::unique_ptr<HeightfieldCollider> m_heightfieldCollider;
        AZ::Data::Asset<Pipeline::HeightFieldAsset> m_bakedHeightfieldAsset;

    };
} // namespace PhysX
