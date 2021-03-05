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
