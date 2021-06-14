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

#pragma once
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <Editor/Attribution/AWSAttributionServiceApi.h>

namespace AWSCore
{
    //! Manages operational metrics for AWS gems
    class AWSAttributionManager
    {
    public:
        AWSAttributionManager();
        virtual ~AWSAttributionManager();

        //! Perform initialization
        void Init();

        //! Run metric check
        void MetricCheck();

    protected:
        virtual void SubmitMetric(AttributionMetric& metric);
        virtual void UpdateMetric(AttributionMetric& metric);
        void UpdateLastSend();
        void SetApiEndpointAndRegion(ServiceAPI::AWSAttributionRequestJob::Config* config);

    private:
        bool ShouldGenerateMetric() const;

        AZStd::string GetEngineVersion() const;
        AZStd::string GetPlatform() const;
        void GetActiveAWSGems(AZStd::vector<AZStd::string>& gemNames);

        void SaveSettingsRegistryFile();

        AZStd::unique_ptr<AZ::SettingsRegistryImpl> m_settingsRegistry;
    };

} // namespace AWSCore
