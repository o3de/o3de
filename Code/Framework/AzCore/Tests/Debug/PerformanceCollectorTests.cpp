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

#include <AzCore/Debug/PerformanceCollector.h>
#include <AzCore/JSON/document.h>
#include <AzCore/std/ranges/ranges_algorithm.h>

namespace UnitTest
{
    class PerformanceCollectorTest
        : public LeakDetectionFixture
    {
    }; //class PerformanceCollectorTest

    TEST_F(PerformanceCollectorTest, CreatePerformanceCollector_CollectPerformance_ValidateStatisticalOutput)
    {
        // In this test we use the PerformanceCollector to collect statistical data for three parameters.
        // Statistical data summarizes the results across a single batch of sampled frames.
        // In this test we run 3 batches, which means we expect 3 rows (aka json object) per measured
        // parameter. Each json object will contain statistical summary of a batch made of 10 frames.

        constexpr AZStd::string_view LogCategory("PerformanceCollectorTest");
        constexpr AZStd::string_view PerfParam1("param1");
        constexpr AZStd::string_view PerfParam2("param2");
        constexpr AZStd::string_view PerfParam3("param3");

        // Define control arguments that will define how much data to collect.
        // In real life these parameters are mirrored by CVARs.
        const AZ::u32 numberOfCaptureBatches = 3;
        const AZ::Debug::PerformanceCollector::DataLogType dataLogType = AZ::Debug::PerformanceCollector::DataLogType::LogStatistics;
        const AZ::u32 waitTimePerCaptureBatch = 0; //0 seconds
        const AZ::u32 frameCountPerCaptureBatch = 10;

        AZ::u32 pendingBatchCount = numberOfCaptureBatches;
        auto onCompleteCallback = [&](AZ::u32 batchCount) {
            pendingBatchCount = batchCount;
        };
        auto paramList = AZStd::to_array<AZStd::string_view>({ PerfParam1, PerfParam2, PerfParam3 });
        AZ::Debug::PerformanceCollector performanceCollector(LogCategory, paramList, onCompleteCallback);
        performanceCollector.UpdateDataLogType(dataLogType);
        performanceCollector.UpdateFrameCountPerCaptureBatch(frameCountPerCaptureBatch);
        performanceCollector.UpdateWaitTimeBeforeEachBatch(AZStd::chrono::seconds(waitTimePerCaptureBatch));
        performanceCollector.UpdateNumberOfCaptureBatches(numberOfCaptureBatches);

        // We will loop more than is required to simulate OnSystemTick, but the amount of collected data should
        // be limited to the specified control parameters.
        const AZ::u32 EXCESS_FRAME_LOOP = 5;
        for (AZ::u32 frameLoop = 0; frameLoop < ((numberOfCaptureBatches*frameCountPerCaptureBatch) + EXCESS_FRAME_LOOP); frameLoop++)
        {
            performanceCollector.FrameTick(); // Required Heart beat.

            performanceCollector.RecordPeriodicEvent(PerfParam1);

            {
                AZ::Debug::ScopeDuration param1Scope(&performanceCollector, PerfParam2);
            }

            {
                AZ::Debug::ScopeDuration param1Scope(&performanceCollector, PerfParam3);
            }
        }

        auto stringWithOutputData = performanceCollector.GetOutputDataBuffer();
        EXPECT_FALSE(stringWithOutputData.empty());
        EXPECT_EQ(pendingBatchCount, 0);

        rapidjson::Document jsonDoc;
        jsonDoc.Parse(stringWithOutputData.c_str());
        ASSERT_TRUE(!jsonDoc.HasParseError());

        // The root is an array of dictionaries.
        ASSERT_TRUE(jsonDoc.IsArray());
        // We have 3 paramaters and 3 capture batches. So we expect 9 arrays.
        auto expectedArraySize = paramList.size() * numberOfCaptureBatches;
        ASSERT_EQ(jsonDoc.Size(), expectedArraySize);
        for (rapidjson::SizeType i = 0; i < expectedArraySize; i += aznumeric_cast<rapidjson::SizeType>(paramList.size()))
        {
            AZStd::unordered_set<AZStd::string> validatedParams;
            AZStd::string paramName = jsonDoc[i]["name"].GetString();
            EXPECT_NE(AZStd::find(paramList.begin(), paramList.end(), paramName), paramList.end());
            ASSERT_FALSE(paramName.empty());
            ASSERT_FALSE(validatedParams.count(paramName) > 0);
            validatedParams.emplace(paramName);
            EXPECT_EQ(LogCategory, jsonDoc[i]["cat"].GetString());
            auto argsObj = jsonDoc[i]["args"].GetObject();
            ASSERT_TRUE(argsObj.HasMember(AZ::Debug::PerformanceCollector::AVG.data()));
            ASSERT_TRUE(argsObj.HasMember(AZ::Debug::PerformanceCollector::MIN.data()));
            ASSERT_TRUE(argsObj.HasMember(AZ::Debug::PerformanceCollector::MAX.data()));
            ASSERT_TRUE(argsObj.HasMember(AZ::Debug::PerformanceCollector::STDEV.data()));
            ASSERT_TRUE(argsObj.HasMember(AZ::Debug::PerformanceCollector::SAMPLE_COUNT.data()));

            auto sampleCount = argsObj[AZ::Debug::PerformanceCollector::SAMPLE_COUNT.data()].GetUint();
            if (paramName == PerfParam1)
            {
                // Periodic events work in deltas. The first frame is just for collecting
                // the Begin time, so there's no delta. For subsequent events there's
                // now previous data to subtract from and that's why we check for (FrameCount - 1).
                EXPECT_EQ(sampleCount, frameCountPerCaptureBatch - 1);
            }
            else
            {
                EXPECT_EQ(sampleCount, frameCountPerCaptureBatch);
            }
        }

    }

