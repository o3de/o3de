/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Statistics/StatisticalProfilerProxy.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <AzCore/std/parallel/spin_mutex.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/string/string.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/Utils/TypeHash.h>

namespace UnitTest
{
    AZ_TYPE_SAFE_INTEGRAL(StringHash, size_t);

    constexpr int SmallIterationCount  = 10;
    constexpr int MediumIterationCount = 1000;
    constexpr int LargeIterationCount  = 100000;

    constexpr AZ::u32 ProfilerProxyGroup = AZ_CRC_CE("StatisticalProfilerProxyTests");

    template<typename StatIdType>
    StatIdType ConvertNameToStatId(const AZStd::string& name);

    template<>
    AZStd::string ConvertNameToStatId<AZStd::string>(const AZStd::string& name)
    {
        return name;
    }

    template<>
    StringHash ConvertNameToStatId<StringHash>(const AZStd::string& name)
    {
        AZStd::hash<AZStd::string> hasher;
        return StringHash(hasher(name));
    }

    template<>
    AZ::Crc32 ConvertNameToStatId<AZ::Crc32>(const AZStd::string& name)
    {
        return AZ::Crc32(name);
    }

    template<>
    AZ::HashValue32 ConvertNameToStatId<AZ::HashValue32>(const AZStd::string& name)
    {
        return AZ::TypeHash32(name.c_str());
    }

    template<>
    AZ::HashValue64 ConvertNameToStatId<AZ::HashValue64>(const AZStd::string& name)
    {
        return AZ::TypeHash64(name.c_str());
    }

    template<typename ProfilerType, typename StatIdType>
    void RecordStatistics(ProfilerType& profiler, const int loopCount, const StatIdType& rootId, const StatIdType& loopId)
    {
        typename ProfilerType::TimedScope rootScope(profiler, rootId);

        int counter = 0;
        for (int i = 0; i < loopCount; ++i)
        {
            typename ProfilerType::TimedScope loopScope(profiler, loopId);
            ++counter;
        }
    }

    void RecordStatistics(const int loopCount,
        const AZ::Statistics::StatisticalProfilerProxy::StatIdType& rootId,
        const AZ::Statistics::StatisticalProfilerProxy::StatIdType& loopId)
    {
        AZ::Statistics::StatisticalProfilerProxy::TimedScope rootScope(ProfilerProxyGroup, rootId);

        int counter = 0;
        for (int i = 0; i < loopCount; ++i)
        {
            AZ::Statistics::StatisticalProfilerProxy::TimedScope loopScope(ProfilerProxyGroup, loopId);
            ++counter;
        }
    }

