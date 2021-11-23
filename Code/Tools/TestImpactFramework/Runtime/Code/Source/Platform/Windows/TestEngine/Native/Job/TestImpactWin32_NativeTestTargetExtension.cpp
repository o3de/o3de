/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Target/Native/TestImpactNativeTestTarget.h>
#include <TestEngine/TestImpactTestEngineException.h>
#include <TestEngine/Native/Job/TestImpactNativeTestTargetExtension.h>

namespace TestImpact
{
    AZStd::string GetTestTargetExtension(const NativeTestTarget* testTarget)
    {
        static constexpr const char* const standAloneExtension = ".exe";
        static constexpr const char* const testRunnerExtension = ".dll"; 

        switch (const auto launchMethod = testTarget->GetLaunchMethod(); launchMethod)
        {
        case LaunchMethod::StandAlone:
        {
            return standAloneExtension;
        }
        case LaunchMethod::TestRunner:
        {
            return testRunnerExtension;
        }
        default:
        {
            throw TestEngineException(
                AZStd::string::format(
                    "Unexpected launch method for target %s: %u",
                    testTarget->GetName().c_str(),
                    aznumeric_cast<unsigned int>(launchMethod)));
        }
        }
    }
} // namespace TestImpact
