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
#ifndef AZCORE_JOBS_JOBEMPTY_H
#define AZCORE_JOBS_JOBEMPTY_H 1

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
        AZ_CLASS_ALLOCATOR(JobEmpty, ThreadPoolAllocator, 0)

        JobEmpty(bool isAutoDelete, JobContext* context = nullptr)
            : Job(isAutoDelete, context)
        {
        }
    protected:
        virtual void Process()  {}
    };
}

#endif
#pragma once