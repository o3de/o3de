/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/ActionManager/ActionManagerFixture.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <QToolBar>

namespace UnitTest
{
    TEST_F(ActionManagerFixture, RegisterToolBar)
    {
        auto outcome = m_toolBarManagerInterface->RegisterToolBar("o3de.toolbar.test", {});
        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, RegisterMenuToolBar)
    {
        m_toolBarManagerInterface->RegisterToolBar("o3de.toolbar.test", {});
        auto outcome = m_toolBarManagerInterface->RegisterToolBar("o3de.toolbar.test", {});
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, AddActionToUnregisteredToolBar)
    {
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});

        auto outcome = m_toolBarManagerInterface->AddActionToToolBar("o3de.toolbar.test", "o3de.action.test", 42);
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, AddActionToToolBar)
    {
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        m_toolBarManagerInterface->RegisterToolBar("o3de.toolbar.test", {});
        
        auto outcome = m_toolBarManagerInterface->AddActionToToolBar("o3de.toolbar.test", "o3de.action.test", 42);
        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, AddActionToToolBarTwice)
    {
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        m_toolBarManagerInterface->RegisterToolBar("o3de.toolbar.test", {});
        
        m_toolBarManagerInterface->AddActionToToolBar("o3de.toolbar.test", "o3de.action.test", 42);
        auto outcome = m_toolBarManagerInterface->AddActionToToolBar("o3de.toolbar.test", "o3de.action.test", 42);
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, AddActionsToToolBar)
    {
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test2", {}, []{});
        m_toolBarManagerInterface->RegisterToolBar("o3de.toolbar.test", {});
        
        AZStd::vector<AZStd::pair<AZStd::string, int>> actions;
        actions.push_back(AZStd::make_pair("o3de.action.test", 42));
        actions.push_back(AZStd::make_pair("o3de.action.test2", 1));

        auto outcome = m_toolBarManagerInterface->AddActionsToToolBar("o3de.toolbar.test", actions);
        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, RemoveActionFromToolBar)
    {
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        m_toolBarManagerInterface->RegisterToolBar("o3de.toolbar.test", {});
        
        m_toolBarManagerInterface->AddActionToToolBar("o3de.toolbar.test", "o3de.action.test", 42);
        
        auto outcome = m_toolBarManagerInterface->RemoveActionFromToolBar("o3de.toolbar.test", "o3de.action.test");
        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, RemoveMissingActionFromToolBar)
    {
        m_toolBarManagerInterface->RegisterToolBar("o3de.toolbar.test", {});
        
        auto outcome = m_toolBarManagerInterface->RemoveActionFromToolBar("o3de.toolbar.test", "o3de.action.test");
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, RemoveActionsFromToolBar)
    {
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test2", {}, []{});
        m_toolBarManagerInterface->RegisterToolBar("o3de.toolbar.test", {});
        
        AZStd::vector<AZStd::pair<AZStd::string, int>> actions;
        actions.push_back(AZStd::make_pair("o3de.action.test", 42));
        actions.push_back(AZStd::make_pair("o3de.action.test2", 1));

        m_toolBarManagerInterface->AddActionsToToolBar("o3de.toolbar.test", actions);

        auto outcome = m_toolBarManagerInterface->RemoveActionsFromToolBar("o3de.toolbar.test", { "o3de.action.test", "o3de.action.test2" });
        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, RemoveMissingActionsFromToolBar)
    {
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test2", {}, []{});
        m_toolBarManagerInterface->RegisterToolBar("o3de.toolbar.test", {});
        
        AZStd::vector<AZStd::pair<AZStd::string, int>> actions;
        actions.push_back(AZStd::make_pair("o3de.action.test", 42));

        m_toolBarManagerInterface->AddActionsToToolBar("o3de.toolbar.test", actions);

        auto outcome = m_toolBarManagerInterface->RemoveActionsFromToolBar("o3de.toolbar.test", { "o3de.action.test", "o3de.action.test2" });
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, GetUnregisteredToolBar)
    {
        QToolBar* toolBar = m_toolBarManagerInterface->GetToolBar("o3de.toolbar.test");
        EXPECT_TRUE(toolBar == nullptr);
    }

    TEST_F(ActionManagerFixture, GetToolBar)
    {
        m_toolBarManagerInterface->RegisterToolBar("o3de.toolbar.test", {});
        
        QToolBar* toolBar = m_toolBarManagerInterface->GetToolBar("o3de.toolbar.test");
        EXPECT_TRUE(toolBar != nullptr);
    }

    TEST_F(ActionManagerFixture, VerifyActionInToolBar)
    {
        // Register toolbar, get it and verify it's empty.
        m_toolBarManagerInterface->RegisterToolBar("o3de.toolbar.test", {});
        QToolBar* toolBar = m_toolBarManagerInterface->GetToolBar("o3de.toolbar.test");
        EXPECT_EQ(toolBar->actions().size(), 0);

        // Register a new action and add it to the toolbar.
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        auto outcome = m_toolBarManagerInterface->AddActionToToolBar("o3de.toolbar.test", "o3de.action.test", 42);

        // Verify the action is now in the menu.
        EXPECT_EQ(toolBar->actions().size(), 1);
    }

    TEST_F(ActionManagerFixture, VerifyActionOrderInToolBar)
    {
        // Register toolbar, get it and verify it's empty.
        m_toolBarManagerInterface->RegisterToolBar("o3de.toolbar.test", {});
        QToolBar* toolBar = m_toolBarManagerInterface->GetToolBar("o3de.toolbar.test");
        EXPECT_EQ(toolBar->actions().size(), 0);

        // Register a new action and add it to the toolbar.
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test1", {}, []{});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test2", {}, []{});
        m_toolBarManagerInterface->AddActionToToolBar("o3de.toolbar.test", "o3de.action.test2", 42);
        m_toolBarManagerInterface->AddActionToToolBar("o3de.toolbar.test", "o3de.action.test1", 1);

        // Verify the actions are now in the menu.
        EXPECT_EQ(toolBar->actions().size(), 2);

        // Verify the order is correct.
        QAction* test1 = m_actionManagerInterface->GetAction("o3de.action.test1");
        QAction* test2 = m_actionManagerInterface->GetAction("o3de.action.test2");

        const auto& actions = toolBar->actions();
        EXPECT_EQ(actions[0], test1);
        EXPECT_EQ(actions[1], test2);
    }

    TEST_F(ActionManagerFixture, VerifyActionOrderInToolBarWithCollision)
    {
        // Register toolbar, get it and verify it's empty.
        m_toolBarManagerInterface->RegisterToolBar("o3de.toolbar.test", {});
        QToolBar* toolBar = m_toolBarManagerInterface->GetToolBar("o3de.toolbar.test");
        EXPECT_EQ(toolBar->actions().size(), 0);
        
        // Register a new action and add it to the toolbar.
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test1", {}, []{});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test2", {}, []{});
        m_toolBarManagerInterface->AddActionToToolBar("o3de.toolbar.test", "o3de.action.test2", 42);
        m_toolBarManagerInterface->AddActionToToolBar("o3de.toolbar.test", "o3de.action.test1", 42);

        // Verify the actions are now in the menu.
        EXPECT_EQ(toolBar->actions().size(), 2);

        // Verify the order is correct (when a collision happens, items should be in order of addition).
        QAction* test1 = m_actionManagerInterface->GetAction("o3de.action.test1");
        QAction* test2 = m_actionManagerInterface->GetAction("o3de.action.test2");

        const auto& actions = toolBar->actions();
        EXPECT_EQ(actions[0], test2);
        EXPECT_EQ(actions[1], test1);
    }

    TEST_F(ActionManagerFixture, VerifySeparatorInToolBar)
    {
        // Register toolbar, get it and verify it's empty.
        m_toolBarManagerInterface->RegisterToolBar("o3de.toolbar.test", {});
        QToolBar* toolBar = m_toolBarManagerInterface->GetToolBar("o3de.toolbar.test");
        EXPECT_EQ(toolBar->actions().size(), 0);

        // Add a separator to the menu.
        m_toolBarManagerInterface->AddSeparatorToToolBar("o3de.toolbar.test", 42);

        // Verify the separator is now in the menu.
        const auto& actions = toolBar->actions();

        EXPECT_EQ(actions.size(), 1);
        EXPECT_TRUE(actions[0]->isSeparator());
    }

    TEST_F(ActionManagerFixture, VerifyComplexToolBar)
    {
        // Combine multiple actions and separators.
        m_toolBarManagerInterface->RegisterToolBar("o3de.toolbar.test", {});

        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test1", {}, []{});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test2", {}, []{});

        // Create a toolbar with this setup. Order of addition is intentionally scrambled to verify sortKeys.
        // - Test 1 Action
        // - Separator
        // - Test 2 Action
        m_toolBarManagerInterface->AddActionToToolBar("o3de.toolbar.test", "o3de.action.test2", 15);
        m_toolBarManagerInterface->AddActionToToolBar("o3de.toolbar.test", "o3de.action.test1", 1);
        m_toolBarManagerInterface->AddSeparatorToToolBar("o3de.toolbar.test", 10);

        // Verify the actions are now in the toolbar in the expected order.
        QToolBar* toolBar = m_toolBarManagerInterface->GetToolBar("o3de.toolbar.test");
        QAction* test1 = m_actionManagerInterface->GetAction("o3de.action.test1");
        QAction* test2 = m_actionManagerInterface->GetAction("o3de.action.test2");

        // Note: separators and sub-menus are still QActions in the context of the toolbar.
        const auto& actions = toolBar->actions();
        EXPECT_EQ(actions.size(), 3);

        // Verify the order is correct.
        EXPECT_EQ(actions[0], test1);
        EXPECT_TRUE(actions[1]->isSeparator());
        EXPECT_EQ(actions[2], test2);
    }

    TEST_F(ActionManagerFixture, GetSortKeyOfActionInToolBar)
    {
        m_toolBarManagerInterface->RegisterToolBar("o3de.toolbar.test", {});
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});

        // Add the action to the toolbar.
        m_toolBarManagerInterface->AddActionToToolBar("o3de.toolbar.test", "o3de.action.test", 42);

        // Verify the API returns the correct sort key.
        auto outcome = m_toolBarManagerInterface->GetSortKeyOfActionInToolBar("o3de.toolbar.test", "o3de.action.test");
        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_EQ(outcome.GetValue(), 42);
    }

    TEST_F(ActionManagerFixture, GetSortKeyOfUnregisteredActionInToolBar)
    {
        m_toolBarManagerInterface->RegisterToolBar("o3de.toolbar.test", {});

        // Verify the API fails as the action is not registered.
        auto outcome = m_toolBarManagerInterface->GetSortKeyOfActionInToolBar("o3de.toolbar.test", "o3de.action.test");
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, GetSortKeyOfActionNotInToolBar)
    {
        m_toolBarManagerInterface->RegisterToolBar("o3de.toolbar.test", {});
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});

        // Verify the API fails as the action is registered but was not added to the toolbar.
        auto outcome = m_toolBarManagerInterface->GetSortKeyOfActionInToolBar("o3de.toolbar.test", "o3de.action.test");
        EXPECT_FALSE(outcome.IsSuccess());
    }

} // namespace UnitTest
