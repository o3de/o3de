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
    MockMultiBuilderInfoHandler::~MockMultiBuilderInfoHandler()
    {
        BusDisconnect();
    }

    void MockMultiBuilderInfoHandler::GetMatchingBuildersInfo(
        const AZStd::string& assetPath, AssetProcessor::BuilderInfoList& builderInfoList)
    {
        AZStd::set<AZ::Uuid> uniqueBuilderDescIDs;

        for (AssetUtilities::BuilderFilePatternMatcher& matcherPair : m_matcherBuilderPatterns)
        {
            if (uniqueBuilderDescIDs.find(matcherPair.GetBuilderDescID()) != uniqueBuilderDescIDs.end())
            {
                continue;
            }
            if (matcherPair.MatchesPath(assetPath))
            {
                const AssetBuilderSDK::AssetBuilderDesc& builderDesc = m_builderDescMap[matcherPair.GetBuilderDescID()];
                uniqueBuilderDescIDs.insert(matcherPair.GetBuilderDescID());
                builderInfoList.push_back(builderDesc);
            }
        }
    }

    void MockMultiBuilderInfoHandler::GetAllBuildersInfo([[maybe_unused]] AssetProcessor::BuilderInfoList& builderInfoList)
    {
        for (const auto& builder : m_builderDescMap)
        {
            builderInfoList.push_back(builder.second);
        }
    }

    void MockMultiBuilderInfoHandler::CreateJobs(
        AssetBuilderExtraInfo extraInfo, const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;

        for (const auto& platform : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor jobDescriptor;
            jobDescriptor.m_priority = 0;
            jobDescriptor.m_critical = true;
            jobDescriptor.m_jobKey = "Mock Job";
            jobDescriptor.SetPlatformIdentifier(platform.m_identifier.c_str());
            jobDescriptor.m_additionalFingerprintInfo = extraInfo.m_jobFingerprint.toUtf8().constData();

            if (!extraInfo.m_jobDependencyFilePath.isEmpty())
            {
                auto jobDependency = AssetBuilderSDK::JobDependency(
                    "Mock Job", "pc", AssetBuilderSDK::JobDependencyType::Order,
                    AssetBuilderSDK::SourceFileDependency(extraInfo.m_jobDependencyFilePath.toUtf8().constData(), AZ::Uuid::CreateNull()));

                if (!extraInfo.m_subIdDependencies.empty())
                {
                    jobDependency.m_productSubIds = extraInfo.m_subIdDependencies;
                }

                jobDescriptor.m_jobDependencyList.push_back(jobDependency);
            }

            if (!extraInfo.m_dependencyFilePath.isEmpty())
            {
                response.m_sourceFileDependencyList.push_back(
                    AssetBuilderSDK::SourceFileDependency(extraInfo.m_dependencyFilePath.toUtf8().constData(), AZ::Uuid::CreateNull()));
            }

            response.m_createJobOutputs.push_back(jobDescriptor);
            m_createJobsCount++;
        }
    }

    void MockMultiBuilderInfoHandler::ProcessJob(
        AssetBuilderExtraInfo extraInfo,
        [[maybe_unused]] const AssetBuilderSDK::ProcessJobRequest& request,
        AssetBuilderSDK::ProcessJobResponse& response)
    {
        response.m_resultCode = AssetBuilderSDK::ProcessJobResultCode::ProcessJobResult_Success;
    }

    void MockMultiBuilderInfoHandler::CreateBuilderDesc(
        const QString& builderName,
        const QString& builderId,
        const AZStd::vector<AssetBuilderSDK::AssetBuilderPattern>& builderPatterns,
        const AssetBuilderExtraInfo& extraInfo)
    {
        CreateBuilderDesc(
            builderName, builderId, builderPatterns,
            AZStd::bind(&MockMultiBuilderInfoHandler::CreateJobs, this, extraInfo, AZStd::placeholders::_1, AZStd::placeholders::_2),
            AZStd::bind(&MockMultiBuilderInfoHandler::ProcessJob, this, extraInfo, AZStd::placeholders::_1, AZStd::placeholders::_2), extraInfo.m_analysisFingerprint);
    }

    void MockMultiBuilderInfoHandler::CreateBuilderDescInfoRef(
        const QString& builderName,
        const QString& builderId,
        const AZStd::vector<AssetBuilderSDK::AssetBuilderPattern>& builderPatterns,
        const AssetBuilderExtraInfo& extraInfo)
    {
        CreateBuilderDesc(
            builderName, builderId, builderPatterns,
            AZStd::bind(&MockMultiBuilderInfoHandler::CreateJobs, this, AZStd::ref(extraInfo), AZStd::placeholders::_1, AZStd::placeholders::_2),
            AZStd::bind(&MockMultiBuilderInfoHandler::ProcessJob, this, AZStd::ref(extraInfo), AZStd::placeholders::_1, AZStd::placeholders::_2),
            extraInfo.m_analysisFingerprint);
    }

    void MockMultiBuilderInfoHandler::CreateBuilderDesc(
        const QString& builderName,
        const QString& builderId,
        const AZStd::vector<AssetBuilderSDK::AssetBuilderPattern>& builderPatterns,
        const AssetBuilderSDK::CreateJobFunction& createJobsFunction,
        const AssetBuilderSDK::ProcessJobFunction& processJobFunction,
        AZStd::optional<QString> analysisFingerprint)
    {
        AssetBuilderSDK::AssetBuilderDesc builderDesc;

        builderDesc.m_name = builderName.toUtf8().data();
        builderDesc.m_patterns = builderPatterns;
        builderDesc.m_busId = AZ::Uuid::CreateString(builderId.toUtf8().data());
        builderDesc.m_builderType = AssetBuilderSDK::AssetBuilderDesc::AssetBuilderType::Internal;
        builderDesc.m_createJobFunction = createJobsFunction;
        builderDesc.m_processJobFunction = processJobFunction;

        if (analysisFingerprint && !analysisFingerprint->isEmpty())
        {
            builderDesc.m_analysisFingerprint = analysisFingerprint.value().toUtf8().constData();
        }

        m_builderDescMap[builderDesc.m_busId] = builderDesc;

        for (const AssetBuilderSDK::AssetBuilderPattern& pattern : builderDesc.m_patterns)
        {
            AssetUtilities::BuilderFilePatternMatcher patternMatcher(pattern, builderDesc.m_busId);
            m_matcherBuilderPatterns.push_back(patternMatcher);
        }
    }

    TraceBusErrorChecker::TraceBusErrorChecker()
    {
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
    }

    TraceBusErrorChecker::~TraceBusErrorChecker()
    {
        EXPECT_FALSE(m_expectingFailure);
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
    }

    void TraceBusErrorChecker::Begin()
    {
        m_expectingFailure = true;
        m_suppressedMessages.clear();
    }

    void TraceBusErrorChecker::End(int expectedCount)
    {
        m_expectingFailure = false;

        if (expectedCount != m_suppressedMessages.size())
        {
            for (const auto& message : m_suppressedMessages)
            {
                AZ::Debug::Trace::Instance().Output("", message.c_str());
            }

            EXPECT_EQ(expectedCount, m_suppressedMessages.size());
        }
    }

    bool TraceBusErrorChecker::OnPreAssert(
        const char* fileName,
        int line,
        const char* func,
        const char* message)
    {
        RecordError(fileName, line, func, message);

        return m_expectingFailure;
    }

    bool TraceBusErrorChecker::OnPreError(
        [[maybe_unused]] const char* window,
        const char* fileName,
        int line,
        const char* func,
        const char* message)
    {
        RecordError(fileName, line, func, message);

        return m_expectingFailure;
    }

    bool TraceBusErrorChecker::OnPreWarning(
        [[maybe_unused]] const char* window,
        const char* fileName,
        int line,
        const char* func,
        const char* message)
    {
        RecordError(fileName, line, func, message);

        return m_expectingFailure;
    }

    void TraceBusErrorChecker::RecordError(const char* fileName, int line, const char* func, const char* message)
    {
        AZStd::string errorMessage = AZStd::string::format("%s(%d): %s - %s\n", fileName, line, func, message);

        if (!m_expectingFailure)
        {
            AZ::Debug::Trace::Instance().Output("", errorMessage.c_str());
            GTEST_NONFATAL_FAILURE_("Unexpected failure occurred");
        }

        m_suppressedMessages.push_back(errorMessage);
    }

    void MockFileStateCache::RegisterForDeleteEvent(AZ::Event<AssetProcessor::FileStateInfo>::Handler& handler)
    {
        handler.Connect(m_deleteEvent);
    }

    bool MockFileStateCache::GetHash(const QString& absolutePath, FileHash* foundHash)
    {
        if (!Exists(absolutePath))
        {
            return false;
        }

        *foundHash = AssetUtilities::GetFileHash(absolutePath.toUtf8().constData(), true);

        return true;
    }
} // namespace UnitTests
