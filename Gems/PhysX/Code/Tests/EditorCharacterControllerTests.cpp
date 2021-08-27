/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/EditorTestUtilities.h>
#include <EditorRigidBodyComponent.h>
#include <PhysXCharacters/Components/EditorCharacterControllerComponent.h>

namespace PhysXEditorTests
{
    TEST_F(PhysXEditorFixture, EditorCharacterControllerComponent_CharacterControllerPlusRigidBodyComponents_EntityIsInvalid)
    {
        EntityPtr entity = CreateInactiveEditorEntity("ShapeColliderComponentEditorEntity");
        entity->CreateComponent<PhysX::EditorCharacterControllerComponent>();
        entity->CreateComponent<PhysX::EditorRigidBodyComponent>();

        // the entity should be in an invalid state because the character controller is incompatible with the rigid body
        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_FALSE(sortOutcome.IsSuccess());
        EXPECT_TRUE(sortOutcome.GetError().m_code == AZ::Entity::DependencySortResult::HasIncompatibleServices);
    }
} // namespace PhysXEditorTests
