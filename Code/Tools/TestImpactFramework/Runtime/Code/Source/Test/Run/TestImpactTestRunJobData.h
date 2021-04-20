/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#pragma once

#include <Test/Job/TestImpactTestJobRunner.h>

namespace TestImpact
{
    //! Per-job data for test runs.
    class TestRunJobData
    {
    public:
        TestRunJobData(const AZ::IO::Path& resultsArtifact);

        //! Returns the path to the test run artifact produced by the test target.
        const AZ::IO::Path& GetRunArtifactPath() const;

    private:
        AZ::IO::Path m_runArtifact; //!< Path to results data.
    };
} // namespace TestImpact
