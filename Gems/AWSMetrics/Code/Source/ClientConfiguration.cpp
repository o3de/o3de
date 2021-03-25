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

#include <ClientConfiguration.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
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

    bool ClientConfiguration::ResetClientConfiguration(const AZStd::string& settingsRegistryPath)
    {
        AZStd::unique_ptr<AZ::SettingsRegistryInterface> settingsRegistry = AZStd::make_unique<AZ::SettingsRegistryImpl>();

        AZ_Printf("AWSMetrics", "Reset client settings using the confiugration file %s", settingsRegistryPath.c_str());
        if (!settingsRegistry->MergeSettingsFile(settingsRegistryPath, AZ::SettingsRegistryInterface::Format::JsonMergePatch))
        {
            AZ_Warning("AWSMetrics", false, "Failed to merge the configuration file");
            return false;
        }

        if (!settingsRegistry->Get(m_maxQueueSizeInMb, "/Amazon/Gems/AWSMetrics/MaxQueueSizeInMb"))
        {
            AZ_Warning("AWSMetrics", false, "Failed to read the maximum queue size setting in the configuration file");
            return false;
        }

        if (!settingsRegistry->Get(m_queueFlushPeriodInSeconds, "/Amazon/Gems/AWSMetrics/QueueFlushPeriodInSeconds"))
        {
            AZ_Warning("AWSMetrics", false, "Failed to read the queue flush period setting in the configuration file");
            return false;
        }

        bool enableOfflineRecording = false;
        if (!settingsRegistry->Get(enableOfflineRecording, "/Amazon/Gems/AWSMetrics/OfflineRecording"))
        {
            AZ_Warning("AWSMetrics", false, "Failed to read the submission target setting in the configuration file");
            return false;
        }
        m_offlineRecordingEnabled = enableOfflineRecording;

        if (!settingsRegistry->Get(m_maxNumRetries, "/Amazon/Gems/AWSMetrics/MaxNumRetries"))
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
        if (!fileIO->ResolvePath(metricsDir, resolvedPath, AZ_MAX_PATH_LEN))
        {
            AZ_Error("AWSMetrics", false, "Failed to resolve the metrics file directory");
            return false;
        }
        m_metricsDir = resolvedPath;

        if (!AzFramework::StringFunc::Path::Join(resolvedPath, metricsFileName, m_metricsFilePath))
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
