/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactRepoPath.h>
#include <TestImpactFramework/TestImpactConfiguration.h>

#include <TestRunner/Native/TestImpactNativeInstrumentedTestRunner.h>
#include <TestRunner/Native/TestImpactNativeRegularTestRunner.h>
#include <TestRunner/Native/TestImpactNativeTestEnumerator.h>

namespace TestImpact
{
    class NativeTestTarget;

    //! Generates the command string to launch the specified test target.
    AZStd::string GenerateLaunchArgument(
        const NativeTestTarget* testTarget, const RepoPath& targetBinaryDir, const RepoPath& testRunnerBinary);

    //! Generates a test enumeration job command.
    NativeTestEnumerator::Command GenerateTestEnumeratorJobInfoCommand(
        const AZStd::string& launchArguement, const RepoPath& runArtifact);

    //! Generates a regular test run job command.
    NativeRegularTestRunner::Command GenerateRegularTestJobInfoCommand(
        const AZStd::string& launchArguement,
        const RepoPath& runArtifact
    );

    //! Generates an instrumented test run job command.
    NativeInstrumentedTestRunner::Command GenerateInstrumentedTestJobInfoCommand(
        const RepoPath& instrumentBindaryPath,
        const RepoPath& coverageArtifactPath,
        CoverageLevel coverageLevel,
        const RepoPath& modulesPath,
        const RepoPath& excludedModulesPath,
        const RepoPath& sourcesPath,
        const typename NativeRegularTestRunner::Command& testRunLaunchCommand);
} // namespace TestImpact
