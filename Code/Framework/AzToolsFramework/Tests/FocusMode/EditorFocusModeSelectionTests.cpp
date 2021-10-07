/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/FocusMode/EditorFocusModeSelectionFixture.h>

namespace AzToolsFramework
{
    TEST_F(EditorFocusModeSelectionFixture, EditorFocusModeSelectionTests_SelectEntityWithFocusOnLevel)
    {
        // Clear the focus, disabling focus mode
        m_focusModeInterface->ClearFocusRoot(AzFramework::EntityContextId::CreateNull());
        // Clear selection
        ClearSelectedEntities();

        // Click on Car Entity
        ClickAtWorldPositionOnViewport(CarEntityPosition);

        // Verify entity is selected
        auto selectedEntitiesAfter = GetSelectedEntities();
        EXPECT_EQ(selectedEntitiesAfter.size(), 1);
        EXPECT_EQ(selectedEntitiesAfter.front(), m_entityMap[CarEntityName]);

        // Restore default state for other tests.
        ClearSelectedEntities();
    }

    TEST_F(EditorFocusModeSelectionFixture, EditorFocusModeSelectionTests_SelectEntityWithFocusOnAncestor)
    {
        // Set the focus on the Street Entity (parent of the test entity)
        m_focusModeInterface->SetFocusRoot(m_entityMap[StreetEntityName]);
        // Clear selection
        ClearSelectedEntities();

        // Click on Car Entity
        ClickAtWorldPositionOnViewport(CarEntityPosition);

        // Verify entity is selected
        auto selectedEntitiesAfter = GetSelectedEntities();
        EXPECT_EQ(selectedEntitiesAfter.size(), 1);
        EXPECT_EQ(selectedEntitiesAfter.front(), m_entityMap[CarEntityName]);

        // Clear the focus, disabling focus mode
        m_focusModeInterface->ClearFocusRoot(AzFramework::EntityContextId::CreateNull());

        // Restore default state for other tests.
        ClearSelectedEntities();
    }

    TEST_F(EditorFocusModeSelectionFixture, EditorFocusModeSelectionTests_SelectEntityWithFocusOnItself)
    {
        // Set the focus on the Car Entity (test entity)
        m_focusModeInterface->SetFocusRoot(m_entityMap[CarEntityName]);
        // Clear selection
        ClearSelectedEntities();

        // Click on Car Entity
        ClickAtWorldPositionOnViewport(CarEntityPosition);

        // Verify entity is selected
        auto selectedEntitiesAfter = GetSelectedEntities();
        EXPECT_EQ(selectedEntitiesAfter.size(), 1);
        EXPECT_EQ(selectedEntitiesAfter.front(), m_entityMap[CarEntityName]);

        // Clear the focus, disabling focus mode
        m_focusModeInterface->ClearFocusRoot(AzFramework::EntityContextId::CreateNull());

        // Restore default state for other tests.
        ClearSelectedEntities();
    }

    TEST_F(EditorFocusModeSelectionFixture, EditorFocusModeSelectionTests_SelectEntityWithFocusOnSibling)
    {
        // Set the focus on the SportsCar Entity (sibling of the test entity)
        m_focusModeInterface->SetFocusRoot(m_entityMap[SportsCarEntityName]);
        // Clear selection
        ClearSelectedEntities();

        // Click on Car Entity
        ClickAtWorldPositionOnViewport(CarEntityPosition);

        // Verify entity is selected
        auto selectedEntitiesAfter = GetSelectedEntities();
        EXPECT_EQ(selectedEntitiesAfter.size(), 0);

        // Clear the focus, disabling focus mode
        m_focusModeInterface->ClearFocusRoot(AzFramework::EntityContextId::CreateNull());

        // Restore default state for other tests.
        ClearSelectedEntities();
    }

    TEST_F(EditorFocusModeSelectionFixture, EditorFocusModeSelectionTests_SelectEntityWithFocusOnDescendant)
    {
        // Set the focus on the Passenger1 Entity (child of the entity)
        m_focusModeInterface->SetFocusRoot(m_entityMap[Passenger1EntityName]);
        // Clear selection
        ClearSelectedEntities();

        // Click on Car Entity
        ClickAtWorldPositionOnViewport(CarEntityPosition);

        // Verify entity is selected
        auto selectedEntitiesAfter = GetSelectedEntities();
        EXPECT_EQ(selectedEntitiesAfter.size(), 0);

        // Clear the focus, disabling focus mode
        m_focusModeInterface->ClearFocusRoot(AzFramework::EntityContextId::CreateNull());

        // Restore default state for other tests.
        ClearSelectedEntities();
    }
}
