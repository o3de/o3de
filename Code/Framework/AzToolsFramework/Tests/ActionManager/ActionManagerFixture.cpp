/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/ActionManager/ActionManagerFixture.h>

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInternalInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInternalInterface.h>
#include <AzToolsFramework/Editor/ActionManagerUtils.h>

namespace UnitTest
{
    void ActionManagerFixture::SetUp()
    {
        LeakDetectionFixture::SetUp();

        m_mainWindow = new QMainWindow();
        m_defaultParentWidget = new QWidget();
        m_widget = new QWidget(m_defaultParentWidget);

        m_actionManager = AZStd::make_unique<AzToolsFramework::ActionManager>();
        m_actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        ASSERT_TRUE(m_actionManagerInterface != nullptr);
        m_actionManagerInternalInterface = AZ::Interface<AzToolsFramework::ActionManagerInternalInterface>::Get();
        ASSERT_TRUE(m_actionManagerInternalInterface != nullptr);

        m_hotKeyManager = AZStd::make_unique<AzToolsFramework::HotKeyManager>();
        m_hotKeyManagerInterface = AZ::Interface<AzToolsFramework::HotKeyManagerInterface>::Get();
        ASSERT_TRUE(m_hotKeyManagerInterface != nullptr);

        m_menuManager = AZStd::make_unique<AzToolsFramework::MenuManager>(m_defaultParentWidget);
        m_menuManagerInterface = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();
        ASSERT_TRUE(m_menuManagerInterface != nullptr);
        m_menuManagerInternalInterface = AZ::Interface<AzToolsFramework::MenuManagerInternalInterface>::Get();
        ASSERT_TRUE(m_menuManagerInternalInterface != nullptr);

        m_toolBarManager = AZStd::make_unique<AzToolsFramework::ToolBarManager>(m_defaultParentWidget);
        m_toolBarManagerInterface = AZ::Interface<AzToolsFramework::ToolBarManagerInterface>::Get();
        ASSERT_TRUE(m_toolBarManagerInterface != nullptr);
        m_toolBarManagerInternalInterface = AZ::Interface<AzToolsFramework::ToolBarManagerInternalInterface>::Get();
        ASSERT_TRUE(m_toolBarManagerInternalInterface != nullptr);
    }

    void ActionManagerFixture::TearDown()
    {
        delete m_defaultParentWidget;

        LeakDetectionFixture::TearDown();
    }

} // namespace UnitTest
