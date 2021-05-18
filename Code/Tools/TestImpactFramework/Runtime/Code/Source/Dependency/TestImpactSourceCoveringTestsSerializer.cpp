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

#include <Dependency/TestImpactDependencyException.h>
#include <Dependency/TestImpactSourceCoveringTestsSerializer.h>

#include <AzCore/JSON/document.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/stringbuffer.h>

namespace TestImpact
{
    AZStd::string SerializeSourceCoveringTestsList(const SourceCoveringTestsList& sourceCoveringTestsList)
    {
        AZStd::string output;
        output.reserve(1U << 24); // Reserve approx. 16Mib as the outputs can be quite large

        for (const auto& source : sourceCoveringTestsList.GetCoverage())
        {
            // Sorce file
            output += AZStd::string::format("%s,%u\n", source.GetPath().c_str(), source.GetNumCoveringTestTargets());

            // Covering test targets
            for (const auto& testTarget : source.GetCoveringTestTargets())
            {
                output += AZStd::string::format(" %s\n", testTarget.c_str());
            }
        }

        return output;
    }

    //SourceCoveringTestsList DeserializeSourceCoveringTestsList(const AZStd::string& sourceCoveringTestsListString)
    //{
    //
    //}
} // namespace TestImpact
