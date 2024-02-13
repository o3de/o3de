/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/PhysXGenericTestFixture.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <AzFramework/Physics/Configuration/RigidBodyConfiguration.h>
#include <AzFramework/Physics/Configuration/SceneConfiguration.h>
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>

#include <Tests/PhysXTestCommon.h>

#include <BoxColliderComponent.h>
#include <CapsuleColliderComponent.h>
#include <RigidBodyComponent.h>
#include <SphereColliderComponent.h>
#include <StaticRigidBodyComponent.h>

namespace PhysX
{
    // helper functions
    AzPhysics::SceneHandle GenericPhysicsFixture::CreateTestScene()
    {
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            AzPhysics::SceneConfiguration sceneConfiguration = physicsSystem->GetDefaultSceneConfiguration();
            sceneConfiguration.m_sceneName = "TestScene";
            sceneConfiguration.m_gravity = AZ::Vector3(0.0f, 0.0f, -10.0f);
            m_testSceneHandle = physicsSystem->AddScene(sceneConfiguration);
            return m_testSceneHandle;
        }
        return AzPhysics::InvalidSceneHandle;
    }

    void GenericPhysicsFixture::DestroyTestScene()
    {
        //clean up the test scene
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            physicsSystem->RemoveScene(m_testSceneHandle);
        }
    }

    void GenericPhysicsFixture::SetUpInternal()
    {
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            AzPhysics::SceneConfiguration sceneConfiguration = physicsSystem->GetDefaultSceneConfiguration();
            sceneConfiguration.m_sceneName = AzPhysics::DefaultPhysicsSceneName;
            m_testSceneHandle = physicsSystem->AddScene(sceneConfiguration);
            m_defaultScene = physicsSystem->GetScene(m_testSceneHandle);
        }

        Physics::DefaultWorldBus::Handler::BusConnect();
    }

    void GenericPhysicsFixture::TearDownInternal()
    {
        Physics::DefaultWorldBus::Handler::BusDisconnect();
        m_defaultScene = nullptr;
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            physicsSystem->RemoveScene(m_testSceneHandle);
        }
        m_testSceneHandle = AzPhysics::InvalidSceneHandle;
        PhysX::TestUtils::ResetPhysXSystem();
    }

    AzPhysics::SceneHandle GenericPhysicsFixture::GetDefaultSceneHandle() const
    {
        return m_testSceneHandle;
    }

    AZStd::unique_ptr<AZ::Entity> GenericPhysicsFixture::AddMultiShapeEntity(const MultiShapeConfig& config)
    {
        AZStd::unique_ptr<AZ::Entity> entity(aznew AZ::Entity("TestShapeEntity"));
        entity->CreateComponent(AZ::Uuid::CreateString("{22B10178-39B6-4C12-BB37-77DB45FDD3B6}")); // TransformComponent
        entity->Init();

        entity->Activate();

        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, config.m_position);
        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetLocalRotation, config.m_rotation);

        entity->Deactivate();

        auto colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
        colliderConfig->m_collisionLayer = config.m_layer;

        AzPhysics::ShapeColliderPairList shapeconfigurationList;

        struct Visitor
        {
            AZStd::shared_ptr<Physics::ColliderConfiguration>& colliderConfig;
            AzPhysics::ShapeColliderPairList& shapeconfigurationList;

            void operator()(const MultiShapeConfig::ShapeList::ShapeData::Box& box) const
            {
                shapeconfigurationList.push_back
                ({
                    colliderConfig,
                    AZStd::make_shared<Physics::BoxShapeConfiguration>(box.m_extent)
                    });
            }
            void operator()(const MultiShapeConfig::ShapeList::ShapeData::Sphere& sphere) const
            {
                shapeconfigurationList.push_back
                ({
                    colliderConfig,
                    AZStd::make_shared<Physics::SphereShapeConfiguration>(sphere.m_radius)
                    });
            }
            void operator()(const MultiShapeConfig::ShapeList::ShapeData::Capsule& capsule) const
            {
                shapeconfigurationList.push_back
                ({
                    colliderConfig,
                    AZStd::make_shared<Physics::CapsuleShapeConfiguration>(capsule.m_height, capsule.m_radius)
                    });
            }
            void operator()(const AZStd::monostate&) const
            {
                AZ_Assert(false, "Invalid shape type");
            }
        };
        Visitor addShapeConfig = { colliderConfig, shapeconfigurationList };

        for (const MultiShapeConfig::ShapeList::ShapeData& shapeConfig : config.m_shapes.m_shapesData)
        {
            AZStd::visit(addShapeConfig, shapeConfig.m_data);
        }

        auto colliderComponent = entity->CreateComponent<BaseColliderComponent>();
        colliderComponent->SetShapeConfigurationList(shapeconfigurationList);

        AzPhysics::RigidBodyConfiguration rigidBodyConfig;
        entity->CreateComponent<RigidBodyComponent>(rigidBodyConfig, m_testSceneHandle);

        entity->Activate();

        for (int i = 0; i < colliderComponent->GetShapes().size(); ++i)
        {
            colliderComponent->GetShapes()[i]->SetLocalPose(config.m_shapes.m_shapesData[i].m_offset, AZ::Quaternion::CreateIdentity());
        }

        return AZStd::unique_ptr<AZ::Entity>{ AZStd::move(entity) };
    }
} // namespace Physics
