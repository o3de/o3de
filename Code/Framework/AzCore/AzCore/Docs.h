/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace AZ
{
    /**
     * \mainpage
     * Welcome to the AZCore library.
     *
     * Check the latest \ref ReleaseNotes "release notes" for this version of AZCore.
     *
     * \li \subpage MemoryManagers
     * \li \subpage JobSystem
     *
     */

    /**
     * \page MemoryManagers Memory Managers
     *
     * todo
     */

    /**
     * \page JobSystem Job System
     *
     * \li \ref BasicUsage
     * \li \ref DesigningGoodJobs
     * \li \ref Techniques
     * \li \ref Examples
     *
     * \section BasicUsage Basic job system usage
     *
     * \subsection Setup Setup
     * Before using jobs, a JobManager instance must be created. This is responsible for scheduling and running the jobs.
     * Multiple JobManagers are supported, but in most cases a single JobManager will suffice.
     *
     * When creating a JobManager, the JobManagerDesc is used to specify the worker threads to create with the JobManager.
     * The details of this depend on the platform. For single-core platforms, i.e. Wii, no worker threads are
     * allowed (job processing will be synchronous on these platforms, and is only provided in order to simplify user
     * code). On PC multiple worker threads are supported, on some platforms the core for each worker thread can also be specified.
     *
     * Optionally the global job context can also be specified, with JobContext::SetGlobalContext(). This is the execution
     * context that will be used for jobs when a context is not explicitly specified. If you have only one JobManager in
     * your application then it can be convenient to set the global context.
     *
     * \subsection Creating Creating jobs
     * Multiple job types are supported, with different methods of specifying their payloads. All jobs take two standard
     * parameters, isAutoDelete, explained below, and the JobContext to use for executing the job. If the context is NULL
     * then the global context will be used to find the parent context.
     *
     * \li <b>JobFunction</b> - This uses AZStd::function to allow any callable object to be used as the payload.
     * \li <b>JobDelegate</b> - Similar to JobFunction, but uses AZStd::delegate, can be slightly more efficient sometimes.
     * \li <b>JobUser</b> - This provides a base class from which the user may derive to implement the payload.
     *
     * \subsection AutoDelete Auto-deletion of jobs
     * Allowing auto-deletion for jobs is the recommended behavior, as it simplifies their usage considerably. An
     * auto-deletion job must have been allocated using aznew. Jobs use an efficient ThreadPoolAllocator as their default
     * allocator, so allocation is efficient, and does not require any synchronization.
     *
     * If you choose to not use auto-deletion, then the jobs must be manually reset with Job::Reset before they can be
     * re-used. Also any job dependencies which were setup must be set again. Care must also be taken when deleting the
     * job, to ensure it is not being used by the JobManager.
     *
     * \subsection Starting Starting jobs
     * A job is started and released for execution by calling Job::Start(). After the job has been started, it is forbidden
     * to access or use the job in any way! This is because once started, the job may be being processed and even deleted
     * before your attempt to access it. Jobs may be started by any thread, including from inside another a worker thread
     * (this is actually the most efficient way to spawn a job!)
     *
     * \subsection Waiting Waiting for jobs
     * After a user (non-worker) thread has started some jobs, it often wants to block until the jobs have finished. The
     * only safe way to do this is by using a JobCompletion job. The JobCompletion job should be set as the final
     * dependent in the series of jobs, and then the user thread can block by calling
     * JobCompletion::StartAndWaitForCompletion().
     *
     * It is also possible for a currently processing job to suspend itself until its child jobs have completed. The child
     * jobs must be started by the parent job by calling Job::StartAsChild(), and then the parent can suspend until they
     * are complete by calling Job::WaitForChildren(). Note that there is a limit to the depth which jobs may be nested,
     * the stack space can be exhausted quite quickly if there is a lot of nesting. There is also a very slight performance
     * penalty to using child jobs, as the parent job can only resume on the same thread from which it was started. If
     * either of these issues are a concern, explicit dependencies and join jobs can be used instead, see \ref Techniques.
     *
     * Finally, it is possible for a user thread to assist in job processing while waiting for a job to complete, by
     * calling Job::StartAndAssistUntilComplete(). This is not usually recommended however, it is better for the user
     * thread to perform other unrelated processing, or for more processing to be moved into the job system. Also, if a
     * worker thread is available on the same core as the user thread, then there is no penalty to just blocking execution
     * with a JobCompletion.
     *
     * \section DesigningGoodJobs Designing good jobs
     *
     * \subsection JobSize Job size
     * The job system is designed for fine-grained job processing. The amount of overhead for each job is kept to a
     * minimum, often the next job can be popped from the queue with no synchronization at all, and the call to the payload
     * processing is just a single virtual function call or function pointer call.
     *
     * So jobs should be kept quite small in general. The advantage to small jobs is that they will utilize all the
     * available cores more efficiently, and can scale easily to larger numbers of cores. A big job can end up blocking
     * other cores which are waiting for it to complete, the other cores may even just run out of work, and then all
     * cores end up waiting for the single remaining job to complete.
     *
     * \subsection Synchronization Synchronization
     * Synchronization between threads should be avoided as much as possible in job processing functions. It is often
     * surprising just how much a single point of synchronization between jobs can impact performance. The fork/join
     * processing model is a good way to avoid synchronization. The parallel containers in AZStd can also be an option.
     *
     * A common synchronization point, which is often overlooked, is memory allocation. If you must allocate memory from
     * inside a job, consider using one of the thread-local allocators which will not synchronize, e.g.
     * ThreadPoolAllocator.
     *
     * \section Techniques Job design techniques
     * Job structure will usually be based on the fork/join model. Jobs will spawn other jobs, i.e. forking, and eventually
     * they will coalesce back, i.e. joining. There are several techniques available in the job system to implement this.
     *
     * \subsection TaskGroup Using task_groups
     * This is the highest level interface available in the job system. It is a pseudo-standard interface, similar to the
     * interface available in both Visual Studio 2010 and the latest versions of Intel's Threading Building Blocks.
     *
     * It is very simple to use, just create a structured_task_group, then fork processing to any function by calling
     * structured_task_group::run() as many times as desired, and then block until all processing is complete by calling
     * structured_task_group::wait(). The usage is identical whether it is used on a user thread or a worker thread.
     *
     * The internal implementation is using child jobs, so it has the same limitations as child jobs, described below.
     *
     * \subsection ChildJobs Using child jobs
     * Child jobs allow the parent to suspend execution until the child jobs are complete. This allows the 'join' logic
     * to be placed in the same function, without needing to implement a separate 'join' job.
     *
     * Child jobs must be started using Job::StartAsChild() from within the parent job. The parent job may then suspend
     * until all its children are complete by calling Job::WaitForChildren().
     *
     * Note that there are some issues to be aware of when using child jobs. There is a limit to the depth which jobs may
     * be nested, as the available stack memory can be exhausted quite quickly if there is a lot of nesting. There is also
     * a very slight performance penalty to using child jobs, as the parent job can only resume on the same thread from
     * which it was started. If either of these issues are a concern, explicit dependencies and join jobs can be used
     * instead as described in the next section.
     *
     * \subsection ExplicitDependencies Using explicit dependencies with continuations
     * Job dependencies can also be managed explicitly. This is the lowest-level and also the most efficient interface
     * available in the job system.
     *
     * A dependency can be set by calling Job::SetDependent(). A dependent job will not run until all of the jobs which
     * have specified it as a dependent have completed. Dependencies can only be set before both the jobs have started.
     *
     * A job can have only one dependent, but multiple jobs can specify the same job as their dependent. This is usually
     * sufficient as dependencies are usually used to implement the 'join' part of the fork/join model. The fork part can
     * be easily implemented by just spawning the forked jobs directly, dependencies are not necessary. If it is absolutely
     * necessary to have a many-to-many dependency relationship, then JobMultipleDependent can be used, please consider
     * re-organizing your job structure instead before using this though.
     *
     * \subsection Continuations Continuations
     * After a job has spawned its forked jobs and join job, its execution is finished. But this often leads to a problem,
     * as whoever spawned the current job may have specified a dependent to run after the current job is complete. But the
     * current job will not conceptually be complete until the join job has finished. The Job::SetContinuation function
     * allows the current job to specify that the join job is a 'continuation' of the current job, and any dependency of
     * the current job should not actually be ran until the join job is complete also.
     *
     * \section Examples Examples
     * See \subpage JobExamples "Job system examples" for examples of job system usage.
     *
     */

    /**
     * \page JobExamples Job system examples
     *
     * \dontinclude jobs.cpp
     *
     * \li \ref JobTypesExample
     * \li \ref ExplicitDependenciesExample
     * \li \ref ChildJobsExample
     * \li \ref MergeSortJobExample
     * \li \ref QuickSortJobExample
     * \li \ref TaskGroupJobExample
     *
     * \section JobTypesExample Using the different job types, with a JobUser example
     * Shows the same job implemented using JobFunction, JobDelegate, and JobUser.
     *
     * \skip // BasicJobExample-Begin
     * \until // BasicJobExample-End
     *
     * \section ExplicitDependenciesExample Fork/join model using explicit dependencies and continuations
     *
     * \skip // FibonacciJobExample-Begin
     * \until // FibonacciJobExample-End
     *
     * \section ChildJobsExample Fork/join model using child jobs
     *
     * \skip // FibonacciJob2Example-Begin
     * \until // FibonacciJob2Example-End
     *
     * \section MergeSortJobExample Merge sort implemented using jobs
     * A merge sort, implemented using child jobs. Note there are more efficient parallel sorting algorithms.
     *
     * \skip // MergeSortJobExample-Begin
     * \until // MergeSortJobExample-End
     *
     * \section QuickSortJobExample Quick sort implemented using continuations only
     * A quick sort, using only dependencies and continuations, as the quick sort algorithm does not require a 'join'
     * step. Note there are more efficient parallel sorting algorithms.
     *
     * \skip // QuickSortJobExample-Begin
     * \until // QuickSortJobExample-End
     *
     * \section TaskGroupJobExample Task group example
     * Example of structured_task_group usage, illustrates the ease of use.
     *
     * \skip // TaskGroupJobExample-Begin
     * \until // TaskGroupJobExample-End
     *
     */
}
