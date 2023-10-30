/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MaterialBuilderUtils.h"
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <AzCore/Settings/SettingsRegistry.h>

namespace AZ::RPI::MaterialBuilderUtils
{
    void AddPossibleDependencies(
        const AZStd::string& originatingSourceFilePath,
        const AZStd::string& referencedSourceFilePath,
        AssetBuilderSDK::CreateJobsResponse& response,
        AssetBuilderSDK::JobDescriptor& jobDescriptor,
        const AZStd::string& jobKey,
        const AssetBuilderSDK::JobDependencyType jobDependencyType,
        const AZStd::vector<AZ::u32>& productSubIds,
        const AZStd::string& platformId)
    {
        for (const auto& path : RPI::AssetUtils::GetPossibleDependencyPaths(originatingSourceFilePath, referencedSourceFilePath))
        {
            if (jobDependencyType != AssetBuilderSDK::JobDependencyType::OrderOnce &&
                jobDependencyType != AssetBuilderSDK::JobDependencyType::OrderOnly)
            {
                MaterialBuilderUtils::AddFingerprintForDependency(path, jobDescriptor);
            }

            // The first path is the highest priority, and will have a job dependency, as this is the one
            // the builder will actually use
            bool dependencyFileFound = false;
            AZ::Data::AssetInfo sourceInfo;
            AZStd::string watchFolder;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                dependencyFileFound,
                &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath,
                path.c_str(),
                sourceInfo,
                watchFolder);

            if (dependencyFileFound)
            {
                AssetBuilderSDK::JobDependency jobDependency;
                jobDependency.m_jobKey = jobKey;
                jobDependency.m_sourceFile.m_sourceFileDependencyPath = path;
                jobDependency.m_type = jobDependencyType;
                jobDependency.m_productSubIds = productSubIds;
                jobDependency.m_platformIdentifier = platformId;
                jobDescriptor.m_jobDependencyList.emplace_back(AZStd::move(jobDependency));
                return;
            }

            // The file was not found so we can't add a job dependency. But we add a source dependency instead so if a file
            // shows up later at this location, it will wake up the builder to try again.
            AssetBuilderSDK::SourceFileDependency sourceDependency;
            sourceDependency.m_sourceFileDependencyPath = path;
            response.m_sourceFileDependencyList.emplace_back(AZStd::move(sourceDependency));
        }
    }

    void AddPossibleImageDependencies(
        const AZStd::string& originatingSourceFilePath,
        const AZStd::string& referencedSourceFilePath,
        AssetBuilderSDK::CreateJobsResponse& response,
        AssetBuilderSDK::JobDescriptor& jobDescriptor)
    {
        if (!referencedSourceFilePath.empty())
        {
            AZStd::string ext;
            AzFramework::StringFunc::Path::GetExtension(referencedSourceFilePath.c_str(), ext, false);

            if (!ext.empty())
            {
                AZStd::to_upper(ext.begin(), ext.end());
                AZStd::string jobKey = "Image Compile: " + ext;

                AddPossibleDependencies(
                    originatingSourceFilePath,
                    referencedSourceFilePath,
                    response,
                    jobDescriptor,
                    jobKey,
                    AssetBuilderSDK::JobDependencyType::OrderOnce);
            }
        }
    }

    void AddFingerprintForDependency(const AZStd::string& path, AssetBuilderSDK::JobDescriptor& jobDescriptor)
    {
        jobDescriptor.m_additionalFingerprintInfo +=
            AZStd::string::format("|%u:%llu", (AZ::u32)AZ::Crc32(path), AZ::IO::SystemFile::ModificationTime(path.c_str()));
    }

    AZStd::string GetRelativeSourcePath(const AZStd::string& path)
    {
        bool pathFound{};
        AZStd::string relativePath;
        AZStd::string rootFolder;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            pathFound, &AzToolsFramework::AssetSystemRequestBus::Events::GenerateRelativeSourcePath, path, relativePath, rootFolder);
        return relativePath;
    }
} // namespace AZ::RPI::MaterialBuilderUtils
