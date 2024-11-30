/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Tests/EditorTestUtilities.h>
#include <EditorRigidBodyComponent.h>
#include <PhysXCharacters/Components/EditorCharacterGameplayComponent.h>
#include <PhysXCharacters/Components/EditorCharacterControllerComponent.h>

namespace PhysXEditorTests
{
    TEST_F(PhysXEditorFixture, EditorGameplayControllerComponent_GameplayControllerWithoutCharacterController_EntityIsInvalid)
    {
        EntityPtr entity = CreateInactiveEditorEntity("ShapeColliderComponentEditorEntity");
        entity->CreateComponent<PhysX::EditorCharacterGameplayComponent>();

        // The entity should be in an invalid state because the Gameplay Controller depends on the Character Controller
        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_FALSE(sortOutcome.IsSuccess());
        EXPECT_TRUE(sortOutcome.GetError().m_code == AZ::Entity::DependencySortResult::MissingRequiredService);
    }

    TEST_F(PhysXEditorFixture, EditorGameplayControllerComponent_GameplayControllerWithCharacterController_EntityIsValid)
    {
        EntityPtr entity = CreateInactiveEditorEntity("ShapeColliderComponentEditorEntity");
        entity->CreateComponent<PhysX::EditorCharacterGameplayComponent>();
        entity->CreateComponent<PhysX::EditorCharacterControllerComponent>();

        // The entity should be in a valid state since the Gameplay Component depends on
        // the Character Controller component and both are present
        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_TRUE(sortOutcome.IsSuccess());
    }
}
