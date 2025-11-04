/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestRunner/Common/Job/TestImpactTestJobInfoUtils.h>

namespace TestImpact
{
    RepoPath GenerateFullQualifiedTargetNameStem(const TestTarget* testTarget)
    {
        if (testTarget->GetNamespace().empty())
        {
            return RepoPath(testTarget->GetName().c_str());
        }
        else
        {
            return RepoPath(AZStd::string::format("%s_%s", testTarget->GetNamespace().c_str(), testTarget->GetName().c_str()));
        }
    }

    RepoPath GenerateTargetEnumerationCacheFilePath(const TestTarget* testTarget, const RepoPath& cacheDir)
    {
        return AZStd::string::format("%s.cache", (cacheDir / RepoPath(testTarget->GetName())).c_str());
    }

    RepoPath GenerateTargetEnumerationArtifactFilePath(const TestTarget* testTarget, const RepoPath& artifactDir)
    {
        return AZStd::string::format("%s.Enumeration.xml", (artifactDir / RepoPath(testTarget->GetName())).c_str());
    }

    RepoPath GenerateTargetRunArtifactFilePath(const TestTarget* testTarget, const RepoPath& artifactDir)
    {
        return AZStd::string::format("%s.xml", (artifactDir / GenerateFullQualifiedTargetNameStem(testTarget)).c_str());
    }

    RepoPath GenerateTargetCoverageArtifactFilePath(const TestTarget* testTarget, const RepoPath& artifactDir)
    {
        return AZStd::string::format("%s.xml", (artifactDir / RepoPath(testTarget->GetName())).c_str());
    }
} // namespace TestImpact
