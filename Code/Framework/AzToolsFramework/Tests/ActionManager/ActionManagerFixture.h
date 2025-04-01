/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

#include <AzTest/AzTest.h>

#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

#include <AzToolsFramework/ActionManager/Action/ActionManager.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManager.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManager.h>
#include <AzToolsFramework/ActionManager/ToolBar/ToolBarManager.h>

#include <QMainWindow>
#include <QWidget>

namespace UnitTest
{
    class ActionManagerFixture : public LeakDetectionFixture
    {
    protected:
        void SetUp() override;
        void TearDown() override;

    public:
        AzToolsFramework::ActionManagerInterface* m_actionManagerInterface = nullptr;
        AzToolsFramework::ActionManagerInternalInterface* m_actionManagerInternalInterface = nullptr;
        AzToolsFramework::HotKeyManagerInterface* m_hotKeyManagerInterface = nullptr;
        AzToolsFramework::MenuManagerInterface* m_menuManagerInterface = nullptr;
        AzToolsFramework::MenuManagerInternalInterface* m_menuManagerInternalInterface = nullptr;
        AzToolsFramework::ToolBarManagerInterface* m_toolBarManagerInterface = nullptr;
        AzToolsFramework::ToolBarManagerInternalInterface* m_toolBarManagerInternalInterface = nullptr;

        QMainWindow* m_mainWindow = nullptr;
        QWidget* m_widget = nullptr;
        QWidget* m_defaultParentWidget = nullptr;

    private:
        AZStd::unique_ptr<AzToolsFramework::ActionManager> m_actionManager;
        AZStd::unique_ptr<AzToolsFramework::HotKeyManager> m_hotKeyManager;
        AZStd::unique_ptr<AzToolsFramework::MenuManager> m_menuManager;
        AZStd::unique_ptr<AzToolsFramework::ToolBarManager> m_toolBarManager;
    };

} // namespace UnitTest
