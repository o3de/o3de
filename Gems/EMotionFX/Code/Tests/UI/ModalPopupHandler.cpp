/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Tests/UI/ModalPopupHandler.h"
#include "Tests/UI/UIFixture.h"

#include <QApplication>
#include <QTimer>
#include <QMenu>
#include <QObject>
#include <QtTest>

namespace EMotionFX
{
    ModalPopupHandler::ModalPopupHandler(QObject* pParent)
        : QObject(pParent)
    {
    }

    void ModalPopupHandler::ShowContextMenuAndTriggerAction(QWidget* widget, const QString& actionName, int timeout, ActionCompletionCallback completionCallback)
    {
        MenuActiveCallback menuCallback = [=](QMenu* menu)
        {
            ASSERT_TRUE(menu) << "Failed to find context menu.";

            m_seenTargetWidget = true;

            QAction* action = menu->findChild<QAction*>(actionName);
            ASSERT_TRUE(action) << "Unable to find context menu action " << actionName.toUtf8().data();
            action->trigger();

            menu->close();

            if (m_actionCompletionCallback)
            {
                m_actionCompletionCallback(actionName);
            }
        };

        m_totalTime = 0;
        m_actionCompletionCallback = completionCallback;
        m_menuActiveCallback = menuCallback;
        m_timeout = timeout;

        // Kick a timer off to check whether the menu is open.
        QTimer::singleShot(WaitTickTime, this, SLOT(CheckForContextMenu()));

        // Open the modal menu.
        UIFixture::BringUpContextMenu(widget, QPoint(10, 10), widget->mapToGlobal(QPoint(10, 10)));
    }

    void ModalPopupHandler::CheckForContextMenu()
    {
        m_totalTime += WaitTickTime;

        if (m_totalTime >= m_timeout)
        {
            if (m_actionCompletionCallback)
            {
                m_actionCompletionCallback("");
                return;
            }
        }

        // Check for the active widget being a popup widget.
        QWidget* qpop = QApplication::activePopupWidget();
        if (!qpop)
        {
            QTimer::singleShot(WaitTickTime, this, SLOT(CheckForContextMenu()));
            return;
        }

        // If the active widget is not a menu, keep waiting.
        QMenu* menu = qobject_cast<QMenu*>(qpop);
        if (!menu)
        {
            QTimer::singleShot(WaitTickTime, this, SLOT(CheckForContextMenu()));
            return;
        }

        // The menu is now active, inform the calling object.
        m_menuActiveCallback(menu);
    }

    void ModalPopupHandler::CheckForPopupWidget()
    {
        m_totalTime += WaitTickTime;

        if (m_totalTime >= m_timeout)
        {
            if (m_actionCompletionCallback)
            {
                m_actionCompletionCallback("");
                m_complete = true;
                return;
            }
        }

        // Check for the active widget being a popup widget.
        QWidget* modalWidget = QApplication::activeModalWidget();
        if (!modalWidget)
        {
            QTimer::singleShot(WaitTickTime, this, &ModalPopupHandler::CheckForPopupWidget);
            return;
        }

        m_seenTargetWidget = true;

        // Inform the calling object.
        m_widgetActiveCallback(modalWidget);
    }

    bool ModalPopupHandler::GetSeenTargetWidget()
    {
        return m_seenTargetWidget;
    }

    void ModalPopupHandler::ResetSeenTargetWidget()
    {
        m_seenTargetWidget = false;
    }

    bool ModalPopupHandler::GetIsComplete()
    {
        return m_complete;
    }

    void ModalPopupHandler::WaitForCompletion(const int timeout)
    {
        static_cast<void>(QTest::qWaitFor([&]() {
            return m_complete;
        }, timeout));
    }
} // namespace EMotionFX

#include <Tests/UI/moc_ModalPopupHandler.cpp>
