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
#include <AzCore/Debug/TraceMessagesDrillerBus.h>

#include <AzCore/Statistics/StatisticalProfilerProxy.h>

//REMARK: The macros CODE_PROFILER_PROXY_PUSH_TIME and CODE_PROFILER_PUSH_TIME will be redefined
//several times in this file to accommodate for the different specializations of the StatisticalProfiler<>
//template.
#ifdef CODE_PROFILER_PROXY_PUSH_TIME
#undef CODE_PROFILER_PROXY_PUSH_TIME
#endif

#ifdef CODE_PROFILER_PUSH_TIME
#undef CODE_PROFILER_PUSH_TIME
#endif

namespace UnitTest
{
    constexpr AZ::u32 ProfilerProxyGroup = AZ_CRC_CE("StatisticalProfilerProxyTests");

    class StatisticalProfilerTest
        : public AllocatorsFixture
    {
    public:

        StatisticalProfilerTest()
        {
        }

        void SetUp() override
        {
            AllocatorsFixture::SetUp();
        }

        ~StatisticalProfilerTest()
        {
        }

        void TearDown() override
        {
            AllocatorsFixture::TearDown();
        }

    }; //class StatisticalProfilerTest

    TEST_F(StatisticalProfilerTest, StatisticalProfilerStringNoMutex_ProfileCode_ValidateStatistics)
    {
//Helper macro.
#define CODE_PROFILER_PUSH_TIME(profiler, scopeNameId) \
    AZ::Statistics::StatisticalProfiler<>::TimedScope AZ_JOIN(scope, __LINE__)(profiler, scopeNameId);

        AZ::Statistics::StatisticalProfiler<> profiler;

        const AZStd::string statNamePerformance("PerformanceResult");
        const AZStd::string statNameBlock("Block");

        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statNamePerformance, statNamePerformance, "us") != nullptr);
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statNameBlock, statNameBlock, "us") != nullptr);

        const int iter_count = 10;
        {
            CODE_PROFILER_PUSH_TIME(profiler, statNamePerformance)
                int counter = 0;
            for (int i = 0; i < iter_count; i++)
            {
                CODE_PROFILER_PUSH_TIME(profiler, statNameBlock)
                    counter++;
            }
        }

        ASSERT_TRUE(profiler.GetStatistic(statNamePerformance) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statNamePerformance)->GetNumSamples(), 1);

        ASSERT_TRUE(profiler.GetStatistic(statNameBlock) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statNameBlock)->GetNumSamples(), iter_count);

#undef CODE_PROFILER_PUSH_TIME

    }

    TEST_F(StatisticalProfilerTest, StatisticalProfilerCrc32NoMutex_ProfileCode_ValidateStatistics)
    {
        //Helper macro.
#define CODE_PROFILER_PUSH_TIME(profiler, scopeNameId) \
    AZ::Statistics::StatisticalProfiler<AZ::Crc32>::TimedScope AZ_JOIN(scope, __LINE__)(profiler, scopeNameId);

        AZ::Statistics::StatisticalProfiler<AZ::Crc32> profiler;

        constexpr AZ::Crc32 statIdPerformance = AZ_CRC_CE("PerformanceResult");
        const AZStd::string statNamePerformance("PerformanceResult");

        constexpr AZ::Crc32 statIdBlock = AZ_CRC_CE("Block");
        const AZStd::string statNameBlock("Block");

        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdPerformance, statNamePerformance, "us") != nullptr);
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdBlock, statNameBlock, "us") != nullptr);

        const int iter_count = 10;
        {
            CODE_PROFILER_PUSH_TIME(profiler, statIdPerformance)
                int counter = 0;
            for (int i = 0; i < iter_count; i++)
            {
                CODE_PROFILER_PUSH_TIME(profiler, statIdBlock)
                    counter++;
            }
        }

        ASSERT_TRUE(profiler.GetStatistic(statIdPerformance) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdPerformance)->GetNumSamples(), 1);

        ASSERT_TRUE(profiler.GetStatistic(statIdBlock) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdBlock)->GetNumSamples(), iter_count);

        ASSERT_TRUE(profiler.GetStatistic(statIdPerformance) != nullptr);

