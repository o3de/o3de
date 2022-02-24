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

    class StatisticalProfilerTest
        : public AllocatorsFixture
    {
    }; //class StatisticalProfilerTest

    TEST_F(StatisticalProfilerTest, StatisticalProfilerStringNoMutex_ProfileCode_ValidateStatistics)
    {
        using ProfilerType = AZ::Statistics::StatisticalProfiler<>;

        ProfilerType profiler;

        const AZStd::string statNamePerformance("PerformanceResult");
        const AZStd::string statNameBlock("Block");

        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statNamePerformance, statNamePerformance, "us") != nullptr);
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statNameBlock, statNameBlock, "us") != nullptr);

        RecordStatistics(profiler, SmallIterationCount, statNamePerformance, statNameBlock);

        ASSERT_TRUE(profiler.GetStatistic(statNamePerformance) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statNamePerformance)->GetNumSamples(), 1);

        ASSERT_TRUE(profiler.GetStatistic(statNameBlock) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statNameBlock)->GetNumSamples(), SmallIterationCount);
    }

    TEST_F(StatisticalProfilerTest, StatisticalProfilerCrc32NoMutex_ProfileCode_ValidateStatistics)
    {
        using ProfilerType = AZ::Statistics::StatisticalProfiler<AZ::Crc32>;

        ProfilerType profiler;

        constexpr AZ::Crc32 statIdPerformance = AZ_CRC_CE("PerformanceResult");
        const AZStd::string statNamePerformance("PerformanceResult");

        constexpr AZ::Crc32 statIdBlock = AZ_CRC_CE("Block");
        const AZStd::string statNameBlock("Block");

        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdPerformance, statNamePerformance, "us") != nullptr);
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdBlock, statNameBlock, "us") != nullptr);

        RecordStatistics(profiler, SmallIterationCount, statIdPerformance, statIdBlock);

        ASSERT_TRUE(profiler.GetStatistic(statIdPerformance) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdPerformance)->GetNumSamples(), 1);

        ASSERT_TRUE(profiler.GetStatistic(statIdBlock) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdBlock)->GetNumSamples(), SmallIterationCount);

        ASSERT_TRUE(profiler.GetStatistic(statIdPerformance) != nullptr);
    }

    TEST_F(StatisticalProfilerTest, StatisticalProfilerStringWithSharedSpinMutex__ProfileCode_ValidateStatistics)
    {
        using ProfilerType = AZ::Statistics::StatisticalProfiler<AZStd::string, AZStd::shared_spin_mutex>;

        ProfilerType profiler;

        const AZStd::string statNamePerformance("PerformanceResult");
        const AZStd::string statNameBlock("Block");

        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statNamePerformance, statNamePerformance, "us") != nullptr);
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statNameBlock, statNameBlock, "us") != nullptr);

        RecordStatistics(profiler, SmallIterationCount, statNamePerformance, statNameBlock);

        ASSERT_TRUE(profiler.GetStatistic(statNamePerformance) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statNamePerformance)->GetNumSamples(), 1);

        ASSERT_TRUE(profiler.GetStatistic(statNameBlock) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statNameBlock)->GetNumSamples(), SmallIterationCount);

        ASSERT_TRUE(profiler.GetStatistic(statNamePerformance) != nullptr);
    }

    TEST_F(StatisticalProfilerTest, StatisticalProfilerCrc32WithSharedSpinMutex__ProfileCode_ValidateStatistics)
    {
        using ProfilerType = AZ::Statistics::StatisticalProfiler<AZ::Crc32, AZStd::shared_spin_mutex>;

        ProfilerType profiler;

        constexpr AZ::Crc32 statIdPerformance = AZ_CRC_CE("PerformanceResult");
        const AZStd::string statNamePerformance("PerformanceResult");

        constexpr AZ::Crc32 statIdBlock = AZ_CRC_CE("Block");
        const AZStd::string statNameBlock("Block");

        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdPerformance, statNamePerformance, "us") != nullptr);
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdBlock, statNameBlock, "us") != nullptr);

        RecordStatistics(profiler, SmallIterationCount, statIdPerformance, statIdBlock);

        ASSERT_TRUE(profiler.GetStatistic(statIdPerformance) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdPerformance)->GetNumSamples(), 1);

        ASSERT_TRUE(profiler.GetStatistic(statIdBlock) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdBlock)->GetNumSamples(), SmallIterationCount);

        ASSERT_TRUE(profiler.GetStatistic(statIdPerformance) != nullptr);
    }

    TEST_F(StatisticalProfilerTest, StatisticalProfilerStringWithSharedSpinMutex_RunProfiledThreads_ValidateStatistics)
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
            RecordStatistics(profiler, SmallIterationCount, statIdThread1, statIdThread1Loop);
        });
        AZStd::thread t2([&](){
            RecordStatistics(profiler, SmallIterationCount, statIdThread2, statIdThread2Loop);
        });
        AZStd::thread t3([&](){
            RecordStatistics(profiler, SmallIterationCount, statIdThread3, statIdThread3Loop);
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

    }

    TEST_F(StatisticalProfilerTest, StatisticalProfilerProxy_ProfileCode_ValidateStatistics)
    {
        using TimedScopeType = AZ::Statistics::StatisticalProfilerProxy::TimedScope;

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

    /** Trace message handler to track messages during tests
*/
    struct MyTraceMessageSink final
        : public AZ::Debug::TraceMessageBus::Handler
    {
        MyTraceMessageSink()
        {
            AZ::Debug::TraceMessageBus::Handler::BusConnect();
        }

        ~MyTraceMessageSink()
        {
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        }

        //////////////////////////////////////////////////////////////////////////
        // TraceMessageBus
        bool OnPrintf(const char* window, const char* message) override
        {
            return OnOutput(window, message);
        }

        bool OnOutput(const char* window, const char* message) override
        {
            printf("%s: %s\n", window, message);
            return false;
        }
    }; //struct MyTraceMessageSink

    class Suite_StatisticalProfilerPerformance
        : public AllocatorsFixture
    {
    public:
        MyTraceMessageSink* m_testSink;

        Suite_StatisticalProfilerPerformance() :m_testSink(nullptr)
        {
        }

        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            m_testSink = new MyTraceMessageSink();
        }

        ~Suite_StatisticalProfilerPerformance()
        {
        }

        void TearDown() override
        {
            // clearing up memory
            delete m_testSink;
            AllocatorsFixture::TearDown();
        }

    }; //class Suite_StatisticalProfilerPerformance

    TEST_F(Suite_StatisticalProfilerPerformance, StatisticalProfilerStringNoMutex_1ThreadPerformance)
    {
        using ProfilerType = AZ::Statistics::StatisticalProfiler<>;

        ProfilerType profiler;

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
        using ProfilerType = AZ::Statistics::StatisticalProfiler<AZ::Crc32>;

        ProfilerType profiler;

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
        using ProfilerType = AZ::Statistics::StatisticalProfiler<AZStd::string, AZStd::shared_spin_mutex>;

        ProfilerType profiler;

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
        using ProfilerType = AZ::Statistics::StatisticalProfiler<AZ::Crc32, AZStd::shared_spin_mutex>;

        ProfilerType profiler;

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
        using TimedScopeType = AZ::Statistics::StatisticalProfilerProxy::TimedScope;

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
