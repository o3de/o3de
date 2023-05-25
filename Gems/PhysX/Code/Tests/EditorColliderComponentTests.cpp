/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/ToolsComponents/EditorNonUniformScaleComponent.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/Manipulators/BoxManipulatorRequestBus.h>
#include <EditorColliderComponent.h>
#include <EditorMeshColliderComponent.h>
#include <EditorShapeColliderComponent.h>
#include <EditorForceRegionComponent.h>
#include <EditorRigidBodyComponent.h>
#include <EditorStaticRigidBodyComponent.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <PhysX/ForceRegionComponentBus.h>
#include <PhysX/MathConversion.h>
#include <PhysX/PhysXLocks.h>
#include <PhysX/SystemComponentBus.h>
#include <RigidBodyComponent.h>
#include <RigidBodyStatic.h>
#include <ShapeColliderComponent.h>
#include <SphereColliderComponent.h>
#include <BoxColliderComponent.h>
#include <CapsuleColliderComponent.h>
#include <StaticRigidBodyComponent.h>
#include <Tests/EditorTestUtilities.h>
#include <Tests/PhysXTestCommon.h>

namespace PhysXEditorTests
{
    TEST_F(PhysXEditorFixture, EditorColliderComponent_RigidBodyDependencySatisfied_EntityIsValid)
    {
        EntityPtr entity = CreateInactiveEditorEntity("ColliderComponentEditorEntity");
        entity->CreateComponent<PhysX::EditorColliderComponent>();
        entity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();

        // the entity should be in a valid state because the component requirement is satisfied
        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_TRUE(sortOutcome.IsSuccess());
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_RigidBodyDependencyMissing_EntityIsInvalid)
    {
        EntityPtr entity = CreateInactiveEditorEntity("ColliderComponentEditorEntity");
        entity->CreateComponent<PhysX::EditorColliderComponent>();

        // the entity should not be in a valid state because the collider component requires a rigid body
        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_FALSE(sortOutcome.IsSuccess());
        EXPECT_TRUE(sortOutcome.GetError().m_code == AZ::Entity::DependencySortResult::MissingRequiredService);
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_MultipleColliderComponents_EntityIsValid)
    {
        EntityPtr entity = CreateInactiveEditorEntity("ColliderComponentEditorEntity");
        entity->CreateComponent<PhysX::EditorColliderComponent>();
        entity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();

        // adding a second collider component should not make the entity invalid
        entity->CreateComponent<PhysX::EditorColliderComponent>();

        // the entity should be in a valid state because the component requirement is satisfied
        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_TRUE(sortOutcome.IsSuccess());
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_WithOtherColliderComponents_EntityIsValid)
    {
        EntityPtr entity = CreateInactiveEditorEntity("ColliderComponentEditorEntity");
        entity->CreateComponent<PhysX::EditorColliderComponent>();
        entity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();

        // the collider component should be compatible with multiple collider components
        entity->CreateComponent<PhysX::EditorMeshColliderComponent>();
        entity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        entity->CreateComponent(LmbrCentral::EditorBoxShapeComponentTypeId);

        // the entity should be in a valid state because the component requirement is satisfied
        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_TRUE(sortOutcome.IsSuccess());
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_ColliderWithBox_CorrectRuntimeComponents)
    {
        // create an editor entity with a collider component
        EntityPtr editorEntity = CreateInactiveEditorEntity("ColliderComponentEditorEntity");
        auto* colliderComponent = editorEntity->CreateComponent<PhysX::EditorColliderComponent>();
        editorEntity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();
        editorEntity->Activate();

        // Set collider to be a box
        AZ::EntityComponentIdPair idPair(editorEntity->GetId(), colliderComponent->GetId());
        PhysX::EditorPrimitiveColliderComponentRequestBus::Event(idPair, &PhysX::EditorPrimitiveColliderComponentRequests::SetShapeType, Physics::ShapeType::Box);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // check that the runtime entity has the expected components
        EXPECT_TRUE(gameEntity->FindComponent<PhysX::BoxColliderComponent>() != nullptr);
        EXPECT_TRUE(gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>() != nullptr);
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_ColliderWithBox_CorrectRuntimeGeometry)
    {
        // create an editor entity with a collider component
        const AZ::Vector3 boxDimensions(2.0f, 3.0f, 4.0f);
        EntityPtr editorEntity = CreateBoxPrimitiveColliderEditorEntity(boxDimensions);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // since there was no editor rigid body component, the runtime entity should have a static rigid body
        const auto* staticBody = azdynamic_cast<PhysX::StaticRigidBody*>(gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>()->GetSimulatedBody());
        const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(staticBody->GetNativePointer());

        PHYSX_SCENE_READ_LOCK(pxRigidStatic->getScene());

        // there should be a single shape on the rigid body and it should be a box
        EXPECT_EQ(pxRigidStatic->getNbShapes(), 1);
        if (pxRigidStatic->getNbShapes() > 0)
        {
            physx::PxShape* shape = nullptr;
            pxRigidStatic->getShapes(&shape, 1, 0);
            EXPECT_EQ(shape->getGeometryType(), physx::PxGeometryType::eBOX);
        }

        // the bounding box of the rigid body should reflect the dimensions of the box set above
        AZ::Aabb aabb = staticBody->GetAabb();
        EXPECT_TRUE(aabb.GetMax().IsClose(0.5f * boxDimensions));
        EXPECT_TRUE(aabb.GetMin().IsClose(-0.5f * boxDimensions));
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_BoxPrimitiveColliderWithTranslationOffset_CorrectEditorStaticBodyGeometry)
    {
        const AZ::Vector3 boxDimensions(5.0f, 8.0f, 6.0f);
        const AZ::Transform transform(AZ::Vector3(2.0f, -6.0f, 5.0f), AZ::Quaternion(0.3f, -0.3f, 0.1f, 0.9f), 1.6f);
        const AZ::Vector3 translationOffset(-4.0f, 3.0f, -1.0f);
        const AZ::Vector3 nonUniformScale(2.0f, 2.5f, 0.5f);
        EntityPtr editorEntity = CreateBoxPrimitiveColliderEditorEntity(boxDimensions, transform, translationOffset, AZ::Quaternion::CreateIdentity(), nonUniformScale);

        AZ::Aabb aabb = GetSimulatedBodyAabb(editorEntity->GetId());
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsClose(AZ::Vector3(-25.488f, -10.16f, -11.448f)));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsClose(AZ::Vector3(1.136f, 18.32f, 16.584f)));
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_BoxPrimitiveColliderWithTranslationOffset_CorrectEditorDynamicBodyGeometry)
    {
        const AZ::Vector3 boxDimensions(6.0f, 4.0f, 1.0f);
        const AZ::Transform transform(AZ::Vector3(-5.0f, -1.0f, 3.0f), AZ::Quaternion(0.7f, 0.5f, -0.1f, 0.5f), 1.2f);
        const AZ::Vector3 translationOffset(6.0f, -5.0f, -4.0f);
        const AZ::Vector3 nonUniformScale(1.5f, 1.5f, 4.0f);
        EntityPtr editorEntity = CreateBoxPrimitiveColliderEditorEntity(
            boxDimensions, transform, translationOffset, AZ::Quaternion::CreateIdentity(), nonUniformScale, RigidBodyType::Dynamic);

        editorEntity->Deactivate();
        editorEntity->Activate();

        AZ::Aabb aabb = GetSimulatedBodyAabb(editorEntity->GetId());
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsClose(AZ::Vector3(-20.264f, 15.68f, -6.864f)));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsClose(AZ::Vector3(-7.592f, 26.0f, 6.672f)));
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_BoxPrimitiveColliderWithTranslationOffset_CorrectRuntimeStaticBodyGeometry)
    {
        const AZ::Vector3 boxDimensions(1.0f, 4.0f, 7.0f);
        const AZ::Transform transform(AZ::Vector3(7.0f, 2.0f, 4.0f), AZ::Quaternion(0.4f, -0.8f, 0.4f, 0.2f), 2.5f);
        const AZ::Vector3 translationOffset(6.0f, -1.0f, -2.0f);
        const AZ::Vector3 nonUniformScale(0.8f, 2.0f, 1.5f);
        EntityPtr editorEntity = CreateBoxPrimitiveColliderEditorEntity(
            boxDimensions, transform, translationOffset, AZ::Quaternion::CreateIdentity(), nonUniformScale);
        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        AZ::Aabb aabb = GetSimulatedBodyAabb(gameEntity->GetId());
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsClose(AZ::Vector3(-4.8f, -14.14f, 5.265f)));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsClose(AZ::Vector3(12.4f, 15.02f, 31.895f)));
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_BoxPrimitiveColliderWithTranslationOffset_CorrectRuntimeDynamicBodyGeometry)
    {
        const AZ::Vector3 boxDimensions(4.0f, 2.0f, 7.0f);
        const AZ::Transform transform(AZ::Vector3(4.0f, 4.0f, 2.0f), AZ::Quaternion(0.1f, 0.3f, 0.9f, 0.3f), 0.8f);
        const AZ::Vector3 translationOffset(-2.0f, 7.0f, -1.0f);
        const AZ::Vector3 nonUniformScale(2.5f, 1.0f, 2.0f);
        EntityPtr editorEntity = CreateBoxPrimitiveColliderEditorEntity(
            boxDimensions, transform, translationOffset, AZ::Quaternion::CreateIdentity(), nonUniformScale, RigidBodyType::Dynamic);
        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        AZ::Aabb aabb = GetSimulatedBodyAabb(gameEntity->GetId());
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsClose(AZ::Vector3(-1.664f, -8.352f, -0.88f)));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsClose(AZ::Vector3(9.536f, 2.848f, 9.04f)));
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_ColliderWithSphere_CorrectRuntimeComponents)
    {
        // create an editor entity with a collider component
        EntityPtr editorEntity = CreateInactiveEditorEntity("ColliderComponentEditorEntity");
        auto* colliderComponent = editorEntity->CreateComponent<PhysX::EditorColliderComponent>();
        editorEntity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();
        editorEntity->Activate();

        // Set collider to be a sphere
        AZ::EntityComponentIdPair idPair(editorEntity->GetId(), colliderComponent->GetId());
        PhysX::EditorPrimitiveColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorPrimitiveColliderComponentRequests::SetShapeType, Physics::ShapeType::Sphere);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // check that the runtime entity has the expected components
        EXPECT_TRUE(gameEntity->FindComponent<PhysX::SphereColliderComponent>() != nullptr);
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_ColliderWithSphereAndTranslationOffset_CorrectEditorStaticBodyGeometry)
    {
        EntityPtr editorEntity = CreateSpherePrimitiveColliderEditorEntity(
            1.6f,
            AZ::Transform(AZ::Vector3(4.0f, 2.0f, -2.0f), AZ::Quaternion(-0.5f, -0.5f, 0.1f, 0.7f), 3.0f),
            AZ::Vector3(2.0f, 3.0f, -7.0f)
        );

        AZ::Aabb aabb = GetSimulatedBodyAabb(editorEntity->GetId());
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsClose(AZ::Vector3(22.12f, -7.24f, -10.4f)));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsClose(AZ::Vector3(31.72f, 2.36f, -0.8f)));
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_ColliderWithSphereAndTranslationOffset_CorrectEditorDynamicBodyGeometry)
    {
        EntityPtr editorEntity = CreateSpherePrimitiveColliderEditorEntity(
            3.5f,
            AZ::Transform(AZ::Vector3(2.0f, -5.0f, -6.0f), AZ::Quaternion(0.7f, 0.1f, 0.7f, 0.1f), 0.4f),
            AZ::Vector3(1.0f, 3.0f, -1.0f),
            AZ::Quaternion::CreateIdentity(),
            AZStd::nullopt,
            RigidBodyType::Dynamic);

        editorEntity->Deactivate();
        editorEntity->Activate();

        AZ::Aabb aabb = GetSimulatedBodyAabb(editorEntity->GetId());
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsClose(AZ::Vector3(0.2f, -7.44f, -6.68f)));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsClose(AZ::Vector3(3.0f, -4.64f, -3.88f)));
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_ColliderWithSphereAndTranslationOffset_CorrectRuntimeStaticBodyGeometry)
    {
        EntityPtr editorEntity = CreateSpherePrimitiveColliderEditorEntity(
            2.0f,
            AZ::Transform(AZ::Vector3(4.0f, 4.0f, -1.0f), AZ::Quaternion(0.8f, -0.2f, 0.4f, 0.4f), 2.4f),
            AZ::Vector3(5.0f, 6.0f, -8.0f)
        );

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        AZ::Aabb aabb = GetSimulatedBodyAabb(gameEntity->GetId());
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsClose(AZ::Vector3(-12.032f, 5.92f, 17.624f)));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsClose(AZ::Vector3(-2.432f, 15.52f, 27.224f)));
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_ColliderWithSphereAndTranslationOffset_CorrectRuntimeDynamicBodyGeometry)
    {
        EntityPtr editorEntity = CreateSpherePrimitiveColliderEditorEntity(
            0.4f,
            AZ::Transform(AZ::Vector3(2.0f, 2.0f, -5.0f), AZ::Quaternion(0.9f, 0.3f, 0.3f, 0.1f), 5.0f),
            AZ::Vector3(4.0f, 7.0f, 3.0f),
            AZ::Quaternion::CreateIdentity(),
            AZStd::nullopt,
            RigidBodyType::Dynamic);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        AZ::Aabb aabb = GetSimulatedBodyAabb(gameEntity->GetId());
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsClose(AZ::Vector3(38.6f, -16.0f, 3.2f)));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsClose(AZ::Vector3(42.6f, -12.0f, 7.2f)));
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_ColliderWithCapsule_CorrectRuntimeComponents)
    {
        // create an editor entity with a collider component
        EntityPtr editorEntity = CreateInactiveEditorEntity("ColliderComponentEditorEntity");
        auto* colliderComponent = editorEntity->CreateComponent<PhysX::EditorColliderComponent>();
        editorEntity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();
        editorEntity->Activate();

        // Set collider to be a capsule
        AZ::EntityComponentIdPair idPair(editorEntity->GetId(), colliderComponent->GetId());
        PhysX::EditorPrimitiveColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorPrimitiveColliderComponentRequests::SetShapeType, Physics::ShapeType::Capsule);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // check that the runtime entity has the expected components
        EXPECT_TRUE(gameEntity->FindComponent<PhysX::CapsuleColliderComponent>() != nullptr);
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_ColliderWithCapsuleAndTranslationOffset_CorrectEditorStaticBodyGeometry)
    {
        EntityPtr editorEntity = CreateCapsulePrimitiveColliderEditorEntity(
            2.0f,
            8.0f,
            AZ::Transform(AZ::Vector3(2.0f, 1.0f, -2.0f), AZ::Quaternion(-0.2f, -0.8f, -0.4f, 0.4f), 4.0f),
            AZ::Vector3(2.0f, 3.0f, 5.0f)
        );

        AZ::Aabb aabb = GetSimulatedBodyAabb(editorEntity->GetId());
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsClose(AZ::Vector3(-16.56f, 9.8f, -7.92f)));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsClose(AZ::Vector3(7.12f, 38.6f, 13.84f)));
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_ColliderWithCapsuleAndTranslationOffset_CorrectEditorDynamicBodyGeometry)
    {
        EntityPtr editorEntity = CreateCapsulePrimitiveColliderEditorEntity(
            1.0f,
            5.0f,
            AZ::Transform(AZ::Vector3(7.0f, -9.0f, 2.0f), AZ::Quaternion(0.7f, 0.1f, 0.7f, 0.1f), 0.2f),
            AZ::Vector3(2.0f, 9.0f, -5.0f),
            AZ::Quaternion::CreateIdentity(),
            AZStd::nullopt,
            RigidBodyType::Dynamic);

        editorEntity->Deactivate();
        editorEntity->Activate();

        AZ::Aabb aabb = GetSimulatedBodyAabb(editorEntity->GetId());
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsClose(AZ::Vector3(5.5f, -10.816f, 2.688f)));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsClose(AZ::Vector3(6.5f, -10.416f, 3.088f)));
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_ColliderWithCapsuleAndTranslationOffset_CorrectRuntimeStaticBodyGeometry)
    {
        EntityPtr editorEntity = CreateCapsulePrimitiveColliderEditorEntity(
            2.0f,
            11.0f,
            AZ::Transform(AZ::Vector3(-4.0f, -3.0f, -1.0f), AZ::Quaternion(0.5f, -0.7f, -0.1f, 0.5f), 0.8f),
            AZ::Vector3(4.0f, 1.0f, -3.0f)
        );

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        AZ::Aabb aabb = GetSimulatedBodyAabb(gameEntity->GetId());
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsClose(AZ::Vector3(-6.4f, -6.92f, -0.36f)));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsClose(AZ::Vector3(1.28f, -1.704f, 5.528f)));
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_ColliderWithCapsuleAndTranslationOffset_CorrectRuntimeDynamicBodyGeometry)
    {
        EntityPtr editorEntity = CreateCapsulePrimitiveColliderEditorEntity(
            0.4f,
            3.0f,
            AZ::Transform(AZ::Vector3(7.0f, 6.0f, -3.0f), AZ::Quaternion(-0.3f, -0.1f, -0.3f, 0.9f), 6.0f),
            AZ::Vector3(2.0f, -7.0f, 7.0f),
            AZ::Quaternion::CreateIdentity(),
            AZStd::nullopt,
            RigidBodyType::Dynamic);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        AZ::Aabb aabb = GetSimulatedBodyAabb(gameEntity->GetId());
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsClose(AZ::Vector3(-11.0f, -7.8f, 47.4f)));
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsClose(AZ::Vector3(-6.2f, 4.92f, 62.76f)));
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_ColliderWithCylinderWithValidRadiusAndValidHeight_CorrectRuntimeGeometry)
    {
        const float validRadius = 1.0f;
        const float validHeight = 1.0f;
        EntityPtr editorEntity = CreateCylinderPrimitiveColliderEditorEntity(validRadius, validHeight);
        
        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());
        
        // since there was no editor rigid body component, the runtime entity should have a static rigid body
        const auto* staticBody = azdynamic_cast<PhysX::StaticRigidBody*>(gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>()->GetSimulatedBody());
        const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(staticBody->GetNativePointer());

        PHYSX_SCENE_READ_LOCK(pxRigidStatic->getScene());
        
        // there should be a single shape on the rigid body and it should be a convex mesh
        EXPECT_EQ(pxRigidStatic->getNbShapes(), 1);
        if (pxRigidStatic->getNbShapes() > 0)
        {
            physx::PxShape* shape = nullptr;
            pxRigidStatic->getShapes(&shape, 1, 0);
            EXPECT_EQ(shape->getGeometryType(), physx::PxGeometryType::eCONVEXMESH);
        }
        
        // the bounding box of the rigid body should reflect the dimensions of the cylinder set above
        AZ::Aabb aabb = staticBody->GetAabb();
        
        // Check that the z positions of the bounding box match that of the cylinder
        EXPECT_THAT(aabb.GetMin().GetZ(), ::testing::FloatNear(-0.5f * validHeight, AZ::Constants::Tolerance));
        EXPECT_THAT(aabb.GetMax().GetZ(), ::testing::FloatNear(0.5f * validHeight, AZ::Constants::Tolerance));
        
        // check that the xy points are not outside the radius of the cylinder
        AZ::Vector2 vecMin(aabb.GetMin().GetX(), aabb.GetMin().GetY());
        AZ::Vector2 vecMax(aabb.GetMax().GetX(), aabb.GetMax().GetY());
        EXPECT_TRUE(AZ::GetAbs(vecMin.GetX()) <= validRadius);
        EXPECT_TRUE(AZ::GetAbs(vecMin.GetY()) <= validRadius);
        EXPECT_TRUE(AZ::GetAbs(vecMax.GetX()) <= validRadius);
        EXPECT_TRUE(AZ::GetAbs(vecMax.GetX()) <= validRadius);        
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_ColliderWithCylinderWithNullRadius_HandledGracefully)
    {
        ValidateInvalidEditorColliderComponentParams(0.f, 1.f);
    }
    
    TEST_F(PhysXEditorFixture, EditorColliderComponent_ColliderWithCylinderWithNullHeight_HandledGracefully)
    {
        ValidateInvalidEditorColliderComponentParams(1.f, 0.f);
    }
    
    TEST_F(PhysXEditorFixture, EditorColliderComponent_ColliderWithCylinderWithNullRadiusAndNullHeight_HandledGracefully)
    {
        ValidateInvalidEditorColliderComponentParams(0.f, 0.f);
    }
    
    TEST_F(PhysXEditorFixture, EditorColliderComponent_ColliderWithCylinderWithNegativeRadiusAndNullHeight_HandledGracefully)
    {
        ValidateInvalidEditorColliderComponentParams(-1.f, 0.f);
    }
    
    TEST_F(PhysXEditorFixture, EditorColliderComponent_ColliderWithCylinderWithNullRadiusAndNegativeHeight_HandledGracefully)
    {
        ValidateInvalidEditorColliderComponentParams(0.f, -1.f);
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_ColliderWithCylinderSwitchingFromNullHeightToValidHeight_HandledGracefully)
    {
        // create an editor entity with a collider component
        EntityPtr editorEntity = CreateInactiveEditorEntity("ColliderComponentEditorEntity");
        auto* colliderComponent = editorEntity->CreateComponent<PhysX::EditorColliderComponent>();
        editorEntity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();
        editorEntity->Activate();

        // Set collider to be a cylinder
        AZ::EntityComponentIdPair idPair(editorEntity->GetId(), colliderComponent->GetId());
        PhysX::EditorPrimitiveColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorPrimitiveColliderComponentRequests::SetShapeType, Physics::ShapeType::Cylinder);

        const float validRadius = 1.0f;
        const float nullHeight = 0.0f;
        const float validHeight = 1.0f;

        PhysX::EditorPrimitiveColliderComponentRequestBus::Event(idPair, &PhysX::EditorPrimitiveColliderComponentRequests::SetCylinderRadius, validRadius);

        {
            UnitTest::ErrorHandler dimensionErrorHandler("SetCylinderHeight: height must be greater than zero.");

            PhysX::EditorPrimitiveColliderComponentRequestBus::Event(idPair, &PhysX::EditorPrimitiveColliderComponentRequests::SetCylinderHeight, nullHeight);

            EXPECT_EQ(dimensionErrorHandler.GetExpectedErrorCount(), 1);
        }

        {
            UnitTest::ErrorHandler dimensionErrorHandler("SetCylinderHeight: height must be greater than zero.");

            PhysX::EditorPrimitiveColliderComponentRequestBus::Event(
                idPair, &PhysX::EditorPrimitiveColliderComponentRequests::SetCylinderHeight, validHeight);

            EXPECT_EQ(dimensionErrorHandler.GetExpectedErrorCount(), 0);
        }
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_ColliderWithBoxAndRigidBody_CorrectRuntimeComponents)
    {
        // create an editor entity with a collider component
        EntityPtr editorEntity = CreateInactiveEditorEntity("ColliderComponentEditorEntity");
        auto* colliderComponent = editorEntity->CreateComponent<PhysX::EditorColliderComponent>();
        editorEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();
        editorEntity->Activate();

        // Set collider to be a box
        AZ::EntityComponentIdPair idPair(editorEntity->GetId(), colliderComponent->GetId());
        PhysX::EditorPrimitiveColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorPrimitiveColliderComponentRequests::SetShapeType, Physics::ShapeType::Box);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // check that the runtime entity has the expected components
        EXPECT_TRUE(gameEntity->FindComponent<PhysX::BoxColliderComponent>() != nullptr);
        EXPECT_TRUE(gameEntity->FindComponent<PhysX::RigidBodyComponent>() != nullptr);
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_ColliderWithBoxAndRigidBody_CorrectRuntimeEntity)
    {
        // create an editor entity with a collider component
        const AZ::Vector3 boxDimensions(2.0f, 3.0f, 4.0f);
        EntityPtr editorEntity = CreateBoxPrimitiveColliderEditorEntity(
            boxDimensions,
            AZ::Transform::CreateIdentity(),
            AZ::Vector3::CreateZero(),
            AZ::Quaternion::CreateIdentity(),
            AZStd::nullopt,
            RigidBodyType::Dynamic);

        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // since there was an editor rigid body component, the runtime entity should have a dynamic rigid body
        const auto* rigidBody = gameEntity->FindComponent<PhysX::RigidBodyComponent>()->GetRigidBody();
        const auto* pxRigidDynamic = static_cast<const physx::PxRigidDynamic*>(rigidBody->GetNativePointer());

        PHYSX_SCENE_READ_LOCK(pxRigidDynamic->getScene());

        // there should be a single shape on the rigid body and it should be a box
        EXPECT_EQ(pxRigidDynamic->getNbShapes(), 1);
        if (pxRigidDynamic->getNbShapes() > 0)
        {
            physx::PxShape* shape = nullptr;
            pxRigidDynamic->getShapes(&shape, 1, 0);
            EXPECT_EQ(shape->getGeometryType(), physx::PxGeometryType::eBOX);
        }

        // the bounding box of the rigid body should reflect the dimensions of the box set above
        AZ::Aabb aabb = rigidBody->GetAabb();
        EXPECT_TRUE(aabb.GetMax().IsClose(0.5f * boxDimensions));
        EXPECT_TRUE(aabb.GetMin().IsClose(-0.5f * boxDimensions));
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_TransformChanged_ColliderUpdated)
    {
        // create an editor entity with a collider component
        EntityPtr editorEntity = CreateInactiveEditorEntity("ColliderComponentEditorEntity");
        auto* colliderComponent = editorEntity->CreateComponent<PhysX::EditorColliderComponent>();
        editorEntity->CreateComponent<PhysX::EditorStaticRigidBodyComponent>();
        editorEntity->Activate();
        
        // Set collider to be a box
        AZ::EntityComponentIdPair idPair(editorEntity->GetId(), colliderComponent->GetId());
        PhysX::EditorPrimitiveColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorPrimitiveColliderComponentRequests::SetShapeType, Physics::ShapeType::Box);

        AZ::Vector3 boxDimensions = AZ::Vector3::CreateOne();
        PhysX::EditorPrimitiveColliderComponentRequestBus::EventResult(
            boxDimensions, idPair, &PhysX::EditorPrimitiveColliderComponentRequests::GetBoxDimensions);

        AZ::EntityId editorEntityId = editorEntity->GetId();

        // update the transform
        const float scale = 2.0f;
        AZ::TransformBus::Event(editorEntityId, &AZ::TransformInterface::SetLocalUniformScale, scale);
        const AZ::Vector3 translation(10.0f, 20.0f, 30.0f);
        AZ::TransformBus::Event(editorEntityId, &AZ::TransformInterface::SetWorldTranslation, translation);

        // make a game entity and check its bounding box is consistent with the changed transform
        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());
        const auto* staticBody = azdynamic_cast<PhysX::StaticRigidBody*>(gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>()->GetSimulatedBody());
        AZ::Aabb aabb = staticBody->GetAabb();
        EXPECT_TRUE(aabb.GetMax().IsClose(translation + 0.5f * scale * boxDimensions));
        EXPECT_TRUE(aabb.GetMin().IsClose(translation - 0.5f * scale * boxDimensions));
    }

    TEST_F(PhysXEditorFixture, EditorColliderComponent_ColliderWithScaleSetToParentEntity_CorrectRuntimeScale)
    {
        // create an editor parent entity (empty, need transform component only)
        EntityPtr editorParentEntity = CreateInactiveEditorEntity("ParentEntity");
        editorParentEntity->Activate();

        // set some scale to parent entity
        const float parentScale = 2.0f;
        AZ::TransformBus::Event(editorParentEntity->GetId(), &AZ::TransformInterface::SetLocalUniformScale, parentScale);

        // create an editor child entity with a collider component
        EntityPtr editorChildEntity = CreateInactiveEditorEntity("ChildEntity");
        auto* colliderComponent = editorChildEntity->CreateComponent<PhysX::EditorColliderComponent>();
        editorChildEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();
        editorChildEntity->Activate();

        // Set collider to be a box
        AZ::EntityComponentIdPair idPair(editorChildEntity->GetId(), colliderComponent->GetId());
        PhysX::EditorPrimitiveColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorPrimitiveColliderComponentRequests::SetShapeType, Physics::ShapeType::Box);

        // set some dimensions to child entity box component
        const AZ::Vector3 boxDimensions(2.0f, 3.0f, 4.0f);
        PhysX::EditorPrimitiveColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorPrimitiveColliderComponentRequests::SetBoxDimensions, boxDimensions);

        // set one entity as parent of another
        AZ::TransformBus::Event(editorChildEntity->GetId(),
            &AZ::TransformBus::Events::SetParentRelative,
            editorParentEntity->GetId());

        // build child game entity (parent will be built implicitly)
        EntityPtr gameChildEntity = CreateActiveGameEntityFromEditorEntity(editorChildEntity.get());

        // since there was an editor rigid body component, the runtime entity should have a dynamic rigid body
        const AzPhysics::RigidBody* rigidBody =
            gameChildEntity->FindComponent<PhysX::RigidBodyComponent>()->GetRigidBody();
        
        // the bounding box of the rigid body should reflect the dimensions of the box set above
        // and parent entity scale
        const AZ::Aabb aabb = rigidBody->GetAabb();
        EXPECT_THAT(aabb.GetMax(), UnitTest::IsClose(0.5f * boxDimensions * parentScale));
        EXPECT_THAT(aabb.GetMin(), UnitTest::IsClose(-0.5f * boxDimensions * parentScale));
    }
} // namespace PhysXEditorTests
