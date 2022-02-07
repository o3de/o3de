/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/string_view.h>

#include <ctype.h>
#include <signal.h>
#include <unistd.h>

namespace AZ
{
    namespace Debug
    {
        namespace Platform
        {
#if defined(AZ_ENABLE_DEBUG_TOOLS)
            bool performDebuggerDetection()
            {
                AZ::IO::SystemFile processStatusFile;
                if (!processStatusFile.Open("/proc/self/status", AZ::IO::SystemFile::SF_OPEN_READ_ONLY))
                {
                    return false;
                }

                char buffer[4096];
                AZ::IO::SystemFile::SizeType numRead = processStatusFile.Read(sizeof(buffer), buffer);

                const AZStd::string_view processStatusView(buffer, buffer + numRead);
                constexpr AZStd::string_view tracerPidString = "TracerPid:";
                const size_t tracerPidOffset = processStatusView.find(tracerPidString);
                if (tracerPidOffset == AZStd::string_view::npos)
                {
                    return false;
                }
                for (size_t i = tracerPidOffset + tracerPidString.length(); i < numRead; ++i)
                {
                    if (!::isspace(processStatusView[i]))
                    {
                        return processStatusView[i] != '0';
                    }
                }
                return false;
            }

            bool IsDebuggerPresent()
            {
                static bool s_detectionPerformed = false;
                static bool s_debuggerDetected = false;
                if (!s_detectionPerformed)
                {
                    s_debuggerDetected = performDebuggerDetection();
                    s_detectionPerformed = true;
                }
                return s_debuggerDetected;
            }

            bool AttachDebugger()
            {
                // Not supported yet
                AZ_Assert(false, "AttachDebugger() is not supported for Unix platform yet");
                return false;
            }

            void HandleExceptions(bool)
            {}

            void DebugBreak()
            {
                raise(SIGINT);
            }
#endif // AZ_ENABLE_DEBUG_TOOLS

            void Terminate(int exitCode)
            {
                _exit(exitCode);
            }
        }
    }
}
