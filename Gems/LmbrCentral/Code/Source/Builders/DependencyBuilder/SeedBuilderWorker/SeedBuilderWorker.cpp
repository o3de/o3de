/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <LmbrCentral_precompiled.h>
#include "SeedBuilderWorker.h"
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Asset/AssetSeedList.h>

namespace DependencyBuilder
{
    SeedBuilderWorker::SeedBuilderWorker()
        : DependencyBuilderWorker("Seed", true)
    {
    }

    void SeedBuilderWorker::RegisterBuilderWorker()
    {
        AssetBuilderSDK::AssetBuilderDesc seedBuilderDescriptor;
        seedBuilderDescriptor.m_name = "SeedBuilderDescriptor";
        seedBuilderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.seed", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        seedBuilderDescriptor.m_busId = azrtti_typeid<SeedBuilderWorker>();
        seedBuilderDescriptor.m_version = 1;
        seedBuilderDescriptor.m_createJobFunction =
            AZStd::bind(&SeedBuilderWorker::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
        seedBuilderDescriptor.m_processJobFunction =
            AZStd::bind(&SeedBuilderWorker::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);

        AssetBuilderSDK::AssetBuilderCommandBus::Handler::BusConnect(seedBuilderDescriptor.m_busId);

        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, seedBuilderDescriptor);
    }

    void SeedBuilderWorker::UnregisterBuilderWorker()
    {
        AssetBuilderSDK::AssetBuilderCommandBus::Handler::BusDisconnect();
    }

    AZ::Outcome<AZStd::vector<AssetBuilderSDK::SourceFileDependency>, AZStd::string> SeedBuilderWorker::GetSourceDependencies(const AssetBuilderSDK::CreateJobsRequest& request) const
    {
        AZStd::string fullPath;
        AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.c_str(), request.m_sourceFile.c_str(), fullPath, false);
        AzFramework::StringFunc::Path::Normalize(fullPath);

        AzFramework::AssetSeedList assetSeedList;
        if (!AZ::Utils::LoadObjectFromFileInPlace(fullPath.c_str(), assetSeedList))
        {
            return AZ::Failure((AZStd::string::format("Unable to deserialize file (%s) from disk.\n", fullPath.c_str())));
        }
        AZStd::vector<AssetBuilderSDK::SourceFileDependency> sourceFileDependencies;
        for (AzFramework::SeedInfo& seedInfo : assetSeedList)
        {
            AssetBuilderSDK::SourceFileDependency dependency;
            dependency.m_sourceFileDependencyUUID = seedInfo.m_assetId.m_guid;
            sourceFileDependencies.emplace_back(dependency);
        }

        return AZ::Success(sourceFileDependencies);
    }
}
