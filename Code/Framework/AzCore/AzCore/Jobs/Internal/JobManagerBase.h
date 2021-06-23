/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_JOBS_INTERNAL_JOBMANAGER_BASE_H
#define AZCORE_JOBS_INTERNAL_JOBMANAGER_BASE_H 1

namespace AZ
{
    class Job;

    namespace Internal
    {
        class JobManagerBase
        {
        public:
            static const AZ::u32 InvalidWorkerThreadId = ~0u;
        protected:
            void Process(Job* job);
        };
    }
}

#endif

