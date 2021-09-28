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
    using EditorFocusModeSelectionFixture = UnitTest::IndirectCallManipulatorViewportInteractionFixtureMixin<EditorFocusModeFixture>;

    TEST_F(EditorFocusModeSelectionFixture, EditorFocusModeSelectionTests_SelectEntity)
    {
        auto selectedEntitiesBefore = GetSelectedEntities();
        EXPECT_TRUE(selectedEntitiesBefore.empty());

        // calculate the position in screen space of the initial entity position
        const auto carScreenPosition = AzFramework::WorldToScreen(CarEntityPosition, m_cameraState);

        // click the entity in the viewport
        m_actionDispatcher->SetStickySelect(true)
            ->CameraState(m_cameraState)
            ->MousePosition(carScreenPosition)
            ->MouseLButtonDown()
            ->MouseLButtonUp();

        // entity is selected
        auto selectedEntitiesAfter = GetSelectedEntities();
        EXPECT_EQ(selectedEntitiesAfter.size(), 1);
        EXPECT_EQ(selectedEntitiesAfter.front(), m_entityMap[CarEntityName]);
    }
}
