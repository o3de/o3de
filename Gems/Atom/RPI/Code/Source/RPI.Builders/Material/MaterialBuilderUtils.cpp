/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MaterialBuilderUtils.h"
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <AzCore/Settings/SettingsRegistry.h>

namespace AZ::RPI::MaterialBuilderUtils
{
    void AddPossibleDependencies(
        const AZStd::string& currentFilePath,
        const AZStd::string& referencedParentPath,
        const char* jobKey,
        AssetBuilderSDK::JobDescriptor& outputJobDescriptor,
        AssetBuilderSDK::CreateJobsResponse& response,
        AssetBuilderSDK::JobDependencyType jobDependencyType,
        AZStd::optional<AZ::u32> productSubId)
    {
        const auto& possibleDependencies = RPI::AssetUtils::GetPossibleDependencyPaths(currentFilePath, referencedParentPath);
        for (const auto& file : possibleDependencies)
        {
            if (jobDependencyType != AssetBuilderSDK::JobDependencyType::OrderOnce &&
                jobDependencyType != AssetBuilderSDK::JobDependencyType::OrderOnly)
            {
                MaterialBuilderUtils::AddFingerprintForDependency(file, outputJobDescriptor);
            }

            // The first path found is the highest priority, and will have a job dependency, as this is the one
            // the builder will actually use
            AZ::Data::AssetInfo sourceInfo;
            AZStd::string watchFolder;
            bool dependencyFileFound = false;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                dependencyFileFound,
                &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath,
                file.c_str(),
                sourceInfo,
                watchFolder);

            if (dependencyFileFound)
            {
                AssetBuilderSDK::JobDependency jobDependency;
                jobDependency.m_jobKey = jobKey;
                jobDependency.m_sourceFile.m_sourceFileDependencyPath = file;
                jobDependency.m_type = jobDependencyType;

                if (productSubId)
                {
                    jobDependency.m_productSubIds.push_back(productSubId.value());
                }

                outputJobDescriptor.m_jobDependencyList.emplace_back(AZStd::move(jobDependency));

                // If the file was found, there is no need to add other possible path for the same dependency file.
                return;
            }

            // The file was not found so we can't add a job dependency. But we add a source dependency instead so if a file
            // shows up later at this location, it will wake up the builder to try again.
            AssetBuilderSDK::SourceFileDependency sourceDependency;
            sourceDependency.m_sourceFileDependencyPath = file;
            response.m_sourceFileDependencyList.emplace_back(AZStd::move(sourceDependency));
        }
    }

    void AddPossibleImageDependencies(
        const AZStd::string& currentFilePath,
        const AZStd::string& imageFilePath,
        AssetBuilderSDK::JobDescriptor& outputJobDescriptor,
        AssetBuilderSDK::CreateJobsResponse& response)
    {
        if (imageFilePath.empty())
        {
            return;
        }

        AZStd::string ext;
        AzFramework::StringFunc::Path::GetExtension(imageFilePath.c_str(), ext, false);
        AZStd::to_upper(ext.begin(), ext.end());
        AZStd::string jobKey = "Image Compile: " + ext;

        if (ext.empty())
        {
            return;
        }

        AddPossibleDependencies(
            currentFilePath,
            imageFilePath,
            jobKey.c_str(),
            outputJobDescriptor,
            response,
            AssetBuilderSDK::JobDependencyType::OrderOnce);
    }

    void AddFingerprintForDependency(
        const AZStd::string& filePath,
        AssetBuilderSDK::JobDescriptor& outputJobDescriptor)
    {
        outputJobDescriptor.m_additionalFingerprintInfo += AZStd::string::format("|%s:%llu", filePath.c_str(), AZ::IO::SystemFile::ModificationTime(filePath.c_str()));
    }
} // namespace AZ::RPI::MaterialBuilderUtils
