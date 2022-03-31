/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/tests/UnitTestUtilities.h>

namespace UnitTests
{
    MockBuilderInfoHandler::~MockBuilderInfoHandler()
    {
        BusDisconnect();
        m_builderDesc = {};
    }

    void MockBuilderInfoHandler::GetMatchingBuildersInfo(
        [[maybe_unused]] const AZStd::string& assetPath, AssetProcessor::BuilderInfoList& builderInfoList)
    {
        builderInfoList.push_back(m_builderDesc);
    }

    void MockBuilderInfoHandler::GetAllBuildersInfo(AssetProcessor::BuilderInfoList& builderInfoList)
    {
        builderInfoList.push_back(m_builderDesc);
    }

    void MockBuilderInfoHandler::CreateJobs(
        const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;

        for (const auto& platform : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor jobDescriptor;
            jobDescriptor.m_priority = 0;
            jobDescriptor.m_critical = true;
            jobDescriptor.m_jobKey = "Mock Job";
            jobDescriptor.SetPlatformIdentifier(platform.m_identifier.c_str());
            jobDescriptor.m_additionalFingerprintInfo = m_jobFingerprint.toUtf8().data();

            if (!m_jobDependencyFilePath.isEmpty())
            {
                AssetBuilderSDK::JobDependency jobDependency(
                    "Mock Job", "pc", AssetBuilderSDK::JobDependencyType::Order,
                    AssetBuilderSDK::SourceFileDependency(m_jobDependencyFilePath.toUtf8().constData(), AZ::Uuid::CreateNull()));

                if (!m_subIdDependencies.empty())
                {
                    jobDependency.m_productSubIds = m_subIdDependencies;
                }

                jobDescriptor.m_jobDependencyList.push_back(jobDependency);
            }

            if (!m_dependencyFilePath.isEmpty())
            {
                response.m_sourceFileDependencyList.push_back(
                    AssetBuilderSDK::SourceFileDependency(m_dependencyFilePath.toUtf8().data(), AZ::Uuid::CreateNull()));
            }
            response.m_createJobOutputs.push_back(jobDescriptor);
            m_createJobsCount++;
        }
    }

    void MockBuilderInfoHandler::ProcessJob(
        [[maybe_unused]] const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        response.m_resultCode = AssetBuilderSDK::ProcessJobResultCode::ProcessJobResult_Success;
    }

    AssetBuilderSDK::AssetBuilderDesc MockBuilderInfoHandler::CreateBuilderDesc(
        const QString& builderName, const QString& builderId, const AZStd::vector<AssetBuilderSDK::AssetBuilderPattern>& builderPatterns)
    {
        AssetBuilderSDK::AssetBuilderDesc builderDesc;

        builderDesc.m_name = builderName.toUtf8().data();
        builderDesc.m_patterns = builderPatterns;
        builderDesc.m_busId = AZ::Uuid::CreateString(builderId.toUtf8().data());
        builderDesc.m_builderType = AssetBuilderSDK::AssetBuilderDesc::AssetBuilderType::Internal;
        builderDesc.m_createJobFunction =
            AZStd::bind(&MockBuilderInfoHandler::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
        builderDesc.m_processJobFunction =
            AZStd::bind(&MockBuilderInfoHandler::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
        return builderDesc;
    }
}
