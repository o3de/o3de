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
#include <AzToolsFramework/Prefab/PrefabEditorPreferences.h>

namespace UnitTest
{
    class EditorFocusModeSelectionFixture : public IndirectCallManipulatorViewportInteractionFixtureMixin<EditorFocusModeFixture>
    {
    public:
        void SetUpEditorFixtureImpl() override
        {
            AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();
            m_formerOutlinerOverrideSetting = AzToolsFramework::Prefab::IsOutlinerOverrideManagementEnabled();
            registry->Set("/O3DE/Autoexec/ConsoleCommands/ed_enableOutlinerOverrideManagement", true);
            IndirectCallManipulatorViewportInteractionFixtureMixin<EditorFocusModeFixture>::SetUpEditorFixtureImpl();
            m_viewportManipulatorInteraction->GetViewportInteraction().SetIconsVisible(false);
        }

        void TearDownEditorFixtureImpl() override
        {
            AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();
            IndirectCallManipulatorViewportInteractionFixtureMixin<EditorFocusModeFixture>::TearDownEditorFixtureImpl();
            registry->Set("/O3DE/Autoexec/ConsoleCommands/ed_enableOutlinerOverrideManagement", m_formerOutlinerOverrideSetting);
        }

        void ClickAtWorldPositionOnViewport(const AZ::Vector3& worldPosition)
        {
            // Calculate the world position in screen space
            const auto carScreenPosition = AzFramework::WorldToScreen(worldPosition, m_cameraState);

            // Click the entity in the viewport
            m_actionDispatcher->CameraState(m_cameraState)->MousePosition(carScreenPosition)->MouseLButtonDown()->MouseLButtonUp();
        }

        void BoxSelectOnViewport()
        {
            // Calculate the position in screen space of where to begin and end the box select action
            const auto beginningPositionWorldBoxSelect = AzFramework::WorldToScreen(AZ::Vector3(-10.0f, 15.0f, 5.0f), m_cameraState);
            const auto endingPositionWorldBoxSelect = AzFramework::WorldToScreen(AZ::Vector3(10.0f, 15.0f, -5.0f), m_cameraState);

            // Perform a box select in the viewport
            m_actionDispatcher->SetStickySelect(true)
                ->CameraState(m_cameraState)
                ->MousePosition(beginningPositionWorldBoxSelect)
                ->MouseLButtonDown()
                ->MousePosition(endingPositionWorldBoxSelect)
                ->MouseLButtonUp();
        }

    protected:
        bool m_formerOutlinerOverrideSetting = false;
    };
} // namespace UnitTest
