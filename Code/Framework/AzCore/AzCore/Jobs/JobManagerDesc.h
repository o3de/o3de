/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/fixed_vector.h>

namespace AZ
{
    /**
     * Descriptor for a single job manager thread, an array of these is specified in JobManagerDesc.
     */
    struct JobManagerThreadDesc
    {
        /**
         *  The CPU ids (as a bitfield) that this thread will be running on, see \ref AZStd::thread_desc::m_cpuId.
         *  Windows: This parameter is ignored.
         *  On other platforms, each bit maps directly to the core numbers [0-n], default is 0
         */
        int     m_cpuId;

        /**
         *  Thread priority.
         *  Defaults to the current platform's default priority
         */
        int     m_priority;

        /**
        *  Thread stack size.
        *  Default is 0, which means we will use the default stack size for each platform.
        */
        int     m_stackSize;

        JobManagerThreadDesc(int cpuId = -1, int priority = 0, int stackSize = -1)
            : m_cpuId(cpuId)
            , m_priority(priority)
            , m_stackSize(stackSize)
        {
        }
    };

    /**
     *  Job manager create descriptor.
     */
    struct JobManagerDesc
    {
        JobManagerDesc() {}

        const char* m_jobManagerName = "";

        using DescList = AZStd::fixed_vector<JobManagerThreadDesc, 64>;
        DescList m_workerThreads; ///< List of worker threads to create
    };
}
