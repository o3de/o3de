/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "EMStudioConfig.h"
#include "NotificationWindow.h"
#include <AzCore/std/containers/vector.h>
#endif


namespace EMStudio
{
    class EMSTUDIO_API NotificationWindowManager
    {
        MCORE_MEMORYOBJECTCATEGORY(NotificationWindowManager, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        MCORE_INLINE NotificationWindowManager()
        {
            m_visibleTime = 5;
        }

        void CreateNotificationWindow(NotificationWindow::EType type, const QString& message);
        void RemoveNotificationWindow(NotificationWindow* notificationWindow);

        MCORE_INLINE NotificationWindow* GetNotificationWindow(uint32 index) const
        {
            return m_notificationWindows[index];
        }

        MCORE_INLINE size_t GetNumNotificationWindow() const
        {
            return m_notificationWindows.size();
        }

        void OnMovedOrResized();

        MCORE_INLINE void SetVisibleTime(int32 timeSeconds)
        {
            m_visibleTime = timeSeconds;
        }

        MCORE_INLINE int32 GetVisibleTime() const
        {
            return m_visibleTime;
        }

    private:
        AZStd::vector<NotificationWindow*> m_notificationWindows;
        int32 m_visibleTime;
    };
} // namespace EMStudio
