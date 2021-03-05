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
#include "EMStudioConfig.h"
#include "NotificationWindow.h"
#include <MCore/Source/Array.h>
#endif


namespace EMStudio
{
    class EMSTUDIO_API NotificationWindowManager
    {
        MCORE_MEMORYOBJECTCATEGORY(NotificationWindowManager, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        MCORE_INLINE NotificationWindowManager()
        {
            mVisibleTime = 5;
        }

        void CreateNotificationWindow(NotificationWindow::EType type, const QString& message);
        void RemoveNotificationWindow(NotificationWindow* notificationWindow);

        MCORE_INLINE NotificationWindow* GetNotificationWindow(uint32 index) const
        {
            return mNotificationWindows[index];
        }

        MCORE_INLINE uint32 GetNumNotificationWindow() const
        {
            return mNotificationWindows.GetLength();
        }

        void OnMovedOrResized();

        MCORE_INLINE void SetVisibleTime(int32 timeSeconds)
        {
            mVisibleTime = timeSeconds;
        }

        MCORE_INLINE int32 GetVisibleTime() const
        {
            return mVisibleTime;
        }

    private:
        MCore::Array<NotificationWindow*> mNotificationWindows;
        int32 mVisibleTime;
    };
} // namespace EMStudio
