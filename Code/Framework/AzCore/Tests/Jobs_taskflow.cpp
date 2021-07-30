/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Jobs/taskflow/taskflow.hpp>
#include <AzCore/std/delegate/delegate.h>
#include <AzCore/std/bind/bind.h>

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Random.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/fixed_list.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/parallel/containers/concurrent_vector.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/PoolAllocator.h>

#include <AzCore/std/time.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <random>

#if AZ_TRAIT_SUPPORTS_MICROSOFT_PPL
// Enable this to test against Microsoft PPL, keep in mind you MUST have Exceptions enabled to use PPL
//# define AZ_COMPARE_TO_PPL
#endif //
//#define AZ_JOBS_PRINT_CALL_ORDER

#if defined(AZ_COMPARE_TO_PPL)
#   include <ppl.h>
#endif // AZ_COMPARE_TO_PPL

using namespace AZ;
using namespace AZ::Debug;

namespace UnitTest
{
    static const int g_tfFibonacciFast = 10;
    static const int g_tfFibonacciFastResult = 55;
#ifdef _DEBUG
    static const int g_tfFibonacciSlow = 15;
    static const int g_tfFibonacciSlowResult = 610;
#else
    static const int g_tfFibonacciSlow = 20;
    static const int g_tfFibonacciSlowResult = 6765;
#endif

    static AZStd::sys_time_t s_totalTaskflowTime = 0;

    class DefaultTaskflowExecutorSetupFixture
        : public AllocatorsTestFixture

    {
    protected:
        tf::Executor* m_executor = nullptr;
        unsigned int m_numWorkerThreads;
    public:
        DefaultTaskflowExecutorSetupFixture(unsigned int numWorkerThreads = 0)
            : m_numWorkerThreads(numWorkerThreads)
        {
        }

        void SetUp() override
        {
            AllocatorsTestFixture::SetUp();

            AllocatorInstance<PoolAllocator>::Create();
            AllocatorInstance<ThreadPoolAllocator>::Create();

            if (m_numWorkerThreads == 0)
            {
                m_numWorkerThreads = AZStd::thread::hardware_concurrency();
            }

            m_executor = new tf::Executor(m_numWorkerThreads);
        }
        
        void TearDown() override
        {
            delete m_executor;

            AllocatorInstance<ThreadPoolAllocator>::Destroy();
            AllocatorInstance<PoolAllocator>::Destroy();

            AllocatorsTestFixture::TearDown();
        }
    };

    // BasicJobExample-Begin
    void TaskflowVector3Sum(const Vector3* array, unsigned int size, Vector3* result)
    {
        Vector3 sum = Vector3::CreateZero();
        for (unsigned int i = 0; i < size; ++i)
        {
            sum += array[i];
        }
        *result = sum;
    }

    class TaskflowVector3SumDelegate
    {
    public:
        TaskflowVector3SumDelegate(const Vector3* array, unsigned int size)
            : m_array(array)
            , m_size(size)
        {
        }
        void Process()
        {
            m_result = Vector3::CreateZero();
            for (unsigned int i = 0; i < m_size; ++i)
            {
                m_result += m_array[i];
            }
        }
        const Vector3& GetResult() const { return m_result; }
    private:
        const Vector3* m_array;
        unsigned int m_size;
        Vector3 m_result;
    };

    struct TaskflowVector3SumFunctor
    {
        TaskflowVector3SumFunctor(const Vector3* array, unsigned int size, Vector3* result)
            : m_array(array)
            , m_size(size)
            , m_result(result)
        {
        }
        void operator()()
        {
            Vector3 sum = Vector3::CreateZero();
            for (unsigned int i = 0; i < m_size; ++i)
            {
                sum += m_array[i];
            }
            * m_result = sum;
        }
    private:
        const Vector3* m_array;
        unsigned int m_size;
        Vector3* m_result;
    };

    class TaskflowVector3SumJob
    {
    public:
        AZ_CLASS_ALLOCATOR(TaskflowVector3SumJob, ThreadPoolAllocator, 0)

        TaskflowVector3SumJob(const Vector3* array, unsigned int size, Vector3* result)
            : m_array(array)
            , m_size(size)
            , m_result(result)
        {
        }
        void operator() ()
        {
            Vector3 sum = Vector3::CreateZero();
            for (unsigned int i = 0; i < m_size; ++i)
            {
                sum += m_array[i];
            }
            *m_result = sum;
        }
    private:
        const Vector3* m_array;
        unsigned int m_size;
        Vector3* m_result;
    };

    class TaskflowBasicTest
        : public DefaultTaskflowExecutorSetupFixture
    {
    public:
        void run()
        {
            const AZStd::fixed_vector<Vector3, 100> vecArray(100, Vector3::CreateOne());

            AZStd::sys_time_t tStart = AZStd::GetTimeNowMicroSecond();

            //test user jobs
            {
                Vector3 result;
                tf::Taskflow taskflow;
                taskflow.emplace(TaskflowVector3SumJob(&vecArray[0], (unsigned int)vecArray.size(), &result));
                m_executor->run(taskflow).wait();
                AZ_TEST_ASSERT(result.IsClose(Vector3(100.0f, 100.0f, 100.0f)));
            }

            //test function jobs
            {
                Vector3 result;
                tf::Taskflow taskflow;
                taskflow.emplace([&vecArray, &result](){TaskflowVector3Sum( &vecArray[0], (unsigned int)vecArray.size(), &result);});
                m_executor->run(taskflow).wait();
                AZ_TEST_ASSERT(result.IsClose(Vector3(100.0f, 100.0f, 100.0f)));
            }

            //test delegate jobs
            {
                TaskflowVector3SumDelegate sumDelegate(&vecArray[0], (unsigned int)vecArray.size());
                tf::Taskflow taskflow;
                taskflow.emplace(AZStd::make_delegate(&sumDelegate, &TaskflowVector3SumDelegate::Process));
                m_executor->run(taskflow).wait();
                AZ_TEST_ASSERT(sumDelegate.GetResult().IsClose(Vector3(100.0f, 100.0f, 100.0f)));
            }

            //test generic jobs
            {
                Vector3 result;
                TaskflowVector3SumFunctor sumFunctor(&vecArray[0], (unsigned int)vecArray.size(), &result);
                tf::Taskflow taskflow;
                taskflow.emplace(sumFunctor);
                m_executor->run(taskflow).wait();
                AZ_TEST_ASSERT(result.IsClose(Vector3(100.0f, 100.0f, 100.0f)));
            }

            s_totalTaskflowTime += AZStd::GetTimeNowMicroSecond() - tStart;
        }
    };

#if AZ_TRAIT_DISABLE_FAILED_JOB_BASIC_TESTS
    TEST_F(TaskflowBasicTest, DISABLED_Test)
#else
    TEST_F(TaskflowBasicTest, Test)
#endif // AZ_TRAIT_DISABLE_FAILED_JOB_BASIC_TESTS
    {
        run();
    }
    // BasicJobExample-End

    struct FibonacciContext
    {
        int m_value1;
        int m_value2;
    };

    // FibonacciTaskflowExample-Begin
    class FibonacciTaskflowJoin
    {
    public:
        AZ_CLASS_ALLOCATOR(FibonacciTaskflowJoin, ThreadPoolAllocator, 0)

        FibonacciTaskflowJoin(int* result, FibonacciContext* context)
            : m_result(result)
            , m_context(context)
        {
        }
        ~FibonacciTaskflowJoin()
        {
            delete m_context;
            m_context = nullptr;
        }
        void operator() ()
        {
            *m_result = m_context->m_value1 + m_context->m_value2;
        }
        int* m_result;
        FibonacciContext* m_context;
    };

    class FibonacciTaskflowFork
    {
    public:
        AZ_CLASS_ALLOCATOR(FibonacciTaskflowFork, ThreadPoolAllocator, 0)

