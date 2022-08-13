/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/fixed_string.h>
#include <AzCore/std/parallel/thread.h>

#include <sched.h>
#include <errno.h>

namespace AZStd
{
    namespace Platform
    {
        void NameCurrentThread(const char*)
        {
            // Threads are named in PostCreateThread on Linux
        }

        void PreCreateSetThreadAffinity(int cpuId, pthread_attr_t& attr)
        {
            // On Linux, thread affinity is set on attr before creating the thread
            if (cpuId >= 0)
            {
                cpu_set_t cpuset;
                CPU_ZERO(&cpuset);
                CPU_SET(cpuId, &cpuset);

                int result = pthread_attr_setaffinity_np(&attr, sizeof(cpuset), &cpuset);
                (void)result;
                AZ_Warning("System", result == 0, "pthread_setaffinity_np failed with code %d: %s\n", result, strerror(result));
            }
        }

        void SetThreadPriority(int priority, pthread_attr_t& attr)
        {
            if (priority == -1)
            {
                pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED);
            }
            else
            {
                struct sched_param    schedParam;
                memset(&schedParam, 0, sizeof(schedParam));
                pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
                schedParam.sched_priority = priority;
                pthread_attr_setschedparam(&attr, &schedParam);
            }
        }

        void PostCreateThread(pthread_t tId, const char* name, int)
        {
            if (const int result = pthread_setname_np(tId, name); result == ERANGE)
            {
                // The name was too long. pthread limits the name to 16
                // characters (including the null terminator). Let's elide the
                // middle.
                const auto len = strlen(name);
                const auto elidedName = AZStd::fixed_string<15>{name, 7} + "." + AZStd::fixed_string<15>{name + len - 7, 7};
                pthread_setname_np(tId, elidedName.c_str());
            }
        }

        uint8_t GetDefaultThreadPriority()
        {
            // pthread priority is an integer between >=1 and <=99 (although only range 1<=>32 is guaranteed)
            // Don't use a scheduling policy value (e.g. SCHED_OTHER or SCHED_FIFO) here.
            return 1;
        }
    }
}
