/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AWSMetricsBus.h>
#include <AWSMetricsGemMock.h>
#include <AWSMetricsConstant.h>
#include <ClientConfiguration.h>
#include <MetricsEvent.h>
#include <MetricsManager.h>

#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/string/conversions.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    namespace IO
    {
        class fileIOMock : public LocalFileIO
        {
        public:
            AZ_RTTI(fileIOMock, "{9F23EB93-917B-401F-AC91-63D85BADB102}", FileIOBase);
            AZ_CLASS_ALLOCATOR(fileIOMock, OSAllocator, 0);

            fileIOMock() = default;
            ~fileIOMock() = default;

            Result Open(const char* filePath, OpenMode mode, HandleType& fileHandle) override
            {
                AZ_UNUSED(filePath);
                AZ_UNUSED(mode);
                AZ_UNUSED(fileHandle);

                return Result(ResultCode::Success);
            }

            Result Close(HandleType fileHandle) override
            {
                AZ_UNUSED(fileHandle);

                return Result(ResultCode::Success);
            }

            Result Read(
                HandleType fileHandle, void* buffer, AZ::u64 size, bool failOnFewerThanSizeBytesRead = false,
                AZ::u64* bytesRead = nullptr) override
            {
                AZ_UNUSED(fileHandle);
                AZ_UNUSED(buffer);
                AZ_UNUSED(size);
                AZ_UNUSED(failOnFewerThanSizeBytesRead);
                AZ_UNUSED(bytesRead);

                return Result(ResultCode::Success);
            }

            Result Write(HandleType fileHandle, const void* buffer, AZ::u64 size, AZ::u64* bytesWritten = nullptr) override
            {
                AZ_UNUSED(fileHandle);
                AZ_UNUSED(buffer);
                AZ_UNUSED(size);
                AZ_UNUSED(bytesWritten);

                return Result(ResultCode::Success);
            }
        };
    } // namespace IO
}; // namespace AZ

namespace AWSMetrics
{
    class MetricsManagerMock
        : public MetricsManager
    {
    private:
        AZ::Outcome<void, AZStd::string> SendMetricsToFile(AZStd::shared_ptr<MetricsQueue> metricsQueue) override
        {
            if (AZ::IO::FileIOBase::GetInstance())
            {
                return AZ::Success();
            }
            else
            {
                return AZ::Failure(AZStd::string{ "Invalid File IO" });
            }
        }
    };

    class AWSMetricsNotificationBusMock
        : protected AWSMetricsNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(AWSMetricsNotificationBusMock, AZ::SystemAllocator, 0);

        AWSMetricsNotificationBusMock()
            : m_numSuccessNotification(0)
            , m_numFailureNotification(0)
        {
            AWSMetricsNotificationBus::Handler::BusConnect();
        }

        ~AWSMetricsNotificationBusMock()
        {
            AWSMetricsNotificationBus::Handler::BusDisconnect();
        }

        void OnSendMetricsSuccess(int requestId) override
        {
            AZ_UNUSED(requestId);

            ++m_numSuccessNotification;
        }

        void OnSendMetricsFailure(int requestId, const AZStd::string& errorMessage) override
        {
            AZ_UNUSED(requestId);
            AZ_UNUSED(errorMessage);

            ++m_numFailureNotification;
        }

