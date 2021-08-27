/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "NotificationWindowManager.h"
#include "EMStudioManager.h"
#include "MainWindow.h"


namespace EMStudio
{
    // spacing between each notification window
    const uint32 notificationWindowSpacing = 2;


    // padding on the bottom and right of the main window
    const uint32 notificationWindowMainWindowPadding = 5;


    // create one notification window and add it in the manager
    void NotificationWindowManager::CreateNotificationWindow(NotificationWindow::EType type, const QString& message)
    {
        // get the main window
        MainWindow* mainWindow = GetMainWindow();

        // create and show the notification window
        NotificationWindow* notificationWindow = new NotificationWindow((QWidget*)mainWindow, type, message);
        notificationWindow->show();

        // compute the height of all notification windows with the spacing
        int allNotificationWindowsHeight = 0;
        for (const NotificationWindow* currentNotificationWindow : m_notificationWindows)
        {
            allNotificationWindowsHeight += currentNotificationWindow->geometry().height() + notificationWindowSpacing;
        }

        // move the notification window
        const QRect& notificationWindowGeometry = notificationWindow->geometry();
        const QPoint mainWindowBottomRight = mainWindow->window()->geometry().bottomRight();
        notificationWindow->move(mainWindowBottomRight.x() - notificationWindowGeometry.width() - notificationWindowMainWindowPadding, mainWindowBottomRight.y() - allNotificationWindowsHeight - notificationWindowGeometry.height() - notificationWindowMainWindowPadding);

        // add the notification window in the array
        m_notificationWindows.emplace_back(notificationWindow);
    }


    // remove one notification window
    void NotificationWindowManager::RemoveNotificationWindow(NotificationWindow* notificationWindow)
    {
        // find the notification window
        auto windowIt = AZStd::find(begin(m_notificationWindows), end(m_notificationWindows), notificationWindow);

        // if not found, stop here
        if (windowIt == end(m_notificationWindows))
        {
            return;
        }

        // move down each notification window after this one, spacing is added on the height
        const int notificationWindowHeight = notificationWindow->geometry().height() + notificationWindowSpacing;
        for (auto it = windowIt + 1; it != end(m_notificationWindows); ++it)
        {
            const QPoint pos = (*it)->pos();
            (*it)->move(pos.x(), pos.y() + notificationWindowHeight);
        }

        // remove the notification window
        m_notificationWindows.erase(windowIt);
    }


    // called when the main window moved or resized
    void NotificationWindowManager::OnMovedOrResized()
    {
        // get the main window
        MainWindow* mainWindow = GetMainWindow();

        // move each notification window
        int currentNotificationWindowHeight = notificationWindowMainWindowPadding;
        for (NotificationWindow* notificationWindow : m_notificationWindows)
        {
            // add the height of the notification window
            currentNotificationWindowHeight += notificationWindow->geometry().height();

            // move the notification window
            const QPoint mainWindowBottomRight = mainWindow->geometry().bottomRight();
            const QRect& notificationWindowGeometry = notificationWindow->geometry();
            notificationWindow->move(mainWindowBottomRight.x() - notificationWindowGeometry.width() - notificationWindowMainWindowPadding, mainWindowBottomRight.y() - currentNotificationWindowHeight);

            // spacing is added after to avoid spacing on the bottom of the first notification window
            currentNotificationWindowHeight += notificationWindowSpacing;
        }
    }
} // namespace EMStudio
