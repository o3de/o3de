/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestRunner/Common/Job/TestImpactTestJobInfoUtils.h>

namespace TestImpact
{
    //! Generates the path to the enumeration cache file for the specified test target.
    RepoPath GenerateTargetEnumerationCacheFilePath(const TestTarget* testTarget, const RepoPath& cacheDir)
    {
        return AZStd::string::format("%s.cache", (cacheDir / RepoPath(testTarget->GetName())).c_str());
    }

    //! Generates the path to the enumeration artifact file for the specified test target.
    RepoPath GenerateTargetEnumerationArtifactFilePath(const TestTarget* testTarget, const RepoPath& artifactDir)
    {
        return AZStd::string::format("%s.Enumeration.xml", (artifactDir / RepoPath(testTarget->GetName())).c_str());
    }

    //! Generates the path to the test run artifact file for the specified test target.
    RepoPath GenerateTargetRunArtifactFilePath(const TestTarget* testTarget, const RepoPath& artifactDir)
    {
        AZStd::string targetName;
        if (testTarget->GetNamespace().empty())
        {
            targetName = testTarget->GetName().c_str();
        }
        else
        {
            targetName = AZStd::string::format("%s_%s", testTarget->GetNamespace().c_str(), testTarget->GetName().c_str());
        }

        return AZStd::string::format("%s.xml", (artifactDir / RepoPath(targetName)).c_str());
    }

    //! Generates the path to the test coverage artifact file for the specified test target.
    RepoPath GenerateTargetCoverageArtifactFilePath(const TestTarget* testTarget, const RepoPath& artifactDir)
    {
        return AZStd::string::format("%s.xml", (artifactDir / RepoPath(testTarget->GetName())).c_str());
    }
} // namespace TestImpact
