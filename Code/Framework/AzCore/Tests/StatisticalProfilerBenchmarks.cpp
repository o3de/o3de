/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/StatisticalProfilerHelpers.h>

#if defined(HAVE_BENCHMARK)

#include <benchmark/benchmark.h>

namespace Benchmark
{
#define REGISTER_STATS_PROFILER_SINGLETHREADED_BENCHMARK(_fixture, _function) \
    BENCHMARK_REGISTER_F(_fixture, _function) \
        ->Iterations(UnitTest::LargeIterationCount);

#define REGISTER_STATS_PROFILER_MULTITHREADED_BENCHMARK(_fixture, _function) \
    BENCHMARK_REGISTER_F(_fixture, _function) \
        ->ThreadRange(1, AZStd::thread::hardware_concurrency()) \
        ->Iterations(UnitTest::LargeIterationCount);

    // -- AZ::Statistics::StatisticalProfiler benchmarks --

    template<class StatIdType = AZStd::string, class MutexType = AZ::NullMutex>
    class StatisticalProfilerBenchmark
        : public UnitTest::AllocatorsBenchmarkFixture
    {
        using ProfilerType = AZ::Statistics::StatisticalProfiler<StatIdType, MutexType>;
        using TimedScopeType = typename ProfilerType::TimedScope;
    public:
        void RunBenchmark(benchmark::State& state)
        {
            if (state.thread_index() == 0)
            {
                m_profiler = new ProfilerType;

                for (int i = 0; i < state.threads(); ++i)
                {
                    const AZStd::string threadStatName = AZStd::string::format("thread%03d", i);
                    const auto threadStatId = UnitTest::ConvertNameToStatId<StatIdType>(threadStatName);

                    m_profiler->GetStatsManager().AddStatistic(threadStatId, threadStatName, "us");
                }
            }

            const AZStd::string statName = AZStd::string::format("thread%03d", state.thread_index());
            const auto statId = UnitTest::ConvertNameToStatId<StatIdType>(statName);

            for ([[maybe_unused]] auto _ : state)
            {
                TimedScopeType timedScope(*m_profiler, statId);
                benchmark::DoNotOptimize(m_profiler);
            }

            if (state.thread_index() == 0)
            {
                delete m_profiler;
            }
        }

    protected:
        ProfilerType* m_profiler;
    };

    BENCHMARK_TEMPLATE_DEFINE_F(StatisticalProfilerBenchmark, String_SingleThreadedPerf)(benchmark::State& state)
    {
        RunBenchmark(state);
    }

    BENCHMARK_TEMPLATE_DEFINE_F(StatisticalProfilerBenchmark, HashValue32_SingleThreadedPerf, AZ::HashValue32)(benchmark::State& state)
    {
        RunBenchmark(state);
    }

    REGISTER_STATS_PROFILER_SINGLETHREADED_BENCHMARK(StatisticalProfilerBenchmark, String_SingleThreadedPerf)
    REGISTER_STATS_PROFILER_SINGLETHREADED_BENCHMARK(StatisticalProfilerBenchmark, HashValue32_SingleThreadedPerf)

    BENCHMARK_TEMPLATE_DEFINE_F(StatisticalProfilerBenchmark, String_Mutex_ThreadedPerf, AZStd::string, AZStd::mutex)(benchmark::State& state)
    {
        RunBenchmark(state);
    }

    BENCHMARK_TEMPLATE_DEFINE_F(StatisticalProfilerBenchmark, HashValue32_Mutex_ThreadedPerf, AZ::HashValue32, AZStd::mutex)(benchmark::State& state)
    {
        RunBenchmark(state);
    }

    REGISTER_STATS_PROFILER_MULTITHREADED_BENCHMARK(StatisticalProfilerBenchmark, String_Mutex_ThreadedPerf)
    REGISTER_STATS_PROFILER_MULTITHREADED_BENCHMARK(StatisticalProfilerBenchmark, HashValue32_Mutex_ThreadedPerf)

