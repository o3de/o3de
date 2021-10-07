/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
    // Tag used to indicate whether a given line is the name or a covering test target
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

        // Add the newline so the deserializer can properly terminate on the last read line
        output += "\n";

        return output;
    }

    SourceCoveringTestsList DeserializeSourceCoveringTestsList(const AZStd::string& sourceCoveringTestsListString)
    {
        AZStd::vector<SourceCoveringTests> sourceCoveringTests;
        AZStd::string source;
        AZStd::vector<AZStd::string> coveringTests;
        sourceCoveringTests.reserve(1U << 16); // Reserve for approx. 65k source files
        const AZStd::string delim = "\n";
        size_t start = 0U;
        size_t end = sourceCoveringTestsListString.find(delim);

        while (end != AZStd::string::npos)
        {
            const auto line = sourceCoveringTestsListString.substr(start, end - start);
            if (line.starts_with(TargetTag))
            {
                // This is a test target covering the most recent source discovered
                coveringTests.push_back(line.substr(1, line.length() - 1));
            }
            else
            {
                // This is a new source file so assign the accumulated test targets to the current source file before proceeding
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

        // Ensure we properly assign the accumulated test targets to the most recent source discovered
        if (!coveringTests.empty())
        {
            sourceCoveringTests.push_back(SourceCoveringTests(source, AZStd::move(coveringTests)));
            coveringTests.clear();
        }

        return SourceCoveringTestsList(AZStd::move(sourceCoveringTests));
    }
} // namespace TestImpact
