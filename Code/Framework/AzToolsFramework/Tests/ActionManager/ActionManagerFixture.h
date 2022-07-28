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
#include <AzToolsFramework/ActionManager/Menu/MenuManager.h>
#include <AzToolsFramework/ActionManager/ToolBar/ToolBarManager.h>

#include <QWidget>

namespace UnitTest
{
    class ActionManagerFixture : public AllocatorsTestFixture
    {
    protected:
        void SetUp() override;
        void TearDown() override;

    public:
        AzToolsFramework::ActionManagerInterface* m_actionManagerInterface = nullptr;
        AzToolsFramework::ActionManagerInternalInterface* m_actionManagerInternalInterface = nullptr;
        AzToolsFramework::MenuManagerInterface* m_menuManagerInterface = nullptr;
        AzToolsFramework::MenuManagerInternalInterface* m_menuManagerInternalInterface = nullptr;
        AzToolsFramework::ToolBarManagerInterface* m_toolBarManagerInterface = nullptr;
        AzToolsFramework::ToolBarManagerInternalInterface* m_toolBarManagerInternalInterface = nullptr;

        QWidget* m_widget = nullptr;

    private:
        AZStd::unique_ptr<AzToolsFramework::ActionManager> m_actionManager;
        AZStd::unique_ptr<AzToolsFramework::MenuManager> m_menuManager;
        AZStd::unique_ptr<AzToolsFramework::ToolBarManager> m_toolBarManager;
    };

} // namespace UnitTest
