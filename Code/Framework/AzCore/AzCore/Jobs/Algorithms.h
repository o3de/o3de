/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_JOBS_ALGORITHMS_H
#define AZCORE_JOBS_ALGORITHMS_H 1

#include <AzCore/Jobs/task_group.h>
#include <AzCore/std/allocator_stack.h>

#include <AzCore/std/parallel/spin_mutex.h>

// A reasonable define for a stack allocator size for the high level jobs.
#define AZ_JOBS_DEFAULT_STACK_ALLOCATOR_SIZE AZStd::GetMax<unsigned>(2048,512 * AZStd::thread::hardware_concurrency())

// Enable this process iterations in slit ranges, this should improve cache access
// otherwise if not enabled all threads will work over the same area (which seems
// to yield similar results)
#define AZ_JOBS_CHUNK_JOB_RANGE_LOCK

namespace AZ
{
    namespace Internal
    {
        typedef int ParallelIndexType;

        /**
         * A helper jobs that will execute a chunk/range of iterations. Usually when we call ParallelFor... we will
         * create one chunk for each worker thread (or size requested by the user).
         * Of course each iteration may take different amount of time, so to improve load balance we do create an assist job(s) that is added to the queues.
         * If any thread picks it up, it will start assisting the current chunk jobs. That way chunks do not need to take
         * the same amount of time/CPU to parallel well.
         * Once the assist job has started it will steal half of the iterations left. The assistant job will have it's
         * assistant to process it's half of range (and going deeper if needed). For the first half of iteration we
         * spawn another assistant job to assist with it. This way we can have reasonable load balancing, while maintains
         * some locality of the processed iterations.
         */
        template<class Function, class Partition>
        class ParallelForChunkJob
            : public Job
        {
        public:
            AZ_CLASS_ALLOCATOR(ParallelForChunkJob, ThreadPoolAllocator);

            typedef ParallelForChunkJob<Function, Partition> ThisType;

            ParallelForChunkJob(ParallelIndexType start, ParallelIndexType end, ParallelIndexType step, const Function& function,
                ThisType* assisting, JobContext* jobContext, bool isAutoDelete)
                : Job(isAutoDelete, jobContext)
#ifdef AZ_JOBS_CHUNK_JOB_RANGE_LOCK
                , m_rangeLock(true)
#endif
                , m_current(start)
                , m_end(end)
                , m_step(step)
                , m_function(function)
                , m_assisting(assisting)
            {
            }

            virtual void Process()
            {
                if (Partition::s_isSpawnAssistJob)
                {
#ifdef AZ_JOBS_CHUNK_JOB_RANGE_LOCK
                    if (m_assisting)  // check if we assist and if the main job was done
                    {
                        ParallelIndexType numIterationsLeftToAssist;
                        {
                            // m_assisting->m_rangeLock.lock() uses back of we never hold to the lock a long time (just an addition)
                            while (!m_assisting->m_rangeLock.try_lock())
                            {
                                AZStd::this_thread::pause(1);
                            }

                            numIterationsLeftToAssist = (m_assisting->m_end - m_assisting->m_current) / (2 * m_step);
                            if (numIterationsLeftToAssist > 0)
                            {
                                m_current = m_assisting->m_current + numIterationsLeftToAssist * m_step;
                                m_end = m_assisting->m_end;
                                m_assisting->m_end = m_current;
                            }
                            else
                            {
                                // not enough iterations for a worker just stop it.
                                m_current = m_end;
                            }

                            // stop stealing
                            m_assisting->m_rangeLock.unlock();
                        }

                        if (m_current == m_end)
                        {
                            return; // we are done, no work range was set from the assisting job
                        }
                        // Here we have 2 choices:
                        // 1. We can either loop the entire assistant job, so when it's done asks for more, but the logic will get even more complicated.
                        // 2. Create another assistant job and just forget about current assistant and never interact with it.
                        // As of now 2 is simpler and when tested did not give much of a difference.

                        // create another assistant on the stack.
                        if (numIterationsLeftToAssist > 1)
                        {
                            ThisType* newAssistant = new(AZ_ALLOCA(sizeof(ThisType)))ThisType(m_end, m_end, m_step, m_function, m_assisting, m_context, false);
                            StartAsChild(newAssistant);
                        }
                    }

                    {
                        // spawn a helper job (before we execute the first function)
                        // to assist if there are any workers available.
                        // This will make sure we have good load balance.
                        ThisType assistChunkJob(m_end, m_end, m_step, m_function, this, m_context, false);
                        if (m_end - m_current > 1)
                        {
                            StartAsChild(&assistChunkJob);
                        }

                        for (; m_current < m_end; m_current += m_step)
                        {
                            // allow stealing from assistant threads
                            m_rangeLock.unlock();

                            // call the user function
                            m_function(m_current);

                            // disallow stealing from assistant threads
                            // m_assisting->m_rangeLock.lock() uses back of we never hold to the lock a long time (just an addition)
                            while (!m_rangeLock.try_lock())
                            {
                                AZStd::this_thread::pause(1);
                            }
                        }

                        m_current = m_end; // indicate that we are done
                        m_rangeLock.unlock();
                        WaitForChildren();
                    }
#else // AZ_JOBS_CHUNK_JOB_RANGE_LOCK
                    ParallelIndexType step = m_step;
                    ParallelIndexType currentElement;
                    AZStd::atomic<ParallelIndexType>* currentPtr = m_assisting ? &m_assisting->m_current : &m_current;

                    currentElement = currentPtr->fetch_add(step, AZStd::memory_order_acq_rel);

                    if (currentElement < m_end)
                    {
                        // spawn a helper job (before we execute the first function)
                        // to assist if there are any workers available.
                        // This will make sure we have good load balance.
                        ParallelForChunkJob<Function, Partition> assistChunkJob(m_end, m_end, step, m_function, this, m_context, false);
                        if (m_end - currentElement > 1)
                        {
                            StartAsChild(&assistChunkJob);
                        }

                        do
                        {
                            m_function(currentElement);
                            currentElement = currentPtr->fetch_add(step, AZStd::memory_order_acq_rel);
                        } while (currentElement < m_end);

                        WaitForChildren();
                    }
#endif // !AZ_JOBS_CHUNK_JOB_RANGE_LOCK
                }
                else
                {
                    ParallelIndexType currentElement = m_current;
                    ParallelIndexType end = m_end;
                    ParallelIndexType step = m_step;
                    while (currentElement < end)
                    {
                        m_function(currentElement);
                        currentElement += step;
                    }
                }
            }

        private:
#ifdef AZ_JOBS_CHUNK_JOB_RANGE_LOCK
            AZStd::spin_mutex                   m_rangeLock;
            ParallelIndexType                   m_current;
#else
            AZStd::atomic<ParallelIndexType>    m_current;
#endif // #ifdef AZ_JOBS_CHUNK_JOB_RANGE_LOCK
            ParallelIndexType                   m_end;
            ParallelIndexType                   m_step;
            const Function&                     m_function;
            ThisType*                           m_assisting;    ///< Pointer to the ChunkJob we are assisting. Valid only for assisting chunk jobs.
        };