        FibonacciTaskflowFork(int n, int* result)
            : m_n(n)
            , m_result(result)
        {
        }
        void operator() (tf::Subflow& subflow)
        {
            AZStd::sys_time_t tStart = AZStd::GetTimeNowMicroSecond();

            //this is a spectacularly inefficient way to compute a Fibonacci number, just an example to test the jobs
            if (m_n < 2)
            {
                *m_result = m_n;
            }
            else
            {
                FibonacciContext* context = new FibonacciContext; // FibonacciTaskflowJoin controls lifetime of this allocation
                tf::Task jobJoin= subflow.emplace(FibonacciTaskflowJoin(m_result, context));
                tf::Task job1   = subflow.emplace(FibonacciTaskflowFork(m_n - 1, &context->m_value1));
                tf::Task job2   = subflow.emplace(FibonacciTaskflowFork(m_n - 2, &context->m_value2));
                job1.precede(jobJoin);
                job2.precede(jobJoin);
            }

            s_totalTaskflowTime += AZStd::GetTimeNowMicroSecond() - tStart;
        }
    private:
        int m_n;
        int* m_result;
    };

    class TaskflowFibonacciTest
        : public DefaultTaskflowExecutorSetupFixture
    {
    public:
        //AZ_CLASS_ALLOCATOR(TaskflowFibonacciTest, ThreadPoolAllocator, 0)

        void run()
        {
            AZStd::sys_time_t tStart = AZStd::GetTimeNowMicroSecond();

            int result = 0;
            tf::Taskflow taskflow;
            taskflow.emplace(FibonacciTaskflowFork(g_tfFibonacciSlow, &result));
            m_executor->run(taskflow).wait();
            AZ_TEST_ASSERT(result == g_tfFibonacciSlowResult);

            s_totalTaskflowTime += AZStd::GetTimeNowMicroSecond() - tStart;
        }
    };

    TEST_F(TaskflowFibonacciTest, Test)
    {
        run();
    }
    // FibonacciTaskflowExample-End

    // FibonacciTaskflow2Example-Begin
    class FibonacciTaskflow2
    {
    public:
        AZ_CLASS_ALLOCATOR(FibonacciTaskflow2, ThreadPoolAllocator, 0)

        FibonacciTaskflow2(int n, int* result)
            : m_n(n)
            , m_result(result)
        {
        }
        void operator() (tf::Subflow& subflow)
        {
            AZStd::sys_time_t tStart = AZStd::GetTimeNowMicroSecond();

            //this is a spectacularly inefficient way to compute a Fibonacci number, just an example to test the jobs
            if (m_n < 2)
            {
                *m_result = m_n;
            }
            else
            {
                int result1 = 0;
                int result2 = 0;
                tf::Task job1 = subflow.emplace(FibonacciTaskflow2(m_n - 1, &result1));
                tf::Task job2 = subflow.emplace(FibonacciTaskflow2(m_n - 2, &result2));
                subflow.join();
                *m_result = result1 + result2;
            }

            s_totalTaskflowTime += AZStd::GetTimeNowMicroSecond() - tStart;
        }
    private:
        int m_n;
        int* m_result;
    };

    class TaskflowFibonacci2Test
        : public DefaultTaskflowExecutorSetupFixture
    {
    public:
        void run()
        {
            AZStd::sys_time_t tStart = AZStd::GetTimeNowMicroSecond();

            int result = 0;
            tf::Taskflow taskflow;
            taskflow.emplace(FibonacciTaskflow2(g_tfFibonacciFast, &result));
            m_executor->run(taskflow).wait();
            AZ_TEST_ASSERT(result == g_tfFibonacciFastResult);

            s_totalTaskflowTime += AZStd::GetTimeNowMicroSecond() - tStart;
        }
    };

    TEST_F(TaskflowFibonacci2Test, Test)
    {
        run();
    }
    // FibonacciTaskflow2Example-End

    // MergeSortTaskflowExample-Begin
    class MergeSortTaskflowJoin
    {
    public:
        AZ_CLASS_ALLOCATOR(MergeSortTaskflowJoin, ThreadPoolAllocator, 0)

        MergeSortTaskflowJoin(int* array, int* tempArray, int size1, int size2)
            : m_array(array)
            , m_tempArray(tempArray)
            , m_size1(size1)
            , m_size2(size2)
        {
        }
        void operator() ()
        {
            //merge
            int pos1 = 0;
            int pos2 = 0;
            int* array1 = &m_array[0];
            int* array2 = &m_array[m_size1];
            int* tempPtr = m_tempArray;
            while ((pos1 < m_size1) && (pos2 < m_size2))
            {
                if (array1[pos1] < array2[pos2])
                {
                    *tempPtr = array1[pos1++];
                }
                else
                {
                    *tempPtr = array2[pos2++];
                }
                ++tempPtr;
            }
            while (pos1 < m_size1)
            {
                *tempPtr = array1[pos1++];
                ++tempPtr;
            }
            while (pos2 < m_size2)
            {
                *tempPtr = array2[pos2++];
                ++tempPtr;
            }

            //copy back to main array, this isn't the most efficient sort, just an example
            memcpy(m_array, m_tempArray, (m_size1 + m_size2) * sizeof(int));
        }
    private:
        int* m_array;
        int* m_tempArray;
        int m_size1;
        int m_size2;
    };

    class MergeSortTaskflowFork
    {
    public:
        AZ_CLASS_ALLOCATOR(MergeSortTaskflowFork, ThreadPoolAllocator, 0)

        MergeSortTaskflowFork(int* array, int* tempArray, int size)
            : m_array(array)
            , m_tempArray(tempArray)
            , m_size(size)
        {
        }
        void operator() (tf::Subflow& subflow)
        {
            unsigned int size1 = m_size / 2;
            unsigned int size2 = m_size - size1;
            int* array1 = &m_array[0];
            int* array2 = &m_array[size1];
            int* tempArray1 = &m_tempArray[0];
            int* tempArray2 = &m_tempArray[size1];
            tf::Task jobJoin = subflow.emplace(MergeSortTaskflowJoin(m_array, m_tempArray, size1, size2));
            if (size1 > 1)
            {
                tf::Task job = subflow.emplace(MergeSortTaskflowFork(array1, tempArray1, size1));
                job.precede(jobJoin);
            }
            if (size2 > 1)
            {
                tf::Task job = subflow.emplace(MergeSortTaskflowFork(array2, tempArray2, size2));
                job.precede(jobJoin);
            }            
        }
    private:
        int* m_array;
        int* m_tempArray;
        int m_size;
    };

    class TaskflowMergeSortTest
        : public DefaultTaskflowExecutorSetupFixture
    {
    public:
        void run()
        {
#ifdef _DEBUG
            const int arraySize = 2000;
#else
            const int arraySize = 100000;
#endif
            SimpleLcgRandom random;
            int* array = reinterpret_cast<int*>(azmalloc(sizeof(int) * arraySize, 4));
            int* tempArray = reinterpret_cast<int*>(azmalloc(sizeof(int) * arraySize, 4));
            for (int i = 0; i < arraySize; ++i)
            {
                array[i] = random.GetRandom();
            }

            AZStd::sys_time_t tStart = AZStd::GetTimeNowMicroSecond();

            tf::Taskflow taskflow;
            taskflow.emplace(MergeSortTaskflowFork(array, tempArray, arraySize));
            m_executor->run(taskflow).wait();

            s_totalTaskflowTime += AZStd::GetTimeNowMicroSecond() - tStart;

            for (int i = 0; i < arraySize - 1; ++i)
            {
                AZ_TEST_ASSERT(array[i] <= array[i + 1]);
            }

            azfree(array);
            azfree(tempArray);
        }
    };

    TEST_F(TaskflowMergeSortTest, Test)
    {
        run();
    }
    // MergeSortTaskflowExample-End

    // QuickSortTaskflowExample-Begin
    class QuickSortTaskflow
    {
    public:
        AZ_CLASS_ALLOCATOR(QuickSortTaskflow, ThreadPoolAllocator, 0)

        QuickSortTaskflow(int* array, int left, int right)
            : m_array(array)
            , m_left(left)
            , m_right(right)
        {
        }
        void operator() (tf::Subflow& subflow)
        {
            if (m_right <= m_left)
            {
                return;
            }

            //partition
            int i = m_left - 1;
            {
                int j = m_right;
                int v = m_array[m_right];
                for (;; )
                {
                    while (m_array[++i] < v)
                    {
                    }
                    while (v < m_array[--j])
                    {
                        if (j == m_left)
                        {
                            break;
                        }
                    }
                    if (i >= j)
                    {
                        break;
                    }
                    AZStd::swap(m_array[i], m_array[j]);
                }
                AZStd::swap(m_array[i], m_array[m_right]);
            }

            tf::Task job1 = subflow.emplace(QuickSortTaskflow(m_array, m_left, i - 1));
            tf::Task job2 = subflow.emplace(QuickSortTaskflow(m_array, i + 1, m_right));
        }
    private:
        int* m_array;
        int m_left;
        int m_right;
    };

