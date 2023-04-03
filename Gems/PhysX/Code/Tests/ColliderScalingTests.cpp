/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AZTestShared/Math/MathTestHelpers.h>
#include <AzToolsFramework/ToolsComponents/EditorNonUniformScaleComponent.h>
#include <BaseColliderComponent.h>
#include <BoxColliderComponent.h>
#include <CapsuleColliderComponent.h>
#include <EditorColliderComponent.h>
#include <EditorRigidBodyComponent.h>
#include <EditorStaticRigidBodyComponent.h>
#include <EditorTestUtilities.h>
#include <RigidBodyComponent.h>
#include <RigidBodyStatic.h>
#include <SphereColliderComponent.h>
#include <StaticRigidBodyComponent.h>

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
        const AZ::Vector3 nonUniformScale(2.0f, 2.5f, 0.5f);

        EntityPtr editorEntity = CreateBoxPrimitiveColliderEditorEntity(
            Physics::ShapeConstants::DefaultBoxDimensions,
            AZ::Transform::CreateIdentity(),
            AZ::Vector3::CreateZero(),
            AZ::Quaternion::CreateIdentity(),
            nonUniformScale);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // because there is a non-uniform scale component, the runtime entity should have a BaseColliderComponent with a
        // cooked mesh shape configuration, rather than a BoxColliderComponent
        EXPECT_TRUE(gameEntity->FindComponent<PhysX::BoxColliderComponent>() == nullptr);
        ExpectSingleConvexRuntimeConfig(gameEntity.get());
    }

    TEST_F(PhysXEditorFixture, BoxCollider_NonUniformScale_RuntimePhysicsAabbCorrect)
    {
        const AZ::Vector3 boxDimensions(0.5f, 0.7f, 0.9f);
        const AZ::Transform transform(AZ::Vector3(5.0f, 6.0f, 7.0f), AZ::Quaternion::CreateRotationX(AZ::DegToRad(30.0f)), 1.5f);
        const AZ::Vector3 translationOffset(1.0f, 2.0f, 3.0f);
        const AZ::Quaternion rotationOffset = AZ::Quaternion::CreateRotationZ(AZ::DegToRad(45.0f));
        const AZ::Vector3 nonUniformScale(0.7f, 0.9f, 1.1f);

        EntityPtr editorEntity = CreateBoxPrimitiveColliderEditorEntity(boxDimensions, transform, translationOffset, rotationOffset, nonUniformScale);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // since there was no editor rigid body component, the runtime entity should have a static rigid body
        const auto* staticBody = azdynamic_cast<PhysX::StaticRigidBody*>(gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>()->GetSimulatedBody());

        const AZ::Aabb aabb = staticBody->GetAabb();
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(5.6045f, 4.9960f, 11.7074f), 1e-3f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(6.4955f, 6.7305f, 13.5662f), 1e-3f));
    }

    TEST_F(PhysXEditorFixture, BoxCollider_WithDynamicRigidBody_NonUniformScale_RuntimePhysicsAabbCorrect)
    {
        const AZ::Vector3 boxDimensions(0.5f, 0.7f, 0.9f);
        const AZ::Transform transform(AZ::Vector3(5.0f, 6.0f, 7.0f), AZ::Quaternion::CreateRotationX(AZ::DegToRad(30.0f)), 1.5f);
        const AZ::Vector3 translationOffset(1.0f, 2.0f, 3.0f);
        const AZ::Quaternion rotationOffset = AZ::Quaternion::CreateRotationZ(AZ::DegToRad(45.0f));
        const AZ::Vector3 nonUniformScale(0.7f, 0.9f, 1.1f);

        EntityPtr editorEntity = CreateBoxPrimitiveColliderEditorEntity(
            boxDimensions, transform, translationOffset, rotationOffset, nonUniformScale, RigidBodyType::Dynamic);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // since there was an editor rigid body component, the runtime entity should have a dynamic rigid body
        const auto* dynamicBody = gameEntity->FindComponent<PhysX::RigidBodyComponent>()->GetRigidBody();

        const AZ::Aabb aabb = dynamicBody->GetAabb();
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(5.6045f, 4.9960f, 11.7074f), 1e-3f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(6.4955f, 6.7305f, 13.5662f), 1e-3f));
    }

    TEST_F(PhysXEditorFixture, CapsuleCollider_NonUniformScaleComponent_RuntimeShapeConfigReplacedWithConvex)
    {
        const AZ::Vector3 nonUniformScale(1.0f, 1.5f, 1.0f);

        EntityPtr editorEntity = CreateCapsulePrimitiveColliderEditorEntity(
            Physics::ShapeConstants::DefaultCapsuleRadius,
            Physics::ShapeConstants::DefaultCapsuleHeight,
            AZ::Transform::CreateIdentity(),
            AZ::Vector3::CreateZero(),
            AZ::Quaternion::CreateIdentity(),
            nonUniformScale);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // because there is a non-uniform scale component, the runtime entity should have a BaseColliderComponent with a
        // cooked mesh shape configuration, rather than a CapsuleColliderComponent
        EXPECT_TRUE(gameEntity->FindComponent<PhysX::CapsuleColliderComponent>() == nullptr);
        ExpectSingleConvexRuntimeConfig(gameEntity.get());
    }

    TEST_F(PhysXEditorFixture, CapsuleCollider_NonUniformScale_RuntimePhysicsAabbCorrect)
    {
        const float capsuleRadius = 0.3f;
        const float capsuleHeight = 1.4f;
        const AZ::Transform transform(AZ::Vector3(3.0f, 1.0f, -4.0f), AZ::Quaternion::CreateRotationY(AZ::DegToRad(90.0f)), 0.5f);
        const AZ::Vector3 translationOffset(2.0f, 5.0f, 3.0f);
        const AZ::Quaternion rotationOffset = AZ::Quaternion::CreateRotationX(AZ::DegToRad(90.0f));
        const AZ::Vector3 nonUniformScale(1.2f, 0.7f, 0.6f);

        EntityPtr editorEntity = CreateCapsulePrimitiveColliderEditorEntity(
            capsuleRadius, capsuleHeight, transform, translationOffset, rotationOffset, nonUniformScale);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // since there was no editor rigid body component, the runtime entity should have a static rigid body
        const auto* staticBody = azdynamic_cast<PhysX::StaticRigidBody*>(gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>()->GetSimulatedBody());

        const AZ::Aabb aabb = staticBody->GetAabb();

        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(3.81f, 2.505f, -5.38f), 1e-3f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(3.99f, 2.995f, -5.02f), 1e-3f));
    }

    TEST_F(PhysXEditorFixture, CapsuleCollider_WithDynamicRigidBody_NonUniformScale_RuntimePhysicsAabbCorrect)
    {
        const float capsuleRadius = 0.3f;
        const float capsuleHeight = 1.4f;
        const AZ::Transform transform(AZ::Vector3(3.0f, 1.0f, -4.0f), AZ::Quaternion::CreateRotationY(AZ::DegToRad(90.0f)), 0.5f);
        const AZ::Vector3 translationOffset(2.0f, 5.0f, 3.0f);
        const AZ::Quaternion rotationOffset = AZ::Quaternion::CreateRotationX(AZ::DegToRad(90.0f));
        const AZ::Vector3 nonUniformScale(1.2f, 0.7f, 0.6f);

        EntityPtr editorEntity = CreateCapsulePrimitiveColliderEditorEntity(
            capsuleRadius, capsuleHeight, transform, translationOffset, rotationOffset, nonUniformScale, RigidBodyType::Dynamic);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // since there was an editor rigid body component, the runtime entity should have a dynamic rigid body
        const auto* dynamicBody = gameEntity->FindComponent<PhysX::RigidBodyComponent>()->GetRigidBody();

        const AZ::Aabb aabb = dynamicBody->GetAabb();

        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(3.81f, 2.505f, -5.38f), 1e-3f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(3.99f, 2.995f, -5.02f), 1e-3f));
    }

    TEST_F(PhysXEditorFixture, SphereCollider_NonUniformScaleComponent_RuntimeShapeConfigReplacedWithConvex)
    {
        const AZ::Vector3 nonUniformScale(1.0f, 1.5f, 1.0f);

        EntityPtr editorEntity = CreateSpherePrimitiveColliderEditorEntity(
            Physics::ShapeConstants::DefaultSphereRadius,
            AZ::Transform::CreateIdentity(),
            AZ::Vector3::CreateZero(),
            AZ::Quaternion::CreateIdentity(),
            nonUniformScale);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // because there is a non-uniform scale component, the runtime entity should have a BaseColliderComponent with a
        // cooked mesh shape configuration, rather than a CapsuleColliderComponent
        EXPECT_TRUE(gameEntity->FindComponent<PhysX::SphereColliderComponent>() == nullptr);
        ExpectSingleConvexRuntimeConfig(gameEntity.get());
    }

    TEST_F(PhysXEditorFixture, SphereCollider_NonUniformScale_RuntimePhysicsAabbCorrect)
    {
        const float sphereRadius = 0.7f;
        const AZ::Transform transform(AZ::Vector3(-2.0f, -1.0f, 3.0f), AZ::Quaternion::CreateRotationX(AZ::DegToRad(90.0f)), 1.2f);
        const AZ::Vector3 translationOffset(3.0f, -2.0f, 2.0f);
        const AZ::Quaternion rotationOffset = AZ::Quaternion::CreateRotationY(AZ::DegToRad(90.0f));
        const AZ::Vector3 nonUniformScale(0.8f, 0.9f, 0.6f);

        EntityPtr editorEntity =
            CreateSpherePrimitiveColliderEditorEntity(sphereRadius, transform, translationOffset, rotationOffset, nonUniformScale);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // since there was no editor rigid body component, the runtime entity should have a static rigid body
        const auto* staticBody = azdynamic_cast<PhysX::StaticRigidBody*>(gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>()->GetSimulatedBody());

        const AZ::Aabb aabb = staticBody->GetAabb();

        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(0.208f, -2.944f, 0.084f), 1e-3f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(1.552f, -1.936f, 1.596f), 1e-3f));
    }

    TEST_F(PhysXEditorFixture, SphereCollider_WithDynamicRigidBody_NonUniformScale_RuntimePhysicsAabbCorrect)
    {
        const float sphereRadius = 0.7f;
        const AZ::Transform transform(AZ::Vector3(-2.0f, -1.0f, 3.0f), AZ::Quaternion::CreateRotationX(AZ::DegToRad(90.0f)), 1.2f);
        const AZ::Vector3 translationOffset(3.0f, -2.0f, 2.0f);
        const AZ::Quaternion rotationOffset = AZ::Quaternion::CreateRotationY(AZ::DegToRad(90.0f));
        const AZ::Vector3 nonUniformScale(0.8f, 0.9f, 0.6f);

        EntityPtr editorEntity = CreateSpherePrimitiveColliderEditorEntity(
            sphereRadius, transform, translationOffset, rotationOffset, nonUniformScale, RigidBodyType::Dynamic);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // since there was an editor rigid body component, the runtime entity should have a dynamic rigid body
        const auto* dynamicBody = gameEntity->FindComponent<PhysX::RigidBodyComponent>()->GetRigidBody();

        const AZ::Aabb aabb = dynamicBody->GetAabb();

        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(0.208f, -2.944f, 0.084f), 1e-3f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(1.552f, -1.936f, 1.596f), 1e-3f));
    }

    TEST_F(PhysXEditorFixture, CylinderCollider_RuntimeShapeConfigUsingConvex)
    {
        EntityPtr editorEntity = CreateCylinderPrimitiveColliderEditorEntity(
            Physics::ShapeConstants::DefaultCylinderRadius,
            Physics::ShapeConstants::DefaultCylinderHeight);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // Since there is no cylinder shape in PhysX, the runtime entity should have a BaseColliderComponent with a
        // cooked mesh shape configuration.
        ExpectSingleConvexRuntimeConfig(gameEntity.get());
    }

    TEST_F(PhysXEditorFixture, CylinderCollider_NonUniformScaleComponent_RuntimeShapeConfigUsingConvex)
    {
        const AZ::Vector3 nonUniformScale(1.0f, 1.5f, 1.0f);

        EntityPtr editorEntity = CreateCylinderPrimitiveColliderEditorEntity(
            Physics::ShapeConstants::DefaultCylinderRadius,
            Physics::ShapeConstants::DefaultCylinderHeight,
            AZ::Transform::CreateIdentity(),
            AZ::Vector3::CreateZero(),
            AZ::Quaternion::CreateIdentity(),
            nonUniformScale);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // Since there is no cylinder shape in PhysX, the runtime entity should have a BaseColliderComponent with a
        // cooked mesh shape configuration.
        ExpectSingleConvexRuntimeConfig(gameEntity.get());
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_CylinderWithOffset_CorrectEditorStaticBodyGeometry)
    {
        const float radius = 1.5f;
        const float height = 7.5f;
        const AZ::Transform transform(AZ::Vector3(3.0f, 3.0f, 5.0f), AZ::Quaternion(0.5f, -0.5f, -0.5f, 0.5f), 1.5f);
        const AZ::Vector3 positionOffset(0.5f, 1.5f, -2.5f);
        const AZ::Quaternion rotationOffset(0.3f, -0.1f, -0.3f, 0.9f);
        EntityPtr editorEntity = CreateCylinderPrimitiveColliderEditorEntity(radius, height, transform, positionOffset, rotationOffset);

        const AZ::Aabb aabb = GetSimulatedBodyAabb(editorEntity->GetId());
        // use a relatively large tolerance, because the cylinder will be a convex approximation rather than an exact primitive
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(0.9f, -1.9f, 2.6f), 0.1f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(12.6f, 6.4f, 11.9f), 0.1f));
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_CylinderWithOffset_CorrectEditorDynamicBodyGeometry)
    {
        const float radius = 3.0f;
        const float height = 11.0f;
        const AZ::Transform transform(AZ::Vector3(5.0f, 6.0f, 7.0f), AZ::Quaternion(0.1f, 0.5f, -0.7f, 0.5f), 1.0f);
        const AZ::Vector3 positionOffset(-6.0f, -4.0f, -2.0f);
        const AZ::Quaternion rotationOffset(0.4f, 0.8f, 0.2f, 0.4f);
        EntityPtr editorEntity = CreateCylinderPrimitiveColliderEditorEntity(
            radius, height, transform, positionOffset, rotationOffset, AZStd::nullopt, RigidBodyType::Dynamic);

        const AZ::Aabb aabb = GetSimulatedBodyAabb(editorEntity->GetId());
        // use a relatively large tolerance, because the cylinder will be a convex approximation rather than an exact primitive
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(-1.7f, 8.2f, 6.1f), 0.1f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(9.6f, 14.2f, 18.5f), 0.1f));
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_CylinderWithOffset_CorrectRuntimeStaticBodyGeometry)
    {
        const float radius = 0.5f;
        const float height = 4.0f;
        const AZ::Transform transform(AZ::Vector3(3.0f, 5.0f, -9.0f), AZ::Quaternion(0.7f, -0.1f, 0.1f, 0.7f), 0.5f);
        const AZ::Vector3 positionOffset(2.0f, 5.0f, 6.0f);
        const AZ::Quaternion rotationOffset(-0.9f, 0.1f, -0.3f, 0.3f);
        EntityPtr editorEntity = CreateCylinderPrimitiveColliderEditorEntity(radius, height, transform, positionOffset, rotationOffset);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        const AZ::Aabb aabb = GetSimulatedBodyAabb(gameEntity->GetId());
        // use a relatively large tolerance, because the cylinder will be a convex approximation rather than an exact primitive
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(2.6f, 1.2f, -7.1f), 0.1f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(3.9f, 2.8f, -5.5f), 0.1f));
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_CylinderWithOffset_CorrectRuntimeDynamicBodyGeometry)
    {
        const float radius = 1.0f;
        const float height = 5.5f;
        const AZ::Transform transform(AZ::Vector3(-4.0f, -1.0f, 2.0f), AZ::Quaternion(0.4f, 0.4f, -0.2f, 0.8f), 1.0f);
        const AZ::Vector3 positionOffset(3.0f, 4.0f, 5.0f);
        const AZ::Quaternion rotationOffset(-0.5f, -0.5f, -0.5f, 0.5f);
        EntityPtr editorEntity = CreateCylinderPrimitiveColliderEditorEntity(
            radius, height, transform, positionOffset, rotationOffset, AZStd::nullopt, RigidBodyType::Dynamic);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        const AZ::Aabb aabb = GetSimulatedBodyAabb(gameEntity->GetId());
        // use a relatively large tolerance, because the cylinder will be a convex approximation rather than an exact primitive
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(0.2f, -5.1f, 1.1f), 0.1f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(5.3f, -0.2f, 5.5f), 0.1f));
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_CylinderWithOffsetAndNonUniformScale_CorrectEditorStaticBodyGeometry)
    {
        const float radius = 0.5f;
        const float height = 4.0f;
        const AZ::Transform transform(AZ::Vector3(4.0f, -3.0f, -2.0f), AZ::Quaternion(0.3f, 0.9f, -0.3f, 0.1f), 2.0f);
        const AZ::Vector3 positionOffset(0.5f, 0.2f, 0.3f);
        const AZ::Quaternion rotationOffset(0.5f, -0.5f, -0.5f, 0.5f);
        const AZ::Vector3 nonUniformScale(0.5f, 2.0f, 2.0f);
        EntityPtr editorEntity =
            CreateCylinderPrimitiveColliderEditorEntity(radius, height, transform, positionOffset, rotationOffset, nonUniformScale);

        const AZ::Aabb aabb = GetSimulatedBodyAabb(editorEntity->GetId());
        // use a relatively large tolerance, because the cylinder will be a convex approximation rather than an exact primitive
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(1.3f, -5.7f, -6.1f), 0.1f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(6.9f, -0.3f, -1.0f), 0.1f));
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_CylinderWithOffsetAndNonUniformScale_CorrectEditorDynamicBodyGeometry)
    {
        const float radius = 1.5f;
        const float height = 9.0f;
        const AZ::Transform transform(AZ::Vector3(2.0f, 5.0f, -3.0f), AZ::Quaternion(0.7f, -0.1f, 0.1f, 0.7f), 0.5f);
        const AZ::Vector3 positionOffset(-1.0f, -1.0f, 0.5f);
        const AZ::Quaternion rotationOffset(0.9f, -0.3f, -0.3f, 0.1f);
        const AZ::Vector3 nonUniformScale(1.5f, 1.0f, 0.5f);
        EntityPtr editorEntity = CreateCylinderPrimitiveColliderEditorEntity(
            radius, height, transform, positionOffset, rotationOffset, nonUniformScale, RigidBodyType::Dynamic);

        const AZ::Aabb aabb = GetSimulatedBodyAabb(editorEntity->GetId());
        // use a relatively large tolerance, because the cylinder will be a convex approximation rather than an exact primitive
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(-1.4f, 3.8f, -5.0f), 0.1f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(4.3f, 6.0f, -2.4f), 0.1f));
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_CylinderWithOffsetAndNonUniformScale_CorrectRuntimeStaticBodyGeometry)
    {
        const float radius = 1.0f;
        const float height = 5.0f;
        const AZ::Transform transform(AZ::Vector3(5.0f, 4.0f, 2.0f), AZ::Quaternion(0.4f, 0.4f, -0.8f, 0.2f), 0.8f);
        const AZ::Vector3 positionOffset(2.0f, 1.0f, -2.0f);
        const AZ::Quaternion rotationOffset(0.7f, -0.1f, -0.5f, 0.5f);
        const AZ::Vector3 nonUniformScale(3.0f, 1.0f, 2.0f);
        EntityPtr editorEntity =
            CreateCylinderPrimitiveColliderEditorEntity(radius, height, transform, positionOffset, rotationOffset, nonUniformScale);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        const AZ::Aabb aabb = GetSimulatedBodyAabb(gameEntity->GetId());
        // use a relatively large tolerance, because the cylinder will be a convex approximation rather than an exact primitive
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(0.6f, 4.0f, -8.8f), 0.1f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(7.8f, 8.1f, 2.1f), 0.1f));
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_CylinderWithOffsetAndNonUniformScale_CorrectRuntimeDynamicBodyGeometry)
    {
        const float radius = 2.0f;
        const float height = 7.0f;
        const AZ::Transform transform(AZ::Vector3(-2.0f, -3.0f, -6.0f), AZ::Quaternion(0.1f, 0.5f, -0.7f, 0.5f), 3.0f);
        const AZ::Vector3 positionOffset(-1.0, 0.5f, -2.0f);
        const AZ::Quaternion rotationOffset(0.2f, -0.4f, 0.8f, 0.4f);
        const AZ::Vector3 nonUniformScale(2.0f, 2.0f, 5.0f);
        EntityPtr editorEntity = CreateCylinderPrimitiveColliderEditorEntity(
            radius, height, transform, positionOffset, rotationOffset, nonUniformScale, RigidBodyType::Dynamic);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        const AZ::Aabb aabb = GetSimulatedBodyAabb(gameEntity->GetId());
        // use a relatively large tolerance, because the cylinder will be a convex approximation rather than an exact primitive
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsCloseTolerance(AZ::Vector3(-25.0f, -20.8f, -53.9f), 0.1f));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsCloseTolerance(AZ::Vector3(10.0f, 70.0f, 17.2f), 0.1f));
    }
} // namespace PhysXEditorTests
