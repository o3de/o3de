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

#include <Dependency/TestImpactDynamicDependencyMap.h>
#include <Dependency/TestImpactDependencyException.h>

namespace TestImpact
{
    DynamicDependencyMap::DynamicDependencyMap(
        AZStd::vector<ProductionTargetDescriptor>&& productionTargetDescriptors,
        AZStd::vector<TestTargetDescriptor>&& testTargetDescriptors)
        : m_productionTargets(AZStd::move(productionTargetDescriptors))
        , m_testTargets(AZStd::move(testTargetDescriptors))
    {
        const auto mapBuildTargetSources = [this](const auto* target)
        {
            for (const auto& source : target->GetSources().m_staticSources)
            {
                if (auto mapping = m_sourceDependencyMap.find(source.String());
                    mapping != m_sourceDependencyMap.end())
                {
                    // This is an existing entry in the dependency map so update the parent build targets with this target
                    mapping->second.m_parentTargets.insert(target);
                }
                else
                {
                    // This is a new entry on the dependency map so create an entry with this parent target and no covering targets
                    m_sourceDependencyMap.emplace(source, DependencyData{ {target}, {} });
                }
            }

            // Populate the autogen input to output mapping with any autogen sources
            for (const auto& autogen : target->GetSources().m_autogenSources)
            {
                for (const auto& output : autogen.m_outputs)
                {
                    m_autogenInputToOutputMap[autogen.m_input.String()].push_back(output.String());
                }
            }
        };

        for (const auto& target : m_productionTargets.GetTargets())
        {
            mapBuildTargetSources(&target);
        }

        for (const auto& target : m_testTargets.GetTargets())
        {
            mapBuildTargetSources(&target);
        }
    }

    size_t DynamicDependencyMap::GetNumTargets() const
    {
        return m_productionTargets.GetNumTargets() + m_testTargets.GetNumTargets();
    }

    size_t DynamicDependencyMap::GetNumSources() const
    {
        return m_sourceDependencyMap.size();
    }

    const BuildTarget* DynamicDependencyMap::GetBuildTarget(const AZStd::string& name) const
    {
        const BuildTarget* buildTarget = nullptr;
        AZStd::visit([&buildTarget](auto&& target)
        {
            if constexpr (IsProductionTarget<decltype(target)> || IsTestTarget<decltype(target)>)
            {
                buildTarget = target;
            }

        }, GetTarget(name));

        return buildTarget;
    }

    const BuildTarget* DynamicDependencyMap::GetBuildTargetOrThrow(const AZStd::string& name) const
    {
        const BuildTarget* buildTarget = nullptr;
        AZStd::visit([&buildTarget](auto&& target)
        {
            if constexpr (IsProductionTarget<decltype(target)> || IsTestTarget<decltype(target)>)
            {
                buildTarget = target;
            }
        }, GetTargetOrThrow(name));

        return buildTarget;
    }

    OptionalTarget DynamicDependencyMap::GetTarget(const AZStd::string& name) const
    {
        if (const auto testTarget = m_testTargets.GetTarget(name);
            testTarget != nullptr)
        {
            return testTarget;
        }
        else if (auto productionTarget = m_productionTargets.GetTarget(name);
            productionTarget != nullptr)
        {
            return productionTarget;
        }

        return AZStd::monostate{};
    }

    Target DynamicDependencyMap::GetTargetOrThrow(const AZStd::string& name) const
    {
        Target buildTarget;
        AZStd::visit([&buildTarget, &name](auto&& target)
        {
            if constexpr (IsProductionTarget<decltype(target)> || IsTestTarget<decltype(target)>)
            {
                buildTarget = target;
            }
            else
            {
                throw(TargetException(AZStd::string::format("Couldn't find target %s", name.c_str()).c_str()));
            }
        }, GetTarget(name));

        return buildTarget;
    }

