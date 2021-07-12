/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzTest/AzTest.h>

#include <AzCore/std/string/string.h>
#include <AzCore/std/parallel/thread.h>

#include <AzCore/Statistics/TimeDataStatisticsManager.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Debug/FrameProfilerBus.h>
#include <AzCore/Debug/FrameProfilerComponent.h>

using namespace AZ;
using namespace Debug;

namespace UnitTest
{
    /**
     * Validate functionality of the convenience class TimeDataStatisticsManager.
     * It is a specialized version of RunningStatisticsManager that works with Timer type
     * of registers that can be captured with the FrameProfilerBus::OnFrameProfilerData()
     */
    class TimeDataStatisticsManagerTest
        : public AllocatorsFixture
        , public FrameProfilerBus::Handler
    {
        static constexpr const char* PARENT_TIMER_STAT = "ParentStat";
        static constexpr const char* CHILD_TIMER_STAT0 = "ChildStat0";
        static constexpr const char* CHILD_TIMER_STAT1 = "ChildStat1";

    public:
        TimeDataStatisticsManagerTest()
            : AllocatorsFixture()
        {
        }

        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            m_statsManager = AZStd::make_unique<Statistics::TimeDataStatisticsManager>();
        }

        void TearDown() override
        {
            m_statsManager = nullptr;
            AllocatorsFixture::TearDown();
        }

        //////////////////////////////////////////////////////////////////////////
        // FrameProfilerBus
        virtual void OnFrameProfilerData(const FrameProfiler::ThreadDataArray& data)
        {
            for (size_t iThread = 0; iThread < data.size(); ++iThread)
            {
                const FrameProfiler::ThreadData& td = data[iThread];
                FrameProfiler::ThreadData::RegistersMap::const_iterator regIt = td.m_registers.begin();
                for (; regIt != td.m_registers.end(); ++regIt)
                {
                    const FrameProfiler::RegisterData& rd = regIt->second;
                    u32 unitTestCrc = AZ_CRC("UnitTest", 0x8089cea8);
                    if (unitTestCrc != rd.m_systemId)
                    {
                        continue; //Not for us.
                    }
                    ASSERT_EQ(ProfilerRegister::PRT_TIME, rd.m_type);
                    const FrameProfiler::FrameData& fd = rd.m_frames.back();
                    m_statsManager->PushTimeDataSample(rd.m_name, fd.m_timeData);
                }
            }
        }
        //////////////////////////////////////////////////////////////////////////

