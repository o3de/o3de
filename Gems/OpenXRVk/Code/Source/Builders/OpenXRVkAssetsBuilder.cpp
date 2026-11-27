/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/StringFunc/StringFunc.h>
#include <AssetBuilderSDK/SerializationDependencies.h>

#include <OpenXRVk/OpenXRVkInteractionProfilesAsset.h>
#include <OpenXRVk/OpenXRVkActionSetsAsset.h>
#include <OpenXRVk/OpenXRVkAssetsValidator.h>

#include "OpenXRVkAssetsBuilder.h"

namespace OpenXRVkBuilders
{
    template<class AssetType>
    static AZStd::unique_ptr<AssetType> LoadAssetAsUniquePtr(const AZStd::string& filePath)
    {
        AZ::ObjectStream::FilterDescriptor loadFilter = AZ::ObjectStream::FilterDescriptor(&AZ::Data::AssetFilterNoAssetLoading, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES);
        auto actionSetsAssetPtr = AZ::Utils::LoadObjectFromFile<AssetType>(filePath, nullptr, loadFilter);
        if (!actionSetsAssetPtr)
        {
            return nullptr;
        }
        return AZStd::unique_ptr<AssetType>(actionSetsAssetPtr);
    }


    void OpenXRAssetsBuilder::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
    {
        //! First get the extension 
        constexpr bool includeDot = false;
        AZStd::string fileExtension;
        bool result = AZ::StringFunc::Path::GetExtension(request.m_sourceFile.c_str(), fileExtension, includeDot);
        if (result && (fileExtension == OpenXRVk::OpenXRInteractionProfilesAsset::s_assetExtension))
        {
            CreateInteractionProfilesAssetJobs(request, response);
            return;
        }

        if (result && (fileExtension == OpenXRVk::OpenXRActionSetsAsset::s_assetExtension))
        {
            CreateActionSetsAssetJobs(request, response);
            return;
        }

        //! Unknown extension.
        AZ_Error(LogName, false, "Unknown file extension [%s] for this builder. Source file [%s]", fileExtension.c_str(), request.m_sourceFile.c_str());
        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Failed;
    }
    
    
    void OpenXRAssetsBuilder::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
    {
        //! First get the extension 
        constexpr bool includeDot = false;
        AZStd::string fileExtension;
        bool result = AZ::StringFunc::Path::GetExtension(request.m_sourceFile.c_str(), fileExtension, includeDot);
        if (result && (fileExtension == OpenXRVk::OpenXRInteractionProfilesAsset::s_assetExtension))
        {
            ProcessInteractionProfilesAssetJob(request, response);
            return;
        }
        if (result && (fileExtension == OpenXRVk::OpenXRActionSetsAsset::s_assetExtension))
        {
            ProcessActionSetsAssetJob(request, response);
        }
    }


