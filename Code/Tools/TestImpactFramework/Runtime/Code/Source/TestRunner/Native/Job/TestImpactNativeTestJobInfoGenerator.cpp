/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Target/Native/TestImpactNativeTestTarget.h>
#include <TestRunner/Native/Job/TestImpactNativeTestJobInfoGenerator.h>
#include <TestRunner/Native/Job/TestImpactNativeTestTargetExtension.h>

namespace TestImpact
{
    NativeTestJobInfoGenerator::NativeTestJobInfoGenerator(
        const RepoPath& sourceDir,
        const RepoPath& targetBinaryDir,
        const RepoPath& cacheDir,
        const RepoPath& artifactDir,
        const RepoPath& testRunnerBinary,
        const RepoPath& instrumentBinary)
        : m_sourceDir(sourceDir)
        , m_targetBinaryDir(targetBinaryDir)
        , m_cacheDir(cacheDir)
        , m_artifactDir(artifactDir)
        , m_testRunnerBinary(testRunnerBinary)
        , m_instrumentBinary(instrumentBinary)
    {
    }

    AZStd::string NativeTestJobInfoGenerator::GenerateLaunchArgument(const NativeTestTarget* testTarget) const
    {
        if (testTarget->GetLaunchMethod() == LaunchMethod::StandAlone)
        {
            return AZStd::string::format(
                "%s%s %s",
                (m_targetBinaryDir / RepoPath(testTarget->GetOutputName())).c_str(),
                GetTestTargetExtension(testTarget).c_str(),
                testTarget->GetCustomArgs().c_str()).c_str();
        }
        else
        {
            return AZStd::string::format(
                "\"%s\" \"%s%s\" %s",
                m_testRunnerBinary.c_str(),
                (m_targetBinaryDir / RepoPath(testTarget->GetOutputName())).c_str(),
                GetTestTargetExtension(testTarget).c_str(),
                testTarget->GetCustomArgs().c_str()).c_str();
        }
    }

    RepoPath NativeTestJobInfoGenerator::GenerateTargetEnumerationCacheFilePath(const NativeTestTarget* testTarget) const
    {
        return AZStd::string::format("%s.cache", (m_cacheDir / RepoPath(testTarget->GetName())).c_str());
    }

    RepoPath NativeTestJobInfoGenerator::GenerateTargetEnumerationArtifactFilePath(const NativeTestTarget* testTarget) const
    {
        return AZStd::string::format("%s.Enumeration.xml", (m_artifactDir / RepoPath(testTarget->GetName())).c_str());
    }

    RepoPath NativeTestJobInfoGenerator::GenerateTargetRunArtifactFilePath(const NativeTestTarget* testTarget) const
    {
        return AZStd::string::format("%s.Run.xml", (m_artifactDir / RepoPath(testTarget->GetName())).c_str());
    }

    RepoPath NativeTestJobInfoGenerator::GenerateTargetCoverageArtifactFilePath(const NativeTestTarget* testTarget) const
    {
        return AZStd::string::format("%s.Coverage.xml", (m_artifactDir / RepoPath(testTarget->GetName())).c_str());
    }

    NativeTestEnumerator::JobInfo NativeTestJobInfoGenerator::GenerateTestEnumerationJobInfo(
        const NativeTestTarget* testTarget, NativeTestEnumerator::JobInfo::Id jobId, NativeTestEnumerator::JobInfo::CachePolicy cachePolicy) const
    {
        using Command = NativeTestEnumerator::Command;
        using JobInfo = NativeTestEnumerator::JobInfo;
        using JobData = NativeTestEnumerator::JobData;
        using Cache = NativeTestEnumerator::JobData::Cache;

        const auto enumerationArtifact = GenerateTargetEnumerationArtifactFilePath(testTarget);
        const Command args =
        {
            AZStd::string::format(
            "%s --gtest_list_tests --gtest_output=xml:\"%s\"",
            GenerateLaunchArgument(testTarget).c_str(),
            enumerationArtifact.c_str())
        };

        return JobInfo(jobId, args, JobData(enumerationArtifact, Cache{ cachePolicy, GenerateTargetEnumerationCacheFilePath(testTarget) }));
    }

