/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorTestUtilities.h>
#include <EditorColliderComponent.h>
#include <EditorRigidBodyComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorNonUniformScaleComponent.h>
#include <AZTestShared/Math/MathTestHelpers.h>
#include <RigidBodyComponent.h>
#include <StaticRigidBodyComponent.h>
#include <RigidBodyStatic.h>
#include <BaseColliderComponent.h>
#include <BoxColliderComponent.h>
#include <CapsuleColliderComponent.h>
#include <SphereColliderComponent.h>

namespace PhysXEditorTests
{
    void ExpectSingleConvexRuntimeConfig(AZ::Entity* gameEntity)
    {
        PhysX::BaseColliderComponent* colliderComponent = gameEntity->FindComponent<PhysX::BaseColliderComponent>();
        ASSERT_TRUE(colliderComponent != nullptr);

        AzPhysics::ShapeColliderPairList shapeConfigList = colliderComponent->GetShapeConfigurations();
        EXPECT_EQ(shapeConfigList.size(), 1);

        for (const auto& shapeConfigPair : shapeConfigList)
        {
            EXPECT_EQ(shapeConfigPair.second->GetShapeType(), Physics::ShapeType::CookedMesh);
        }
    }

    TEST_F(PhysXEditorFixture, BoxCollider_NonUniformScaleComponent_RuntimeShapeConfigReplacedWithConvex)
    {
        EntityPtr editorEntity = CreateInactiveEditorEntity("Box");
        editorEntity->CreateComponent<PhysX::EditorColliderComponent>(Physics::ColliderConfiguration(), Physics::BoxShapeConfiguration());
        editorEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        editorEntity->Activate();

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // because there is a non-uniform scale component, the runtime entity should have a BaseColliderComponent with a
        // cooked mesh shape configuration, rather than a BoxColliderComponent
        EXPECT_TRUE(gameEntity->FindComponent<PhysX::BoxColliderComponent>() == nullptr);
        ExpectSingleConvexRuntimeConfig(gameEntity.get());
    }

