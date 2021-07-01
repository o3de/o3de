/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PhysX_precompiled.h"

#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <EditorColliderComponent.h>
#include <EditorShapeColliderComponent.h>
#include <EditorRigidBodyComponent.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <PhysX/EditorColliderComponentRequestBus.h>
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

    // LYN-1241 - Test disabled due to AZ_Error reports about MaterialLibrary being not found in the AssetCatalog
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
} // namespace PhysXEditorTests