    void DynamicDependencyMap::ReplaceSourceCoverage(const SourceCoveringTestsList& sourceCoverageDelta)
    {
        for (const auto& sourceCoverage : sourceCoverageDelta.GetCoverage())
        {
            // Autogen input files are not compiled sources and thus supplying coverage data for them makes no sense
            AZ_TestImpact_Eval(
                m_autogenInputToOutputMap.find(sourceCoverage.GetPath().String()) == m_autogenInputToOutputMap.end(),
                DependencyException, AZStd::string::format("Couldn't replace source coverage for %s, source file is an autogen input file",
                    sourceCoverage.GetPath().c_str()).c_str());

            auto [it, inserted] = m_sourceDependencyMap.insert(sourceCoverage.GetPath().String());
            auto& [key, sourceDependency] = *it;

            // Clear any existing coverage for the delta
            sourceDependency.m_coveringTestTargets.clear();

            // Update the dependency with any new coverage data
            for (const auto& unresolvedTestTarget : sourceCoverage.GetCoveringTestTargets())
            {
                if (const TestTarget* testTarget = m_testTargets.GetTarget(unresolvedTestTarget);
                    testTarget)
                {
                    // Source to covering test target mapping
                    sourceDependency.m_coveringTestTargets.insert(testTarget);

                    // Build target to covering test target mapping
                    for (const auto& parentTarget : sourceDependency.m_parentTargets)
                    {
                        m_buildTargetCoverage[parentTarget.GetBuildTarget()].insert(testTarget);
                    }
                }
                else
                {
                    AZ_Warning("ReplaceSourceCoverage", false, AZStd::string::format("Test target %s exists in the coverage data "
                        "but has since been removed from the build system", unresolvedTestTarget.c_str()).c_str());
                }
            }

            // If the new coverage data results in a parentless and coverageless entry, consider it a dead entry and remove accordingly
            if (sourceDependency.m_coveringTestTargets.empty() && sourceDependency.m_parentTargets.empty())
            {
                m_sourceDependencyMap.erase(it);
            }
        }
    }

    void DynamicDependencyMap::ClearSourceCoverage(const AZStd::vector<RepoPath>& paths)
    {
        for (const auto& path : paths)
        {
            if (const auto outputSources = m_autogenInputToOutputMap.find(path.String());
                outputSources != m_autogenInputToOutputMap.end())
            {
                // Clearing the coverage data of an autogen input source instead clears the coverage data of its output sources
                for (const auto& outputSource : outputSources->second)
                {
                    ReplaceSourceCoverage(SourceCoveringTestsList(AZStd::vector<SourceCoveringTests>{ SourceCoveringTests(RepoPath(outputSource)) }));
                }
            }
            else
            {
                ReplaceSourceCoverage(SourceCoveringTestsList(AZStd::vector<SourceCoveringTests>{ SourceCoveringTests(RepoPath(path)) }));
            }
        }
    }

    const ProductionTargetList& DynamicDependencyMap::GetProductionTargetList() const
    {
        return m_productionTargets;
    }

    const TestTargetList& DynamicDependencyMap::GetTestTargetList() const
    {
        return m_testTargets;
    }

    AZStd::vector<const TestTarget*> DynamicDependencyMap::GetCoveringTestTargetsForProductionTarget(const ProductionTarget& productionTarget) const
    {
        AZStd::vector<const TestTarget*> coveringTestTargets;
        if (const auto coverage = m_buildTargetCoverage.find(&productionTarget);
            coverage != m_buildTargetCoverage.end())
        {
            coveringTestTargets.reserve(coverage->second.size());
            AZStd::copy(coverage->second.begin(), coverage->second.end(), AZStd::back_inserter(coveringTestTargets));
        }

        return coveringTestTargets;
    }

    AZStd::optional<SourceDependency> DynamicDependencyMap::GetSourceDependency(const RepoPath& path) const
    {
        AZStd::unordered_set<ParentTarget> parentTargets;
        AZStd::unordered_set<const TestTarget*> coveringTestTargets;

        const auto getSourceDependency = [&parentTargets, &coveringTestTargets, this](const AZStd::string& path)
        {
            const auto sourceDependency = m_sourceDependencyMap.find(path);
            if (sourceDependency != m_sourceDependencyMap.end())
            {
                for (const auto& parentTarget : sourceDependency->second.m_parentTargets)
                {
                    parentTargets.insert(parentTarget);
                }

                for (const auto& testTarget : sourceDependency->second.m_coveringTestTargets)
                {
                    coveringTestTargets.insert(testTarget);
                }
            }
        };

        if (const auto outputSources = m_autogenInputToOutputMap.find(path.String()); outputSources != m_autogenInputToOutputMap.end())
        {
            // Consolidate the parentage and coverage of each of the autogen input file's generated output files
            for (const auto& outputSource : outputSources->second)
            {
                getSourceDependency(outputSource);
            }
        }
        else
        {
            getSourceDependency(path.String());
        }

        if (!parentTargets.empty() || !coveringTestTargets.empty())
        {
            return SourceDependency(path, DependencyData{ AZStd::move(parentTargets), AZStd::move(coveringTestTargets) });
        }

        return AZStd::nullopt;
    }

    SourceDependency DynamicDependencyMap::GetSourceDependencyOrThrow(const RepoPath& path) const
    {
        auto sourceDependency = GetSourceDependency(path);
        AZ_TestImpact_Eval(sourceDependency.has_value(), DependencyException, AZStd::string::format("Couldn't find source %s", path.c_str()).c_str());
        return sourceDependency.value();
    }

