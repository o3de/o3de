/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AWSMetricsBus.h>
#include <AWSMetricsConstant.h>
#include <AWSMetricsServiceApi.h>
#include <ClientConfiguration.h>
#include <DefaultClientIdProvider.h>
#include <MetricsEvent.h>
#include <MetricsEventBuilder.h>
#include <MetricsManager.h>

#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AWSMetrics
{
    MetricsManager::MetricsManager()
        : m_clientConfiguration(AZStd::make_unique<ClientConfiguration>())
        , m_clientIdProvider(IdentityProvider::CreateIdentityProvider())
        , m_monitorTerminated(true)
        , m_sendMetricsId(0)
    {
    }

    MetricsManager::~MetricsManager()
    {
        ShutdownMetrics();
    }

    bool MetricsManager::Init()
    {
        if (!m_clientConfiguration->InitClientConfiguration())
        {
            return false;
        }

        SetupJobContext();

        return true;
    }

    void MetricsManager::StartMetrics()
    {
        if (!m_monitorTerminated)
        {
            // The background thread has been started.
            return;
        }
        m_monitorTerminated = false;

        // Start a separate thread to monitor and consume the metrics queue.
        // Avoid using the job system since the worker is long-running over multiple frames
        m_monitorThread = AZStd::thread(AZStd::bind(&MetricsManager::MonitorMetricsQueue, this));
    }

    void MetricsManager::MonitorMetricsQueue()
    {
        // Continue to loop until the monitor is terminated.
        while (!m_monitorTerminated)
        {
            // The thread will wake up either when the metrics event queue is full (try_acquire_for call returns true),
            // or the flush period limit is hit (try_acquire_for call returns false).
            m_waitEvent.try_acquire_for(AZStd::chrono::seconds(m_clientConfiguration->GetQueueFlushPeriodInSeconds()));
            FlushMetricsAsync();
        }
    }

    void MetricsManager::SetupJobContext()
    {
        // Avoid using the default job context since we will do blocking IO instead of CPU/memory intensive work
        unsigned int numWorkerThreads = AZ::GetMin(DesiredMaxWorkers, AZStd::thread::hardware_concurrency());

        AZ::JobManagerDesc jobDesc;
        AZ::JobManagerThreadDesc threadDesc;
        for (unsigned int i = 0; i < numWorkerThreads; ++i)
        {
            jobDesc.m_workerThreads.push_back(threadDesc);
        }

        m_jobManager.reset(aznew AZ::JobManager{ jobDesc });
        m_jobContext.reset(aznew AZ::JobContext{ *m_jobManager });
    }

    bool MetricsManager::SubmitMetrics(const AZStd::vector<MetricsAttribute>& metricsAttributes, int eventPriority, const AZStd::string& eventSourceOverride)
    {
        MetricsEvent metricsEvent = MetricsEventBuilder().
            AddDefaultMetricsAttributes(m_clientIdProvider->GetIdentifier(), eventSourceOverride).
            AddMetricsAttributes(metricsAttributes).
            SetMetricsPriority(eventPriority).
            Build();

        if (!metricsEvent.ValidateAgainstSchema())
        {
            m_globalStats.m_numDropped++;
            return false;
        }

        AZStd::lock_guard<AZStd::mutex> lock(m_metricsMutex);
        m_metricsQueue.AddMetrics(metricsEvent);

        if (m_metricsQueue.GetSizeInBytes() >= static_cast<size_t>(m_clientConfiguration->GetMaxQueueSizeInBytes()))
        {
            // Flush the metrics queue when the accumulated metrics size hits the limit
            m_waitEvent.release();
        }

        return true;
    }

    bool MetricsManager::SendMetricsAsync(const AZStd::vector<MetricsAttribute>& metricsAttributes, int eventPriority, const AZStd::string & eventSourceOverride)
    {
        MetricsEvent metricsEvent = MetricsEventBuilder().
            AddDefaultMetricsAttributes(m_clientIdProvider->GetIdentifier(), eventSourceOverride).
            AddMetricsAttributes(metricsAttributes).
            SetMetricsPriority(eventPriority).
            Build();

        if (!metricsEvent.ValidateAgainstSchema())
        {
            m_globalStats.m_numDropped++;
            return false;
        }

        auto metricsToFlush = AZStd::make_shared<MetricsQueue>();
        metricsToFlush->AddMetrics(metricsEvent);

        SendMetricsAsync(metricsToFlush);
        return true;
    }

    void MetricsManager::SendMetricsAsync(AZStd::shared_ptr<MetricsQueue> metricsQueue)
    {
        if (m_clientConfiguration->OfflineRecordingEnabled())
        {
            SendMetricsToLocalFileAsync(metricsQueue);
        }
        else
        {
            // Constant used to convert size limit from MB to Bytes.
            static constexpr int MbToBytes = 1000000;

            while (metricsQueue->GetNumMetrics() > 0)
            {
                // Break the metrics queue by the payload and records count limits. Make one or more service API requests to send all the buffered metrics.
                MetricsQueue metricsEventsToProcess;
                metricsQueue->PopBufferedEventsByServiceLimits(metricsEventsToProcess, AwsMetricsMaxRestApiPayloadSizeInMb * MbToBytes, AwsMetricsMaxKinesisBatchedRecordCount);

                SendMetricsToServiceApiAsync(metricsEventsToProcess);
            }
        }
    }

    void MetricsManager::SendMetricsToLocalFileAsync(AZStd::shared_ptr<MetricsQueue> metricsQueue)
    {
        int requestId = ++m_sendMetricsId;

        // Send metrics to a local file
        AZ::Job* job{nullptr};
        job = AZ::CreateJobFunction(
            [this, metricsQueue, requestId]()
            {
                AZ::Outcome<void, AZStd::string> outcome = SendMetricsToFile(metricsQueue);

                if (outcome.IsSuccess())
                {
                    // Generate response records for success call to keep consistency with the Service API response
                    ServiceAPI::MetricsEventSuccessResponsePropertyEvents responseRecords;
                    int numMetricsEventsInRequest = metricsQueue->GetNumMetrics();
                    for (int index = 0; index < numMetricsEventsInRequest; ++index)
                    {
                        ServiceAPI::MetricsEventSuccessResponseRecord responseRecord;
                        responseRecord.result = AwsMetricsSuccessResponseRecordResult;

                        responseRecords.emplace_back(responseRecord);
                    }

                    OnResponseReceived(*metricsQueue, responseRecords);

                    AZ::TickBus::QueueFunction([requestId]()
                    {
                        AWSMetricsNotificationBus::Broadcast(&AWSMetricsNotifications::OnSendMetricsSuccess, requestId);
                    });
                }
                else
                {
                    OnResponseReceived(*metricsQueue);

                    AZStd::string errorMessage = outcome.GetError();
                    AZ::TickBus::QueueFunction([requestId, errorMessage]()
                    {
                        AWSMetricsNotificationBus::Broadcast(&AWSMetricsNotifications::OnSendMetricsFailure, requestId, errorMessage);
                    });
                }
            },
            true, m_jobContext.get());

        job->Start();
    }

    void MetricsManager::SendMetricsToServiceApiAsync(const MetricsQueue& metricsQueue)
    {
        int requestId = ++m_sendMetricsId;

        ServiceAPI::PostProducerEventsRequestJob* requestJob = ServiceAPI::PostProducerEventsRequestJob::Create(
            [this, requestId](ServiceAPI::PostProducerEventsRequestJob* successJob)
            {
                OnResponseReceived(successJob->parameters.data, successJob->result.events);

                AZ::TickBus::QueueFunction([requestId]()
                {
                    AWSMetricsNotificationBus::Broadcast(&AWSMetricsNotifications::OnSendMetricsSuccess, requestId);
                });
            },
            [this, requestId](ServiceAPI::PostProducerEventsRequestJob* failedJob)
            {
                OnResponseReceived(failedJob->parameters.data);

                AZStd::string errorMessage = failedJob->error.message;
                AZ::TickBus::QueueFunction([requestId, errorMessage]()
                {
                    AWSMetricsNotificationBus::Broadcast(&AWSMetricsNotifications::OnSendMetricsFailure, requestId, errorMessage);
                });
            });

        requestJob->parameters.data = AZStd::move(metricsQueue);
        requestJob->Start();
    }

    void MetricsManager::OnResponseReceived(const MetricsQueue& metricsEventsInRequest, const ServiceAPI::MetricsEventSuccessResponsePropertyEvents& responseRecords)
    {
        MetricsQueue metricsEventsForRetry;
        int numMetricsEventsInRequest = metricsEventsInRequest.GetNumMetrics();
        for (int index = 0; index < numMetricsEventsInRequest; ++index)
        {
            MetricsEvent metricsEvent = metricsEventsInRequest[index];

            if (responseRecords.size() > 0 && responseRecords[index].result == AwsMetricsSuccessResponseRecordResult)
            {
                // The metrics event is sent to the backend successfully.
                if (metricsEvent.GetNumFailures() == 0)
                {
                    m_globalStats.m_numEvents++;
                }
                else
                {
                    // Reduce the number of errors when the retry succeeds.
                    m_globalStats.m_numErrors--;
                }

                m_globalStats.m_numSuccesses++;
                m_globalStats.m_sendSizeInBytes += static_cast<uint32_t>(metricsEvent.GetSizeInBytes());
            }
            else
            {
                metricsEvent.MarkFailedSubmission();

                // The metrics event failed to be sent to the backend for the first time.
                if (metricsEvent.GetNumFailures() == 1)
                {
                    m_globalStats.m_numErrors++;
                    m_globalStats.m_numEvents++;
                }

                if (metricsEvent.GetNumFailures() <= m_clientConfiguration->GetMaxNumRetries())
                {
                    metricsEventsForRetry.AddMetrics(metricsEvent);
                }
                else
                {
                    m_globalStats.m_numDropped++;
                }
            }
        }

        PushMetricsForRetry(metricsEventsForRetry);
    }

    void MetricsManager::PushMetricsForRetry(MetricsQueue& metricsEventsForRetry)
    {
        if (m_clientConfiguration->GetMaxNumRetries() == 0)
        {
            // No retry is required.
            m_globalStats.m_numDropped += metricsEventsForRetry.GetNumMetrics();
            return;
        }

        // Push failed events to the front of the queue and reserve the order.
        AZStd::lock_guard<AZStd::mutex> lock(m_metricsMutex);
        m_metricsQueue.PushMetricsToFront(metricsEventsForRetry);

        // Filter metrics events by priority since the queue might be full.
        m_globalStats.m_numDropped += m_metricsQueue.FilterMetricsByPriority(m_clientConfiguration->GetMaxQueueSizeInBytes());
    }

    AZ::Outcome<void, AZStd::string> MetricsManager::SendMetricsToFile(AZStd::shared_ptr<MetricsQueue> metricsQueue)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_metricsFileMutex);

        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetDirectInstance();
        if (!fileIO)
        {
            return AZ::Failure(AZStd::string{ "No FileIoBase Instance." });
        }

        const char* metricsFileFullPath = m_clientConfiguration->GetMetricsFileFullPath();
        const char* metricsFileDir = m_clientConfiguration->GetMetricsFileDir();
        MetricsQueue existingMetricsEvents;
        if (!metricsFileFullPath || !metricsFileDir)
        {
            return AZ::Failure(AZStd::string{ "Failed to get the metrics file directory or path." });
        }
        if (fileIO->Exists(metricsFileFullPath) && !existingMetricsEvents.ReadFromJson(metricsFileFullPath))
        {
            return AZ::Failure(AZStd::string{ "Failed to read the existing metrics on disk" });
        }
        else if (!fileIO->Exists(metricsFileDir) && !fileIO->CreatePath(metricsFileDir))
        {
            return AZ::Failure(AZStd::string{ "Failed to create metrics directory" });
        }

        // Append a copy of the metrics queue in the request to the existing metrics events and keep the original submission order.
        // Do not modify the metrics queue in the request directly for identifying the metrics events for retry on failure.
        MetricsQueue metricsEventsInRequest = *metricsQueue;
        existingMetricsEvents.AppendMetrics(metricsEventsInRequest);
        AZStd::string serializedMetrics = existingMetricsEvents.SerializeToJson();

        AZ::IO::HandleType fileHandle;
        if (!fileIO->Open(metricsFileFullPath, AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeText, fileHandle))
        {
            return AZ::Failure(AZStd::string{ "Failed to open metrics file" });
        }
        fileIO->Write(fileHandle, serializedMetrics.c_str(), serializedMetrics.size());
        fileIO->Close(fileHandle);

        return AZ::Success();
    }

    void MetricsManager::FlushMetricsAsync()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_metricsMutex);
        if (m_metricsQueue.GetNumMetrics() == 0)
        {
            return;
        }

        auto metricsToFlush = AZStd::make_shared<MetricsQueue>();
        metricsToFlush->AppendMetrics(m_metricsQueue);
        m_metricsQueue.ClearMetrics();

        SendMetricsAsync(metricsToFlush);
    }

    void MetricsManager::ShutdownMetrics()
    {
        if (m_monitorTerminated)
        {
            return;
        }

        // Terminate the monitor thread
        m_monitorTerminated = true;
        m_waitEvent.release();

        if (m_monitorThread.joinable())
        {
            m_monitorThread.join();
        }
    }

    AZ::s64 MetricsManager::GetNumBufferedMetrics()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_metricsMutex);
        return m_metricsQueue.GetNumMetrics();
    }

    const GlobalStatistics& MetricsManager::GetGlobalStatistics() const
    {
        return m_globalStats;
    }

    void MetricsManager::UpdateOfflineRecordingStatus(bool enable, bool submitLocalMetrics)
    {
        m_clientConfiguration->UpdateOfflineRecordingStatus(enable);

        if (!enable && submitLocalMetrics)
        {
            SubmitLocalMetricsAsync();
        }       
    }

    void MetricsManager::SubmitLocalMetricsAsync()
    {
        AZ::Job* job{ nullptr };
        job = AZ::CreateJobFunction([this]()
        {
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetDirectInstance();
            if (!fileIO)
            {
                AZ_Error("AWSMetrics", false, "No FileIoBase Instance.");
                return;
            }

            // Read metrics from the local metrics file.
            AZStd::lock_guard<AZStd::mutex> file_lock(m_metricsFileMutex);

            if (!fileIO->Exists(GetMetricsFilePath()))
            {
                // Local metrics file doesn't exist.
                return;
            }

            MetricsQueue offlineRecords;
            if (!offlineRecords.ReadFromJson(GetMetricsFilePath()))
            {
                AZ_Error("AWSMetrics", false, "Failed to read from the local metrics file %s", GetMetricsFilePath());
                return;
            }

            // Submit the metrics read from the local metrics file.
            int numOfflineRecords = offlineRecords.GetNumMetrics();
            for (int index = 0; index < numOfflineRecords; ++index)
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_metricsMutex);
                m_metricsQueue.AddMetrics(offlineRecords[index]);

                if (m_metricsQueue.GetSizeInBytes() >= static_cast<size_t>(m_clientConfiguration->GetMaxQueueSizeInBytes()))
                {
                    // Flush the metrics queue when the accumulated metrics size hits the limit
                    m_waitEvent.release();
                }
            }

            // Remove the local metrics file after reading all its content.
            if (!fileIO->Remove(GetMetricsFilePath()))
            {
                AZ_Error("AWSMetrics", false, "Failed to remove the local metrics file %s", GetMetricsFilePath());
                return;
            }
        }, true, m_jobContext.get());

        job->Start();
    }

    const char* MetricsManager::GetMetricsFileDirectory() const
    {
        return m_clientConfiguration->GetMetricsFileDir();
    }

    const char* MetricsManager::GetMetricsFilePath() const
    {
        return m_clientConfiguration->GetMetricsFileFullPath();
    }

    int MetricsManager::GetNumTotalRequests() const
    {
        return m_sendMetricsId.load();
    }
}