#undef CODE_PROFILER_PUSH_TIME

    }

    TEST_F(StatisticalProfilerTest, StatisticalProfilerStringWithSharedSpinMutex__ProfileCode_ValidateStatistics)
    {
        //Helper macro.
#define CODE_PROFILER_PUSH_TIME(profiler, scopeNameId) \
    AZ::Statistics::StatisticalProfiler<AZStd::string, AZStd::shared_spin_mutex>::TimedScope AZ_JOIN(scope, __LINE__)(profiler, scopeNameId);

        AZ::Statistics::StatisticalProfiler<AZStd::string, AZStd::shared_spin_mutex> profiler;

        const AZStd::string statNamePerformance("PerformanceResult");
        const AZStd::string statNameBlock("Block");

        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statNamePerformance, statNamePerformance, "us") != nullptr);
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statNameBlock, statNameBlock, "us") != nullptr);

        const int iter_count = 10;
        {
            CODE_PROFILER_PUSH_TIME(profiler, statNamePerformance)
                int counter = 0;
            for (int i = 0; i < iter_count; i++)
            {
                CODE_PROFILER_PUSH_TIME(profiler, statNameBlock)
                    counter++;
            }
        }

        ASSERT_TRUE(profiler.GetStatistic(statNamePerformance) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statNamePerformance)->GetNumSamples(), 1);

        ASSERT_TRUE(profiler.GetStatistic(statNameBlock) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statNameBlock)->GetNumSamples(), iter_count);

        ASSERT_TRUE(profiler.GetStatistic(statNamePerformance) != nullptr);

#undef CODE_PROFILER_PUSH_TIME

    }

    TEST_F(StatisticalProfilerTest, StatisticalProfilerCrc32WithSharedSpinMutex__ProfileCode_ValidateStatistics)
    {
        //Helper macro.
#define CODE_PROFILER_PUSH_TIME(profiler, scopeNameId) \
    AZ::Statistics::StatisticalProfiler<AZ::Crc32, AZStd::shared_spin_mutex>::TimedScope AZ_JOIN(scope, __LINE__)(profiler, scopeNameId);

        AZ::Statistics::StatisticalProfiler<AZ::Crc32, AZStd::shared_spin_mutex> profiler;

        constexpr AZ::Crc32 statIdPerformance = AZ_CRC_CE("PerformanceResult");
        const AZStd::string statNamePerformance("PerformanceResult");

        constexpr AZ::Crc32 statIdBlock = AZ_CRC_CE("Block");
        const AZStd::string statNameBlock("Block");

        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdPerformance, statNamePerformance, "us") != nullptr);
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdBlock, statNameBlock, "us") != nullptr);

        const int iter_count = 10;
        {
            CODE_PROFILER_PUSH_TIME(profiler, statIdPerformance)
                int counter = 0;
            for (int i = 0; i < iter_count; i++)
            {
                CODE_PROFILER_PUSH_TIME(profiler, statIdBlock)
                    counter++;
            }
        }

        ASSERT_TRUE(profiler.GetStatistic(statIdPerformance) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdPerformance)->GetNumSamples(), 1);

        ASSERT_TRUE(profiler.GetStatistic(statIdBlock) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdBlock)->GetNumSamples(), iter_count);

        ASSERT_TRUE(profiler.GetStatistic(statIdPerformance) != nullptr);

#undef CODE_PROFILER_PUSH_TIME

    }

