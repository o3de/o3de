/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace DependencyBuilder
{
    //! This builder is responsible for handling those source assets that do not emit any products but does
    //! contain dependencies information in them and therefore needs to be tracked by the asset pipeline. 
    class DependencyBuilderWorker
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:

        DependencyBuilderWorker(AZStd::string jobKey, bool critical);

        //!AssetBuilderSDK::AssetBuilderCommandBus interface
        void ShutDown() override;

        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

        virtual AZ::Outcome<AZStd::vector<AssetBuilderSDK::SourceFileDependency>, AZStd::string> GetSourceDependencies(const AssetBuilderSDK::CreateJobsRequest& request) const;

        // Have the builder register a new worker when a new file type is handled
        virtual void RegisterBuilderWorker() = 0;

        // Unregister the builder worker
        virtual void UnregisterBuilderWorker() = 0;

    private:

        AZStd::string m_jobKey;
        bool m_critical = false;
        bool m_isShuttingDown = false;
    };
}
