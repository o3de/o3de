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
        auto outcome = m_actionManagerInterface->RegisterAction(
            "o3de.context.test", "o3de.action.test", {},
            []()
            {
                ;
            }
        );

        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, RegisterAction)
    {
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);

        auto outcome = m_actionManagerInterface->RegisterAction(
            "o3de.context.test", "o3de.action.test", {},
            []()
            {
                ;
            }
        );

        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, RegisterActionTwice)
    {
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);

        m_actionManagerInterface->RegisterAction(
            "o3de.context.test", "o3de.action.test", {},
            []()
            {
                ;
            }
        );

        auto outcome = m_actionManagerInterface->RegisterAction(
            "o3de.context.test", "o3de.action.test", {},
            []()
            {
                ;
            }
        );

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
        m_actionManagerInterface->RegisterAction(
            "o3de.context.test", "o3de.action.test", {},
            []()
            {
                ;
            }
        );

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

} // namespace UnitTest