#define CODE_PROFILER_PUSH_TIME(profiler, scopeNameId) \
    AZ::Statistics::StatisticalProfiler<AZStd::string, AZStd::shared_spin_mutex>::TimedScope AZ_JOIN(scope, __LINE__)(profiler, scopeNameId);

    static void simple_thread01(AZ::Statistics::StatisticalProfiler<AZStd::string, AZStd::shared_spin_mutex>* profiler, int loop_cnt)
    {
        const AZStd::string simple_thread("simple_thread1");
        const AZStd::string simple_thread_loop("simple_thread1_loop");

        CODE_PROFILER_PUSH_TIME(*profiler, simple_thread);

        static int counter = 0;
        for (int i = 0; i < loop_cnt; i++)
        {
            CODE_PROFILER_PUSH_TIME(*profiler, simple_thread_loop);
            counter++;
        }
    }

    static void simple_thread02(AZ::Statistics::StatisticalProfiler<AZStd::string, AZStd::shared_spin_mutex>* profiler, int loop_cnt)
    {
        const AZStd::string simple_thread("simple_thread2");
        const AZStd::string simple_thread_loop("simple_thread2_loop");

        CODE_PROFILER_PUSH_TIME(*profiler, simple_thread);

        static int counter = 0;
        for (int i = 0; i < loop_cnt; i++)
        {
            CODE_PROFILER_PUSH_TIME(*profiler, simple_thread_loop);
            counter++;
        }
    }

    static void simple_thread03(AZ::Statistics::StatisticalProfiler<AZStd::string, AZStd::shared_spin_mutex>* profiler, int loop_cnt)
    {
        const AZStd::string simple_thread("simple_thread3");
        const AZStd::string simple_thread_loop("simple_thread3_loop");

        CODE_PROFILER_PUSH_TIME(*profiler, simple_thread);

        static int counter = 0;
        for (int i = 0; i < loop_cnt; i++)
        {
            CODE_PROFILER_PUSH_TIME(*profiler, simple_thread_loop);
            counter++;
        }
    }

#undef CODE_PROFILER_PUSH_TIME

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
        const int iter_count = 10;
        AZStd::thread t1(AZStd::bind(&simple_thread01, &profiler, iter_count));
        AZStd::thread t2(AZStd::bind(&simple_thread02, &profiler, iter_count));
        AZStd::thread t3(AZStd::bind(&simple_thread03, &profiler, iter_count));
        t1.join();
        t2.join();
        t3.join();

        ASSERT_TRUE(profiler.GetStatistic(statIdThread1) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread1)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread1Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread1Loop)->GetNumSamples(), iter_count);

        ASSERT_TRUE(profiler.GetStatistic(statIdThread2) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread2)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread2Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread2Loop)->GetNumSamples(), iter_count);

        ASSERT_TRUE(profiler.GetStatistic(statIdThread3) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread3)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread3Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread3Loop)->GetNumSamples(), iter_count);

    }

    TEST_F(StatisticalProfilerTest, StatisticalProfilerProxy_ProfileCode_ValidateStatistics)
    {
#define CODE_PROFILER_PROXY_PUSH_TIME(profiler, scopeNameId) \
    AZ::Statistics::StatisticalProfilerProxy::TimedScope AZ_JOIN(scope, __LINE__)(profiler, scopeNameId);

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

        const int iter_count = 10;
        {
            CODE_PROFILER_PROXY_PUSH_TIME(ProfilerProxyGroup, statIdPerformance)
            int counter = 0;
            for (int i = 0; i < iter_count; i++)
            {
                CODE_PROFILER_PROXY_PUSH_TIME(ProfilerProxyGroup, statIdBlock)
                    counter++;
            }
        }

        ASSERT_TRUE(profiler.GetStatistic(statIdPerformance) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdPerformance)->GetNumSamples(), 1);

        ASSERT_TRUE(profiler.GetStatistic(statIdBlock) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdBlock)->GetNumSamples(), iter_count);

        //Clean Up
        proxy->ActivateProfiler(ProfilerProxyGroup, false);

#undef CODE_PROFILER_PROXY_PUSH_TIME

    }

#define CODE_PROFILER_PROXY_PUSH_TIME(profiler, scopeNameId) \
    AZ::Statistics::StatisticalProfilerProxy::TimedScope AZ_JOIN(scope, __LINE__)(profiler, scopeNameId);

    static void simple_thread1(int loop_cnt)
    {
        const AZ::Statistics::StatisticalProfilerProxy::StatIdType simple_thread1("simple_thread1");
        const AZ::Statistics::StatisticalProfilerProxy::StatIdType simple_thread1_loop("simple_thread1_loop");

        CODE_PROFILER_PROXY_PUSH_TIME(ProfilerProxyGroup, simple_thread1);

        static int counter = 0;
        for (int i = 0; i < loop_cnt; i++)
        {
            CODE_PROFILER_PROXY_PUSH_TIME(ProfilerProxyGroup, simple_thread1_loop);
            counter++;
        }
    }

    static void simple_thread2(int loop_cnt)
    {
        const AZ::Statistics::StatisticalProfilerProxy::StatIdType simple_thread2("simple_thread2");
        const AZ::Statistics::StatisticalProfilerProxy::StatIdType simple_thread2_loop("simple_thread2_loop");

        CODE_PROFILER_PROXY_PUSH_TIME(ProfilerProxyGroup, simple_thread2);

        static int counter = 0;
        for (int i = 0; i < loop_cnt; i++)
        {
            CODE_PROFILER_PROXY_PUSH_TIME(ProfilerProxyGroup, simple_thread2_loop);
            counter++;
        }
    }

    static void simple_thread3(int loop_cnt)
    {
        const AZ::Statistics::StatisticalProfilerProxy::StatIdType simple_thread3("simple_thread3");
        const AZ::Statistics::StatisticalProfilerProxy::StatIdType simple_thread3_loop("simple_thread3_loop");

        CODE_PROFILER_PROXY_PUSH_TIME(ProfilerProxyGroup, simple_thread3);

        static int counter = 0;
        for (int i = 0; i < loop_cnt; i++)
        {
            CODE_PROFILER_PROXY_PUSH_TIME(ProfilerProxyGroup, simple_thread3_loop);
            counter++;
        }
    }