    class AllocatorsWithTraceFixture
        : public AllocatorsFixture
        , public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            AZ::Debug::TraceMessageBus::Handler::BusConnect();
        }

        void TearDown() override
        {
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
            AllocatorsFixture::TearDown();
        }

        bool OnPrintf(const char* window, const char* message) override
        {
            return OnOutput(window, message);
        }

        bool OnOutput(const char* window, const char* message) override
        {
            printf("%s: %s", window, message);
            return false;
        }
    };

    // -- AZ::Statistics::StatisticalProfiler tests --

    template<class S = AZStd::string, class M = AZ::NullMutex>
    struct StatisticalProfilerTestTraits
    {
        using StatIdType = S;
        using MutexType = M;
        using ProfilerType = AZ::Statistics::StatisticalProfiler<StatIdType, MutexType>;
    };

    template<class Traits>
    class StatisticalProfilerFixture
        : public AllocatorsWithTraceFixture
    {
    };

    using StatisticalProfilerTestTypes = ::testing::Types<
        StatisticalProfilerTestTraits<>,
        StatisticalProfilerTestTraits<StringHash>,
        StatisticalProfilerTestTraits<AZ::Crc32>,
        StatisticalProfilerTestTraits<AZ::HashValue32>,
        StatisticalProfilerTestTraits<AZ::HashValue64>,

        StatisticalProfilerTestTraits<AZStd::string, AZStd::mutex>,
        StatisticalProfilerTestTraits<StringHash, AZStd::mutex>,
        StatisticalProfilerTestTraits<AZ::Crc32, AZStd::mutex>,
        StatisticalProfilerTestTraits<AZ::HashValue32, AZStd::mutex>,
        StatisticalProfilerTestTraits<AZ::HashValue64, AZStd::mutex>,

        StatisticalProfilerTestTraits<AZStd::string, AZStd::spin_mutex>,
        StatisticalProfilerTestTraits<StringHash, AZStd::spin_mutex>,
        StatisticalProfilerTestTraits<AZ::Crc32, AZStd::spin_mutex>,
        StatisticalProfilerTestTraits<AZ::HashValue32, AZStd::spin_mutex>,
        StatisticalProfilerTestTraits<AZ::HashValue64, AZStd::spin_mutex>,

        StatisticalProfilerTestTraits<AZStd::string, AZStd::shared_mutex>,
        StatisticalProfilerTestTraits<StringHash, AZStd::shared_mutex>,
        StatisticalProfilerTestTraits<AZ::Crc32, AZStd::shared_mutex>,
        StatisticalProfilerTestTraits<AZ::HashValue32, AZStd::shared_mutex>,
        StatisticalProfilerTestTraits<AZ::HashValue64, AZStd::shared_mutex>
    >;
    TYPED_TEST_CASE(StatisticalProfilerFixture, StatisticalProfilerTestTypes);

    TYPED_TEST(StatisticalProfilerFixture, ProfileCode_SingleThread_ValidateStatistics)
    {
        using StatIdType = typename TypeParam::StatIdType;

        typename TypeParam::ProfilerType profiler;

        const AZStd::string statNamePerformance("PerformanceResult");
        const auto statIdPerformance = ConvertNameToStatId<StatIdType>(statNamePerformance);

        const AZStd::string statNameBlock("Block");
        const auto statIdBlock = ConvertNameToStatId<StatIdType>(statNameBlock);

        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdPerformance, statNamePerformance, "us") != nullptr);
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdBlock, statNameBlock, "us") != nullptr);

        RecordStatistics(profiler, SmallIterationCount, statIdPerformance, statIdBlock);

        ASSERT_TRUE(profiler.GetStatistic(statIdPerformance) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdPerformance)->GetNumSamples(), 1);

        ASSERT_TRUE(profiler.GetStatistic(statIdBlock) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdBlock)->GetNumSamples(), SmallIterationCount);
    }

    template<class Traits>
    class ThreadedStatisticalProfilerFixture
        : public AllocatorsWithTraceFixture
    {
    };

    using ThreadedStatisticalProfilerTestTypes = ::testing::Types<
        StatisticalProfilerTestTraits<AZStd::string, AZStd::mutex>,
        StatisticalProfilerTestTraits<StringHash, AZStd::mutex>,
        StatisticalProfilerTestTraits<AZ::Crc32, AZStd::mutex>,
        StatisticalProfilerTestTraits<AZ::HashValue32, AZStd::mutex>,
        StatisticalProfilerTestTraits<AZ::HashValue64, AZStd::mutex>,

        StatisticalProfilerTestTraits<AZStd::string, AZStd::spin_mutex>,
        StatisticalProfilerTestTraits<StringHash, AZStd::spin_mutex>,
        StatisticalProfilerTestTraits<AZ::Crc32, AZStd::spin_mutex>,
        StatisticalProfilerTestTraits<AZ::HashValue32, AZStd::spin_mutex>,
        StatisticalProfilerTestTraits<AZ::HashValue64, AZStd::spin_mutex>,

        StatisticalProfilerTestTraits<AZStd::string, AZStd::shared_mutex>,
        StatisticalProfilerTestTraits<StringHash, AZStd::shared_mutex>,
        StatisticalProfilerTestTraits<AZ::Crc32, AZStd::shared_mutex>,
        StatisticalProfilerTestTraits<AZ::HashValue32, AZStd::shared_mutex>,
        StatisticalProfilerTestTraits<AZ::HashValue64, AZStd::shared_mutex>
    >;
    TYPED_TEST_CASE(ThreadedStatisticalProfilerFixture, ThreadedStatisticalProfilerTestTypes);

    TYPED_TEST(ThreadedStatisticalProfilerFixture, ProfileCode_4Threads_ValidateStatistics)
    {
        using StatIdType = typename TypeParam::StatIdType;

        typename TypeParam::ProfilerType profiler;

        const AZStd::string statNameThread1("thread1");
        const auto statIdThread1 = ConvertNameToStatId<StatIdType>(statNameThread1);
        const AZStd::string statNameThread1Loop("thread1_loop");
        const auto statIdThread1Loop = ConvertNameToStatId<StatIdType>(statNameThread1Loop);

        const AZStd::string statNameThread2("thread2");
        const auto statIdThread2 = ConvertNameToStatId<StatIdType>(statNameThread2);
        const AZStd::string statNameThread2Loop("thread2_loop");
        const auto statIdThread2Loop = ConvertNameToStatId<StatIdType>(statNameThread2Loop);

        const AZStd::string statNameThread3("thread3");
        const auto statIdThread3 = ConvertNameToStatId<StatIdType>(statNameThread3);
        const AZStd::string statNameThread3Loop("thread3_loop");
        const auto statIdThread3Loop = ConvertNameToStatId<StatIdType>(statNameThread3Loop);

        const AZStd::string statNameThread4("thread4");
        const auto statIdThread4 = ConvertNameToStatId<StatIdType>(statNameThread4);
        const AZStd::string statNameThread4Loop("thread4_loop");
        const auto statIdThread4Loop = ConvertNameToStatId<StatIdType>(statNameThread4Loop);

        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread1, statNameThread1, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread1Loop, statNameThread1Loop, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread2, statNameThread2, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread2Loop, statNameThread2Loop, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread3, statNameThread3, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread3Loop, statNameThread3Loop, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread4, statNameThread4, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread4Loop, statNameThread4Loop, "us"));

        AZStd::thread t1([&](){
            RecordStatistics(profiler, MediumIterationCount, statIdThread1, statIdThread1Loop);
        });
        AZStd::thread t2([&](){
            RecordStatistics(profiler, MediumIterationCount, statIdThread2, statIdThread2Loop);
        });
        AZStd::thread t3([&](){
            RecordStatistics(profiler, MediumIterationCount, statIdThread3, statIdThread3Loop);
        });
        AZStd::thread t4([&](){
            RecordStatistics(profiler, MediumIterationCount, statIdThread4, statIdThread4Loop);
        });
        t1.join();
        t2.join();
        t3.join();
        t4.join();

        ASSERT_TRUE(profiler.GetStatistic(statIdThread1) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread1)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread1Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread1Loop)->GetNumSamples(), MediumIterationCount);

        ASSERT_TRUE(profiler.GetStatistic(statIdThread2) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread2)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread2Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread2Loop)->GetNumSamples(), MediumIterationCount);

        ASSERT_TRUE(profiler.GetStatistic(statIdThread3) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread3)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread3Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread3Loop)->GetNumSamples(), MediumIterationCount);

        ASSERT_TRUE(profiler.GetStatistic(statIdThread4) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread4)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread4Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread4Loop)->GetNumSamples(), MediumIterationCount);
    }

    // -- AZ::Statistics::StatisticalProfilerProxy tests --

    class StatisticalProfilerProxyFixture
        : public AllocatorsWithTraceFixture
    {
    public:
        using ProxyType = AZ::Statistics::StatisticalProfilerProxy;

        void SetUp() override
        {
            AllocatorsWithTraceFixture::SetUp();
            ProxyType::TimedScope::ClearCachedProxy();
        }
    };

    TEST_F(StatisticalProfilerProxyFixture, ProfileCode_SingleThread_ValidateStatistics)
    {
        ProxyType profilerProxy;

        ProxyType* proxy = AZ::Interface<ProxyType>::Get();
        ASSERT_TRUE(proxy != nullptr);

        ProxyType::StatisticalProfilerType& profiler = proxy->GetProfiler(ProfilerProxyGroup);

        const AZStd::string statNamePerformance("PerformanceResult");
        const auto statIdPerformance = ConvertNameToStatId<ProxyType::StatIdType>(statNamePerformance);

        const AZStd::string statNameBlock("Block");
        const auto statIdBlock = ConvertNameToStatId<ProxyType::StatIdType>(statNameBlock);

        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdPerformance, statNamePerformance, "us") != nullptr);
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdBlock, statNameBlock, "us") != nullptr);

        proxy->ActivateProfiler(ProfilerProxyGroup, true);

        RecordStatistics(SmallIterationCount, statIdPerformance, statIdBlock);

        ASSERT_TRUE(profiler.GetStatistic(statIdPerformance) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdPerformance)->GetNumSamples(), 1);

        ASSERT_TRUE(profiler.GetStatistic(statIdBlock) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdBlock)->GetNumSamples(), SmallIterationCount);
    }

    TEST_F(StatisticalProfilerProxyFixture, ProfileCode_4Threads_ValidateStatistics)
    {
        ProxyType profilerProxy;

        ProxyType* proxy = AZ::Interface<ProxyType>::Get();
        ASSERT_TRUE(proxy != nullptr);

        ProxyType::StatisticalProfilerType& profiler = proxy->GetProfiler(ProfilerProxyGroup);

        const AZStd::string statNameThread1("thread1");
        const auto statIdThread1 = ConvertNameToStatId<ProxyType::StatIdType>(statNameThread1);
        const AZStd::string statNameThread1Loop("thread1_loop");
        const auto statIdThread1Loop = ConvertNameToStatId<ProxyType::StatIdType>(statNameThread1Loop);

        const AZStd::string statNameThread2("thread2");
        const auto statIdThread2 = ConvertNameToStatId<ProxyType::StatIdType>(statNameThread2);
        const AZStd::string statNameThread2Loop("thread2_loop");
        const auto statIdThread2Loop = ConvertNameToStatId<ProxyType::StatIdType>(statNameThread2Loop);

        const AZStd::string statNameThread3("thread3");
        const auto statIdThread3 = ConvertNameToStatId<ProxyType::StatIdType>(statNameThread3);
        const AZStd::string statNameThread3Loop("thread3_loop");
        const auto statIdThread3Loop = ConvertNameToStatId<ProxyType::StatIdType>(statNameThread3Loop);

        const AZStd::string statNameThread4("thread4");
        const auto statIdThread4 = ConvertNameToStatId<ProxyType::StatIdType>(statNameThread4);
        const AZStd::string statNameThread4Loop("thread4_loop");
        const auto statIdThread4Loop = ConvertNameToStatId<ProxyType::StatIdType>(statNameThread4Loop);

        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread1, statNameThread1, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread1Loop, statNameThread1Loop, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread2, statNameThread2, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread2Loop, statNameThread2Loop, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread3, statNameThread3, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread3Loop, statNameThread3Loop, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread4, statNameThread4, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread4Loop, statNameThread4Loop, "us"));

        proxy->ActivateProfiler(ProfilerProxyGroup, true);

        AZStd::thread t1([&](){
            RecordStatistics(MediumIterationCount, statIdThread1, statIdThread1Loop);
        });
        AZStd::thread t2([&](){
            RecordStatistics(MediumIterationCount, statIdThread2, statIdThread2Loop);
        });
        AZStd::thread t3([&](){
            RecordStatistics(MediumIterationCount, statIdThread3, statIdThread3Loop);
        });
        AZStd::thread t4([&](){
            RecordStatistics(MediumIterationCount, statIdThread4, statIdThread4Loop);
        });
        t1.join();
        t2.join();
        t3.join();
        t4.join();

        ASSERT_TRUE(profiler.GetStatistic(statIdThread1) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread1)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread1Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread1Loop)->GetNumSamples(), MediumIterationCount);

        ASSERT_TRUE(profiler.GetStatistic(statIdThread2) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread2)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread2Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread2Loop)->GetNumSamples(), MediumIterationCount);

        ASSERT_TRUE(profiler.GetStatistic(statIdThread3) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread3)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread3Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread3Loop)->GetNumSamples(), MediumIterationCount);

        ASSERT_TRUE(profiler.GetStatistic(statIdThread4) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread4)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread4Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread4Loop)->GetNumSamples(), MediumIterationCount);

        proxy->ActivateProfiler(ProfilerProxyGroup, false);
    }
}//namespace UnitTest

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

    public:
        void RunBenchmark(benchmark::State& state)
        {
            if (state.thread_index == 0)
            {
                m_profiler = new ProfilerType;

                for (int i = 0; i < state.threads; ++i)
                {
                    const AZStd::string threadStatName = AZStd::string::format("thread%03d", i);
                    const auto threadStatId = UnitTest::ConvertNameToStatId<StatIdType>(threadStatName);

                    m_profiler->GetStatsManager().AddStatistic(threadStatId, threadStatName, "us");
                }
            }

            const AZStd::string statName = AZStd::string::format("thread%03d", state.thread_index);
            const auto statId = UnitTest::ConvertNameToStatId<StatIdType>(statName);

            for (auto _ : state)
            {
                typename ProfilerType::TimedScope timedScope(*m_profiler, statId);
                benchmark::DoNotOptimize(m_profiler);
            }

            if (state.thread_index == 0)
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

    BENCHMARK_TEMPLATE_DEFINE_F(StatisticalProfilerBenchmark, Hash_SingleThreadedPerf, UnitTest::StringHash)(benchmark::State& state)
    {
        RunBenchmark(state);
    }

    BENCHMARK_TEMPLATE_DEFINE_F(StatisticalProfilerBenchmark, Crc32_SingleThreadedPerf, AZ::Crc32)(benchmark::State& state)
    {
        RunBenchmark(state);
    }

    BENCHMARK_TEMPLATE_DEFINE_F(StatisticalProfilerBenchmark, HashValue32_SingleThreadedPerf, AZ::HashValue32)(benchmark::State& state)
    {
        RunBenchmark(state);
    }

    BENCHMARK_TEMPLATE_DEFINE_F(StatisticalProfilerBenchmark, HashValue64_SingleThreadedPerf, AZ::HashValue64)(benchmark::State& state)
    {
        RunBenchmark(state);
    }

    REGISTER_STATS_PROFILER_SINGLETHREADED_BENCHMARK(StatisticalProfilerBenchmark, String_SingleThreadedPerf)
    REGISTER_STATS_PROFILER_SINGLETHREADED_BENCHMARK(StatisticalProfilerBenchmark, Hash_SingleThreadedPerf)
    REGISTER_STATS_PROFILER_SINGLETHREADED_BENCHMARK(StatisticalProfilerBenchmark, Crc32_SingleThreadedPerf)
    REGISTER_STATS_PROFILER_SINGLETHREADED_BENCHMARK(StatisticalProfilerBenchmark, HashValue32_SingleThreadedPerf)
    REGISTER_STATS_PROFILER_SINGLETHREADED_BENCHMARK(StatisticalProfilerBenchmark, HashValue64_SingleThreadedPerf)

    BENCHMARK_TEMPLATE_DEFINE_F(StatisticalProfilerBenchmark, String_Mutex_ThreadedPerf, AZStd::string, AZStd::mutex)(benchmark::State& state)
    {
        RunBenchmark(state);
    }

    BENCHMARK_TEMPLATE_DEFINE_F(StatisticalProfilerBenchmark, Hash_Mutex_ThreadedPerf, UnitTest::StringHash, AZStd::mutex)(benchmark::State& state)
    {
        RunBenchmark(state);
    }

    BENCHMARK_TEMPLATE_DEFINE_F(StatisticalProfilerBenchmark, Crc32_Mutex_ThreadedPerf, AZ::Crc32, AZStd::mutex)(benchmark::State& state)
    {
        RunBenchmark(state);
    }

    BENCHMARK_TEMPLATE_DEFINE_F(StatisticalProfilerBenchmark, HashValue32_Mutex_ThreadedPerf, AZ::HashValue32, AZStd::mutex)(benchmark::State& state)
    {
        RunBenchmark(state);
    }

    BENCHMARK_TEMPLATE_DEFINE_F(StatisticalProfilerBenchmark, HashValue64_Mutex_ThreadedPerf, AZ::HashValue64, AZStd::mutex)(benchmark::State& state)
    {
        RunBenchmark(state);
    }

    REGISTER_STATS_PROFILER_MULTITHREADED_BENCHMARK(StatisticalProfilerBenchmark, String_Mutex_ThreadedPerf)
    REGISTER_STATS_PROFILER_MULTITHREADED_BENCHMARK(StatisticalProfilerBenchmark, Hash_Mutex_ThreadedPerf)
    REGISTER_STATS_PROFILER_MULTITHREADED_BENCHMARK(StatisticalProfilerBenchmark, Crc32_Mutex_ThreadedPerf)
    REGISTER_STATS_PROFILER_MULTITHREADED_BENCHMARK(StatisticalProfilerBenchmark, HashValue32_Mutex_ThreadedPerf)
    REGISTER_STATS_PROFILER_MULTITHREADED_BENCHMARK(StatisticalProfilerBenchmark, HashValue64_Mutex_ThreadedPerf)

    BENCHMARK_TEMPLATE_DEFINE_F(StatisticalProfilerBenchmark, String_SpinMutex_ThreadedPerf, AZStd::string, AZStd::spin_mutex)(benchmark::State& state)
    {
        RunBenchmark(state);
    }

    BENCHMARK_TEMPLATE_DEFINE_F(StatisticalProfilerBenchmark, Hash_SpinMutex_ThreadedPerf, UnitTest::StringHash, AZStd::spin_mutex)(benchmark::State& state)
    {
        RunBenchmark(state);
    }

    BENCHMARK_TEMPLATE_DEFINE_F(StatisticalProfilerBenchmark, Crc32_SpinMutex_ThreadedPerf, AZ::Crc32, AZStd::spin_mutex)(benchmark::State& state)
    {
        RunBenchmark(state);
    }

    BENCHMARK_TEMPLATE_DEFINE_F(StatisticalProfilerBenchmark, HashValue32_SpinMutex_ThreadedPerf, AZ::HashValue32, AZStd::spin_mutex)(benchmark::State& state)
    {
        RunBenchmark(state);
    }

    BENCHMARK_TEMPLATE_DEFINE_F(StatisticalProfilerBenchmark, HashValue64_SpinMutex_ThreadedPerf, AZ::HashValue64, AZStd::spin_mutex)(benchmark::State& state)
    {
        RunBenchmark(state);
    }

    REGISTER_STATS_PROFILER_MULTITHREADED_BENCHMARK(StatisticalProfilerBenchmark, String_SpinMutex_ThreadedPerf)
    REGISTER_STATS_PROFILER_MULTITHREADED_BENCHMARK(StatisticalProfilerBenchmark, Hash_SpinMutex_ThreadedPerf)
    REGISTER_STATS_PROFILER_MULTITHREADED_BENCHMARK(StatisticalProfilerBenchmark, Crc32_SpinMutex_ThreadedPerf)
    REGISTER_STATS_PROFILER_MULTITHREADED_BENCHMARK(StatisticalProfilerBenchmark, HashValue32_SpinMutex_ThreadedPerf)
    REGISTER_STATS_PROFILER_MULTITHREADED_BENCHMARK(StatisticalProfilerBenchmark, HashValue64_SpinMutex_ThreadedPerf)

    BENCHMARK_TEMPLATE_DEFINE_F(StatisticalProfilerBenchmark, String_SharedMutex_ThreadedPerf, AZStd::string, AZStd::shared_mutex)(benchmark::State& state)
    {
        RunBenchmark(state);
    }

    BENCHMARK_TEMPLATE_DEFINE_F(StatisticalProfilerBenchmark, Hash_SharedMutex_ThreadedPerf, UnitTest::StringHash, AZStd::shared_mutex)(benchmark::State& state)
    {
        RunBenchmark(state);
    }

    BENCHMARK_TEMPLATE_DEFINE_F(StatisticalProfilerBenchmark, Crc32_SharedMutex_ThreadedPerf, AZ::Crc32, AZStd::shared_mutex)(benchmark::State& state)
    {
        RunBenchmark(state);
    }

    BENCHMARK_TEMPLATE_DEFINE_F(StatisticalProfilerBenchmark, HashValue32_SharedMutex_ThreadedPerf, AZ::HashValue32, AZStd::shared_mutex)(benchmark::State& state)
    {
        RunBenchmark(state);
    }

    BENCHMARK_TEMPLATE_DEFINE_F(StatisticalProfilerBenchmark, HashValue64_SharedMutex_ThreadedPerf, AZ::HashValue64, AZStd::shared_mutex)(benchmark::State& state)
    {
        RunBenchmark(state);
    }

    REGISTER_STATS_PROFILER_MULTITHREADED_BENCHMARK(StatisticalProfilerBenchmark, String_SharedMutex_ThreadedPerf)
    REGISTER_STATS_PROFILER_MULTITHREADED_BENCHMARK(StatisticalProfilerBenchmark, Hash_SharedMutex_ThreadedPerf)
    REGISTER_STATS_PROFILER_MULTITHREADED_BENCHMARK(StatisticalProfilerBenchmark, Crc32_SharedMutex_ThreadedPerf)
    REGISTER_STATS_PROFILER_MULTITHREADED_BENCHMARK(StatisticalProfilerBenchmark, HashValue32_SharedMutex_ThreadedPerf)
    REGISTER_STATS_PROFILER_MULTITHREADED_BENCHMARK(StatisticalProfilerBenchmark, HashValue64_SharedMutex_ThreadedPerf)

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
            if (state.thread_index == 0)
            {
                ProxyType::TimedScope::ClearCachedProxy();

                m_proxy = new ProxyType;
                ProfilerType& profiler = m_proxy->GetProfiler(UnitTest::ProfilerProxyGroup);

                for (int i = 0; i < state.threads; ++i)
                {
                    const AZStd::string threadStatName = AZStd::string::format("thread%03d", i);
                    const auto threadStatId = UnitTest::ConvertNameToStatId<StatIdType>(threadStatName);

                    profiler.GetStatsManager().AddStatistic(threadStatId, threadStatName, "us");
                }

                m_proxy->ActivateProfiler(UnitTest::ProfilerProxyGroup, true);
            }

            const AZStd::string statName = AZStd::string::format("thread%03d", state.thread_index);
            const auto statId = UnitTest::ConvertNameToStatId<StatIdType>(statName);

            for (auto _ : state)
            {
                typename TimedScopeType timedScope(UnitTest::ProfilerProxyGroup, statId);
                benchmark::DoNotOptimize(m_proxy);
            }

            if (state.thread_index == 0)
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
