/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Jobs/JobGraph.h>
#include <AzCore/Jobs/JobExecutor.h>
#include <AzCore/Memory/PoolAllocator.h>

#include <AzCore/UnitTest/TestTypes.h>

#include <random>

using AZ::JobDescriptor;
using AZ::JobGraph;
using AZ::JobGraphEvent;
using AZ::JobExecutor;
using AZ::Internal::TypeErasedJob;
using AZ::JobPriority;

static JobDescriptor defaultJD{ "JobGraphTestJob", "JobGraphTests" };

namespace UnitTest
{
    class JobGraphTestFixture : public AllocatorsTestFixture
    {
    public:
        void SetUp() override
        {
            AllocatorsTestFixture::SetUp();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            m_executor = aznew JobExecutor(4);
        }

        void TearDown() override
        {
            azdestroy(m_executor);
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();
            AllocatorsTestFixture::TearDown();
        }

    protected:
        JobExecutor* m_executor;
    };

    TEST(JobGraphTests, TrivialJobLambda)
    {
        int x = 0;

        TypeErasedJob job(
            defaultJD,
            [&x]()
            {
                ++x;
            });
        job.Invoke();

        EXPECT_EQ(1, x);
    }

    TEST(JobGraphTests, TrivialJobLambdaMove)
    {
        int x = 0;

        TypeErasedJob job(
            defaultJD,
            [&x]()
            {
                ++x;
            });

        TypeErasedJob job2 = AZStd::move(job);

        job2.Invoke();

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

    TEST(JobGraphTests, MoveOnlyJobLambda)
    {
        TrackMoves tm;
        int moveCount = 0;

        TypeErasedJob job(
            defaultJD,
            [tm = AZStd::move(tm), &moveCount]
            {
                moveCount = tm.moveCount;
            });
        job.Invoke();

        // Two moves are expected. Once into the capture body of the lambda, once to construct
        // the type erased job
        EXPECT_EQ(2, moveCount);
    }

    TEST(JobGraphTests, MoveOnlyJobLambdaMove)
    {
        TrackMoves tm;
        int moveCount = 0;

        TypeErasedJob job(
            defaultJD,
            [tm = AZStd::move(tm), &moveCount]
            {
                moveCount = tm.moveCount;
            });

        TypeErasedJob job2 = AZStd::move(job);
        job2.Invoke();

        EXPECT_EQ(3, moveCount);
    }

    TEST(JobGraphTests, CopyOnlyJobLambda)
    {
        TrackCopies tc;
        int copyCount = 0;

        TypeErasedJob job(
            defaultJD,
            [tc, &copyCount]
            {
                copyCount = tc.copyCount;
            });
        job.Invoke();

        // Two copies are expected. Once into the capture body of the lambda, once to construct
        // the type erased job
        EXPECT_EQ(2, copyCount);
    }

    TEST(JobGraphTests, CopyOnlyJobLambdaMove)
    {
        TrackCopies tc;
        int copyCount = 0;

        TypeErasedJob job(
            defaultJD,
            [tc, &copyCount]
            {
                copyCount = tc.copyCount;
            });
        TypeErasedJob job2 = AZStd::move(job);
        job2.Invoke();

        EXPECT_EQ(3, copyCount);
    }

    TEST(JobGraphTests, DestroyLambda)
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
            TypeErasedJob job(
                defaultJD,
                [td = AZStd::move(td)]
                {
                });
            job.Invoke();
            // Destructor should not have run yet (except on moved-from instances)
            EXPECT_EQ(x, 0);
        }

