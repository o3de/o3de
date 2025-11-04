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
    TEST_F(ActionManagerFixture, AssignWidgetToActionContext)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});

        auto outcome = m_hotKeyManagerInterface->AssignWidgetToActionContext("o3de.context.test", m_widget);
        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, RemoveWidgetFromActionContext)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});

        m_hotKeyManagerInterface->AssignWidgetToActionContext("o3de.context.test", m_widget);
        auto outcome = m_hotKeyManagerInterface->RemoveWidgetFromActionContext("o3de.context.test", m_widget);
        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, SetHotKeyToAction)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});

        auto outcome = m_hotKeyManagerInterface->SetActionHotKey("o3de.action.test", "Ctrl+Z");
        EXPECT_TRUE(outcome.IsSuccess());
    }

    TEST_F(ActionManagerFixture, SetInvalidHotKeyToAction)
    {
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
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
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {}, []{});

        m_hotKeyManagerInterface->SetActionHotKey("o3de.action.test", "Ctrl+Z");

        QAction* action = m_actionManagerInternalInterface->GetAction("o3de.action.test");
        EXPECT_TRUE(action->shortcut() == QKeySequence("Ctrl+Z"));
    }

    TEST_F(ActionManagerFixture, VerifyActionHotkeyTriggered)
    {
        bool actionTriggered = false;
        m_actionManagerInterface->RegisterActionContext("o3de.context.test", {});
        m_hotKeyManagerInterface->AssignWidgetToActionContext("o3de.context.test", m_widget);
        m_actionManagerInterface->RegisterAction("o3de.context.test", "o3de.action.test", {},
            [&actionTriggered]()
            {
                actionTriggered = true;
            }
        );

        m_hotKeyManagerInterface->SetActionHotKey("o3de.action.test", "Ctrl+Z");

        // Make sure to set the active window and give our m_widget focus so that the events get propogated correctly
        QApplication::setActiveWindow(m_defaultParentWidget);
        m_widget->setFocus();

        // Trigger a shortcut event to our widget, which should in turn trigger our action
        QShortcutEvent testEvent(QKeySequence("Ctrl+Z"), 0, true);
        QApplication::sendEvent(m_widget, &testEvent);

        EXPECT_TRUE(actionTriggered);

        QApplication::setActiveWindow(nullptr);
    }

    TEST_F(ActionManagerFixture, VerifyAmbiguousShortcutsHandled)
    {
        // Ambiguous shortcuts occur when a parent and child both have an action with the same shortcut
        // and the child is focused, because Qt propagates shortcut events upwards.
        // This test verifies that we correctly capture these ambiguous shortcuts in the child and
        // trigger the appropriate action.
        bool parentActionTriggered = false;
        m_actionManagerInterface->RegisterActionContext("o3de.context.parent", {});
        m_hotKeyManagerInterface->AssignWidgetToActionContext("o3de.context.parent", m_widget);
        m_actionManagerInterface->RegisterAction("o3de.context.parent", "o3de.action.parent", {},
            [&parentActionTriggered]()
            {
                parentActionTriggered = true;
            }
        );

        m_hotKeyManagerInterface->SetActionHotKey("o3de.action.parent", "Ctrl+Z");

        // Setup a child of our parent widget with an action that has the same shortcut
        QWidget* childWidget = new QWidget(m_widget);
        bool childActionTriggered = false;
        m_actionManagerInterface->RegisterActionContext("o3de.context.child", {});
        m_hotKeyManagerInterface->AssignWidgetToActionContext("o3de.context.child", childWidget);
        m_actionManagerInterface->RegisterAction("o3de.context.child", "o3de.action.child", {},
            [&childActionTriggered]()
            {
                childActionTriggered = true;
            }
        );

        m_hotKeyManagerInterface->SetActionHotKey("o3de.action.child", "Ctrl+Z");

        // Make sure to set the active window and give our childWidget focus so that the events get propogated correctly
        QApplication::setActiveWindow(m_defaultParentWidget);
        childWidget->setFocus();

        // setting Focus actually requires us to pump the event pump.
        QCoreApplication::processEvents();

        // Trigger a shortcut event to our child widget, which should in turn only trigger our child action
        QShortcutEvent testEvent(QKeySequence("Ctrl+Z"), 0, true);
        QApplication::sendEvent(childWidget, &testEvent);

        EXPECT_TRUE(childActionTriggered);
        EXPECT_FALSE(parentActionTriggered);

        QApplication::setActiveWindow(nullptr);
    }

} // namespace UnitTest
