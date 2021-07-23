/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

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

            // The purpose of this method is to allow jobs queued past a natural limit to have a place to
            // go. Generally, queuing jobs past some implementation defined size may result in a slower
            // path protected by locks.
            virtual void QueueUnbounded(Job* job, bool shouldActivateWorker) = 0;
        };
    }
}