    BENCHMARK_TEMPLATE_DEFINE_F(StatisticalProfilerBenchmark, String_SpinMutex_ThreadedPerf, AZStd::string, AZStd::spin_mutex)(benchmark::State& state)
    {
        RunBenchmark(state);
    }

    BENCHMARK_TEMPLATE_DEFINE_F(StatisticalProfilerBenchmark, HashValue32_SpinMutex_ThreadedPerf, AZ::HashValue32, AZStd::spin_mutex)(benchmark::State& state)
    {
        RunBenchmark(state);
    }

    REGISTER_STATS_PROFILER_MULTITHREADED_BENCHMARK(StatisticalProfilerBenchmark, String_SpinMutex_ThreadedPerf)
    REGISTER_STATS_PROFILER_MULTITHREADED_BENCHMARK(StatisticalProfilerBenchmark, HashValue32_SpinMutex_ThreadedPerf)

    BENCHMARK_TEMPLATE_DEFINE_F(StatisticalProfilerBenchmark, String_SharedMutex_ThreadedPerf, AZStd::string, AZStd::shared_mutex)(benchmark::State& state)
    {
        RunBenchmark(state);
    }

    BENCHMARK_TEMPLATE_DEFINE_F(StatisticalProfilerBenchmark, HashValue32_SharedMutex_ThreadedPerf, AZ::HashValue32, AZStd::shared_mutex)(benchmark::State& state)
    {
        RunBenchmark(state);
    }

    REGISTER_STATS_PROFILER_MULTITHREADED_BENCHMARK(StatisticalProfilerBenchmark, String_SharedMutex_ThreadedPerf)
    REGISTER_STATS_PROFILER_MULTITHREADED_BENCHMARK(StatisticalProfilerBenchmark, HashValue32_SharedMutex_ThreadedPerf)

    // -- AZ::Statistics::StatisticalProfilerProxy benchmarks --

    class StatisticalProfilerProxyBenchmark
        : public UnitTest::AllocatorsBenchmarkFixture
    {
        using ProxyType = AZ::Statistics::StatisticalProfilerProxy;
        using ProfilerType = ProxyType::StatisticalProfilerType;
        using StatIdType = ProxyType::StatIdType;
        using TimedScopeType = ProxyType::TimedScope;

    public:
        void RunBenchmark(benchmark::State& state)
        {
            if (state.thread_index() == 0)
            {
                TimedScopeType::ClearCachedProxy();

                m_proxy = new ProxyType;
                ProfilerType& profiler = m_proxy->GetProfiler(UnitTest::ProfilerProxyGroup);

                for (int i = 0; i < state.threads(); ++i)
                {
                    const AZStd::string threadStatName = AZStd::string::format("thread%03d", i);
                    const auto threadStatId = UnitTest::ConvertNameToStatId<StatIdType>(threadStatName);

                    profiler.GetStatsManager().AddStatistic(threadStatId, threadStatName, "us");
                }

                m_proxy->ActivateProfiler(UnitTest::ProfilerProxyGroup, true);
            }

            const AZStd::string statName = AZStd::string::format("thread%03d", state.thread_index());
            const auto statId = UnitTest::ConvertNameToStatId<StatIdType>(statName);

            for ([[maybe_unused]] auto _ : state)
            {
                TimedScopeType timedScope(UnitTest::ProfilerProxyGroup, statId);
                benchmark::DoNotOptimize(m_proxy);
            }

            if (state.thread_index() == 0)
            {
                delete m_proxy;
            }
        }

    protected:
        ProxyType* m_proxy;
    };

    BENCHMARK_DEFINE_F(StatisticalProfilerProxyBenchmark, GeneralPerf)(benchmark::State& state)
    {
        RunBenchmark(state);
    }

    REGISTER_STATS_PROFILER_MULTITHREADED_BENCHMARK(StatisticalProfilerProxyBenchmark, GeneralPerf)
} // namespace Benchmark

#endif // defined(HAVE_BENCHMARK)
