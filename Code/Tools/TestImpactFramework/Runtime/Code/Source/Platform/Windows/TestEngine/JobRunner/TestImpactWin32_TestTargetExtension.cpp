/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include <Target/TestImpactTestTarget.h>
#include <TestEngine/TestImpactTestEngineException.h>
#include <TestEngine/JobRunner/TestImpactTestTargetExtension.h>

namespace TestImpact
{
    AZStd::string GetTestTargetExtension(const TestTarget* testTarget)
    {
        static constexpr char* const standAloneExtension = ".exe"; 
        static constexpr char* const testRunnerExtension = ".dll"; 

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
}