        int ChildFunction0(int numIterations, int sleepTimeMilliseconds)
        {
            AZ_PROFILE_TIMER("UnitTest", CHILD_TIMER_STAT0);
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(sleepTimeMilliseconds));
            int result = 5;
            for (int i = 0; i < numIterations; ++i)
            {
                result += i % 3;
            }
            return result;
        }

        int ChildFunction1(int numIterations, int sleepTimeMilliseconds)
        {
            AZ_PROFILE_TIMER("UnitTest", CHILD_TIMER_STAT1);
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(sleepTimeMilliseconds));
            int result = 5;
            for (int i = 0; i < numIterations; ++i)
            {
                result += i % 3;
            }
            return result;
        }

        int ParentFunction(int numIterations, int sleepTimeMilliseconds)
        {
            AZ_PROFILE_TIMER("UnitTest", PARENT_TIMER_STAT);
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(sleepTimeMilliseconds));
            int result = 0;
            result += ChildFunction0(numIterations, sleepTimeMilliseconds);
            result += ChildFunction1(numIterations, sleepTimeMilliseconds);
            return result;
        }

        void run()
        {
           Debug::FrameProfilerBus::Handler::BusConnect();
           
           ComponentApplication app;
           ComponentApplication::Descriptor desc;
           desc.m_useExistingAllocator = true;
           desc.m_enableDrilling = false;  // we already created a memory driller for the test (AllocatorsFixture)
           ComponentApplication::StartupParameters startupParams;
           startupParams.m_allocator = &AllocatorInstance<SystemAllocator>::Get();
           Entity* systemEntity = app.Create(desc, startupParams);
           systemEntity->CreateComponent<FrameProfilerComponent>();
           
           systemEntity->Init();
           systemEntity->Activate(); // start frame component
           
           const int sleepTimeAllFuncsMillis = 1;
           const int numIterations = 10;
           for (int iterationCounter = 0; iterationCounter < numIterations; ++iterationCounter)
           {
               ParentFunction(numIterations, sleepTimeAllFuncsMillis);
               //Collect all samples.
               app.Tick();
           }

           //Verify we have three running stats.
           {
               AZStd::vector<Statistics::NamedRunningStatistic*> allStats;
               m_statsManager->GetAllStatistics(allStats);
               EXPECT_EQ(allStats.size(), 3);
           }

           AZStd::string parentStatName(PARENT_TIMER_STAT);
           AZStd::string child0StatName(CHILD_TIMER_STAT0);
           AZStd::string child1StatName(CHILD_TIMER_STAT1);
           ASSERT_TRUE(m_statsManager->GetStatistic(parentStatName) != nullptr);
           ASSERT_TRUE(m_statsManager->GetStatistic(child0StatName) != nullptr);
           ASSERT_TRUE(m_statsManager->GetStatistic(child1StatName) != nullptr);
           
           EXPECT_EQ(m_statsManager->GetStatistic(parentStatName)->GetNumSamples(), numIterations);
           EXPECT_EQ(m_statsManager->GetStatistic(child0StatName)->GetNumSamples(), numIterations);
           EXPECT_EQ(m_statsManager->GetStatistic(child1StatName)->GetNumSamples(), numIterations);

           const double minimumExpectDurationOfChildFunctionMicros = 1;
           const double minimumExpectDurationOfParentFunctionMicros = 1;

           EXPECT_GE(m_statsManager->GetStatistic(parentStatName)->GetMinimum(), minimumExpectDurationOfParentFunctionMicros);
           EXPECT_GE(m_statsManager->GetStatistic(parentStatName)->GetAverage(), minimumExpectDurationOfParentFunctionMicros);
           EXPECT_GE(m_statsManager->GetStatistic(parentStatName)->GetMaximum(), minimumExpectDurationOfParentFunctionMicros);

           EXPECT_GE(m_statsManager->GetStatistic(child0StatName)->GetMinimum(), minimumExpectDurationOfChildFunctionMicros);
           EXPECT_GE(m_statsManager->GetStatistic(child0StatName)->GetAverage(), minimumExpectDurationOfChildFunctionMicros);
           EXPECT_GE(m_statsManager->GetStatistic(child0StatName)->GetMaximum(), minimumExpectDurationOfChildFunctionMicros);
                                                                                                                      
           EXPECT_GE(m_statsManager->GetStatistic(child1StatName)->GetMinimum(), minimumExpectDurationOfChildFunctionMicros);
           EXPECT_GE(m_statsManager->GetStatistic(child1StatName)->GetAverage(), minimumExpectDurationOfChildFunctionMicros);
           EXPECT_GE(m_statsManager->GetStatistic(child1StatName)->GetMaximum(), minimumExpectDurationOfChildFunctionMicros);

           //Let's validate TimeDataStatisticsManager::RemoveStatistics()
           m_statsManager->RemoveStatistic(child1StatName);
           ASSERT_TRUE(m_statsManager->GetStatistic(parentStatName) != nullptr);
           ASSERT_TRUE(m_statsManager->GetStatistic(child0StatName) != nullptr);
           EXPECT_EQ(m_statsManager->GetStatistic(child1StatName), nullptr);

           //Let's store the sample count for both parentStatName and child0StatName.
           const AZ::u64 numSamplesParent = m_statsManager->GetStatistic(parentStatName)->GetNumSamples();
           const AZ::u64 numSamplesChild0 = m_statsManager->GetStatistic(child0StatName)->GetNumSamples();

           //Let's call child1 function again and call app.Tick(). child1StatName should be readded to m_statsManager.
           ChildFunction1(numIterations, sleepTimeAllFuncsMillis);
           app.Tick();
           ASSERT_TRUE(m_statsManager->GetStatistic(child1StatName) != nullptr);
           EXPECT_EQ(m_statsManager->GetStatistic(parentStatName)->GetNumSamples(), numSamplesParent);
           EXPECT_EQ(m_statsManager->GetStatistic(child0StatName)->GetNumSamples(), numSamplesChild0);
           EXPECT_EQ(m_statsManager->GetStatistic(child1StatName)->GetNumSamples(), 1);

           Debug::FrameProfilerBus::Handler::BusDisconnect();
           app.Destroy();
        }

        AZStd::unique_ptr<Statistics::TimeDataStatisticsManager> m_statsManager;
    };//class TimeDataStatisticsManagerTest

    TEST_F(TimeDataStatisticsManagerTest, Test)
    {
        run();
    }
    //End of all Tests of TimeDataStatisticsManagerTest

}//namespace UnitTest
