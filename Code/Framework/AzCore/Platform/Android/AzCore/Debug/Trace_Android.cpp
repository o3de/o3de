/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Trace.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/string/fixed_string.h>

#include <android/log.h>

namespace AZ::Debug::Platform
{
    void OutputToDebugger(AZStd::string_view window, AZStd::string_view message)
    {
        constexpr size_t MaxMessageBufferLength = 4096;
        AZStd::fixed_string<MaxMessageBufferLength> windowBuffer(window);
        AZStd::fixed_string<MaxMessageBufferLength> messageBuffer;
        while (message.size() > messageBuffer.max_size())
        {
            // Transfer over messageBuffer max_size() characters over to output buffer
            messageBuffer = message.substr(0, messageBuffer.max_size());
            message = message.substr(messageBuffer.max_size());
            __android_log_write(ANDROID_LOG_INFO, windowBuffer.c_str(), messageBuffer.c_str());
        }

        if (!message.empty())
        {
            messageBuffer = message;
            __android_log_write(ANDROID_LOG_INFO, windowBuffer.c_str(), messageBuffer.c_str());
        }
    }
}
