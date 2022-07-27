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
#include <AzToolsFramework/ViewportUi/ViewportUiSwitcher.h>

#include <QAction>
#include <QApplication>
#include <QKeyEvent>
#include <QWidget>

namespace UnitTest
{
    using ViewportUiSwitcher = AzToolsFramework::ViewportUi::Internal::ViewportUiSwitcher;
    using ButtonGroup = AzToolsFramework::ViewportUi::Internal::ButtonGroup;
    using Button = AzToolsFramework::ViewportUi::Internal::Button;
    using ButtonId = AzToolsFramework::ViewportUi::ButtonId;

    TEST(ViewportUiSwitcher, AddButtonIncreasesSwitcherWidth)
    {
        auto buttonGroup = AZStd::make_shared<ButtonGroup>();
        ViewportUiSwitcher viewportUiSwitcher(buttonGroup);
        viewportUiSwitcher.resize(viewportUiSwitcher.minimumSizeHint());

        // need to initialize cluster with a single button or size will be invalid
        viewportUiSwitcher.AddButton(AZStd::make_unique<Button>("", ButtonId(3)).get());
        viewportUiSwitcher.SetActiveButton(ButtonId(3));
        QSize initialSize = viewportUiSwitcher.size();

        // add a second button to increase the size
        viewportUiSwitcher.AddButton(AZStd::make_unique<Button>("", ButtonId(4)).get());
        viewportUiSwitcher.AddButton(AZStd::make_unique<Button>("", ButtonId(5)).get());
        QSize finalSize = viewportUiSwitcher.size();

        bool sizeIncrease = initialSize.height() == finalSize.height() && initialSize.width() < finalSize.width();
        AZ_Printf("test", "initialSize witdh %i finalSize witdth %i", initialSize.width(), finalSize.width())
        EXPECT_TRUE(sizeIncrease);
    }

    TEST(ViewportUiSwitcher, RemoveClusterButtonDecreasesSwitcherWidth)
    {
        auto buttonGroup = AZStd::make_shared<ButtonGroup>();
        ViewportUiSwitcher ViewportUiSwitcher(buttonGroup);
        ViewportUiSwitcher.resize(ViewportUiSwitcher.minimumSizeHint());

        // need to initialize cluster with a single button or size will be invalid
        ViewportUiSwitcher.AddButton(AZStd::make_unique<Button>("", ButtonId(1)).get());

        // add a second button to increase the size
        ViewportUiSwitcher.AddButton(AZStd::make_unique<Button>("", ButtonId(2)).get());
        QSize initialSize = ViewportUiSwitcher.size();

        // remove the second button
        ViewportUiSwitcher.RemoveButton(ButtonId(1));
        QSize finalSize = ViewportUiSwitcher.size();

        bool sizeDecrease = initialSize.height() == finalSize.height() && initialSize.width() > finalSize.width();

        EXPECT_TRUE(sizeDecrease);
    }

} // namespace UnitTest
