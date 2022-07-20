/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Trace.h>

#include <assert.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

namespace AZ
{
    namespace Debug
    {
        namespace Platform
        {
#if defined(AZ_ENABLE_DEBUG_TOOLS)
            // Apple Technical Q&A QA1361
            // https://developer.apple.com/library/mac/qa/qa1361/_index.html
            // Returns true if the current process is being debugged (either
            // running under the debugger or has a debugger attached post facto).
            bool IsDebuggerPresent()
            {
                int                 mib[4];
                struct kinfo_proc   info;
                size_t              size;

                // Initialize the flags so that, if sysctl fails for some bizarre
                // reason, we get a predictable result.

                info.kp_proc.p_flag = 0;

                // Initialize mib, which tells sysctl the info we want, in this case
                // we're looking for information about a specific process ID.

                mib[0] = CTL_KERN;
                mib[1] = KERN_PROC;
                mib[2] = KERN_PROC_PID;
                mib[3] = getpid();

                // Call sysctl.

                size = sizeof(info);
                [[maybe_unused]] int sysctlResult = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);
                assert(sysctlResult == 0);

                // We're being debugged if the P_TRACED flag is set.

                return ((info.kp_proc.p_flag & P_TRACED) != 0);
            }

            bool AttachDebugger()
            {
                // Not supported yet
                AZ_Assert(false, "AttachDebugger() is not supported for Mac platform yet");
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

            void OutputToDebugger(const char*, const char*)
            {
                // std::cout << title << ": " << message;
            }
        }
    }
}
