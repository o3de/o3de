/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