        /**
         * Helper function that will split the [start,end] range into equal chunks. One chunk per worker thread, in attempt to balance the load
         * up front. Of course the ParallelForChunkJob will do it's load balancing too.
         */
        template<class Function, class Partition>
        inline void ParallelForLauncher(ParallelIndexType start, ParallelIndexType end, ParallelIndexType step, const Function& function, Job* dependent
            , const Partition& partition, JobContext* jobContext, AZStd::stack_allocator* allocator)
        {
            ParallelIndexType numToProcess = end - start;
            ParallelIndexType numChunks = partition.GetNumChunks(numToProcess, jobContext);
            // There is no real benefit creating bigger chunks by default
            const ParallelIndexType minIterationsPerChunk = 1 /*8*/;

            ParallelIndexType numIterationsLeft = (numToProcess + step - 1) / step;
            ParallelIndexType numIterationsPerChunk = AZStd::GetMax(numIterationsLeft / numChunks, minIterationsPerChunk);

            ParallelIndexType index = start;
            while (numIterationsLeft > 0)
            {
                ParallelIndexType chunkIterations = AZStd::GetMin(numIterationsLeft, numIterationsPerChunk);
                numIterationsLeft -= chunkIterations;

                //add stragglers to this chunk if there's not enough to form their own chunk
                if (numIterationsLeft < minIterationsPerChunk)
                {
                    chunkIterations += numIterationsLeft;
                    numIterationsLeft = 0;
                }

                typedef ParallelForChunkJob<Function, Partition> ChunkJobType;

                ChunkJobType* chunkJob;
                if (allocator)
                {
                    chunkJob = new(allocator->allocate(sizeof(ChunkJobType), AZStd::alignment_of<ChunkJobType>::value))ChunkJobType(index, index + (chunkIterations * step), step, function, nullptr, jobContext, false);
                }
                else
                {
                    chunkJob = aznew ChunkJobType(index, index + (chunkIterations * step), step, function, nullptr, jobContext, true);
                }

                chunkJob->SetDependent(dependent);
                chunkJob->Start();

                index += chunkIterations * step;
            }
        }

