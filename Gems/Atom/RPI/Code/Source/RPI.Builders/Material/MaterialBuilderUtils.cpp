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
        AZStd::vector<AssetBuilderSDK::JobDependency>& jobDependencies,
        AZStd::vector<AssetBuilderSDK::SourceFileDependency>& sourceDependencies,
        bool forceOrderOnce,
        AZStd::optional<AZ::u32> productSubId)
    {
        bool dependencyFileFound = false;

        AZStd::vector<AZStd::string> possibleDependencies = RPI::AssetUtils::GetPossibleDependencyPaths(currentFilePath, referencedParentPath);
        for (auto& file : possibleDependencies)
        {
            // The first path found is the highest priority, and will have a job dependency, as this is the one
            // the builder will actually use
            if (!dependencyFileFound)
            {
                AZ::Data::AssetInfo sourceInfo;
                AZStd::string watchFolder;
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
                    jobDependency.m_type = AssetBuilderSDK::JobDependencyType::Order;

                    if (productSubId)
                    {
                        jobDependency.m_productSubIds.push_back(productSubId.value());
                    }

                    if (forceOrderOnce)
                    {
                        jobDependency.m_type = AssetBuilderSDK::JobDependencyType::OrderOnce;
                    }

                    jobDependencies.push_back(jobDependency);
                }
                else
                {
                    // The file was not found so we can't add a job dependency. But we add a source dependency instead so if a file
                    // shows up later at this location, it will wake up the builder to try again.

                    AssetBuilderSDK::SourceFileDependency sourceDependency;
                    sourceDependency.m_sourceFileDependencyPath = file;
                    sourceDependencies.push_back(sourceDependency);
                }
            }
        }
    }

    void AddPossibleImageDependencies(
        const AZStd::string& currentFilePath,
        const AZStd::string& imageFilePath,
        AZStd::vector<AssetBuilderSDK::JobDependency>& jobDependencies,
        AZStd::vector<AssetBuilderSDK::SourceFileDependency>& sourceDependencies)
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

        bool forceOrderOnce = true;
        AddPossibleDependencies(currentFilePath,
            imageFilePath,
            jobKey.c_str(),
            jobDependencies,
            sourceDependencies,
            forceOrderOnce);
    }
}
