/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GlobalStatistics.h>
#include <MetricsQueue.h>
#include <IdentityProvider.h>
#include <AWSMetricsServiceApi.h>

#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/parallel/mutex.h>

namespace AWSMetrics
{
    class ClientConfiguration;

    //! Metrics manager handles direct or batch sending metrics to backend
    class MetricsManager
    {
    public:
        static const unsigned int DesiredMaxWorkers = 2;

        MetricsManager();
        virtual ~MetricsManager();

        //! Initializing the metrics manager
        //! @return Whether the operation is successful.
        bool Init();
        //! Start sending metircs to the backend or a local file.
        void StartMetrics();
        //! Stop sending metircs to the backend or a local file.
        void ShutdownMetrics();

        //! Implementation for submitting metrics.
        //! Metrics will be buffered before sending in batch.
        //! @param metricsAttributes Attributes of the metrics.
        //! @param eventPriority Priority of the event. Default to 0 which is considered as the highest priority.
        //! @param eventSourceOverride Event source used to override the default value.
        //! @return Whether the submission is successful.
        bool SubmitMetrics(const AZStd::vector<MetricsAttribute>& metricsAttributes = AZStd::vector<MetricsAttribute>(),
            int eventPriority = 0, const AZStd::string& eventSourceOverride = "");

        //! Implementation for sending metrics asynchronously.
        //! @param metricsAttributes Attributes of the metrics.
        //! @param eventSourceOverride Event source used to override the default value.
        //! @param eventPriority Priority of the event. Default to 0 which is considered as the highest priority.
        //! @return Whether the request is sent successfully.
        bool SendMetricsAsync(const AZStd::vector<MetricsAttribute>& metricsAttributes = AZStd::vector<MetricsAttribute>(),
            int eventPriority = 0, const AZStd::string& eventSourceOverride = "");

        //! Update the global stats and add qualified failed metrics events back to the buffer for retry.
        //! @param metricsEventsInRequest Metrics events in the original request.
        //! @param responseRecords Response records from the call. Each record in the list contains the result for sending the corresponding metrics event.
        void OnResponseReceived(const MetricsQueue& metricsEventsInRequest, const ServiceAPI::MetricsEventSuccessResponsePropertyEvents& responseRecords =
                ServiceAPI::MetricsEventSuccessResponsePropertyEvents());

        //! Implementation for flush all metrics buffered in memory.
        void FlushMetricsAsync();

        //! Get the total number of metrics buffered in the metrics queue.
        //! @return the number of buffered metrics
        AZ::s64 GetNumBufferedMetrics();

        //! Retrieve the global statistics for sending metrics.
        //! @return Global statistics for sending metrics
        const GlobalStatistics& GetGlobalStatistics() const;

        //! Enable/Disable the offline recording and resubmit metrics stored in the local metrics file if the client switches to the online mode.
        //! @param enable Whether to enable the offline recording.
        //! @param submitLocalMetrics Whether to resubmit local metrics if exist.
        void UpdateOfflineRecordingStatus(bool enable, bool submitLocalMetrics = false);

        //! Get the directory of the local metrics file.
        //! @return Directory of the local metrics file.
        const char* GetMetricsFileDirectory() const;

        //! Get the path to the local metrics file.
        //! @return Path to the local metrics file.
        const char* GetMetricsFilePath() const;

        //! Get the total number of requests for sending metrics events.
        //! This value could be different to the number of submitted metrics events since metrics events could be sent in batch.
        //! @return Total number of requests for sending metrics events.
        int GetNumTotalRequests() const;

    protected:
        //! Send metrics to a local file.
        //! @param metricsQueue metricsQueue Metrics queue that stores the metrics.
        //! @return Outcome of the operation.
        virtual AZ::Outcome<void, AZStd::string> SendMetricsToFile(AZStd::shared_ptr<MetricsQueue> metricsQueue);

    private:
        //! Job management
        void SetupJobContext();

        //! Monitor the buffered metrics queue and consume metrics when the size or elapsed time limit is hit.
        void MonitorMetricsQueue();

        //! Send metrics to a local file or the backend service asynchronously.
        //! @param metricsQueue Metrics queue that stores the metrics.
        void SendMetricsAsync(AZStd::shared_ptr<MetricsQueue> metricsQueue);

        //! Send the batched metrics events to the local metrics file asynchronously.
        //! @param metricsQueue Metrics events to send.
        void SendMetricsToLocalFileAsync(AZStd::shared_ptr<MetricsQueue> metricsQueue);

        //! Send the batched metrics events to the Service API asynchronously.
        //! @param metricsQueue Metrics events to send.
        void SendMetricsToServiceApiAsync(const MetricsQueue& metricsQueue);

        //! Push metrics events to the front of the queue for retry.
        //! @param metricsEventsForRetry Metrics events for retry.
        void PushMetricsForRetry(MetricsQueue& metricsEventsForRetry);

        void SubmitLocalMetricsAsync();

        AZStd::mutex m_metricsMutex; //!< Mutex to protect the metrics queue
        MetricsQueue m_metricsQueue; //!< Queue fo buffering the metrics events

        AZStd::mutex m_metricsFileMutex; //!< Mutex to protect the local metrics file

        AZStd::atomic<int> m_sendMetricsId;//!< Request ID for sending metrics

        AZStd::thread m_monitorThread; //!< Thread to monitor and consume the metrics queue
        AZStd::atomic<bool> m_monitorTerminated;
        AZStd::binary_semaphore m_waitEvent;

        // Client Configurations.
        AZStd::unique_ptr<ClientConfiguration> m_clientConfiguration;

        AZStd::unique_ptr<AZ::JobManager> m_jobManager{ nullptr };
        AZStd::unique_ptr <AZ::JobContext> m_jobContext{ nullptr };

        AZStd::unique_ptr<AZ::SettingsRegistryInterface> m_settingsRegistry;

        AZStd::unique_ptr<IdentityProvider> m_clientIdProvider;

        GlobalStatistics m_globalStats;
    };
}
