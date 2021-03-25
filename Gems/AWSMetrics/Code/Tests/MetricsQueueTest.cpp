/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AWSMetricsConstant.h>
#include <AWSMetricsGemMock.h>
#include <MetricsEvent.h>
#include <MetricsQueue.h>

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AWSMetrics
{
    class MetricsQueueTest
        : public AWSMetricsGemAllocatorFixture
    {
    public:
        const int NUM_TEST_METRICS = 10;
        const AZStd::string ATTR_NAME = "name";
        const AZStd::string ATTR_VALUE = "value";
    };

    TEST_F(MetricsQueueTest, AddMetrics_SingleMetrics_Success)
    {
        MetricsQueue queue;
        int numMetrics = queue.GetNumMetrics();

        queue.AddMetrics(MetricsEvent());

        ASSERT_EQ(queue.GetNumMetrics(), ++numMetrics);
    }

    TEST_F(MetricsQueueTest, AppendMetrics_EmptyQueue_Success)
    {
        MetricsQueue queue;
        int numMetrics = queue.GetNumMetrics();

        MetricsQueue anotherQueue;
        for (int index = 0; index < NUM_TEST_METRICS; ++index)
        {
            anotherQueue.AddMetrics(MetricsEvent());
        }
        
        queue.AppendMetrics(anotherQueue);

        ASSERT_EQ(queue.GetNumMetrics(), numMetrics + NUM_TEST_METRICS);
    }

    TEST_F(MetricsQueueTest, AppendMetrics_NoneEmptyQueue_Success)
    {
        MetricsQueue queue;
        queue.AddMetrics(MetricsEvent());
        int numMetrics = queue.GetNumMetrics();

        MetricsQueue anotherQueue;
        for (int index = 0; index < NUM_TEST_METRICS; ++index)
        {
            anotherQueue.AddMetrics(MetricsEvent());
        }

        queue.AppendMetrics(anotherQueue);

        ASSERT_EQ(queue.GetNumMetrics(), numMetrics + NUM_TEST_METRICS);
    }

    TEST_F(MetricsQueueTest, PushMetricsToFront_EmptyQueue_Success)
    {
        MetricsQueue queue;
        int numMetrics = queue.GetNumMetrics();

        MetricsQueue anotherQueue;
        for (int index = 0; index < NUM_TEST_METRICS; ++index)
        {
            anotherQueue.AddMetrics(MetricsEvent());
        }

        queue.PushMetricsToFront(anotherQueue);

        ASSERT_EQ(queue.GetNumMetrics(), numMetrics + NUM_TEST_METRICS);
    }

    TEST_F(MetricsQueueTest, PushMetricsToFront_NoneEmptyQueue_Success)
    {
        MetricsQueue queue;
        MetricsEvent metrics;
        metrics.AddAttribute(MetricsAttribute(ATTR_NAME, ATTR_VALUE));
        queue.AddMetrics(metrics);
        int numMetrics = queue.GetNumMetrics();

        MetricsQueue anotherQueue;
        for (int index = 0; index < NUM_TEST_METRICS; ++index)
        {
            anotherQueue.AddMetrics(MetricsEvent());
        }

        queue.PushMetricsToFront(anotherQueue);

        ASSERT_EQ(queue.GetNumMetrics(), numMetrics + NUM_TEST_METRICS);
        ASSERT_EQ(queue[NUM_TEST_METRICS].GetNumAttributes(), 1);
    }

    TEST_F(MetricsQueueTest, FilterMetricsByPriority_ReachMaxCapacity_FilterOutLowerPriorityMetrics)
    {
        MetricsQueue queue;
        for (int index = 0; index < NUM_TEST_METRICS; ++index)
        {
            MetricsEvent metrics;
            metrics.AddAttribute(MetricsAttribute(ATTR_NAME, ATTR_VALUE));
            metrics.SetEventPriority(index % 2);

            // Use the number of failures to check the order of metrics events later.
            // Older events will have less number of failures than the newer ones based on the settings in this test.
            for (int numRetries = 0; numRetries < index; ++numRetries)
            {
                metrics.MarkFailedSubmission();
            }

            queue.AddMetrics(metrics);
        }

        int maxCapacity = queue[0].GetSizeInBytes() * NUM_TEST_METRICS / 2;

        ASSERT_EQ(queue.FilterMetricsByPriority(maxCapacity), NUM_TEST_METRICS / 2);
        ASSERT_EQ(queue.GetNumMetrics(), NUM_TEST_METRICS / 2);

        for (int index = 0; index < queue.GetNumMetrics(); ++index)
        {
            ASSERT_EQ(queue[index].GetEventPriority(), 0);

            if (index > 0 && queue[index].GetEventPriority() == queue[index - 1].GetEventPriority())
            {
                // Check the order of metrics events in the queue.
                // Newer events should be placed in front of the older ones.
                ASSERT_LT(queue[index].GetNumFailures(), queue[index - 1].GetNumFailures());
            }
        }
    }

    TEST_F(MetricsQueueTest, ClearMetrics_NoneEmptyQueue_Success)
    {
        MetricsQueue queue;
        for (int index = 0; index < NUM_TEST_METRICS; ++index)
        {
            queue.AddMetrics(MetricsEvent());
        }
        ASSERT_EQ(queue.GetNumMetrics(), NUM_TEST_METRICS);

        queue.ClearMetrics();

        ASSERT_EQ(queue.GetNumMetrics(), 0);
    }

    TEST_F(MetricsQueueTest, SerializeToJsonForLocalFile_DefaultandCustomMetricsAttributes_Success)
    {
        MetricsQueue queue;
        MetricsEvent metrics;
        metrics.AddAttribute(MetricsAttribute(ATTR_NAME, ATTR_VALUE));
        metrics.AddAttribute(MetricsAttribute(METRICS_ATTRIBUTE_KEY_EVENT_NAME, ATTR_VALUE));
        queue.AddMetrics(metrics);

        AZStd::string serializedQueue = AZStd::string::format("[{\"%s\":\"%s\",\"%s\":{\"%s\":\"%s\"}}]",
            METRICS_ATTRIBUTE_KEY_EVENT_NAME, ATTR_VALUE.c_str(), METRICS_ATTRIBUTE_KEY_EVENT_DATA, ATTR_NAME.c_str(), ATTR_VALUE.c_str());
        ASSERT_EQ(queue.SerializeToJson(), serializedQueue);
    }

    TEST_F(MetricsQueueTest, SerializeToJsonForServiceApi_DefaultandCustomMetricsAttributes_Success)
    {
        MetricsQueue queue;
        MetricsEvent metrics;
        metrics.AddAttribute(MetricsAttribute(ATTR_NAME, ATTR_VALUE));
        metrics.AddAttribute(MetricsAttribute(METRICS_ATTRIBUTE_KEY_EVENT_NAME, ATTR_VALUE));
        queue.AddMetrics(metrics);

        std::ostream stream(nullptr);
        AWSCore::JsonOutputStream jsonStream{ stream };
        AWSCore::JsonWriter writer{ jsonStream };

        ASSERT_TRUE(queue.SerializeToJson(writer));
    }

    TEST_F(MetricsQueueTest, ReadFromJson_DefaultandCustomMetricsAttributes_Success)
    {
        MetricsQueue queue;

        AZStd::string testFilePath = GetDefaultTestFilePath();
        AZStd::string serializedQueue = AZStd::string::format("[{\"%s\":\"%s\",\"%s\":{\"%s\":\"%s\"}}]",
            METRICS_ATTRIBUTE_KEY_EVENT_NAME, ATTR_VALUE.c_str(), METRICS_ATTRIBUTE_KEY_EVENT_DATA, ATTR_NAME.c_str(), ATTR_VALUE.c_str());
        ASSERT_TRUE(CreateFile(testFilePath, serializedQueue));
        
        ASSERT_TRUE(queue.ReadFromJson(testFilePath));
        ASSERT_EQ(queue.GetNumMetrics(), 1);

        ASSERT_TRUE(RemoveFile(testFilePath));
    }

    TEST_F(MetricsQueueTest, ReadFromJson_InvalidJsonFilePath_Fail)
    {
        MetricsQueue queue;

        AZStd::string testFilePath = GetDefaultTestFilePath();

        AZ_TEST_START_TRACE_SUPPRESSION;
        ASSERT_FALSE(queue.ReadFromJson(testFilePath));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(MetricsQueueTest, ReadFromJson_InvalidJsonFormat_Fail)
    {
        MetricsQueue queue;

        AZStd::string testFilePath = GetDefaultTestFilePath();
        AZStd::string serializedQueue = AZStd::string::format("{\"%s\":\"%s\",\"%s\":{\"%s\":\"%s\"}}",
            METRICS_ATTRIBUTE_KEY_EVENT_NAME, ATTR_VALUE.c_str(), METRICS_ATTRIBUTE_KEY_EVENT_DATA, ATTR_NAME.c_str(), ATTR_VALUE.c_str());
        ASSERT_TRUE(CreateFile(testFilePath, serializedQueue));

        AZ_TEST_START_TRACE_SUPPRESSION;
        ASSERT_FALSE(queue.ReadFromJson(testFilePath));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        ASSERT_TRUE(RemoveFile(testFilePath));
    }

     TEST_F(MetricsQueueTest, ReadFromString_DefaultandCustomMetricsAttributes_Success)
    {
        MetricsQueue queue;

        AZStd::string serializedQueue = AZStd::string::format(
            "[{\"%s\":\"%s\",\"%s\":{\"%s\":\"%s\"}}]", METRICS_ATTRIBUTE_KEY_EVENT_NAME, ATTR_VALUE.c_str(),
            METRICS_ATTRIBUTE_KEY_EVENT_DATA, ATTR_NAME.c_str(), ATTR_VALUE.c_str());

        ASSERT_TRUE(queue.ReadFromString(serializedQueue));
        ASSERT_EQ(queue.GetNumMetrics(), 1);
    }

    TEST_F(MetricsQueueTest, ReadFromString_InvalidJsonFormat_Fail)
    {
        MetricsQueue queue;

        AZStd::string serializedQueue = AZStd::string::format(
            "{\"%s\":\"%s\",\"%s\":{\"%s\":\"%s\"}}", METRICS_ATTRIBUTE_KEY_EVENT_NAME, ATTR_VALUE.c_str(),
            METRICS_ATTRIBUTE_KEY_EVENT_DATA, ATTR_NAME.c_str(), ATTR_VALUE.c_str());

        AZ_TEST_START_TRACE_SUPPRESSION;
        ASSERT_FALSE(queue.ReadFromString(serializedQueue));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(MetricsQueueTest, PopBufferedEventsByServiceLimits_QueueSizeExceedsLimits_BufferedEventsAddedToNewQueue)
    {
        MetricsEvent metrics;
        metrics.AddAttribute(MetricsAttribute(ATTR_NAME, ATTR_VALUE));
        int sizeOfEachMetrics = metrics.GetSizeInBytes();

        MetricsQueue queue;
        queue.AddMetrics(metrics);
     
        for (int index = 0; index < NUM_TEST_METRICS - 1; ++index)
        {
            MetricsEvent newMetrics;
            newMetrics.AddAttribute(MetricsAttribute(ATTR_NAME, ATTR_VALUE));

            queue.AddMetrics(newMetrics);
        }

        MetricsQueue anotherQueue;
        // Payload size limit is hit.
        queue.PopBufferedEventsByServiceLimits(anotherQueue, sizeOfEachMetrics, NUM_TEST_METRICS + 1);

        ASSERT_EQ(queue.GetNumMetrics(), NUM_TEST_METRICS - 1);
        ASSERT_EQ(queue.GetSizeInBytes(), sizeOfEachMetrics * (NUM_TEST_METRICS - 1));
        ASSERT_EQ(anotherQueue.GetNumMetrics(), 1);
        ASSERT_EQ(anotherQueue.GetSizeInBytes(), sizeOfEachMetrics);

        // Records count limit is hit.
        queue.PopBufferedEventsByServiceLimits(anotherQueue, sizeOfEachMetrics * NUM_TEST_METRICS, 1);

        ASSERT_EQ(queue.GetNumMetrics(), NUM_TEST_METRICS - 2);
        ASSERT_EQ(queue.GetSizeInBytes(), sizeOfEachMetrics * (NUM_TEST_METRICS - 2));
        ASSERT_EQ(anotherQueue.GetNumMetrics(), 2);
        ASSERT_EQ(anotherQueue.GetSizeInBytes(), sizeOfEachMetrics * 2);
    }
}