    TEST_F(PerformanceCollectorTest, CreatePerformanceCollector_CollectPerformance_ValidateAllSamplesOutput)
    {
        // In this test we use the PerformanceCollector to collect all data for three parameters.
        // the data is NOT statistically summarize so each time RecordPeriodicEvent() or
        // the AZ::Debug::ScopeDuration is used, a unique row (aka json object) will be added to the output json.

        const AZStd::string LogCategory("PerformanceCollectorTest");
        constexpr AZStd::string_view PerfParam1("param1");
        constexpr AZStd::string_view PerfParam2("param2");
        constexpr AZStd::string_view PerfParam3("param3");

        // Define control arguments that will define how much data to collect.
        // In real life these parameters are mirrored by CVARs.
        const AZ::u32 numberOfCaptureBatches = 3;
        const AZ::Debug::PerformanceCollector::DataLogType dataLogType = AZ::Debug::PerformanceCollector::DataLogType::LogAllSamples;
        const AZ::u32 waitTimePerCaptureBatch = 0; //0 seconds
        const AZ::u32 frameCountPerCaptureBatch = 10;

        AZ::u32 pendingBatchCount = numberOfCaptureBatches;
        auto onCompleteCallback = [&](AZ::u32 batchCount) {
            pendingBatchCount = batchCount;
        };
        auto paramList = AZStd::to_array<AZStd::string_view>({ PerfParam1, PerfParam2, PerfParam3 });
        AZ::Debug::PerformanceCollector performanceCollector(LogCategory, paramList, onCompleteCallback);
        performanceCollector.UpdateDataLogType(dataLogType);
        performanceCollector.UpdateFrameCountPerCaptureBatch(frameCountPerCaptureBatch);
        performanceCollector.UpdateWaitTimeBeforeEachBatch(AZStd::chrono::seconds(waitTimePerCaptureBatch));
        performanceCollector.UpdateNumberOfCaptureBatches(numberOfCaptureBatches);

        // We will loop more than is required to simulate OnSystemTick, but the amount of collected data should
        // be limited to the specified control parameters.
        const AZ::u32 EXCESS_FRAME_LOOP = 5;
        for (AZ::u32 frameLoop = 0; frameLoop < ((numberOfCaptureBatches*frameCountPerCaptureBatch) + EXCESS_FRAME_LOOP); frameLoop++)
        {
            performanceCollector.FrameTick(); // Required Heart beat.

            performanceCollector.RecordPeriodicEvent(PerfParam1);

            {
                AZ::Debug::ScopeDuration param1Scope(&performanceCollector, PerfParam2);
            }

            {
                AZ::Debug::ScopeDuration param1Scope(&performanceCollector, PerfParam3);
            }
        }

        auto stringWithOutputData = performanceCollector.GetOutputDataBuffer();
        EXPECT_FALSE(stringWithOutputData.empty());
        EXPECT_EQ(pendingBatchCount, 0);

        rapidjson::Document jsonDoc;
        jsonDoc.Parse(stringWithOutputData.c_str());
        ASSERT_TRUE(!jsonDoc.HasParseError());

        // The root is an array of dictionaries.
        ASSERT_TRUE(jsonDoc.IsArray());

        // When in DataLogType::LogAllSamples mode, each recorded data becomes a single dictionary in the
        // output json file. But in this test the parameter PerfParam1 is recorded with RecordPeriodicEvent()
        // which always skips the first frame because that frame is used to store the first previous value.
        auto expectedArraySize = (paramList.size() - 1) * numberOfCaptureBatches * frameCountPerCaptureBatch;
        expectedArraySize += numberOfCaptureBatches * (frameCountPerCaptureBatch - 1);
        ASSERT_EQ(jsonDoc.Size(), expectedArraySize);
        for (rapidjson::SizeType i = 0; i < expectedArraySize; i++)
        {
            AZStd::string paramName = jsonDoc[i]["name"].GetString();
            ASSERT_FALSE(paramName.empty());
            EXPECT_NE(AZStd::find(paramList.begin(), paramList.end(), paramName), paramList.end());
            EXPECT_EQ(LogCategory, jsonDoc[i]["cat"].GetString());
            EXPECT_TRUE(jsonDoc[i].HasMember("dur"));
        }
    }

    TEST_F(PerformanceCollectorTest, CreatePerformnaceCollector_WithDefaultFileExtension_ValidateFileExtension)
    {
        auto paramList = AZStd::to_array<AZStd::string_view>({ "param1" });
        auto onCompleteCallback = [](AZ::u32)
        {
        };

        AZ::Debug::PerformanceCollector performanceCollector("PerformanceCollectorTest", paramList, onCompleteCallback);

        const AZStd::string& actualExtension = performanceCollector.GetFileExtension();
        ASSERT_EQ("json", actualExtension);
    }

    TEST_F(PerformanceCollectorTest, CreatePerformanceCollector_WithCustomFileExtension_ValidateFileExtension)
    {
        auto paramList = AZStd::to_array<AZStd::string_view>({ "param1" });
        auto onCompleteCallback = [](AZ::u32)
        {
        };
        const AZStd::string testFileExtention("test.json");

        AZ::Debug::PerformanceCollector performanceCollector("PerformanceCollectorTest", paramList, onCompleteCallback, testFileExtention);

        const AZStd::string& actualExtension = performanceCollector.GetFileExtension();
        ASSERT_EQ(testFileExtention, actualExtension);
    }

}//namespace UnitTest
