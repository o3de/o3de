/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Logging/StartupLogSinkReporter.h>

namespace AZ
{
    namespace Debug
    {
        bool StartupLogSink::OnPreAssert(const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message)
        {
            // Don't add zero length messages to error strings it will prematurely consume asserts
            // before they can register and gives us no information
            if (strlen(message) > 0)
            {
                m_errorStringsCollected.emplace_back(message);
            }
            return false;
        }

        bool StartupLogSink::OnPreError(const char* /*window*/, const char* fileName, int line, const char* func, const char* message)
        {
            return OnPreAssert(fileName, line, func, message);
        }
    }
} // namespace AZ
