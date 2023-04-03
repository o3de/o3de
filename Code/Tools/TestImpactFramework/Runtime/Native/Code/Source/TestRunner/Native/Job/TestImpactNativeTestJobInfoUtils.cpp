/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Target/Native/TestImpactNativeTestTarget.h>
#include <TestRunner/Common/Job/TestImpactTestJobInfoUtils.h>
#include <TestEngine/Native/TestImpactNativeTestTargetExtension.h>
#include <TestRunner/Native/Job/TestImpactNativeTestJobInfoUtils.h>

namespace TestImpact
{
    AZStd::string GenerateLaunchArgument(
        const NativeTestTarget* testTarget, const RepoPath& targetBinaryDir, const RepoPath& testRunnerBinary)
    {
        if (testTarget->GetLaunchMethod() == LaunchMethod::StandAlone)
        {
            return AZStd::string::format(
                       "%s%s %s", (targetBinaryDir / RepoPath(testTarget->GetOutputName())).c_str(),
                       GetTestTargetExtension(testTarget).c_str(), testTarget->GetCustomArgs().c_str())
                .c_str();
        }
        else
        {
            return AZStd::string::format(
                       "\"%s\" \"%s%s\" %s", testRunnerBinary.c_str(),
                       (targetBinaryDir / RepoPath(testTarget->GetOutputName())).c_str(), GetTestTargetExtension(testTarget).c_str(),
                       testTarget->GetCustomArgs().c_str())
                .c_str();
        }
    }

    NativeTestEnumerator::Command GenerateTestEnumeratorJobInfoCommand(
        const AZStd::string& launchArguement, const RepoPath& runArtifact)
    {
        const auto regularCommand = GenerateRegularTestJobInfoCommand(launchArguement, runArtifact);
        return { AZStd::string::format("%s --gtest_list_tests", regularCommand.m_args.c_str()) };
    }

    NativeRegularTestRunner::Command GenerateRegularTestJobInfoCommand(
        const AZStd::string& launchArguement,
        const RepoPath& runArtifact
    )
    {
        return { AZStd::string::format("%s --gtest_output=xml:\"%s\"", launchArguement.c_str(), runArtifact.c_str()) };
    }
} // namespace TestImpact
