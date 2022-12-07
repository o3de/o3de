/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <EditorColliderComponent.h>
#include <EditorShapeColliderComponent.h>
#include <EditorRigidBodyComponent.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <PhysX/EditorColliderComponentRequestBus.h>
#include <PhysX/PhysXLocks.h>
#include <Tests/EditorTestUtilities.h>
#include <Tests/PhysXTestCommon.h>

namespace PhysXEditorTests
{
    TEST_F(PhysXEditorFixture, EditorRigidBodyComponent_EntityLocalScaleChangedAndPhysicsUpdateHappened_RigidBodyScaleWasUpdated)
    {
        // Create editor entity
        EntityPtr editorEntity = CreateInactiveEditorEntity("Entity");
        editorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();        
        editorEntity->CreateComponent(LmbrCentral::EditorBoxShapeComponentTypeId);

        const auto* rigidBodyComponent = editorEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();

        editorEntity->Activate();

        const AZ::Aabb originalAabb = rigidBodyComponent->GetRigidBody()->GetAabb();

        // Update the scale
        float scale = 2.0f;
        AZ::TransformBus::Event(editorEntity->GetId(), &AZ::TransformInterface::SetLocalUniformScale, scale);

        // Trigger editor physics world update so EditorRigidBodyComponent can process scale change
        auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();
        physicsSystem->Simulate(0.1f);

        const AZ::Aabb finalAabb = rigidBodyComponent->GetRigidBody()->GetAabb();

        EXPECT_THAT(finalAabb.GetMax(), UnitTest::IsClose(originalAabb.GetMax() * scale));
        EXPECT_THAT(finalAabb.GetMin(), UnitTest::IsClose(originalAabb.GetMin() * scale));
    }

    TEST_F(PhysXEditorFixture, EditorRigidBodyComponent_EntityScaledAndColliderHasNonZeroOffset_RigidBodyAabbMatchesScaledOffset)
    {
        // Create editor entity
        EntityPtr editorEntity = CreateInactiveEditorEntity("Entity");

        const auto* rigidBodyComponent = editorEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();
        const auto* colliderComponent = editorEntity->CreateComponent<PhysX::EditorColliderComponent>();

        editorEntity->Activate();

        AZ::EntityComponentIdPair idPair(editorEntity->GetId(), colliderComponent->GetId());

        // Set collider to be a sphere
        const Physics::ShapeType shapeType = Physics::ShapeType::Sphere;
        PhysX::EditorColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorColliderComponentRequests::SetShapeType, shapeType);

