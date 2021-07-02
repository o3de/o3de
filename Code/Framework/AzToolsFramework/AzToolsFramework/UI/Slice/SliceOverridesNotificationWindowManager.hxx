/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "SliceOverridesNotificationWindow.hxx"
#endif


namespace AzToolsFramework
{
    /**
    * This class is used to manage the notification windows.
    */
    class SliceOverridesNotificationWindowManager
        : public QObject
    {
        Q_OBJECT
    public:
        explicit SliceOverridesNotificationWindowManager(QObject* parent = nullptr);

        /**
        * Create a new notification window.
        * \param type type of the notification.
        * \param message content of the notification.
        */
        void CreateNotificationWindow(SliceOverridesNotificationWindow::EType type, const QString& message);

        /**
        * Remove an existing notification window.
        * \param notificationWindow pointer to the notification window to remove.
        */
        void RemoveNotificationWindow(SliceOverridesNotificationWindow* notificationWindow);

        /**
        * Retrieve a notification window.
        * \index index of the notification window to retrieve.
        * return pointer to the notification window to retrieve.
        */
        SliceOverridesNotificationWindow* GetNotificationWindow(uint index) const
        {
            return m_notificationWindows[index];
        }

        /**
        * Get the total number of notification windows.
        * return number of notification windows.
        */
        size_t GetNumNotificationWindow() const
        {
            return m_notificationWindows.size();
        }

    private:
        AZStd::vector<SliceOverridesNotificationWindow*> m_notificationWindows;
    };
} // namespace AzToolsFramework
