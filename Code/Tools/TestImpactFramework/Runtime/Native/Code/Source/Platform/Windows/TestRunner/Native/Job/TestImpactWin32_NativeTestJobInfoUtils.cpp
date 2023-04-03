/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestRunner/Native/Job/TestImpactNativeTestJobInfoUtils.h>

namespace TestImpact
{
    NativeInstrumentedTestRunner::Command GenerateInstrumentedTestJobInfoCommand(
        const RepoPath& instrumentBindaryPath,
        const RepoPath& coverageArtifactPath,
        CoverageLevel coverageLevel,
        const RepoPath& modulesPath,
        const RepoPath& excludedModulesPath,
        const RepoPath& sourcesPath,
        const NativeRegularTestRunner::Command& testRunLaunchCommand)
    {
        return {
            AZStd::string::format(
                "\"%s\" " // 1. Instrumented test runner
                "--coverage_level %s " // 2. Coverage level
                "--export_type cobertura:\"%s\" " // 3. Test coverage artifact path
                "--modules \"%s\" " // 4. Modules path
                "--excluded_modules \"%s\" " // 5. Exclude modules
                "--sources \"%s\" -- " // 6. Sources path
                "%s ", // 7. Launch command

                instrumentBindaryPath.c_str(), // 1. Instrumented test runner
                (coverageLevel == CoverageLevel::Line ? "line" : "source"), // 2. Coverage level
                coverageArtifactPath.c_str(), // 3. Test coverage artifact path
                modulesPath.c_str(), // 4. Modules path
                excludedModulesPath.c_str(), // 5. Exclude modules
                sourcesPath.c_str(), // 6. Sources path
                testRunLaunchCommand.m_args.c_str() // 7. Launch command
            )
        };
    }
} // namespace TestImpact
