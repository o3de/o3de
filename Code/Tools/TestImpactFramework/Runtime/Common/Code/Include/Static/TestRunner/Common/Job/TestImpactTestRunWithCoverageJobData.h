/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestRunner/Common/Job/TestImpactTestRunJobData.h>

namespace TestImpact
{
    //! Per-job data for test runs that also produce coverage artifacts.
    class TestRunWithCoverageJobData
        : public TestRunJobData
    {
    public:
        TestRunWithCoverageJobData(const RepoPath& resultsArtifact, const RepoPath& coverageArtifact);

        //! Copy and move constructors/assignment operators.
        TestRunWithCoverageJobData(const TestRunWithCoverageJobData& other);
        TestRunWithCoverageJobData(TestRunWithCoverageJobData&& other);
        TestRunWithCoverageJobData& operator=(const TestRunWithCoverageJobData& other);
        TestRunWithCoverageJobData& operator=(TestRunWithCoverageJobData&& other);

        //! Returns the path to the coverage artifact produced by the test target.
        const RepoPath& GetCoverageArtifactPath() const;

    private:
        RepoPath m_coverageArtifact; //!< Path to coverage data.
    };
} // namespace TestImpact