    class TaskflowQuickSortTest
        : public DefaultTaskflowExecutorSetupFixture
    {
    public:
        void run()
        {
#ifdef _DEBUG
            const int arraySize = 2000;
#else
            const int arraySize = 100000;
#endif
            SimpleLcgRandom random;
            int* array = reinterpret_cast<int*>(azmalloc(sizeof(int) * arraySize, 4));
            for (int i = 0; i < arraySize; ++i)
            {
                array[i] = random.GetRandom();
            }

            AZStd::sys_time_t tStart = AZStd::GetTimeNowMicroSecond();

            tf::Taskflow taskflow;
            taskflow.emplace(QuickSortTaskflow(array, 0, arraySize - 1));
            m_executor->run(taskflow).wait();

            s_totalTaskflowTime += AZStd::GetTimeNowMicroSecond() - tStart;

            for (int i = 0; i < arraySize - 1; ++i)
            {
                AZ_TEST_ASSERT(array[i] <= array[i + 1]);
            }

            azfree(array);
        }
    };

    TEST_F(TaskflowQuickSortTest, Test)
    {
        run();
    }
    // QuickSortTaskflowExample-End


#if 0 // remove until can discuss need. Unclear how taskflow supports cancelling part of a taskgraph
    class JobCancellationTest
        : public DefaultTaskflowExecutorSetupFixture
    {
    public:
        void SetUp() override
        {
            DefaultTaskflowExecutorSetupFixture::SetUp();

            m_cancelGroup1 = aznew JobCancelGroup();
            m_cancelGroup2 = aznew JobCancelGroup(m_cancelGroup1);
            m_cancelGroup3 = aznew JobCancelGroup(m_cancelGroup2);
            m_context1 = aznew JobContext(m_jobContext->GetJobManager(), *m_cancelGroup1);
            m_context2 = aznew JobContext(m_jobContext->GetJobManager(), *m_cancelGroup2);
            m_context3 = aznew JobContext(m_jobContext->GetJobManager(), *m_cancelGroup3);
            m_value = 0;
        }

        void TearDown() override
        {
            delete m_context3;
            delete m_context2;
            delete m_context1;
            delete m_cancelGroup3;
            delete m_cancelGroup2;
            delete m_cancelGroup1;

            DefaultTaskflowExecutorSetupFixture::TearDown();
        }

        void run()
        {
            AZStd::sys_time_t tStart = AZStd::GetTimeNowMicroSecond();
            TaskflowCompletion completion(m_jobContext);
            {
                m_value = 0;
                completion.Reset(true);
                StartJobs(&completion);
                completion.StartAndWaitForCompletion();
                AZ_TEST_ASSERT(m_value == 111);
            }
            {
                m_value = 0;
                completion.Reset(true);
                m_cancelGroup3->Cancel(); //cancel before starting jobs, so test is deterministic
                StartJobs(&completion);
                completion.StartAndWaitForCompletion();
                m_cancelGroup3->Reset();
                AZ_TEST_ASSERT(m_value == 110);
            }
            {
                m_value = 0;
                completion.Reset(true);
                m_cancelGroup2->Cancel(); //cancel before starting jobs, so test is deterministic
                StartJobs(&completion);
                completion.StartAndWaitForCompletion();
                m_cancelGroup2->Reset();
                AZ_TEST_ASSERT(m_value == 100);
            }
            {
                m_value = 0;
                completion.Reset(true);
                m_cancelGroup1->Cancel(); //cancel before starting jobs, so test is deterministic
                StartJobs(&completion);
                completion.StartAndWaitForCompletion();
                m_cancelGroup1->Reset();
                AZ_TEST_ASSERT(m_value == 0);
            }

            s_totalTaskflowTime += AZStd::GetTimeNowMicroSecond() - tStart;
        }
        void StartJobs(Job* dependent)
        {
            {
                Job* job = CreateTaskflowFunction(AZStd::bind(&JobCancellationTest::Add, this, 100), true, m_context1);
                job->SetDependent(dependent);
                job->Start();
            }
            {
                Job* job = CreateTaskflowFunction(AZStd::bind(&JobCancellationTest::Add, this, 10), true, m_context2);
                job->SetDependent(dependent);
                job->Start();
            }
            {
                Job* job = CreateTaskflowFunction(AZStd::bind(&JobCancellationTest::Add, this, 1), true, m_context3);
                job->SetDependent(dependent);
                job->Start();
            }
        }

        void Add(int x)
        {
            m_value += x;
        }
    private:
        JobCancelGroup* m_cancelGroup1;
        JobCancelGroup* m_cancelGroup2;
        JobCancelGroup* m_cancelGroup3;
        JobContext* m_context1;
        JobContext* m_context2;
        JobContext* m_context3;

        AZStd::atomic<int> m_value;
    };

    TEST_F(JobCancellationTest, Test)
    {
        run();
    }
#endif

#if 0 // taskflow doesn't support current thread assisting in the workgraph
    class JobAssistTest
        : public DefaultTaskflowExecutorSetupFixture
    {
    public:
        void run()
        {
            AZStd::sys_time_t tStart = AZStd::GetTimeNowMicroSecond();

            int result = 0;
            int result = 0;
            tf::Taskflow taskflow;
            taskflow.emplace(FibonacciTaskflowFork(g_tfFibonacciSlow, &result));
            m_executor->run(taskflow).wait();

            Job* job = aznew FibonacciTaskflowFork(g_tfFibonacciSlow, &result, m_jobContext);
            job->StartAndAssistUntilComplete();
            AZ_TEST_ASSERT(result == g_tfFibonacciSlowResult);

            s_totalTaskflowTime += AZStd::GetTimeNowMicroSecond() - tStart;
        }
    };

    TEST_F(JobAssistTest, Test)
    {
        run();
    }
#endif

#if 0 // structured_task_group && Jobs/Algorithms.h not yet implemented on taskflow

    // TaskGroupJobExample-Begin
    class JobTaskGroupTest
        : public DefaultTaskflowExecutorSetupFixture
    {
    public:
        void run()
        {
            AZStd::sys_time_t tStart = AZStd::GetTimeNowMicroSecond();

            int result;
            CalcFibonacci(g_tfFibonacciFast, &result);
            AZ_TEST_ASSERT(result == g_tfFibonacciFastResult);

            structured_task_group group(m_jobContext);
            group.run(&JobTaskGroupTest::TestFunc);
            group.wait();

            s_totalTaskflowTime += AZStd::GetTimeNowMicroSecond() - tStart;
        }

        static void TestFunc()
        {
        }
        void CalcFibonacci(int n, int* result)
        {
            //this is a spectacularly inefficient way to compute a Fibonacci number, just an example to test the jobs
            if (n < 2)
            {
                *result = n;
            }
            else
            {
                int result1, result2;
                structured_task_group group(m_jobContext);
                group.run(AZStd::bind(&JobTaskGroupTest::CalcFibonacci, this, n - 1, &result1));
                group.run(AZStd::bind(&JobTaskGroupTest::CalcFibonacci, this, n - 2, &result2));
                group.wait();
                *result = result1 + result2;
            }
        }
    };

    TEST_F(JobTaskGroupTest, Test)
    {
        run();
    }
    // TaskGroupJobExample-End

    class JobGlobalContextTest
        : public DefaultTaskflowExecutorSetupFixture
    {
    public:
        void run()
        {
            AZStd::sys_time_t tStart = AZStd::GetTimeNowMicroSecond();

            int result;
            CalcFibonacci(g_tfFibonacciFast, &result);
            AZ_TEST_ASSERT(result == g_tfFibonacciFastResult);

            s_totalTaskflowTime += AZStd::GetTimeNowMicroSecond() - tStart;
        }

        void CalcFibonacci(int n, int* result)
        {
            //this is a spectacularly inefficient way to compute a Fibonacci number, just an example to test the jobs
            if (n < 2)
            {
                *result = n;
            }
            else
            {
                int result1, result2;
                structured_task_group group;
                group.run(AZStd::bind(&JobGlobalContextTest::CalcFibonacci, this, n - 1, &result1));
                group.run(AZStd::bind(&JobGlobalContextTest::CalcFibonacci, this, n - 2, &result2));
                group.wait();
                *result = result1 + result2;
            }
        }
    };

