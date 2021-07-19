/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Target/TestImpactTestTarget.h>
#include <TestEngine/JobRunner/TestImpactTestJobInfoGenerator.h>
#include <TestEngine/JobRunner/TestImpactTestTargetExtension.h>

namespace TestImpact
{
    TestJobInfoGenerator::TestJobInfoGenerator(
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

    AZStd::string TestJobInfoGenerator::GenerateLaunchArgument(const TestTarget* testTarget) const
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

    RepoPath TestJobInfoGenerator::GenerateTargetEnumerationCacheFilePath(const TestTarget* testTarget) const
    {
        return AZStd::string::format("%s.cache", (m_cacheDir / RepoPath(testTarget->GetName())).c_str());
    }

    RepoPath TestJobInfoGenerator::GenerateTargetEnumerationArtifactFilePath(const TestTarget* testTarget) const
    {
        return AZStd::string::format("%s.Enumeration.xml", (m_artifactDir / RepoPath(testTarget->GetName())).c_str());
    }

    RepoPath TestJobInfoGenerator::GenerateTargetRunArtifactFilePath(const TestTarget* testTarget) const
    {
        return AZStd::string::format("%s.Run.xml", (m_artifactDir / RepoPath(testTarget->GetName())).c_str());
    }

    RepoPath TestJobInfoGenerator::GenerateTargetCoverageArtifactFilePath(const TestTarget* testTarget) const
    {
        return AZStd::string::format("%s.Coverage.xml", (m_artifactDir / RepoPath(testTarget->GetName())).c_str());
    }

    TestEnumerator::JobInfo TestJobInfoGenerator::GenerateTestEnumerationJobInfo(
        const TestTarget* testTarget,
        TestEnumerator::JobInfo::Id jobId,
        TestEnumerator::JobInfo::CachePolicy cachePolicy) const
    {
        using Command = TestEnumerator::Command;
        using JobInfo = TestEnumerator::JobInfo;
        using JobData = TestEnumerator::JobData;
        using Cache = TestEnumerator::JobData::Cache;

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

    TestRunner::JobInfo TestJobInfoGenerator::GenerateRegularTestRunJobInfo(
        const TestTarget* testTarget,
        TestRunner::JobInfo::Id jobId) const
    {
        using Command = TestRunner::Command;
        using JobInfo = TestRunner::JobInfo;
        using JobData = TestRunner::JobData;

        const auto runArtifact = GenerateTargetRunArtifactFilePath(testTarget);
        const Command args =
        {
            AZStd::string::format(
            "%s --gtest_output=xml:\"%s\"",
            GenerateLaunchArgument(testTarget).c_str(),
            runArtifact.c_str())
        };
    
        return JobInfo(jobId, args, JobData(runArtifact));
    }
    
    InstrumentedTestRunner::JobInfo TestJobInfoGenerator::GenerateInstrumentedTestRunJobInfo(
        const TestTarget* testTarget,
        InstrumentedTestRunner::JobInfo::Id jobId,
        CoverageLevel coverageLevel) const
    {
        using Command = InstrumentedTestRunner::Command;
        using JobInfo = InstrumentedTestRunner::JobInfo;
        using JobData = InstrumentedTestRunner::JobData;

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
    
        return JobInfo(jobId, args, JobData(runArtifact, coverageArtifact));
    }

    AZStd::vector<TestEnumerator::JobInfo> TestJobInfoGenerator::GenerateTestEnumerationJobInfos(
        const AZStd::vector<const TestTarget*>& testTargets,
        TestEnumerator::JobInfo::CachePolicy cachePolicy) const
    {
        AZStd::vector<TestEnumerator::JobInfo> jobInfos;
        jobInfos.reserve(testTargets.size());
        for (size_t jobId = 0; jobId < testTargets.size(); jobId++)
        {
            jobInfos.push_back(GenerateTestEnumerationJobInfo(testTargets[jobId], { jobId }, cachePolicy));
        }

        return jobInfos;
    }

    AZStd::vector<TestRunner::JobInfo> TestJobInfoGenerator::GenerateRegularTestRunJobInfos(
        const AZStd::vector<const TestTarget*>& testTargets) const
    {
        AZStd::vector<TestRunner::JobInfo> jobInfos;
        jobInfos.reserve(testTargets.size());
        for (size_t jobId = 0; jobId < testTargets.size(); jobId++)
        {
            jobInfos.push_back(GenerateRegularTestRunJobInfo(testTargets[jobId], { jobId }));
        }

        return jobInfos;
    }

    AZStd::vector<InstrumentedTestRunner::JobInfo> TestJobInfoGenerator::GenerateInstrumentedTestRunJobInfos(
        const AZStd::vector<const TestTarget*>& testTargets,
        CoverageLevel coverageLevel) const
    {
        AZStd::vector<InstrumentedTestRunner::JobInfo> jobInfos;
        jobInfos.reserve(testTargets.size());
        for (size_t jobId = 0; jobId < testTargets.size(); jobId++)
        {
            jobInfos.push_back(GenerateInstrumentedTestRunJobInfo(testTargets[jobId], { jobId }, coverageLevel));
        }

        return jobInfos;
    }
}
