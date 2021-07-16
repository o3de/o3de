/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactRuntime.h>

#include <Artifact/Factory/TestImpactBuildTargetDescriptorFactory.h>
#include <Artifact/TestImpactArtifactException.h>

#include <AzCore/JSON/document.h>
#include <AzCore/std/string/regex.h>

namespace TestImpact
{
    AutogenSources PairAutogenSources(
        const AZStd::vector<RepoPath>& inputSources,
        const AZStd::vector<RepoPath>& outputSources,
        const AZStd::string& autogenMatcher)
    {
        AutogenSources autogenSources;
        const auto matcherPattern = AZStd::regex(autogenMatcher);
        AZStd::smatch inputMatches, outputMatches;

        // This has the potential to be optimized to O(n(n-1)/2) time complexity but to be perfectly honest it's not a serious
        // bottleneck right now and easier gains would be achieved by constructing build target artifacts in parallel rather than
        // trying to squeeze any more juice here as each build target is independent of one and other with no shared memory
        for (const auto& input : inputSources)
        {
            AutogenPairs autogenPairs;
            autogenPairs.m_input = input.String();
            const AZStd::string inputString = input.Stem().Native();
            if (AZStd::regex_search(inputString, inputMatches, matcherPattern))
            {
                for (const auto& output : outputSources)
                {
                    const AZStd::string outputString = output.Stem().Native();
                    if (AZStd::regex_search(outputString, outputMatches, matcherPattern))
                    {
                        // Note: [0] contains the whole match, [1] contains the first capture group
                        const auto& inputMatch = inputMatches[1];
                        const auto& outputMatch = outputMatches[1];
                        if (inputMatch == outputMatch)
                        {
                            autogenPairs.m_outputs.emplace_back(output);
                        }
                    }
                }
            }

            if (!autogenPairs.m_outputs.empty())
            {
                autogenSources.emplace_back(AZStd::move(autogenPairs));
            }
        }

        return autogenSources;
    }

    BuildTargetDescriptor BuildTargetDescriptorFactory(
        const AZStd::string& buildTargetData,
        const AZStd::vector<AZStd::string>& staticSourceExtensionIncludes,
        const AZStd::vector<AZStd::string>& autogenInputExtensionIncludes,
        const AZStd::string& autogenMatcher)
    {
        // Keys for pertinent JSON node and attribute names
        constexpr const char* Keys[] =
        {
            "target",
            "name",
            "output_name",
            "path",
            "sources",
            "static",
            "input",
            "output"
        };

        enum
        {
            TargetKey,
            NameKey,
            OutputNameKey,
            PathKey,
            SourcesKey,
            StaticKey,
            InputKey,
            OutputKey
        };

        AZ_TestImpact_Eval(!autogenMatcher.empty(), ArtifactException, "Autogen matcher cannot be empty");

        BuildTargetDescriptor buildTargetDescriptor;
        rapidjson::Document buildTarget;

        if (buildTarget.Parse(buildTargetData.c_str()).HasParseError())
        {
            throw TestImpact::ArtifactException("Could not parse build target data");
        }

        const auto& target = buildTarget[Keys[TargetKey]];
        buildTargetDescriptor.m_buildMetaData.m_name = target[Keys[NameKey]].GetString();
        buildTargetDescriptor.m_buildMetaData.m_outputName = target[Keys[OutputNameKey]].GetString();
        buildTargetDescriptor.m_buildMetaData.m_path = target["path"].GetString();

        AZ_TestImpact_Eval(!buildTargetDescriptor.m_buildMetaData.m_name.empty(), ArtifactException, "Target name cannot be empty");
        AZ_TestImpact_Eval(
            !buildTargetDescriptor.m_buildMetaData.m_outputName.empty(), ArtifactException, "Target output name cannot be empty");
        AZ_TestImpact_Eval(!buildTargetDescriptor.m_buildMetaData.m_path.empty(), ArtifactException, "Target path cannot be empty");

        const auto& sources = buildTarget[Keys[SourcesKey]];
        const auto& staticSources = sources[Keys[StaticKey]].GetArray();
        if (!staticSources.Empty())
        {
            buildTargetDescriptor.m_sources.m_staticSources = AZStd::vector<RepoPath>();

            for (const auto& source : staticSources)
            {
                const RepoPath sourcePath = RepoPath(source.GetString());
                if (AZStd::find(
                        staticSourceExtensionIncludes.begin(), staticSourceExtensionIncludes.end(), sourcePath.Extension().Native()) !=
                    staticSourceExtensionIncludes.end())
                {
                    buildTargetDescriptor.m_sources.m_staticSources.emplace_back(AZStd::move(sourcePath));
                }
            }
        }

        const auto& inputSources = buildTarget[Keys[SourcesKey]][Keys[InputKey]].GetArray();
        const auto& outputSources = buildTarget[Keys[SourcesKey]][Keys[OutputKey]].GetArray();
        if (!inputSources.Empty() || !outputSources.Empty())
        {
            AZ_TestImpact_Eval(
                !inputSources.Empty() && !outputSources.Empty(), ArtifactException, "Autogen malformed, input or output sources are empty");

            AZStd::vector<RepoPath> inputPaths;
            AZStd::vector<RepoPath> outputPaths;
            inputPaths.reserve(inputSources.Size());
            outputPaths.reserve(outputSources.Size());

            for (const auto& source : inputSources)
            {
                const RepoPath sourcePath = RepoPath(source.GetString());
                if (AZStd::find(
                        autogenInputExtensionIncludes.begin(), autogenInputExtensionIncludes.end(), sourcePath.Extension().Native()) !=
                    autogenInputExtensionIncludes.end())
                {
                    inputPaths.emplace_back(AZStd::move(sourcePath));
                }
            }

            for (const auto& source : outputSources)
            {
                outputPaths.emplace_back(AZStd::move(RepoPath(source.GetString())));
            }

            buildTargetDescriptor.m_sources.m_autogenSources = PairAutogenSources(inputPaths, outputPaths, autogenMatcher);
        }

        return buildTargetDescriptor;
    }
} // namespace TestImpact
