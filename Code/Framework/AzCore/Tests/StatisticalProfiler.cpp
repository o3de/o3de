/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzTest/AzTest.h>

#include <AzCore/Math/Crc.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/shared_spin_mutex.h>
#include <AzCore/Debug/TraceMessageBus.h>

#include <AzCore/Statistics/StatisticalProfilerProxy.h>

namespace UnitTest
{
    constexpr int SmallIterationCount = 10;
    constexpr int MediumIterationCount = 100;
    constexpr int LargeIterationCount = 1000000;

    constexpr AZ::u32 ProfilerProxyGroup = AZ_CRC_CE("StatisticalProfilerProxyTests");

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
        StatisticalProfilerTestTraits<AZ::Crc32>,

        StatisticalProfilerTestTraits<AZStd::string, AZStd::shared_spin_mutex>,
        StatisticalProfilerTestTraits<AZ::Crc32, AZStd::shared_spin_mutex>
    >;
    TYPED_TEST_CASE(StatisticalProfilerFixture, StatisticalProfilerTestTypes);

    TYPED_TEST(StatisticalProfilerFixture, ProfileCode_SingleThread_ValidateStatistics)
    {
        using StatIdType = typename TypeParam::StatIdType;

        typename TypeParam::ProfilerType profiler;

        const AZStd::string statNamePerformance("PerformanceResult");
        const StatIdType statIdPerformance(statNamePerformance);

        const AZStd::string statNameBlock("Block");
        const StatIdType statIdBlock(statNameBlock);

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
        StatisticalProfilerTestTraits<AZStd::string, AZStd::shared_spin_mutex>,
        StatisticalProfilerTestTraits<AZ::Crc32, AZStd::shared_spin_mutex>
    >;
    TYPED_TEST_CASE(ThreadedStatisticalProfilerFixture, ThreadedStatisticalProfilerTestTypes);

    TYPED_TEST(ThreadedStatisticalProfilerFixture, ProfileCode_4Threads_ValidateStatistics)
    {
        using StatIdType = typename TypeParam::StatIdType;

        typename TypeParam::ProfilerType profiler;

        const AZStd::string statNameThread1("thread1");
        const StatIdType statIdThread1(statNameThread1);
        const AZStd::string statNameThread1Loop("thread1_loop");
        const StatIdType statIdThread1Loop(statNameThread1Loop);

        const AZStd::string statNameThread2("thread2");
        const StatIdType statIdThread2(statNameThread2);
        const AZStd::string statNameThread2Loop("thread2_loop");
        const StatIdType statIdThread2Loop(statNameThread2Loop);

        const AZStd::string statNameThread3("thread3");
        const StatIdType statIdThread3(statNameThread3);
        const AZStd::string statNameThread3Loop("thread3_loop");
        const StatIdType statIdThread3Loop(statNameThread3Loop);

        const AZStd::string statNameThread4("thread4");
        const StatIdType statIdThread4(statNameThread4);
        const AZStd::string statNameThread4Loop("thread4_loop");
        const StatIdType statIdThread4Loop(statNameThread4Loop);

        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread1, statNameThread1, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread1Loop, statNameThread1Loop, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread2, statNameThread2, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread2Loop, statNameThread2Loop, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread3, statNameThread3, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread3Loop, statNameThread3Loop, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread4, statNameThread4, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread4Loop, statNameThread4Loop, "us"));

        //Let's kickoff the threads to see how much contention affects the profiler's performance.
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

    class StatisticalProfilerTest
        : public AllocatorsWithTraceFixture
    {
    };

