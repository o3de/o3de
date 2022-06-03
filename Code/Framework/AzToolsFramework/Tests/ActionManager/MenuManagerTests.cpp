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
        auto outcome = m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});
        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, RegisterMenuTwice)
    {
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});
        auto outcome = m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, AddActionToUnregisteredMenu)
    {
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});

        auto outcome = m_menuManagerInterface->AddActionToMenu("o3de.menu.test", "o3de.action.test", 42);
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, AddActionToMenu)
    {
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});

        auto outcome = m_menuManagerInterface->AddActionToMenu("o3de.menu.test", "o3de.action.test", 42);
        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, GetUnregisteredMenu)
    {
        QMenu* menu = m_menuManagerInterface->GetMenu("o3de.menu.test");
        EXPECT_TRUE(menu == nullptr);
    }

    TEST_F(ActionManagerFixture, GetMenu)
    {
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});

        QMenu* menu = m_menuManagerInterface->GetMenu("o3de.menu.test");
        EXPECT_TRUE(menu != nullptr);
    }

    TEST_F(ActionManagerFixture, VerifyActionInMenu)
    {
        // Register menu, get it and verify it's empty.
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});
        QMenu* menu = m_menuManagerInterface->GetMenu("o3de.menu.test");
        EXPECT_EQ(menu->actions().size(), 0);

        // Register a new action and add it to the menu.
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        m_menuManagerInterface->AddActionToMenu("o3de.menu.test", "o3de.action.test", 42);

        // Verify the action is now in the menu.
        EXPECT_EQ(menu->actions().size(), 1);
    }

    TEST_F(ActionManagerFixture, VerifyActionInMenuTwice)
    {
        // Register menu, get it and verify it's empty.
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});
        QMenu* menu = m_menuManagerInterface->GetMenu("o3de.menu.test");
        EXPECT_EQ(menu->actions().size(), 0);

        // Register a new action and add it to the menu twice.
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        m_menuManagerInterface->AddActionToMenu("o3de.menu.test", "o3de.action.test", 42);
        m_menuManagerInterface->AddActionToMenu("o3de.menu.test", "o3de.action.test", 42);

        // Verify the action was added to the menu just once.
        // This is the correct behavior.
        EXPECT_EQ(menu->actions().size(), 1);
    }

    TEST_F(ActionManagerFixture, VerifyActionOrderInMenu)
    {
        // Register menu, get it and verify it's empty.
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});
        QMenu* menu = m_menuManagerInterface->GetMenu("o3de.menu.test");
        EXPECT_EQ(menu->actions().size(), 0);

        // Register a new action and add it to the menu.
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test1", {}, []{});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test2", {}, []{});
        m_menuManagerInterface->AddActionToMenu("o3de.menu.test", "o3de.action.test2", 42);
        m_menuManagerInterface->AddActionToMenu("o3de.menu.test", "o3de.action.test1", 1);

        // Verify the actions are now in the menu.
        EXPECT_EQ(menu->actions().size(), 2);

        // Verify the order is correct
        QAction* test1 = m_actionManagerInterface->GetAction("o3de.action.test1");
        QAction* test2 = m_actionManagerInterface->GetAction("o3de.action.test2");

        const auto& actions = menu->actions();
        EXPECT_EQ(actions[0], test1);
        EXPECT_EQ(actions[1], test2);
    }

    TEST_F(ActionManagerFixture, VerifyActionOrderInMenuWithCollision)
    {
        // Register menu, get it and verify it's empty.
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});
        QMenu* menu = m_menuManagerInterface->GetMenu("o3de.menu.test");
        EXPECT_EQ(menu->actions().size(), 0);

        // Register a new action and add it to the menu.
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test1", {}, []{});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test2", {}, []{});
        m_menuManagerInterface->AddActionToMenu("o3de.menu.test", "o3de.action.test2", 42);
        m_menuManagerInterface->AddActionToMenu("o3de.menu.test", "o3de.action.test1", 42);

        // Verify the actions are now in the menu.
        EXPECT_EQ(menu->actions().size(), 2);

        // Verify the order is correct (when a collision happens, items should be in order of addition).
        QAction* test1 = m_actionManagerInterface->GetAction("o3de.action.test1");
        QAction* test2 = m_actionManagerInterface->GetAction("o3de.action.test2");

        const auto& actions = menu->actions();
        EXPECT_EQ(actions[0], test2);
        EXPECT_EQ(actions[1], test1);
    }

    TEST_F(ActionManagerFixture, VerifySeparatorInMenu)
    {
        // Register menu, get it and verify it's empty.
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});
        QMenu* menu = m_menuManagerInterface->GetMenu("o3de.menu.test");
        EXPECT_EQ(menu->actions().size(), 0);

        // Add a separator to the menu
        m_menuManagerInterface->AddSeparatorToMenu("o3de.menu.test", 42);

        // Verify the separator is now in the menu.
        const auto& actions = menu->actions();

        EXPECT_EQ(actions.size(), 1);
        EXPECT_TRUE(actions[0]->isSeparator());
    }

    TEST_F(ActionManagerFixture, VerifySubMenuInMenu)
    {
        // Register menu, get it and verify it's empty.
        m_menuManagerInterface->RegisterMenu("o3de.menu.testMenu", {});
        m_menuManagerInterface->RegisterMenu("o3de.menu.testSubMenu", {});

        // Add the subMenu to the menu
        m_menuManagerInterface->AddSubMenuToMenu("o3de.menu.testMenu", "o3de.menu.testSubMenu", 42);

        // Verify the submenu is now in the menu.
        QMenu* menu = m_menuManagerInterface->GetMenu("o3de.menu.testMenu");
        QMenu* submenu = m_menuManagerInterface->GetMenu("o3de.menu.testSubMenu");
        const auto& actions = menu->actions();

        EXPECT_EQ(actions.size(), 1);
        EXPECT_EQ(actions[0]->menu(), submenu);
    }

    TEST_F(ActionManagerFixture, VerifyComplexMenu)
    {
        // Combine multiple actions, separators and submenus.
        m_menuManagerInterface->RegisterMenu("o3de.menu.testMenu", {});
        m_menuManagerInterface->RegisterMenu("o3de.menu.testSubMenu", {});

        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test1", {}, []{});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test2", {}, []{});

        // Create a menu with this setup. Order of addition is intentionally scrambled to verify sortKeys.
        // - Test 1 Action
        // - Test 2 Action
        // - Separator
        // - SubMenu
        //   - Test 2 Action
        // 
        // Note: it is legal to add the same action to multiple different menus.
        m_menuManagerInterface->AddActionToMenu("o3de.menu.testMenu", "o3de.action.test2", 12);
        m_menuManagerInterface->AddActionToMenu("o3de.menu.testSubMenu", "o3de.action.test2", 1);
        m_menuManagerInterface->AddSubMenuToMenu("o3de.menu.testMenu", "o3de.menu.testSubMenu", 42);
        m_menuManagerInterface->AddActionToMenu("o3de.menu.testMenu", "o3de.action.test1", 11);
        m_menuManagerInterface->AddSeparatorToMenu("o3de.menu.testMenu", 18);

        // Verify the actions are now in the menu in the expected order.
        QMenu* menu = m_menuManagerInterface->GetMenu("o3de.menu.testMenu");
        QMenu* submenu = m_menuManagerInterface->GetMenu("o3de.menu.testSubMenu");
        QAction* test1 = m_actionManagerInterface->GetAction("o3de.action.test1");
        QAction* test2 = m_actionManagerInterface->GetAction("o3de.action.test2");

        // Note: separators and sub-menus are still QActions in the context of the menu.
        EXPECT_EQ(menu->actions().size(), 4);

        // Verify the order is correct
        const auto& actions = menu->actions();
        EXPECT_EQ(actions[0], test1);
        EXPECT_EQ(actions[1], test2);
        EXPECT_TRUE(actions[2]->isSeparator());
        EXPECT_EQ(actions[3]->menu(), submenu);

        const auto& subactions = submenu->actions();
        EXPECT_EQ(subactions[0], test2);
    }

} // namespace UnitTest
