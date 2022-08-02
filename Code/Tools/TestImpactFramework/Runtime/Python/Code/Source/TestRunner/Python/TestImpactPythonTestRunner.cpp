/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestRunner/Python/TestImpactPythonTestRunner.h>

namespace TestImpact
{
    PythonTestRunner::JobPayloadOutcome PythonTestRunner::PayloadExtractor([[maybe_unused]]const JobInfo& jobData, [[maybe_unused]]const JobMeta& jobMeta)
    {
    //    AZStd::optional<TestRun> run;
    //    try
    //    {
    //        run = TestRun(
    //            JUnit::TestRunSuitesFactory(ReadFileContents<TestRunnerException>(jobData.GetRunArtifactPath())),
    //            jobMeta.m_duration.value());
    //
    //        // Python tests have a separate coverage file per test case so we will attempt to parse each enumerated test case coverage
    //        TestCaseCoverage coverage;
    //        for (const auto& testSuite : run->GetTestSuites())
    //        {
    //            for (const auto& testCase : testSuite.m_tests)
    //            {
    //                const RepoPath covergeFile =
    //                    jobData.GetCoverageArtifactPath() / RepoPath(AZStd::string::format("%s.pycoverage", testCase.m_name.c_str()));
    //                coverage.emplace(
    //                    testCase.m_name,
    //                    TestCoverage(PythonCoverage::ModuleCoveragesFactory(ReadFileContents<TestRunnerException>(covergeFile))));
    //            }
    //        }
    //
    //        return AZ::Success(JobPayload{ run, AZStd::move(coverage) });
    //    } catch (const Exception& e)
    //    {
    //        return AZ::Failure(AZStd::string(e.what()));
    //    }
        return AZ::Failure(AZStd::string("Not implemented"));
    }
} // namespace TestImpact
