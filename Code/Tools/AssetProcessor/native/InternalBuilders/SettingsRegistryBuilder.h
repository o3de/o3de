/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AzCore/JSON/writer.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AssetProcessor
{
    class SettingsRegistryBuilder
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        SettingsRegistryBuilder();
        ~SettingsRegistryBuilder() override = default;

        bool Initialize();
        void Uninitialize();

        void ShutDown() override;

        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

    protected:
        AZStd::vector<AZStd::string> ReadExcludesFromRegistry() const;

    private:
        AZ::Uuid m_builderId;
        AZ::Data::AssetType m_assetType;
        bool m_isShuttingDown{ false };
    };
} // namespace AssetProcessor