    TEST_F(JobGlobalContextTest, Test)
    {
        run();
    }


    class JobParallelInvokeTest
        : public DefaultTaskflowExecutorSetupFixture
    {
    public:
        void run()
        {
            AZStd::sys_time_t tStart = AZStd::GetTimeNowMicroSecond();

            int result1, result2, result3;
            parallel_invoke(
                AZStd::bind(&JobParallelInvokeTest::FibTask, this, g_tfFibonacciSlow, &result1),
                AZStd::bind(&JobParallelInvokeTest::FibTask, this, g_tfFibonacciFast, &result2), m_jobContext);
            AZ_TEST_ASSERT(result1 == g_tfFibonacciSlowResult);
            AZ_TEST_ASSERT(result2 == g_tfFibonacciFastResult);

            parallel_invoke(
                AZStd::bind(&JobParallelInvokeTest::FibTask, this, g_tfFibonacciFast, &result1),
                AZStd::bind(&JobParallelInvokeTest::FibTask, this, g_tfFibonacciSlow, &result2),
                AZStd::bind(&JobParallelInvokeTest::FibTask, this, g_tfFibonacciSlow, &result3), m_jobContext);
            AZ_TEST_ASSERT(result1 == g_tfFibonacciFastResult);
            AZ_TEST_ASSERT(result2 == g_tfFibonacciSlowResult);
            AZ_TEST_ASSERT(result3 == g_tfFibonacciSlowResult);

            s_totalTaskflowTime += AZStd::GetTimeNowMicroSecond() - tStart;
        }

        void FibTask(int n, int* result)
        {
            *result = CalcFibonacci(n);
        }

        int CalcFibonacci(int n)
        {
            if (n < 2)
            {
                return n;
            }
            return CalcFibonacci(n - 1) + CalcFibonacci(n - 2);
        }
    };

    TEST_F(JobParallelInvokeTest, Test)
    {
        run();
    }

    class JobParallelForTest
        : public DefaultTaskflowExecutorSetupFixture
    {
    public:
        void TearDown() override
        {
            m_results.set_capacity(0);
            DefaultTaskflowExecutorSetupFixture::TearDown();
        }

        void run()
        {
            const int maxFibonacci = 30;

            AZStd::sys_time_t tStart = AZStd::GetTimeNowMicroSecond();

            //just a few iterations
            {
                m_results.resize(maxFibonacci);

                parallel_for(0, maxFibonacci, AZStd::bind(&JobParallelForTest::FibTask, this, AZStd::placeholders::_1, maxFibonacci));

                AZ_TEST_ASSERT(m_results[0] == 0);
                AZ_TEST_ASSERT(m_results[1] == 1);
                for (int i = 2; i < maxFibonacci; ++i)
                {
                    AZ_TEST_ASSERT(m_results[i] == (m_results[i - 1] + m_results[i - 2]));
                }
            }

            //lots and lots of iterations
            {
                const int numSets = 500;
                const int numIterations = numSets * maxFibonacci;

                m_results.resize(numIterations);

                parallel_for(0, numIterations, AZStd::bind(&JobParallelForTest::FibTask, this, AZStd::placeholders::_1, maxFibonacci));

                for (int setIndex = 0; setIndex < numSets; ++setIndex)
                {
                    int offset = setIndex * maxFibonacci;
                    AZ_TEST_ASSERT(m_results[offset + 0] == 0);
                    AZ_TEST_ASSERT(m_results[offset + 1] == 1);
                    for (int i = 2; i < maxFibonacci; ++i)
                    {
                        AZ_TEST_ASSERT(m_results[offset + i] == (m_results[offset + i - 1] + m_results[offset + i - 2]));
                    }
                }
            }

            //step size
            {
                const int numIterations = 100;
                const int step = 3;
                m_results.resize(numIterations * step);

                parallel_for(0, numIterations * step, step, AZStd::bind(&JobParallelForTest::Double, this, AZStd::placeholders::_1));

                for (int i = 0; i < numIterations * step; i += step)
                {
                    AZ_TEST_ASSERT(m_results[i] == i * 2);
                }
            }

            //start only
            {
                const int numIterations = 100;
                m_results.resize(numIterations);

                TaskflowCompletion doneJob;
                parallel_for_start(0, numIterations, AZStd::bind(&JobParallelForTest::Double, this, AZStd::placeholders::_1), &doneJob);
                doneJob.StartAndWaitForCompletion();

                for (int i = 0; i < numIterations; ++i)
                {
                    AZ_TEST_ASSERT(m_results[i] == i * 2);
                }
            }

            s_totalTaskflowTime += AZStd::GetTimeNowMicroSecond() - tStart;
        }

        void FibTask(int i, int maxFibonacci)
        {
            m_results[i] = CalcFibonacci(i % maxFibonacci);
        }

        int CalcFibonacci(int n)
        {
            if (n < 2)
            {
                return n;
            }
            return CalcFibonacci(n - 1) + CalcFibonacci(n - 2);
        }

        void Double(int i)
        {
            m_results[i] = i * 2;
        }

    private:
        AZStd::vector<int> m_results;
    };

    TEST_F(JobParallelForTest, Test)
    {
        run();
    }

    class JobParallelForEachTest
        : public DefaultTaskflowExecutorSetupFixture
    {
    public:
        void run()
        {
            AZStd::sys_time_t tStart = AZStd::GetTimeNowMicroSecond();

            //random access iterator
            {
                const int numValues = 1000;
                AZStd::fixed_vector<int, 1000> values;
                for (int i = 0; i < numValues; ++i)
                {
                    values.push_back(i);
                }

                parallel_for_each(values.begin(), values.end(), AZStd::bind(&Double, AZStd::placeholders::_1));

                for (int i = 0; i < numValues; ++i)
                {
                    AZ_TEST_ASSERT(values[i] == 2 * i);
                }
            }

            //forward iterator
            {
                const int numValues = 1000;
                AZStd::fixed_list<int, numValues> values;
                for (int i = 0; i < numValues; ++i)
                {
                    values.push_back(i);
                }

                parallel_for_each(values.begin(), values.end(), AZStd::bind(&Double, AZStd::placeholders::_1));

                int i = 0;
                for (AZStd::fixed_list<int, numValues>::const_iterator iter = values.begin(); iter != values.end(); ++iter)
                {
                    AZ_TEST_ASSERT(*iter == 2 * i);
                    ++i;
                }
            }

            //start only
            {
                const int numValues = 1000;
                AZStd::fixed_vector<int, numValues> values;
                for (int i = 0; i < numValues; ++i)
                {
                    values.push_back(i);
                }

                TaskflowCompletion doneJob;
                parallel_for_each_start(values.begin(), values.end(), &JobParallelForEachTest::Double, &doneJob);
                doneJob.StartAndWaitForCompletion();

                for (int i = 0; i < numValues; ++i)
                {
                    AZ_TEST_ASSERT(values[i] == 2 * i);
                }
            }

            s_totalTaskflowTime += AZStd::GetTimeNowMicroSecond() - tStart;
        }

        static void Double(int& i)
        {
            i *= 2;
        }

    private:
        AZStd::vector<int> m_results;
    };

    TEST_F(JobParallelForEachTest, Test)
    {
        run();
    }

