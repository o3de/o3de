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
