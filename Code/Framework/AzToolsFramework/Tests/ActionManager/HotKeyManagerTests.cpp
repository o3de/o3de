/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/ActionManager/ActionManagerFixture.h>

#include<QKeySequence>

namespace UnitTest
{
    TEST_F(ActionManagerFixture, SetHotKeyToAction)
    {
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});

        auto outcome = m_hotKeyManagerInterface->SetActionHotKey("o3de.action.test", "Ctrl+Z");
        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, SetInvalidHotKeyToAction)
    {
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});

        auto outcome = m_hotKeyManagerInterface->SetActionHotKey("o3de.action.test", "SomeWeirdString");
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, SetHotKeyToUnregisteredAction)
    {
        auto outcome = m_hotKeyManagerInterface->SetActionHotKey("o3de.context.test", "Ctrl+Z");
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, VerifyActionHotkey)
    {
        m_actionManagerInterface->RegisterActionContext("", "o3de.context.test", {}, m_widget);
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});

        m_hotKeyManagerInterface->SetActionHotKey("o3de.action.test", "Ctrl+Z");

        QAction* action = m_actionManagerInternalInterface->GetAction("o3de.action.test");
        EXPECT_TRUE(action->shortcut() == QKeySequence("Ctrl+Z"));
    }

} // namespace UnitTest
