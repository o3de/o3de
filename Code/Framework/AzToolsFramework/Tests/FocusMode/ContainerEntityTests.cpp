/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/FocusMode/EditorFocusModeFixture.h>

namespace UnitTest
{
    TEST_F(EditorFocusModeFixture, ContainerEntityRegister)
    {
        // Registering an entity is successful.
        auto outcome = m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[CarEntityName]);
        EXPECT_TRUE(outcome.IsSuccess());

        // Restore default state for other tests.
        m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[CarEntityName]);
    }

    TEST_F(EditorFocusModeFixture, ContainerEntityRegisterTwice)
    {
        // Registering an entity twice fails.
        m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[CarEntityName]);
        auto outcome = m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[CarEntityName]);
        EXPECT_FALSE(outcome.IsSuccess());

        // Restore default state for other tests.
        m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[CarEntityName]);
    }

    TEST_F(EditorFocusModeFixture, ContainerEntityUnregister)
    {
        // Unregistering a container entity is successful.
        m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[CarEntityName]);
        auto outcome = m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[CarEntityName]);
        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(EditorFocusModeFixture, ContainerEntityUnregisterRegularEntity)
    {
        // Unregistering an entity that was not previously registered fails.
        auto outcome = m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[CarEntityName]);
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(EditorFocusModeFixture, ContainerEntityUnregisterTwice)
    {
        // Unregistering a container entity twice fails.
        auto outcome = m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[CarEntityName]);
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(EditorFocusModeFixture, ContainerEntityIsContainerOnRegularEntity)
    {
        // If a regular entity is passed, IsContainer returns false.
        // Note that we use a different entity than the tests above to validate a completely new EntityId.
        bool isContainer = m_containerEntityInterface->IsContainer(m_entityMap[SportsCarEntityName]);
        EXPECT_FALSE(isContainer);
    }

    TEST_F(EditorFocusModeFixture, ContainerEntityIsContainerOnRegisteredContainer)
    {
        // If a container entity is passed, IsContainer returns true.
        m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[SportsCarEntityName]);
        bool isContainer = m_containerEntityInterface->IsContainer(m_entityMap[SportsCarEntityName]);
        EXPECT_TRUE(isContainer);

        // Restore default state for other tests.
        m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[SportsCarEntityName]);
    }

    TEST_F(EditorFocusModeFixture, ContainerEntityIsContainerOnUnRegisteredContainer)
    {
        // If an entity that was previously a container but was then unregistered is passed, IsContainer returns false.
        m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[SportsCarEntityName]);
        m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[SportsCarEntityName]);

        bool isContainer = m_containerEntityInterface->IsContainer(m_entityMap[SportsCarEntityName]);
        EXPECT_FALSE(isContainer);
    }

    TEST_F(EditorFocusModeFixture, ContainerEntitySetContainerOpenOnRegularEntity)
    {
        // Setting a regular entity to open should return a failure.
        auto outcome = m_containerEntityInterface->SetContainerOpen(m_entityMap[StreetEntityName], true);
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(EditorFocusModeFixture, ContainerEntitySetContainerOpen)
    {
        // Set a container entity to open, and verify the operation was successful.
        m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[StreetEntityName]);
        auto outcome = m_containerEntityInterface->SetContainerOpen(m_entityMap[StreetEntityName], true);
        EXPECT_TRUE(outcome.IsSuccess());

        // Restore default state for other tests.
        m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[StreetEntityName]);
    }

    TEST_F(EditorFocusModeFixture, ContainerEntitySetContainerOpenTwice)
    {
        // Set a container entity to open twice, and verify that does not cause a failure (as intended).
        m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[StreetEntityName]);
        m_containerEntityInterface->SetContainerOpen(m_entityMap[StreetEntityName], true);
        auto outcome = m_containerEntityInterface->SetContainerOpen(m_entityMap[StreetEntityName], true);
        EXPECT_TRUE(outcome.IsSuccess());

        // Restore default state for other tests.
        m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[StreetEntityName]);
    }

    TEST_F(EditorFocusModeFixture, ContainerEntitySetContainerClosed)
    {
        // Set a container entity to closed, and verify the operation was successful.
        m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[StreetEntityName]);
        auto outcome = m_containerEntityInterface->SetContainerOpen(m_entityMap[StreetEntityName], true);
        EXPECT_TRUE(outcome.IsSuccess());

        // Restore default state for other tests.
        m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[StreetEntityName]);
    }

    TEST_F(EditorFocusModeFixture, ContainerEntityIsContainerOpenOnRegularEntity)
    {
        // Query open state on a regular entity, and verify it returns true.
        // Open containers behave exactly as regular entities, so this is the expected return value.
        bool isOpen = m_containerEntityInterface->IsContainerOpen(m_entityMap[CityEntityName]);
        EXPECT_TRUE(isOpen);
    }

    TEST_F(EditorFocusModeFixture, ContainerEntityIsContainerOpenOnDefaultContainerEntity)
    {
        // Query open state on a newly registered container entity, and verify it returns false.
        // Containers are registered closed by default.
        m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[CityEntityName]);
        bool isOpen = m_containerEntityInterface->IsContainerOpen(m_entityMap[CityEntityName]);
        EXPECT_FALSE(isOpen);

        // Restore default state for other tests.
        m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[CityEntityName]);
    }

    TEST_F(EditorFocusModeFixture, ContainerEntityIsContainerOpenOnOpenContainerEntity)
    {
        // Query open state on a container entity that was opened, and verify it returns true.
        m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[CityEntityName]);
        m_containerEntityInterface->SetContainerOpen(m_entityMap[CityEntityName], true);
        bool isOpen = m_containerEntityInterface->IsContainerOpen(m_entityMap[CityEntityName]);
        EXPECT_TRUE(isOpen);

        // Restore default state for other tests.
        m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[CityEntityName]);
    }

    TEST_F(EditorFocusModeFixture, ContainerEntityIsContainerOpenOnClosedContainerEntity)
    {
        // Query open state on a container entity that was opened and then closed, and verify it returns false.
        m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[CityEntityName]);
        m_containerEntityInterface->SetContainerOpen(m_entityMap[CityEntityName], true);
        m_containerEntityInterface->SetContainerOpen(m_entityMap[CityEntityName], false);
        bool isOpen = m_containerEntityInterface->IsContainerOpen(m_entityMap[CityEntityName]);
        EXPECT_FALSE(isOpen);

        // Restore default state for other tests.
        m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[CityEntityName]);
    }

    TEST_F(EditorFocusModeFixture, ContainerEntityContainerOpenStateIsPreserved)
    {
        // Register an entity as container, open it, then unregister it.
        // When the entity is registered again, the open state should be preserved.
        // This behavior is necessary for the system to work alongside Prefab propagation refreshes.
        m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[CityEntityName]);
        m_containerEntityInterface->SetContainerOpen(m_entityMap[CityEntityName], true);
        m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[CityEntityName]);

        m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[CityEntityName]);
        bool isOpen = m_containerEntityInterface->IsContainerOpen(m_entityMap[CityEntityName]);
        EXPECT_TRUE(isOpen);

        // Restore default state for other tests.
        m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[CityEntityName]);
    }

    TEST_F(EditorFocusModeFixture, ContainerEntityClearSucceeds)
    {
        // The Clear function works if no container is registered.
        auto outcome = m_containerEntityInterface->Clear(m_editorEntityContextId);
        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(EditorFocusModeFixture, ContainerEntityClearFailsIfContainersAreStillRegistered)
    {
        // The Clear function fails if a container is registered.
        m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[Passenger1EntityName]);
        auto outcome = m_containerEntityInterface->Clear(m_editorEntityContextId);
        EXPECT_FALSE(outcome.IsSuccess());

        // Restore default state for other tests.
        m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[Passenger1EntityName]);
    }

    TEST_F(EditorFocusModeFixture, ContainerEntityClearSucceedsIfContainersAreUnregistered)
    {
        // The Clear function fails if a container is registered.
        m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[Passenger1EntityName]);
        m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[Passenger1EntityName]);
        auto outcome = m_containerEntityInterface->Clear(m_editorEntityContextId);
        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(EditorFocusModeFixture, ContainerEntityClearDeletesPreservedOpenStates)
    {
        // Register an entity as container, open it, unregister it, then call clear.
        // When the entity is registered again, the open state should not be preserved.
        m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[Passenger1EntityName]);
        m_containerEntityInterface->SetContainerOpen(m_entityMap[Passenger1EntityName], true);
        m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[Passenger1EntityName]);

        m_containerEntityInterface->Clear(m_editorEntityContextId);

        m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[Passenger1EntityName]);
        bool isOpen = m_containerEntityInterface->IsContainerOpen(m_entityMap[Passenger1EntityName]);
        EXPECT_FALSE(isOpen);

        // Restore default state for other tests.
        m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[Passenger1EntityName]);
    }

    TEST_F(EditorFocusModeFixture, ContainerEntityFindHighestSelectableEntityWithNoContainers)
    {
        // When no containers are in the way, the function will just return the entityId that was passed to it.
        AZ::EntityId selectedEntityId = m_containerEntityInterface->FindHighestSelectableEntity(m_entityMap[Passenger2EntityName]);
        EXPECT_EQ(selectedEntityId, m_entityMap[Passenger2EntityName]);
    }

    TEST_F(EditorFocusModeFixture, ContainerEntityFindHighestSelectableEntityWithClosedContainer)
    {
        // If a closed container is an ancestor of the queried entity, the closed container is selected.
        m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[SportsCarEntityName]); // Containers are closed by default
        AZ::EntityId selectedEntityId = m_containerEntityInterface->FindHighestSelectableEntity(m_entityMap[Passenger2EntityName]);
        EXPECT_EQ(selectedEntityId, m_entityMap[SportsCarEntityName]);

        // Restore default state for other tests.
        m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[SportsCarEntityName]);
    }

    TEST_F(EditorFocusModeFixture, ContainerEntityFindHighestSelectableEntityWithOpenContainer)
    {
        // If an open container is an ancestor of the queried entity, it is ignored.
        m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[SportsCarEntityName]);
        m_containerEntityInterface->SetContainerOpen(m_entityMap[SportsCarEntityName], true);

        AZ::EntityId selectedEntityId = m_containerEntityInterface->FindHighestSelectableEntity(m_entityMap[Passenger2EntityName]);
        EXPECT_EQ(selectedEntityId, m_entityMap[SportsCarEntityName]);

        // Restore default state for other tests.
        m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[SportsCarEntityName]);
    }

    TEST_F(EditorFocusModeFixture, ContainerEntityFindHighestSelectableEntityWithMultipleClosedContainers)
    {
        // If multiple closed containers are ancestors of the queried entity, the highest closed container is selected.
        m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[StreetEntityName]);
        m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[SportsCarEntityName]);

        AZ::EntityId selectedEntityId = m_containerEntityInterface->FindHighestSelectableEntity(m_entityMap[Passenger2EntityName]);
        EXPECT_EQ(selectedEntityId, m_entityMap[StreetEntityName]);

        // Restore default state for other tests.
        m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[StreetEntityName]);
        m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[SportsCarEntityName]);
    }

    TEST_F(EditorFocusModeFixture, ContainerEntityFindHighestSelectableEntityWithMultipleContainers)
    {
        // If multiple containers are ancestors of the queried entity, the highest closed container is selected.
        m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[StreetEntityName]);
        m_containerEntityInterface->RegisterEntityAsContainer(m_entityMap[SportsCarEntityName]);
        m_containerEntityInterface->SetContainerOpen(m_entityMap[StreetEntityName], true);

        AZ::EntityId selectedEntityId = m_containerEntityInterface->FindHighestSelectableEntity(m_entityMap[Passenger2EntityName]);
        EXPECT_EQ(selectedEntityId, m_entityMap[StreetEntityName]);

        // Restore default state for other tests.
        m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[StreetEntityName]);
        m_containerEntityInterface->UnregisterEntityAsContainer(m_entityMap[SportsCarEntityName]);
    }
} // namespace UnitTest
