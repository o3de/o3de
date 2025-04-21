/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactRepoPath.h>

#include <Target/Common/TestImpactTestTarget.h>

namespace TestImpact
{
    //! Generates the fully qualified target name (`namespace::name` or `name`) for use in artifact files.
    RepoPath GenerateFullQualifiedTargetNameStem(const TestTarget* testTarget);

    //! Generates the path to the enumeration cache file for the specified test target.
    RepoPath GenerateTargetEnumerationCacheFilePath(const TestTarget* testTarget, const RepoPath& cacheDir);

    //! Generates the path to the enumeration artifact file for the specified test target.
    RepoPath GenerateTargetEnumerationArtifactFilePath(const TestTarget* testTarget, const RepoPath& artifactDir);

    //! Generates the path to the test run artifact file for the specified test target.
    RepoPath GenerateTargetRunArtifactFilePath(const TestTarget* testTarget, const RepoPath& artifactDir);

    //! Generates the path to the test coverage artifact file for the specified test target.
    RepoPath GenerateTargetCoverageArtifactFilePath(const TestTarget* testTarget, const RepoPath& artifactDir);
} // namespace TestImpact
