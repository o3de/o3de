/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionStudio/EMStudioSDK/Source/MainWindowEventFilter.h>

namespace EMStudio
{
    bool NativeEventFilter::nativeEventFilter(const QByteArray& /*eventType*/, void* message, long* /*result*/)
    {
        return false;
    }
}
