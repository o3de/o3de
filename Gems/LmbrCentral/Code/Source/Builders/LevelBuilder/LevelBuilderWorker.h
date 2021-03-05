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

        bool PopulateLevelDataDependenciesHelper(AZ::IO::GenericStream* stream, AssetBuilderSDK::ProductPathDependencySet& productDependencies) const;
        bool PopulateVegetationMapDataDependenciesHelper(AZ::IO::GenericStream* stream, AssetBuilderSDK::ProductPathDependencySet& productDependencies) const;
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

        void PopulateLevelDataDependencies(
            const AZStd::string& levelPakFile,
            const AZStd::string& levelPath,
            AssetBuilderSDK::ProductPathDependencySet& productDependencies) const;

        void PopulateVegetationMapDataDependencies(
            const AZStd::string& levelPakFile,
            AssetBuilderSDK::ProductPathDependencySet& productDependencies) const;

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
