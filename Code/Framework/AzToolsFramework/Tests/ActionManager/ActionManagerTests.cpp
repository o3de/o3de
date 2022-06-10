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
    TEST_F(ActionManagerFixture, RegisterActionContextWithoutWidget)
    {
        auto outcome =
            m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, nullptr);
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, RegisterActionContext)
    {
        auto outcome =
            m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, RegisterActionToUnregisteredContext)
    {
        auto outcome = m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, RegisterAction)
    {
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);

        auto outcome = m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});

        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, RegisterActionTwice)
    {
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        auto outcome = m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});

        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, RegisterCheckableActionToUnregisteredContext)
    {
        auto outcome = m_actionManagerInterface->RegisterCheckableAction("o3de.context.test", "o3de.action.test", {}, []{}, []()->bool{ return true; });
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, RegisterCheckableAction)
    {
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);

        auto outcome = m_actionManagerInterface->RegisterCheckableAction("o3de.context.test", "o3de.action.test", {}, []{}, []()->bool{ return true; });

        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, RegisterCheckableActionTwice)
    {
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        m_actionManagerInterface->RegisterCheckableAction("o3de.context.test", "o3de.action.test", {}, []{}, []()->bool{ return true; });
        auto outcome = m_actionManagerInterface->RegisterCheckableAction("o3de.context.test", "o3de.action.test", {}, []{}, []()->bool{ return true; });

        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, GetUnregisteredAction)
    {
        QAction* action = m_actionManagerInterface->GetAction("o3de.action.test");
        EXPECT_EQ(action, nullptr);
    }

    TEST_F(ActionManagerFixture, GetAction)
    {
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});

        QAction* action = m_actionManagerInterface->GetAction("o3de.action.test");
        EXPECT_TRUE(action != nullptr);
    }

    TEST_F(ActionManagerFixture, GetAndTriggerAction)
    {
        bool actionTriggered = false;

        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        m_actionManagerInterface->RegisterAction(
            "o3de.context.test", "o3de.action.test", {},
            [&actionTriggered]()
            {
                actionTriggered = true;
            }
        );

        QAction* action = m_actionManagerInterface->GetAction("o3de.action.test");
        
        action->trigger();
        EXPECT_TRUE(actionTriggered);
    }

    TEST_F(ActionManagerFixture, TriggerUnregisteredAction)
    {
        auto outcome = m_actionManagerInterface->TriggerAction("o3de.action.test");
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, TriggerAction)
    {
        bool actionTriggered = false;

        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
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

        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
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
        
        QAction* action = m_actionManagerInterface->GetAction("o3de.action.checkableTest");
        EXPECT_TRUE(action->isChecked());
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

        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
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
        
        QAction* action = m_actionManagerInterface->GetAction("o3de.action.checkableTest");
        EXPECT_FALSE(action->isChecked());

        // When the property driving the action's state is changed outside the Action Manager system,
        // the caller should ensure the actions relying on it are updated accordingly.

        actionToggle = true;
        
        EXPECT_FALSE(action->isChecked());
        
        auto outcome = m_actionManagerInterface->UpdateAction("o3de.action.checkableTest");
        
        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_TRUE(action->isChecked());
    }

    TEST_F(ActionManagerFixture, UpdateNonCheckableAction)
    {
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});
        
        auto outcome = m_actionManagerInterface->UpdateAction("o3de.action.test");
        
        EXPECT_FALSE(outcome.IsSuccess());
    }

} // namespace UnitTest
