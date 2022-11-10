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
            
        const bool currentFileIsMaterial = AzFramework::StringFunc::Path::IsExtension(currentFilePath.c_str(), MaterialSourceData::Extension);
        const bool referencedFileIsMaterialType = AzFramework::StringFunc::Path::IsExtension(referencedParentPath.c_str(), MaterialTypeSourceData::Extension);
        const bool shouldFinalizeMaterialAssets = MaterialBuilderUtils::BuildersShouldFinalizeMaterialAssets();

        AZStd::vector<AZStd::string> possibleDependencies = RPI::AssetUtils::GetPossibleDepenencyPaths(currentFilePath, referencedParentPath);
        for (auto& file : possibleDependencies)
        {
            // The first path found is the highest priority, and will have a job dependency, as this is the one
            // the builder will actually use
            if (!dependencyFileFound)
            {
                AZ::Data::AssetInfo sourceInfo;
                AZStd::string watchFolder;
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(dependencyFileFound, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, file.c_str(), sourceInfo, watchFolder);

                if (dependencyFileFound)
                {
                    AssetBuilderSDK::JobDependency jobDependency;
                    jobDependency.m_jobKey = jobKey;
                    jobDependency.m_sourceFile.m_sourceFileDependencyPath = file;
                    jobDependency.m_type = AssetBuilderSDK::JobDependencyType::Order;
                        
                    if(productSubId)
                    {
                        jobDependency.m_productSubIds.push_back(productSubId.value());
                    }
                        
                    // If we aren't finalizing material assets, then a normal job dependency isn't needed because the MaterialTypeAsset data won't be used.
                    // However, we do still need at least an OrderOnce dependency to ensure the Asset Processor knows about the material type asset so the builder can get it's AssetId.
                    // This can significantly reduce AP processing time when a material type or its shaders are edited.
                    if (forceOrderOnce || (currentFileIsMaterial && referencedFileIsMaterialType && !shouldFinalizeMaterialAssets))
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

    bool BuildersShouldFinalizeMaterialAssets()
    {
        // Disable this registry setting to improve iteration times when making changes to widely used shaders and material types,
        // like standard PBR, that require a large number of model assets to be reprocessed by the AP. Disabling finalization will
        // also disable build dependencies between materials and material types. Without those dependencies in place, loading and
        // reloading material assets will require special handling because typical asset notifications will not be sent when
        // dependencies are changed. Before, this option was disabled by default because of the long iteration times, but caused hot
        // reload problems so we enabled it again. We should explore options for handling dependencies on standard PBR differently
        // at the model builder level, and hopefully improve the iteration times that way. In that case, we should come back and
        // remove the deferred-finalize option entirely.
        bool shouldFinalize = true;

        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            settingsRegistry->Get(shouldFinalize, "/O3DE/Atom/RPI/MaterialBuilder/FinalizeMaterialAssets");
        }

        return shouldFinalize;
    }
}
