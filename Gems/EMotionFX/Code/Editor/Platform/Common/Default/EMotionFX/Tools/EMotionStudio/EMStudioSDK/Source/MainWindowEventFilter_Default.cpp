/*
 * Copyright (c) Contributors to the Open 3D Engine Project
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
