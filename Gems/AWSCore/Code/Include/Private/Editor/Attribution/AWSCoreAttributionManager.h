/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

#include <Editor/Attribution/AWSAttributionServiceApi.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>


namespace AZ
{
    class SettingsRegistryInterface;
}

namespace AWSCore
{
    //! Manages operational metrics for AWS gems
    class AWSAttributionManager
        : private AzToolsFramework::EditorEvents::Bus::Handler
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
        virtual void ShowConsentDialog();

    private:
        bool ShouldGenerateMetric() const;
        bool CheckAWSCredentialsConfigured();
        bool CheckConsentShown();
        AZStd::string GetEngineVersion() const;
        AZStd::string GetPlatform() const;
        void GetActiveAWSGems(AZStd::vector<AZStd::string>& gemNames);

        void SaveSettingsRegistryFile();

        // AzToolsFramework::EditorEvents interface implementation
        void NotifyMainWindowInitialized(QMainWindow* mainWindow) override;

        AZ::SettingsRegistryInterface* m_settingsRegistry;
    };

} // namespace AWSCore