        /** This job has two purposes, it acts as the final empty job on which we block for completion, and it holds
         * the function instance for the loop, to which all the other jobs keep a reference.
         */
        template<class Function, class Partition>
        class ParallelForFinishJob
            : public JobEmpty
        {
        public:
            AZ_CLASS_ALLOCATOR(ParallelForFinishJob, ThreadPoolAllocator);

            ParallelForFinishJob(const Function& function, JobContext* jobContext, const Partition& partition, bool isAutoDelete)
                : JobEmpty(isAutoDelete, jobContext)
                , m_function(function)
                , m_partition(partition)
            {}
            void QueueWork(ParallelIndexType start, ParallelIndexType end, ParallelIndexType step, AZStd::stack_allocator* allocator)
            {
                Internal::ParallelForLauncher(start, end, step, m_function, this, m_partition, m_context, allocator);
            }
        private:
            Function m_function;
            Partition m_partition;
        };

        //
        //parallel_for_each jobs and helpers
        //

        /**
         * Forward iterator chunk/range helper job. We use stack memory and
         * a wait to process elements.
         */
        template<class ForwardIterator, class Function, class Partition>
        class ParallelForEachForwardFinishJobChunkHelper
            : public Job
        {
        public:
            static const int MaxNumberOfIterations = 512;  // max on 64 bit platform 4 KB of stack space

            typedef typename AZStd::iterator_traits<ForwardIterator>::value_type value_type;

            AZ_CLASS_ALLOCATOR(ParallelForEachForwardFinishJobChunkHelper, ThreadPoolAllocator);

            ParallelForEachForwardFinishJobChunkHelper(ForwardIterator start, ForwardIterator end, const Function& function, const Partition& partition, JobContext* jobContext, bool isAutoDelete)
                : Job(isAutoDelete, jobContext)
                , m_start(start)
                , m_end(end)
                , m_function(function)
                , m_partition(partition)
            {}

            void operator()(ParallelIndexType i) const
            {
                m_function(*m_elements[i]);
            }

        private:
            virtual void Process()
            {
                value_type* tempArray[MaxNumberOfIterations];
                m_elements = tempArray;
                ForwardIterator processed = m_start;

                ParallelIndexType numElements = 0;
                while (processed != m_end)  // no need to check numElements it's already checked in QueueWork
                {
                    tempArray[numElements++] = &(*processed);
                    ++processed;
                }

                AZ_STACK_ALLOCATOR(stackAllocator, AZ_JOBS_DEFAULT_STACK_ALLOCATOR_SIZE);
                JobEmpty finishJob(false, m_context);
                Internal::ParallelForLauncher(0, numElements, 1, *this, &finishJob, m_partition, m_context, &stackAllocator);
                StartAsChild(&finishJob);
                WaitForChildren();
            }

            ForwardIterator     m_start;
            ForwardIterator     m_end;
            value_type**        m_elements;
            const Function&     m_function;
            const Partition&    m_partition;
        };

