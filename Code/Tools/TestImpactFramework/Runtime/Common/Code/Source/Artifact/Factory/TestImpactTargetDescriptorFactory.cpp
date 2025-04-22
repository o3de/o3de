/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Artifact/Factory/TestImpactTargetDescriptorFactory.h>
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

    TargetDescriptor TargetDescriptorFactory(
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
            "type",
            "output_name",
            "path",
            "sources",
            "static",
            "input",
            "output",
            "dependencies",
            "build",
            "runtime",
            "production",
            "test"
        };

        enum Fields
        {
            TargetKey,
            NameKey,
            TargetTypeKey,
            OutputNameKey,
            PathKey,
            SourcesKey,
            StaticKey,
            InputKey,
            OutputKey,
            DependenciesKey,
            BuildDependenciesKey,
            RuntimeDependenciesKey,
            ProductionTargetTypeKey,
            TestTargetTypeKey,
            // Checksum
            _CHECKSUM_
        };

        static_assert(Fields::_CHECKSUM_ == AZStd::size(Keys));
        AZ_TestImpact_Eval(!autogenMatcher.empty(), ArtifactException, "Autogen matcher cannot be empty");

        TargetDescriptor descriptor;
        rapidjson::Document buildTarget;

        if (buildTarget.Parse(buildTargetData.c_str()).HasParseError())
        {
            throw TestImpact::ArtifactException("Could not parse build target data");
        }

        const auto& target = buildTarget[Keys[TargetKey]];
        descriptor.m_name = target[Keys[NameKey]].GetString();
        descriptor.m_outputName = target[Keys[OutputNameKey]].GetString();
        descriptor.m_path = target[Keys[PathKey]].GetString();

        if (const auto& targetType = target[Keys[TargetTypeKey]];
            targetType == Keys[ProductionTargetTypeKey])
        {
            descriptor.m_type = TargetType::ProductionTarget;
        }
        else if (targetType == Keys[TestTargetTypeKey])
        {
            descriptor.m_type = TargetType::TestTarget;
        }
        else
        {
            throw(ArtifactException(AZStd::string::format("Unexpected target type '%s'", targetType.GetString())));
        }

        AZ_TestImpact_Eval(!descriptor.m_name.empty(), ArtifactException, "Target name cannot be empty");
        AZ_TestImpact_Eval(!descriptor.m_outputName.empty(), ArtifactException, "Target output name cannot be empty");
        AZ_TestImpact_Eval(!descriptor.m_path.empty(), ArtifactException, "Target path cannot be empty");

        const auto extractDependencies = [](const auto& dependenciesArray)
        {
            DependencyList dependencies;
            dependencies.reserve(dependenciesArray.Size());

            for (const auto& dependency : dependenciesArray)
            {
                dependencies.push_back(dependency.GetString());
            }

            return dependencies;
        };

        descriptor.m_dependencies.m_build = extractDependencies(target[Keys[DependenciesKey]][Keys[BuildDependenciesKey]].GetArray());
        descriptor.m_dependencies.m_runtime = extractDependencies(target[Keys[DependenciesKey]][Keys[RuntimeDependenciesKey]].GetArray());

        const auto& sources = buildTarget[Keys[SourcesKey]];
        const auto& staticSources = sources[Keys[StaticKey]].GetArray();
        if (!staticSources.Empty())
        {
            descriptor.m_sources.m_staticSources = AZStd::vector<RepoPath>();

            for (const auto& source : staticSources)
            {
                const RepoPath sourcePath = RepoPath(source.GetString());
                if (AZStd::find(
                        staticSourceExtensionIncludes.begin(), staticSourceExtensionIncludes.end(), sourcePath.Extension().Native()) !=
                    staticSourceExtensionIncludes.end())
                {
                    descriptor.m_sources.m_staticSources.emplace_back(AZStd::move(sourcePath));
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

            descriptor.m_sources.m_autogenSources = PairAutogenSources(inputPaths, outputPaths, autogenMatcher);
        }

        return descriptor;
    }
} // namespace TestImpact