        // Destructor should have run now
        EXPECT_EQ(x, 1);
    }

    TEST_F(JobGraphTestFixture, SerialGraph)
    {
        int x = 0;

        JobGraph graph;
        auto a = graph.AddJob(
            defaultJD,
            [&]
            {
                x += 3;
            });
        auto b = graph.AddJob(
            defaultJD,
            [&]
            {
                x = 4 * x;
            });
        auto c = graph.AddJob(
            defaultJD,
            [&]
            {
                x -= 1;
            });

        a.Precedes(b);
        b.Precedes(c);

        JobGraphEvent ev;
        graph.SubmitOnExecutor(*m_executor, &ev);
        ev.Wait();

        EXPECT_EQ(11, x);
    }

    TEST_F(JobGraphTestFixture, DetachedGraph)
    {
        int x = 0;

        JobGraphEvent ev;

        {
            JobGraph graph;
            auto a = graph.AddJob(
                defaultJD,
                [&]
                {
                    x += 3;
                });
            auto b = graph.AddJob(
                defaultJD,
                [&]
                {
                    x = 4 * x;
                });
            auto c = graph.AddJob(
                defaultJD,
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

    TEST_F(JobGraphTestFixture, ForkJoin)
    {
        AZStd::atomic<int> x = 0;

        // Job a initializes x to 3
        // Job b and c toggles the lowest two bits atomically
        // Job d decrements x

        JobGraph graph;
        auto a = graph.AddJob(
            defaultJD,
            [&]
            {
                x = 0b111;
            });
        auto b = graph.AddJob(
            defaultJD,
            [&]
            {
                x ^= 1;
            });
        auto c = graph.AddJob(
            defaultJD,
            [&]
            {
                x ^= 2;
            });
        auto d = graph.AddJob(
            defaultJD,
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
        d.Succeeds(b, c);

        JobGraphEvent ev;
        graph.SubmitOnExecutor(*m_executor, &ev);
        ev.Wait();

        EXPECT_EQ(3, x);
    }

    TEST_F(JobGraphTestFixture, SpawnSubgraph)
    {
        AZStd::atomic<int> x = 0;

        JobGraph graph;
        auto a = graph.AddJob(
            defaultJD,
            [&]
            {
                x = 0b111;
            });
        auto b = graph.AddJob(
            defaultJD,
            [&]
            {
                x ^= 1;
            });
        auto c = graph.AddJob(
            defaultJD,
            [&]
            {
                x ^= 2;

                JobGraph subgraph;
                auto e = subgraph.AddJob(
                    defaultJD,
                    [&]
                    {
                        x ^= 0b1000;
                    });
                auto f = subgraph.AddJob(
                    defaultJD,
                    [&]
                    {
                        x ^= 0b10000;
                    });
                auto g = subgraph.AddJob(
                    defaultJD,
                    [&]
                    {
                        x += 0b1000;
                    });
                e.Precedes(g);
                f.Precedes(g);
                JobGraphEvent ev;
                subgraph.SubmitOnExecutor(*m_executor, &ev);
                ev.Wait();
            });
        auto d = graph.AddJob(
            defaultJD,
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

        JobGraphEvent ev;
        graph.SubmitOnExecutor(*m_executor, &ev);
        ev.Wait();

        EXPECT_EQ(3 | 0b100000, x);
    }

    TEST_F(JobGraphTestFixture, RetainedGraph)
    {
        AZStd::atomic<int> x = 0;

        JobGraph graph;
        auto a = graph.AddJob(
            defaultJD,
            [&]
            {
                x = 0b111;
            });
        auto b = graph.AddJob(
            defaultJD,
            [&]
            {
                x ^= 1;
            });
        auto c = graph.AddJob(
            defaultJD,
            [&]
            {
                x ^= 2;
            });
        auto d = graph.AddJob(
            defaultJD,
            [&]
            {
                x -= 1;
            });
        auto e = graph.AddJob(
            defaultJD,
            [&]
            {
                x ^= 0b1000;
            });
        auto f = graph.AddJob(
            defaultJD,
            [&]
            {
                x ^= 0b10000;
            });
        auto g = graph.AddJob(
            defaultJD,
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
        g.Succeeds(e, f);
        g.Precedes(d);

        JobGraphEvent ev;
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
    class JobGraphBenchmarkFixture : public ::benchmark::Fixture
    {
    public:
        void SetUp(benchmark::State&) override
        {
            executor = new JobExecutor;
            graph = new JobGraph;
        }

        void TearDown(benchmark::State&) override
        {
            delete graph;
            delete executor;
        }

        JobDescriptor descriptors[4] = { { "critical", "benchmark", JobPriority::CRITICAL },
                                         { "high", "benchmark", JobPriority::HIGH },
                                         { "medium", "benchmark", JobPriority::MEDIUM },
                                         { "low", "benchmark", JobPriority::LOW } };

        JobGraph* graph;
        JobExecutor* executor;
    };

    BENCHMARK_F(JobGraphBenchmarkFixture, QueueToDequeue)(benchmark::State& state)
    {
        graph->AddJob(
            descriptors[2],
            []
            {
            });
        for (auto _ : state)
        {
            JobGraphEvent ev;
            graph->SubmitOnExecutor(*executor, &ev);
            ev.Wait();
        }
    }

    BENCHMARK_F(JobGraphBenchmarkFixture, OneAfterAnother)(benchmark::State& state)
    {
        auto a = graph->AddJob(
            descriptors[2],
            []
            {
            });
        auto b = graph->AddJob(
            descriptors[2],
            []
            {
            });
        a.Precedes(b);

        for (auto _ : state)
        {
            JobGraphEvent ev;
            graph->SubmitOnExecutor(*executor, &ev);
            ev.Wait();
        }
    }

    BENCHMARK_F(JobGraphBenchmarkFixture, FourToOneJoin)(benchmark::State& state)
    {
        auto [a, b, c, d, e] = graph->AddJobs(
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

        e.Succeeds(a, b, c, d);

        for (auto _ : state)
        {
            JobGraphEvent ev;
            graph->SubmitOnExecutor(*executor, &ev);
            ev.Wait();
        }
    }
} // namespace Benchmark
#endif
