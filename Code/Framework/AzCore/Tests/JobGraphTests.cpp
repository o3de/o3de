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
        b.Precedes(d);
        c.Precedes(d);

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
        e.Precedes(g);
        f.Precedes(g);
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
        static const int32_t LIGHT_WEIGHT_JOB_CALCULATE_PI_DEPTH = 1;
        static const int32_t MEDIUM_WEIGHT_JOB_CALCULATE_PI_DEPTH = 1024;
        static const int32_t HEAVY_WEIGHT_JOB_CALCULATE_PI_DEPTH = 1048576;

        static const int32_t SMALL_NUMBER_OF_JOBS = 10;
        static const int32_t MEDIUM_NUMBER_OF_JOBS = 1024;
        static const int32_t LARGE_NUMBER_OF_JOBS = 16384;
        static AZStd::atomic<int32_t> s_numIncompleteJobs;

        int m_depth = 1;
        JobGraph* graphs;

        void SetUp(benchmark::State&) override
        {
            s_numIncompleteJobs = 0;

            m_executor = aznew JobExecutor(0);
            graphs = new JobGraph[4];

            // Generate some random priorities
            m_randomPriorities.resize(LARGE_NUMBER_OF_JOBS);
            std::mt19937_64 randomPriorityGenerator(1); // Always use the same seed
            std::uniform_int_distribution<> randomPriorityDistribution(0, static_cast<uint8_t>(AZ::JobPriority::PRIORITY_COUNT));
            std::generate(
                m_randomPriorities.begin(), m_randomPriorities.end(),
                [&randomPriorityDistribution, &randomPriorityGenerator]()
                {
                    return randomPriorityDistribution(randomPriorityGenerator);
                });

            // Generate some random depths
            m_randomDepths.resize(LARGE_NUMBER_OF_JOBS);
            std::mt19937_64 randomDepthGenerator(1); // Always use the same seed
            std::uniform_int_distribution<> randomDepthDistribution(
                LIGHT_WEIGHT_JOB_CALCULATE_PI_DEPTH, HEAVY_WEIGHT_JOB_CALCULATE_PI_DEPTH);
            std::generate(
                m_randomDepths.begin(), m_randomDepths.end(),
                [&randomDepthDistribution, &randomDepthGenerator]()
                {
                    return randomDepthDistribution(randomDepthGenerator);
                });

            for (size_t i = 0; i != 4; ++i)
            {
                graphs[i].AddJob(
                    descriptors[i],
                    [this]
                    {
                        benchmark::DoNotOptimize(CalculatePi(m_depth));
                        --s_numIncompleteJobs;
                    });
            }
        }

        void TearDown(benchmark::State&) override
        {
            delete[] graphs;
            azdestroy(m_executor);
            m_randomDepths = {};
            m_randomPriorities = {};
        }

        JobDescriptor descriptors[4] = { { "critical", "benchmark", JobPriority::CRITICAL },
                                         { "high", "benchmark", JobPriority::HIGH },
                                         { "mediium", "benchmark", JobPriority::MEDIUM },
                                         { "low", "benchmark", JobPriority::LOW } };

        static inline double CalculatePi(AZ::u32 depth)
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

        void RunCalculatePiJob(int32_t depth, int8_t priority)
        {
            m_depth = depth;
            ++s_numIncompleteJobs;

            graphs[priority].SubmitOnExecutor(*m_executor);
        }

        void RunMultipleCalculatePiJobsWithDefaultPriority(uint32_t numberOfJobs, int32_t depth)
        {
            for (size_t i = 0; i != numberOfJobs; ++i)
            {
                RunCalculatePiJob(depth, 2);
            }

            while (s_numIncompleteJobs > 0)
            {
            }
        }

        void RunMultipleCalculatePiJobsWithRandomPriority(uint32_t numberOfJobs, int32_t depth)
        {
            for (size_t i = 0; i != numberOfJobs; ++i)
            {
                RunCalculatePiJob(depth, m_randomPriorities[i]);
            }

            while (s_numIncompleteJobs > 0)
            {
            }
        }

        void RunMultipleCalculatePiJobsWithRandomDepthAndDefaultPriority(uint32_t numberOfJobs)
        {
            for (size_t i = 0; i != numberOfJobs; ++i)
            {
                RunCalculatePiJob(m_randomDepths[i], 0);
            }

            while (s_numIncompleteJobs > 0)
            {
            }
        }

        void RunMultipleCalculatePiJobsWithRandomDepthAndRandomPriority(uint32_t numberOfJobs)
        {
            for (size_t i = 0; i != numberOfJobs; ++i)
            {
                RunCalculatePiJob(m_randomDepths[i], m_randomPriorities[i]);
            }

            while (s_numIncompleteJobs > 0)
            {
            }
        }

        JobExecutor* m_executor;
        AZStd::vector<AZ::u32> m_randomDepths;
        AZStd::vector<AZ::s8> m_randomPriorities;
    };

    AZStd::atomic<int32_t> JobGraphBenchmarkFixture::s_numIncompleteJobs = 0;

    BENCHMARK_F(JobGraphBenchmarkFixture, RunSmallNumberOfLightWeightJobsWithDefaultPriority)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithDefaultPriority(SMALL_NUMBER_OF_JOBS, LIGHT_WEIGHT_JOB_CALCULATE_PI_DEPTH);
        }
    }

    BENCHMARK_F(JobGraphBenchmarkFixture, RunMediumNumberOfLightWeightJobsWithDefaultPriority)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithDefaultPriority(MEDIUM_NUMBER_OF_JOBS, LIGHT_WEIGHT_JOB_CALCULATE_PI_DEPTH);
        }
    }

    BENCHMARK_F(JobGraphBenchmarkFixture, RunLargeNumberOfLightWeightJobsWithDefaultPriority)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithDefaultPriority(LARGE_NUMBER_OF_JOBS, LIGHT_WEIGHT_JOB_CALCULATE_PI_DEPTH);
        }
    }

    BENCHMARK_F(JobGraphBenchmarkFixture, RunSmallNumberOfMediumWeightJobsWithDefaultPriority)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithDefaultPriority(SMALL_NUMBER_OF_JOBS, MEDIUM_WEIGHT_JOB_CALCULATE_PI_DEPTH);
        }
    }

    BENCHMARK_F(JobGraphBenchmarkFixture, RunMediumNumberOfMediumWeightJobsWithDefaultPriority)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithDefaultPriority(MEDIUM_NUMBER_OF_JOBS, MEDIUM_WEIGHT_JOB_CALCULATE_PI_DEPTH);
        }
    }

    BENCHMARK_F(JobGraphBenchmarkFixture, RunLargeNumberOfMediumWeightJobsWithDefaultPriority)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithDefaultPriority(LARGE_NUMBER_OF_JOBS, MEDIUM_WEIGHT_JOB_CALCULATE_PI_DEPTH);
        }
    }

    BENCHMARK_F(JobGraphBenchmarkFixture, RunSmallNumberOfHeavyWeightJobsWithDefaultPriority)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithDefaultPriority(SMALL_NUMBER_OF_JOBS, HEAVY_WEIGHT_JOB_CALCULATE_PI_DEPTH);
        }
    }

    BENCHMARK_F(JobGraphBenchmarkFixture, RunMediumNumberOfHeavyWeightJobsWithDefaultPriority)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithDefaultPriority(MEDIUM_NUMBER_OF_JOBS, HEAVY_WEIGHT_JOB_CALCULATE_PI_DEPTH);
        }
    }

    BENCHMARK_F(JobGraphBenchmarkFixture, RunLargeNumberOfHeavyWeightJobsWithDefaultPriority)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithDefaultPriority(LARGE_NUMBER_OF_JOBS, HEAVY_WEIGHT_JOB_CALCULATE_PI_DEPTH);
        }
    }

    BENCHMARK_F(JobGraphBenchmarkFixture, RunSmallNumberOfRandomWeightJobsWithDefaultPriority)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithRandomDepthAndDefaultPriority(SMALL_NUMBER_OF_JOBS);
        }
    }

    BENCHMARK_F(JobGraphBenchmarkFixture, RunMediumNumberOfRandomWeightJobsWithDefaultPriority)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithRandomDepthAndDefaultPriority(MEDIUM_NUMBER_OF_JOBS);
        }
    }

    BENCHMARK_F(JobGraphBenchmarkFixture, RunLargeNumberOfRandomWeightJobsWithDefaultPriority)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithRandomDepthAndDefaultPriority(LARGE_NUMBER_OF_JOBS);
        }
    }

    BENCHMARK_F(JobGraphBenchmarkFixture, RunSmallNumberOfLightWeightJobsWithRandomPriorities)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithRandomPriority(SMALL_NUMBER_OF_JOBS, LIGHT_WEIGHT_JOB_CALCULATE_PI_DEPTH);
        }
    }

    BENCHMARK_F(JobGraphBenchmarkFixture, RunMediumNumberOfLightWeightJobsWithRandomPriorities)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithRandomPriority(MEDIUM_NUMBER_OF_JOBS, LIGHT_WEIGHT_JOB_CALCULATE_PI_DEPTH);
        }
    }

    BENCHMARK_F(JobGraphBenchmarkFixture, RunLargeNumberOfLightWeightJobsWithRandomPriorities)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithRandomPriority(LARGE_NUMBER_OF_JOBS, LIGHT_WEIGHT_JOB_CALCULATE_PI_DEPTH);
        }
    }

    BENCHMARK_F(JobGraphBenchmarkFixture, RunSmallNumberOfMediumWeightJobsWithRandomPriorities)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithRandomPriority(SMALL_NUMBER_OF_JOBS, MEDIUM_WEIGHT_JOB_CALCULATE_PI_DEPTH);
        }
    }

    BENCHMARK_F(JobGraphBenchmarkFixture, RunMediumNumberOfMediumWeightJobsWithRandomPriorities)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithRandomPriority(MEDIUM_NUMBER_OF_JOBS, MEDIUM_WEIGHT_JOB_CALCULATE_PI_DEPTH);
        }
    }

    BENCHMARK_F(JobGraphBenchmarkFixture, RunLargeNumberOfMediumWeightJobsWithRandomPriorities)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithRandomPriority(LARGE_NUMBER_OF_JOBS, MEDIUM_WEIGHT_JOB_CALCULATE_PI_DEPTH);
        }
    }

    BENCHMARK_F(JobGraphBenchmarkFixture, RunSmallNumberOfHeavyWeightJobsWithRandomPriorities)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithRandomPriority(SMALL_NUMBER_OF_JOBS, HEAVY_WEIGHT_JOB_CALCULATE_PI_DEPTH);
        }
    }

    BENCHMARK_F(JobGraphBenchmarkFixture, RunMediumNumberOfHeavyWeightJobsWithRandomPriorities)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithRandomPriority(MEDIUM_NUMBER_OF_JOBS, HEAVY_WEIGHT_JOB_CALCULATE_PI_DEPTH);
        }
    }

    BENCHMARK_F(JobGraphBenchmarkFixture, RunLargeNumberOfHeavyWeightJobsWithRandomPriorities)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithRandomPriority(LARGE_NUMBER_OF_JOBS, HEAVY_WEIGHT_JOB_CALCULATE_PI_DEPTH);
        }
    }

    BENCHMARK_F(JobGraphBenchmarkFixture, RunSmallNumberOfRandomWeightJobsWithRandomPriorities)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithRandomDepthAndRandomPriority(SMALL_NUMBER_OF_JOBS);
        }
    }

    BENCHMARK_F(JobGraphBenchmarkFixture, RunMediumNumberOfRandomWeightJobsWithRandomPriorities)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithRandomDepthAndRandomPriority(MEDIUM_NUMBER_OF_JOBS);
        }
    }

    BENCHMARK_F(JobGraphBenchmarkFixture, RunLargeNumberOfRandomWeightJobsWithRandomPriorities)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            RunMultipleCalculatePiJobsWithRandomDepthAndRandomPriority(LARGE_NUMBER_OF_JOBS);
        }
    }
} // namespace Benchmark
#endif
