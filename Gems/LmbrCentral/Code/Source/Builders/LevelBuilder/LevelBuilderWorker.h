/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Slice/SliceAsset.h>

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace LevelBuilder
{
    //! The level builder is copy job that examines level.pak files for asset references,
    //! to output product dependencies.
    class LevelBuilderWorker
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        AZ_RTTI(LevelBuilderWorker, "{C68340F1-1272-418D-8CF4-BF0AEC1E3C54}");

        //! Asset Builder Callback Functions
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

        //!AssetBuilderSDK::AssetBuilderCommandBus interface
        void ShutDown() override;

        bool PopulateMissionDependenciesHelper(AZ::IO::GenericStream* stream, AssetBuilderSDK::ProductPathDependencySet& productDependencies) const;
        void PopulateLevelSliceDependenciesHelper(
            const AZStd::string& levelSliceName,
            AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
            AssetBuilderSDK::ProductPathDependencySet& productPathDependencies) const;
        void PopulateLevelSliceDependenciesHelper(
            AZ::Data::Asset<AZ::SliceAsset>& sliceAsset, 
            AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
            AssetBuilderSDK::ProductPathDependencySet& productPathDependencies) const;
        void PopulateLevelAudioControlDependenciesHelper(const AZStd::string& levelName, AssetBuilderSDK::ProductPathDependencySet& productDependencies) const;

        void PopulateOptionalLevelDependencies(
            const AZStd::string& sourceRelativeFile,
            AssetBuilderSDK::ProductPathDependencySet& productPathDependencies) const;

    private:
        void PopulateProductDependencies(
            const AZStd::string& levelPakFile,
            const AZStd::string& sourceRelativeFile,
            const AZStd::string& tempDirectory,
            AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
            AssetBuilderSDK::ProductPathDependencySet& productPathDependencies) const;

        void AddLevelRelativeSourcePathProductDependency(
            const AZStd::string& optionalDependencyRelativeToLevel,
            const AZStd::string& sourceLevelPakPath,
            AssetBuilderSDK::ProductPathDependencySet& productPathDependencies) const;

        void PopulateLevelSliceDependencies(
            const AZStd::string& levelPath,
            AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
            AssetBuilderSDK::ProductPathDependencySet& productPathDependencies) const;

        void PopulateMissionDependencies(
            const AZStd::string& levelPakFile,
            const AZStd::string& levelPath,
            AssetBuilderSDK::ProductPathDependencySet& productDependencies) const;

        void PopulateLevelAudioControlDependencies(
            const AZStd::string& levelPakFile,
            AssetBuilderSDK::ProductPathDependencySet& productDependencies) const;        

        bool m_isShuttingDown = false;
    };
}
