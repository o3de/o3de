/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/OpenMode.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Date/DateFormat.h>

#include "PerformanceCollector.h"


namespace AZ::Debug
{
    PerformanceCollector::PerformanceCollector(
        const AZStd::string_view logCategory,
        AZStd::span<const AZStd::string_view> m_metricNames,
        OnBatchCompleteCallback onBatchCompleteCallback,
        const AZStd::string_view fileExtension)
        : m_logCategory(logCategory)
        , m_onBatchCompleteCallback(onBatchCompleteCallback)
        , m_fileExtension(fileExtension)
    {
        for (const auto& metricName : m_metricNames)
        {
            [[maybe_unused]] const auto statisticPtr = m_statisticsManager.AddStatistic(metricName, metricName, "us");
            AZ_Assert(statisticPtr, "Failed to add metric with name <%.*s>. Maybe already added?", AZ_STRING_ARG(metricName));
        }
        RestartPeriodicEventStamps();
    }

    bool PerformanceCollector::IsWaitingBeforeCapture()
    {
        return !m_numberOfCaptureBatches || m_isWaitingBeforeNextBatch;
    }

    void PerformanceCollector::FrameTick()
    {
        if (!m_numberOfCaptureBatches)
        {
            return;
        }

        if (m_isWaitingBeforeNextBatch)
        {
            if (m_waitTimeBeforeEachBatch.count() > 0)
            {
                AZStd::chrono::steady_clock::time_point now = AZStd::chrono::steady_clock::now();
                if (m_startWaitTime.time_since_epoch().count() == 0)
                {
                    AZ_Trace(LogName, "Will Wait %lld seconds before starting batch %u...\n", m_waitTimeBeforeEachBatch.count(), m_numberOfCaptureBatches);
                    m_startWaitTime = now;
                }
                auto duration = AZStd::chrono::duration_cast<AZStd::chrono::seconds>(now - m_startWaitTime);
                if (duration < m_waitTimeBeforeEachBatch)
                {
                    return; // Do nothing. Keep waiting.
                }
            }
            AZ_Trace(LogName, "Waited %lld seconds. Will start collecting performance numbers for %u frames at batch %u...\n", m_waitTimeBeforeEachBatch.count(), m_frameCountPerCaptureBatch, m_numberOfCaptureBatches);
            m_isWaitingBeforeNextBatch = false;
        }

        m_frameCountInCurrentBatch++;
        if (m_frameCountInCurrentBatch <= m_frameCountPerCaptureBatch)
        {
            // All good nothing to do.
            return;
        }

        // Current batch is complete.
        m_numberOfCaptureBatches--;
        m_isWaitingBeforeNextBatch = true;
        m_frameCountInCurrentBatch = 0;
        m_startWaitTime = {};

        if (m_dataLogType == DataLogType::LogStatistics)
        {
            //It is time to write the statistical summaries to the Log file.
            RecordStatistics();
            m_statisticsManager.ResetAllStatistics();
        }
        RestartPeriodicEventStamps();

        m_onBatchCompleteCallback(m_numberOfCaptureBatches);
        if (!m_numberOfCaptureBatches)
        {
            // This will close the file that contains performance results for all batches.
            m_eventLogger.ResetStream(nullptr);
            AZ_Info(LogName, "Performance data output file <%s> is ready\n", m_outputFilePath.c_str());
        }

    }

    void PerformanceCollector::RecordSample(AZStd::string_view metricName, AZStd::chrono::microseconds microSeconds)
    {
        if (m_dataLogType == DataLogType::LogStatistics)
        {
            m_statisticsManager.PushSampleForStatistic(metricName, aznumeric_caster(microSeconds.count()));
        }
        else
        {
            Metrics::CompleteArgs completeArgs;
            completeArgs.m_name = metricName;
            completeArgs.m_cat = m_logCategory;
            completeArgs.m_dur = microSeconds;
            m_eventLogger.RecordCompleteEvent(completeArgs);
        }
    }

    void PerformanceCollector::RecordPeriodicEvent(AZStd::string_view metricName)
    {
        if (IsWaitingBeforeCapture())
        {
            return;
        }
        auto prevStamp = m_periodicEventStamps[metricName];
        AZStd::chrono::steady_clock::time_point now = AZStd::chrono::steady_clock::now();
        m_periodicEventStamps[metricName] = now;
        if (prevStamp.time_since_epoch().count() == 0)
        {
            return;
        }
        auto duration = AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(now - prevStamp);
        RecordSample(metricName, duration);
    }