    TEST_F(PhysXEditorFixture, BoxCollider_NonUniformScale_RuntimePhysicsAabbCorrect)
    {
        EntityPtr editorEntity = CreateInactiveEditorEntity("Box");

        Physics::ColliderConfiguration colliderConfig;
        colliderConfig.m_rotation = AZ::Quaternion::CreateRotationZ(AZ::DegToRad(45.0f));
        colliderConfig.m_position = AZ::Vector3(1.0f, 2.0f, 3.0f);
        Physics::BoxShapeConfiguration boxShapeConfig(AZ::Vector3(0.5f, 0.7f, 0.9f));
        editorEntity->CreateComponent<PhysX::EditorColliderComponent>(colliderConfig, boxShapeConfig);
        editorEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        editorEntity->Activate();

        AZ::EntityId editorId = editorEntity->GetId();
        AZ::Transform worldTM;
        worldTM.SetUniformScale(1.5f);
        worldTM.SetTranslation(AZ::Vector3(5.0f, 6.0f, 7.0f));
        worldTM.SetRotation(AZ::Quaternion::CreateRotationX(AZ::DegToRad(30.0f)));
        AZ::TransformBus::Event(editorId, &AZ::TransformBus::Events::SetWorldTM, worldTM);
        AZ::NonUniformScaleRequestBus::Event(editorId, &AZ::NonUniformScaleRequests::SetScale, AZ::Vector3(0.7f, 0.9f, 1.1f));

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // since there was no editor rigid body component, the runtime entity should have a static rigid body
        const auto* staticBody = azdynamic_cast<PhysX::StaticRigidBody*>(gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>()->GetSimulatedBody());

        const AZ::Aabb aabb = staticBody->GetAabb();
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(5.6045f, 4.9960f, 11.7074f), 1e-3f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(6.4955f, 6.7305f, 13.5662f), 1e-3f));
    }

    TEST_F(PhysXEditorFixture, BoxColliderRigidBody_NonUniformScale_RuntimePhysicsAabbCorrect)
    {
        EntityPtr editorEntity = CreateInactiveEditorEntity("Box");

        Physics::ColliderConfiguration colliderConfig;
        colliderConfig.m_rotation = AZ::Quaternion::CreateRotationZ(AZ::DegToRad(45.0f));
        colliderConfig.m_position = AZ::Vector3(1.0f, 2.0f, 3.0f);
        Physics::BoxShapeConfiguration boxShapeConfig(AZ::Vector3(0.5f, 0.7f, 0.9f));
        editorEntity->CreateComponent<PhysX::EditorColliderComponent>(colliderConfig, boxShapeConfig);
        editorEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        editorEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();
        editorEntity->Activate();

        AZ::EntityId editorId = editorEntity->GetId();
        AZ::Transform worldTM;
        worldTM.SetUniformScale(1.5f);
        worldTM.SetTranslation(AZ::Vector3(5.0f, 6.0f, 7.0f));
        worldTM.SetRotation(AZ::Quaternion::CreateRotationX(AZ::DegToRad(30.0f)));
        AZ::TransformBus::Event(editorId, &AZ::TransformBus::Events::SetWorldTM, worldTM);
        AZ::NonUniformScaleRequestBus::Event(editorId, &AZ::NonUniformScaleRequests::SetScale, AZ::Vector3(0.7f, 0.9f, 1.1f));

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // since there was an editor rigid body component, the runtime entity should have a dynamic rigid body
        const auto* dynamicBody = gameEntity->FindComponent<PhysX::RigidBodyComponent>()->GetRigidBody();

        const AZ::Aabb aabb = dynamicBody->GetAabb();
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(5.6045f, 4.9960f, 11.7074f), 1e-3f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(6.4955f, 6.7305f, 13.5662f), 1e-3f));
    }

    TEST_F(PhysXEditorFixture, CapsuleCollider_NonUniformScaleComponent_RuntimeShapeConfigReplacedWithConvex)
    {
        EntityPtr editorEntity = CreateInactiveEditorEntity("Capsule");
        editorEntity->CreateComponent<PhysX::EditorColliderComponent>(Physics::ColliderConfiguration(), Physics::CapsuleShapeConfiguration());
        editorEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        editorEntity->Activate();

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // because there is a non-uniform scale component, the runtime entity should have a BaseColliderComponent with a
        // cooked mesh shape configuration, rather than a CapsuleColliderComponent
        EXPECT_TRUE(gameEntity->FindComponent<PhysX::CapsuleColliderComponent>() == nullptr);
        ExpectSingleConvexRuntimeConfig(gameEntity.get());
    }

    TEST_F(PhysXEditorFixture, CapsuleCollider_NonUniformScale_RuntimePhysicsAabbCorrect)
    {
        EntityPtr editorEntity = CreateInactiveEditorEntity("Capsule");

        Physics::ColliderConfiguration colliderConfig;
        colliderConfig.m_rotation = AZ::Quaternion::CreateRotationX(AZ::DegToRad(90.0f));
        colliderConfig.m_position = AZ::Vector3(2.0f, 5.0f, 3.0f);
        Physics::CapsuleShapeConfiguration capsuleShapeConfig(1.4f, 0.3f);
        editorEntity->CreateComponent<PhysX::EditorColliderComponent>(colliderConfig, capsuleShapeConfig);
        editorEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        editorEntity->Activate();

        AZ::EntityId capsuleId = editorEntity->GetId();
        AZ::Transform worldTM;
        worldTM.SetUniformScale(0.5f);
        worldTM.SetTranslation(AZ::Vector3(3.0f, 1.0f, -4.0f));
        worldTM.SetRotation(AZ::Quaternion::CreateRotationY(AZ::DegToRad(90.0f)));
        AZ::TransformBus::Event(capsuleId, &AZ::TransformBus::Events::SetWorldTM, worldTM);
        AZ::NonUniformScaleRequestBus::Event(capsuleId, &AZ::NonUniformScaleRequests::SetScale, AZ::Vector3(1.2f, 0.7f, 0.6f));

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // since there was no editor rigid body component, the runtime entity should have a static rigid body
        const auto* staticBody = azdynamic_cast<PhysX::StaticRigidBody*>(gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>()->GetSimulatedBody());

        const AZ::Aabb aabb = staticBody->GetAabb();

        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(3.81f, 2.505f, -5.38f), 1e-3f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(3.99f, 2.995f, -5.02f), 1e-3f));
    }

    TEST_F(PhysXEditorFixture, CapsuleColliderRigidBody_NonUniformScale_RuntimePhysicsAabbCorrect)
    {
        EntityPtr editorEntity = CreateInactiveEditorEntity("Capsule");

        Physics::ColliderConfiguration colliderConfig;
        colliderConfig.m_rotation = AZ::Quaternion::CreateRotationX(AZ::DegToRad(90.0f));
        colliderConfig.m_position = AZ::Vector3(2.0f, 5.0f, 3.0f);
        Physics::CapsuleShapeConfiguration capsuleShapeConfig(1.4f, 0.3f);
        editorEntity->CreateComponent<PhysX::EditorColliderComponent>(colliderConfig, capsuleShapeConfig);
        editorEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        editorEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();
        editorEntity->Activate();

        AZ::EntityId capsuleId = editorEntity->GetId();
        AZ::Transform worldTM;
        worldTM.SetUniformScale(0.5f);
        worldTM.SetTranslation(AZ::Vector3(3.0f, 1.0f, -4.0f));
        worldTM.SetRotation(AZ::Quaternion::CreateRotationY(AZ::DegToRad(90.0f)));
        AZ::TransformBus::Event(capsuleId, &AZ::TransformBus::Events::SetWorldTM, worldTM);
        AZ::NonUniformScaleRequestBus::Event(capsuleId, &AZ::NonUniformScaleRequests::SetScale, AZ::Vector3(1.2f, 0.7f, 0.6f));

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // since there was an editor rigid body component, the runtime entity should have a dynamic rigid body
        const auto* dynamicBody = gameEntity->FindComponent<PhysX::RigidBodyComponent>()->GetRigidBody();

        const AZ::Aabb aabb = dynamicBody->GetAabb();

        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(3.81f, 2.505f, -5.38f), 1e-3f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(3.99f, 2.995f, -5.02f), 1e-3f));
    }

    TEST_F(PhysXEditorFixture, SphereCollider_NonUniformScaleComponent_RuntimeShapeConfigReplacedWithConvex)
    {
        EntityPtr editorEntity = CreateInactiveEditorEntity("Sphere");
        editorEntity->CreateComponent<PhysX::EditorColliderComponent>(Physics::ColliderConfiguration(), Physics::SphereShapeConfiguration());
        editorEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        editorEntity->Activate();

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // because there is a non-uniform scale component, the runtime entity should have a BaseColliderComponent with a
        // cooked mesh shape configuration, rather than a CapsuleColliderComponent
        EXPECT_TRUE(gameEntity->FindComponent<PhysX::SphereColliderComponent>() == nullptr);
        ExpectSingleConvexRuntimeConfig(gameEntity.get());
    }

    TEST_F(PhysXEditorFixture, SphereCollider_NonUniformScale_RuntimePhysicsAabbCorrect)
    {
        EntityPtr editorEntity = CreateInactiveEditorEntity("Sphere");

        Physics::ColliderConfiguration colliderConfig;
        colliderConfig.m_rotation = AZ::Quaternion::CreateRotationY(AZ::DegToRad(90.0f));
        colliderConfig.m_position = AZ::Vector3(3.0f, -2.0f, 2.0f);
        Physics::SphereShapeConfiguration sphereShapeConfig(0.7f);
        editorEntity->CreateComponent<PhysX::EditorColliderComponent>(colliderConfig, sphereShapeConfig);
        editorEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        editorEntity->Activate();

        AZ::EntityId sphereId = editorEntity->GetId();
        AZ::Transform worldTM;
        worldTM.SetUniformScale(1.2f);
        worldTM.SetTranslation(AZ::Vector3(-2.0f, -1.0f, 3.0f));
        worldTM.SetRotation(AZ::Quaternion::CreateRotationX(AZ::DegToRad(90.0f)));
        AZ::TransformBus::Event(sphereId, &AZ::TransformBus::Events::SetWorldTM, worldTM);
        AZ::NonUniformScaleRequestBus::Event(sphereId, &AZ::NonUniformScaleRequests::SetScale, AZ::Vector3(0.8f, 0.9f, 0.6f));

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // since there was no editor rigid body component, the runtime entity should have a static rigid body
        const auto* staticBody = azdynamic_cast<PhysX::StaticRigidBody*>(gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>()->GetSimulatedBody());

        const AZ::Aabb aabb = staticBody->GetAabb();

        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(0.208f, -2.944f, 0.084f), 1e-3f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(1.552f, -1.936f, 1.596f), 1e-3f));
    }

    TEST_F(PhysXEditorFixture, SphereColliderRigidBody_NonUniformScale_RuntimePhysicsAabbCorrect)
    {
        EntityPtr editorEntity = CreateInactiveEditorEntity("Sphere");

        Physics::ColliderConfiguration colliderConfig;
        colliderConfig.m_rotation = AZ::Quaternion::CreateRotationY(AZ::DegToRad(90.0f));
        colliderConfig.m_position = AZ::Vector3(3.0f, -2.0f, 2.0f);
        Physics::SphereShapeConfiguration sphereShapeConfig(0.7f);
        editorEntity->CreateComponent<PhysX::EditorColliderComponent>(colliderConfig, sphereShapeConfig);
        editorEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        editorEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();
        editorEntity->Activate();

        AZ::EntityId sphereId = editorEntity->GetId();
        AZ::Transform worldTM;
        worldTM.SetUniformScale(1.2f);
        worldTM.SetTranslation(AZ::Vector3(-2.0f, -1.0f, 3.0f));
        worldTM.SetRotation(AZ::Quaternion::CreateRotationX(AZ::DegToRad(90.0f)));
        AZ::TransformBus::Event(sphereId, &AZ::TransformBus::Events::SetWorldTM, worldTM);
        AZ::NonUniformScaleRequestBus::Event(sphereId, &AZ::NonUniformScaleRequests::SetScale, AZ::Vector3(0.8f, 0.9f, 0.6f));

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // since there was an editor rigid body component, the runtime entity should have a dynamic rigid body
        const auto* dynamicBody = gameEntity->FindComponent<PhysX::RigidBodyComponent>()->GetRigidBody();

        const AZ::Aabb aabb = dynamicBody->GetAabb();

        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(0.208f, -2.944f, 0.084f), 1e-3f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(1.552f, -1.936f, 1.596f), 1e-3f));
    }
} // namespace PhysXEditorTests
