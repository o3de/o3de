/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/ActionManager/ActionManagerFixture.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace UnitTest
{
    TEST_F(ActionManagerFixture, RegisterMenu)
    {
        auto outcome =
            m_menuManagerInterface->RegisterMenu("o3de.menu.test", "Test");
        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, RegisterMenuTwice)
    {
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", "Test");
        auto outcome = m_menuManagerInterface->RegisterMenu("o3de.menu.test", "Test");
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, AddActionToUnregisteredMenu)
    {
        m_actionManagerInterface->RegisterActionContext(m_widget, "o3de.context.test", "Test", "");

        m_actionManagerInterface->RegisterAction(
            "o3de.context.test", "o3de.action.test", "Test Action", "Executes Test Action", "Test", "",
            []()
            {
                ;
            }
        );

        auto outcome = m_menuManagerInterface->AddActionToMenu("o3de.action.test", "o3de.menu.test", 42);
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, AddActionToMenu)
    {
        m_actionManagerInterface->RegisterActionContext(m_widget, "o3de.context.test", "Test", "");

        m_actionManagerInterface->RegisterAction(
            "o3de.context.test", "o3de.action.test", "Test Action", "Executes Test Action", "Test", "",
            []()
            {
                ;
            }
        );

        m_menuManagerInterface->RegisterMenu("o3de.menu.test", "Test");

        auto outcome = m_menuManagerInterface->AddActionToMenu("o3de.action.test", "o3de.menu.test", 42);
        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, GetUnregisteredMenu)
    {
        QMenu* menu = m_menuManagerInterface->GetMenu("o3de.menu.test");
        EXPECT_TRUE(menu == nullptr);
    }

    TEST_F(ActionManagerFixture, GetMenu)
    {
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", "Test");

        QMenu* menu = m_menuManagerInterface->GetMenu("o3de.menu.test");
        EXPECT_TRUE(menu != nullptr);
    }

} // namespace UnitTest
