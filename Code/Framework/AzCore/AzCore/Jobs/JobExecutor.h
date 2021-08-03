/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Jobs/Internal/JobTypeEraser.h>
#include <AzCore/Jobs/JobDescriptor.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ
{
    class JobGraphEvent;

    namespace Internal
    {
        class CompiledJobGraph final
        {
        public:
            AZ_CLASS_ALLOCATOR(CompiledJobGraph, SystemAllocator, 0)

            CompiledJobGraph(
                AZStd::vector<TypeErasedJob>&& jobs,
                AZStd::unordered_map<uint32_t, AZStd::vector<uint32_t>>& links,
                size_t linkCount,
                bool retained);

            ~CompiledJobGraph();

            AZStd::vector<TypeErasedJob>& Jobs() noexcept
            {
                return m_jobs;
            }

            // Indicate that a constituent job has finished and decrement a counter to determine if the
            // graph should be freed
            void Release();

        private:
            friend class JobGraph;
            friend class JobWorker;

            AZStd::vector<TypeErasedJob> m_jobs;
            AZStd::vector<uint32_t> m_successors;
            AZStd::atomic<uint32_t>* m_dependencyCounts = nullptr;
            JobGraphEvent* m_waitEvent = nullptr;
            AZStd::atomic<uint32_t> m_remaining;
            bool m_retained;
        };

        class JobWorker;
    } // namespace Internal

    class JobExecutor
    {
    public:
        AZ_CLASS_ALLOCATOR(JobExecutor, SystemAllocator, 0);

        static JobExecutor& Instance();

        // Passing 0 for the threadCount requests for the thread count to match the hardware concurrency
        JobExecutor(uint32_t threadCount = 0);
        ~JobExecutor();

        void Submit(Internal::CompiledJobGraph& graph);

        void Submit(Internal::TypeErasedJob& job);

        // Busy wait until jobs are cleared from the executor (note, does not prevent future jobs from being submitted)
        void Drain();
    private:
        friend class Internal::JobWorker;

        Internal::JobWorker* m_workers;
        uint32_t m_threadCount = 0;
        AZStd::atomic<uint32_t> m_lastSubmission;
        AZStd::atomic<uint64_t> m_remaining;
    };
} // namespace AZ
