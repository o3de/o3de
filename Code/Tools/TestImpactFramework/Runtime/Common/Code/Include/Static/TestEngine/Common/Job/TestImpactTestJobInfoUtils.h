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
    //! Generates the path to the enumeration cache file for the specified test target.
    template<typename TestTarget>
    RepoPath GenerateTargetEnumerationCacheFilePath(const TestTarget* testTarget, const RepoPath& cacheDir)
    {
        return AZStd::string::format("%s.cache", (cacheDir / RepoPath(testTarget->GetName())).c_str());
    }

    //! Generates the path to the enumeration artifact file for the specified test target.
    template<typename TestTarget>
    RepoPath GenerateTargetEnumerationArtifactFilePath(const TestTarget* testTarget, const RepoPath& artifactDir)
    {
        return AZStd::string::format("%s.Enumeration.xml", (artifactDir / RepoPath(testTarget->GetName())).c_str());
    }

    //! Generates the path to the test run artifact file for the specified test target.
    template<typename TestTarget>
    RepoPath GenerateTargetRunArtifactFilePath(const TestTarget* testTarget, const RepoPath& artifactDir)
    {
        return AZStd::string::format("%s.Run.xml", (artifactDir / RepoPath(testTarget->GetName())).c_str());
    }

    //! Generates the path to the test coverage artifact file for the specified test target.
    template<typename TestTarget>
    RepoPath GenerateTargetCoverageArtifactFilePath(const TestTarget* testTarget, const RepoPath& artifactDir)
    {
        return AZStd::string::format("%s.Coverage.xml", (artifactDir / RepoPath(testTarget->GetName())).c_str());
    }
} // namespace TestImpact