        // this job has two purposes, it acts as the final empty job on which we block for completion, and it holds
        // the function instance for the loop. In addition we adapt the elements to a random access chunks, because
        // as of now we don't really support forward iteration at a lower level.
        template<class ForwardIterator, class Function, class Partition>
        class ParallelForEachForwardFinishJob
            : public JobEmpty
        {
        public:
            AZ_CLASS_ALLOCATOR(ParallelForEachForwardFinishJob, ThreadPoolAllocator);

            typedef ParallelForEachForwardFinishJobChunkHelper<ForwardIterator, Function, Partition> ChunkHelperJob;

            ParallelForEachForwardFinishJob(const Function& function, const Partition& partition, JobContext* jobContext, bool isAutoDelete)
                : JobEmpty(isAutoDelete, jobContext)
                , m_function(function)
                , m_partition(partition)
            {}

            void QueueWork(const ForwardIterator& start, const ForwardIterator& end, AZStd::stack_allocator* allocator)
            {
                if (start == end)
                {
                    return;
                }

                ForwardIterator queued = start;
                while (queued != end)
                {
                    int numElements = 0;
                    ForwardIterator newStart = queued;
                    while (queued != end && numElements < ChunkHelperJob::MaxNumberOfIterations)
                    {
                        ++queued;
                        ++numElements;
                    }

                    ChunkHelperJob* finishJob;
                    if (allocator)
                    {
                        finishJob = new(allocator->allocate(sizeof(ChunkHelperJob), AZStd::alignment_of<ChunkHelperJob>::value))ChunkHelperJob(newStart, queued, m_function, m_partition, m_context, false);
                    }
                    else
                    {
                        finishJob = aznew ChunkHelperJob(newStart, queued, m_function, m_partition, m_context, true);
                    }

                    finishJob->SetDependent(this);
                    finishJob->Start();
                }
            }

        private:
            Function        m_function;
            Partition       m_partition;
        };

        //this job has two purposes, it acts as the final empty job on which we block for completion, and it holds
        //the function instance for the loop.
        template<class RandomIterator, class Function, class Partition>
        class ParallelForEachRandomFinishJob
            : public JobEmpty
        {
        public:
            AZ_CLASS_ALLOCATOR(ParallelForEachRandomFinishJob, ThreadPoolAllocator);

            typedef typename AZStd::iterator_traits<RandomIterator>::value_type value_type;

            ParallelForEachRandomFinishJob(RandomIterator start, const Function& function, const Partition& partition, JobContext* jobContext, bool isAutoDelete)
                : JobEmpty(isAutoDelete, jobContext)
                , m_start(start)
                , m_function(function)
                , m_partition(partition)
            {
            }
            void QueueWork(ParallelIndexType numIterations, AZStd::stack_allocator* allocator)
            {
                Internal::ParallelForLauncher(0, numIterations, 1, *this, this, m_partition, m_context, allocator);
            }
            void operator()(ParallelIndexType i) const
            {
                m_function(m_start[i]);
            }
        private:
            RandomIterator m_start;
            Function m_function;
            Partition m_partition;
        };

        // Generic parallel for each with forward iterators
        template<class ForwardIterator, class IteratorCategory, class Function, class Partition>
        struct ParallelForEachHelper
        {
            typedef ParallelForEachForwardFinishJob<ForwardIterator, Function, Partition> FinishJobType;

            static void Run(ForwardIterator start, ForwardIterator end, const Function& function, Job* dependent, const Partition& partition, JobContext* jobContext, AZStd::stack_allocator* allocator)
            {
                FinishJobType* finishJob;
                if (allocator)
                {
                    finishJob = new(allocator->allocate(sizeof(FinishJobType), AZStd::alignment_of<FinishJobType>::value))FinishJobType(function, partition, jobContext, false);
                }
                else
                {
                    finishJob = aznew FinishJobType(function, partition, jobContext, true);
                }

                finishJob->QueueWork(start, end, allocator);
                finishJob->SetDependent(dependent);
                finishJob->Start();
            }
        };

        // Specialization for random access iterators, they don't need to pre-traverse and copy items
        template<class RandomIterator, class Function, class Partition>
        struct ParallelForEachHelper<RandomIterator, AZStd::random_access_iterator_tag, Function, Partition>
        {
            typedef ParallelForEachRandomFinishJob<RandomIterator, Function, Partition> FinishJobType;

            static void Run(RandomIterator start, RandomIterator end, const Function& function, Job* dependent, const Partition& partition, JobContext* jobContext, AZStd::stack_allocator* allocator)
            {
                size_t numIterations = end - start;

                FinishJobType* finishJob;
                if (allocator)
                {
                    finishJob = new(allocator->allocate(sizeof(FinishJobType), AZStd::alignment_of<FinishJobType>::value))FinishJobType(start, function, partition, jobContext, false);
                }
                else
                {
                    finishJob = aznew FinishJobType(start, function, partition, jobContext, true);
                }

                finishJob->QueueWork(static_cast<ParallelIndexType>(numIterations), allocator);
                finishJob->SetDependent(dependent);
                finishJob->Start();
            }
        };

    }

    /**
     *   Default partition algorithms. It will create one chunk for each worker thread, it will spawn assisting jobs
     *   for load balance (if iteration time is NOT the same) and we will process one task at a time
     **/
    struct auto_partitioner
    {
        static const bool s_isSpawnAssistJob = true;

