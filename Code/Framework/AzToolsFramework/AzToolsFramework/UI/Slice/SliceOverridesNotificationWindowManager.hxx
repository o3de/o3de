/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
