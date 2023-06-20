/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/TraceMessageBus.h>

#include <Tests/StatisticalProfilerHelpers.h>

namespace UnitTest
{
    template<typename ProfilerType, typename StatIdType>
    void RecordStatistics(ProfilerType& profiler, const int loopCount, const StatIdType& rootId, const StatIdType& loopId)
    {
        using TimedScopeType = typename ProfilerType::TimedScope;

        TimedScopeType rootScope(profiler, rootId);

        for (int i = 0; i < loopCount; ++i)
        {
            TimedScopeType loopScope(profiler, loopId);
        }
    }

    void RecordStatistics(const int loopCount,
        const AZ::Statistics::StatisticalProfilerProxy::StatIdType& rootId,
        const AZ::Statistics::StatisticalProfilerProxy::StatIdType& loopId)
    {
        AZ::Statistics::StatisticalProfilerProxy::TimedScope rootScope(ProfilerProxyGroup, rootId);

        for (int i = 0; i < loopCount; ++i)
        {
            AZ::Statistics::StatisticalProfilerProxy::TimedScope loopScope(ProfilerProxyGroup, loopId);
        }
    }

    class AllocatorsWithTraceFixture
        : public LeakDetectionFixture
        , public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();
            AZ::Debug::TraceMessageBus::Handler::BusConnect();
        }

        void TearDown() override
        {
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
            LeakDetectionFixture::TearDown();
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
        using ProfilerType = typename TypeParam::ProfilerType;
        using StatIdType = typename TypeParam::StatIdType;

        ProfilerType profiler;

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
        using ProfilerType = typename TypeParam::ProfilerType;
        using StatIdType = typename TypeParam::StatIdType;

        ProfilerType profiler;

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