        // number of iterations we process at a time from a single job/thread.
        inline Internal::ParallelIndexType GetWorkGrain()
        {
            return static_cast<Internal::ParallelIndexType>(1);
        }

        // How many jobs/chucks to spawn for numElementsToProcess elements.
        inline Internal::ParallelIndexType GetNumChunks(Internal::ParallelIndexType numElementsToProcess, JobContext* jobContext) const
        {
            (void)numElementsToProcess;
            return static_cast<Internal::ParallelIndexType>(jobContext->GetJobManager().GetNumWorkerThreads());
        }
    };

    /**
     * Same as \ref auto_partitioner, except it doesn't NOT spawn assisting jobs.
     */
    struct static_partitioner
    {
        static const bool s_isSpawnAssistJob = false;

        inline Internal::ParallelIndexType GetNumChunks(Internal::ParallelIndexType numElementsToProcess, JobContext* jobContext) const
        {
            (void)numElementsToProcess;
            return static_cast<Internal::ParallelIndexType>(jobContext->GetJobManager().GetNumWorkerThreads());
        }
    };

    /**
     * You control the chunk size (number of jobs spawned, since we do one per chunk), no assisting jobs are spawned
     */
    struct simple_partitioner
    {
        static const bool s_isSpawnAssistJob = false;

        explicit simple_partitioner(Internal::ParallelIndexType chunkSize)
            : m_chunkSize(chunkSize)
        {
            AZ_Assert(m_chunkSize > 1, "Chunk size must be > 0");
        }

        inline Internal::ParallelIndexType GetNumChunks(Internal::ParallelIndexType numElementsToProcess, JobContext* jobContext) const
        {
            (void)jobContext;
            Internal::ParallelIndexType numChunks = numElementsToProcess / m_chunkSize;
            if (numChunks == 0)
            {
                numChunks = 1;
            }
            return numChunks;
        }

        Internal::ParallelIndexType m_chunkSize;
    };

    /**
     * TODO
     */
    //struct affinity_partitioner
    //{

    //};

    /**
     * Parallel for loop over a range with a step. The function must have a single int parameter and return void. This
     * function will block until the loop is complete. It can be called from within a job, or from a non-worker thread,
     * in which case the thread will block on a completion event. Works with both small and large ranges, the
     * iterations will be distributed efficiently in either case.
     */
    template<class IndexType, class Function, class Partition>
    void parallel_for(IndexType start, IndexType end, IndexType step, const Function& function, const Partition& partition, JobContext* jobContext = nullptr)
    {
        JobContext* context = jobContext ? jobContext : JobContext::GetParentContext();

        Internal::ParallelForFinishJob<Function, Partition> finishJob(function, context, partition, false);

        AZ_STACK_ALLOCATOR(stackAllocator, AZ_JOBS_DEFAULT_STACK_ALLOCATOR_SIZE);

        finishJob.QueueWork(static_cast<Internal::ParallelIndexType>(start), static_cast<Internal::ParallelIndexType>(end), static_cast<Internal::ParallelIndexType>(step), &stackAllocator);

        finishJob.StartAndWaitForCompletion();
    }

    template<class IndexType, class Function>
    void parallel_for(IndexType start, IndexType end, IndexType step, const Function& function, JobContext* jobContext = nullptr)
    {
        parallel_for(start, end, step, function, auto_partitioner(), jobContext);
    }

    /**
     * Parallel for loop over a range. The function must have a single int parameter and return void. This
     * function will block until the loop is complete. It can be called from within a job, or from a non-worker thread,
     * in which case the thread will block on a completion event. Works with both small and large ranges, the
     * iterations will be distributed efficiently in either case.
     */
    template<class IndexType, class Function, class Partition>
    void parallel_for(IndexType start, IndexType end, const Function& function, const Partition& partition, JobContext* jobContext = nullptr)
    {
        parallel_for(static_cast<Internal::ParallelIndexType>(start), static_cast<Internal::ParallelIndexType>(end), static_cast<Internal::ParallelIndexType>(1), function, partition, jobContext);
    }
    template<class IndexType, class Function>
    void parallel_for(IndexType start, IndexType end, const Function& function, JobContext* jobContext = nullptr)
    {
        parallel_for(static_cast<Internal::ParallelIndexType>(start), static_cast<Internal::ParallelIndexType>(end), static_cast<Internal::ParallelIndexType>(1), function, auto_partitioner(), jobContext);
    }

