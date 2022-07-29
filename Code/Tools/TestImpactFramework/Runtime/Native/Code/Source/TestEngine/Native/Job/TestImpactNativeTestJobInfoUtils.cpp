/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Target/Native/TestImpactNativeTestTarget.h>
#include <TestEngine/Native/TestImpactNativeTestTargetExtension.h>
#include <TestEngine/Native/Job/TestImpactNativeTestJobInfoUtils.h>

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
} // namespace TestImpact
