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

#include <AzCore/Statistics/StatisticsManager.h>

using namespace AZ;
using namespace Debug;

namespace UnitTest
{
    class StatisticsTest
        : public LeakDetectionFixture
    {
    public:
        StatisticsTest()
        {
        }

        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            m_dataSamples = AZStd::make_unique<AZStd::vector<u32>>();
            const u32 numSamples = 100;
            m_dataSamples->set_capacity(numSamples);
            for (u32 i = 0; i < numSamples; ++i)
            {
                m_dataSamples->push_back(i);
            }
        }

        ~StatisticsTest() override
        {
        }

        void TearDown() override
        {
            // clearing up memory
            m_dataSamples = nullptr;

            LeakDetectionFixture::TearDown();
        }

    protected:
        AZStd::unique_ptr<AZStd::vector<u32>> m_dataSamples;
    }; //class StatisticsTest

    TEST_F(StatisticsTest, RunningStatistic_ProcessAnArrayOfNumbers_GetExpectedStatisticalData)
    {
        Statistics::RunningStatistic runningStat;

        ASSERT_TRUE(m_dataSamples.get() != nullptr);
        const AZStd::vector<u32>& dataSamples = *m_dataSamples;
        for (u32 sample : dataSamples)
        {
            runningStat.PushSample(sample);
        }

        EXPECT_EQ(runningStat.GetNumSamples(), dataSamples.size());
        EXPECT_EQ(runningStat.GetMostRecentSample(), dataSamples.back());
        EXPECT_EQ(runningStat.GetMinimum(), dataSamples[0]);
        EXPECT_EQ(runningStat.GetMaximum(), dataSamples.back());
        EXPECT_NEAR(runningStat.GetAverage(), 49.5, 0.001);
        EXPECT_NEAR(runningStat.GetVariance(), 841.666, 0.001);
        EXPECT_NEAR(runningStat.GetStdev(), 29.011, 0.001);
        EXPECT_NEAR(runningStat.GetVariance(Statistics::VarianceType::P), 833.25, 0.001);
        EXPECT_NEAR(runningStat.GetStdev(Statistics::VarianceType::P), 28.866, 0.001);

        //Reset the stat object.
        runningStat.Reset();
        EXPECT_EQ(runningStat.GetNumSamples(), 0);
        EXPECT_EQ(runningStat.GetAverage(), 0.0);
        EXPECT_EQ(runningStat.GetStdev(), 0.0);
    }


    TEST_F(StatisticsTest, StatisticsManager_AddAndRemoveStatisticistics_CollectionIntegrityIsCorrect)
    {
        Statistics::StatisticsManager<> statsManager;
        AZStd::string statName0("stat0");
        AZStd::string statName1("stat1");
        AZStd::string statName2("stat2");
        AZStd::string statName3("stat3");
        EXPECT_TRUE(statsManager.AddStatistic(statName0, statName0, ""));
        EXPECT_TRUE(statsManager.AddStatistic(statName1, statName1, ""));
        EXPECT_TRUE(statsManager.AddStatistic(statName2, statName2, ""));
        EXPECT_TRUE(statsManager.AddStatistic(statName3, statName3, ""));

        //Validate the number of running statistics object we have so far.
        {
            AZStd::vector<Statistics::NamedRunningStatistic*> allStats;
            statsManager.GetAllStatistics(allStats);
            EXPECT_TRUE(allStats.size() == 4);
        }

        //Try to add an Stat that already exist. expect to fail.
        EXPECT_EQ(statsManager.AddStatistic(statName1), nullptr);

        //Remove stat1.
        statsManager.RemoveStatistic(statName1);
        //Validate the number of running statistics object we have so far.
        {
            AZStd::vector<Statistics::NamedRunningStatistic*> allStats;
            statsManager.GetAllStatistics(allStats);
            EXPECT_TRUE(allStats.size() == 3);
        }

        //Add stat1 again, expect to pass.
        EXPECT_TRUE(statsManager.AddStatistic(statName1));

        //Get a pointer to stat2.
        Statistics::NamedRunningStatistic* stat2 = statsManager.GetStatistic(statName2);
        ASSERT_TRUE(stat2 != nullptr);
        EXPECT_EQ(stat2->GetName(), statName2);
    }

    TEST_F(StatisticsTest, StatisticsManager_DistributeSamplesAcrossStatistics_StatisticsAreCorrect)
    {
        Statistics::StatisticsManager<> statsManager;
        AZStd::string statName0("stat0");
        AZStd::string statName1("stat1");
        AZStd::string statName2("stat2");
        AZStd::string statName3("stat3");

        EXPECT_TRUE(statsManager.AddStatistic(statName3));
        EXPECT_TRUE(statsManager.AddStatistic(statName0));
        EXPECT_TRUE(statsManager.AddStatistic(statName2));
        EXPECT_TRUE(statsManager.AddStatistic(statName1));

        //Distribute the 100 samples of data evenly across the 4 running statistics.
        ASSERT_TRUE(m_dataSamples.get() != nullptr);
        const AZStd::vector<u32>& dataSamples = *m_dataSamples;
        const size_t numSamples = dataSamples.size();
        const size_t numSamplesPerStat = numSamples / 4;
        size_t sampleIndex = 0;
        size_t nextStopIndex = numSamplesPerStat;
        while (sampleIndex < nextStopIndex)
        {
            statsManager.PushSampleForStatistic(statName0, dataSamples[sampleIndex]);
            sampleIndex++;
        }
        nextStopIndex += numSamplesPerStat;
        while (sampleIndex < nextStopIndex)
        {
            statsManager.PushSampleForStatistic(statName1, dataSamples[sampleIndex]);
            sampleIndex++;
        }
        nextStopIndex += numSamplesPerStat;
        while (sampleIndex < nextStopIndex)
        {
            statsManager.PushSampleForStatistic(statName2, dataSamples[sampleIndex]);
            sampleIndex++;
        }
        nextStopIndex += numSamplesPerStat;
        while (sampleIndex < nextStopIndex)
        {
            statsManager.PushSampleForStatistic(statName3, dataSamples[sampleIndex]);
            sampleIndex++;
        }

        EXPECT_NEAR(statsManager.GetStatistic(statName0)->GetAverage(), 12.0, 0.001);
        EXPECT_NEAR(statsManager.GetStatistic(statName1)->GetAverage(), 37.0, 0.001);
        EXPECT_NEAR(statsManager.GetStatistic(statName2)->GetAverage(), 62.0, 0.001);
        EXPECT_NEAR(statsManager.GetStatistic(statName3)->GetAverage(), 87.0, 0.001);

        EXPECT_NEAR(statsManager.GetStatistic(statName0)->GetStdev(), 7.359, 0.001);
        EXPECT_NEAR(statsManager.GetStatistic(statName1)->GetStdev(), 7.359, 0.001);
        EXPECT_NEAR(statsManager.GetStatistic(statName2)->GetStdev(), 7.359, 0.001);
        EXPECT_NEAR(statsManager.GetStatistic(statName3)->GetStdev(), 7.359, 0.001);

        //Reset one of the stats.
        statsManager.ResetStatistic(statName2);
        EXPECT_EQ(statsManager.GetStatistic(statName2)->GetAverage(), 0.0);
        //Reset all of the stats.
        statsManager.ResetAllStatistics();
        EXPECT_EQ(statsManager.GetStatistic(statName0)->GetNumSamples(), 0);
        EXPECT_EQ(statsManager.GetStatistic(statName1)->GetNumSamples(), 0);
        EXPECT_EQ(statsManager.GetStatistic(statName2)->GetNumSamples(), 0);
        EXPECT_EQ(statsManager.GetStatistic(statName3)->GetNumSamples(), 0);
    }

    TEST_F(StatisticsTest, StatisticsManagerCrc32_DistributeSamplesAcrossStatistics_StatisticsAreCorrect)
    {
        Statistics::StatisticsManager<AZ::Crc32> statsManager;
        AZ::Crc32 statName0 = AZ_CRC_CE("stat0");
        AZ::Crc32 statName1 = AZ_CRC_CE("stat1");
        AZ::Crc32 statName2 = AZ_CRC_CE("stat2");
        AZ::Crc32 statName3 = AZ_CRC_CE("stat3");

        EXPECT_TRUE(statsManager.AddStatistic(statName3) != nullptr);
        EXPECT_TRUE(statsManager.AddStatistic(statName0) != nullptr);
        EXPECT_TRUE(statsManager.AddStatistic(statName2) != nullptr);
        EXPECT_TRUE(statsManager.AddStatistic(statName1) != nullptr);

        EXPECT_TRUE(statsManager.GetStatistic(statName3) != nullptr);
        EXPECT_TRUE(statsManager.GetStatistic(statName0) != nullptr);
        EXPECT_TRUE(statsManager.GetStatistic(statName1) != nullptr);
        EXPECT_TRUE(statsManager.GetStatistic(statName2) != nullptr);

        //Distribute the 100 samples of data evenly across the 4 running statistics.
        ASSERT_TRUE(m_dataSamples.get() != nullptr);
        const AZStd::vector<u32>& dataSamples = *m_dataSamples;
        const size_t numSamples = dataSamples.size();
        const size_t numSamplesPerStat = numSamples / 4;
        size_t sampleIndex = 0;
        size_t nextStopIndex = numSamplesPerStat;
        while (sampleIndex < nextStopIndex)
        {
            statsManager.PushSampleForStatistic(statName0, dataSamples[sampleIndex]);
            sampleIndex++;
        }
        nextStopIndex += numSamplesPerStat;
        while (sampleIndex < nextStopIndex)
        {
            statsManager.PushSampleForStatistic(statName1, dataSamples[sampleIndex]);
            sampleIndex++;
        }
        nextStopIndex += numSamplesPerStat;
        while (sampleIndex < nextStopIndex)
        {
            statsManager.PushSampleForStatistic(statName2, dataSamples[sampleIndex]);
            sampleIndex++;
        }
        nextStopIndex += numSamplesPerStat;
        while (sampleIndex < nextStopIndex)
        {
            statsManager.PushSampleForStatistic(statName3, dataSamples[sampleIndex]);
            sampleIndex++;
        }

        EXPECT_NEAR(statsManager.GetStatistic(statName0)->GetAverage(), 12.0, 0.001);
        EXPECT_NEAR(statsManager.GetStatistic(statName1)->GetAverage(), 37.0, 0.001);
        EXPECT_NEAR(statsManager.GetStatistic(statName2)->GetAverage(), 62.0, 0.001);
        EXPECT_NEAR(statsManager.GetStatistic(statName3)->GetAverage(), 87.0, 0.001);

        EXPECT_NEAR(statsManager.GetStatistic(statName0)->GetStdev(), 7.359, 0.001);
        EXPECT_NEAR(statsManager.GetStatistic(statName1)->GetStdev(), 7.359, 0.001);
        EXPECT_NEAR(statsManager.GetStatistic(statName2)->GetStdev(), 7.359, 0.001);
        EXPECT_NEAR(statsManager.GetStatistic(statName3)->GetStdev(), 7.359, 0.001);

        //Reset one of the stats.
        statsManager.ResetStatistic(statName2);
        EXPECT_EQ(statsManager.GetStatistic(statName2)->GetAverage(), 0.0);
        //Reset all of the stats.
        statsManager.ResetAllStatistics();
        EXPECT_EQ(statsManager.GetStatistic(statName0)->GetNumSamples(), 0);
        EXPECT_EQ(statsManager.GetStatistic(statName1)->GetNumSamples(), 0);
        EXPECT_EQ(statsManager.GetStatistic(statName2)->GetNumSamples(), 0);
        EXPECT_EQ(statsManager.GetStatistic(statName3)->GetNumSamples(), 0);
    }

}//namespace UnitTest