#undef CODE_PROFILER_PROXY_PUSH_TIME

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
        const int iter_count = 10;
        AZStd::thread t1(AZStd::bind(&simple_thread1, iter_count));
        AZStd::thread t2(AZStd::bind(&simple_thread2, iter_count));
        AZStd::thread t3(AZStd::bind(&simple_thread3, iter_count));
        t1.join();
        t2.join();
        t3.join();

        ASSERT_TRUE(profiler.GetStatistic(statIdThread1) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread1)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread1Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread1Loop)->GetNumSamples(), iter_count);

        ASSERT_TRUE(profiler.GetStatistic(statIdThread2) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread2)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread2Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread2Loop)->GetNumSamples(), iter_count);

        ASSERT_TRUE(profiler.GetStatistic(statIdThread3) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread3)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread3Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread3Loop)->GetNumSamples(), iter_count);

        //Clean Up
        proxy->ActivateProfiler(ProfilerProxyGroup, false);
    }

    /** Trace message handler to track messages during tests
*/
    struct MyTraceMessageSink final
        : public AZ::Debug::TraceMessageDrillerBus::Handler
    {
        MyTraceMessageSink()
        {
            AZ::Debug::TraceMessageDrillerBus::Handler::BusConnect();
        }

        ~MyTraceMessageSink()
        {
            AZ::Debug::TraceMessageDrillerBus::Handler::BusDisconnect();
        }

        //////////////////////////////////////////////////////////////////////////
        // TraceMessageDrillerBus
        void OnPrintf(const char* window, const char* message) override
        {
            OnOutput(window, message);
        }

        void OnOutput(const char* window, const char* message) override
        {
            printf("%s: %s\n", window, message);
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
        //Helper macro.
#define CODE_PROFILER_PUSH_TIME(profiler, scopeNameId) \
    AZ::Statistics::StatisticalProfiler<>::TimedScope AZ_JOIN(scope, __LINE__)(profiler, scopeNameId);

        AZ::Statistics::StatisticalProfiler<> profiler;

        const AZStd::string statNamePerformance("PerformanceResult");
        const AZStd::string statNameBlock("Block");

        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statNamePerformance, statNamePerformance, "us") != nullptr);
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statNameBlock, statNameBlock, "us") != nullptr);

        const int iter_count = 1000000;
        {
            CODE_PROFILER_PUSH_TIME(profiler, statNamePerformance)
                int counter = 0;
            for (int i = 0; i < iter_count; i++)
            {
                CODE_PROFILER_PUSH_TIME(profiler, statNameBlock)
                    counter++;
            }
        }

        ASSERT_TRUE(profiler.GetStatistic(statNamePerformance) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statNamePerformance)->GetNumSamples(), 1);

        ASSERT_TRUE(profiler.GetStatistic(statNameBlock) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statNameBlock)->GetNumSamples(), iter_count);

        profiler.LogAndResetStats("StatisticalProfilerStringNoMutex");

        ASSERT_TRUE(profiler.GetStatistic(statNamePerformance) != nullptr);

