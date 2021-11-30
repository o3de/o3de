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
    class NativeTestTarget;

    //! Generates the command string to launch the specified test target.
    AZStd::string GenerateLaunchArgument(
        const NativeTestTarget* testTarget, const RepoPath& targetBinaryDir, const RepoPath& testRunnerBinary);

    //! Generates the path to the enumeration cache file for the specified test target.
    RepoPath GenerateTargetEnumerationCacheFilePath(const NativeTestTarget* testTarget, const RepoPath& cacheDir);

    //! Generates the path to the enumeration artifact file for the specified test target.
    RepoPath GenerateTargetEnumerationArtifactFilePath(const NativeTestTarget* testTarget, const RepoPath& artifactDir);

    //! Generates the path to the test run artifact file for the specified test target.
    RepoPath GenerateTargetRunArtifactFilePath(const NativeTestTarget* testTarget, const RepoPath& artifactDir);

    //! Generates the path to the test coverage artifact file for the specified test target.
    RepoPath GenerateTargetCoverageArtifactFilePath(const NativeTestTarget* testTarget, const RepoPath& artifactDir);
} // namespace TestImpact
