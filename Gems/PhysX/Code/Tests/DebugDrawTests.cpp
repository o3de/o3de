/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/UnitTest/TestDebugDisplayRequests.h>
#include <EditorTestUtilities.h>
#include <EditorColliderComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorNonUniformScaleComponent.h>
#include <AZTestShared/Math/MathTestHelpers.h>

namespace PhysXEditorTests
{
    TEST_F(PhysXEditorFixture, BoxCollider_NonUniformScale_DebugDrawCorrect)
    {
        EntityPtr boxEntity = CreateInactiveEditorEntity("Box");

        Physics::ColliderConfiguration colliderConfig;
        colliderConfig.m_rotation = AZ::Quaternion::CreateRotationZ(AZ::DegToRad(45.0f));
        colliderConfig.m_position = AZ::Vector3(1.0f, 2.0f, 3.0f);
        Physics::BoxShapeConfiguration boxShapeConfig(AZ::Vector3(0.5f, 0.7f, 0.9f));
        boxEntity->CreateComponent<PhysX::EditorColliderComponent>(colliderConfig, boxShapeConfig);
        boxEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        boxEntity->Activate();

        AZ::EntityId boxId = boxEntity->GetId();
        AZ::Transform worldTM;
        worldTM.SetUniformScale(1.5f);
        worldTM.SetTranslation(AZ::Vector3(5.0f, 6.0f, 7.0f));
        worldTM.SetRotation(AZ::Quaternion::CreateRotationX(AZ::DegToRad(30.0f)));
        AZ::TransformBus::Event(boxId, &AZ::TransformBus::Events::SetWorldTM, worldTM);
        AZ::NonUniformScaleRequestBus::Event(boxId, &AZ::NonUniformScaleRequests::SetScale, AZ::Vector3(0.7f, 0.9f, 1.1f));

        UnitTest::TestDebugDisplayRequests testDebugDisplayRequests;
        AzFramework::EntityDebugDisplayEventBus::Event(boxId, &AzFramework::EntityDebugDisplayEvents::DisplayEntityViewport,
            AzFramework::ViewportInfo{ 0 }, testDebugDisplayRequests);
        const AZ::Aabb debugDrawAabb = testDebugDisplayRequests.GetAabb();

        EXPECT_THAT(debugDrawAabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(5.6045f, 4.9960f, 11.7074f), 1e-3f));
        EXPECT_THAT(debugDrawAabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(6.4955f, 6.7305f, 13.5662f), 1e-3f));
    }

    TEST_F(PhysXEditorFixture, BoxCollider_NonUniformScale_DebugDrawAlignsWithAabb)
    {
        EntityPtr boxEntity = CreateInactiveEditorEntity("Box");

        Physics::ColliderConfiguration colliderConfig;
        colliderConfig.m_rotation = AZ::Quaternion::CreateRotationY(AZ::DegToRad(30.0f));
        colliderConfig.m_position = AZ::Vector3(3.0f, -1.0f, 2.0f);
        Physics::BoxShapeConfiguration boxShapeConfig(AZ::Vector3(1.2f, 0.4f, 1.3f));
        boxEntity->CreateComponent<PhysX::EditorColliderComponent>(colliderConfig, boxShapeConfig);
        boxEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        boxEntity->Activate();

        AZ::EntityId boxId = boxEntity->GetId();
        AZ::Transform worldTM;
        worldTM.SetUniformScale(1.2f);
        worldTM.SetTranslation(AZ::Vector3(4.0f, -3.0f, 1.0f));
        worldTM.SetRotation(AZ::Quaternion::CreateRotationZ(AZ::DegToRad(45.0f)));
        AZ::TransformBus::Event(boxId, &AZ::TransformBus::Events::SetWorldTM, worldTM);
        AZ::NonUniformScaleRequestBus::Event(boxId, &AZ::NonUniformScaleRequests::SetScale, AZ::Vector3(1.1f, 0.6f, 1.3f));

        UnitTest::TestDebugDisplayRequests testDebugDisplayRequests;
        AzFramework::EntityDebugDisplayEventBus::Event(boxId, &AzFramework::EntityDebugDisplayEvents::DisplayEntityViewport,
            AzFramework::ViewportInfo{ 0 }, testDebugDisplayRequests);
        const AZ::Aabb debugDrawAabb = testDebugDisplayRequests.GetAabb();

        AZ::Aabb colliderAabb = AZ::Aabb::CreateNull();
        PhysX::ColliderShapeRequestBus::EventResult(colliderAabb, boxId, &PhysX::ColliderShapeRequests::GetColliderShapeAabb);

        EXPECT_TRUE(debugDrawAabb.IsClose(colliderAabb, 1e-3f));
    }

    TEST_F(PhysXEditorFixture, BoxCollider_UnitNonUniformScale_DebugDrawIdenticalToNoNonUniformScale)
    {
        EntityPtr boxEntity = CreateInactiveEditorEntity("Box");

        Physics::ColliderConfiguration colliderConfig;
        colliderConfig.m_rotation = AZ::Quaternion::CreateRotationZ(AZ::DegToRad(45.0f));
        colliderConfig.m_position = AZ::Vector3(1.0f, -2.0f, 1.0f);
        Physics::BoxShapeConfiguration boxShapeConfig(AZ::Vector3(0.8f, 0.7f, 1.6f));
        boxEntity->CreateComponent<PhysX::EditorColliderComponent>(colliderConfig, boxShapeConfig);
        boxEntity->Activate();

        AZ::EntityId boxId = boxEntity->GetId();
        AZ::Transform worldTM;
        worldTM.SetUniformScale(1.2f);
        worldTM.SetTranslation(AZ::Vector3(4.0f, -3.0f, 1.0f));
        worldTM.SetRotation(AZ::Quaternion::CreateRotationZ(AZ::DegToRad(45.0f)));
        AZ::TransformBus::Event(boxId, &AZ::TransformBus::Events::SetWorldTM, worldTM);

        UnitTest::TestDebugDisplayRequests testDebugDisplayRequests;
        AzFramework::EntityDebugDisplayEventBus::Event(boxId, &AzFramework::EntityDebugDisplayEvents::DisplayEntityViewport,
            AzFramework::ViewportInfo{ 0 }, testDebugDisplayRequests);
        const AZ::Aabb debugDrawAabbNoNonUniformScale = testDebugDisplayRequests.GetAabb();

        // now add a non-uniform scale component but with scale (1, 1, 1)
        boxEntity->Deactivate();
        boxEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        boxEntity->Activate();

        // the Aabb for the debug draw points should not have changed
        testDebugDisplayRequests.ClearPoints();
        AzFramework::EntityDebugDisplayEventBus::Event(boxId, &AzFramework::EntityDebugDisplayEvents::DisplayEntityViewport,
            AzFramework::ViewportInfo{ 0 }, testDebugDisplayRequests);
        const AZ::Aabb debugDrawAabbUnitNonUniformScale = testDebugDisplayRequests.GetAabb();

        EXPECT_TRUE(debugDrawAabbUnitNonUniformScale.IsClose(debugDrawAabbNoNonUniformScale, 1e-3f));
    }

    TEST_F(PhysXEditorFixture, CapsuleCollider_NonUniformScale_DebugDrawCorrect)
    {
        EntityPtr capsuleEntity = CreateInactiveEditorEntity("Capsule");

        Physics::ColliderConfiguration colliderConfig;
        colliderConfig.m_rotation = AZ::Quaternion::CreateRotationX(AZ::DegToRad(90.0f));
        colliderConfig.m_position = AZ::Vector3(2.0f, 5.0f, 3.0f);
        Physics::CapsuleShapeConfiguration capsuleShapeConfig(1.4f, 0.3f);
        capsuleEntity->CreateComponent<PhysX::EditorColliderComponent>(colliderConfig, capsuleShapeConfig);
        capsuleEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        capsuleEntity->Activate();

        AZ::EntityId capsuleId = capsuleEntity->GetId();
        AZ::Transform worldTM;
        worldTM.SetUniformScale(0.5f);
        worldTM.SetTranslation(AZ::Vector3(3.0f, 1.0f, -4.0f));
        worldTM.SetRotation(AZ::Quaternion::CreateRotationY(AZ::DegToRad(90.0f)));
        AZ::TransformBus::Event(capsuleId, &AZ::TransformBus::Events::SetWorldTM, worldTM);
        AZ::NonUniformScaleRequestBus::Event(capsuleId, &AZ::NonUniformScaleRequests::SetScale, AZ::Vector3(1.2f, 0.7f, 0.6f));

        UnitTest::TestDebugDisplayRequests testDebugDisplayRequests;
        AzFramework::EntityDebugDisplayEventBus::Event(capsuleId, &AzFramework::EntityDebugDisplayEvents::DisplayEntityViewport,
            AzFramework::ViewportInfo{ 0 }, testDebugDisplayRequests);
        const AZ::Aabb debugDrawAabb = testDebugDisplayRequests.GetAabb();

        EXPECT_THAT(debugDrawAabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(3.81f, 2.505f, -5.38f), 1e-3f));
        EXPECT_THAT(debugDrawAabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(3.99f, 2.995f, -5.02f), 1e-3f));
    }

    TEST_F(PhysXEditorFixture, CapsuleCollider_NonUniformScale_DebugDrawAlignsWithAabb)
    {
        EntityPtr capsuleEntity = CreateInactiveEditorEntity("Capsule");

        Physics::ColliderConfiguration colliderConfig;
        colliderConfig.m_rotation = AZ::Quaternion::CreateRotationZ(AZ::DegToRad(60.0f));
        colliderConfig.m_position = AZ::Vector3(2.0f, -2.0f, 3.0f);
        Physics::CapsuleShapeConfiguration capsuleShapeConfig(1.2f, 0.2f);
        capsuleEntity->CreateComponent<PhysX::EditorColliderComponent>(colliderConfig, capsuleShapeConfig);
        capsuleEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        capsuleEntity->Activate();

        AZ::EntityId capsuleId = capsuleEntity->GetId();
        AZ::Transform worldTM;
        worldTM.SetUniformScale(1.4f);
        worldTM.SetTranslation(AZ::Vector3(1.0f, -4.0f, 4.0f));
        worldTM.SetRotation(AZ::Quaternion::CreateRotationX(AZ::DegToRad(45.0f)));
        AZ::TransformBus::Event(capsuleId, &AZ::TransformBus::Events::SetWorldTM, worldTM);
        AZ::NonUniformScaleRequestBus::Event(capsuleId, &AZ::NonUniformScaleRequests::SetScale, AZ::Vector3(0.8f, 0.9f, 0.4f));

        UnitTest::TestDebugDisplayRequests testDebugDisplayRequests;
        AzFramework::EntityDebugDisplayEventBus::Event(capsuleId, &AzFramework::EntityDebugDisplayEvents::DisplayEntityViewport,
            AzFramework::ViewportInfo{ 0 }, testDebugDisplayRequests);
        const AZ::Aabb debugDrawAabb = testDebugDisplayRequests.GetAabb();

        AZ::Aabb colliderAabb = AZ::Aabb::CreateNull();
        PhysX::ColliderShapeRequestBus::EventResult(colliderAabb, capsuleId, &PhysX::ColliderShapeRequests::GetColliderShapeAabb);

        EXPECT_TRUE(debugDrawAabb.IsClose(colliderAabb, 1e-3f));
    }

    TEST_F(PhysXEditorFixture, SphereCollider_NonUniformScale_DebugDrawCorrect)
    {
        EntityPtr sphereEntity = CreateInactiveEditorEntity("Sphere");

        Physics::ColliderConfiguration colliderConfig;
        colliderConfig.m_rotation = AZ::Quaternion::CreateRotationY(AZ::DegToRad(90.0f));
        colliderConfig.m_position = AZ::Vector3(3.0f, -2.0f, 2.0f);
        Physics::SphereShapeConfiguration sphereShapeConfig(0.7f);
        sphereEntity->CreateComponent<PhysX::EditorColliderComponent>(colliderConfig, sphereShapeConfig);
        sphereEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        sphereEntity->Activate();

        AZ::EntityId sphereId = sphereEntity->GetId();
        AZ::Transform worldTM;
        worldTM.SetUniformScale(1.2f);
        worldTM.SetTranslation(AZ::Vector3(-2.0f, -1.0f, 3.0f));
        worldTM.SetRotation(AZ::Quaternion::CreateRotationX(AZ::DegToRad(90.0f)));
        AZ::TransformBus::Event(sphereId, &AZ::TransformBus::Events::SetWorldTM, worldTM);
        AZ::NonUniformScaleRequestBus::Event(sphereId, &AZ::NonUniformScaleRequests::SetScale, AZ::Vector3(0.8f, 0.9f, 0.6f));

        UnitTest::TestDebugDisplayRequests testDebugDisplayRequests;
        AzFramework::EntityDebugDisplayEventBus::Event(sphereId, &AzFramework::EntityDebugDisplayEvents::DisplayEntityViewport,
            AzFramework::ViewportInfo{ 0 }, testDebugDisplayRequests);
        const AZ::Aabb debugDrawAabb = testDebugDisplayRequests.GetAabb();

        EXPECT_THAT(debugDrawAabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(0.208f, -2.944f, 0.084f), 1e-3f));
        EXPECT_THAT(debugDrawAabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(1.552f, -1.936f, 1.596f), 1e-3f));
    }

    TEST_F(PhysXEditorFixture, SphereCollider_NonUniformScale_DebugDrawAlignsWithAabb)
    {
        EntityPtr sphereEntity = CreateInactiveEditorEntity("Sphere");

        Physics::ColliderConfiguration colliderConfig;
        colliderConfig.m_rotation = AZ::Quaternion::CreateRotationX(AZ::DegToRad(-30.0f));
        colliderConfig.m_position = AZ::Vector3(-1.0f, -2.0f, 1.0f);
        Physics::SphereShapeConfiguration sphereShapeConfig(0.4f);
        sphereEntity->CreateComponent<PhysX::EditorColliderComponent>(colliderConfig, sphereShapeConfig);
        sphereEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        sphereEntity->Activate();

        AZ::EntityId sphereId = sphereEntity->GetId();
        AZ::Transform worldTM;
        worldTM.SetUniformScale(0.8f);
        worldTM.SetTranslation(AZ::Vector3(2.0f, -1.0f, 3.0f));
        worldTM.SetRotation(AZ::Quaternion::CreateRotationY(AZ::DegToRad(45.0f)));
        AZ::TransformBus::Event(sphereId, &AZ::TransformBus::Events::SetWorldTM, worldTM);
        AZ::NonUniformScaleRequestBus::Event(sphereId, &AZ::NonUniformScaleRequests::SetScale, AZ::Vector3(0.6f, 1.3f, 0.8f));

        UnitTest::TestDebugDisplayRequests testDebugDisplayRequests;
        AzFramework::EntityDebugDisplayEventBus::Event(sphereId, &AzFramework::EntityDebugDisplayEvents::DisplayEntityViewport,
            AzFramework::ViewportInfo{ 0 }, testDebugDisplayRequests);
        const AZ::Aabb debugDrawAabb = testDebugDisplayRequests.GetAabb();

        AZ::Aabb colliderAabb = AZ::Aabb::CreateNull();
        PhysX::ColliderShapeRequestBus::EventResult(colliderAabb, sphereId, &PhysX::ColliderShapeRequests::GetColliderShapeAabb);

        EXPECT_TRUE(debugDrawAabb.IsClose(colliderAabb, 1e-3f));
    }
} // namespace PhysXEditorTests