    TEST_F(StatisticalProfilerTest, StatisticalProfilerProxy_ProfileCode_ValidateStatistics)
    {
        AZ::Statistics::StatisticalProfilerProxy::TimedScope::ClearCachedProxy();
        AZ::Statistics::StatisticalProfilerProxy profilerProxy;
        AZ::Statistics::StatisticalProfilerProxy* proxy = AZ::Interface<AZ::Statistics::StatisticalProfilerProxy>::Get();
        AZ::Statistics::StatisticalProfilerProxy::StatisticalProfilerType& profiler = proxy->GetProfiler(ProfilerProxyGroup);

        const AZ::Statistics::StatisticalProfilerProxy::StatIdType statIdPerformance("PerformanceResult");
        const AZStd::string statNamePerformance("PerformanceResult");

        const AZ::Statistics::StatisticalProfilerProxy::StatIdType statIdBlock("Block");
        const AZStd::string statNameBlock("Block");

        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdPerformance, statNamePerformance, "us") != nullptr);
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdBlock, statNameBlock, "us") != nullptr);

        proxy->ActivateProfiler(ProfilerProxyGroup, true);

        RecordStatistics(SmallIterationCount, statIdPerformance, statIdBlock);

        ASSERT_TRUE(profiler.GetStatistic(statIdPerformance) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdPerformance)->GetNumSamples(), 1);

        ASSERT_TRUE(profiler.GetStatistic(statIdBlock) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdBlock)->GetNumSamples(), SmallIterationCount);

        //Clean Up
        proxy->ActivateProfiler(ProfilerProxyGroup, false);
    }

    TEST_F(StatisticalProfilerTest, StatisticalProfilerProxy3_RunProfiledThreads_ValidateStatistics)
    {
        AZ::Statistics::StatisticalProfilerProxy::TimedScope::ClearCachedProxy();
        AZ::Statistics::StatisticalProfilerProxy profilerProxy;
        AZ::Statistics::StatisticalProfilerProxy* proxy = AZ::Interface<AZ::Statistics::StatisticalProfilerProxy>::Get();
        AZ::Statistics::StatisticalProfilerProxy::StatisticalProfilerType& profiler = proxy->GetProfiler(ProfilerProxyGroup);

        const AZ::Statistics::StatisticalProfilerProxy::StatIdType statIdThread1("simple_thread1");
        const AZStd::string statNameThread1("simple_thread1");
        const AZ::Statistics::StatisticalProfilerProxy::StatIdType statIdThread1Loop("simple_thread1_loop");
        const AZStd::string statNameThread1Loop("simple_thread1_loop");

        const AZ::Statistics::StatisticalProfilerProxy::StatIdType statIdThread2("simple_thread2");
        const AZStd::string statNameThread2("simple_thread2");
        const AZ::Statistics::StatisticalProfilerProxy::StatIdType statIdThread2Loop("simple_thread2_loop");
        const AZStd::string statNameThread2Loop("simple_thread2_loop");

        const AZ::Statistics::StatisticalProfilerProxy::StatIdType statIdThread3("simple_thread3");
        const AZStd::string statNameThread3("simple_thread3");
        const AZ::Statistics::StatisticalProfilerProxy::StatIdType statIdThread3Loop("simple_thread3_loop");
        const AZStd::string statNameThread3Loop("simple_thread3_loop");

        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread1, statNameThread1, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread1Loop, statNameThread1Loop, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread2, statNameThread2, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread2Loop, statNameThread2Loop, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread3, statNameThread3, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread3Loop, statNameThread3Loop, "us"));

        proxy->ActivateProfiler(ProfilerProxyGroup, true);

        //Let's kickoff the threads to see how much contention affects the profiler's performance.
        AZStd::thread t1([&](){
            RecordStatistics(SmallIterationCount, statIdThread1, statIdThread1Loop);
        });
        AZStd::thread t2([&](){
            RecordStatistics(SmallIterationCount, statIdThread2, statIdThread2Loop);
        });
        AZStd::thread t3([&](){
            RecordStatistics(SmallIterationCount, statIdThread3, statIdThread3Loop);
        });
        t1.join();
        t2.join();
        t3.join();

        ASSERT_TRUE(profiler.GetStatistic(statIdThread1) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread1)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread1Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread1Loop)->GetNumSamples(), SmallIterationCount);

        ASSERT_TRUE(profiler.GetStatistic(statIdThread2) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread2)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread2Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread2Loop)->GetNumSamples(), SmallIterationCount);

        ASSERT_TRUE(profiler.GetStatistic(statIdThread3) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread3)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread3Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread3Loop)->GetNumSamples(), SmallIterationCount);

        //Clean Up
        proxy->ActivateProfiler(ProfilerProxyGroup, false);
    }

    class Suite_StatisticalProfilerPerformance
        : public AllocatorsWithTraceFixture
    {
    };

    TEST_F(Suite_StatisticalProfilerPerformance, StatisticalProfilerStringNoMutex_1ThreadPerformance)
    {
        AZ::Statistics::StatisticalProfiler<> profiler;

        const AZStd::string statNamePerformance("PerformanceResult");
        const AZStd::string statNameBlock("Block");

        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statNamePerformance, statNamePerformance, "us") != nullptr);
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statNameBlock, statNameBlock, "us") != nullptr);

        RecordStatistics(profiler, LargeIterationCount, statNamePerformance, statNameBlock);

        ASSERT_TRUE(profiler.GetStatistic(statNamePerformance) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statNamePerformance)->GetNumSamples(), 1);

        ASSERT_TRUE(profiler.GetStatistic(statNameBlock) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statNameBlock)->GetNumSamples(), LargeIterationCount);

        profiler.LogAndResetStats("StatisticalProfilerStringNoMutex");

        ASSERT_TRUE(profiler.GetStatistic(statNamePerformance) != nullptr);
    }

    TEST_F(Suite_StatisticalProfilerPerformance, StatisticalProfilerCrc32NoMutex_1ThreadPerformance)
    {
        AZ::Statistics::StatisticalProfiler<AZ::Crc32> profiler;

        constexpr AZ::Crc32 statIdPerformance = AZ_CRC_CE("PerformanceResult");
        const AZStd::string statNamePerformance("PerformanceResult");

        constexpr AZ::Crc32 statIdBlock = AZ_CRC_CE("Block");
        const AZStd::string statNameBlock("Block");

        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdPerformance, statNamePerformance, "us") != nullptr);
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdBlock, statNameBlock, "us") != nullptr);

        RecordStatistics(profiler, LargeIterationCount, statIdPerformance, statIdBlock);

        ASSERT_TRUE(profiler.GetStatistic(statIdPerformance) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdPerformance)->GetNumSamples(), 1);

        ASSERT_TRUE(profiler.GetStatistic(statIdBlock) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdBlock)->GetNumSamples(), LargeIterationCount);

        profiler.LogAndResetStats("StatisticalProfilerCrc32NoMutex");

        ASSERT_TRUE(profiler.GetStatistic(statIdPerformance) != nullptr);
    }

    TEST_F(Suite_StatisticalProfilerPerformance, StatisticalProfilerStringWithSharedSpinMutex_1ThreadPerformance)
    {
        AZ::Statistics::StatisticalProfiler<AZStd::string, AZStd::shared_spin_mutex> profiler;

        const AZStd::string statNamePerformance("PerformanceResult");
        const AZStd::string statNameBlock("Block");

        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statNamePerformance, statNamePerformance, "us") != nullptr);
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statNameBlock, statNameBlock, "us") != nullptr);

        RecordStatistics(profiler, LargeIterationCount, statNamePerformance, statNameBlock);

        ASSERT_TRUE(profiler.GetStatistic(statNamePerformance) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statNamePerformance)->GetNumSamples(), 1);

        ASSERT_TRUE(profiler.GetStatistic(statNameBlock) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statNameBlock)->GetNumSamples(), LargeIterationCount);

        profiler.LogAndResetStats("StatisticalProfilerStringWithSharedSpinMutex");

        ASSERT_TRUE(profiler.GetStatistic(statNamePerformance) != nullptr);
    }

    TEST_F(Suite_StatisticalProfilerPerformance, StatisticalProfilerCrc32WithSharedSpinMutex_1ThreadPerformance)
    {
        AZ::Statistics::StatisticalProfiler<AZ::Crc32, AZStd::shared_spin_mutex> profiler;

        constexpr AZ::Crc32 statIdPerformance = AZ_CRC_CE("PerformanceResult");
        const AZStd::string statNamePerformance("PerformanceResult");

        constexpr AZ::Crc32 statIdBlock = AZ_CRC_CE("Block");
        const AZStd::string statNameBlock("Block");

        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdPerformance, statNamePerformance, "us") != nullptr);
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdBlock, statNameBlock, "us") != nullptr);

        RecordStatistics(profiler, LargeIterationCount, statIdPerformance, statIdBlock);

        ASSERT_TRUE(profiler.GetStatistic(statIdPerformance) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdPerformance)->GetNumSamples(), 1);

        ASSERT_TRUE(profiler.GetStatistic(statIdBlock) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdBlock)->GetNumSamples(), LargeIterationCount);

        profiler.LogAndResetStats("StatisticalProfilerCrc32WithSharedSpinMutex");

        ASSERT_TRUE(profiler.GetStatistic(statIdPerformance) != nullptr);
    }

    TEST_F(Suite_StatisticalProfilerPerformance, StatisticalProfilerStringWithSharedSpinMutex3Threads_3ThreadsPerformance)
    {
        AZ::Statistics::StatisticalProfiler<AZStd::string, AZStd::shared_spin_mutex> profiler;

        const AZStd::string statIdThread1 = "simple_thread1";
        const AZStd::string statNameThread1("simple_thread1");
        const AZStd::string statIdThread1Loop = "simple_thread1_loop";
        const AZStd::string statNameThread1Loop("simple_thread1_loop");

        const AZStd::string statIdThread2 = "simple_thread2";
        const AZStd::string statNameThread2("simple_thread2");
        const AZStd::string statIdThread2Loop = "simple_thread2_loop";
        const AZStd::string statNameThread2Loop("simple_thread2_loop");

        const AZStd::string statIdThread3 = "simple_thread3";
        const AZStd::string statNameThread3("simple_thread3");
        const AZStd::string statIdThread3Loop = "simple_thread3_loop";
        const AZStd::string statNameThread3Loop("simple_thread3_loop");

        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread1, statNameThread1, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread1Loop, statNameThread1Loop, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread2, statNameThread2, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread2Loop, statNameThread2Loop, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread3, statNameThread3, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread3Loop, statNameThread3Loop, "us"));

        //Let's kickoff the threads to see how much contention affects the profiler's performance.
        AZStd::thread t1([&](){
            RecordStatistics(profiler, LargeIterationCount, statIdThread1, statIdThread1Loop);
        });
        AZStd::thread t2([&](){
            RecordStatistics(profiler, LargeIterationCount, statIdThread2, statIdThread2Loop);
        });
        AZStd::thread t3([&](){
            RecordStatistics(profiler, LargeIterationCount, statIdThread3, statIdThread3Loop);
        });
        t1.join();
        t2.join();
        t3.join();

        ASSERT_TRUE(profiler.GetStatistic(statIdThread1) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread1)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread1Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread1Loop)->GetNumSamples(), LargeIterationCount);

        ASSERT_TRUE(profiler.GetStatistic(statIdThread2) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread2)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread2Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread2Loop)->GetNumSamples(), LargeIterationCount);

        ASSERT_TRUE(profiler.GetStatistic(statIdThread3) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread3)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread3Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread3Loop)->GetNumSamples(), LargeIterationCount);

        profiler.LogAndResetStats("3_Threads_StatisticalProfiler");

        ASSERT_TRUE(profiler.GetStatistic(statIdThread1) != nullptr);
    }

    TEST_F(Suite_StatisticalProfilerPerformance, StatisticalProfilerProxy_1ThreadPerformance)
    {
        AZ::Statistics::StatisticalProfilerProxy::TimedScope::ClearCachedProxy();
        AZ::Statistics::StatisticalProfilerProxy profilerProxy;
        AZ::Statistics::StatisticalProfilerProxy* proxy = AZ::Interface<AZ::Statistics::StatisticalProfilerProxy>::Get();
        AZ::Statistics::StatisticalProfilerProxy::StatisticalProfilerType& profiler = proxy->GetProfiler(ProfilerProxyGroup);

        const AZ::Statistics::StatisticalProfilerProxy::StatIdType statIdPerformance("PerformanceResult");
        const AZStd::string statNamePerformance("PerformanceResult");

        const AZ::Statistics::StatisticalProfilerProxy::StatIdType statIdBlock("Block");
        const AZStd::string statNameBlock("Block");

        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdPerformance, statNamePerformance, "us") != nullptr);
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdBlock, statNameBlock, "us") != nullptr);

        proxy->ActivateProfiler(ProfilerProxyGroup, true);

        RecordStatistics(LargeIterationCount, statIdPerformance, statIdBlock);

        ASSERT_TRUE(profiler.GetStatistic(statIdPerformance) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdPerformance)->GetNumSamples(), 1);

        ASSERT_TRUE(profiler.GetStatistic(statIdBlock) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdBlock)->GetNumSamples(), LargeIterationCount);

        profiler.LogAndResetStats("StatisticalProfilerProxy");

        //Clean Up
        proxy->ActivateProfiler(ProfilerProxyGroup, false);
    }

    TEST_F(Suite_StatisticalProfilerPerformance, StatisticalProfilerProxy_3ThreadsPerformance)
    {
        AZ::Statistics::StatisticalProfilerProxy::TimedScope::ClearCachedProxy();
        AZ::Statistics::StatisticalProfilerProxy profilerProxy;
        AZ::Statistics::StatisticalProfilerProxy* proxy = AZ::Interface<AZ::Statistics::StatisticalProfilerProxy>::Get();
        AZ::Statistics::StatisticalProfilerProxy::StatisticalProfilerType& profiler = proxy->GetProfiler(ProfilerProxyGroup);

        const AZ::Statistics::StatisticalProfilerProxy::StatIdType statIdThread1("simple_thread1");
        const AZStd::string statNameThread1("simple_thread1");
        const AZ::Statistics::StatisticalProfilerProxy::StatIdType statIdThread1Loop("simple_thread1_loop");
        const AZStd::string statNameThread1Loop("simple_thread1_loop");

        const AZ::Statistics::StatisticalProfilerProxy::StatIdType statIdThread2("simple_thread2");
        const AZStd::string statNameThread2("simple_thread2");
        const AZ::Statistics::StatisticalProfilerProxy::StatIdType statIdThread2Loop("simple_thread2_loop");
        const AZStd::string statNameThread2Loop("simple_thread2_loop");

        const AZ::Statistics::StatisticalProfilerProxy::StatIdType statIdThread3("simple_thread3");
        const AZStd::string statNameThread3("simple_thread3");
        const AZ::Statistics::StatisticalProfilerProxy::StatIdType statIdThread3Loop("simple_thread3_loop");
        const AZStd::string statNameThread3Loop("simple_thread3_loop");

        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread1, statNameThread1, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread1Loop, statNameThread1Loop, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread2, statNameThread2, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread2Loop, statNameThread2Loop, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread3, statNameThread3, "us"));
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdThread3Loop, statNameThread3Loop, "us"));

        proxy->ActivateProfiler(ProfilerProxyGroup, true);

        //Let's kickoff the threads to see how much contention affects the profiler's performance.
        AZStd::thread t1([&](){
            RecordStatistics(LargeIterationCount, statIdThread1, statIdThread1Loop);
        });
        AZStd::thread t2([&](){
            RecordStatistics(LargeIterationCount, statIdThread2, statIdThread2Loop);
        });
        AZStd::thread t3([&](){
            RecordStatistics(LargeIterationCount, statIdThread3, statIdThread3Loop);
        });
        t1.join();
        t2.join();
        t3.join();

        ASSERT_TRUE(profiler.GetStatistic(statIdThread1) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread1)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread1Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread1Loop)->GetNumSamples(), LargeIterationCount);

        ASSERT_TRUE(profiler.GetStatistic(statIdThread2) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread2)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread2Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread2Loop)->GetNumSamples(), LargeIterationCount);

        ASSERT_TRUE(profiler.GetStatistic(statIdThread3) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread3)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread3Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread3Loop)->GetNumSamples(), LargeIterationCount);

        profiler.LogAndResetStats("3_Threads_StatisticalProfilerProxy");

        //Clean Up
        proxy->ActivateProfiler(ProfilerProxyGroup, false);
    }
}//namespace UnitTest
