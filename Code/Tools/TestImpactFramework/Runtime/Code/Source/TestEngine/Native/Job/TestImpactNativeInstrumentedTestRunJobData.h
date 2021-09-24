/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestEngine/Native/Job/TestImpactNativeRegularTestRunJobData.h>

namespace TestImpact::Native
{
    //! Per-job data for instrumented test runs.
    class InstrumentedTestRunJobData
        : public TestRunJobData
    {
    public:
        InstrumentedTestRunJobData(const RepoPath& resultsArtifact, const RepoPath& coverageArtifact);

        //! Returns the path to the coverage artifact produced by the test target.
        const RepoPath& GetCoverageArtifactPath() const;

    private:
        RepoPath m_coverageArtifact; //!< Path to coverage data.
    };
} // namespace TestImpact