    class PERF_JobParallelForOverheadTest
        : public DefaultTaskflowExecutorSetupFixture
    {
    public:
        static const size_t numElementsScale = 1;

#ifdef _DEBUG
        static const size_t m_numElements = 10000 / numElementsScale;
#else
        static const size_t m_numElements = 100000 / numElementsScale;
#endif

        PERF_JobParallelForOverheadTest()
            : DefaultTaskflowExecutorSetupFixture()
        {}

        void TearDown() override
        {
            m_vectors.set_capacity(0);
            m_vectors1.set_capacity(0);
            m_results.set_capacity(0);
            m_transforms.set_capacity(0);
            m_callOrder.clear();

            DefaultTaskflowExecutorSetupFixture::TearDown();
        }

        void run()
        {
            m_vectors.reserve(m_numElements);
            m_vectors1.reserve(m_numElements);
            //m_callOrder.resize(m_numElements);
            m_results.reserve(m_numElements);
            m_transforms.reserve(m_numElements);
            for (size_t i = 0; i < m_numElements; ++i)
            {
                m_vectors.push_back(Vector3::CreateOne());
                m_vectors1.push_back(Vector3::CreateOne());
                m_results.push_back(Vector3::CreateZero());
                m_transforms.push_back(Transform::CreateRotationX(static_cast<float>(i) / 3.0f));
            }

            AZStd::sys_time_t tStart = 0;
            AZStd::sys_time_t nonParallelMS = 0, parallelForMS = 0, parallelForPPLMS = 0;
            AZStd::sys_time_t nonParallelProcessMS = 0, parallelForProcessMS = 0, parallelForProcessPPLMS = 0;

            static const int numOfArrayIterations = /*5*/ 1;

            // non parallel test
            m_processElementsTime = 0;
            tStart = AZStd::GetTimeNowMicroSecond();
            for (int i = 0; i < numOfArrayIterations; ++i)
            {
                for (size_t j = 0; j < m_numElements; ++j)
                {
                    ProcessElement(j);
                }
            }
            nonParallelMS = AZStd::GetTimeNowMicroSecond() - tStart;
            nonParallelProcessMS = m_processElementsTime;


            // parallel_for test
            {
#ifdef AZ_JOBS_PRINT_CALL_ORDER
                m_callOrder.clear();
#endif //#ifdef AZ_JOBS_PRINT_CALL_ORDER
                m_processElementsTime = 0;
                tStart = AZStd::GetTimeNowMicroSecond();
                for (int i = 0; i < numOfArrayIterations; ++i)
                {
                    parallel_for(static_cast<size_t>(0), m_numElements, AZStd::bind(&PERF_JobParallelForOverheadTest::ProcessElement, this, AZStd::placeholders::_1) /*,static_partitioner()*/);
                }
                parallelForMS = AZStd::GetTimeNowMicroSecond() - tStart;
                parallelForProcessMS = m_processElementsTime;
            }


#if defined(AZ_COMPARE_TO_PPL)
            // compare to MS Concurrency::parallel_for
            {
#   ifdef AZ_JOBS_PRINT_CALL_ORDER
                m_callOrder.clear();
#   endif // #ifdef AZ_JOBS_PRINT_CALL_ORDER
                m_processElementsTime = 0;
                tStart = AZStd::GetTimeNowMicroSecond();
                //Concurrency::auto_partitioner part;
                for (int i = 0; i < numOfArrayIterations; ++i)
                {
                    Concurrency::parallel_for(static_cast<size_t>(0), m_numElements, AZStd::bind(&PERF_JobParallelForOverheadTest::ProcessElement, this, AZStd::placeholders::_1) /*,part*/);
                }
                parallelForPPLMS = AZStd::GetTimeNowMicroSecond() - tStart;
                parallelForProcessPPLMS = m_processElementsTime;
            }
#endif // AZ_COMPARE_TO_PPL

            AZ_Printf("UnitTest", "\n\nJob overhead test. Serial %lld (%lld) Parallel %lld (%lld) PPL %lld (%lld) Total: %lld\n\n", nonParallelMS, nonParallelProcessMS, parallelForMS, parallelForProcessMS, parallelForPPLMS, parallelForProcessPPLMS, s_totalTaskflowTime);

#ifdef AZ_JOBS_PRINT_CALL_ORDER
            // Find all unique threads
            typedef AZStd::unordered_set<AZStd::native_thread_id_type> ThreadSetType;
            ThreadSetType threads;
            for (unsigned int i = 0; i < m_callOrder.size(); ++i)
            {
                threads.insert(m_callOrder[i].second);
            }
            // print order by thread
            unsigned int totalProcessedElements = 0;
            printf("Elements processed by %d threads:\n", threads.size());
            for (ThreadSetType::iterator it = threads.begin(); it != threads.end(); ++it)
            {
                unsigned int elementsProcessed = 0;
                AZStd::native_thread_id_type threadId = *it;
                printf("Thread %d!\n", threadId);
                for (unsigned int i = 0; i < m_callOrder.size(); ++i)
                {
                    if (m_callOrder[i].second == threadId)
                    {
                        if (elementsProcessed % 10 == 0)
                        {
                            printf("%d\n", m_callOrder[i].first);
                        }
                        else
                        {
                            printf("%d,", m_callOrder[i].first);
                        }
                        elementsProcessed++;
                    }
                }
                totalProcessedElements += elementsProcessed;
                printf("\nTotal Elements for thread %d are %d\n\n\n\n\n", threadId, elementsProcessed);
            }

            printf("\nTotal Elements %d\n", totalProcessedElements);

            m_jobManager->PrintStats();
#endif
        }

        void ProcessElement(size_t index)
        {
            int numIterations = 50;
            if (index > m_numElements * 7 / 8 || index < m_numElements / 8) // simulate asymmetrical load
            {
                numIterations = 100;
            }
            //int numIterations = m_random.GetRandom() % 100;

#ifdef AZ_JOBS_PRINT_CALL_ORDER
            m_callOrder.push_back(AZStd::make_pair(index, AZStd::this_thread::get_id().m_id));
#endif
            for (int i = 0; i < numIterations; ++i)
            {
                Transform tm = m_transforms[index].GetInverse();
                Transform tm1 = tm.GetInverse();
                tm = tm1 * tm;
                Vector3 v = m_vectors[index] * m_vectors1[index].GetLength();
                m_results[index] = tm.TransformVector(v);
            }
        }

    private:
        AZStd::vector<Vector3> m_vectors;
        AZStd::vector<Vector3> m_vectors1;
        AZStd::concurrent_vector<AZStd::pair<size_t, AZStd::native_thread_id_type> >  m_callOrder;
        AZStd::vector<Transform> m_transforms;
        AZStd::vector<Vector3> m_results;
        AZStd::sys_time_t m_processElementsTime;
        SimpleLcgRandom m_random;
    };

#ifdef ENABLE_PERFORMANCE_TEST
    TEST_F(PERF_JobParallelForOverheadTest, Test)
    {
        run();
    }
#endif
#endif // Jobs/Algorithms.h not yet implemented on taskflow

    class TaskflowFunctionTestWithoutCurrentJobArg
        : public DefaultTaskflowExecutorSetupFixture
    {
    public:
        void run()
        {
            constexpr size_t JobCount = 32;
            size_t jobData[JobCount] = { 0 };

            tf::Taskflow taskflow;
            for (size_t i = 0; i < JobCount; ++i)
            {
                taskflow.emplace([i, &jobData](){jobData[i] = i + 1;});
            }
            m_executor->run(taskflow).wait();

            for (size_t i = 0; i < JobCount; ++i)
            {
                EXPECT_EQ(jobData[i], i + 1);
            }
        }
    };

    TEST_F(TaskflowFunctionTestWithoutCurrentJobArg, Test)
    {
        run();
    }

    class TaskflowFunctionTestWithCurrentJobArg
        : public DefaultTaskflowExecutorSetupFixture
    {
    public:
        void run()
        {
            constexpr size_t JobCount = 32;
            size_t jobData[JobCount] = { 0 };

            tf::Taskflow taskflow;
            // Push a parent job that pushes the work as child jobs (requires the current job, so this is a real world test of "functor with current job as param")
            taskflow.emplace([this, &jobData, JobCount](tf::Subflow& subflow)
                {
                    // EXPECT_EQ(m_jobManager->GetCurrentJob(), &thisJob); // not sure there is an equivalent to this in taskflow

                    for (size_t i = 0; i < JobCount; ++i)
                    {
                        subflow.emplace([this, i, &jobData]()
                            {
                                // EXPECT_EQ(m_jobManager->GetCurrentJob(), &thisJob); // not sure there is an equivalent to this in taskflow
                                jobData[i] = i + 1;
                            }
                        );
                    }

                    subflow.join(); // Note: this is required before the parent job returns
                }
            );
            m_executor->run(taskflow).wait();

            for (size_t i = 0; i < JobCount; ++i)
            {
                EXPECT_EQ(jobData[i], i + 1);
            }
        }
    };