    void PerformanceCollector::RecordStatistics()
    {
        AZStd::vector<Statistics::NamedRunningStatistic*> statistics;
        m_statisticsManager.GetAllStatistics(statistics);
        AZ_Warning(LogName, !statistics.empty(), "There are no statistics to report.");
        for (const auto statistic : statistics)
        {
            using EventObjectStorage = AZStd::fixed_vector<AZ::Metrics::EventField, 8>;
            EventObjectStorage statisticalParams;
            statisticalParams.emplace_back(AVG, statistic->GetAverage());
            statisticalParams.emplace_back(MIN, statistic->GetMinimum());
            statisticalParams.emplace_back(MAX, statistic->GetMaximum());
            statisticalParams.emplace_back(SAMPLE_COUNT, statistic->GetNumSamples());
            statisticalParams.emplace_back(UNITS, statistic->GetUnits());
            statisticalParams.emplace_back(VARIANCE, statistic->GetVariance());
            statisticalParams.emplace_back(STDEV, statistic->GetStdev());
            statisticalParams.emplace_back(MOST_RECENT_SAMPLE, statistic->GetMostRecentSample());

            Metrics::CompleteArgs completeArgs;
            completeArgs.m_name = statistic->GetName();
            completeArgs.m_cat = m_logCategory;
            completeArgs.m_dur = AZStd::chrono::microseconds(aznumeric_cast<long long>(statistic->GetAverage()));
            completeArgs.m_args = statisticalParams;
            m_eventLogger.RecordCompleteEvent(completeArgs);
        }
    }

    void PerformanceCollector::RestartPeriodicEventStamps()
    {
        AZStd::vector<Statistics::NamedRunningStatistic*> statistics;
        m_statisticsManager.GetAllStatistics(statistics);
        m_periodicEventStamps.clear();
        for (auto statistic : statistics)
        {
            m_periodicEventStamps[statistic->GetName()] = {};
        }
    }

    void PerformanceCollector::UpdateDataLogType(DataLogType newValue)
    {
        if (m_dataLogType == newValue)
        {
            return;
        }

        if (m_numberOfCaptureBatches > 0)
        {
            AZ_Warning(LogName, false, "%s changes to control params are rejected while data is being captured.", __FUNCTION__);
            return;
        }

        m_dataLogType = newValue;
    }

    void PerformanceCollector::UpdateFrameCountPerCaptureBatch(AZ::u32 newValue)
    {
        if (m_frameCountPerCaptureBatch == newValue)
        {
            return;
        }

        if (m_numberOfCaptureBatches > 0)
        {
            AZ_Warning(LogName, false, "%s changes to control params are rejected while data is being captured.", __FUNCTION__);
            return;
        }

        m_frameCountPerCaptureBatch = newValue;
    }

    void PerformanceCollector::UpdateWaitTimeBeforeEachBatch(AZStd::chrono::seconds seconds)
    {
        if (m_waitTimeBeforeEachBatch.count() == seconds.count())
        {
            return;
        }

        if (m_numberOfCaptureBatches > 0)
        {
            AZ_Warning(LogName, false, "%s changes to control params are rejected while data is being captured.", __FUNCTION__);
            return;
        }

        m_waitTimeBeforeEachBatch = seconds;
    }

    void PerformanceCollector::UpdateNumberOfCaptureBatches(AZ::u32 newValue)
    {
        if (m_numberOfCaptureBatches == newValue)
        {
            return;
        }
        if (m_numberOfCaptureBatches == 0)
        {
            CreateOutputJsonFile();
        }
        m_numberOfCaptureBatches = newValue;
        AZ_Trace(LogName, "%s updated value to %u\n", __FUNCTION__, m_numberOfCaptureBatches);
    }

    void PerformanceCollector::CreateOutputJsonFile()
    {
        m_outputFilePath.clear();
        m_outputDataBuffer.clear();

        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        if (!settingsRegistry)
        {
            // Most likely running under unit test. It is a good idea to not use File I/O during unit tests
            // if possible, as it reduces flaky errors with File I/O failures in Jenkins, etc. Also prevents
            // pollution of the filesystem.
            auto bufferStream = AZStd::make_unique<AZ::IO::ByteContainerStream<AZStd::string>>(&m_outputDataBuffer);
            m_eventLogger.ResetStream(AZStd::move(bufferStream));
        }
        else
        {
            settingsRegistry->Get(m_outputFilePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectUserPath);
            AZ::Date::Iso8601TimestampString utcTimestamp;
            AZ::Date::GetFilenameCompatibleFormatNowWithMicroseconds(utcTimestamp);
            m_outputFilePath /= AZStd::string::format("Performance_%s_%s.%s", m_logCategory.c_str(), utcTimestamp.c_str(), m_fileExtension.c_str());

            constexpr AZ::IO::OpenMode openMode = AZ::IO::OpenMode::ModeWrite;
            auto stream = AZStd::make_unique<AZ::IO::SystemFileStream>(m_outputFilePath.c_str(), openMode);
            m_eventLogger.ResetStream(AZStd::move(stream));
        }
    }

} //namespace AZ::Debug
