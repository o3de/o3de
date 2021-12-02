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
    TEST_F(EditorFocusModeSelectionFixture, ContainerEntitySelectionTests_FindHighestSelectableEntityWithNoContainers)
    {
        // When no containers are in the way, the function will just return the entityId of the entity that was clicked.
        
        // Click on Car Entity
        ClickAtWorldPositionOnViewport(WorldCarEntityPosition);

        // Verify the correct entity is selected
        auto selectedEntitiesAfter = GetSelectedEntities();
        EXPECT_EQ(selectedEntitiesAfter.size(), 1);
        EXPECT_EQ(selectedEntitiesAfter.front(), m_entityMap[CarEntityName]);
    }

    TEST_F(EditorFocusModeSelectionFixture, ContainerEntitySelectionTests_FindHighestSelectableEntityWithClosedContainer)
    {
        // If a closed container is an ancestor of the queried entity, the closed container is selected.
        m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[StreetEntityName]); // Containers are closed by default

        // Click on Car Entity
        ClickAtWorldPositionOnViewport(WorldCarEntityPosition);

        // Verify the correct entity is selected
        auto selectedEntitiesAfter = GetSelectedEntities();
        EXPECT_EQ(selectedEntitiesAfter.size(), 1);
        EXPECT_EQ(selectedEntitiesAfter.front(), m_entityMap[StreetEntityName]);

        // Restore default state for other tests.
        m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[StreetEntityName]);
    }

    TEST_F(EditorFocusModeSelectionFixture, ContainerEntitySelectionTests_FindHighestSelectableEntityWithOpenContainer)
    {
        // If a closed container is an ancestor of the queried entity, the closed container is selected.
        m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[StreetEntityName]);
        m_containerEntityInterface->SetContainerOpen(m_entityMap[StreetEntityName], true);

        // Click on Car Entity
        ClickAtWorldPositionOnViewport(WorldCarEntityPosition);

        // Verify the correct entity is selected
        auto selectedEntitiesAfter = GetSelectedEntities();
        EXPECT_EQ(selectedEntitiesAfter.size(), 1);
        EXPECT_EQ(selectedEntitiesAfter.front(), m_entityMap[CarEntityName]);

        // Restore default state for other tests.
        m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[StreetEntityName]);
    }

    TEST_F(EditorFocusModeSelectionFixture, ContainerEntitySelectionTests_FindHighestSelectableEntityWithMultipleClosedContainers)
    {
        // If multiple closed containers are ancestors of the queried entity, the highest closed container is selected.
        m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[StreetEntityName]);
        m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[CityEntityName]);

        // Click on Car Entity
        ClickAtWorldPositionOnViewport(WorldCarEntityPosition);

        // Verify the correct entity is selected
        auto selectedEntitiesAfter = GetSelectedEntities();
        EXPECT_EQ(selectedEntitiesAfter.size(), 1);
        EXPECT_EQ(selectedEntitiesAfter.front(), m_entityMap[CityEntityName]);

        // Restore default state for other tests.
        m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[StreetEntityName]);
        m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[CityEntityName]);
    }

    TEST_F(EditorFocusModeSelectionFixture, ContainerEntitySelectionTests_FindHighestSelectableEntityWithMultipleContainers)
    {
        // If multiple containers are ancestors of the queried entity, the highest closed container is selected.
        m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[StreetEntityName]);
        m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[CityEntityName]);
        m_containerEntityInterface->SetContainerOpen(m_entityMap[CityEntityName], true);

        // Click on Car Entity
        ClickAtWorldPositionOnViewport(WorldCarEntityPosition);

        // Verify the correct entity is selected
        auto selectedEntitiesAfter = GetSelectedEntities();
        EXPECT_EQ(selectedEntitiesAfter.size(), 1);
        EXPECT_EQ(selectedEntitiesAfter.front(), m_entityMap[StreetEntityName]);

        // Restore default state for other tests.
        m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[StreetEntityName]);
        m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[CityEntityName]);
    }
}
