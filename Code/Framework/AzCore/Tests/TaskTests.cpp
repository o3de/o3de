/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Task/TaskGraph.h>
#include <AzCore/Task/TaskExecutor.h>
#include <AzCore/Memory/PoolAllocator.h>

#include <AzCore/UnitTest/TestTypes.h>

#include <random>

using AZ::TaskDescriptor;
using AZ::TaskGraph;
using AZ::TaskGraphEvent;
using AZ::TaskExecutor;
using AZ::Internal::Task;
using AZ::TaskPriority;

static TaskDescriptor defaultTD{ "TaskGraphTestTask", "TaskGraphTests" };

namespace UnitTest
{
    class TaskGraphTestFixture : public AllocatorsTestFixture
    {
    public:
        void SetUp() override
        {
            AllocatorsTestFixture::SetUp();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            m_executor = aznew TaskExecutor();
        }

        void TearDown() override
        {
            azdestroy(m_executor);
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();
            AllocatorsTestFixture::TearDown();
        }

    protected:
        TaskExecutor* m_executor;
    };

    TEST(TaskGraphTests, TrivialTaskLambda)
    {
        int x = 0;

        Task task(
            defaultTD,
            [&x]()
            {
                ++x;
            });
        task.Invoke();

        EXPECT_EQ(1, x);
    }

    TEST(TaskGraphTests, TrivialTaskLambdaMove)
    {
        int x = 0;

        Task task(
            defaultTD,
            [&x]()
            {
                ++x;
            });

        Task task2 = AZStd::move(task);

        task2.Invoke();

        EXPECT_EQ(1, x);
    }

    struct TrackMoves
    {
        TrackMoves() = default;

        TrackMoves(const TrackMoves&) = delete;

        TrackMoves(TrackMoves&& other)
            : moveCount{other.moveCount + 1}
        {
        }

        int moveCount = 0;
    };

    struct TrackCopies
    {
        TrackCopies() = default;

        TrackCopies(TrackCopies&&) = delete;

        TrackCopies(const TrackCopies& other)
            : copyCount{other.copyCount + 1}
        {
        }

        int copyCount = 0;
    };

    /*
    TEST(TaskGraphTests, ThisShouldNotCompile)
    {
        auto lambda = []
        {
        };

        Task task(defaultTD, lambda);
        task.Invoke();
    }
    */

    TEST(TaskGraphTests, MoveOnlyTaskLambda)
    {
        TrackMoves tm;
        int moveCount = 0;

        Task task(
            defaultTD,
            [tm = AZStd::move(tm), &moveCount]
            {
                moveCount = tm.moveCount;
            });
        task.Invoke();

        // Two moves are expected. Once into the capture body of the lambda, once to construct
        // the type erased task
        EXPECT_EQ(2, moveCount);
    }

    TEST(TaskGraphTests, MoveOnlyTaskLambdaMove)
    {
        TrackMoves tm;
        int moveCount = 0;

        Task task(
            defaultTD,
            [tm = AZStd::move(tm), &moveCount]
            {
                moveCount = tm.moveCount;
            });

        Task task2 = AZStd::move(task);
        task2.Invoke();

        EXPECT_EQ(3, moveCount);
    }

    TEST(TaskGraphTests, CopyOnlyTaskLambda)
    {
        TrackCopies tc;
        int copyCount = 0;

        Task task(
            defaultTD,
            [tc, &copyCount]
            {
                copyCount = tc.copyCount;
            });
        task.Invoke();

        // Two copies are expected. Once into the capture body of the lambda, once to construct
        // the type erased task
        EXPECT_EQ(2, copyCount);
    }

    TEST(TaskGraphTests, CopyOnlyTaskLambdaMove)
    {
        TrackCopies tc;
        int copyCount = 0;

        Task task(
            defaultTD,
            [tc, &copyCount]
            {
                copyCount = tc.copyCount;
            });
        Task task2 = AZStd::move(task);
        task2.Invoke();

        EXPECT_EQ(3, copyCount);
    }