    /**
     * Parallel for loop over a range with a step. The function must have a single int parameter and return void. This
     * function will return immediately once the loop has been started, so you must supply a dependent job which will
     * be notified when the loop completes. It can be called from within a job, or from a non-worker thread.
     * Works with both small and large ranges, the iterations will be distributed efficiently in either case.
     */
    template<class IndexType, class Function, class Partition>
    void parallel_for_start(IndexType start, IndexType end, IndexType step, const Function& function, Job* dependent, const Partition& partition, JobContext* jobContext = nullptr, AZStd::stack_allocator* allocator = nullptr)
    {
        JobContext* context = jobContext ? jobContext : JobContext::GetParentContext();

        typedef Internal::ParallelForFinishJob<Function, Partition> FinishJobType;

        FinishJobType* finishJob;
        if (allocator)
        {
            finishJob = new(allocator->allocate(sizeof(FinishJobType), AZStd::alignment_of<FinishJobType>::value))FinishJobType(function, context, partition, false);
        }
        else
        {
            finishJob = aznew FinishJobType(function, context, partition, true);
        }

        finishJob->QueueWork(static_cast<Internal::ParallelIndexType>(start), static_cast<Internal::ParallelIndexType>(end), static_cast<Internal::ParallelIndexType>(step), allocator);

        finishJob->SetDependent(dependent);
        finishJob->Start();
    }

    template<class IndexType, class Function>
    void parallel_for_start(IndexType start, IndexType end, IndexType step, const Function& function, Job* dependent, JobContext* jobContext = nullptr, AZStd::stack_allocator* allocator = nullptr)
    {
        parallel_for_start(start, end, step, function, dependent, auto_partitioner(), jobContext, allocator);
    }

    /**
     * Parallel for loop over a range. The function must have a single int parameter and return void. This
     * function will return immediately once the loop has been started, so you must supply a dependent job which will
     * be notified when the loop completes. It can be called from within a job, or from a non-worker thread.
     * Works with both small and large ranges, the iterations will be distributed efficiently in either case.
     */
    template<class IndexType, class Function, class Partition>
    void parallel_for_start(IndexType start, IndexType end, const Function& function, Job* dependent, const Partition& partition, JobContext* jobContext = nullptr, AZStd::stack_allocator* allocator = nullptr)
    {
        parallel_for_start(static_cast<Internal::ParallelIndexType>(start), static_cast<Internal::ParallelIndexType>(end), static_cast<Internal::ParallelIndexType>(1), function, dependent, partition, jobContext, allocator);
    }
    template<class IndexType, class Function>
    void parallel_for_start(IndexType start, IndexType end, const Function& function, Job* dependent, JobContext* jobContext = nullptr, AZStd::stack_allocator* allocator = nullptr)
    {
        parallel_for_start(static_cast<Internal::ParallelIndexType>(start), static_cast<Internal::ParallelIndexType>(end), static_cast<Internal::ParallelIndexType>(1), function, dependent, auto_partitioner(), jobContext, allocator);
    }

    /**
     * Parallel for loop over a range of items. The function must have a single parameter of type T, the iterator
     * value type, and return void. This function will block until the loop is complete. It can be called from within
     * a job, or from a non-worker thread, in which case the thread will block on a completion event. Works with both
     * small and large ranges, the iterations will be distributed efficiently in either case. It works with both
     * forward iterators and random access iterators, but performance will be better with random access iterators.
     */
    template<class Iterator, class Function, class Partition>
    void parallel_for_each(Iterator start, Iterator end, const Function& function, const Partition& partition, JobContext* jobContext = nullptr)
    {
        JobContext* context = jobContext ? jobContext : JobContext::GetParentContext();

        JobEmpty finishJob(false, context);

        AZ_STACK_ALLOCATOR(stackAllocator, AZ_JOBS_DEFAULT_STACK_ALLOCATOR_SIZE);

        Internal::ParallelForEachHelper<Iterator, typename AZStd::iterator_traits<Iterator>::iterator_category, Function, Partition>
            ::Run(start, end, function, &finishJob, partition, context, &stackAllocator);

        finishJob.StartAndWaitForCompletion();
    }

    template<class Iterator, class Function>
    void parallel_for_each(Iterator start, Iterator end, const Function& function,  JobContext* jobContext = nullptr)
    {
        parallel_for_each(start, end, function, auto_partitioner(), jobContext);
    }

