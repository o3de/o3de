/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionStudio/EMStudioSDK/Source/MainWindowEventFilter.h>
#include <EMotionStudio/EMStudioSDK/Source/MainWindow.h>
#include <AzCore/PlatformIncl.h>
#include <dbt.h>

namespace EMStudio
{
    bool NativeEventFilter::nativeEventFilter(const QByteArray& /*eventType*/, void* message, long* /*result*/)
    {
        MSG* msg = static_cast<MSG*>(message);
        if (msg->message == WM_DEVICECHANGE)
        {
            if (msg->wParam == DBT_DEVICEREMOVECOMPLETE || msg->wParam == DBT_DEVICEARRIVAL || msg->wParam == DBT_DEVNODES_CHANGED)
            {
                // The reason why there are multiple of such messages is because it emits messages for all related hardware nodes.
                // But we do not know the name of the hardware to look for here either, so we can't filter that.
                emit m_MainWindow->HardwareChangeDetected();
            }
        }

        return false;
    }
}