    SourceCoveringTestsList DynamicDependencyMap::ExportSourceCoverage() const
    {
        AZStd::vector<SourceCoveringTests> coverage;
        for (const auto& [path, dependency] : m_sourceDependencyMap)
        {
            AZStd::vector<AZStd::string> souceCoveringTests;
            for (const auto& testTarget : dependency.m_coveringTestTargets)
            {
                souceCoveringTests.push_back(testTarget->GetName());
            }

            coverage.push_back(SourceCoveringTests(RepoPath(path), AZStd::move(souceCoveringTests)));
        }

        return SourceCoveringTestsList(AZStd::move(coverage));
    }

    AZStd::vector<AZStd::string> DynamicDependencyMap::GetOrphanSourceFiles() const
    {
        AZStd::vector<AZStd::string> orphans;
        for (const auto& [source, dependency] : m_sourceDependencyMap)
        {
            if (dependency.m_parentTargets.empty())
            {
                orphans.push_back(source);
            }
        }

        return orphans;
    }

    ChangeDependencyList DynamicDependencyMap::ApplyAndResoveChangeList(const ChangeList& changeList)
    {
        AZStd::vector<SourceDependency> createDependencies;
        AZStd::vector<SourceDependency> updateDependencies;
        AZStd::vector<SourceDependency> deleteDependencies;

        // Keep track of the coverage to delete as a post step rather than deleting it in situ so that erroneous change lists
        // do not corrupt the dynamic dependency map
        AZStd::vector<RepoPath> coverageToDelete;

        // Create operations
        for (const auto& createdFile : changeList.m_createdFiles)
        {
            auto sourceDependency = GetSourceDependency(createdFile);
            if (sourceDependency.has_value())
            {
                if (sourceDependency->GetNumCoveringTestTargets())
                {
                    const AZStd::string msg = AZStd::string::format("The newly-created file %s belongs to a build target yet "
                        "still has coverage data in the source covering test list implying that a delete CRUD operation has been "
                        "missed, thus the integrity of the source covering test list has been compromised", createdFile.c_str());
                    AZ_Error("File Creation", false, msg.c_str());
                    throw DependencyException(msg);
                }

                if (sourceDependency->GetNumParentTargets())
                {
                    createDependencies.emplace_back(AZStd::move(*sourceDependency));
                }
            }
        }
    
        // Update operations
        for (const auto& updatedFile : changeList.m_updatedFiles)
        {
            auto sourceDependency = GetSourceDependency(updatedFile);
            if (sourceDependency.has_value())
            {
                if (sourceDependency->GetNumParentTargets())
                {
                    updateDependencies.emplace_back(AZStd::move(*sourceDependency));
                }
                else
                {
                    if (sourceDependency->GetNumCoveringTestTargets())
                    {
                        AZ_Warning(
                            "File Update", false, AZStd::string::format("Source file %s is potentially an orphan (used by build targets "
                            "without explicitly being added to the build system, e.g. an include directive pulling in a header from the "
                            "repository). Running the covering tests for this file with instrumentation will confirm whether or nor this "
                            "is the case", updatedFile.c_str()).c_str());

                        updateDependencies.emplace_back(AZStd::move(*sourceDependency));
                        coverageToDelete.push_back(updatedFile);
                    }
                }
            }
        }
    
        // Delete operations
        for (const auto& deletedFile : changeList.m_deletedFiles)
        {
            auto sourceDependency = GetSourceDependency(deletedFile);
            if (!sourceDependency.has_value())
            {
                continue;
            }

            if (sourceDependency->GetNumParentTargets())
            {
                if (sourceDependency->GetNumCoveringTestTargets())
                {
                    const AZStd::string msg = AZStd::string::format("The deleted file %s still belongs to a build target and still "
                        "has coverage data in the source covering test list, implying that the integrity of both the source to target "
                        "mappings and the source covering test list has been compromised", deletedFile.c_str());
                    AZ_Error("File Delete", false, msg.c_str());
                    throw DependencyException(msg);
                }
                else
                {
                    const AZStd::string msg = AZStd::string::format("The deleted file %s still belongs to a build target implying "
                        "that the integrity of the source to target mappings has been compromised", deletedFile.c_str());
                    AZ_Error("File Delete", false, msg.c_str());
                    throw DependencyException(msg);
                }
            }
            else
            {
                if (sourceDependency->GetNumCoveringTestTargets())
                {
                    deleteDependencies.emplace_back(AZStd::move(*sourceDependency));
                    coverageToDelete.push_back(deletedFile);
                }
            }
        }

        if (!coverageToDelete.empty())
        {
            ClearSourceCoverage(coverageToDelete);
        }

        return ChangeDependencyList(AZStd::move(createDependencies), AZStd::move(updateDependencies), AZStd::move(deleteDependencies));
    }
} // namespace TestImpact
