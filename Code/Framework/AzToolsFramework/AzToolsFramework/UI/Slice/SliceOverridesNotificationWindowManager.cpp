/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/Slice/SliceOverridesNotificationWindowManager.hxx>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace AzToolsFramework
{
    // spacing between each notification window
    const uint notificationWindowSpacing = 2;


    // padding on the bottom and right of the main window
    const uint notificationWindowMainWindowPadding = 5;

    SliceOverridesNotificationWindowManager::SliceOverridesNotificationWindowManager(QObject* parent)
        : QObject(parent)
    {
    }


    // create one notification window and add it in the manager
    void SliceOverridesNotificationWindowManager::CreateNotificationWindow(SliceOverridesNotificationWindow::EType type, const QString& message)
    {
        QWidget* mainWindow = nullptr;
        EditorRequests::Bus::BroadcastResult(mainWindow, &EditorRequests::Bus::Events::GetMainWindow);

        // create and show the notification window
        SliceOverridesNotificationWindow* notificationWindow = new SliceOverridesNotificationWindow(mainWindow, type, message);
        connect(notificationWindow, &SliceOverridesNotificationWindow::RemoveNotificationWindow, this, &SliceOverridesNotificationWindowManager::RemoveNotificationWindow);
        notificationWindow->show();

        // compute the height of all notification windows with the spacing
        int allNotificationWindowsHeight = 0;
        const size_t numNotificationWindows = m_notificationWindows.size();
        for (size_t i = 0; i < numNotificationWindows; ++i)
        {
            allNotificationWindowsHeight += m_notificationWindows[i]->geometry().height() + notificationWindowSpacing;
        }

        // move the notification window
        const QRect& notificationWindowGeometry = notificationWindow->geometry();
        const QPoint mainWindowBottomRight = mainWindow->geometry().bottomRight();
        const QPoint globaleMainWindowBottomRight = mainWindow->mapToGlobal(mainWindowBottomRight);
        notificationWindow->move(
            globaleMainWindowBottomRight.x() - notificationWindowGeometry.width() - notificationWindowMainWindowPadding,
            mainWindowBottomRight.y() - allNotificationWindowsHeight - notificationWindowGeometry.height() - notificationWindowMainWindowPadding);

        // add the notification window in the array
        m_notificationWindows.push_back(notificationWindow);
    }


    // remove one notification window
    void SliceOverridesNotificationWindowManager::RemoveNotificationWindow(SliceOverridesNotificationWindow* notificationWindow)
    {
        // find the notification window
        size_t index = m_notificationWindows.size() + 1;

        for (size_t notificationWindowIndex = 0; notificationWindowIndex < m_notificationWindows.size(); ++notificationWindowIndex)
        {
            if (m_notificationWindows[notificationWindowIndex] == notificationWindow)
            {
                index = notificationWindowIndex;
            }
        }

        // if not found, stop here
        if (index == m_notificationWindows.size() + 1)
        {
            return;
        }

        // move down each notification window after this one, spacing is added on the height
        const int notificationWindowHeight = notificationWindow->geometry().height() + notificationWindowSpacing;
        const size_t numNotificationWindows = m_notificationWindows.size();
        for (size_t notificationWindowIndex = index + 1; notificationWindowIndex < numNotificationWindows; ++notificationWindowIndex)
        {
            const QPoint pos = m_notificationWindows[notificationWindowIndex]->pos();
            m_notificationWindows[notificationWindowIndex]->move(pos.x(), pos.y() + notificationWindowHeight);
        }

        // remove the notification window
        m_notificationWindows.erase(m_notificationWindows.begin() + index);
    }

} // namespace AzToolsFramework

#include "UI/Slice/moc_SliceOverridesNotificationWindowManager.cpp"
