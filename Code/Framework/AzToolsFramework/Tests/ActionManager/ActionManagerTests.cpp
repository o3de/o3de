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
    TEST_F(ActionManagerFixture, RegisterActionContext)
    {
        auto outcome =
            m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, VerifyActionContextIsRegistered)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        EXPECT_TRUE(m_actionManagerInterface->IsActionContextRegistered("o3de.context.test"));
    }

    TEST_F(ActionManagerFixture, RegisterActionToUnregisteredContext)
    {
        auto outcome = m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, RegisterAction)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});

        auto outcome = m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});

        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, RegisterActionTwice)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        auto outcome = m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});

        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, VerifyActionIsRegistered)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});

        EXPECT_TRUE(m_actionManagerInterface->IsActionRegistered("o3de.action.test"));
    }

    TEST_F(ActionManagerFixture, RegisterCheckableActionToUnregisteredContext)
    {
        auto outcome = m_actionManagerInterface->RegisterCheckableAction("o3de.context.test", "o3de.action.test", {}, []{}, []()->bool{ return true; });
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, RegisterCheckableAction)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});

        auto outcome = m_actionManagerInterface->RegisterCheckableAction("o3de.context.test", "o3de.action.test", {}, []{}, []()->bool{ return true; });

        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, RegisterCheckableActionTwice)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterCheckableAction("o3de.context.test", "o3de.action.test", {}, []{}, []()->bool{ return true; });
        auto outcome = m_actionManagerInterface->RegisterCheckableAction("o3de.context.test", "o3de.action.test", {}, []{}, []()->bool{ return true; });

        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, GetUnregisteredAction)
    {
        QAction* action = m_actionManagerInternalInterface->GetAction("o3de.action.test");
        EXPECT_EQ(action, nullptr);
    }

    TEST_F(ActionManagerFixture, GetAction)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});

        QAction* action = m_actionManagerInternalInterface->GetAction("o3de.action.test");
        EXPECT_TRUE(action != nullptr);
    }

    TEST_F(ActionManagerFixture, GetAndTriggerAction)
    {
        bool actionTriggered = false;

        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction(
            "o3de.context.test", "o3de.action.test", {},
            [&actionTriggered]()
            {
                actionTriggered = true;
            }
        );

        QAction* action = m_actionManagerInternalInterface->GetAction("o3de.action.test");
        
        action->trigger();
        EXPECT_TRUE(actionTriggered);
    }

    TEST_F(ActionManagerFixture, GetActionName)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Test Name";
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", actionProperties, []{});

        auto outcome = m_actionManagerInterface->GetActionName("o3de.action.test");
        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_THAT(outcome.GetValue().c_str(), ::testing::StrEq("Test Name"));
    }

    TEST_F(ActionManagerFixture, SetActionName)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Wrong Name";
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", actionProperties, []{});

        auto setOutcome = m_actionManagerInterface->SetActionName("o3de.action.test", "Correct Name");
        EXPECT_TRUE(setOutcome.IsSuccess());

        auto getOutcome = m_actionManagerInterface->GetActionName("o3de.action.test");
        EXPECT_THAT(getOutcome.GetValue().c_str(), ::testing::StrEq("Correct Name"));
    }

    TEST_F(ActionManagerFixture, GetActionDescription)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_description = "Test Description";
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", actionProperties, []{});

        auto outcome = m_actionManagerInterface->GetActionDescription("o3de.action.test");
        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_THAT(outcome.GetValue().c_str(), ::testing::StrEq("Test Description"));
    }

    TEST_F(ActionManagerFixture, SetActionDescription)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_description = "Wrong Description";
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", actionProperties, []{});

        auto setOutcome = m_actionManagerInterface->SetActionDescription("o3de.action.test", "Correct Description");
        EXPECT_TRUE(setOutcome.IsSuccess());

        auto getOutcome = m_actionManagerInterface->GetActionDescription("o3de.action.test");
        EXPECT_THAT(getOutcome.GetValue().c_str(), ::testing::StrEq("Correct Description"));
    }

    TEST_F(ActionManagerFixture, GetActionCategory)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_category = "Test Category";
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", actionProperties, []{});

        auto outcome = m_actionManagerInterface->GetActionCategory("o3de.action.test");
        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_THAT(outcome.GetValue().c_str(), ::testing::StrEq("Test Category"));
    }

    TEST_F(ActionManagerFixture, SetActionCategory)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_category = "Wrong Category";
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", actionProperties, []{});

        auto setOutcome = m_actionManagerInterface->SetActionCategory("o3de.action.test", "Correct Category");
        EXPECT_TRUE(setOutcome.IsSuccess());

        auto getOutcome = m_actionManagerInterface->GetActionCategory("o3de.action.test");
        EXPECT_THAT(getOutcome.GetValue().c_str(), ::testing::StrEq("Correct Category"));
    }

    TEST_F(ActionManagerFixture, VerifyIncorrectIconPath)
    {
        // Since we don't want to have the unit tests depend on a resource file, this will only test the case where an incorrect path is set.
        // When a path that does not point to a resource is passed, the icon path string is cleared and the icon will be null.
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_iconPath = ":/Some/Incorrect/Path.svg";
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", actionProperties, []{});

        auto outcome = m_actionManagerInterface->GetActionIconPath("o3de.action.test");
        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_TRUE(outcome.GetValue().empty());

        QAction* action = m_actionManagerInternalInterface->GetAction("o3de.action.test");
        EXPECT_TRUE(action->icon().isNull());
    }

    TEST_F(ActionManagerFixture, TriggerUnregisteredAction)
    {
        auto outcome = m_actionManagerInterface->TriggerAction("o3de.action.test");
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, TriggerAction)
    {
        bool actionTriggered = false;

        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction(
            "o3de.context.test", "o3de.action.test", {},
            [&actionTriggered]()
            {
                actionTriggered = true;
            }
        );

        auto outcome = m_actionManagerInterface->TriggerAction("o3de.action.test");

        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_TRUE(actionTriggered);
    }

    TEST_F(ActionManagerFixture, TriggerCheckableAction)
    {
        // Verify that triggering a checkable action automatically calls the update callback to refresh the checkable state.
        bool actionToggle = false;

        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterCheckableAction(
            "o3de.context.test", "o3de.action.checkableTest", {},
            [&]()
            {
                actionToggle = !actionToggle;
            },
            [&]() -> bool
            {
                return actionToggle;
            }
        );

        auto outcome = m_actionManagerInterface->TriggerAction("o3de.action.checkableTest");

        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_TRUE(actionToggle);

        // Note that we don't expose an API to directly query the checked state of an action.
        // This is because the checkable state is just a UI indicator. Operations relying on the
        // checked state of an action should instead rely on the value of the property they visualize.
        // In this example, logic should not rely on the checked state of o3de.action.checkableTest,
        // but on the value of actionToggle itself (just like the update callback does).
        
        QAction* action = m_actionManagerInternalInterface->GetAction("o3de.action.checkableTest");
        EXPECT_TRUE(action->isChecked());
    }

    TEST_F(ActionManagerFixture, InstallEnabledStateCallback)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        
        auto outcome = m_actionManagerInterface->InstallEnabledStateCallback("o3de.action.test", []() { return false; });
        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, VerifyEnabledStateCallback)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});

        {
            auto enabledOutcome = m_actionManagerInterface->IsActionEnabled("o3de.action.test");
            EXPECT_TRUE(enabledOutcome.IsSuccess());
            EXPECT_TRUE(enabledOutcome.GetValue());
        }

        m_actionManagerInterface->InstallEnabledStateCallback("o3de.action.test", []() { return false; });
        
        {
            auto enabledOutcome = m_actionManagerInterface->IsActionEnabled("o3de.action.test");
            EXPECT_TRUE(enabledOutcome.IsSuccess());
            EXPECT_FALSE(enabledOutcome.GetValue());
        }
    }

    TEST_F(ActionManagerFixture, VerifyEnabledStateCallbackUpdate)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});

        bool enabledState = false;

        m_actionManagerInterface->InstallEnabledStateCallback("o3de.action.test", [&]() {
            return enabledState;
        });
        
        {
            auto enabledOutcome = m_actionManagerInterface->IsActionEnabled("o3de.action.test");
            EXPECT_TRUE(enabledOutcome.IsSuccess());
            EXPECT_FALSE(enabledOutcome.GetValue());
        }

        enabledState = true;
        
        {
            auto enabledOutcome = m_actionManagerInterface->IsActionEnabled("o3de.action.test");
            EXPECT_TRUE(enabledOutcome.IsSuccess());
            EXPECT_FALSE(enabledOutcome.GetValue());
        }

        m_actionManagerInterface->UpdateAction("o3de.action.test");
        
        {
            auto enabledOutcome = m_actionManagerInterface->IsActionEnabled("o3de.action.test");
            EXPECT_TRUE(enabledOutcome.IsSuccess());
            EXPECT_TRUE(enabledOutcome.GetValue());
        }
    }

    TEST_F(ActionManagerFixture, InstallMultipleEnabledStateCallbacks)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        
        m_actionManagerInterface->InstallEnabledStateCallback("o3de.action.test", []() { return false; });
        auto outcome = m_actionManagerInterface->InstallEnabledStateCallback("o3de.action.test", []() { return false; });
        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, VerifyEnabledStateCallbacks)
    {
        // Results of Enabled State Callbacks are chained with AND.
        // So all callbacks need to return TRUE for an Action to be enabled.
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        
        m_actionManagerInterface->InstallEnabledStateCallback("o3de.action.test", []() { return true; });

        {
            auto enabledOutcome = m_actionManagerInterface->IsActionEnabled("o3de.action.test");
            EXPECT_TRUE(enabledOutcome.IsSuccess());
            EXPECT_TRUE(enabledOutcome.GetValue());
        }

        m_actionManagerInterface->InstallEnabledStateCallback("o3de.action.test", []() { return false; });
        
        {
            auto enabledOutcome = m_actionManagerInterface->IsActionEnabled("o3de.action.test");
            EXPECT_TRUE(enabledOutcome.IsSuccess());
            EXPECT_FALSE(enabledOutcome.GetValue());
        }
    }

    TEST_F(ActionManagerFixture, UpdateUnregisteredAction)
    {
        auto outcome = m_actionManagerInterface->UpdateAction("o3de.action.test");
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, UpdateCheckableAction)
    {
        // Verify the ability to update the checked state of a Checkable Action.
        bool actionToggle = false;

        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterCheckableAction(
            "o3de.context.test", "o3de.action.checkableTest", {},
            [&]()
            {
                actionToggle = !actionToggle;
            },
            [&]() -> bool
            {
                return actionToggle;
            }
        );
        
        QAction* action = m_actionManagerInternalInterface->GetAction("o3de.action.checkableTest");
        EXPECT_FALSE(action->isChecked());

        // When the property driving the action's state is changed outside the Action Manager system,
        // the caller should ensure the actions relying on it are updated accordingly.

        actionToggle = true;
        
        EXPECT_FALSE(action->isChecked());
        
        auto outcome = m_actionManagerInterface->UpdateAction("o3de.action.checkableTest");
        
        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_TRUE(action->isChecked());
    }

    TEST_F(ActionManagerFixture, RegisterWidgetAction)
    {
        auto outcome = m_actionManagerInterface->RegisterWidgetAction(
            "o3de.widgetAction.test", {},
            []() -> QWidget*
            {
                return nullptr;
            }
        );
        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, RegisterWidgetActionTwice)
    {
        m_actionManagerInterface->RegisterWidgetAction(
            "o3de.widgetAction.test", {},
            []() -> QWidget*
            {
                return nullptr;
            }
        );
        auto outcome = m_actionManagerInterface->RegisterWidgetAction(
            "o3de.widgetAction.test", {},
            []() -> QWidget*
            {
                return nullptr;
            }
        );
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, VerifyWidgetActionIsRegistered)
    {
        m_actionManagerInterface->RegisterWidgetAction(
            "o3de.widgetAction.test",
            {},
            []() -> QWidget*
            {
                return nullptr;
            }
        );
        EXPECT_TRUE(m_actionManagerInterface->IsWidgetActionRegistered("o3de.widgetAction.test"));
    }

    TEST_F(ActionManagerFixture, GetWidgetActionName)
    {
        AzToolsFramework::WidgetActionProperties widgetActionProperties;
        widgetActionProperties.m_name = "Test Widget";

        m_actionManagerInterface->RegisterWidgetAction(
            "o3de.widgetAction.test",
            widgetActionProperties,
            []() -> QWidget*
            {
                return nullptr;
            }
        );

        auto outcome = m_actionManagerInterface->GetWidgetActionName("o3de.widgetAction.test");
        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_THAT(outcome.GetValue().c_str(), ::testing::StrEq("Test Widget"));
    }

    TEST_F(ActionManagerFixture, SetWidgetActionName)
    {
        AzToolsFramework::WidgetActionProperties widgetActionProperties;
        widgetActionProperties.m_name = "Wrong Widget Name";

        m_actionManagerInterface->RegisterWidgetAction(
            "o3de.widgetAction.test",
            widgetActionProperties,
            []() -> QWidget*
            {
                return nullptr;
            }
        );

        auto setOutcome = m_actionManagerInterface->SetWidgetActionName("o3de.widgetAction.test", "Correct Widget Name");
        EXPECT_TRUE(setOutcome.IsSuccess());

        auto getOutcome = m_actionManagerInterface->GetWidgetActionName("o3de.widgetAction.test");
        EXPECT_THAT(getOutcome.GetValue().c_str(), ::testing::StrEq("Correct Widget Name"));
    }

    TEST_F(ActionManagerFixture, GetWidgetActionCategory)
    {
        AzToolsFramework::WidgetActionProperties widgetActionProperties;
        widgetActionProperties.m_category = "Test Widget Category";

        m_actionManagerInterface->RegisterWidgetAction(
            "o3de.widgetAction.test",
            widgetActionProperties,
            []() -> QWidget*
            {
                return nullptr;
            }
        );

        auto outcome = m_actionManagerInterface->GetWidgetActionCategory("o3de.widgetAction.test");
        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_THAT(outcome.GetValue().c_str(), ::testing::StrEq("Test Widget Category"));
    }

    TEST_F(ActionManagerFixture, SetWidgetActionCategory)
    {
        AzToolsFramework::WidgetActionProperties widgetActionProperties;
        widgetActionProperties.m_category = "Wrong Widget Category";

        m_actionManagerInterface->RegisterWidgetAction(
            "o3de.widgetAction.test",
            widgetActionProperties,
            []() -> QWidget*
            {
                return nullptr;
            }
        );

        auto setOutcome = m_actionManagerInterface->SetWidgetActionCategory("o3de.widgetAction.test", "Correct Widget Category");
        EXPECT_TRUE(setOutcome.IsSuccess());

        auto getOutcome = m_actionManagerInterface->GetWidgetActionCategory("o3de.widgetAction.test");
        EXPECT_THAT(getOutcome.GetValue().c_str(), ::testing::StrEq("Correct Widget Category"));
    }

    TEST_F(ActionManagerFixture, RegisterActionUpdater)
    {
        auto outcome = m_actionManagerInterface->RegisterActionUpdater("o3de.updater.onTestChange");
        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, AddUnregisteredActionToUpdater)
    {
        m_actionManagerInterface->RegisterActionUpdater("o3de.updater.onTestChange");
        auto outcome = m_actionManagerInterface->AddActionToUpdater("o3de.updater.onTestChange", "o3de.action.test");
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, AddActionToUnregisteredUpdater)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        auto outcome = m_actionManagerInterface->AddActionToUpdater("o3de.updater.onTestChange", "o3de.action.test");
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, AddActionToUpdater)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        m_actionManagerInterface->RegisterActionUpdater("o3de.updater.onTestChange");

        auto outcome = m_actionManagerInterface->AddActionToUpdater("o3de.updater.onTestChange", "o3de.action.test");
        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, TriggerActionUpdater)
    {
        // Action Updaters are meant to be triggered when a specific event happens.
        // This will mostly be achieved by handling a notification bus and calling the
        // "TriggerActionUpdater" function for the updater identifier there.
        // This test only verifies that the underlying function works, but does not
        // represent the expected setup for using the system.

        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});

        m_actionManagerInterface->RegisterActionUpdater("o3de.updater.onTestChange");
        m_actionManagerInterface->AddActionToUpdater("o3de.updater.onTestChange", "o3de.action.test");

        bool enabledState = false;

        m_actionManagerInterface->InstallEnabledStateCallback("o3de.action.test", [&]() {
            return enabledState;
        });
        
        {
            auto enabledOutcome = m_actionManagerInterface->IsActionEnabled("o3de.action.test");
            EXPECT_TRUE(enabledOutcome.IsSuccess());
            EXPECT_FALSE(enabledOutcome.GetValue());
        }

        enabledState = true;
        {
            auto enabledOutcome = m_actionManagerInterface->IsActionEnabled("o3de.action.test");
            EXPECT_TRUE(enabledOutcome.IsSuccess());
            EXPECT_FALSE(enabledOutcome.GetValue());
        }

        m_actionManagerInterface->TriggerActionUpdater("o3de.updater.onTestChange");
        {
            auto enabledOutcome = m_actionManagerInterface->IsActionEnabled("o3de.action.test");
            EXPECT_TRUE(enabledOutcome.IsSuccess());
            EXPECT_TRUE(enabledOutcome.GetValue());
        }
    }

    TEST_F(ActionManagerFixture, SetUnregisteredActionContextModeOnAction)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});

        auto outcome = m_actionManagerInterface->AssignModeToAction("o3de.context.mode.test", "o3de.action.test");
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, SetActionContextModeOnAction)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterActionContextMode("o3de.context.test", "o3de.context.mode.test");
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});

        {
            auto outcome = m_actionManagerInterface->IsActionEnabled("o3de.action.test");
            EXPECT_TRUE(outcome.GetValue());
        }

        // Set Action to the "o3de.context.mode.test" mode and verify it's no longer enabled (since "o3de.context.test" is set to "default").
        m_actionManagerInterface->AssignModeToAction("o3de.context.mode.test", "o3de.action.test");

        {
            auto outcome = m_actionManagerInterface->IsActionEnabled("o3de.action.test");
            EXPECT_FALSE(outcome.GetValue());
        }
    }

    TEST_F(ActionManagerFixture, SetActionContextDefaultModeOnAction)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});

        {
            auto outcome = m_actionManagerInterface->IsActionEnabled("o3de.action.test");
            EXPECT_TRUE(outcome.GetValue());
        }

        // Set Action to the "default" mode and verify it's still enabled (since "o3de.context.test" is set to "default").
        m_actionManagerInterface->AssignModeToAction("default", "o3de.action.test");

        {
            auto outcome = m_actionManagerInterface->IsActionEnabled("o3de.action.test");
            EXPECT_TRUE(outcome.GetValue());
        }
    }

    TEST_F(ActionManagerFixture, ChangeModeAndVerifyActionWithNoSetMode)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterActionContextMode("o3de.context.test", "o3de.context.mode.test");
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});

        {
            auto outcome = m_actionManagerInterface->IsActionEnabled("o3de.action.test");
            EXPECT_TRUE(outcome.GetValue());
        }

        // Set the Action Context Mode to "o3de.context.mode.test" and verify that the action is still enabled.
        m_actionManagerInterface->SetActiveActionContextMode("o3de.context.test", "o3de.context.mode.test");

        {
            auto outcome = m_actionManagerInterface->IsActionEnabled("o3de.action.test");
            EXPECT_TRUE(outcome.GetValue());
        }
    }

    TEST_F(ActionManagerFixture, ChangeModeAndVerifyActionSetToDefaultMode)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterActionContextMode("o3de.context.test", "o3de.context.mode.test");
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});

        {
            auto outcome = m_actionManagerInterface->IsActionEnabled("o3de.action.test");
            EXPECT_TRUE(outcome.GetValue());
        }

        // Set Action to the "default" mode and verify it's still enabled (since "o3de.context.test" is set to "default").
        m_actionManagerInterface->AssignModeToAction("default", "o3de.action.test");

        {
            auto outcome = m_actionManagerInterface->IsActionEnabled("o3de.action.test");
            EXPECT_TRUE(outcome.GetValue());
        }

        // Set the Action Context Mode to "o3de.context.mode.test" and verify that the action is now disabled.
        m_actionManagerInterface->SetActiveActionContextMode("o3de.context.test", "o3de.context.mode.test");

        {
            auto outcome = m_actionManagerInterface->IsActionEnabled("o3de.action.test");
            EXPECT_FALSE(outcome.GetValue());
        }
    }

    TEST_F(ActionManagerFixture, ModeSwitchingTest)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterActionContextMode("o3de.context.test", "o3de.context.mode.test1");
        m_actionManagerInterface->RegisterActionContextMode("o3de.context.test", "o3de.context.mode.test2");

        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.testDefault", {}, []{});
        m_actionManagerInterface->AssignModeToAction("default", "o3de.action.testDefault");

        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test1", {}, []{});
        m_actionManagerInterface->AssignModeToAction("o3de.context.mode.test1", "o3de.action.test1");

        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test2", {}, []{});
        m_actionManagerInterface->AssignModeToAction("o3de.context.mode.test2", "o3de.action.test2");

        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.testAll", {}, []{});

        // At the beginning, "o3de.context.test" is set to mode "default".
        EXPECT_TRUE(m_actionManagerInterface->IsActionEnabled("o3de.action.testDefault").GetValue());
        EXPECT_FALSE(m_actionManagerInterface->IsActionEnabled("o3de.action.test1").GetValue());
        EXPECT_FALSE(m_actionManagerInterface->IsActionEnabled("o3de.action.test2").GetValue());
        EXPECT_TRUE(m_actionManagerInterface->IsActionEnabled("o3de.action.testAll").GetValue());

        // Set the Action Context Mode to "o3de.context.mode.test1" and verify that the actions are updated accordingly.
        m_actionManagerInterface->SetActiveActionContextMode("o3de.context.test", "o3de.context.mode.test1");

        EXPECT_FALSE(m_actionManagerInterface->IsActionEnabled("o3de.action.testDefault").GetValue());
        EXPECT_TRUE(m_actionManagerInterface->IsActionEnabled("o3de.action.test1").GetValue());
        EXPECT_FALSE(m_actionManagerInterface->IsActionEnabled("o3de.action.test2").GetValue());
        EXPECT_TRUE(m_actionManagerInterface->IsActionEnabled("o3de.action.testAll").GetValue());

        // Set the Action Context Mode to "o3de.context.mode.test2" and verify that the actions are updated accordingly.
        m_actionManagerInterface->SetActiveActionContextMode("o3de.context.test", "o3de.context.mode.test2");

        EXPECT_FALSE(m_actionManagerInterface->IsActionEnabled("o3de.action.testDefault").GetValue());
        EXPECT_FALSE(m_actionManagerInterface->IsActionEnabled("o3de.action.test1").GetValue());
        EXPECT_TRUE(m_actionManagerInterface->IsActionEnabled("o3de.action.test2").GetValue());
        EXPECT_TRUE(m_actionManagerInterface->IsActionEnabled("o3de.action.testAll").GetValue());

        // Set the Action Context Mode back to "default" and verify that the actions are updated accordingly.
        m_actionManagerInterface->SetActiveActionContextMode("o3de.context.test", "default");

        EXPECT_TRUE(m_actionManagerInterface->IsActionEnabled("o3de.action.testDefault").GetValue());
        EXPECT_FALSE(m_actionManagerInterface->IsActionEnabled("o3de.action.test1").GetValue());
        EXPECT_FALSE(m_actionManagerInterface->IsActionEnabled("o3de.action.test2").GetValue());
        EXPECT_TRUE(m_actionManagerInterface->IsActionEnabled("o3de.action.testAll").GetValue());
    }

} // namespace UnitTest