    /**
     * Parallel for loop over a range of items. The function must have a single parameter of type T, the iterator
     * value type, and return void. This function will return immediately once the loop has been started, so you must
     * supply a dependent job which will be notified when the loop completes. It can be called from within
     * a job, or from a non-worker thread. Works with both small and large ranges, the iterations will be distributed
     * efficiently in either case. It works with both forward iterators and random access iterators, but performance
     * will be better with random access iterators.
     */
    template<class Iterator, class Function, class Partition>
    void parallel_for_each_start(Iterator start, Iterator end, const Function& function, Job* dependent, const Partition& partition, JobContext* jobContext = nullptr, AZStd::stack_allocator* allocator = nullptr)
    {
        JobContext* context = jobContext ? jobContext : JobContext::GetParentContext();

        Internal::ParallelForEachHelper<Iterator, typename AZStd::iterator_traits<Iterator>::iterator_category, Function, Partition>
            ::Run(start, end, function, dependent, partition, context, allocator);
    }
    template<class Iterator, class Function>
    void parallel_for_each_start(Iterator start, Iterator end, const Function& function, Job* dependent, JobContext* jobContext = nullptr, AZStd::stack_allocator* allocator = nullptr)
    {
        parallel_for_each_start(start, end, function, dependent, auto_partitioner(), jobContext, allocator);
    }

    /**
     * Invokes the specified functions in parallel and waits until they are all complete. Overloads for up to 8
     * function parameters are provided.
     */
    template<typename F1, typename F2>
    void parallel_invoke(const F1& f1, const F2& f2, JobContext* jobContext = nullptr)
    {
        structured_task_group group(jobContext);
        group.run(f1);
        group.run_and_wait(f2);
    }

    template<typename F1, typename F2, typename F3>
    void parallel_invoke(const F1& f1, const F2& f2, const F3& f3, JobContext* jobContext = nullptr)
    {
        structured_task_group group(jobContext);
        group.run(f1);
        group.run(f2);
        group.run_and_wait(f3);
    }

    template<typename F1, typename F2, typename F3, typename F4>
    void parallel_invoke(const F1& f1, const F2& f2, const F3& f3, const F4& f4, JobContext* jobContext = nullptr)
    {
        structured_task_group group(jobContext);
        group.run(f1);
        group.run(f2);
        group.run(f3);
        group.run_and_wait(f4);
    }

    template<typename F1, typename F2, typename F3, typename F4, typename F5>
    void parallel_invoke(const F1& f1, const F2& f2, const F3& f3, const F4& f4, const F5& f5, JobContext* jobContext = nullptr)
    {
        structured_task_group group(jobContext);
        group.run(f1);
        group.run(f2);
        group.run(f3);
        group.run(f4);
        group.run_and_wait(f5);
    }

    template<typename F1, typename F2, typename F3, typename F4, typename F5, typename F6>
    void parallel_invoke(const F1& f1, const F2& f2, const F3& f3, const F4& f4, const F5& f5, const F6& f6, JobContext* jobContext = nullptr)
    {
        structured_task_group group(jobContext);
        group.run(f1);
        group.run(f2);
        group.run(f3);
        group.run(f4);
        group.run(f5);
        group.run_and_wait(f6);
    }

    template<typename F1, typename F2, typename F3, typename F4, typename F5, typename F6, typename F7>
    void parallel_invoke(const F1& f1, const F2& f2, const F3& f3, const F4& f4, const F5& f5, const F6& f6, const F7& f7, JobContext* jobContext = nullptr)
    {
        structured_task_group group(jobContext);
        group.run(f1);
        group.run(f2);
        group.run(f3);
        group.run(f4);
        group.run(f5);
        group.run(f6);
        group.run_and_wait(f7);
    }

    template<typename F1, typename F2, typename F3, typename F4, typename F5, typename F6, typename F7, typename F8>
    void parallel_invoke(const F1& f1, const F2& f2, const F3& f3, const F4& f4, const F5& f5, const F6& f6, const F7& f7, const F8& f8, JobContext* jobContext = nullptr)
    {
        structured_task_group group(jobContext);
        group.run(f1);
        group.run(f2);
        group.run(f3);
        group.run(f4);
        group.run(f5);
        group.run(f6);
        group.run(f7);
        group.run_and_wait(f8);
    }
}

#endif
#pragma once
