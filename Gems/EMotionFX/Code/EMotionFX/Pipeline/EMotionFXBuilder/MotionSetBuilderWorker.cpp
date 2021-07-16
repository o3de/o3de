/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "MotionSetBuilderWorker.h"

#include <EMotionFX/Source/Importer/Importer.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <Integration/Assets/MotionSetAsset.h>

#include <AzFramework/StringFunc/StringFunc.h>

namespace EMotionFX
{
    namespace EMotionFXBuilder
    {
        void MotionSetBuilderWorker::RegisterBuilderWorker()
        {
            AssetBuilderSDK::AssetBuilderDesc motionSetBuilderDescriptor;
            motionSetBuilderDescriptor.m_name = "MotionSetBuilderWorker";
            motionSetBuilderDescriptor.m_patterns.emplace_back(AssetBuilderSDK::AssetBuilderPattern("*.motionset", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
            motionSetBuilderDescriptor.m_busId = azrtti_typeid<MotionSetBuilderWorker>();
            motionSetBuilderDescriptor.m_version = 2;
            motionSetBuilderDescriptor.m_createJobFunction =
                AZStd::bind(&MotionSetBuilderWorker::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
            motionSetBuilderDescriptor.m_processJobFunction =
                AZStd::bind(&MotionSetBuilderWorker::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);

            BusConnect(motionSetBuilderDescriptor.m_busId);

            AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, motionSetBuilderDescriptor);
        }

        void MotionSetBuilderWorker::ShutDown()
        {
            m_isShuttingDown = true;
        }

        void MotionSetBuilderWorker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
        {
            if (m_isShuttingDown)
            {
                response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
                return;
            }

            for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
            {
                AssetBuilderSDK::JobDescriptor descriptor;
                descriptor.m_jobKey = "motionset";
                descriptor.m_critical = true;
                descriptor.SetPlatformIdentifier(info.m_identifier.c_str());
                response.m_createJobOutputs.push_back(descriptor);
            }

            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
        }

        void MotionSetBuilderWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
        {
            AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "MotionSetBuilderWorker Starting Job.\n");

            if (m_isShuttingDown)
            {
                AZ_TracePrintf(AssetBuilderSDK::WarningWindow, "Cancelled job %s because shutdown was requested.\n", request.m_fullPath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                return;
            }

            AZStd::string fileName;
            AzFramework::StringFunc::Path::GetFullFileName(request.m_fullPath.c_str(), fileName);

            AZStd::string destPath;
            // Do all work inside the tempDirPath.
            AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), fileName.c_str(), destPath, true);

            AssetBuilderSDK::JobProduct jobProduct(request.m_fullPath, azrtti_typeid<Integration::MotionSetAsset>(), 0);

            if (!ParseProductDependencies(request.m_fullPath, request.m_sourceFile, jobProduct.m_pathDependencies))
            {
                AZ_Error(AssetBuilderSDK::ErrorWindow, false, "Error during outputing product dependencies for asset %s.\n", fileName.c_str());
            }

            jobProduct.m_dependenciesHandled = true; // We've output the dependencies immediately above so it's OK to tell the AP we've handled dependencies
            response.m_outputProducts.push_back(jobProduct);
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        }

        bool MotionSetBuilderWorker::ParseProductDependencies(const AZStd::string& fullPath, [[maybe_unused]] const AZStd::string& sourceFile, AssetBuilderSDK::ProductPathDependencySet& pathDependencies)
        {
            AZ::ObjectStream::FilterDescriptor loadFilter = AZ::ObjectStream::FilterDescriptor(&AZ::Data::AssetFilterNoAssetLoading, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES);
            AZStd::unique_ptr<MotionSet> motionSet(GetImporter().LoadMotionSet(fullPath, nullptr, loadFilter));

            if (!motionSet)
            {
                return false;
            }

            for (const AZStd::pair<AZStd::string, MotionSet::MotionEntry*>& it : motionSet->GetMotionEntries())
            {
                pathDependencies.emplace(it.second->GetFilename(), AssetBuilderSDK::ProductPathDependencyType::ProductFile);
            }

            return true;
        }
    }
}