    TEST(TaskGraphTests, DestroyLambda)
    {
        // This test ensures that for a lambda with a destructor, the destructor is invoked
        // exactly once on a non-moved-from object.
        int x = 0;
        struct TrackDestroy
        {
            TrackDestroy(int* px)
                : count{ px }
            {
            }
            TrackDestroy(TrackDestroy&& other)
                : count{ other.count }
            {
                other.count = nullptr;
            }
            ~TrackDestroy()
            {
                if (count)
                {
                    ++*count;
                }
            }
            int* count = nullptr;
        };

        {
            TrackDestroy td{ &x };
            Task task(
                defaultTD,
                [td = AZStd::move(td)]
                {
                    AZ_UNUSED(td);
                });
            task.Invoke();
            // Destructor should not have run yet (except on moved-from instances)
            EXPECT_EQ(x, 0);
        }

        // Destructor should have run now
        EXPECT_EQ(x, 1);
    }

    TEST_F(TaskGraphTestFixture, SingleTask)
    {
        AZStd::atomic_int32_t x = 0;

        TaskGraph graph;
        graph.AddTask(
            defaultTD,
            [&x]
            {
                x = 1;
            });

        TaskGraphEvent ev;
        graph.SubmitOnExecutor(*m_executor, &ev);
        ev.Wait();

        EXPECT_EQ(1, x);
    }


    TEST_F(TaskGraphTestFixture, SingleTaskChain)
    {
        AZStd::atomic_int32_t x = 0;

        TaskGraph graph;
        auto a = graph.AddTask(
            defaultTD,
            [&x]
            {
                x += 1;
            });
        auto b = graph.AddTask(
            defaultTD,
            [&x]
            {
                x += 1;
            });
        b.Precedes(a);

        TaskGraphEvent ev;
        graph.SubmitOnExecutor(*m_executor, &ev);
        ev.Wait();

        EXPECT_EQ(2, x);
    }

    TEST_F(TaskGraphTestFixture, MultipleIndependentTaskChains)
    {
        AZStd::atomic_int32_t x = 0;
        constexpr int numChains = 5;

        TaskGraph graph;
        for( int i = 0; i < numChains; ++i)
        {
            auto a = graph.AddTask(
                defaultTD,
                [&x]
                {
                    x += 1;
                });
            auto b = graph.AddTask(
                defaultTD,
                [&x]
                {
                    x += 1;
                });
            b.Precedes(a);
        }

        TaskGraphEvent ev;
        graph.SubmitOnExecutor(*m_executor, &ev);
        ev.Wait();

        EXPECT_EQ(2*numChains, x);
    }

    TEST_F(TaskGraphTestFixture, VariadicInterface)
    {
        int x = 0;

        TaskGraph graph;
        auto [a, b, c] = graph.AddTasks(
            defaultTD,
            [&]
            {
                x += 3;
            },
            [&]
            {
                x = 4 * x;
            },
            [&]
            {
                x -= 1;
            });

        a.Precedes(b);
        b.Precedes(c);

        TaskGraphEvent ev;
        graph.SubmitOnExecutor(*m_executor, &ev);
        ev.Wait();

        EXPECT_EQ(11, x);
    }

    TEST_F(TaskGraphTestFixture, SerialGraph)
    {
        int x = 0;

        TaskGraph graph;
        auto a = graph.AddTask(
            defaultTD,
            [&]
            {
                x += 3;
            });
        auto b = graph.AddTask(
            defaultTD,
            [&]
            {
                x = 4 * x;
            });
        auto c = graph.AddTask(
            defaultTD,
            [&]
            {
                x -= 1;
            });

        a.Precedes(b);
        b.Precedes(c);

        TaskGraphEvent ev;
        graph.SubmitOnExecutor(*m_executor, &ev);
        ev.Wait();

        EXPECT_EQ(11, x);
    }

