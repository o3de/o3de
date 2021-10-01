/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/FocusMode/EditorFocusModeFixture.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/string/string.h>

#include <AzFramework/Viewport/ViewportScreen.h>

#include <AzManipulatorTestFramework/AzManipulatorTestFramework.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkTestHelpers.h>
#include <AzManipulatorTestFramework/DirectManipulatorViewportInteraction.h>
#include <AzManipulatorTestFramework/ImmediateModeActionDispatcher.h>
#include <AzManipulatorTestFramework/IndirectManipulatorViewportInteraction.h>

#include <AzToolsFramework/Component/EditorComponentAPIBus.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/ViewportSelection/EditorVisibleEntityDataCache.h>


namespace AzToolsFramework
{
    class EditorFocusModeSelectionFixture
        : public UnitTest::IndirectCallManipulatorViewportInteractionFixtureMixin<EditorFocusModeFixture>
    {
    public:
        void ClickAtWorldPositionOnViewport(const AZ::Vector3& worldPosition)
        {
            // Calculate the world position in screen space
            const auto carScreenPosition = AzFramework::WorldToScreen(worldPosition, m_cameraState);

            // Click the entity in the viewport
            m_actionDispatcher->CameraState(m_cameraState)->MousePosition(carScreenPosition)->MouseLButtonDown()->MouseLButtonUp();
        }
    };

    void ClearSelectedEntities()
    {
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequestBus::Events::SetSelectedEntities, AzToolsFramework::EntityIdList());
    }

    AzToolsFramework::EntityIdList GetSelectedEntities()
    {
        AzToolsFramework::EntityIdList selectedEntities;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            selectedEntities, &AzToolsFramework::ToolsApplicationRequestBus::Events::GetSelectedEntities);
        return selectedEntities;
    }

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
    }
}