#undef CODE_PROFILER_PUSH_TIME

    }

    TEST_F(Suite_StatisticalProfilerPerformance, StatisticalProfilerCrc32NoMutex_1ThreadPerformance)
    {
        //Helper macro.
#define CODE_PROFILER_PUSH_TIME(profiler, scopeNameId) \
    AZ::Statistics::StatisticalProfiler<AZ::Crc32>::TimedScope AZ_JOIN(scope, __LINE__)(profiler, scopeNameId);

        AZ::Statistics::StatisticalProfiler<AZ::Crc32> profiler;

        constexpr AZ::Crc32 statIdPerformance = AZ_CRC_CE("PerformanceResult");
        const AZStd::string statNamePerformance("PerformanceResult");

        constexpr AZ::Crc32 statIdBlock = AZ_CRC_CE("Block");
        const AZStd::string statNameBlock("Block");

        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdPerformance, statNamePerformance, "us") != nullptr);
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdBlock, statNameBlock, "us") != nullptr);

        const int iter_count = 1000000;
        {
            CODE_PROFILER_PUSH_TIME(profiler, statIdPerformance)
                int counter = 0;
            for (int i = 0; i < iter_count; i++)
            {
                CODE_PROFILER_PUSH_TIME(profiler, statIdBlock)
                    counter++;
            }
        }

        ASSERT_TRUE(profiler.GetStatistic(statIdPerformance) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdPerformance)->GetNumSamples(), 1);

        ASSERT_TRUE(profiler.GetStatistic(statIdBlock) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdBlock)->GetNumSamples(), iter_count);

        profiler.LogAndResetStats("StatisticalProfilerCrc32NoMutex");

        ASSERT_TRUE(profiler.GetStatistic(statIdPerformance) != nullptr);

#undef CODE_PROFILER_PUSH_TIME

    }

    TEST_F(Suite_StatisticalProfilerPerformance, StatisticalProfilerStringWithSharedSpinMutex_1ThreadPerformance)
    {
        //Helper macro.
#define CODE_PROFILER_PUSH_TIME(profiler, scopeNameId) \
    AZ::Statistics::StatisticalProfiler<AZStd::string, AZStd::shared_spin_mutex>::TimedScope AZ_JOIN(scope, __LINE__)(profiler, scopeNameId);

        AZ::Statistics::StatisticalProfiler<AZStd::string, AZStd::shared_spin_mutex> profiler;

        const AZStd::string statNamePerformance("PerformanceResult");
        const AZStd::string statNameBlock("Block");

        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statNamePerformance, statNamePerformance, "us") != nullptr);
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statNameBlock, statNameBlock, "us") != nullptr);

        const int iter_count = 1000000;
        {
            CODE_PROFILER_PUSH_TIME(profiler, statNamePerformance)
                int counter = 0;
            for (int i = 0; i < iter_count; i++)
            {
                CODE_PROFILER_PUSH_TIME(profiler, statNameBlock)
                    counter++;
            }
        }

        ASSERT_TRUE(profiler.GetStatistic(statNamePerformance) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statNamePerformance)->GetNumSamples(), 1);

        ASSERT_TRUE(profiler.GetStatistic(statNameBlock) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statNameBlock)->GetNumSamples(), iter_count);

        profiler.LogAndResetStats("StatisticalProfilerStringWithSharedSpinMutex");

        ASSERT_TRUE(profiler.GetStatistic(statNamePerformance) != nullptr);

#undef CODE_PROFILER_PUSH_TIME

    }

    TEST_F(Suite_StatisticalProfilerPerformance, StatisticalProfilerCrc32WithSharedSpinMutex_1ThreadPerformance)
    {
        //Helper macro.
#define CODE_PROFILER_PUSH_TIME(profiler, scopeNameId) \
    AZ::Statistics::StatisticalProfiler<AZ::Crc32, AZStd::shared_spin_mutex>::TimedScope AZ_JOIN(scope, __LINE__)(profiler, scopeNameId);

        AZ::Statistics::StatisticalProfiler<AZ::Crc32, AZStd::shared_spin_mutex> profiler;

        constexpr AZ::Crc32 statIdPerformance = AZ_CRC_CE("PerformanceResult");
        const AZStd::string statNamePerformance("PerformanceResult");

        constexpr AZ::Crc32 statIdBlock = AZ_CRC_CE("Block");
        const AZStd::string statNameBlock("Block");

        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdPerformance, statNamePerformance, "us") != nullptr);
        ASSERT_TRUE(profiler.GetStatsManager().AddStatistic(statIdBlock, statNameBlock, "us") != nullptr);

        const int iter_count = 1000000;
        {
            CODE_PROFILER_PUSH_TIME(profiler, statIdPerformance)
                int counter = 0;
            for (int i = 0; i < iter_count; i++)
            {
                CODE_PROFILER_PUSH_TIME(profiler, statIdBlock)
                    counter++;
            }
        }

        ASSERT_TRUE(profiler.GetStatistic(statIdPerformance) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdPerformance)->GetNumSamples(), 1);

        ASSERT_TRUE(profiler.GetStatistic(statIdBlock) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdBlock)->GetNumSamples(), iter_count);

        profiler.LogAndResetStats("StatisticalProfilerCrc32WithSharedSpinMutex");

        ASSERT_TRUE(profiler.GetStatistic(statIdPerformance) != nullptr);

