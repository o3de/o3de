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
            if (priority <= -1)
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

        //////////////////////////////////////////////////////////////////////////////////
        // Apple pthread -> NSThread quality of service level map
        // QOS class name              | min pthread priority | max pthread priority | comment
        // QOS_CLASS_USER_INTERACTIVE  |          38          |          47          |  Per-frame work
        // QOS_CLASS_USER_INITIATED    |          32          |          37          |  Asynchronous / Cross frame work
        // QOS_CLASS_DEFAULT           |          21          |          31          |  Streaming / Multiple frames deadline
        // QOS_CLASS_UTILITY           |           5          |          20          |  Background asset download
        // QOS_CLASS_BACKGROUN         |           0          |           4          |  Will be prevented from using whole core.
        uint8_t GetDefaultThreadPriority()
        {
            // We currently set the thread priority somewhere in the middle of QOS_CLASS_DEFAULT by default, since our threads generally
            // don't need to finish in a single frame. Set this value higher if the default thread priorities are causing the threads to
            // get starved too often.
            return 25;
        }
    }
}
