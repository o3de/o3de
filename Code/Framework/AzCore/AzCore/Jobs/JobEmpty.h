/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Jobs/Job.h>

namespace AZ
{
    /**
     * An empty job which performs no processing. This can be useful in setting up some kinds of dependency chains,
     * and is used internally by the job system for that purpose.
     */
    class JobEmpty
        : public Job
    {
    public:
        AZ_CLASS_ALLOCATOR(JobEmpty, ThreadPoolAllocator);

        JobEmpty(bool isAutoDelete, JobContext* context = nullptr)
            : Job(isAutoDelete, context)
        {
        }
    protected:
        virtual void Process()  {}
    };
}
