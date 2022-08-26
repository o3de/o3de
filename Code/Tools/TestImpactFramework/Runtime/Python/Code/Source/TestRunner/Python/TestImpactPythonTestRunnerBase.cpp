/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactUtils.h>

#include <TestRunner/Python/TestImpactPythonTestRunnerBase.h>

#include <AzCore/std/containers/unordered_set.h>

namespace TestImpact
{
    PythonTestRunnerBase::PythonTestRunnerBase(const ArtifactDir& artifactDir)
        : TestRunnerWithCoverage(1)
        , m_artifactDir(artifactDir)
    {
    }

    PythonTestRunnerBase::JobPayloadOutcome PythonTestRunnerBase::PayloadExtractor(
        [[maybe_unused]] const JobInfo& jobData, [[maybe_unused]] const JobMeta& jobMeta)
    {
        AZStd::optional<TestRun> run;
        try
        {
            run = TestRun(
                JUnit::TestRunSuitesFactory(ReadFileContents<TestRunnerException>(jobData.GetRunArtifactPath())),
                AZStd::chrono::milliseconds{ 0 });
        }
        catch (const Exception& e)
        {
            // No run result is a failure as all Python tests will be exporting their results to JUnit format
            return AZ::Failure(AZStd::string(e.what()));
        }

        try
        {
            AZStd::unordered_set<AZStd::string> coveredModules;
            const auto testCaseFiles = ListFiles(jobData.GetCoverageArtifactPath(), "*.pycoverage");

            for (const auto& testCaseFile : testCaseFiles)
            {
                const auto coverage = PythonCoverage::ModuleCoveragesFactory(ReadFileContents<TestRunnerException>(testCaseFile));

                for (const auto& coveredModule : coverage.m_components)
                {
                    coveredModules.insert(coveredModule);
                }
            }

            AZStd::vector<ModuleCoverage> moduleCoverages;
            for (const auto& coveredModule : coveredModules)
            {
                moduleCoverages.emplace_back(coveredModule, AZStd::vector<SourceCoverage>{});
            }

            // No coverage is not a failure as not all Python tests are capable of producing coverage
            return AZ::Success(JobPayload{ run, TestCoverage(AZStd::move(moduleCoverages)) });
        }
        catch (const Exception& e)
        {
            return AZ::Failure(AZStd::string(e.what()));
        }
    }
} // namespace TestImpact
