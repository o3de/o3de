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

#include <AzCore/std/parallel/atomic.h>
#include <AzCore/XML/rapidxml.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace AudioControlBuilder
{
    //! The Audio Control Builder Worker handles scanning XML files that are output by the Audio Controls editor
    //! for asset references to audio files and registers those files as product dependencies.
    class AudioControlBuilderWorker
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        AZ_RTTI(AudioControlBuilderWorker, "{3AD18978-9025-482A-B06A-17EF0EB4D7CA}");

        AudioControlBuilderWorker();
        ~AudioControlBuilderWorker() = default;

        //! Asset Builder Callback Functions
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

        //! AssetBuilderSDK::AssetBuilderCommandBus interface
        void ShutDown() override;

        bool ParseProductDependencies(
            const AssetBuilderSDK::ProcessJobRequest& request,
            AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
            AssetBuilderSDK::ProductPathDependencySet& pathDependencies);

    private:
        void ParseProductDependenciesFromXmlFile(
            const AZ::rapidxml::xml_node<char>* node,
            const AZStd::string& fullPath,
            const AZStd::string& sourceFile,
            const AZStd::string& platformIdentifier,
            AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
            AssetBuilderSDK::ProductPathDependencySet& pathDependencies);

        AZStd::string m_globalScopeControlsPath;
        AZStd::atomic_bool m_isShuttingDown;
    };

} // namespace AudioControlBuilder
