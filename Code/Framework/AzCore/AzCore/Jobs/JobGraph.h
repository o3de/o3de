/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// NOTE: If adding additional header/symbol dependencies, consider if such additions are better
// suited in the private CompiledJobGraph implementation instead to keep this header lean.
#include <AzCore/Jobs/Internal/JobTypeEraser.h>
#include <AzCore/Jobs/JobDescriptor.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/binary_semaphore.h>

namespace AZ
{
    namespace Internal
    {
        class CompiledJobGraph;
    }
    class JobExecutor;

    // A JobToken is returned each time a Job is added to the JobGraph. JobTokens are used to
    // express dependencies between jobs within the graph.
    class JobToken final
    {
    public:
        // Indicate that this job must finish before the job token(s) passed as the argument
        template <typename... JT>
        void Precedes(JT&... tokens);

        // Indicate that this job must finish after the job token(s) passed as the argument
        template <typename... JT>
        void Succeeds(JT&... tokens);

    private:
        friend class JobGraph;

        void PrecedesInternal(JobToken& comesAfter);

        // Only the JobGraph should be creating JobToken
        JobToken(JobGraph& parent, size_t index);

        JobGraph& m_parent;
        size_t m_index;
    };

    // A JobGraphEvent may be used to block until a job graph has finished executing. Usage
    // is NOT recommended for the majority of tasks (prefer to simply containing expanding/contracting
    // the graph without synchronization over the course of the frame). However, the event
    // is useful for the edges of the computation graph.
    //
    // You are responsible for ensuring the event object lifetime exceeds the job graph lifetime.
    //
    // After the JobGraphEvent is signaled, you are allowed to reuse the same JobGraphEvent
    // for a future submission.
    class JobGraphEvent
    {
    public:
        bool IsSignaled();
        void Wait();

    private:
        friend class ::AZ::Internal::CompiledJobGraph;
        friend class JobGraph;
        void Signal();

        AZStd::binary_semaphore m_semaphore;
    };

    // The JobGraph encapsulates a set of jobs and their interdependencies. After adding
    // jobs, and marking dependencies as necessary, the entire graph is submitted via
    // the JobGraph::Submit method.
    //
    // The JobGraph MAY be retained across multiple frames and resubmitted, provided the
    // user provides some guarantees (see comments associated with JobGraph::Retain).
    class JobGraph final
    {
    public:
        ~JobGraph();

        // Reset the state of the job graph to begin recording jobs and edges again
        // NOTE: Graph must be in a "settled" state (cannot be in-flight)
        void Reset();

        // Add a job to the graph, retrieiving a token that can be used to express dependencies
        // between jobs. The first argument specifies the JobKind, used for tracking the job.
        // NOTE: This operation is invalid if the graph is in-flight
        template<typename Lambda>
        JobToken AddJob(JobDescriptor const& descriptor, Lambda&& lambda);

        template <typename... Lambdas>
        AZStd::array<JobToken, sizeof...(Lambdas)> AddJobs(JobDescriptor const& descriptor, Lambdas&&... lambdas);

        // By default, you are responsible for retaining the JobGraph, indicating you promise that
        // this JobGraph will live as long as it takes for all constituent jobs to complete.
        // Once retained, this job graph can be resubmitted after completion without any
        // modifications. JobTokens that were created as a result of adding jobs used to
        // mark dependencies DO NOT need to outlive the job graph.
        //
        // Invoking Detach PRIOR to submission indicates you wish the jobs associated with this
        // JobGraph to deallocate upon completion. After invoking Detach, you may let this JobGraph
        // go out of scope or deallocate after submission.
        //
        // NOTE: The JobGraph has no concept of resources used by design. Resubmission
        // of the job graph is expected to rely on either indirection, or safe overwriting
        // of previously used memory to supply new data (this can even be done as the first
        // job in the graph).
        // NOTE: This operation is invalid if the graph is in-flight
        void Detach();

        // Invoke the job graph, asserting if there are dependency violations. Note that
        // submitting the same graph multiple times to process simultaneously is VALID
        // behavior. This is, for example, a mechanism that allows a job graph to loop
        // in perpetuity (in fact, the entire frame could be modeled as a single job graph,
        // where the final job resubmits the job graph again).
        //
        // This API is not designed to protect against memory safety violations (nothing
        // can prevent a user from incorrectly aliasing memory unsafely even without repeated
        // submission). To catch memory safety violations, it is ENCOURAGED that you access
        // data through JobResource<T> handles.
        void Submit(JobGraphEvent* waitEvent = nullptr);

        // Same as submit but run on a different executor than the default system executor
        void SubmitOnExecutor(JobExecutor& executor, JobGraphEvent* waitEvent = nullptr);

    private:
        friend class JobToken;
        friend class Internal::CompiledJobGraph;

        Internal::CompiledJobGraph* m_compiledJobGraph = nullptr;

        AZStd::vector<Internal::TypeErasedJob> m_jobs;

        // Job index |-> Dependent job indices
        AZStd::unordered_map<uint32_t, AZStd::vector<uint32_t>> m_links;

        uint32_t m_linkCount = 0;
        bool m_retained = true;
        AZStd::atomic<bool> m_submitted = false;
    };
} // namespace AZ

#include <AzCore/Jobs/JobGraph.inl>
