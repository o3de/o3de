/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

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
    class EditorFocusModeSelectionFixture : public UnitTest::IndirectCallManipulatorViewportInteractionFixtureMixin<EditorFocusModeFixture>
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
} // namespace AzToolsFramework
