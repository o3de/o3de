/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ClientConfiguration.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AWSMetrics
{
    // Initialize the settings based on the default values in awsMetricsClientConfiguration.setreg.
    ClientConfiguration::ClientConfiguration()
        : m_maxQueueSizeInMb(0.3)
        , m_queueFlushPeriodInSeconds(60)
        , m_offlineRecordingEnabled(false)
        , m_maxNumRetries(1)
    {
    }

    bool ClientConfiguration::InitClientConfiguration()
    {
        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        if (!settingsRegistry)
        {
            AZ_Warning("AWSMetrics", false, "Failed to load the setting registry");
            return false;
        }

        if (!settingsRegistry->Get(
                m_maxQueueSizeInMb,
                AZStd::string::format("%s%s", AZ::SettingsRegistryMergeUtils::OrganizationRootKey, AWSMetricsMaxQueueSizeInMbKey)))
        {
            AZ_Warning("AWSMetrics", false, "Failed to read the maximum queue size setting from the setting registry");
            return false;
        }

        if (!settingsRegistry->Get(
                m_queueFlushPeriodInSeconds,
                AZStd::string::format("%s%s", AZ::SettingsRegistryMergeUtils::OrganizationRootKey, AWSMetricsQueueFlushPeriodInSecondsKey)))
        {
            AZ_Warning("AWSMetrics", false, "Failed to read the queue flush period setting from the setting registry");
            return false;
        }

        bool enableOfflineRecording = false;
        if (!settingsRegistry->Get(
                enableOfflineRecording,
                AZStd::string::format("%s%s", AZ::SettingsRegistryMergeUtils::OrganizationRootKey, AWSMetricsOfflineRecordingEnabledKey)))
        {
            AZ_Warning("AWSMetrics", false, "Failed to read the submission target setting from the setting registry");
            return false;
        }
        m_offlineRecordingEnabled = enableOfflineRecording;

        if (!settingsRegistry->Get(
                m_maxNumRetries,
                AZStd::string::format("%s%s", AZ::SettingsRegistryMergeUtils::OrganizationRootKey, AWSMetricsMaxNumRetriesKey)))
        {
            AZ_Warning("AWSMetrics", false, "Failed to read the maximum number of retries setting in the configuration file");
            return false;
        }

        return ResolveMetricsFilePath();
    }

    bool ClientConfiguration::ResolveMetricsFilePath()
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetDirectInstance();
        if (!fileIO)
        {
            AZ_Error("AWSMetrics", false, "No FileIoBase Instance");
            return false;
        }

        char resolvedPath[AZ_MAX_PATH_LEN] = { 0 };
        if (!fileIO->ResolvePath(AwsMetricsLocalFileDir, resolvedPath, AZ_MAX_PATH_LEN))
        {
            AZ_Error("AWSMetrics", false, "Failed to resolve the metrics file directory");
            return false;
        }
        m_metricsDir = resolvedPath;

        if (!AzFramework::StringFunc::Path::Join(resolvedPath, AwsMetricsLocalFileName, m_metricsFilePath))
        {
            AZ_Error("AWSMetrics", false, "Failed to construct the metrics file path");
            return false;
        }

        return true;
    }

    AZ::s64 ClientConfiguration::GetMaxQueueSizeInBytes() const
    {
        return m_maxQueueSizeInMb * 1000000;
    }

    AZ::s64 ClientConfiguration::GetQueueFlushPeriodInSeconds() const
    {
        return m_queueFlushPeriodInSeconds;
    }

    bool ClientConfiguration::OfflineRecordingEnabled() const
    {
        return m_offlineRecordingEnabled;
    }

    AZ::s64 ClientConfiguration::GetMaxNumRetries() const
    {
        return m_maxNumRetries;
    }

    const char* ClientConfiguration::GetMetricsFileDir() const
    {
        return m_metricsDir.c_str();
    }

    const char* ClientConfiguration::GetMetricsFileFullPath() const
    {
        return m_metricsFilePath.c_str();
    }

    void ClientConfiguration::UpdateOfflineRecordingStatus(bool enable)
    {
        m_offlineRecordingEnabled = enable;
    }
}
