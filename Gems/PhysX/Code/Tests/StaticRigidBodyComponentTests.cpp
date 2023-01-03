/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <PhysX/PhysXLocks.h>
#include <Source/EditorColliderComponent.h>
#include <Source/EditorShapeColliderComponent.h>
#include <Source/EditorRigidBodyComponent.h>
#include <Source/EditorColliderComponent.h>
#include <Source/StaticRigidBodyComponent.h>

#include <Tests/EditorTestUtilities.h>
#include <Tests/PhysXTestCommon.h>

namespace PhysXEditorTests
{
    int GetEditorStaticRigidBodyCount()
    {
        AzPhysics::SceneHandle sceneHandle;
        Physics::EditorWorldBus::BroadcastResult(sceneHandle, &Physics::EditorWorldRequests::GetEditorSceneHandle);

        physx::PxScene* pxScene = nullptr;

        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            if (AzPhysics::Scene* scene = physicsSystem->GetScene(sceneHandle))
            {
                pxScene = static_cast<physx::PxScene*>(scene->GetNativePointer());
            }
        }

        PHYSX_SCENE_READ_LOCK(pxScene);

        return pxScene->getNbActors(physx::PxActorTypeFlag::eRIGID_STATIC);
    }

    void AddEditorBoxShapeComponent(EntityPtr& editorEntity)
    {
        editorEntity->CreateComponent(LmbrCentral::EditorBoxShapeComponentTypeId);

        // Set some dimensions to box component so we have shapes for physics collider
        const AZ::Vector3 boxDimensions(2.0f, 3.0f, 4.0f);
        LmbrCentral::BoxShapeComponentRequestsBus::Event(editorEntity->GetId(),
            &LmbrCentral::BoxShapeComponentRequests::SetBoxDimensions,
            boxDimensions);
    }

    TEST_F(PhysXEditorFixture, StaticRigidBodyComponent_NoRigidBody_RuntimeStaticRigidBodyComponentCreated)
    {
        // Create editor entity
        EntityPtr editorEntity = CreateInactiveEditorEntity("Entity");
        editorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        AddEditorBoxShapeComponent(editorEntity);

        // Create game entity and verify StaticRigidBodyComponent was created
        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());
        const auto* staticRigidBody = gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>();

        EXPECT_TRUE(staticRigidBody != nullptr);
    }

    TEST_F(PhysXEditorFixture, StaticRigidBodyComponent_RigidBody_NoRuntimeStaticRigidBodyComponent)
    {
        // Create editor entity
        EntityPtr editorEntity = CreateInactiveEditorEntity("Entity");
        editorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        AddEditorBoxShapeComponent(editorEntity);

        // Add EditorRigidBodyComponent (depends on PhysXColliderService and
        // should prevent runtime StaticRigidBodyComponent creation)
        editorEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();

        // Create game entity and verify StaticRigidBodyComponent was NOT created
        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());
        const auto* staticRigidBody = gameEntity->FindComponent<PhysX::StaticRigidBodyComponent>();

        EXPECT_TRUE(staticRigidBody == nullptr);
    }

    TEST_F(PhysXEditorFixture, StaticRigidBodyComponent_MultipleColliderComponents_SingleRuntimeStaticRigidBodyComponent)
    {
        // Create editor entity
        EntityPtr editorEntity = CreateInactiveEditorEntity("Entity");
        AddEditorBoxShapeComponent(editorEntity);

        // Add two EditorColliderComponent components to the entity
        editorEntity->CreateComponent<PhysX::EditorColliderComponent>();
        editorEntity->CreateComponent<PhysX::EditorColliderComponent>();

        // Create game entity and verify only one StaticRigidBodyComponent was created
        EntityPtr gameEntity = CreateActiveGameEntityFromEditorEntity(editorEntity.get());
        AZ::Entity::ComponentArrayType staticRigidBodyComponents =
            gameEntity->FindComponents(AZ::AzTypeInfo<PhysX::StaticRigidBodyComponent>::Uuid());

        EXPECT_EQ(staticRigidBodyComponents.size(), 1);
    }

    TEST_F(PhysXEditorFixture, StaticRigidBodyComponent_EditorColliderAndNoRigidBodyComponent_EditorStaticRigidBodyCreated)
    {
        // Get current number of static rigid body actors in editor world
        const int originalStaticRigidBodyCount = GetEditorStaticRigidBodyCount();

        // Create editor entity
        EntityPtr editorEntity = CreateInactiveEditorEntity("Entity");
        editorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        AddEditorBoxShapeComponent(editorEntity);

        editorEntity->Activate();

        // Verify number of static rigid body actors increased by 1 
        EXPECT_EQ(GetEditorStaticRigidBodyCount(), originalStaticRigidBodyCount + 1);
    }

    TEST_F(PhysXEditorFixture, StaticRigidBodyComponent_EditorColliderAndRigidBodyComponent_NoEditorStaticRigidBodyCreated)
    {
        // Get current number of static rigid body actors in editor world
        const int originalStaticRigidBodyCount = GetEditorStaticRigidBodyCount();

        // Create editor entity
        EntityPtr editorEntity = CreateInactiveEditorEntity("Entity");
        editorEntity->CreateComponent<PhysX::EditorShapeColliderComponent>();
        AddEditorBoxShapeComponent(editorEntity);

        editorEntity->CreateComponent<PhysX::EditorRigidBodyComponent>();

        editorEntity->Activate();

        // Verify number of static rigid body actors has not changed
        EXPECT_EQ(GetEditorStaticRigidBodyCount(), originalStaticRigidBodyCount);
    }
}
