/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <utilities/AssetUtilEBusHelper.h>

namespace UnitTests
{
    struct MockBuilderInfoHandler : public AssetProcessor::AssetBuilderInfoBus::Handler
    {
        ~MockBuilderInfoHandler();

        //! AssetProcessor::AssetBuilderInfoBus Interface
        void GetMatchingBuildersInfo(const AZStd::string& assetPath, AssetProcessor::BuilderInfoList& builderInfoList) override;
        void GetAllBuildersInfo(AssetProcessor::BuilderInfoList& builderInfoList) override;

        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

        AssetBuilderSDK::AssetBuilderDesc CreateBuilderDesc(
            const QString& builderName,
            const QString& builderId,
            const AZStd::vector<AssetBuilderSDK::AssetBuilderPattern>& builderPatterns);

        AssetBuilderSDK::AssetBuilderDesc m_builderDesc;
        QString m_jobFingerprint;
        QString m_dependencyFilePath;
        QString m_jobDependencyFilePath;
        AZStd::vector<AZ::u32> m_subIdDependencies;
        int m_createJobsCount = 0;
    };
}
