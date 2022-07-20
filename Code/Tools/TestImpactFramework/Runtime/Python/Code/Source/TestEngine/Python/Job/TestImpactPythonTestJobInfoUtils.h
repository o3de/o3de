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
    class PythonTestTarget;

    //! Generates the command string to launch the specified test target.
    RepoPath GenerateTestScriptPath(const PythonTestTarget* testTarget, const RepoPath& repoDir);
} // namespace TestImpact
