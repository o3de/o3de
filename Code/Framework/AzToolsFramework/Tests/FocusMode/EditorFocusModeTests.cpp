/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/FocusMode/EditorFocusModeFixture.h>

namespace AzToolsFramework
{
    TEST_F(EditorFocusModeFixture, EditorFocusModeTests_SetFocus)
    {
        // When an entity is set as the focus root, GetFocusRoot should return its EntityId.
        m_focusModeInterface->SetFocusRoot(m_entityMap[CarEntityName]);
        EXPECT_EQ(m_focusModeInterface->GetFocusRoot(m_editorEntityContextId), m_entityMap[CarEntityName]);

        // Restore default expected focus.
        m_focusModeInterface->ClearFocusRoot(m_editorEntityContextId);
    }

    TEST_F(EditorFocusModeFixture, EditorFocusModeTests_ClearFocus)
    {
        // Change the value from the default.
        m_focusModeInterface->SetFocusRoot(m_entityMap[CarEntityName]);

        // Calling ClearFocusRoot restores the default focus root (which is an invalid EntityId).
        m_focusModeInterface->ClearFocusRoot(m_editorEntityContextId);
        EXPECT_EQ(m_focusModeInterface->GetFocusRoot(m_editorEntityContextId), AZ::EntityId());
    }

    TEST_F(EditorFocusModeFixture, EditorFocusModeTests_IsInFocusSubTree_AncestorsDescendants)
    {
        // When the focus is set to an entity, all its descendants are in the focus subtree while the ancestors aren't.
        m_focusModeInterface->SetFocusRoot(m_entityMap[StreetEntityName]);

        EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[CityEntityName]), false);
        EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[StreetEntityName]), true);
        EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[CarEntityName]), true);
        EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[Passenger1EntityName]), true);
        EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[SportsCarEntityName]), true);
        EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[Passenger2EntityName]), true);
    }

    TEST_F(EditorFocusModeFixture, EditorFocusModeTests_IsInFocusSubTree_Siblings)
    {
        // If the root entity has siblings, they are also outside of the focus subtree.
        m_focusModeInterface->SetFocusRoot(m_entityMap[CarEntityName]);

        EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[CityEntityName]), false);
        EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[StreetEntityName]), false);
        EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[CarEntityName]), true);
        EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[Passenger1EntityName]), true);
        EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[SportsCarEntityName]), false);
        EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[Passenger2EntityName]), false);
    }

    TEST_F(EditorFocusModeFixture, EditorFocusModeTests_IsInFocusSubTree_Leaf)
    {
        // If the root is a leaf, then the focus subtree will consists of just that entity.
        m_focusModeInterface->SetFocusRoot(m_entityMap[Passenger2EntityName]);

        EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[CityEntityName]), false);
        EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[StreetEntityName]), false);
        EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[CarEntityName]), false);
        EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[Passenger1EntityName]), false);
        EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[SportsCarEntityName]), false);
        EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[Passenger2EntityName]), true);
    }

    TEST_F(EditorFocusModeFixture, EditorFocusModeTests_IsInFocusSubTree_Clear)
    {
        // Change the value from the default.
        m_focusModeInterface->SetFocusRoot(m_entityMap[StreetEntityName]);

        // When the focus is cleared, the whole level is in the focus subtree; so we expect all entities to return true.
        m_focusModeInterface->ClearFocusRoot(m_editorEntityContextId);

        EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[CityEntityName]), true);
        EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[StreetEntityName]), true);
        EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[CarEntityName]), true);
        EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[Passenger1EntityName]), true);
        EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[SportsCarEntityName]), true);
        EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[Passenger2EntityName]), true);
    }
}
