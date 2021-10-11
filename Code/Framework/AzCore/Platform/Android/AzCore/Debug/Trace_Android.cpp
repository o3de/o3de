/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Trace.h>

#include <android/log.h>

namespace AZ
{
    namespace Debug
    {
        namespace Platform
        {
            void OutputToDebugger([[maybe_unused]] const char* window, [[maybe_unused]] const char* message)
            {
                __android_log_print(ANDROID_LOG_INFO, window, "%s", message);
            }
        }
    }
}