    TEST_F(TaskflowFunctionTestWithCurrentJobArg, Test)
    {
        run();
    }

#if 0
    using JobLegacyJobExecutorIsRunning = DefaultTaskflowExecutorSetupFixture;
    TEST_F(JobLegacyJobExecutorIsRunning, Test)
    {
        // Note: Legacy JobExecutor exists as an adapter to Legacy CryEngine jobs.
        // When writing new jobs instead favor direct use of the AZ::Job type family
        AZ::LegacyJobExecutor jobExecutor;
        EXPECT_FALSE(jobExecutor.IsRunning());

        // Completion fences and IsRunning()
        {
            jobExecutor.PushCompletionFence();
            EXPECT_TRUE(jobExecutor.IsRunning());
            jobExecutor.PopCompletionFence();
            EXPECT_FALSE(jobExecutor.IsRunning());
        }

        AZStd::atomic_bool jobExecuted{ false };
        AZStd::binary_semaphore jobSemaphore;

        jobExecutor.StartJob([&jobSemaphore, &jobExecuted]
            {
                // Wait until the test thread releases
                jobExecuted = true;
                jobSemaphore.acquire();
            }
        );
        EXPECT_TRUE(jobExecutor.IsRunning());

        // Allow the job to complete
        jobSemaphore.release();

        // Wait for completion
        jobExecutor.WaitForCompletion();
        EXPECT_FALSE(jobExecutor.IsRunning());
        EXPECT_TRUE(jobExecuted);
    }

    using JobLegacyJobExecutorWaitForCompletion = DefaultTaskflowExecutorSetupFixture;
    TEST_F(JobLegacyJobExecutorWaitForCompletion, Test)
    {
        // Note: Legacy JobExecutor exists as an adapter to Legacy CryEngine jobs.
        // When writing new jobs instead favor direct use of the AZ::Job type family
        AZ::LegacyJobExecutor jobExecutor;

        // Semaphores used to park job threads until released
        const AZ::u32 numParkJobs = AZ::JobContext::GetGlobalContext()->GetJobManager().GetNumWorkerThreads();
        AZStd::vector<AZStd::binary_semaphore> jobSemaphores(numParkJobs);

        // Data destination for workers
        const AZ::u32 workJobCount = numParkJobs * 2;
        AZStd::vector<AZ::u32> jobData(workJobCount, 0);

        // Touch completion multiple times as a test of correctly transitioning in and out of the all jobs completed state
        AZ::u32 NumCompletionCycles = 5;
        for (AZ::u32 completionItrIdx = 0; completionItrIdx < NumCompletionCycles; ++completionItrIdx)
        {
            // Intentionally park every job thread
            for (auto& jobSemaphore : jobSemaphores)
            {
                jobExecutor.StartJob([&jobSemaphore]
                {
                    jobSemaphore.acquire();
                }
                );
            }
            EXPECT_TRUE(jobExecutor.IsRunning());

            // Kick off verifiable "work" jobs
            for (AZ::u32 i = 0; i < workJobCount; ++i)
            {
                jobExecutor.StartJob([i, &jobData]
                {
                    jobData[i] = i + 1;
                }
                );
            }
            EXPECT_TRUE(jobExecutor.IsRunning());

            // Now released our parked job threads
            for (auto& jobSemaphore : jobSemaphores)
            {
                jobSemaphore.release();
            }

            // And wait for all jobs to finish
            jobExecutor.WaitForCompletion();
            EXPECT_FALSE(jobExecutor.IsRunning());

            // Verify our workers ran and clear data
            for (size_t i = 0; i < workJobCount; ++i)
            {
                EXPECT_EQ(jobData[i], i + 1);
                jobData[i] = 0;
            }
        }
    }
#endif

    class TaskflowCompletionCompleteNotScheduled
        : public DefaultTaskflowExecutorSetupFixture
    {
    public:
        TaskflowCompletionCompleteNotScheduled()
            : DefaultTaskflowExecutorSetupFixture(1) // Only 1 worker to serialize job execution
        {
        }

        void run()
        {
            AZStd::atomic_int32_t testSequence{0};

            tf::Taskflow taskflow1;

            taskflow1.emplace([&testSequence]
                {
                    ++testSequence; // 0 => 1
                }
            );
            tf::Future<void> firstJobFuture = m_executor->run(taskflow1);

            AZStd::binary_semaphore testThreadSemaphore;
            AZStd::binary_semaphore jobSemaphore;

            tf::Taskflow taskflow2;
            taskflow2.emplace([&testSequence, &testThreadSemaphore, &jobSemaphore]
                {
                    testThreadSemaphore.release();
                    jobSemaphore.acquire();
                    ++testSequence; // 1 => 2
                }
            );
            tf::Future<void> secondJobFuture = m_executor->run(taskflow2);

            // guarantee the parkedJob has started, with only 1 job thread this means firstJob must be complete
            testThreadSemaphore.acquire();

            // If waiting on completion is unscheduled and executes immediately, then the value of sequence will be 1
            firstJobFuture.wait();
            EXPECT_EQ(testSequence, 1); 

            // Allow the second job to finish
            jobSemaphore.release();

            // Safety sync before exiting this scope
            secondJobFuture.wait();
            EXPECT_EQ(testSequence, 2);
        }
    };

    TEST_F(TaskflowCompletionCompleteNotScheduled, Test)
    {
        run();
    }
#if 0 // job priority not supported in taskflow
    class TestJobWithPriority : public Job
    {
    public:
        static AZStd::atomic<AZ::s32> s_numIncompleteJobs;

        AZ_CLASS_ALLOCATOR(TestJobWithPriority, ThreadPoolAllocator, 0)

        TestJobWithPriority(AZ::s8 priority, const char* name, JobContext* context, AZStd::binary_semaphore& binarySemaphore, AZStd::vector<AZStd::string>& namesOfProcessedJobs)
            : Job(true, context, false, priority)
            , m_name(name)
            , m_binarySemaphore(binarySemaphore)
            , m_namesOfProcessedJobs(namesOfProcessedJobs)
        {
            ++s_numIncompleteJobs;
        }

        void Process() override
        {
            // Ensure the job does not complete until it is able to acquire the semaphore,
            // then add its name to the vector of processed jobs.
            m_binarySemaphore.acquire();
            m_namesOfProcessedJobs.push_back(m_name);
            m_binarySemaphore.release();
            --s_numIncompleteJobs;
        }

    private:
        const AZStd::string m_name;
        AZStd::binary_semaphore& m_binarySemaphore;
        AZStd::vector<AZStd::string>& m_namesOfProcessedJobs;
    };
    AZStd::atomic<AZ::s32> TestJobWithPriority::s_numIncompleteJobs = 0;

    class JobPriorityTestFixture : public DefaultTaskflowExecutorSetupFixture
    {
    public:
        JobPriorityTestFixture() : DefaultTaskflowExecutorSetupFixture(1) // Only 1 worker to serialize job execution
        {
        }