    /////////////////////////////////////////////////////////////////////////////////
    // OpenXRInteractionProfilesAsset Support Begin
    void OpenXRAssetsBuilder::CreateInteractionProfilesAssetJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
    {
        for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor jobDescriptor;
            // Very high priority because this asset is required to initialize the OpenXR runtime
            // and initialize the I/O actions system.
            jobDescriptor.m_priority = 1000;
            jobDescriptor.m_critical = true;
            jobDescriptor.m_jobKey = InteractionProfilesAssetJobKey;
            jobDescriptor.SetPlatformIdentifier(platformInfo.m_identifier.c_str());
            response.m_createJobOutputs.emplace_back(AZStd::move(jobDescriptor));
        } // for all request.m_enabledPlatforms

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    
    void OpenXRAssetsBuilder::ProcessInteractionProfilesAssetJob([[maybe_unused]] const AssetBuilderSDK::ProcessJobRequest& request, [[maybe_unused]] AssetBuilderSDK::ProcessJobResponse& response) const
    {
        // Open the file, and make sure there's no redundant data, the OpenXR Paths are well formatted, etc.
       auto interactionProfilesAssetPtr = LoadAssetAsUniquePtr<OpenXRVk::OpenXRInteractionProfilesAsset>(request.m_fullPath);
       if (!interactionProfilesAssetPtr)
       {
           AZ_Error(LogName, false, "Failed to load interaction profile source asset [%s]", request.m_fullPath.c_str());
           response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
           return;
       }

       auto outcome = OpenXRVkAssetsValidator::ValidateInteractionProfilesAsset(*interactionProfilesAssetPtr.get());
       if (!outcome.IsSuccess())
       {
           AZ_Error(LogName, false, "Invalid InteractionProfilesAsset [%s]. Reason:\n%s",
               request.m_fullPath.c_str(), outcome.GetError().c_str());
           response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
           return;
       }

       // We keep exact same asset name and extension.
       AZStd::string assetFileName;
       AZ::StringFunc::Path::GetFullFileName(request.m_fullPath.c_str(), assetFileName);

       // Construct product full path
       AZStd::string assetOutputPath;
       AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), assetFileName.c_str(), assetOutputPath, true);

       bool result = AZ::Utils::SaveObjectToFile(assetOutputPath, AZ::DataStream::ST_XML, interactionProfilesAssetPtr.get());
       if (result == false)
       {
           AZ_Error(LogName, false, "Failed to save asset to %s", assetOutputPath.c_str());
           response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
           return;
       }

       AssetBuilderSDK::JobProduct jobProduct;
       if (!AssetBuilderSDK::OutputObject(interactionProfilesAssetPtr.get(), assetOutputPath,
           azrtti_typeid<OpenXRVk::OpenXRInteractionProfilesAsset>(),
           aznumeric_cast<uint32_t>(0), jobProduct))
       {
           AZ_Error(LogName, false, "Failed to declare output product assets.");
           response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
           return;
       }
       response.m_outputProducts.emplace_back(AZStd::move(jobProduct));
       response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    }
    // OpenXRInteractionProfilesAsset Support End
    /////////////////////////////////////////////////////////////////////////////////////


    void OpenXRAssetsBuilder::CreateActionSetsAssetJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
    {
        // Make sure the InteractionProfiles asset referenced in this ActionSets asset exists. and if so,
        // also declare job dependency.
        constexpr bool caseInsensitive = false;
        constexpr bool normalize = true;
        AZStd::string sourcePath;
        AZ::StringFunc::Path::Join(request.m_watchFolder.c_str(), request.m_sourceFile.c_str(), sourcePath, caseInsensitive, normalize);
        auto actionSetsAssetPtr = LoadAssetAsUniquePtr<OpenXRVk::OpenXRActionSetsAsset>(sourcePath);
        if (!actionSetsAssetPtr)
        {
            AZ_Error(LogName, false, "Failed to load the ActionSets asset at path[%s].", request.m_sourceFile.c_str());
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Failed;
            return;
        }

        if (!actionSetsAssetPtr->m_interactionProfilesAsset.GetId().IsValid())
        {
            AZ_Error(LogName, false, "Need a valid InteractionPRofile Asset UUID to setup the job dependency.");
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Failed;
            return;
        }
        
        for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor jobDescriptor;
            // Very high priority because this asset is required to initialize the OpenXR runtime
            // and initialize the I/O actions system.
            jobDescriptor.m_priority = 999;
            jobDescriptor.m_critical = true;
            jobDescriptor.m_jobKey = ActionSetsAssetJobKey;
            jobDescriptor.SetPlatformIdentifier(platformInfo.m_identifier.c_str());

            AssetBuilderSDK::SourceFileDependency sourceFileDependency{};
            sourceFileDependency.m_sourceFileDependencyUUID = actionSetsAssetPtr->m_interactionProfilesAsset.GetId().m_guid;
            auto jobDependency = AssetBuilderSDK::JobDependency(InteractionProfilesAssetJobKey, platformInfo.m_identifier,
                AssetBuilderSDK::JobDependencyType::Order, sourceFileDependency);
            jobDescriptor.m_jobDependencyList.emplace_back(AZStd::move(jobDependency));

            response.m_createJobOutputs.emplace_back(AZStd::move(jobDescriptor));
        } // for all request.m_enabledPlatforms

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }


    //! Each action in an actionSet has a "name" and a "localizedName". The "name" can never be empty, but
    //! if "localizedName" is empty we automatically patch it as an identical copy of "name".
    static void FixEmptyLocalizedNames(OpenXRVk::OpenXRActionSetsAsset& actionSetAsset)
    {
        for (auto& actionSetDescriptor : actionSetAsset.m_actionSetDescriptors)
        {
            if (actionSetDescriptor.m_localizedName.empty())
            {
                AZ_Printf(OpenXRAssetsBuilder::LogName, "ActionSet had empty LocalizedName. Taking new value of [%s]", actionSetDescriptor.m_name.c_str());
                actionSetDescriptor.m_localizedName = actionSetDescriptor.m_name;
            }
            for (auto& actionDescriptor : actionSetDescriptor.m_actionDescriptors)
            {
                if (actionDescriptor.m_localizedName.empty())
                {
                    AZ_Printf(OpenXRAssetsBuilder::LogName, "Action in ActionSet [%s] had empty LocalizedName. Taking new value of [%s]",
                        actionSetDescriptor.m_name.c_str(), actionDescriptor.m_name.c_str());
                    actionDescriptor.m_localizedName = actionDescriptor.m_name;
                }
            }
        }
    }


    static AZStd::string GetInteractionProfileAssetSourcePath(const OpenXRVk::OpenXRActionSetsAsset& actionSetsAsset)
    {
        const auto& sourceUuid = actionSetsAsset.m_interactionProfilesAsset.GetId().m_guid;
        bool foundSource = false;
        AZ::Data::AssetInfo sourceAssetInfo;
        AZStd::string sourceWatchFolder;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(foundSource, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourceUUID,
            sourceUuid, sourceAssetInfo, sourceWatchFolder);
        AZStd::string sourcePath;
        if (foundSource)
        {
            constexpr bool caseInsensitive = false;
            constexpr bool normalize = true;
            AZ::StringFunc::Path::Join(sourceWatchFolder.c_str(), sourceAssetInfo.m_relativePath.c_str(), sourcePath, caseInsensitive, normalize);
        }

        return sourcePath;
    }


    void OpenXRAssetsBuilder::ProcessActionSetsAssetJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
    {
        auto actionSetsAssetPtr = LoadAssetAsUniquePtr<OpenXRVk::OpenXRActionSetsAsset>(request.m_fullPath);
        if (!actionSetsAssetPtr)
        {
            AZ_Error(LogName, false, "Failed to Load ActionsSet asset from File %s", request.m_fullPath.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        FixEmptyLocalizedNames(*actionSetsAssetPtr.get());

        // The Action Sets Asset contains an asset reference to the OpenXRInteractionProfilesAsset that was used
        // to construct the data in it. Because we are running in a builder context, the OpenXRInteractionProfilesAsset
        // is loaded with a null handle, BUT the AssetHint is valid and we'll use the AssetHint to discover
        // the OpenXRInteractionProfilesAsset and load it manually.
        auto interactionProfileSourcePath = GetInteractionProfileAssetSourcePath(*actionSetsAssetPtr.get());
        if (interactionProfileSourcePath.empty())
        {
            AZ_Error(LogName, false, "An ActionSets source asset requires a valid InteractionProfiles source asset.");
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }
        auto interactionProfileAssetPtr = LoadAssetAsUniquePtr<OpenXRVk::OpenXRInteractionProfilesAsset>(interactionProfileSourcePath);
        if (!interactionProfileAssetPtr)
        {
            AZ_Error(LogName, false, "Failed to Load InteractionProfiles asset from File %s", interactionProfileSourcePath.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        auto outcome = OpenXRVkAssetsValidator::ValidateActionSetsAsset(*actionSetsAssetPtr.get(), *interactionProfileAssetPtr.get());
        if (!outcome.IsSuccess())
        {
            AZ_Error(LogName, false, "Invalid source ActionSets content when using source InteractionProfiles asset file [%s]. Reason:\n%s",
                interactionProfileSourcePath.c_str(), outcome.GetError().c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        // We keep exact same asset name and extension.
        AZStd::string assetFileName;
        AZ::StringFunc::Path::GetFullFileName(request.m_fullPath.c_str(), assetFileName);

        // Construct product full path
        AZStd::string assetOutputPath;
        AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), assetFileName.c_str(), assetOutputPath, true);

        bool result = AZ::Utils::SaveObjectToFile(assetOutputPath, AZ::DataStream::ST_XML, actionSetsAssetPtr.get());
        if (result == false)
        {
            AZ_Error(LogName, false, "Failed to save asset to %s", assetOutputPath.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        // This step is very important, because it declares that this OpenXRActionsSetAsset depends on a OpenXRInteractionProfilesAsset.
        // This will guarantee that when the OpenXRActionsSetAsset is loaded at runtime, the Asset Catalog will report OnAssetReady
        // only after the OpenXRInteractionProfilesAsset is already fully loaded and ready.
        AssetBuilderSDK::JobProduct jobProduct;
        if (!AssetBuilderSDK::OutputObject(actionSetsAssetPtr.get(), assetOutputPath, azrtti_typeid<OpenXRVk::OpenXRActionSetsAsset>(),
            aznumeric_cast<uint32_t>(0), jobProduct))
        {
            AZ_Error(LogName, false, "Failed to output asset jobs and runtime dependencies.");
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }
        response.m_outputProducts.emplace_back(AZStd::move(jobProduct));
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    }

    
} // namespace OpenXRVkBuilders