#undef CODE_PROFILER_PUSH_TIME

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
        const int iter_count = 1000000;
        AZStd::thread t1(AZStd::bind(&simple_thread01, &profiler, iter_count));
        AZStd::thread t2(AZStd::bind(&simple_thread02, &profiler, iter_count));
        AZStd::thread t3(AZStd::bind(&simple_thread03, &profiler, iter_count));
        t1.join();
        t2.join();
        t3.join();

        ASSERT_TRUE(profiler.GetStatistic(statIdThread1) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread1)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread1Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread1Loop)->GetNumSamples(), iter_count);

        ASSERT_TRUE(profiler.GetStatistic(statIdThread2) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread2)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread2Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread2Loop)->GetNumSamples(), iter_count);

        ASSERT_TRUE(profiler.GetStatistic(statIdThread3) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread3)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread3Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread3Loop)->GetNumSamples(), iter_count);

        profiler.LogAndResetStats("3_Threads_StatisticalProfiler");

        ASSERT_TRUE(profiler.GetStatistic(statIdThread1) != nullptr);

    }

#define CODE_PROFILER_PROXY_PUSH_TIME(profiler, scopeNameId) \
    AZ::Statistics::StatisticalProfilerProxy::TimedScope AZ_JOIN(scope, __LINE__)(profiler, scopeNameId);

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

        const int iter_count = 1000000;
        {
            CODE_PROFILER_PROXY_PUSH_TIME(ProfilerProxyGroup, statIdPerformance)
                int counter = 0;
            for (int i = 0; i < iter_count; i++)
            {
                CODE_PROFILER_PROXY_PUSH_TIME(ProfilerProxyGroup, statIdBlock)
                    counter++;
            }
        }

        ASSERT_TRUE(profiler.GetStatistic(statIdPerformance) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdPerformance)->GetNumSamples(), 1);

        ASSERT_TRUE(profiler.GetStatistic(statIdBlock) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdBlock)->GetNumSamples(), iter_count);

        profiler.LogAndResetStats("StatisticalProfilerProxy");

        //Clean Up
        proxy->ActivateProfiler(ProfilerProxyGroup, false);
    }

#undef CODE_PROFILER_PROXY_PUSH_TIME

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
        const int iter_count = 1000000;
        AZStd::thread t1(AZStd::bind(&simple_thread1, iter_count));
        AZStd::thread t2(AZStd::bind(&simple_thread2, iter_count));
        AZStd::thread t3(AZStd::bind(&simple_thread3, iter_count));
        t1.join();
        t2.join();
        t3.join();

        ASSERT_TRUE(profiler.GetStatistic(statIdThread1) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread1)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread1Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread1Loop)->GetNumSamples(), iter_count);

        ASSERT_TRUE(profiler.GetStatistic(statIdThread2) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread2)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread2Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread2Loop)->GetNumSamples(), iter_count);

        ASSERT_TRUE(profiler.GetStatistic(statIdThread3) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread3)->GetNumSamples(), 1);
        ASSERT_TRUE(profiler.GetStatistic(statIdThread3Loop) != nullptr);
        EXPECT_EQ(profiler.GetStatistic(statIdThread3Loop)->GetNumSamples(), iter_count);

        profiler.LogAndResetStats("3_Threads_StatisticalProfilerProxy");

        //Clean Up
        proxy->ActivateProfiler(ProfilerProxyGroup, false);
    }

}//namespace UnitTest
