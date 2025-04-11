/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CopyDependencyBuilderWorker.h"

#include <AzFramework/StringFunc/StringFunc.h>

namespace CopyDependencyBuilder
{
    CopyDependencyBuilderWorker::CopyDependencyBuilderWorker(AZStd::string jobKey, bool critical, bool skipServer)
        : m_jobKey(jobKey)
        , m_critical(critical)
        , m_skipServer(skipServer)
    {
    }

    void CopyDependencyBuilderWorker::ShutDown()
    {
        m_isShuttingDown = true;
    }

    void CopyDependencyBuilderWorker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        // Add source dependencies to CreateJobsResponse
        auto sourceDependencesResult = GetSourceDependencies(request);
        if (!sourceDependencesResult.IsSuccess())
        {
            AZ_Error(AssetBuilderSDK::ErrorWindow, false, sourceDependencesResult.TakeError().c_str());
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Failed;
            return;
        }
        response.m_sourceFileDependencyList = sourceDependencesResult.TakeValue();

        for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
        {
            if (m_skipServer && info.m_identifier == "server")
            {
                continue;
            }

            AssetBuilderSDK::JobDescriptor descriptor;
            descriptor.m_jobKey = m_jobKey;
            descriptor.m_critical = m_critical;
            descriptor.SetPlatformIdentifier(info.m_identifier.c_str());

            // Add source dependencies to job parameters and pass them to ProcessJob
            const int sourceDependencyStartPoint = static_cast<int>(descriptor.m_jobParameters.size());
            const int sourceDependenciesNum = static_cast<int>(response.m_sourceFileDependencyList.size());
            descriptor.m_jobParameters[AZ_CRC_CE("sourceDependencyStartPoint")] = AZStd::to_string(sourceDependencyStartPoint);
            descriptor.m_jobParameters[AZ_CRC_CE("sourceDependenciesNum")] = AZStd::to_string(sourceDependenciesNum);

            for (int index = 0; index < sourceDependenciesNum; ++index)
            {
                descriptor.m_jobParameters[sourceDependencyStartPoint + index] = response.m_sourceFileDependencyList[index].m_sourceFileDependencyPath;
            }

            response.m_createJobOutputs.push_back(descriptor);
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    AZ::Outcome<AZStd::vector<AssetBuilderSDK::SourceFileDependency>, AZStd::string> CopyDependencyBuilderWorker::GetSourceDependencies(
        const AssetBuilderSDK::CreateJobsRequest& /*request*/) const
    {
        return AZ::Success(AZStd::vector<AssetBuilderSDK::SourceFileDependency>{});
    }

    void CopyDependencyBuilderWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "CopyDependencyBuilderWorker Starting Job.\n");

        if (m_isShuttingDown)
        {
            AZ_TracePrintf(AssetBuilderSDK::WarningWindow, "Cancelled job %s because shutdown was requested.\n", request.m_fullPath.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }

        AZStd::string fileName;
        AzFramework::StringFunc::Path::GetFullFileName(request.m_fullPath.c_str(), fileName);

        AssetBuilderSDK::JobProduct jobProduct(request.m_fullPath, GetAssetType(fileName));

        if (!ParseProductDependencies(request, jobProduct.m_dependencies, jobProduct.m_pathDependencies))
        {
            AZ_Error(AssetBuilderSDK::ErrorWindow, false, "Error during outputing product dependencies for asset %s.\n", fileName.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        jobProduct.m_dependenciesHandled = true; // We've output the dependencies immediately above so it's OK to tell the AP we've handled dependencies
        response.m_outputProducts.push_back(jobProduct);

        auto reverseSourceDependencesResult = GetSourcesToReprocess(request);
        if (!reverseSourceDependencesResult.IsSuccess())
        {
            AZ_Error(AssetBuilderSDK::ErrorWindow, false, reverseSourceDependencesResult.TakeError().c_str());
            // The primary use of this system, the XMLSchemaSystem, is edited using the asset editor.
            // The asset editing system does not play nice with failed jobs. It's common for GetSourcesToReprocess to fail
            // due to configuration issues. It shouldn't block editing the file using the asset editor.
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            return;
        }

        response.m_sourcesToReprocess = reverseSourceDependencesResult.TakeValue();
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    }

    AZ::Outcome<AZStd::vector<AZStd::string>, AZStd::string> CopyDependencyBuilderWorker::GetSourcesToReprocess(const AssetBuilderSDK::ProcessJobRequest& /*request*/) const
    {
        return AZ::Success(AZStd::vector<AZStd::string>{});
    }

    AZ::Data::AssetType CopyDependencyBuilderWorker::GetAssetType(const AZStd::string& fileName) const
    {
        static const char* vegDescriptorListExtension = ".vegdescriptorlist";
        static AZ::Data::AssetType vegDescriptorListType("{60961B36-E3CA-4877-B197-1462C1363F6E}"); // DescriptorListAsset in Vegetation Gem

        if (fileName.ends_with(vegDescriptorListExtension))
        {
            return vegDescriptorListType;
        }
        return AZ::Data::AssetType::CreateNull();
    }
}