        void RunTest()
        {
            AZStd::vector<AZStd::string> namesOfProcessedJobs;

            // The queue is empty, and calling 'Start' on a job inserts it into the job queue, but it won't necesarily begin processing immediately.
            // So we need to set the priority of the first job queued to the highest available, ensuring it will get processed first even if the lone
            // worker thread active in this test does not inspect the queue until after all the jobs below have been 'started' (this is guaranteed by
            // the fact that jobs with equal priority values are processed in FIFO order).
            //
            // The binary semaphore ensures that this first job does not complete until we've finished queuing all the other jobs.
            //
            // All jobs that we start in this test have the 'isAutoDelete' param set to true, so we don't have to store them or clean up.
            AZStd::binary_semaphore binarySemaphore;
            (aznew TestJobWithPriority(127, "FirstJobQueued", m_jobContext, binarySemaphore, namesOfProcessedJobs))->Start();

            // Queue a number of other jobs before releasing the semaphore that will allow "FirstJobQueued" to complete.
            // These additional jobs should be processed in order of their priority, or where the priority is equal in the order they were queued.
            (aznew TestJobWithPriority(-128, "LowestPriority", m_jobContext, binarySemaphore, namesOfProcessedJobs))->Start();
            (aznew TestJobWithPriority(-1, "LowPriority1", m_jobContext, binarySemaphore, namesOfProcessedJobs))->Start();
            (aznew TestJobWithPriority(-1, "LowPriority2", m_jobContext, binarySemaphore, namesOfProcessedJobs))->Start();
            (aznew TestJobWithPriority(-1, "LowPriority3", m_jobContext, binarySemaphore, namesOfProcessedJobs))->Start();
            (aznew TestJobWithPriority(0, "DefaultPriority1", m_jobContext, binarySemaphore, namesOfProcessedJobs))->Start();
            (aznew TestJobWithPriority(0, "DefaultPriority2", m_jobContext, binarySemaphore, namesOfProcessedJobs))->Start();
            (aznew TestJobWithPriority(0, "DefaultPriority3", m_jobContext, binarySemaphore, namesOfProcessedJobs))->Start();
            (aznew TestJobWithPriority(1, "HighPriority1", m_jobContext, binarySemaphore, namesOfProcessedJobs))->Start();
            (aznew TestJobWithPriority(1, "HighPriority2", m_jobContext, binarySemaphore, namesOfProcessedJobs))->Start();
            (aznew TestJobWithPriority(1, "HighPriority3", m_jobContext, binarySemaphore, namesOfProcessedJobs))->Start();
            (aznew TestJobWithPriority(127, "HighestPriority", m_jobContext, binarySemaphore, namesOfProcessedJobs))->Start();

            // Release the binary semaphore so the first queued job will complete. The rest of the queued jobs should now complete in order of their priority.
            binarySemaphore.release();

            // Wait until all the jobs have completed. Ideally we would start one last job (with the lowest priority so it is guaranteed to be processed last
            // even if the jobs started above have yet to complete) and wait until it has completed, but this results in this main thread picking up jobs and
            // thorwing all the careful scheduling on the one worker thread active in this test out the window (please see Job::StartAndAssistUntilComplete).
            while (TestJobWithPriority::s_numIncompleteJobs > 0) {}

            // Verify that the jobs were completed in the expected order.
            EXPECT_EQ(namesOfProcessedJobs[0], "FirstJobQueued");
            EXPECT_EQ(namesOfProcessedJobs[1], "HighestPriority");
            EXPECT_EQ(namesOfProcessedJobs[2], "HighPriority1");
            EXPECT_EQ(namesOfProcessedJobs[3], "HighPriority2");
            EXPECT_EQ(namesOfProcessedJobs[4], "HighPriority3");
            EXPECT_EQ(namesOfProcessedJobs[5], "DefaultPriority1");
            EXPECT_EQ(namesOfProcessedJobs[6], "DefaultPriority2");
            EXPECT_EQ(namesOfProcessedJobs[7], "DefaultPriority3");
            EXPECT_EQ(namesOfProcessedJobs[8], "LowPriority1");
            EXPECT_EQ(namesOfProcessedJobs[9], "LowPriority2");
            EXPECT_EQ(namesOfProcessedJobs[10], "LowPriority3");
            EXPECT_EQ(namesOfProcessedJobs[11], "LowestPriority");
        }
    };

    TEST_F(JobPriorityTestFixture, Test)
    {
        RunTest();
    }
#endif // job priority unsupported in taskflow
} // UnitTest

#if defined(HAVE_BENCHMARK)
namespace Benchmark
{
    double TaskflowCalculatePi(AZ::u32 depth)
    {
        double pi = 0.0;
        for (AZ::u32 i = 0; i < depth; ++i)
        {
            const double numerator = static_cast<double>(((i % 2) * 2) - 1);
            const double denominator = static_cast<double>((2 * i) - 1);
            pi += numerator / denominator;
        }
        return (pi - 1.0) * 4;
    }

    class TestTaskflowCalculatePi
    {
    public:
        static AZStd::atomic<AZ::s32> s_numIncompleteJobs;

        AZ_CLASS_ALLOCATOR(TestTaskflowCalculatePi, ThreadPoolAllocator, 0)

        TestTaskflowCalculatePi(AZ::u32 depth)
            : m_depth(depth)
        {
            ++s_numIncompleteJobs;
        }

        void operator() ()
        {
            benchmark::DoNotOptimize(TaskflowCalculatePi(m_depth));
            --s_numIncompleteJobs;
        }
    private:
        const AZ::u32 m_depth;
    };
    AZStd::atomic<AZ::s32> TestTaskflowCalculatePi::s_numIncompleteJobs = 0;

    class TaskflowBenchmarkFixture : public ::benchmark::Fixture
    {
    public:
        static const AZ::s32 LIGHT_WEIGHT_JOB_CALCULATE_PI_DEPTH = 1;
        static const AZ::s32 MEDIUM_WEIGHT_JOB_CALCULATE_PI_DEPTH = 1024;
        static const AZ::s32 HEAVY_WEIGHT_JOB_CALCULATE_PI_DEPTH = 1048576;

        static const AZ::u32 SMALL_NUMBER_OF_JOBS = 10;
        static const AZ::u32 MEDIUM_NUMBER_OF_JOBS = 1024;
        static const AZ::u32 LARGE_NUMBER_OF_JOBS = 16384;

        void SetUp([[maybe_unused]] ::benchmark::State& state) override
        {
            AllocatorInstance<PoolAllocator>::Create();
            AllocatorInstance<ThreadPoolAllocator>::Create();

            // Generate some random depths
            m_randomDepths.resize(LARGE_NUMBER_OF_JOBS);
            std::mt19937_64 randomDepthGenerator(1); // Always use the same seed
            std::uniform_int_distribution<> randomDepthDistribution(LIGHT_WEIGHT_JOB_CALCULATE_PI_DEPTH,
                                                                    HEAVY_WEIGHT_JOB_CALCULATE_PI_DEPTH);
            std::generate(m_randomDepths.begin(), m_randomDepths.end(), [&randomDepthDistribution, &randomDepthGenerator]()
            {
                return randomDepthDistribution(randomDepthGenerator);
            });
        }

        void TearDown([[maybe_unused]] ::benchmark::State& state) override
        {
            // We must clear these vectors before destroying the allocators. 
            m_randomPriorities = {};
            m_randomDepths = {};

            AllocatorInstance<ThreadPoolAllocator>::Destroy();
            AllocatorInstance<PoolAllocator>::Destroy();
        }

    protected:
        inline void RunCalculatePiJob(AZ::s32 depth)
        {
            tf::Taskflow taskflow;
            taskflow.emplace(TestTaskflowCalculatePi(depth));
            m_executor.run(AZStd::move(taskflow)); // use the move version so executor manages taskflow lifetime
        }

        inline void RunMultipleCalculatePiJobsWithDefaultPriority(AZ::u32 numberOfJobs, AZ::s32 depth)
        {
            for (AZ::u32 i = 0; i < numberOfJobs; ++i)
            {
                RunCalculatePiJob(depth);
            }

            // Wait until all the jobs have completed.
            while (TestTaskflowCalculatePi::s_numIncompleteJobs > 0) {}
        }

        // inline void RunMultipleCalculatePiJobsWithRandomPriority(AZ::u32 numberOfJobs, AZ::s32 depth)
        // {
        //     for (AZ::u32 i = 0; i < numberOfJobs; ++i)
        //     {
        //         RunCalculatePiJob(depth, m_randomPriorities[i]);
        //     }

        //     // Wait until all the jobs have completed.
        //     while (TestTaskflowCalculatePi::s_numIncompleteJobs > 0) {}
        // }

        inline void RunMultipleCalculatePiJobsWithRandomDepthAndDefaultPriority(AZ::u32 numberOfJobs)
        {
            for (AZ::u32 i = 0; i < numberOfJobs; ++i)
            {
                RunCalculatePiJob(m_randomDepths[i]);
            }

            // Wait until all the jobs have completed.
            while (TestTaskflowCalculatePi::s_numIncompleteJobs > 0) {}
        }

        // inline void RunMultipleCalculatePiJobsWithRandomDepthAndRandomPriority(AZ::u32 numberOfJobs)
        // {
        //     for (AZ::u32 i = 0; i < numberOfJobs; ++i)
        //     {
        //         RunCalculatePiJob(m_randomDepths[i],m_randomPriorities[i]);
        //     }

        //     // Wait until all the jobs have completed.
        //     while (TestTaskflowCalculatePi::s_numIncompleteJobs > 0) {}
        // }

    protected:

        tf::Executor m_executor;
        AZStd::vector<AZ::u32> m_randomDepths;
        AZStd::vector<AZ::s8> m_randomPriorities;
    };

