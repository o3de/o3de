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

#include <Editor/Attribution/AWSCoreAttributionManager.h>
#include <Editor/Attribution/AWSCoreAttributionMetric.h>
#include <AzCore/std/string/string.h>
#include <AzCore/PlatformId/PlatformId.h>

namespace AWSCore
{
    void AWSAttributionManager::Init()
    {
        // TODO: Perform any setup
    }

    void AWSAttributionManager::MetricCheck()
    {
        if (ShouldGenerateMetric())
        {
            // 1. Gather metadata and assemble metric
            AttributionMetric metric;
            UpdateMetric(metric);

            // 2. Identify region and chose attribution endpoint
            
            // 3. Post metric
        }
        UpdateLastCheck();
    }

    bool AWSAttributionManager::ShouldGenerateMetric() const
    {
        // 1. Check settings reg/config to see if metric emission is enabled
        // 2. Check last check time
        return false;
    }

    void AWSAttributionManager::UpdateLastCheck()
    {
        // Update registry setting about time check
    }

    AZStd::string AWSAttributionManager::GetEngineVersion() const
    {
        // Read from engine.json
        return "";
    }

    AZStd::string AWSAttributionManager::GetPlatform() const
    {
        return AZ::GetPlatformName(AZ::g_currentPlatform);
    }

    void AWSAttributionManager::GetActiveAWSGems(AZStd::vector<AZStd::string>& gems)
    {
        // Read from project's runtimedependencies.cmake?
        AZ_UNUSED(gems);
    }

    void AWSAttributionManager::UpdateMetric(AttributionMetric& metric)
    {
        AZStd::string engineVersion = this->GetEngineVersion();
        metric.SetO3DEVersion(engineVersion);

        AZStd::string platform = this->GetPlatform();
        metric.SetPlatform(platform, "");
        // etc.
    }

    void AWSAttributionManager::SubmitMetric(AttributionMetric& metric)
    {
        // Submit metric
        AZ_UNUSED(metric);
    }

} // namespace AWSCore