        // Set collider sphere radius
        const float sphereRadius = 1.0f;
        PhysX::EditorColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorColliderComponentRequests::SetSphereRadius, sphereRadius);

        // Notify listeners that collider has changed
        Physics::ColliderComponentEventBus::Event(editorEntity->GetId(), &Physics::ColliderComponentEvents::OnColliderChanged);

        auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();
        
        // Update editor world to let updates to be applied
        physicsSystem->Simulate(0.1f);

        const AZ::Aabb originalAabb = rigidBodyComponent->GetRigidBody()->GetAabb();
        
        const AZ::Vector3 offset(5.0f, 0.0f, 0.0f);
        PhysX::EditorColliderComponentRequestBus::Event(
            idPair, &PhysX::EditorColliderComponentRequests::SetColliderOffset, offset);

        // Update the scale
        float scale = 2.0f;
        AZ::TransformBus::Event(editorEntity->GetId(), &AZ::TransformInterface::SetLocalUniformScale, scale);

        // Update editor world to let updates to be applied
        physicsSystem->Simulate(0.1f);

        const AZ::Aabb finalAabb = rigidBodyComponent->GetRigidBody()->GetAabb();

        EXPECT_THAT(finalAabb.GetMax(), UnitTest::IsClose((originalAabb.GetMax() + offset) * scale));
        EXPECT_THAT(finalAabb.GetMin(), UnitTest::IsClose((originalAabb.GetMin() + offset) * scale));
    }

    TEST_F(PhysXEditorFixture, EditorRigidBodyComponent_CylinderShapeTypeInEditorCollider_ActorWithConvexMeshTypeCreated)
    {
        // Create editor entity
        EntityPtr editorEntity = CreateInactiveEditorEntity("Entity");

        const auto* rigidBodyComponent = editorEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();
        const auto* colliderComponent = editorEntity->CreateComponent<PhysX::EditorColliderComponent>();

        editorEntity->Activate();

        AZ::EntityComponentIdPair idPair(editorEntity->GetId(), colliderComponent->GetId());

        // Set collider to be a cylinder
        const Physics::ShapeType shapeType = Physics::ShapeType::Cylinder;
        PhysX::EditorColliderComponentRequestBus::Event(idPair, &PhysX::EditorColliderComponentRequests::SetShapeType, shapeType);

        // Set collider cylinder radius and height
        const float cylinderRadius = 0.5f;
        PhysX::EditorColliderComponentRequestBus::Event(idPair, &PhysX::EditorColliderComponentRequests::SetCylinderRadius, cylinderRadius);

        const float cylinderHeight = 4.0f;
        PhysX::EditorColliderComponentRequestBus::Event(idPair, &PhysX::EditorColliderComponentRequests::SetCylinderHeight, cylinderHeight);

        // Notify listeners that collider has changed
        Physics::ColliderComponentEventBus::Event(editorEntity->GetId(), &Physics::ColliderComponentEvents::OnColliderChanged);

        auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();

        // Update editor world to let updates to be applied
        physicsSystem->Simulate(0.1f);

        const AzPhysics::RigidBody* rigidBody = rigidBodyComponent->GetRigidBody();
        ASSERT_EQ(rigidBody->GetShapeCount(), 1);

        const AZ::Aabb bodyAabb = rigidBodyComponent->GetRigidBody()->GetAabb();

        // X and Y extents of the AABB should be equal to the cylinder diameter while the Z one is the height.
        EXPECT_TRUE(AZ::IsClose(bodyAabb.GetXExtent(), cylinderRadius * 2.0f));
        EXPECT_TRUE(AZ::IsClose(bodyAabb.GetYExtent(), cylinderRadius * 2.0f));
        EXPECT_TRUE(AZ::IsClose(bodyAabb.GetZExtent(), cylinderHeight));

        AZStd::shared_ptr<const Physics::Shape> shape = rigidBody->GetShape(0);
        const physx::PxRigidBody* pxRigidBody = static_cast<const physx::PxRigidBody*>(rigidBody->GetNativePointer());
        const physx::PxShape* pxShape = static_cast<const physx::PxShape*>(shape->GetNativePointer());
        // Check the geometry is a type of Convex
        PHYSX_SCENE_READ_LOCK(pxRigidBody->getScene());
        EXPECT_EQ(pxShape->getGeometryType(), physx::PxGeometryType::eCONVEXMESH);
    }

    TEST_F(PhysXEditorFixture, EditorRigidBodyComponent_CylinderColliderZeroRadius_NoColliderCreated)
    {
        // Create editor entities
        EntityPtr editorEntity = CreateInactiveEditorEntity("ZeroRadius");

        UnitTest::ErrorHandler expectedError("SetCylinderRadius: radius must be greater than zero.");

        const auto* rigidBodyComponent = editorEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();
        const auto* colliderComponent = editorEntity->CreateComponent<PhysX::EditorColliderComponent>();

        editorEntity->Activate();

        AZ::EntityComponentIdPair idPair(editorEntity->GetId(), colliderComponent->GetId());

        // Set collider to be a cylinder
        const Physics::ShapeType shapeType = Physics::ShapeType::Cylinder;
        PhysX::EditorColliderComponentRequestBus::Event(idPair, &PhysX::EditorColliderComponentRequests::SetShapeType, shapeType);

        // Set collider cylinder radius to zero
        PhysX::EditorColliderComponentRequestBus::Event(idPair, &PhysX::EditorColliderComponentRequests::SetCylinderRadius, 0.0f);

        // Notify listeners that collider has changed
        Physics::ColliderComponentEventBus::Event(editorEntity->GetId(), &Physics::ColliderComponentEvents::OnColliderChanged);

        // Verify no shapes are created and there's an expected error
        const AzPhysics::RigidBody* rigidBody = rigidBodyComponent->GetRigidBody();
        EXPECT_EQ(rigidBody->GetShapeCount(), 0);
        EXPECT_EQ(expectedError.GetErrorCount(), 1);
    }

    TEST_F(PhysXEditorFixture, EditorRigidBodyComponent_CylinderColliderZeroHeight_NoColliderCreated)
    {
        // Create editor entities
        EntityPtr editorEntity =  CreateInactiveEditorEntity("ZeroHeight");

        UnitTest::ErrorHandler expectedError("SetCylinderHeight: height must be greater than zero.");

        const auto* rigidBodyComponent = editorEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();
        const auto* colliderComponent = editorEntity->CreateComponent<PhysX::EditorColliderComponent>();

        editorEntity->Activate();

        AZ::EntityComponentIdPair idPair(editorEntity->GetId(), colliderComponent->GetId());

        // Set collider to be a cylinder
        const Physics::ShapeType shapeType = Physics::ShapeType::Cylinder;
        PhysX::EditorColliderComponentRequestBus::Event(idPair, &PhysX::EditorColliderComponentRequests::SetShapeType, shapeType);

        // Set collider cylinder height to zero
        PhysX::EditorColliderComponentRequestBus::Event(idPair, &PhysX::EditorColliderComponentRequests::SetCylinderHeight, 0.0f);

        // Notify listeners that collider has changed
        Physics::ColliderComponentEventBus::Event(editorEntity->GetId(), &Physics::ColliderComponentEvents::OnColliderChanged);

        // Verify no shapes are created and there's an expected error
        const AzPhysics::RigidBody* rigidBody = rigidBodyComponent->GetRigidBody();
        EXPECT_EQ(rigidBody->GetShapeCount(), 0);
        EXPECT_EQ(expectedError.GetErrorCount(), 1);
    }

    TEST_F(PhysXEditorFixture, EditorRigidBodyComponent_CylinderColliderSetInvalidSubdivisions_WarningIssued)
    {
        // Create editor entities
        EntityPtr editorEntity = CreateInactiveEditorEntity("InvalidSubdivisions");

        UnitTest::ErrorHandler expectedError("clamped into allowed range");

        editorEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();
        const auto* colliderComponent = editorEntity->CreateComponent<PhysX::EditorColliderComponent>();

        editorEntity->Activate();

        AZ::EntityComponentIdPair idPair(editorEntity->GetId(), colliderComponent->GetId());

        // Set collider to be a cylinder
        const Physics::ShapeType shapeType = Physics::ShapeType::Cylinder;
        PhysX::EditorColliderComponentRequestBus::Event(idPair, &PhysX::EditorColliderComponentRequests::SetShapeType, shapeType);

        // Set collider subdivision values outside the allowed range
        const AZ::u8 subdivisionsTooSmall = PhysX::Utils::MinFrustumSubdivisions - 1;
        PhysX::EditorColliderComponentRequestBus::Event(idPair, &PhysX::EditorColliderComponentRequests::SetCylinderSubdivisionCount, subdivisionsTooSmall);
        EXPECT_EQ(expectedError.GetExpectedWarningCount(), 1);

        const AZ::u8 subdivisionsTooLarge = PhysX::Utils::MaxFrustumSubdivisions + 1;
        PhysX::EditorColliderComponentRequestBus::Event(idPair, &PhysX::EditorColliderComponentRequests::SetCylinderSubdivisionCount, subdivisionsTooLarge);
        EXPECT_EQ(expectedError.GetExpectedWarningCount(), 2);
    }
} // namespace PhysXEditorTests
