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

namespace TestImpact
{
    PythonTestRunnerBase::PythonTestRunnerBase(const RepoPath& artifactDir)
        : TestRunnerWithCoverage(1)
        , m_artifactDir(artifactDir)
    {
    }

    typename PythonTestRunnerBase::PayloadMap PythonTestRunnerBase::PayloadMapProducer(
        [[maybe_unused]] const typename PythonTestRunnerBase::JobDataMap& jobDataMap)
    {
        PayloadMap runs;

        // 1. Get all pycoverage files in artifact dir
        // 2. For each coverage file:
        //      Add the parent script as the key to a map
        //      Create module coverages with this test case covering
        //      Add module coverages to map key's vector of module coverages
        // 3. Walk the map:
        //      Create a PythonTestCoverage for each entry
        //      Add the PythonTestCoverage to the payload map


        /*
        VERSION 2

            MAKE A DIR FOR EACH TEST CASE'S TARGET NAME
            PYCOVERAGE GEM TAKES PARENT SCRIPT, LOOKS UP IN ALL.TESTS AND FINDS TARGET NAME
            PUT COVERAGE FILES IN TARGET NAME DIR
            HAVE TestRunWithCoverageJobData JOB DATA PUT TARGET NAME DIR AS ARTIFACT PATH
            PAYLOADEXTRACTOR READS ARTIFACT DIR, READS ALL FILES AND GENERATES COVERAGE FROM THAT
        */

        return runs;
    }
} // namespace TestImpact
