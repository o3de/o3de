/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace CopyDependencyBuilder
{
    class CopyDependencyBuilderWorker
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:

        CopyDependencyBuilderWorker(AZStd::string jobKey, bool critical, bool skipServer);

        //!AssetBuilderSDK::AssetBuilderCommandBus interface
        void ShutDown() override;

        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

        virtual AZ::Outcome<AZStd::vector<AssetBuilderSDK::SourceFileDependency>, AZStd::string> GetSourceDependencies(const AssetBuilderSDK::CreateJobsRequest& request) const;
        virtual AZ::Outcome<AZStd::vector<AZStd::string>, AZStd::string> GetSourcesToReprocess(const AssetBuilderSDK::ProcessJobRequest& request) const;

        // Have the builder register a new worker when a new file type is handled
        virtual void RegisterBuilderWorker() = 0;

        // Unregister the builder worker
        virtual void UnregisterBuilderWorker() = 0;

        // Parse the asset file and get product dependencies
        virtual bool ParseProductDependencies(
            const AssetBuilderSDK::ProcessJobRequest& request,
            AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
            AssetBuilderSDK::ProductPathDependencySet& pathDependencies) = 0;

        // Get the asset type. 
        // Return AZ::Data::AssetType::CreateNull() if the type is unknown and we will
        // infer the asset type by product file name when the JobProduct is created
        virtual AZ::Data::AssetType GetAssetType(const AZStd::string& fileName) const;

    private:

        AZStd::string m_jobKey;
        bool m_critical = false;
        bool m_isShuttingDown = false;
        bool m_skipServer = false;
    };
}
