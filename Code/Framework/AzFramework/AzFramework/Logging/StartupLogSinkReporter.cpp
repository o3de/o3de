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
