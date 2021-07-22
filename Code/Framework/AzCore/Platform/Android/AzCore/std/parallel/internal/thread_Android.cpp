/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/parallel/thread.h>

#include <sched.h>
#include <errno.h>

namespace AZStd
{
    namespace Platform
    {
        void NameCurrentThread(const char*)
        {
            // Threads are named in PostCreateThread on Android
        }

        void PreCreateSetThreadAffinity(int , pthread_attr_t&)
        {
            // Unimplemented on Android
        }

        void SetThreadPriority(int priority, pthread_attr_t&)
        {
            // (not supported at v r10d)
            AZ_Warning("System", priority <= 0, "Thread priorities %d not supported on Android!\n", priority);
            (void)priority;
        }

        void PostCreateThread(pthread_t tId, const char* name, int)
        {
            pthread_setname_np(tId, name);
        }
    }
}
