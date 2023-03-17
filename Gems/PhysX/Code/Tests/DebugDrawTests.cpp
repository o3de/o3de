/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/IntersectSegment.h>
#include <AzFramework/UnitTest/TestDebugDisplayRequests.h>
#include <EditorTestUtilities.h>
#include <EditorColliderComponent.h>
#include <EditorShapeColliderComponent.h>
#include <EditorStaticRigidBodyComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorNonUniformScaleComponent.h>
#include <AZTestShared/Math/MathTestHelpers.h>
#include <AZTestShared/Utils/Utils.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <LmbrCentral/Shape/EditorShapeComponentBus.h>

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
        boxEntity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();
        boxEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        boxEntity->Activate();

        AZ::EntityId boxId = boxEntity->GetId();
        AZ::Transform worldTM;
        worldTM.SetUniformScale(1.5f);
        worldTM.SetTranslation(AZ::Vector3(5.0f, 6.0f, 7.0f));
        worldTM.SetRotation(AZ::Quaternion::CreateRotationX(AZ::DegToRad(30.0f)));
        AZ::TransformBus::Event(boxId, &AZ::TransformBus::Events::SetWorldTM, worldTM);
        AZ::NonUniformScaleRequestBus::Event(boxId, &AZ::NonUniformScaleRequests::SetScale, AZ::Vector3(0.7f, 0.9f, 1.1f));

        const AZ::Aabb debugDrawAabb = GetDebugDrawAabb(boxId);

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
        boxEntity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();
        boxEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        boxEntity->Activate();

        AZ::EntityId boxId = boxEntity->GetId();
        AZ::Transform worldTM;
        worldTM.SetUniformScale(1.2f);
        worldTM.SetTranslation(AZ::Vector3(4.0f, -3.0f, 1.0f));
        worldTM.SetRotation(AZ::Quaternion::CreateRotationZ(AZ::DegToRad(45.0f)));
        AZ::TransformBus::Event(boxId, &AZ::TransformBus::Events::SetWorldTM, worldTM);
        AZ::NonUniformScaleRequestBus::Event(boxId, &AZ::NonUniformScaleRequests::SetScale, AZ::Vector3(1.1f, 0.6f, 1.3f));

        const AZ::Aabb debugDrawAabb = GetDebugDrawAabb(boxId);

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
        boxEntity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();
        boxEntity->Activate();

        AZ::EntityId boxId = boxEntity->GetId();
        AZ::Transform worldTM;
        worldTM.SetUniformScale(1.2f);
        worldTM.SetTranslation(AZ::Vector3(4.0f, -3.0f, 1.0f));
        worldTM.SetRotation(AZ::Quaternion::CreateRotationZ(AZ::DegToRad(45.0f)));
        AZ::TransformBus::Event(boxId, &AZ::TransformBus::Events::SetWorldTM, worldTM);

        const AZ::Aabb debugDrawAabbNoNonUniformScale = GetDebugDrawAabb(boxId);

        // now add a non-uniform scale component but with scale (1, 1, 1)
        boxEntity->Deactivate();
        boxEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        boxEntity->Activate();

        // the Aabb for the debug draw points should not have changed
        const AZ::Aabb debugDrawAabbUnitNonUniformScale = GetDebugDrawAabb(boxId);

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
        capsuleEntity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();
        capsuleEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        capsuleEntity->Activate();

        AZ::EntityId capsuleId = capsuleEntity->GetId();
        AZ::Transform worldTM;
        worldTM.SetUniformScale(0.5f);
        worldTM.SetTranslation(AZ::Vector3(3.0f, 1.0f, -4.0f));
        worldTM.SetRotation(AZ::Quaternion::CreateRotationY(AZ::DegToRad(90.0f)));
        AZ::TransformBus::Event(capsuleId, &AZ::TransformBus::Events::SetWorldTM, worldTM);
        AZ::NonUniformScaleRequestBus::Event(capsuleId, &AZ::NonUniformScaleRequests::SetScale, AZ::Vector3(1.2f, 0.7f, 0.6f));

        const AZ::Aabb debugDrawAabb = GetDebugDrawAabb(capsuleId);

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
        capsuleEntity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();
        capsuleEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        capsuleEntity->Activate();

        AZ::EntityId capsuleId = capsuleEntity->GetId();
        AZ::Transform worldTM;
        worldTM.SetUniformScale(1.4f);
        worldTM.SetTranslation(AZ::Vector3(1.0f, -4.0f, 4.0f));
        worldTM.SetRotation(AZ::Quaternion::CreateRotationX(AZ::DegToRad(45.0f)));
        AZ::TransformBus::Event(capsuleId, &AZ::TransformBus::Events::SetWorldTM, worldTM);
        AZ::NonUniformScaleRequestBus::Event(capsuleId, &AZ::NonUniformScaleRequests::SetScale, AZ::Vector3(0.8f, 0.9f, 0.4f));

        const AZ::Aabb debugDrawAabb = GetDebugDrawAabb(capsuleId);

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
        sphereEntity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();
        sphereEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        sphereEntity->Activate();

        AZ::EntityId sphereId = sphereEntity->GetId();
        AZ::Transform worldTM;
        worldTM.SetUniformScale(1.2f);
        worldTM.SetTranslation(AZ::Vector3(-2.0f, -1.0f, 3.0f));
        worldTM.SetRotation(AZ::Quaternion::CreateRotationX(AZ::DegToRad(90.0f)));
        AZ::TransformBus::Event(sphereId, &AZ::TransformBus::Events::SetWorldTM, worldTM);
        AZ::NonUniformScaleRequestBus::Event(sphereId, &AZ::NonUniformScaleRequests::SetScale, AZ::Vector3(0.8f, 0.9f, 0.6f));

        const AZ::Aabb debugDrawAabb = GetDebugDrawAabb(sphereId);

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
        sphereEntity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();
        sphereEntity->CreateComponent<AzToolsFramework::Components::EditorNonUniformScaleComponent>();
        sphereEntity->Activate();

        AZ::EntityId sphereId = sphereEntity->GetId();
        AZ::Transform worldTM;
        worldTM.SetUniformScale(0.8f);
        worldTM.SetTranslation(AZ::Vector3(2.0f, -1.0f, 3.0f));
        worldTM.SetRotation(AZ::Quaternion::CreateRotationY(AZ::DegToRad(45.0f)));
        AZ::TransformBus::Event(sphereId, &AZ::TransformBus::Events::SetWorldTM, worldTM);
        AZ::NonUniformScaleRequestBus::Event(sphereId, &AZ::NonUniformScaleRequests::SetScale, AZ::Vector3(0.6f, 1.3f, 0.8f));

        const AZ::Aabb debugDrawAabb = GetDebugDrawAabb(sphereId);

        AZ::Aabb colliderAabb = AZ::Aabb::CreateNull();
        PhysX::ColliderShapeRequestBus::EventResult(colliderAabb, sphereId, &PhysX::ColliderShapeRequests::GetColliderShapeAabb);

        EXPECT_TRUE(debugDrawAabb.IsClose(colliderAabb, 1e-3f));
    }

    TEST_F(PhysXEditorFixture, ShapeCollider_BoxWithTranslationOffset_DebugDrawCorrect)
    {
        const AZ::Vector3 boxDimensions(3.0f, 4.0f, 5.0f);
        const AZ::Transform transform(AZ::Vector3(1.0f, 2.0f, 3.0f), AZ::Quaternion(0.4f, -0.2f, -0.4f, 0.8f), 0.7f);
        const AZ::Vector3 translationOffset(2.0f, -5.0f, 3.0f);
        const AZ::Vector3 nonUniformScale(1.0f, 1.5f, 2.0f);

        EntityPtr boxShapeEntity = CreateBoxShapeColliderEditorEntity(boxDimensions, transform, translationOffset, nonUniformScale);

        // turn off the shape visibility, so that only the shape collider component debug draws
        LmbrCentral::EditorShapeComponentRequestsBus::Event(
            boxShapeEntity->GetId(), &LmbrCentral::EditorShapeComponentRequests::SetVisibleInEditor, false);

        const AZ::Aabb debugDrawAabb = GetDebugDrawAabb(boxShapeEntity->GetId());

        EXPECT_THAT(debugDrawAabb.GetMin(), UnitTest::IsClose(AZ::Vector3(-7.246f, -6.302f, -2.46f)));
        EXPECT_THAT(debugDrawAabb.GetMax(), UnitTest::IsClose(AZ::Vector3(0.51f, 0.25f, 5.1f)));
    }

    TEST_F(PhysXEditorFixture, ShapeCollider_BoxWithTranslationOffset_SamplePointsCorrect)
    {
        const AZ::Vector3 boxDimensions(6.0f, 2.0f, 7.0f);
        const AZ::Transform transform(AZ::Vector3(4.0f, 7.0f, -2.0f), AZ::Quaternion(0.5f, -0.1f, -0.7f, 0.5f), 1.5f);
        const AZ::Vector3 translationOffset(4.0f, 1.0f, 6.0f);
        const AZ::Vector3 nonUniformScale(2.0f, 1.0f, 1.5f);

        EntityPtr boxShapeEntity = CreateBoxShapeColliderEditorEntity(boxDimensions, transform, translationOffset, nonUniformScale);

        const AZStd::vector<AZ::Vector3> samplePoints =
            boxShapeEntity->FindComponent<PhysX::EditorShapeColliderComponent>()->GetSamplePoints();
        AZ::Vector3 sampleMin = AZ::Vector3(AZ::Constants::FloatMax);
        AZ::Vector3 sampleMax = AZ::Vector3(-AZ::Constants::FloatMax);
        for (const auto& point : samplePoints)
        {
            sampleMin = sampleMin.GetMin(point);
            sampleMax = sampleMax.GetMax(point);
        }

        EXPECT_THAT(sampleMin, UnitTest::IsClose(AZ::Vector3(-13.1f, -18.935f, -11.9f)));
        EXPECT_THAT(sampleMax, UnitTest::IsClose(AZ::Vector3(1.3f, 2.575f, 8.38f)));
    }

    TEST_F(PhysXEditorFixture, ShapeCollider_SphereWithTranslationOffset_DebugDrawCorrect)
    {
        EntityPtr sphereShapeEntity = CreateSphereShapeColliderEditorEntity(
            2.5f,
            AZ::Transform(AZ::Vector3(-5.0f, -3.0f, 1.0f), AZ::Quaternion(-0.3f, 0.9f, -0.1f, 0.3f), 1.4f),
            AZ::Vector3(4.0f, -4.0f, 6.0f)
        );

        // turn off the shape visibility, so that only the shape collider component debug draws
        LmbrCentral::EditorShapeComponentRequestsBus::Event(
            sphereShapeEntity->GetId(), &LmbrCentral::EditorShapeComponentRequests::SetVisibleInEditor, false);

        const AZ::Aabb debugDrawAabb = GetDebugDrawAabb(sphereShapeEntity->GetId());

        // use a large tolerance because the debug draw will only approximate a perfect sphere
        EXPECT_THAT(debugDrawAabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(-4.4f, -14.3f, -9.9f), 0.1f));
        EXPECT_THAT(debugDrawAabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(2.6f, -7.3f, -2.9f), 0.1f));
    }

    TEST_F(PhysXEditorFixture, ShapeCollider_SphereWithTranslationOffset_SamplePointsCorrect)
    {
        EntityPtr sphereShapeEntity = CreateSphereShapeColliderEditorEntity(
            0.6f,
            AZ::Transform(AZ::Vector3(4.0f, -1.0f, -1.0f), AZ::Quaternion(-0.7f, 0.5f, 0.1f, 0.5f), 2.5f),
            AZ::Vector3(-2.0f, 5.0f, -3.0f)
        );

        const AZStd::vector<AZ::Vector3> samplePoints =
            sphereShapeEntity->FindComponent<PhysX::EditorShapeColliderComponent>()->GetSamplePoints();
        float maxDist = 0.0f;

        // the points should be on the surface of a sphere of radius 1.5 centered at (-11.1, -4, -1.7)
        for (const auto& point : samplePoints)
        {
            maxDist = AZ::GetMax(maxDist, point.GetDistance(AZ::Vector3(-11.1f, -4.0f, -1.7f)));
        }

        EXPECT_NEAR(maxDist, 1.5f, 0.01f);
    }

    TEST_F(PhysXEditorFixture, ShapeCollider_CapsuleWithTranslationOffset_DebugDrawCorrect)
    {
        EntityPtr capsuleShapeEntity = CreateCapsuleShapeColliderEditorEntity(
            1.5f,
            6.0f,
            AZ::Transform(AZ::Vector3(2.0f, 6.0f, -1.0f), AZ::Quaternion(0.9f, 0.1f, 0.3f, 0.3f), 2.0f),
            AZ::Vector3(-3.0f, -4.0f, -5.0f)
        );

        // turn off the shape visibility, so that only the shape collider component debug draws
        LmbrCentral::EditorShapeComponentRequestsBus::Event(
            capsuleShapeEntity->GetId(), &LmbrCentral::EditorShapeComponentRequests::SetVisibleInEditor, false);

        const AZ::Aabb debugDrawAabb = GetDebugDrawAabb(capsuleShapeEntity->GetId());

        // use a large tolerance because the debug draw will only approximate a perfect capsule
        EXPECT_THAT(debugDrawAabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(-13.6f, 10.6f, -7.2f), 0.1f));
        EXPECT_THAT(debugDrawAabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(-4.0f, 19.48f, 2.64f), 0.1f));
    }

    TEST_F(PhysXEditorFixture, ShapeCollider_CapsuleWithTranslationOffset_SamplePointsCorrect)
    {
        EntityPtr sphereShapeEntity = CreateCapsuleShapeColliderEditorEntity(
            1.5f,
            6.0f,
            AZ::Transform(AZ::Vector3(2.0f, 6.0f, -1.0f), AZ::Quaternion(0.9f, 0.1f, 0.3f, 0.3f), 2.0f),
            AZ::Vector3(-3.0f, -4.0f, -5.0f)
        );

        const AZStd::vector<AZ::Vector3> samplePoints =
            sphereShapeEntity->FindComponent<PhysX::EditorShapeColliderComponent>()->GetSamplePoints();
        float maxDistSq = 0.0f;

        // the points should be on the surface of a capsule of radius 3
        for (const auto& point : samplePoints)
        {
            float distSq =
                AZ::Intersect::PointSegmentDistanceSq(point, AZ::Vector3(-10.6f, 16.48f, -0.36f), AZ::Vector3(-7.0f, 13.6f, -4.2f));
            maxDistSq = AZ::GetMax(maxDistSq, distSq);
        }

        EXPECT_NEAR(maxDistSq, 9.0f, 0.01f);
    }

    TEST_F(PhysXEditorFixture, Collider_CylinderWithOffset_CorrectDebugDraw)
    {
        const float radius = 2.0f;
        const float height = 7.5f;
        const AZ::Transform transform(AZ::Vector3(-1.0f, -3.0f, -4.0f), AZ::Quaternion(0.3f, 0.1f, 0.9f, 0.3f), 1.0f);
        const AZ::Vector3 positionOffset(2.0f, 6.0f, -3.0f);
        const AZ::Quaternion rotationOffset(-0.5f, -0.1f, 0.7f, 0.5f);
        EntityPtr editorEntity = CreateCylinderPrimitiveColliderEditorEntity(radius, height, transform, positionOffset, rotationOffset);

        const AZ::Aabb debugDrawAabb = GetDebugDrawAabb(editorEntity->GetId());

        // use a relatively large tolerance, because the cylinder will be a convex approximation rather than an exact primitive
        EXPECT_THAT(debugDrawAabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(-10.9f, -10.8f, -5.7f), 0.1f));
        EXPECT_THAT(debugDrawAabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(-3.1f, -2.4f, -0.8f), 0.1f));
    }

    TEST_F(PhysXEditorFixture, Collider_CylinderWithOffsetAndRigidBody_CorrectDebugDraw)
    {
        const float radius = 2.5f;
        const float height = 9.0f;
        const AZ::Transform transform(AZ::Vector3(4.0f, -2.0f, 4.0f), AZ::Quaternion(0.2f, 0.8f, -0.4f, 0.4f), 1.5f);
        const AZ::Vector3 positionOffset(2.0f, 3.0f, -7.0f);
        const AZ::Quaternion rotationOffset(-0.1f, -0.7f, 0.1f, 0.7f);
        EntityPtr editorEntity = CreateCylinderPrimitiveColliderEditorEntity(
            radius, height, transform, positionOffset, rotationOffset, AZStd::nullopt, RigidBodyType::Dynamic);

        const AZ::Aabb debugDrawAabb = GetDebugDrawAabb(editorEntity->GetId());

        // use a relatively large tolerance, because the cylinder will be a convex approximation rather than an exact primitive
        EXPECT_THAT(debugDrawAabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(-7.0f, 5.4f, -4.4f), 0.1f));
        EXPECT_THAT(debugDrawAabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(7.1f, 12.8f, 10.8f), 0.1f));
    }

    TEST_F(PhysXEditorFixture, Collider_CylinderWithOffsetAndNonUniformScale_CorrectDebugDraw)
    {
        const float radius = 1.5f;
        const float height = 6.0f;
        const AZ::Transform transform(AZ::Vector3(2.0f, 4.0f, -7.0f), AZ::Quaternion(0.4f, 0.8f, 0.2f, 0.4f), 0.6f);
        const AZ::Vector3 positionOffset(3.0f, -2.0f, -6.0f);
        const AZ::Quaternion rotationOffset(0.3f, 0.3f, -0.1f, 0.9f);
        const AZ::Vector3 nonUniformScale(2.0f, 0.5f, 0.8f);
        EntityPtr editorEntity =
            CreateCylinderPrimitiveColliderEditorEntity(radius, height, transform, positionOffset, rotationOffset, nonUniformScale);

        const AZ::Aabb debugDrawAabb = GetDebugDrawAabb(editorEntity->GetId());

        // use a relatively large tolerance, because the cylinder will be a convex approximation rather than an exact primitive
        EXPECT_THAT(debugDrawAabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(-2.9f, 4.1f, -9.6f), 0.1f));
        EXPECT_THAT(debugDrawAabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(-0.9f, 8.9f, -5.1f), 0.1f));
    }

    TEST_F(PhysXEditorFixture, Collider_CylinderWithOffsetNonUniformScaleAndRigidBody_CorrectDebugDraw)
    {
        const float radius = 2.5f;
        const float height = 9.0f;
        const AZ::Transform transform(AZ::Vector3(-3.0f, -2.0f, -4.0f), AZ::Quaternion(0.5f, 0.1f, -0.7f, 0.5f), 1.8f);
        const AZ::Vector3 positionOffset(2.0f, 7.0f, -1.0f);
        const AZ::Quaternion rotationOffset(0.5f, -0.5f, -0.5f, 0.5f);
        const AZ::Vector3 nonUniformScale(0.6f, 0.8f, 1.4f);
        EntityPtr editorEntity = CreateCylinderPrimitiveColliderEditorEntity(
            radius, height, transform, positionOffset, rotationOffset, nonUniformScale, RigidBodyType::Dynamic);

        const AZ::Aabb debugDrawAabb = GetDebugDrawAabb(editorEntity->GetId());

        // use a relatively large tolerance, because the cylinder will be a convex approximation rather than an exact primitive
        EXPECT_THAT(debugDrawAabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(1.9f, -13.8f, -10.5f), 0.1f));
        EXPECT_THAT(debugDrawAabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(11.3f, 0.8f, 3.9f), 0.1f));
    }
} // namespace PhysXEditorTests
