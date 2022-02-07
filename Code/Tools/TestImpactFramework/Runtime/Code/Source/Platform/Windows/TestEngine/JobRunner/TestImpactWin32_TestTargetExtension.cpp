/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Target/TestImpactTestTarget.h>
#include <TestEngine/TestImpactTestEngineException.h>
#include <TestEngine/JobRunner/TestImpactTestTargetExtension.h>

namespace TestImpact
{
    AZStd::string GetTestTargetExtension(const TestTarget* testTarget)
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
