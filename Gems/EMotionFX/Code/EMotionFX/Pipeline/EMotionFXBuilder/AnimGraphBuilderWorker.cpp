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
#include "AnimGraphBuilderWorker.h"

#include <EMotionFX/Source/Importer/Importer.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphReferenceNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>

#include <AzCore/Serialization/ObjectStream.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace EMotionFX
{
    namespace EMotionFXBuilder
    {
        void AnimGraphBuilderWorker::RegisterBuilderWorker()
        {
            AssetBuilderSDK::AssetBuilderDesc animGraphBuilderDescriptor;
            animGraphBuilderDescriptor.m_name = "AnimGraphBuilderWorker";
            animGraphBuilderDescriptor.m_patterns.emplace_back(AssetBuilderSDK::AssetBuilderPattern("*.animgraph", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
            animGraphBuilderDescriptor.m_busId = azrtti_typeid<AnimGraphBuilderWorker>();
            animGraphBuilderDescriptor.m_version = 2;
            animGraphBuilderDescriptor.m_createJobFunction =
                AZStd::bind(&AnimGraphBuilderWorker::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
            animGraphBuilderDescriptor.m_processJobFunction =
                AZStd::bind(&AnimGraphBuilderWorker::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);

            BusConnect(animGraphBuilderDescriptor.m_busId);

            AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, animGraphBuilderDescriptor);
        }

        void AnimGraphBuilderWorker::ShutDown()
        {
            m_isShuttingDown = true;
        }

        void AnimGraphBuilderWorker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
        {
            if (m_isShuttingDown)
            {
                response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
                return;
            }

            for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
            {
                AssetBuilderSDK::JobDescriptor descriptor;
                descriptor.m_jobKey = "animgraph";
                descriptor.m_critical = true;
                descriptor.SetPlatformIdentifier(info.m_identifier.c_str());
                response.m_createJobOutputs.push_back(descriptor);
            }

            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
        }

        void AnimGraphBuilderWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
        {
            AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "AnimGraphBuilderWorker Starting Job.\n");

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

            AssetBuilderSDK::JobProduct jobProduct(request.m_fullPath, azrtti_typeid <Integration::AnimGraphAsset>(), 0);

            if (!ParseProductDependencies(request.m_fullPath, request.m_sourceFile, jobProduct.m_dependencies))
            {
                AZ_Error(AssetBuilderSDK::ErrorWindow, false, "Error during outputing product dependencies for asset %s.\n", fileName.c_str());
            }

            jobProduct.m_dependenciesHandled = true; // We've output the dependencies immediately above so it's OK to tell the AP we've handled dependencies
            response.m_outputProducts.push_back(jobProduct);
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        }

        bool AnimGraphBuilderWorker::ParseProductDependencies(const AZStd::string& fullPath, const AZStd::string& sourceFile, AZStd::vector<AssetBuilderSDK::ProductDependency>& pathDependencies)
        {
            AZ_UNUSED(sourceFile);

            AZ::ObjectStream::FilterDescriptor loadFilter = AZ::ObjectStream::FilterDescriptor(&AZ::Data::AssetFilterNoAssetLoading, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES);
            AZStd::unique_ptr<AnimGraph> animGraph(GetImporter().LoadAnimGraph(fullPath, nullptr, loadFilter));

            if (!animGraph)
            {
                return false;
            }

            AZStd::vector<AnimGraphNode*> animGraphNodes;
            animGraph->RecursiveCollectNodesOfType(azrtti_typeid<AnimGraphReferenceNode>(), &animGraphNodes);

            for (AnimGraphNode* node : animGraphNodes)
            {
                AnimGraphReferenceNode* referenceNode = static_cast<AnimGraphReferenceNode*>(node);

                AZ::Data::Asset<Integration::AnimGraphAsset> referencedAnimGraphAsset = referenceNode->GetReferencedAnimGraphAsset();
                if (referencedAnimGraphAsset)
                {
                    pathDependencies.emplace_back(AssetBuilderSDK::ProductDependency(referencedAnimGraphAsset.GetId(), 0));
                }

                AZ::Data::Asset<Integration::MotionSetAsset> referencedMotionSetAsset = referenceNode->GetReferencedMotionSetAsset();
                if (referencedMotionSetAsset)
                {
                    pathDependencies.emplace_back(AssetBuilderSDK::ProductDependency(referencedMotionSetAsset.GetId(), 0));
                }
            }

            return true;
        }
    } // namespace EMotionFXBuilder
} // namespace EMotionFX
