/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Transform.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ViewportUi/ViewportUiDisplay.h>

namespace UnitTest
{
    using ButtonGroup = AzToolsFramework::ViewportUi::Internal::ButtonGroup;
    using ButtonId = AzToolsFramework::ViewportUi::ButtonId;

    TEST(ClusterTest, AddButtonAddsButtonToClusterAndReturnsId)
    {
        auto buttonGroup = AZStd::make_unique<ButtonGroup>();
        auto buttonId = buttonGroup->AddButton("");

        auto button = buttonGroup->GetButton(buttonId);
        EXPECT_TRUE(button != nullptr);
    }

    TEST(ClusterTest, SetHighlightedButtonChangesButtonStateToSelected)
    {
        auto buttonGroup = AZStd::make_unique<ButtonGroup>();
        auto buttonId = buttonGroup->AddButton("");

        // check button is not highlighted by default
        auto button = buttonGroup->GetButton(buttonId);
        EXPECT_FALSE(button->m_state == AzToolsFramework::ViewportUi::Internal::Button::State::Selected);

        buttonGroup->SetHighlightedButton(buttonId);
        EXPECT_TRUE(button->m_state == AzToolsFramework::ViewportUi::Internal::Button::State::Selected);
    }

    TEST(ClusterTest, ConnectEventHandlerConnectsHandlerToButtonTriggeredEvent)
    {
        auto buttonGroup = AZStd::make_unique<ButtonGroup>();
        auto buttonId = buttonGroup->AddButton("");

        // create a handler which will be triggered by the cluster
        bool handlerTriggered = false;
        auto testButtonId = ButtonId(buttonId);
        AZ::Event<ButtonId>::Handler handler(
            [&handlerTriggered, testButtonId](ButtonId buttonId)
            {
                if (buttonId == testButtonId)
                {
                    handlerTriggered = true;
                }
            });

        buttonGroup->ConnectEventHandler(handler);
        buttonGroup->PressButton(buttonId);

        EXPECT_TRUE(handlerTriggered);
    }
} // namespace UnitTest
