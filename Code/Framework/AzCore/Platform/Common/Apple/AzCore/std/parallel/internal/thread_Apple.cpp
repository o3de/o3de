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

#include <mach/thread_policy.h>
#include <mach/thread_act.h>

namespace AZStd
{
    namespace Platform
    {
        void NameCurrentThread(const char* name)
        {
            // On Apple, we name a new thread from that thread itself
            if (!name)
            {
                name = "AZStd thread";
            }
            pthread_setname_np(name);
        }

        void PreCreateSetThreadAffinity(int, pthread_attr_t&)
        {
            // Affinity set in PostCreateThread on Apple platforms
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

        void PostCreateThread(pthread_t tId, const char*, int cpuId)
        {
            // On Apple platforms thread affinity is set after the thread is created
            if (cpuId >= 0) // TODO: add a proper mask to support more than one core that can run the thread!!!
            {
                mach_port_t mach_thread = pthread_mach_thread_np(tId);
                thread_affinity_policy_data_t policyData = { cpuId };
                thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY, (thread_policy_t)& policyData, 1);
            }
        }
    }
}
