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

#include <AzCore/PlatformIncl.h>

#include <QTimer>
#include <QtWinExtras/QtWin>
#include <QWidget>

namespace AzQtComponents
{
    namespace Platform
    {
        void HandleFloatingWindow(QWidget* floatingWindow)
        {
            // Set the WS_EX_APPWINDOW flag on our floating window so that it can be minimized into the taskbar
            // and will show up grouped with our application
            HWND windowId = HWND(floatingWindow->winId());
            QTimer::singleShot(0, [windowId] {
                BOOL wasVisible = ::IsWindowVisible(windowId);
                if (wasVisible)
                {
                    ::ShowWindow(windowId, SW_HIDE);
                }
                ::SetWindowLong(windowId, GWL_EXSTYLE, GetWindowLong(windowId, GWL_EXSTYLE) | WS_EX_APPWINDOW);
                if (wasVisible)
                {
                    ::ShowWindow(windowId, SW_SHOW);
                }
            });
        }

        bool FloatingWindowsSupportMinimize()
        {
            return true;
        }
    }
}