        int m_numSuccessNotification;
        int m_numFailureNotification;
    };

    class MetricsManagerTest
        : public AWSMetricsGemAllocatorFixture
        , protected AWSMetricsRequestBus::Handler
    {
    public:
        // Size for each test metrics event will be 180 bytes.
        static constexpr int TestMetricsEventSizeInBytes = 180;
        static constexpr int MbToBytes = 1000000;
        static constexpr int DefaultFlushPeriodInSeconds = 1;
        static constexpr int MaxNumMetricsEvents = 10;

        static constexpr int SleepTimeForProcessingInMs = 100;
        // Timeout for metrics events processing in milliseconds.
        static constexpr int TimeoutForProcessingInMs = DefaultFlushPeriodInSeconds * MaxNumMetricsEvents * 1000;

        static const char* const AttrValue;

        void SetUp() override
        {
            AWSMetricsGemAllocatorFixture::SetUp();
            AWSMetricsRequestBus::Handler::BusConnect();

            m_metricsManager = AZStd::make_unique<MetricsManagerMock>();
            AZStd::string configFilePath = CreateClientConfigFile(true, (double) TestMetricsEventSizeInBytes / MbToBytes * 2, DefaultFlushPeriodInSeconds, 0);
            m_settingsRegistry->MergeSettingsFile(configFilePath, AZ::SettingsRegistryInterface::Format::JsonMergePatch, {});
            m_metricsManager->Init();

            ReplaceLocalFileIOWithMockIO();
        }

        void TearDown() override
        {
            RevertMockIOToLocalFileIO();

            RemoveFile(GetDefaultTestFilePath());

            m_metricsManager.reset();

            AWSMetricsRequestBus::Handler::BusDisconnect();
            AWSMetricsGemAllocatorFixture::TearDown();
        }

        void ResetClientConfig(bool offlineRecordingEnabled, double maxQueueSizeInMb, int queueFlushPeriodInSeconds, int MaxNumRetries)
        {
            RevertMockIOToLocalFileIO();

            AZStd::string configFilePath = CreateClientConfigFile(offlineRecordingEnabled, maxQueueSizeInMb, queueFlushPeriodInSeconds, MaxNumRetries);
            m_settingsRegistry->MergeSettingsFile(configFilePath, AZ::SettingsRegistryInterface::Format::JsonMergePatch, {});
            m_metricsManager->Init();

            ReplaceLocalFileIOWithMockIO();
        }

        void ReplaceLocalFileIOWithMockIO()
        {
            m_fileIOMock = aznew AZ::IO::fileIOMock();
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_fileIOMock);
        }

        void RevertMockIOToLocalFileIO()
        {
            AZ::IO::FileIOBase::SetInstance(nullptr);
            delete m_fileIOMock;
            AZ::IO::FileIOBase::SetInstance(m_localFileIO);
        }

        // AWSMetricsRequestBus interface implementation
        bool SubmitMetrics(const AZStd::vector<MetricsAttribute>& metricsAttributes, int eventPriority = 0,
            const AZStd::string& eventSourceOverride = "", bool bufferMetrics = true) override
        {
            if (bufferMetrics)
            {
                return m_metricsManager->SubmitMetrics(metricsAttributes, eventPriority, eventSourceOverride);
            }
            else
            {
                return m_metricsManager->SendMetricsAsync(metricsAttributes, eventPriority, eventSourceOverride);
            }
        };

        //! Flush all metrics buffered in memory.
        virtual void FlushMetrics() override
        {
            m_metricsManager->FlushMetricsAsync();
        };

        //! Wait for either timeout or all the expected metrics events are processed.
        void WaitForProcessing(int expectedNumProcessedEvents)
        {
            const GlobalStatistics& originalStats = m_metricsManager->GetGlobalStatistics();
            int currentNumProcessedEvents = originalStats.m_numEvents;

            int processingTime = 0;
            int numTotalRequests = 0;
            while (processingTime < TimeoutForProcessingInMs)
            {
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(SleepTimeForProcessingInMs));
                processingTime += SleepTimeForProcessingInMs;

                const GlobalStatistics& currentStats = m_metricsManager->GetGlobalStatistics();
                currentNumProcessedEvents = currentStats.m_numEvents;
                numTotalRequests = m_metricsManager->GetNumTotalRequests();

                if (currentNumProcessedEvents == expectedNumProcessedEvents)
                {
                    // All the expectd metrics events has been sent. Flush the tick bus queue until we get all the notifications.
                    AZ::TickBus::ExecuteQueuedEvents();
                    if (numTotalRequests ==
                        m_notifications.m_numSuccessNotification + m_notifications.m_numFailureNotification)
                    {
                        break;
                    }
                }
            }   
        }

        AZStd::unique_ptr<MetricsManagerMock> m_metricsManager;
        AWSMetricsNotificationBusMock m_notifications;

        AZ::IO::FileIOBase* m_fileIOMock;
    };

    const char* const MetricsManagerTest::AttrValue = "value";

    TEST_F(MetricsManagerTest, SubmitMetrics_MaxFlushPeriod_SendToLocalFile)
    {
        m_metricsManager->StartMetrics();
        
        AZStd::vector<MetricsAttribute> metricsAttributes;
        metricsAttributes.emplace_back(AZStd::move(MetricsAttribute(AwsMetricsAttributeKeyEventName, AttrValue)));

        bool result = false;
        AWSMetricsRequestBus::BroadcastResult(result, &AWSMetricsRequests::SubmitMetrics, metricsAttributes, 0, "", true);
        ASSERT_TRUE(result);

        WaitForProcessing(1);
        ASSERT_EQ(m_notifications.m_numSuccessNotification, 1);
        ASSERT_EQ(m_notifications.m_numFailureNotification, 0);
        ASSERT_EQ(m_metricsManager->GetNumBufferedMetrics(), 0);

        m_metricsManager->ShutdownMetrics();
    }

    TEST_F(MetricsManagerTest, SubmitMetrics_MaxQueueSize_SendToLocalFile)
    {
        // Reset the config file to change the max queue size and set a flush period larger than the timeout.
        ResetClientConfig(true, 0.0, (TimeoutForProcessingInMs + 1), 0);

        m_metricsManager->StartMetrics();

        AZStd::vector<MetricsAttribute> metricsAttributes;
        metricsAttributes.emplace_back(AZStd::move(MetricsAttribute(AwsMetricsAttributeKeyEventName, AttrValue)));

        bool result = false;
        AWSMetricsRequestBus::BroadcastResult(result, &AWSMetricsRequests::SubmitMetrics, metricsAttributes, 0, "", true);
        ASSERT_TRUE(result);

        WaitForProcessing(1);
        ASSERT_EQ(m_notifications.m_numSuccessNotification, 1);
        ASSERT_EQ(m_notifications.m_numFailureNotification, 0);
        ASSERT_EQ(m_metricsManager->GetNumBufferedMetrics(), 0);

        m_metricsManager->ShutdownMetrics();
    }

    TEST_F(MetricsManagerTest, SubmitMetricsFromMultipleThreads_WithAndWithoutBuffer_SendToLocalFile)
    {
        m_metricsManager->StartMetrics();

        AZStd::vector<AZStd::thread> producers;

        for (int index = 0; index < MaxNumMetricsEvents; ++index)
        {
            producers.emplace_back(AZStd::thread([index]()
            {
                AZStd::vector<MetricsAttribute> metricsAttributes;
                metricsAttributes.emplace_back(AZStd::move(MetricsAttribute(AwsMetricsAttributeKeyEventName, AttrValue)));

                bool result = false;
                // Submit metrics with or without buffer
                AWSMetricsRequestBus::BroadcastResult(result, &AWSMetricsRequests::SubmitMetrics, metricsAttributes, 0, "", index % 2 == 0);
                ASSERT_TRUE(result);
            }));
        }

        for (AZStd::thread& producer : producers) {
            producer.join();
        }

        // Flush the metrics queue to send all the remaining buffered metrics
        AWSMetricsRequestBus::Broadcast(&AWSMetricsRequests::FlushMetrics);

        WaitForProcessing(MaxNumMetricsEvents);
        const GlobalStatistics& stats = m_metricsManager->GetGlobalStatistics();
        EXPECT_EQ(stats.m_numEvents, MaxNumMetricsEvents);
        EXPECT_EQ(stats.m_numSuccesses, MaxNumMetricsEvents);
        EXPECT_EQ(stats.m_numErrors, 0);
        EXPECT_EQ(stats.m_sendSizeInBytes, TestMetricsEventSizeInBytes * MaxNumMetricsEvents);

        m_metricsManager->ShutdownMetrics();
    }

    TEST_F(MetricsManagerTest, SubmitMetrics_NoMetircsAttributes_Fail)
    {
        bool result = false;
        AWSMetricsRequestBus::BroadcastResult(result, &AWSMetricsRequests::SubmitMetrics, AZStd::vector<MetricsAttribute>(), 0, "", true);
        ASSERT_FALSE(result);

        ASSERT_EQ(m_metricsManager->GetNumBufferedMetrics(), 0);
    }

    TEST_F(MetricsManagerTest, SendMetricsAsync_NoMetircsAttributes_Fail)
    {
        bool result = false;
        AWSMetricsRequestBus::BroadcastResult(result, &AWSMetricsRequests::SubmitMetrics, AZStd::vector<MetricsAttribute>(), 0, "", false);
        ASSERT_FALSE(result);
    }

    TEST_F(MetricsManagerTest, SendMetricsAsync_InvalidFileIO_Fail)
    {
        AZ::IO::FileIOBase::SetInstance(nullptr);

        AZStd::vector<MetricsAttribute> metricsAttributes;
        metricsAttributes.emplace_back(AZStd::move(MetricsAttribute(AwsMetricsAttributeKeyEventName, AttrValue)));

        bool result = false;
        AWSMetricsRequestBus::BroadcastResult(result, &AWSMetricsRequests::SubmitMetrics, metricsAttributes, 0, "", false);
        ASSERT_TRUE(result);

        WaitForProcessing(1);
        ASSERT_EQ(m_notifications.m_numSuccessNotification, 0);
        ASSERT_EQ(m_notifications.m_numFailureNotification, 1);
    }

    TEST_F(MetricsManagerTest, FlushMetrics_NonEmptyQueue_Success)
    {
        ResetClientConfig(true, (double)TestMetricsEventSizeInBytes * (MaxNumMetricsEvents + 1) / MbToBytes,
            DefaultFlushPeriodInSeconds, 1);

        for (int index = 0; index < MaxNumMetricsEvents; ++index)
        {
            AZStd::vector<MetricsAttribute> metricsAttributes;
            metricsAttributes.emplace_back(AZStd::move(MetricsAttribute(AwsMetricsAttributeKeyEventName, AttrValue)));

            bool result = false;
            AWSMetricsRequestBus::BroadcastResult(result, &AWSMetricsRequests::SubmitMetrics, metricsAttributes, 0, "", true);
            ASSERT_TRUE(result);
        }
        ASSERT_EQ(m_metricsManager->GetNumBufferedMetrics(), MaxNumMetricsEvents);

        AWSMetricsRequestBus::Broadcast(&AWSMetricsRequests::FlushMetrics);

        WaitForProcessing(MaxNumMetricsEvents);
        ASSERT_EQ(m_notifications.m_numSuccessNotification, 1);
        ASSERT_EQ(m_notifications.m_numFailureNotification, 0);
        ASSERT_EQ(m_metricsManager->GetNumBufferedMetrics(), 0);
    }

    TEST_F(MetricsManagerTest, ResetOfflineRecordingStatus_ResubmitLocalMetrics_Success)
    {
        // Disable offline recording in the config file.
        ResetClientConfig(false, (double)TestMetricsEventSizeInBytes * 2 / MbToBytes, 0, 0);

        // Enable offline recording after initialize the metric manager.
        m_metricsManager->UpdateOfflineRecordingStatus(true);

        AZStd::vector<MetricsAttribute> metricsAttributes;
        metricsAttributes.emplace_back(AZStd::move(MetricsAttribute(AwsMetricsAttributeKeyEventName, AttrValue)));

        bool result = false;
        AWSMetricsRequestBus::BroadcastResult(result, &AWSMetricsRequests::SubmitMetrics, metricsAttributes, 0, "", false);
        ASSERT_TRUE(result);

        WaitForProcessing(1);
        ASSERT_EQ(m_notifications.m_numSuccessNotification, 1);
        ASSERT_EQ(m_notifications.m_numFailureNotification, 0);
        ASSERT_EQ(m_metricsManager->GetNumBufferedMetrics(), 0);

        RevertMockIOToLocalFileIO();
        AZStd::string localmetrics = "[{\"event_name\": \"test\"}]";
        ASSERT_TRUE(m_localFileIO->Exists(m_metricsManager->GetMetricsFileDirectory()) || m_localFileIO->CreatePath(m_metricsManager->GetMetricsFileDirectory()));
        ASSERT_TRUE(CreateFile(m_metricsManager->GetMetricsFilePath(), localmetrics));

        // Disable offline recording and resubmitted metrics stored in the local file.
        m_metricsManager->UpdateOfflineRecordingStatus(false, true);

        //! Wait for either timeout or the local metrics events are re-added to the buffer.
        int processingTime = 0;
        while (processingTime < TimeoutForProcessingInMs && m_localFileIO->Exists(m_metricsManager->GetMetricsFilePath()))
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(SleepTimeForProcessingInMs));
            processingTime += SleepTimeForProcessingInMs;
        }
       
        ASSERT_EQ(m_metricsManager->GetNumBufferedMetrics(), 1);
        ASSERT_FALSE(m_localFileIO->Exists(m_metricsManager->GetMetricsFilePath()));

        ReplaceLocalFileIOWithMockIO();
    }

    TEST_F(MetricsManagerTest, OnResponseReceived_WithResponseRecords_RetryFailedMetrics)
    {
        // Reset the config file to change the max queue size setting.
        ResetClientConfig(false, (double)TestMetricsEventSizeInBytes * (MaxNumMetricsEvents + 1) / MbToBytes,
            DefaultFlushPeriodInSeconds, 1);

        MetricsQueue metricsEvents;
        ServiceAPI::MetricsEventSuccessResponsePropertyEvents responseRecords;
        for (int index = 0; index < MaxNumMetricsEvents; ++index)
        {
            MetricsEvent newEvent;
            newEvent.AddAttribute(MetricsAttribute(AwsMetricsAttributeKeyEventName, AttrValue));

            metricsEvents.AddMetrics(newEvent);

            ServiceAPI::MetricsEventSuccessResponseRecord responseRecord;
            if (index % 2 == 0)
            {
                responseRecord.errorCode = "Error";
            }
            else
            {
                responseRecord.result = "Ok";
            }
            responseRecords.emplace_back(responseRecord);
        }

        m_metricsManager->OnResponseReceived(metricsEvents, responseRecords);

        const GlobalStatistics& stats = m_metricsManager->GetGlobalStatistics();
        EXPECT_EQ(stats.m_numEvents, MaxNumMetricsEvents);
        EXPECT_EQ(stats.m_numSuccesses, MaxNumMetricsEvents / 2);
        EXPECT_EQ(stats.m_numErrors, MaxNumMetricsEvents / 2);
        EXPECT_EQ(stats.m_numDropped, 0);

        int metricsEventSize = static_cast<int>(sizeof(AwsMetricsAttributeKeyEventName) - 1 + strlen(AttrValue));
        EXPECT_EQ(stats.m_sendSizeInBytes, metricsEventSize * MaxNumMetricsEvents / 2);

        ASSERT_EQ(m_metricsManager->GetNumBufferedMetrics(), MaxNumMetricsEvents / 2);
    }

    TEST_F(MetricsManagerTest, OnResponseReceived_NoResponseRecords_RetryAllMetrics)
    {
        // Reset the config file to change the max queue size setting.
        ResetClientConfig(false, (double)TestMetricsEventSizeInBytes * (MaxNumMetricsEvents + 1) / MbToBytes,
            DefaultFlushPeriodInSeconds, 1);

        MetricsQueue metricsEvents;
        for (int index = 0; index < MaxNumMetricsEvents; ++index)
        {
            MetricsEvent newEvent;
            newEvent.AddAttribute(MetricsAttribute(AwsMetricsAttributeKeyEventName, AttrValue));

            metricsEvents.AddMetrics(newEvent);
        }

        m_metricsManager->OnResponseReceived(metricsEvents);

        const GlobalStatistics& stats = m_metricsManager->GetGlobalStatistics();
        EXPECT_EQ(stats.m_numEvents, MaxNumMetricsEvents);
        EXPECT_EQ(stats.m_numSuccesses, 0);
        EXPECT_EQ(stats.m_sendSizeInBytes, 0);
        EXPECT_EQ(stats.m_numDropped, 0);

        ASSERT_EQ(m_metricsManager->GetNumBufferedMetrics(), MaxNumMetricsEvents);
    }

    TEST_F(MetricsManagerTest, OnResponseReceived_MaxNumRetires_DropMetrics)
    {
        // Reset the config file to change the max queue size setting.
        ResetClientConfig(false, (double)TestMetricsEventSizeInBytes * (MaxNumMetricsEvents + 1) / MbToBytes,
            DefaultFlushPeriodInSeconds, 1);

        MetricsQueue metricsEvents;
        for (int index = 0; index < MaxNumMetricsEvents; ++index)
        {
            MetricsEvent newMetricsEvent;

            // Number of failures exceeds the maximum number of retries setting.
            newMetricsEvent.MarkFailedSubmission();
            newMetricsEvent.MarkFailedSubmission();

            metricsEvents.AddMetrics(newMetricsEvent);
        }

        m_metricsManager->OnResponseReceived(metricsEvents);

        const GlobalStatistics& stats = m_metricsManager->GetGlobalStatistics();
        EXPECT_EQ(stats.m_numEvents, 0);
        EXPECT_EQ(stats.m_numSuccesses, 0);
        // Number of errors is expected to be 0 here since the test metrics events
        // have been retried for multiple times and we do not increase the total number of errors in this case.
        EXPECT_EQ(stats.m_numErrors, 0);
        EXPECT_EQ(stats.m_sendSizeInBytes, 0);
        EXPECT_EQ(stats.m_numDropped, MaxNumMetricsEvents);

        ASSERT_EQ(m_metricsManager->GetNumBufferedMetrics(), 0);
    }

    TEST_F(MetricsManagerTest, PushMetricsForRetries_NoRetry_DropMetrics)
    {
        // Reset the config file to change the max queue size setting.
        ResetClientConfig(false, (double)TestMetricsEventSizeInBytes * (MaxNumMetricsEvents + 1) / MbToBytes,
            DefaultFlushPeriodInSeconds, 0);

        MetricsQueue metricsEvents;
        for (int index = 0; index < MaxNumMetricsEvents; ++index)
        {
            metricsEvents.AddMetrics(MetricsEvent());
        }

        m_metricsManager->OnResponseReceived(metricsEvents);

        const GlobalStatistics& stats = m_metricsManager->GetGlobalStatistics();
        EXPECT_EQ(stats.m_numEvents, MaxNumMetricsEvents);
        EXPECT_EQ(stats.m_numSuccesses, 0);
        EXPECT_EQ(stats.m_numErrors, MaxNumMetricsEvents);
        EXPECT_EQ(stats.m_sendSizeInBytes, 0);
        EXPECT_EQ(stats.m_numDropped, MaxNumMetricsEvents);

        ASSERT_EQ(m_metricsManager->GetNumBufferedMetrics(), 0);
    }

    class ClientConfigurationTest
        : public AWSMetricsGemAllocatorFixture
    {
    public:
        const double DEFAULT_MAX_QUEUE_SIZE_IN_MB = 0.0004;
        const int DefaultFlushPeriodInSeconds = 1;
        const int DEFAULT_MAX_NUM_RETRIES = 1;

        void SetUp() override
        {
            AWSMetricsGemAllocatorFixture::SetUp();

            m_clientConfiguration = AZStd::make_unique<ClientConfiguration>();
        }

        AZStd::unique_ptr<ClientConfiguration> m_clientConfiguration;
    };

    TEST_F(ClientConfigurationTest, ResetClientConfiguration_ValidClientConfiguration_Success)
    {
        AZStd::string configFilePath = CreateClientConfigFile(true, DEFAULT_MAX_QUEUE_SIZE_IN_MB, DefaultFlushPeriodInSeconds, DEFAULT_MAX_NUM_RETRIES);
        m_settingsRegistry->MergeSettingsFile(configFilePath, AZ::SettingsRegistryInterface::Format::JsonMergePatch, {});
        ASSERT_TRUE(m_clientConfiguration->InitClientConfiguration());

        ASSERT_TRUE(m_clientConfiguration->OfflineRecordingEnabled());
        ASSERT_EQ(m_clientConfiguration->GetMaxQueueSizeInBytes(), DEFAULT_MAX_QUEUE_SIZE_IN_MB * 1000000);
        ASSERT_EQ(m_clientConfiguration->GetQueueFlushPeriodInSeconds(), DefaultFlushPeriodInSeconds);
        ASSERT_EQ(m_clientConfiguration->GetMaxNumRetries(), DEFAULT_MAX_NUM_RETRIES);

        char resolvedPath[AZ_MAX_PATH_LEN] = { 0 };
        ASSERT_TRUE(m_localFileIO->ResolvePath(AwsMetricsLocalFileDir, resolvedPath, AZ_MAX_PATH_LEN));
        AZStd::string expectedMetricsFilePath;
        ASSERT_TRUE(AzFramework::StringFunc::Path::Join(resolvedPath, AwsMetricsLocalFileName, expectedMetricsFilePath));

        ASSERT_EQ(strcmp(m_clientConfiguration->GetMetricsFileDir(), resolvedPath), 0);
        ASSERT_EQ(m_clientConfiguration->GetMetricsFileFullPath(), expectedMetricsFilePath);
    }
}
