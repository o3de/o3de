/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/ActionManager/ActionManagerFixture.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <QMenu>
#include <QMenuBar>

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

    TEST_F(ActionManagerFixture, VerifyMenuIsRegistered)
    {
        auto outcome = m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});
        EXPECT_TRUE(m_menuManagerInterface->IsMenuRegistered("o3de.menu.test"));
    }

    TEST_F(ActionManagerFixture, RegisterMenuBar)
    {
        auto outcome = m_menuManagerInterface->RegisterMenuBar("o3de.menubar.test", m_mainWindow);
        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, RegisterMenuBarTwice)
    {
        m_menuManagerInterface->RegisterMenuBar("o3de.menubar.test", m_mainWindow);
        auto outcome = m_menuManagerInterface->RegisterMenuBar("o3de.menubar.test", m_mainWindow);
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, AddActionToUnregisteredMenu)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});

        auto outcome = m_menuManagerInterface->AddActionToMenu("o3de.menu.test", "o3de.action.test", 42);
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, AddActionToMenu)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});

        auto outcome = m_menuManagerInterface->AddActionToMenu("o3de.menu.test", "o3de.action.test", 42);
        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, AddActionToMenuTwice)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});

        m_menuManagerInterface->AddActionToMenu("o3de.menu.test", "o3de.action.test", 42);
        auto outcome = m_menuManagerInterface->AddActionToMenu("o3de.menu.test", "o3de.action.test", 42);
        EXPECT_FALSE(outcome.IsSuccess());
    }
    
    TEST_F(ActionManagerFixture, AddActionsToMenu)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test2", {}, []{});
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});

        AZStd::vector<AZStd::pair<AZStd::string, int>> actions;
        actions.push_back(AZStd::make_pair("o3de.action.test", 42));
        actions.push_back(AZStd::make_pair("o3de.action.test2", 1));

        auto outcome = m_menuManagerInterface->AddActionsToMenu("o3de.menu.test", actions);
        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, RemoveActionFromMenu)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});

        m_menuManagerInterface->AddActionToMenu("o3de.menu.test", "o3de.action.test", 42);

        auto outcome = m_menuManagerInterface->RemoveActionFromMenu("o3de.menu.test", "o3de.action.test");
        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, RemoveMissingActionFromMenu)
    {
        m_toolBarManagerInterface->RegisterToolBar("o3de.toolbar.test", {});
        
        auto outcome = m_menuManagerInterface->RemoveActionFromMenu("o3de.menu.test", "o3de.action.test");
        EXPECT_FALSE(outcome.IsSuccess());
    }
    
    TEST_F(ActionManagerFixture, RemoveActionsFromMenu)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test2", {}, []{});
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});

        AZStd::vector<AZStd::pair<AZStd::string, int>> actions;
        actions.push_back(AZStd::make_pair("o3de.action.test", 42));
        actions.push_back(AZStd::make_pair("o3de.action.test2", 1));
        m_menuManagerInterface->AddActionsToMenu("o3de.menu.test", actions);

        auto outcome = m_menuManagerInterface->RemoveActionsFromMenu("o3de.menu.test", { "o3de.action.test", "o3de.action.test2" });
        EXPECT_TRUE(outcome.IsSuccess());
    }
    
    TEST_F(ActionManagerFixture, RemoveMissingActionsFromMenu)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test2", {}, []{});
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});

        AZStd::vector<AZStd::pair<AZStd::string, int>> actions;
        actions.push_back(AZStd::make_pair("o3de.action.test", 42));
        m_menuManagerInterface->AddActionsToMenu("o3de.menu.test", actions);

        auto outcome = m_menuManagerInterface->RemoveActionsFromMenu("o3de.menu.test", { "o3de.action.test", "o3de.action.test2" });
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, GetUnregisteredMenu)
    {
        QMenu* menu = m_menuManagerInternalInterface->GetMenu("o3de.menu.test");
        EXPECT_TRUE(menu == nullptr);
    }

    TEST_F(ActionManagerFixture, GetMenu)
    {
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});

        QMenu* menu = m_menuManagerInternalInterface->GetMenu("o3de.menu.test");
        EXPECT_TRUE(menu != nullptr);
    }

    TEST_F(ActionManagerFixture, VerifyActionInMenu)
    {
        // Register menu, get it and verify it's empty.
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});
        QMenu* menu = m_menuManagerInternalInterface->GetMenu("o3de.menu.test");
        EXPECT_EQ(menu->actions().size(), 0);

        // Register a new action and add it to the menu.
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        m_menuManagerInterface->AddActionToMenu("o3de.menu.test", "o3de.action.test", 42);

        // Manually trigger Menu refresh - Editor will call this once per tick.
        m_menuManagerInternalInterface->RefreshMenus();

        // Verify the action is now in the menu.
        EXPECT_EQ(menu->actions().size(), 1);
    }

    TEST_F(ActionManagerFixture, VerifyActionOrderInMenu)
    {
        // Register menu, get it and verify it's empty.
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});
        QMenu* menu = m_menuManagerInternalInterface->GetMenu("o3de.menu.test");
        EXPECT_EQ(menu->actions().size(), 0);

        // Register a new action and add it to the menu.
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test1", {}, []{});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test2", {}, []{});
        m_menuManagerInterface->AddActionToMenu("o3de.menu.test", "o3de.action.test2", 42);
        m_menuManagerInterface->AddActionToMenu("o3de.menu.test", "o3de.action.test1", 1);

        // Manually trigger Menu refresh - Editor will call this once per tick.
        m_menuManagerInternalInterface->RefreshMenus();

        // Verify the actions are now in the menu.
        EXPECT_EQ(menu->actions().size(), 2);

        // Verify the order is correct.
        QAction* test1 = m_actionManagerInternalInterface->GetAction("o3de.action.test1");
        QAction* test2 = m_actionManagerInternalInterface->GetAction("o3de.action.test2");

        const auto& actions = menu->actions();
        EXPECT_EQ(actions[0], test1);
        EXPECT_EQ(actions[1], test2);
    }

    TEST_F(ActionManagerFixture, VerifyActionOrderInMenuWithCollision)
    {
        // Register menu, get it and verify it's empty.
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});
        QMenu* menu = m_menuManagerInternalInterface->GetMenu("o3de.menu.test");
        EXPECT_EQ(menu->actions().size(), 0);

        // Register a new action and add it to the menu.
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test1", {}, []{});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test2", {}, []{});
        m_menuManagerInterface->AddActionToMenu("o3de.menu.test", "o3de.action.test2", 42);
        m_menuManagerInterface->AddActionToMenu("o3de.menu.test", "o3de.action.test1", 42);

        // Manually trigger Menu refresh - Editor will call this once per tick.
        m_menuManagerInternalInterface->RefreshMenus();

        // Verify the actions are now in the menu.
        EXPECT_EQ(menu->actions().size(), 2);

        // Verify the order is correct (when a collision happens, items should be in order of addition).
        QAction* test1 = m_actionManagerInternalInterface->GetAction("o3de.action.test1");
        QAction* test2 = m_actionManagerInternalInterface->GetAction("o3de.action.test2");

        const auto& actions = menu->actions();
        EXPECT_EQ(actions[0], test2);
        EXPECT_EQ(actions[1], test1);
    }

    TEST_F(ActionManagerFixture, VerifySeparatorInMenu)
    {
        // Register menu, get it and verify it's empty.
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});
        QMenu* menu = m_menuManagerInternalInterface->GetMenu("o3de.menu.test");
        EXPECT_EQ(menu->actions().size(), 0);

        // Add a separator to the menu.
        m_menuManagerInterface->AddSeparatorToMenu("o3de.menu.test", 42);

        // Manually trigger Menu refresh - Editor will call this once per tick.
        m_menuManagerInternalInterface->RefreshMenus();

        // Verify the separator is now in the menu.
        const auto& actions = menu->actions();

        EXPECT_EQ(actions.size(), 1);
        EXPECT_TRUE(actions[0]->isSeparator());
    }

    TEST_F(ActionManagerFixture, VerifySubMenuInMenu)
    {
        // Register menu and submenu.
        m_menuManagerInterface->RegisterMenu("o3de.menu.testMenu", {});
        m_menuManagerInterface->RegisterMenu("o3de.menu.testSubMenu", {});

        // Add the sub-menu to the menu.
        m_menuManagerInterface->AddSubMenuToMenu("o3de.menu.testMenu", "o3de.menu.testSubMenu", 42);

        // Add an action to the sub-menu, else it will be empty and not be displayed.
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        m_menuManagerInterface->AddActionToMenu("o3de.menu.testSubMenu", "o3de.action.test", 42);

        // Manually trigger Menu refresh - Editor will call this once per tick.
        m_menuManagerInternalInterface->RefreshMenus();

        // Verify the sub-menu is now in the menu.
        QMenu* menu = m_menuManagerInternalInterface->GetMenu("o3de.menu.testMenu");
        QMenu* submenu = m_menuManagerInternalInterface->GetMenu("o3de.menu.testSubMenu");
        const auto& actions = menu->actions();

        EXPECT_EQ(actions.size(), 1);
        EXPECT_EQ(actions[0]->menu(), submenu);
    }

    TEST_F(ActionManagerFixture, AddSubMenuToMenuTwice)
    {
        // Register menu and submenu.
        m_menuManagerInterface->RegisterMenu("o3de.menu.testMenu", {});
        m_menuManagerInterface->RegisterMenu("o3de.menu.testSubMenu", {});

        // Add the sub-menu to the menu.
        m_menuManagerInterface->AddSubMenuToMenu("o3de.menu.testMenu", "o3de.menu.testSubMenu", 42);
        auto outcome = m_menuManagerInterface->AddSubMenuToMenu("o3de.menu.testMenu", "o3de.menu.testSubMenu", 42);
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, AddSubMenuToItself)
    {
        // Register menu.
        m_menuManagerInterface->RegisterMenu("o3de.menu.testMenu", {});

        // Add the menu to itself.
        auto outcome = m_menuManagerInterface->AddSubMenuToMenu("o3de.menu.testMenu", "o3de.menu.testMenu", 42);
        EXPECT_FALSE(outcome.IsSuccess());
    }
    
    TEST_F(ActionManagerFixture, AddSubMenusToMenu)
    {
        // Register menu and submenus.
        m_menuManagerInterface->RegisterMenu("o3de.menu.testMenu", {});
        m_menuManagerInterface->RegisterMenu("o3de.menu.testSubMenu1", {});
        m_menuManagerInterface->RegisterMenu("o3de.menu.testSubMenu2", {});

        // Add an action to the sub-menus, else they will be empty and not be displayed.
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        m_menuManagerInterface->AddActionToMenu("o3de.menu.testSubMenu1", "o3de.action.test", 42);
        m_menuManagerInterface->AddActionToMenu("o3de.menu.testSubMenu2", "o3de.action.test", 42);

        // Add the sub-menus to the menu.
        AZStd::vector<AZStd::pair<AZStd::string, int>> testMenus;
        testMenus.emplace_back("o3de.menu.testSubMenu1", 100);
        testMenus.emplace_back("o3de.menu.testSubMenu2", 200);

        m_menuManagerInterface->AddSubMenusToMenu("o3de.menu.testMenu", testMenus);

        // Manually trigger Menu refresh - Editor will call this once per tick.
        m_menuManagerInternalInterface->RefreshMenus();

        // Verify the sub-menus are now in the menu.
        QMenu* menu = m_menuManagerInternalInterface->GetMenu("o3de.menu.testMenu");
        QMenu* submenu1 = m_menuManagerInternalInterface->GetMenu("o3de.menu.testSubMenu1");
        QMenu* submenu2 = m_menuManagerInternalInterface->GetMenu("o3de.menu.testSubMenu2");
        const auto& actions = menu->actions();

        EXPECT_EQ(actions.size(), 2);
        EXPECT_EQ(actions[0]->menu(), submenu1);
        EXPECT_EQ(actions[1]->menu(), submenu2);
    }

    TEST_F(ActionManagerFixture, RemoveSubMenuFromMenu)
    {
        // Register menu and submenu.
        m_menuManagerInterface->RegisterMenu("o3de.menu.testMenu", {});
        m_menuManagerInterface->RegisterMenu("o3de.menu.testSubMenu", {});

        // Add the sub-menu to the menu.
        m_menuManagerInterface->AddSubMenuToMenu("o3de.menu.testMenu", "o3de.menu.testSubMenu", 42);

        // Remove the sub-menu from the menu.
        m_menuManagerInterface->RemoveSubMenuFromMenu("o3de.menu.testMenu", "o3de.menu.testSubMenu");

        // Manually trigger Menu refresh - Editor will call this once per tick.
        m_menuManagerInternalInterface->RefreshMenus();

        // Verify the sub-menu is not in the menu.
        QMenu* menu = m_menuManagerInternalInterface->GetMenu("o3de.menu.testMenu");
        const auto& actions = menu->actions();

        EXPECT_EQ(actions.size(), 0);
    }

    TEST_F(ActionManagerFixture, RemoveSubMenuFromMenuWithoutAdding)
    {
        // Register menu and submenu.
        m_menuManagerInterface->RegisterMenu("o3de.menu.testMenu", {});

        // Remove the sub-menu from the menu.
        auto outcome = m_menuManagerInterface->RemoveSubMenuFromMenu("o3de.menu.testMenu", "o3de.menu.testSubMenu");
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, RemoveSubMenuFromMenuTwice)
    {
        // Register menu and submenu.
        m_menuManagerInterface->RegisterMenu("o3de.menu.testMenu", {});
        m_menuManagerInterface->RegisterMenu("o3de.menu.testSubMenu", {});

        // Add the sub-menu to the menu.
        m_menuManagerInterface->AddSubMenuToMenu("o3de.menu.testMenu", "o3de.menu.testSubMenu", 42);

        // Remove the sub-menu from the menu twice.
        m_menuManagerInterface->RemoveSubMenuFromMenu("o3de.menu.testMenu", "o3de.menu.testSubMenu");
        auto outcome = m_menuManagerInterface->RemoveSubMenuFromMenu("o3de.menu.testMenu", "o3de.menu.testSubMenu");
        EXPECT_FALSE(outcome.IsSuccess());
    }
    
    TEST_F(ActionManagerFixture, RemoveSubMenusFromMenu)
    {
        // Register menu and submenus.
        m_menuManagerInterface->RegisterMenu("o3de.menu.testMenu", {});
        m_menuManagerInterface->RegisterMenu("o3de.menu.testSubMenu1", {});
        m_menuManagerInterface->RegisterMenu("o3de.menu.testSubMenu2", {});
        m_menuManagerInterface->RegisterMenu("o3de.menu.testSubMenu3", {});

        // Add an action to the sub-menus, else they will be empty and not be displayed.
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        m_menuManagerInterface->AddActionToMenu("o3de.menu.testSubMenu1", "o3de.action.test", 42);
        m_menuManagerInterface->AddActionToMenu("o3de.menu.testSubMenu2", "o3de.action.test", 42);
        m_menuManagerInterface->AddActionToMenu("o3de.menu.testSubMenu3", "o3de.action.test", 42);

        // Add the sub-menus to the menu.
        AZStd::vector<AZStd::pair<AZStd::string, int>> testMenuAdds;
        testMenuAdds.emplace_back("o3de.menu.testSubMenu1", 100);
        testMenuAdds.emplace_back("o3de.menu.testSubMenu2", 200);
        testMenuAdds.emplace_back("o3de.menu.testSubMenu3", 300);

        m_menuManagerInterface->AddSubMenusToMenu("o3de.menu.testMenu", testMenuAdds);

        // Remove two sub-menus from the menu.
        AZStd::vector<AZStd::string> testMenuRemoves;
        testMenuRemoves.emplace_back("o3de.menu.testSubMenu1");
        testMenuRemoves.emplace_back("o3de.menu.testSubMenu2");
        m_menuManagerInterface->RemoveSubMenusFromMenu("o3de.menu.testMenu", testMenuRemoves);

        // Manually trigger Menu refresh - Editor will call this once per tick.
        m_menuManagerInternalInterface->RefreshMenus();

        // Verify only one sub-menu is now in the menu.
        QMenu* menu = m_menuManagerInternalInterface->GetMenu("o3de.menu.testMenu");
        QMenu* submenu3 = m_menuManagerInternalInterface->GetMenu("o3de.menu.testSubMenu3");
        const auto& actions = menu->actions();

        EXPECT_EQ(actions.size(), 1);
        EXPECT_EQ(actions[0]->menu(), submenu3);
    }

    TEST_F(ActionManagerFixture, AddUnregisteredWidgetInMenu)
    {
        // Try to add a widget without registering it.
        auto outcome = m_menuManagerInterface->AddWidgetToMenu("o3de.menu.test", "someUnregisteredWidgetIdentifier", 42);
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, VerifyWidgetInMenu)
    {
        // Register menu and widget action.
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});

        QWidget* widget = new QWidget();
        m_actionManagerInterface->RegisterWidgetAction(
            "o3de.widgetAction.test", {}, [widget]()
            {
                // Note: the WidgetAction generator function should create a new widget every time it's called.
                // This implementation is technically incorrect, but it allows us to test the correct behavior.
                return widget;
            }
        );

        // Add the widget to the menu.
        m_menuManagerInterface->AddWidgetToMenu("o3de.menu.test", "o3de.widgetAction.test", 42);

        // Manually trigger Menu refresh - Editor will call this once per tick.
        m_menuManagerInternalInterface->RefreshMenus();

        // Verify the widget is now in the menu.
        QMenu* menu = m_menuManagerInternalInterface->GetMenu("o3de.menu.test");
        const auto& actions = menu->actions();

        EXPECT_EQ(actions.size(), 1);

        QWidgetAction* widgetAction = qobject_cast<QWidgetAction*>(actions[0]);
        EXPECT_TRUE(widgetAction != nullptr);
        EXPECT_TRUE(widgetAction->defaultWidget() == widget);
    }

    TEST_F(ActionManagerFixture, VerifyComplexMenu)
    {
        // Combine multiple actions, separators and submenus.
        m_menuManagerInterface->RegisterMenu("o3de.menu.testMenu", {});
        m_menuManagerInterface->RegisterMenu("o3de.menu.testSubMenu", {});

        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
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

        // Manually trigger Menu refresh - Editor will call this once per tick.
        m_menuManagerInternalInterface->RefreshMenus();

        // Verify the actions are now in the menu in the expected order.
        QMenu* menu = m_menuManagerInternalInterface->GetMenu("o3de.menu.testMenu");
        QMenu* submenu = m_menuManagerInternalInterface->GetMenu("o3de.menu.testSubMenu");
        QAction* test1 = m_actionManagerInternalInterface->GetAction("o3de.action.test1");
        QAction* test2 = m_actionManagerInternalInterface->GetAction("o3de.action.test2");

        // Note: separators and sub-menus are still QActions in the context of the menu.
        EXPECT_EQ(menu->actions().size(), 4);

        // Verify the order is correct.
        const auto& actions = menu->actions();
        EXPECT_EQ(actions[0], test1);
        EXPECT_EQ(actions[1], test2);
        EXPECT_TRUE(actions[2]->isSeparator());
        EXPECT_EQ(actions[3]->menu(), submenu);

        const auto& subactions = submenu->actions();
        EXPECT_EQ(subactions[0], test2);
    }
    
    TEST_F(ActionManagerFixture, AddMenuToUnregisteredMenuBar)
    {
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});

        auto outcome = m_menuManagerInterface->AddMenuToMenuBar("o3de.menubar.test", "o3de.menu.test", 42);
        EXPECT_FALSE(outcome.IsSuccess());
    }
    
    TEST_F(ActionManagerFixture, AddMenuToMenuBar)
    {
        m_menuManagerInterface->RegisterMenuBar("o3de.menubar.test", m_mainWindow);
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});

        auto outcome = m_menuManagerInterface->AddMenuToMenuBar("o3de.menubar.test", "o3de.menu.test", 42);
        EXPECT_TRUE(outcome.IsSuccess());
    }
    
    TEST_F(ActionManagerFixture, AddMenuToMenuBarTwice)
    {
        m_menuManagerInterface->RegisterMenuBar("o3de.menubar.test", m_mainWindow);
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});

        m_menuManagerInterface->AddMenuToMenuBar("o3de.menubar.test", "o3de.menu.test", 42);
        auto outcome = m_menuManagerInterface->AddMenuToMenuBar("o3de.menubar.test", "o3de.menu.test", 42);
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, VerifyMenuInMenuBar)
    {
        m_menuManagerInterface->RegisterMenuBar("o3de.menubar.test", m_mainWindow);
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});

        // Add the menu to the menu bar.
        m_menuManagerInterface->AddMenuToMenuBar("o3de.menubar.test", "o3de.menu.test", 42);

        // Manually trigger MenuBar refresh - Editor will call this once per tick.
        m_menuManagerInternalInterface->RefreshMenuBars();

        // Verify the submenu is now in the menu.
        QMenuBar* menubar = m_mainWindow->menuBar();
        QMenu* menu = m_menuManagerInternalInterface->GetMenu("o3de.menu.test");
        const auto& actions = menubar->actions();

        EXPECT_EQ(actions.size(), 1);
        EXPECT_EQ(actions[0]->menu(), menu);
    }

    TEST_F(ActionManagerFixture, VerifyComplexMenuBar)
    {
        // Register multiple menus.
        m_menuManagerInterface->RegisterMenuBar("o3de.menubar.test", m_mainWindow);
        m_menuManagerInterface->RegisterMenu("o3de.menu.testMenu1", {});
        m_menuManagerInterface->RegisterMenu("o3de.menu.testMenu2", {});
        m_menuManagerInterface->RegisterMenu("o3de.menu.testMenu3", {});

        // Create a menu bar with this setup. Order of addition is intentionally scrambled to verify sortKeys.
        // - Menu 1
        // - Menu 2
        // - Menu 3
        
        m_menuManagerInterface->AddMenuToMenuBar("o3de.menubar.test", "o3de.menu.testMenu2", 42);
        m_menuManagerInterface->AddMenuToMenuBar("o3de.menubar.test", "o3de.menu.testMenu3", 42);
        m_menuManagerInterface->AddMenuToMenuBar("o3de.menubar.test", "o3de.menu.testMenu1", 16);

        // Manually trigger MenuBar refresh - Editor will call this once per tick.
        m_menuManagerInternalInterface->RefreshMenuBars();

        // Verify the menus are now in the menu bar in the expected order.
        QMenuBar* menubar = m_mainWindow->menuBar();
        QMenu* testMenu1 = m_menuManagerInternalInterface->GetMenu("o3de.menu.testMenu1");
        QMenu* testMenu2 = m_menuManagerInternalInterface->GetMenu("o3de.menu.testMenu2");
        QMenu* testMenu3 = m_menuManagerInternalInterface->GetMenu("o3de.menu.testMenu3");

        // Note: menus are represented via a QAction with a submenu property in Qt.
        EXPECT_EQ(menubar->actions().size(), 3);

        // Verify the order is correct.
        const auto& actions = menubar->actions();
        EXPECT_EQ(actions[0]->menu(), testMenu1);
        EXPECT_EQ(actions[1]->menu(), testMenu2);
        EXPECT_EQ(actions[2]->menu(), testMenu3);
    }

    TEST_F(ActionManagerFixture, GetSortKeyOfActionInMenu)
    {
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});

        // Add the action to the menu.
        m_menuManagerInterface->AddActionToMenu("o3de.menu.test", "o3de.action.test", 42);

        // Verify the API returns the correct sort key.
        auto outcome = m_menuManagerInterface->GetSortKeyOfActionInMenu("o3de.menu.test", "o3de.action.test");
        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_EQ(outcome.GetValue(), 42);
    }

    TEST_F(ActionManagerFixture, GetSortKeyOfUnregisteredActionInMenu)
    {
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});

        // Verify the API fails as the action is not registered.
        auto outcome = m_menuManagerInterface->GetSortKeyOfActionInMenu("o3de.menu.test", "o3de.action.test");
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, GetSortKeyOfActionNotInMenu)
    {
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});

        // Verify the API fails as the action is registered but was not added to the menu.
        auto outcome = m_menuManagerInterface->GetSortKeyOfActionInMenu("o3de.menu.test", "o3de.action.test");
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, GetSortKeyOfSubMenuInMenu)
    {
        m_menuManagerInterface->RegisterMenu("o3de.menu.testMenu", {});
        m_menuManagerInterface->RegisterMenu("o3de.menu.testSubMenu", {});

        // Add the sub-menu to the menu.
        m_menuManagerInterface->AddSubMenuToMenu("o3de.menu.testMenu", "o3de.menu.testSubMenu", 42);

        // Verify the API returns the correct sort key.
        auto outcome = m_menuManagerInterface->GetSortKeyOfSubMenuInMenu("o3de.menu.testMenu", "o3de.menu.testSubMenu");
        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_EQ(outcome.GetValue(), 42);
    }

    TEST_F(ActionManagerFixture, GetSortKeyOfUnregisteredSubMenuInMenu)
    {
        m_menuManagerInterface->RegisterMenu("o3de.menu.testMenu", {});

        // Verify the API fails as the sub-menu is not registered.
        auto outcome = m_menuManagerInterface->GetSortKeyOfActionInMenu("o3de.menu.testMenu", "o3de.menu.testSubMenu");
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, GetSortKeyOfSubMenuNotInMenu)
    {
        m_menuManagerInterface->RegisterMenu("o3de.menu.testMenu", {});
        m_menuManagerInterface->RegisterMenu("o3de.menu.testSubMenu", {});

        // Verify the API fails as the sub-menu is registered but was not added to the menu.
        auto outcome = m_menuManagerInterface->GetSortKeyOfActionInMenu("o3de.menu.testMenu", "o3de.menu.testSubMenu");
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, GetSortKeyOfWidgetInMenu)
    {
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});
        m_actionManagerInterface->RegisterWidgetAction(
            "o3de.widgetAction.test",
            {},
            []() -> QWidget*
            {
                return nullptr;
            }
        );

        // Add the widget to the menu.
        m_menuManagerInterface->AddWidgetToMenu("o3de.menu.test", "o3de.widgetAction.test", 42);

        // Verify the API returns the correct sort key.
        auto outcome = m_menuManagerInterface->GetSortKeyOfWidgetInMenu("o3de.menu.test", "o3de.widgetAction.test");
        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_EQ(outcome.GetValue(), 42);
    }

    TEST_F(ActionManagerFixture, GetSortKeyOfUnregisteredWidgetInMenu)
    {
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});

        // Verify the API fails as the widget is not registered.
        auto outcome = m_menuManagerInterface->GetSortKeyOfWidgetInMenu("o3de.menu.test", "o3de.widgetAction.test");
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, GetSortKeyOfWidgetNotInMenu)
    {
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});
        m_actionManagerInterface->RegisterWidgetAction(
            "o3de.widgetAction.test",
            {},
            []() -> QWidget*
            {
                return nullptr;
            }
        );

        // Verify the API fails as the widget is registered but was not added to the menu.
        auto outcome = m_menuManagerInterface->GetSortKeyOfWidgetInMenu("o3de.menu.test", "o3de.widgetAction.test");
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, GetSortKeyOfMenuInMenuBar)
    {
        m_menuManagerInterface->RegisterMenuBar("o3de.menubar.test", m_mainWindow);
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});

        // Add the menu to the menu bar.
        m_menuManagerInterface->AddMenuToMenuBar("o3de.menubar.test", "o3de.menu.test", 42);

        // Verify the API returns the correct sort key.
        auto outcome = m_menuManagerInterface->GetSortKeyOfMenuInMenuBar("o3de.menubar.test", "o3de.menu.test");
        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_EQ(outcome.GetValue(), 42);
    }

    TEST_F(ActionManagerFixture, GetSortKeyOfUnregisteredMenuInMenuBar)
    {
        m_menuManagerInterface->RegisterMenuBar("o3de.menubar.test", m_mainWindow);

        // Verify the API fails as the menu is not registered.
        auto outcome = m_menuManagerInterface->GetSortKeyOfActionInMenu("o3de.menubar.test", "o3de.menu.test");
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, GetSortKeyOfMenuNotInMenuBar)
    {
        m_menuManagerInterface->RegisterMenuBar("o3de.menubar.test", m_mainWindow);
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});

        // Verify the API fails as the menu is registered but was not added to the menu bar.
        auto outcome = m_menuManagerInterface->GetSortKeyOfActionInMenu("o3de.menubar.test", "o3de.menu.test");
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, VerifyHideFromMenusWhenDisabledTrue)
    {
        // Register menu, get it and verify it's empty.
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});
        QMenu* menu = m_menuManagerInternalInterface->GetMenu("o3de.menu.test");
        EXPECT_EQ(menu->actions().size(), 0);

        // Register a new action and add it to the menu. MenuVisibility is set to HideWhenDisabled by default.
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        m_menuManagerInterface->AddActionToMenu("o3de.menu.test", "o3de.action.test", 42);

        // Add enabled state callback.
        bool enabledState = true;
        m_actionManagerInterface->InstallEnabledStateCallback(
            "o3de.action.test",
            [&]()
            {
                return enabledState;
            }
        );

        // Manually trigger Menu refresh - Editor will call this once per tick.
        m_menuManagerInternalInterface->RefreshMenus();

        // Verify the action is now in the menu.
        EXPECT_EQ(menu->actions().size(), 1);

        // Set the action as disabled.
        enabledState = false;
        m_actionManagerInterface->UpdateAction("o3de.action.test");

        // Manually trigger Menu refresh - Editor will call this once per tick.
        m_menuManagerInternalInterface->RefreshMenus();

        // Verify the action is no longer in the menu.
        EXPECT_EQ(menu->actions().size(), 0);
    }

    TEST_F(ActionManagerFixture, VerifyMenuVisibilityAlwaysShow)
    {
        // Register menu, get it and verify it's empty.
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});
        QMenu* menu = m_menuManagerInternalInterface->GetMenu("o3de.menu.test");
        EXPECT_EQ(menu->actions().size(), 0);

        // Register a new action and add it to the menu. Have MenuVisibility set to AlwaysShow.
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", actionProperties, []{});
        m_menuManagerInterface->AddActionToMenu("o3de.menu.test", "o3de.action.test", 42);

        // Add enabled state callback.
        bool enabledState = true;
        m_actionManagerInterface->InstallEnabledStateCallback(
            "o3de.action.test",
            [&]()
            {
                return enabledState;
            }
        );

        // Manually trigger Menu refresh - Editor will call this once per tick.
        m_menuManagerInternalInterface->RefreshMenus();

        // Verify the action is now in the menu.
        EXPECT_EQ(menu->actions().size(), 1);

        // Set the action as disabled.
        enabledState = false;
        m_actionManagerInterface->UpdateAction("o3de.action.test");

        // Manually trigger Menu refresh - Editor will call this once per tick.
        m_menuManagerInternalInterface->RefreshMenus();

        // Verify the action is still in the menu.
        EXPECT_EQ(menu->actions().size(), 1);
    }

    TEST_F(ActionManagerFixture, VerifyActionIsHiddenWhenChangingMode)
    {
        // Register menu, get it and verify it's empty.
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});
        QMenu* menu = m_menuManagerInternalInterface->GetMenu("o3de.menu.test");
        EXPECT_EQ(menu->actions().size(), 0);

        // Register a new action and add it to the default mode.
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, "o3de.action.test");

        // Add the action to the menu.
        m_menuManagerInterface->AddActionToMenu("o3de.menu.test", "o3de.action.test", 42);

        // Manually trigger Menu refresh - Editor will call this once per tick.
        m_menuManagerInternalInterface->RefreshMenus();

        // Verify the action is now in the menu.
        EXPECT_EQ(menu->actions().size(), 1);

        // Register a new mode and switch to it.
        m_actionManagerInterface->RegisterActionContextMode("o3de.context.test", "testMode");
        m_actionManagerInterface->SetActiveActionContextMode("o3de.context.test", "testMode");

        // Manually trigger Menu refresh - Editor will call this once per tick.
        m_menuManagerInternalInterface->RefreshMenus();

        // Verify the action is no longer in the menu.
        EXPECT_EQ(menu->actions().size(), 0);
    }

    TEST_F(ActionManagerFixture, VerifyMenuVisibilityAlwaysShowWhenChangingMode)
    {
        // Register menu, get it and verify it's empty.
        m_menuManagerInterface->RegisterMenu("o3de.menu.test", {});
        QMenu* menu = m_menuManagerInternalInterface->GetMenu("o3de.menu.test");
        EXPECT_EQ(menu->actions().size(), 0);

        // Register a new action and add it to the default mode. Have MenuVisibility set to AlwaysShow.
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::AlwaysShow;

        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", actionProperties, []{});
        m_actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, "o3de.action.test");

        // Add the action to the menu.
        m_menuManagerInterface->AddActionToMenu("o3de.menu.test", "o3de.action.test", 42);

        // Manually trigger Menu refresh - Editor will call this once per tick.
        m_menuManagerInternalInterface->RefreshMenus();

        // Verify the action is now in the menu.
        EXPECT_EQ(menu->actions().size(), 1);

        // Register a new mode and switch to it.
        m_actionManagerInterface->RegisterActionContextMode("o3de.context.test", "testMode");
        m_actionManagerInterface->SetActiveActionContextMode("o3de.context.test", "testMode");

        // Manually trigger Menu refresh - Editor will call this once per tick.
        m_menuManagerInternalInterface->RefreshMenus();

        // Verify the action is still in the menu.
        EXPECT_EQ(menu->actions().size(), 1);
    }

    TEST_F(ActionManagerFixture, VerifySubMenuIsHiddenWhenEmptied)
    {
        // Register menus
        m_menuManagerInterface->RegisterMenu("o3de.menu.testMenu", {});
        m_menuManagerInterface->RegisterMenu("o3de.menu.testSubMenu", {});
        m_menuManagerInterface->AddSubMenuToMenu("o3de.menu.testMenu", "o3de.menu.testSubMenu", 42);

        // Register a new action and add it to the sub-menu.
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        m_menuManagerInterface->AddActionToMenu("o3de.menu.testSubMenu", "o3de.action.test", 42);

        // Add enabled state callback.
        bool enabledState = true;
        m_actionManagerInterface->InstallEnabledStateCallback(
            "o3de.action.test",
            [&]()
            {
                return enabledState;
            }
        );

        // Manually trigger Menu refresh - Editor will call this once per tick.
        m_menuManagerInternalInterface->RefreshMenus();

        // Verify the sub-menu is now in the menu.
        {
            QMenu* menu = m_menuManagerInternalInterface->GetMenu("o3de.menu.testMenu");
            QMenu* submenu = m_menuManagerInternalInterface->GetMenu("o3de.menu.testSubMenu");
            const auto& actions = menu->actions();

            EXPECT_EQ(actions.size(), 1);
            EXPECT_EQ(actions[0]->menu(), submenu);
        }

        // Set the action as disabled.
        enabledState = false;
        m_actionManagerInterface->UpdateAction("o3de.action.test");

        // Manually trigger Menu refresh - Editor will call this once per tick.
        m_menuManagerInternalInterface->RefreshMenus();

        // Verify the sub-menu is no longer part of the menu since it is empty.
        {
            QMenu* menu = m_menuManagerInternalInterface->GetMenu("o3de.menu.testMenu");
            EXPECT_EQ(menu->actions().size(), 0);
        }
    }

    TEST_F(ActionManagerFixture, VerifySubMenuIsShownWhenFilled)
    {
        // Register menus
        m_menuManagerInterface->RegisterMenu("o3de.menu.testMenu", {});
        m_menuManagerInterface->RegisterMenu("o3de.menu.testSubMenu", {});
        m_menuManagerInterface->AddSubMenuToMenu("o3de.menu.testMenu", "o3de.menu.testSubMenu", 42);

        // Register a new action and add it to the sub-menu.
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        m_menuManagerInterface->AddActionToMenu("o3de.menu.testSubMenu", "o3de.action.test", 42);

        // Add enabled state callback.
        bool enabledState = false;
        m_actionManagerInterface->InstallEnabledStateCallback(
            "o3de.action.test",
            [&]()
            {
                return enabledState;
            }
        );

        // Manually trigger Menu refresh - Editor will call this once per tick.
        m_menuManagerInternalInterface->RefreshMenus();

        // Verify the sub-menu is not part of the menu since it is empty.
        {
            QMenu* menu = m_menuManagerInternalInterface->GetMenu("o3de.menu.testMenu");
            EXPECT_EQ(menu->actions().size(), 0);
        }

        // Set the action as enabled.
        enabledState = true;
        m_actionManagerInterface->UpdateAction("o3de.action.test");

        // Manually trigger Menu refresh - Editor will call this once per tick.
        m_menuManagerInternalInterface->RefreshMenus();

        // Verify the sub-menu is in the menu again.
        {
            QMenu* menu = m_menuManagerInternalInterface->GetMenu("o3de.menu.testMenu");
            QMenu* submenu = m_menuManagerInternalInterface->GetMenu("o3de.menu.testSubMenu");
            const auto& actions = menu->actions();

            EXPECT_EQ(actions.size(), 1);
            EXPECT_EQ(actions[0]->menu(), submenu);
        }
    }

    TEST_F(ActionManagerFixture, VerifySimpleAddSubMenuCircularDependency)
    {
        // Register menus
        m_menuManagerInterface->RegisterMenu("o3de.menu.testMenu", {});
        m_menuManagerInterface->RegisterMenu("o3de.menu.testSubMenu", {});
        m_menuManagerInterface->AddSubMenuToMenu("o3de.menu.testMenu", "o3de.menu.testSubMenu", 42);

        // Verify I can't add "o3de.menu.testMenu" as a sub-menu for "o3de.menu.testSubMenu"
        // as it would cause a circular dependency.
        auto outcome = m_menuManagerInterface->AddSubMenuToMenu("o3de.menu.testSubMenu", "o3de.menu.testMenu", 42);
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, VerifyNestedAddSubMenuCircularDependency)
    {
        // Register menus
        m_menuManagerInterface->RegisterMenu("o3de.menu.testMenu", {});
        m_menuManagerInterface->RegisterMenu("o3de.menu.testSubMenu", {});
        m_menuManagerInterface->RegisterMenu("o3de.menu.testSubSubMenu", {});
        m_menuManagerInterface->AddSubMenuToMenu("o3de.menu.testMenu", "o3de.menu.testSubMenu", 42);
        m_menuManagerInterface->AddSubMenuToMenu("o3de.menu.testSubMenu", "o3de.menu.testSubSubMenu", 42);

        // Verify I can't add "o3de.menu.testMenu" as a sub-menu for "o3de.menu.testSubSubMenu"
        // as it would cause a circular dependency.
        auto outcome = m_menuManagerInterface->AddSubMenuToMenu("o3de.menu.testSubSubMenu", "o3de.menu.testMenu", 42);
        EXPECT_FALSE(outcome.IsSuccess());
    }

} // namespace UnitTest