    TEST_F(TaskGraphTestFixture, DetachedGraph)
    {
        int x = 0;

        TaskGraphEvent ev;

        {
            TaskGraph graph;
            auto a = graph.AddTask(
                defaultTD,
                [&]
                {
                    x += 3;
                });
            auto b = graph.AddTask(
                defaultTD,
                [&]
                {
                    x = 4 * x;
                });
            auto c = graph.AddTask(
                defaultTD,
                [&]
                {
                    x -= 1;
                });

            a.Precedes(b);
            b.Precedes(c);
            graph.Detach();
            graph.SubmitOnExecutor(*m_executor, &ev);
        }

        ev.Wait();

        EXPECT_EQ(11, x);
    }

    TEST_F(TaskGraphTestFixture, ForkJoin)
    {
        AZStd::atomic<int> x = 0;

        // Task a initializes x to 3
        // Task b and c toggles the lowest two bits atomically
        // Task d decrements x

        TaskGraph graph;
        auto a = graph.AddTask(
            defaultTD,
            [&]
            {
                x = 0b111;
            });
        auto b = graph.AddTask(
            defaultTD,
            [&]
            {
                x ^= 1;
            });
        auto c = graph.AddTask(
            defaultTD,
            [&]
            {
                x ^= 2;
            });
        auto d = graph.AddTask(
            defaultTD,
            [&]
            {
                x -= 1;
            });

        //   a  <-- Root
        //  / \
        // b   c
        //  \ /
        //   d

        a.Precedes(b, c);
        d.Follows(b, c);

        TaskGraphEvent ev;
        graph.SubmitOnExecutor(*m_executor, &ev);
        ev.Wait();

        EXPECT_EQ(3, x);
    }

    // Waiting inside a task is disallowed , test that it fails correctly
    TEST_F(TaskGraphTestFixture, SpawnSubgraph)
    {
        AZStd::atomic<int> x = 0;

        TaskGraph graph;
        auto a = graph.AddTask(
            defaultTD,
            [&]
            {
                x = 0b111;
            });
        auto b = graph.AddTask(
            defaultTD,
            [&]
            {
                x ^= 1;
            });
        auto c = graph.AddTask(
            defaultTD,
            [&]
            {
                x ^= 2;

                TaskGraph subgraph;
                auto e = subgraph.AddTask(
                    defaultTD,
                    [&]
                    {
                        x ^= 0b1000;
                    });
                auto f = subgraph.AddTask(
                    defaultTD,
                    [&]
                    {
                        x ^= 0b10000;
                    });
                auto g = subgraph.AddTask(
                    defaultTD,
                    [&]
                    {
                        x += 0b1000;
                    });
                e.Precedes(g);
                f.Precedes(g);
                TaskGraphEvent ev;
                subgraph.SubmitOnExecutor(*m_executor, &ev);
                // TaskGraphEvent::Wait asserts if called on a worker thread, suppress & validate assert
                AZ_TEST_START_TRACE_SUPPRESSION;
                ev.Wait();
                AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            });
        auto d = graph.AddTask(
            defaultTD,
            [&]
            {
                x -= 1;
            });

        // NOTE: The ideal way to express this topology is without the wait on the subgraph
        // at task g, but this is more an illustrative test. Better is to express the entire
        // graph in a single larger graph.
        //   a  <-- Root
        //  / \
        // b   c - f
        //  \   \   \
        //   \   e - g
        //    \     /
        //     \   /
        //      \ /
        //       d

        a.Precedes(b);
        a.Precedes(c);
        b.Precedes(d);
        c.Precedes(d);

        TaskGraphEvent ev;
        graph.SubmitOnExecutor(*m_executor, &ev);
        ev.Wait();
    }

