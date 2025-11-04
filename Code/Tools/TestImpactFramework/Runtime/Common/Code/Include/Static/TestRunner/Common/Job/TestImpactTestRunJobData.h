/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactRepoPath.h>

namespace TestImpact
{
    //! Per-job data for test runs.
    class TestRunJobData
    {
    public:
        TestRunJobData(const RepoPath& resultsArtifact);

        //! Copy and move constructors/assignment operators.
        TestRunJobData(const TestRunJobData& other);
        TestRunJobData(TestRunJobData&& other);
        TestRunJobData& operator=(const TestRunJobData& other);
        TestRunJobData& operator=(TestRunJobData&& other);

        //! Returns the path to the test run artifact produced by the test target.
        const RepoPath& GetRunArtifactPath() const;

    private:
        RepoPath m_runArtifact; //!< Path to results data.
    };
} // namespace TestImpact