    BENCHMARK_F(TaskflowBenchmarkFixture, RunSmallNumberOfLightWeightTaskflowWithDefaultPriority)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithDefaultPriority(SMALL_NUMBER_OF_JOBS, LIGHT_WEIGHT_JOB_CALCULATE_PI_DEPTH);
        }
    }

    BENCHMARK_F(TaskflowBenchmarkFixture, RunMediumNumberOfLightWeightTaskflowsWithDefaultPriority)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithDefaultPriority(MEDIUM_NUMBER_OF_JOBS, LIGHT_WEIGHT_JOB_CALCULATE_PI_DEPTH);
        }
    }

    BENCHMARK_F(TaskflowBenchmarkFixture, RunLargeNumberOfLightWeightTaskflowWithDefaultPriority)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithDefaultPriority(LARGE_NUMBER_OF_JOBS, LIGHT_WEIGHT_JOB_CALCULATE_PI_DEPTH);
        }
    }

    BENCHMARK_F(TaskflowBenchmarkFixture, RunSmallNumberOfMediumWeightTaskflowWithDefaultPriority)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithDefaultPriority(SMALL_NUMBER_OF_JOBS, MEDIUM_WEIGHT_JOB_CALCULATE_PI_DEPTH);
        }
    }

    BENCHMARK_F(TaskflowBenchmarkFixture, RunMediumNumberOfMediumWeightTaskflowWithDefaultPriority)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithDefaultPriority(MEDIUM_NUMBER_OF_JOBS, MEDIUM_WEIGHT_JOB_CALCULATE_PI_DEPTH);
        }
    }

    BENCHMARK_F(TaskflowBenchmarkFixture, RunLargeNumberOfMediumWeightTaskflowWithDefaultPriority)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithDefaultPriority(LARGE_NUMBER_OF_JOBS, MEDIUM_WEIGHT_JOB_CALCULATE_PI_DEPTH);
        }
    }

    BENCHMARK_F(TaskflowBenchmarkFixture, RunSmallNumberOfHeavyWeightTaskflowWithDefaultPriority)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithDefaultPriority(SMALL_NUMBER_OF_JOBS, HEAVY_WEIGHT_JOB_CALCULATE_PI_DEPTH);
        }
    }

    BENCHMARK_F(TaskflowBenchmarkFixture, RunMediumNumberOfHeavyWeightTaskflowWithDefaultPriority)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithDefaultPriority(MEDIUM_NUMBER_OF_JOBS, HEAVY_WEIGHT_JOB_CALCULATE_PI_DEPTH);
        }
    }

    BENCHMARK_F(TaskflowBenchmarkFixture, RunLargeNumberOfHeavyWeightTaskflowWithDefaultPriority)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithDefaultPriority(LARGE_NUMBER_OF_JOBS, HEAVY_WEIGHT_JOB_CALCULATE_PI_DEPTH);
        }
    }

    BENCHMARK_F(TaskflowBenchmarkFixture, RunSmallNumberOfRandomWeightTaskflowWithDefaultPriority)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithRandomDepthAndDefaultPriority(SMALL_NUMBER_OF_JOBS);
        }
    }

    BENCHMARK_F(TaskflowBenchmarkFixture, RunMediumNumberOfRandomWeightTaskflowWithDefaultPriority)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithRandomDepthAndDefaultPriority(MEDIUM_NUMBER_OF_JOBS);
        }
    }

    BENCHMARK_F(TaskflowBenchmarkFixture, RunLargeNumberOfRandomWeightTaskflowWithDefaultPriority)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithRandomDepthAndDefaultPriority(LARGE_NUMBER_OF_JOBS);
        }
    }

    // BENCHMARK_F(TaskflowBenchmarkFixture, RunSmallNumberOfLightWeightJobsWithRandomPriorities)(benchmark::State& state)
    // {
    //     for (auto _ : state)
    //     {
    //         RunMultipleCalculatePiJobsWithRandomPriority(SMALL_NUMBER_OF_JOBS, LIGHT_WEIGHT_JOB_CALCULATE_PI_DEPTH);
    //     }
    // }

    // BENCHMARK_F(TaskflowBenchmarkFixture, RunMediumNumberOfLightWeightJobsWithRandomPriorities)(benchmark::State& state)
    // {
    //     for (auto _ : state)
    //     {
    //         RunMultipleCalculatePiJobsWithRandomPriority(MEDIUM_NUMBER_OF_JOBS, LIGHT_WEIGHT_JOB_CALCULATE_PI_DEPTH);
    //     }
    // }

    // BENCHMARK_F(TaskflowBenchmarkFixture, RunLargeNumberOfLightWeightJobsWithRandomPriorities)(benchmark::State& state)
    // {
    //     for (auto _ : state)
    //     {
    //         RunMultipleCalculatePiJobsWithRandomPriority(LARGE_NUMBER_OF_JOBS, LIGHT_WEIGHT_JOB_CALCULATE_PI_DEPTH);
    //     }
    // }

    // BENCHMARK_F(TaskflowBenchmarkFixture, RunSmallNumberOfMediumWeightJobsWithRandomPriorities)(benchmark::State& state)
    // {
    //     for (auto _ : state)
    //     {
    //         RunMultipleCalculatePiJobsWithRandomPriority(SMALL_NUMBER_OF_JOBS, MEDIUM_WEIGHT_JOB_CALCULATE_PI_DEPTH);
    //     }
    // }

    // BENCHMARK_F(TaskflowBenchmarkFixture, RunMediumNumberOfMediumWeightJobsWithRandomPriorities)(benchmark::State& state)
    // {
    //     for (auto _ : state)
    //     {
    //         RunMultipleCalculatePiJobsWithRandomPriority(MEDIUM_NUMBER_OF_JOBS, MEDIUM_WEIGHT_JOB_CALCULATE_PI_DEPTH);
    //     }
    // }

    // BENCHMARK_F(TaskflowBenchmarkFixture, RunLargeNumberOfMediumWeightJobsWithRandomPriorities)(benchmark::State& state)
    // {
    //     for (auto _ : state)
    //     {
    //         RunMultipleCalculatePiJobsWithRandomPriority(LARGE_NUMBER_OF_JOBS, MEDIUM_WEIGHT_JOB_CALCULATE_PI_DEPTH);
    //     }
    // }

    // BENCHMARK_F(TaskflowBenchmarkFixture, RunSmallNumberOfHeavyWeightJobsWithRandomPriorities)(benchmark::State& state)
    // {
    //     for (auto _ : state)
    //     {
    //         RunMultipleCalculatePiJobsWithRandomPriority(SMALL_NUMBER_OF_JOBS, HEAVY_WEIGHT_JOB_CALCULATE_PI_DEPTH);
    //     }
    // }

    // BENCHMARK_F(TaskflowBenchmarkFixture, RunMediumNumberOfHeavyWeightJobsWithRandomPriorities)(benchmark::State& state)
    // {
    //     for (auto _ : state)
    //     {
    //         RunMultipleCalculatePiJobsWithRandomPriority(MEDIUM_NUMBER_OF_JOBS, HEAVY_WEIGHT_JOB_CALCULATE_PI_DEPTH);
    //     }
    // }

    // BENCHMARK_F(TaskflowBenchmarkFixture, RunLargeNumberOfHeavyWeightJobsWithRandomPriorities)(benchmark::State& state)
    // {
    //     for (auto _ : state)
    //     {
    //         RunMultipleCalculatePiJobsWithRandomPriority(LARGE_NUMBER_OF_JOBS, HEAVY_WEIGHT_JOB_CALCULATE_PI_DEPTH);
    //     }
    // }

    // BENCHMARK_F(TaskflowBenchmarkFixture, RunSmallNumberOfRandomWeightJobsWithRandomPriorities)(benchmark::State& state)
    // {
    //     for (auto _ : state)
    //     {
    //         RunMultipleCalculatePiJobsWithRandomDepthAndRandomPriority(SMALL_NUMBER_OF_JOBS);
    //     }
    // }

    // BENCHMARK_F(TaskflowBenchmarkFixture, RunMediumNumberOfRandomWeightJobsWithRandomPriorities)(benchmark::State& state)
    // {
    //     for (auto _ : state)
    //     {
    //         RunMultipleCalculatePiJobsWithRandomDepthAndRandomPriority(MEDIUM_NUMBER_OF_JOBS);
    //     }
    // }

    // BENCHMARK_F(TaskflowBenchmarkFixture, RunLargeNumberOfRandomWeightJobsWithRandomPriorities)(benchmark::State& state)
    // {
    //     for (auto _ : state)
    //     {
    //         RunMultipleCalculatePiJobsWithRandomDepthAndRandomPriority(LARGE_NUMBER_OF_JOBS);
    //     }
    // }
} // Benchmark

#endif // HAVE_BENCHMARK