    TEST_F(TaskGraphTestFixture, RetainedGraph)
    {
        AZStd::atomic<int> x = 0;

        TaskGraph graph;
        auto a = graph.AddTask(
            defaultTD,
            [&]
            {
                x = 0b111;
            });
        auto b = graph.AddTask(
            defaultTD,
            [&]
            {
                x ^= 1;
            });
        auto c = graph.AddTask(
            defaultTD,
            [&]
            {
                x ^= 2;
            });
        auto d = graph.AddTask(
            defaultTD,
            [&]
            {
                x -= 1;
            });
        auto e = graph.AddTask(
            defaultTD,
            [&]
            {
                x ^= 0b1000;
            });
        auto f = graph.AddTask(
            defaultTD,
            [&]
            {
                x ^= 0b10000;
            });
        auto g = graph.AddTask(
            defaultTD,
            [&]
            {
                x += 0b1000;
            });

        //   a  <-- Root
        //  / \
        // b   c - f
        //  \   \   \
        //   \   e - g
        //    \     /
        //     \   /
        //      \ /
        //       d

        a.Precedes(b, c);
        b.Precedes(d);
        c.Precedes(e, f);
        g.Follows(e, f);
        g.Precedes(d);

        TaskGraphEvent ev;
        graph.SubmitOnExecutor(*m_executor, &ev);
        ev.Wait();

        EXPECT_EQ(3 | 0b100000, x);
        x = 0;

        graph.SubmitOnExecutor(*m_executor, &ev);
        ev.Wait();

        EXPECT_EQ(3 | 0b100000, x);
    }
} // namespace UnitTest

#if defined(HAVE_BENCHMARK)
namespace Benchmark
{
    class TaskGraphBenchmarkFixture : public ::benchmark::Fixture
    {
        void internalSetUp()
        {
            executor = new TaskExecutor;
            graph = new TaskGraph;
        }

        void internalTearDown()
        {
            delete graph;
            delete executor;
        }

    public:
        void SetUp(const benchmark::State&) override
        {
            internalSetUp();
        }
        void SetUp(benchmark::State&) override
        {
            internalSetUp();
        }

        void TearDown(const benchmark::State&) override
        {
            internalTearDown();
        }
        void TearDown(benchmark::State&) override
        {
            internalTearDown();
        }

        TaskDescriptor descriptors[4] = { { "critical", "benchmark", TaskPriority::CRITICAL },
                                         { "high", "benchmark", TaskPriority::HIGH },
                                         { "medium", "benchmark", TaskPriority::MEDIUM },
                                         { "low", "benchmark", TaskPriority::LOW } };

        TaskGraph* graph;
        TaskExecutor* executor;
    };

    BENCHMARK_F(TaskGraphBenchmarkFixture, QueueToDequeue)(benchmark::State& state)
    {
        graph->AddTask(
            descriptors[2],
            []
            {
            });
        for (auto _ : state)
        {
            TaskGraphEvent ev;
            graph->SubmitOnExecutor(*executor, &ev);
            ev.Wait();
        }
    }

    BENCHMARK_F(TaskGraphBenchmarkFixture, OneAfterAnother)(benchmark::State& state)
    {
        auto a = graph->AddTask(
            descriptors[2],
            []
            {
            });
        auto b = graph->AddTask(
            descriptors[2],
            []
            {
            });
        a.Precedes(b);

        for (auto _ : state)
        {
            TaskGraphEvent ev;
            graph->SubmitOnExecutor(*executor, &ev);
            ev.Wait();
        }
    }

    BENCHMARK_F(TaskGraphBenchmarkFixture, FourToOneJoin)(benchmark::State& state)
    {
        auto [a, b, c, d, e] = graph->AddTasks(
            descriptors[2],
            []
            {
            },
            []
            {
            },
            []
            {
            },
            []
            {
            },
            []
            {
            });

        e.Follows(a, b, c, d);

        for (auto _ : state)
        {
            TaskGraphEvent ev;
            graph->SubmitOnExecutor(*executor, &ev);
            ev.Wait();
        }
    }
} // namespace Benchmark
#endif