    NativeRegularTestRunner::JobInfo NativeTestJobInfoGenerator::GenerateRegularTestRunJobInfo(
        const NativeTestTarget* testTarget, NativeRegularTestRunner::JobInfo::Id jobId) const
    {
        using Command = NativeRegularTestRunner::Command;
        using JobInfo = NativeRegularTestRunner::JobInfo;
        using JobData = NativeRegularTestRunner::JobData;

        const auto runArtifact = GenerateTargetRunArtifactFilePath(testTarget);
        const Command args =
        {
            AZStd::string::format(
            "%s --gtest_output=xml:\"%s\"",
            GenerateLaunchArgument(testTarget).c_str(),
            runArtifact.c_str())
        };
    
        return JobInfo(jobId, args, JobData(testTarget->GetLaunchMethod(), runArtifact));
    }
    
    NativeInstrumentedTestRunner::JobInfo NativeTestJobInfoGenerator::GenerateInstrumentedTestRunJobInfo(
        const NativeTestTarget* testTarget, NativeInstrumentedTestRunner::JobInfo::Id jobId,
        CoverageLevel coverageLevel) const
    {
        using Command = NativeInstrumentedTestRunner::Command;
        using JobInfo = NativeInstrumentedTestRunner::JobInfo;
        using JobData = NativeInstrumentedTestRunner::JobData;

        const auto coverageArtifact = GenerateTargetCoverageArtifactFilePath(testTarget);
        const auto runArtifact = GenerateTargetRunArtifactFilePath(testTarget);
        const Command args =
        {
            AZStd::string::format(
            "\"%s\" "                                                           // 1. Instrumented test runner
            "--coverage_level %s "                                              // 2. Coverage level
            "--export_type cobertura:\"%s\" "                                   // 3. Test coverage artifact path
            "--modules \"%s\" "                                                 // 4. Modules path
            "--excluded_modules \"%s\" "                                        // 5. Exclude modules
            "--sources \"%s\" -- "                                              // 6. Sources path
            "%s "                                                               // 7. Launch command
            "--gtest_output=xml:\"%s\"",                                        // 8. Result artifact

            m_instrumentBinary.c_str(),                                         // 1. Instrumented test runner
            (coverageLevel == CoverageLevel::Line ? "line" : "source"),         // 2. Coverage level
            coverageArtifact.c_str(),                                           // 3. Test coverage artifact path
            m_targetBinaryDir.c_str(),                                          // 4. Modules path
            m_testRunnerBinary.c_str(),                                         // 5. Exclude modules
            m_sourceDir.c_str(),                                                // 6. Sources path
            GenerateLaunchArgument(testTarget).c_str(),                         // 7. Launch command
            runArtifact.c_str())                                                // 8. Result artifact
        };
    
        return JobInfo(jobId, args, JobData(testTarget->GetLaunchMethod(), runArtifact, coverageArtifact));
    }

    AZStd::vector<NativeTestEnumerator::JobInfo> NativeTestJobInfoGenerator::GenerateTestEnumerationJobInfos(
        const AZStd::vector<const NativeTestTarget*>& testTargets, NativeTestEnumerator::JobInfo::CachePolicy cachePolicy) const
    {
        AZStd::vector<NativeTestEnumerator::JobInfo> jobInfos;
        jobInfos.reserve(testTargets.size());
        for (size_t jobId = 0; jobId < testTargets.size(); jobId++)
        {
            jobInfos.push_back(GenerateTestEnumerationJobInfo(testTargets[jobId], { jobId }, cachePolicy));
        }

        return jobInfos;
    }

    AZStd::vector<NativeRegularTestRunner::JobInfo> NativeTestJobInfoGenerator::GenerateRegularTestRunJobInfos(
        const AZStd::vector<const NativeTestTarget*>& testTargets) const
    {
        AZStd::vector<NativeRegularTestRunner::JobInfo> jobInfos;
        jobInfos.reserve(testTargets.size());
        for (size_t jobId = 0; jobId < testTargets.size(); jobId++)
        {
            jobInfos.push_back(GenerateRegularTestRunJobInfo(testTargets[jobId], { jobId }));
        }

        return jobInfos;
    }

    AZStd::vector<NativeInstrumentedTestRunner::JobInfo> NativeTestJobInfoGenerator::GenerateInstrumentedTestRunJobInfos(
        const AZStd::vector<const NativeTestTarget*>& testTargets,
        CoverageLevel coverageLevel) const
    {
        AZStd::vector<NativeInstrumentedTestRunner::JobInfo> jobInfos;
        jobInfos.reserve(testTargets.size());
        for (size_t jobId = 0; jobId < testTargets.size(); jobId++)
        {
            jobInfos.push_back(GenerateInstrumentedTestRunJobInfo(testTargets[jobId], { jobId }, coverageLevel));
        }

        return jobInfos;
    }
}
