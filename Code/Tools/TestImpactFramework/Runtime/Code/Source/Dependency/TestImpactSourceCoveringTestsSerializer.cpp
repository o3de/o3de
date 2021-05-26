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
    constexpr char TargetTag = '-';

    AZStd::string SerializeSourceCoveringTestsList(const SourceCoveringTestsList& sourceCoveringTestsList)
    {
        AZStd::string output;
        output.reserve(1U << 24); // Reserve approx. 16Mib as the outputs can be quite large

        for (const auto& source : sourceCoveringTestsList.GetCoverage())
        {
            // Source file
            output += source.GetPath().String();
            output += "\n";

            // Covering test targets
            for (const auto& testTarget : source.GetCoveringTestTargets())
            {
                output += AZStd::string::format("%c%s\n", TargetTag, testTarget.c_str());
            }
        }

        // NEEDED!
        output += "\n";

        return output;
    }

    SourceCoveringTestsList DeserializeSourceCoveringTestsList(const AZStd::string& sourceCoveringTestsListString)
    {
        AZStd::vector<SourceCoveringTests> sourceCoveringTests;
        sourceCoveringTests.reserve(1U << 16); // Reserve for approx. 65k source files
        const AZStd::string delim = "\n";
        auto start = 0U;
        auto end = sourceCoveringTestsListString.find(delim);
        AZStd::string source;
        AZStd::vector<AZStd::string> coveringTests;
        while (end != AZStd::string::npos)
        {
            const auto line = sourceCoveringTestsListString.substr(start, end - start);
            if (line.starts_with(TargetTag))
            {
                coveringTests.push_back(line.substr(1, line.length() - 1));
            }
            else
            {
                if (!coveringTests.empty())
                {
                    sourceCoveringTests.push_back(SourceCoveringTests(source, AZStd::move(coveringTests)));
                    coveringTests.clear();
                }

                source = line;
            }

            start = end + delim.length();
            end = sourceCoveringTestsListString.find(delim, start);
        }

        if (!coveringTests.empty())
        {
            sourceCoveringTests.push_back(SourceCoveringTests(source, AZStd::move(coveringTests)));
            coveringTests.clear();
        }

        return SourceCoveringTestsList(AZStd::move(sourceCoveringTests));
    }
} // namespace TestImpact
