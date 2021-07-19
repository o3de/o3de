/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/ViewportUi/ButtonGroup.h>
#include <AzToolsFramework/ViewportUi/ViewportUiCluster.h>
#include <QAction>
#include <QApplication>
#include <QKeyEvent>
#include <QWidget>

namespace UnitTest
{
    using ViewportUiCluster = AzToolsFramework::ViewportUi::Internal::ViewportUiCluster;
    using ButtonGroup = AzToolsFramework::ViewportUi::Internal::ButtonGroup;
    using Button = AzToolsFramework::ViewportUi::Internal::Button;
    using ButtonId = AzToolsFramework::ViewportUi::ButtonId;

    TEST(ViewportUiCluster, RegisterButtonIncreasesClusterHeight)
    {
        auto buttonGroup = AZStd::make_shared<ButtonGroup>();
        ViewportUiCluster viewportUiCluster(buttonGroup);
        viewportUiCluster.resize(viewportUiCluster.minimumSizeHint());

        // need to initialize cluster with a single button or size will be invalid
        viewportUiCluster.RegisterButton(AZStd::make_unique<Button>("", ButtonId(1)).get());
        QSize initialSize = viewportUiCluster.size();

        // add a second button to increase the size
        viewportUiCluster.RegisterButton(AZStd::make_unique<Button>("", ButtonId(2)).get());
        QSize finalSize = viewportUiCluster.size();

        bool sizeIncrease = initialSize.width() == finalSize.width() && initialSize.height() < finalSize.height();

        EXPECT_TRUE(sizeIncrease);
    }

    TEST(ViewportUiCluster, RemoveClusterButtonDecreasesClusterHeight)
    {
        auto buttonGroup = AZStd::make_shared<ButtonGroup>();
        ViewportUiCluster viewportUiCluster(buttonGroup);
        viewportUiCluster.resize(viewportUiCluster.minimumSizeHint());

        // need to initialize cluster with a single button or size will be invalid
        viewportUiCluster.RegisterButton(AZStd::make_unique<Button>("", ButtonId(1)).get());

        // add a second button to increase the size
        viewportUiCluster.RegisterButton(AZStd::make_unique<Button>("", ButtonId(2)).get());
        QSize initialSize = viewportUiCluster.size();

        // remove the second button
        viewportUiCluster.RemoveButton(ButtonId(1));
        QSize finalSize = viewportUiCluster.size();

        bool sizeDecrease = initialSize.width() == finalSize.width() && initialSize.height() > finalSize.height();

        EXPECT_TRUE(sizeDecrease);
    }

    TEST(ViewportUiCluster, UpdateChangesActiveButton)
    {
        auto buttonGroup = AZStd::make_shared<ButtonGroup>();
        ViewportUiCluster viewportUiCluster(buttonGroup);

        // register a button to the cluster
        auto button = AZStd::make_unique<Button>("", ButtonId(1));
        viewportUiCluster.RegisterButton(button.get());

        // get the action corresponding to the button
        auto widgetCallbacks = viewportUiCluster.GetWidgetCallbacks();
        auto action = static_cast<QAction*>(widgetCallbacks.GetWidgets()[0].data());

        // verify action is not checked by default
        EXPECT_FALSE(action->isChecked());

        // set the button to selected and update the ViewportUiCluster to sync
        button->m_state = AzToolsFramework::ViewportUi::Internal::Button::State::Selected;
        viewportUiCluster.Update();

        EXPECT_TRUE(action->isChecked());
    }

    TEST(ViewportUiCluster, TriggeringActionTriggersClusterEventForButton)
    {
        auto buttonGroup = AZStd::make_shared<ButtonGroup>();
        ViewportUiCluster viewportUiCluster(buttonGroup);

        // create a handler which will be triggered by the button
        bool handlerTriggered = false;
        auto testButtonId = ButtonId(1);
        AZ::Event<ButtonId>::Handler handler(
            [&handlerTriggered, testButtonId](ButtonId buttonId)
            {
                if (buttonId == testButtonId)
                {
                    handlerTriggered = true;
                }
            });
        buttonGroup->ConnectEventHandler(handler);

        // register the button
        auto button = AZStd::make_unique<Button>("", testButtonId);
        viewportUiCluster.RegisterButton(button.get());

        // trigger the action, which should activate the handler
        auto widgetCallbacks = viewportUiCluster.GetWidgetCallbacks();
        auto action = static_cast<QAction*>(widgetCallbacks.GetWidgets()[0].data());
        action->trigger();

        EXPECT_TRUE(handlerTriggered);
    }
} // namespace UnitTest
