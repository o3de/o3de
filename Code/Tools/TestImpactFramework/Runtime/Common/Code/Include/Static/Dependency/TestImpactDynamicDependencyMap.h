/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactChangeList.h>
#include <TestImpactFramework/TestImpactPolicy.h>

#include <Dependency/TestImpactDependencyException.h>
#include <Dependency/TestImpactSourceCoveringTestsList.h>
#include <Dependency/TestImpactSourceDependency.h>
#include <Dependency/TestImpactChangeDependencyList.h>
#include <BuildTarget/Common/TestImpactBuildTargetList.h>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>

namespace TestImpact
{
    //! Representation of the repository source tree and its relation to the build targets and coverage data.
    template<typename ProductionTarget, typename TestTarget>
    class DynamicDependencyMap
    {
    public:
        //! Constructs the dependency map with entries for each build target's source files with empty test coverage data.
        DynamicDependencyMap(const BuildTargetList<ProductionTarget, TestTarget>* buildTargetList);

        const BuildTargetList<ProductionTarget, TestTarget>* GetBuildTargetList() const;

        //! Gets the total number of unique source files in the repository.
        //! @note This includes autogen output sources.
        size_t GetNumSources() const;

        //! Gets the test targets covering the specified production target.
        //! @param productionTarget The production target to retrieve the covering tests for.
        AZStd::vector<const TestTarget*> GetCoveringTestTargetsForProductionTarget(const ProductionTarget& productionTarget) const;

        //! Returns the test targets that cover one or more sources in the repository.
        AZStd::vector<const TestTarget*> GetCoveringTests() const;

        //! Returns the test targets that do not cover any sources in the repository.
        AZStd::vector<const TestTarget*> GetNotCoveringTests() const;

        //! Returns all the production targets with test coverage.
        AZStd::unordered_set<const TestTarget*> GetCoveredProductionTargets() const;

        //! Gets the source dependency for the specified source file.
        //! @note Autogen input source dependencies are the consolidated source dependencies of all of their generated output sources.
        //! @returns If found, the source dependency information for the specified source file, otherwise empty.
        AZStd::optional<SourceDependency<ProductionTarget, TestTarget>> GetSourceDependency(const RepoPath& path) const;

        //! Gets the source dependency for the specified source file or throw DependencyException.
        SourceDependency<ProductionTarget, TestTarget> GetSourceDependencyOrThrow(const RepoPath& path) const;

        //! Replaces the source coverage of the specified sources with the specified source coverage.
        //! @param sourceCoverageDelta The source coverage delta to replace in the dependency map.
        void ReplaceSourceCoverage(const SourceCoveringTestsList& sourceCoverageDelta);

        //! Clears all of the existing source coverage in the dependency map.
        void ClearAllSourceCoverage();

        //! Exports the coverage of all sources in the dependency map.
        SourceCoveringTestsList ExportSourceCoverage() const;

        //! Gets the list of orphaned source files in the dependency map that have coverage data but belong to no parent build targets.
        AZStd::vector<AZStd::string> GetOrphanedSourceFiles() const;

        //! Applies the specified change list to the dependency map and resolves the change list to a change dependency list
        //! containing the updated source dependencies for each source file in the change list.
        //! @param changeList The change list to apply and resolve.
        //! @param integrityFailurePolicy The policy to use for handling integrity errors in the source dependency map.
        //! @returns The change list as resolved to the appropriate source dependencies.
        [[nodiscard]] ChangeDependencyList<ProductionTarget, TestTarget> ApplyAndResoveChangeList(
            const ChangeList& changeList, Policy::IntegrityFailure integrityFailurePolicy);

        //! Removes the specified test target from all source coverage.
        void RemoveTestTargetFromSourceCoverage(const TestTarget* testTarget);

    private:
        //! Internal handler for ReplaceSourceCoverage where the pruning of parentless and coverageless source depenencies after the
        //! source coverage has been replaced must be explicitly stated.
        //! @note The covered targets for the source dependency's parent test target(s) will not be pruned if those covering targets are removed.
        //! @param sourceCoverageDelta The source coverage delta to replace in the dependency map.
        //! @param pruneIfNoParentsOrCoverage Flag to specify whether or not newly parentless and coverageless dependencies will be removed.
        void ReplaceSourceCoverageInternal(const SourceCoveringTestsList& sourceCoverageDelta, bool pruneIfNoParentsOrCoverage);

        //! Clears the source coverage of the specified sources.
        //! @note The covering targets for the parent test target(s) will not be pruned if those covering targets are removed.
        void ClearSourceCoverage(const AZStd::vector<RepoPath>& paths);

        //! The dependency map of sources to their parent build targets and covering test targets.
        AZStd::unordered_map<AZStd::string, DependencyData<ProductionTarget, TestTarget>> m_sourceDependencyMap;

        //! Map of all test targets and the sources they cover.
        AZStd::unordered_map<const TestTarget*, AZStd::unordered_set<AZStd::string>> m_testTargetSourceCoverage;

        //! The map of build targets and their covering test targets.
        //! @note As per the note for ReplaceSourceCoverageInternal, this map is currently not pruned when source coverage is replaced.
        AZStd::unordered_map<BuildTarget<ProductionTarget, TestTarget>, AZStd::unordered_set<const TestTarget*>>
            m_buildTargetCoverage;

        //! Mapping of autogen input sources to their generated output sources.
        AZStd::unordered_map<AZStd::string, AZStd::vector<AZStd::string>> m_autogenInputToOutputMap;

        //! The list of build targets (both test and production) in the repository.
        const BuildTargetList<ProductionTarget, TestTarget>* m_buildTargetList;
    };

    template<typename ProductionTarget, typename TestTarget>
    DynamicDependencyMap<ProductionTarget, TestTarget>::DynamicDependencyMap(const BuildTargetList<ProductionTarget, TestTarget>* buildTargetList)
        : m_buildTargetList(buildTargetList)
    {
        const auto mapBuildTargetSources = [this](const auto* target)
        {
            for (const auto& source : target->GetSources().m_staticSources)
            {
                if (auto mapping = m_sourceDependencyMap.find(source.String()); mapping != m_sourceDependencyMap.end())
                {
                    // This is an existing entry in the dependency map so update the parent build targets with this target
                    mapping->second.m_parentTargets.insert(target);
                }
                else
                {
                    // This is a new entry on the dependency map so create an entry with this parent target and no covering targets
                    m_sourceDependencyMap.emplace(source.String(), DependencyData<ProductionTarget, TestTarget>{ { target }, { /* No covering test targets */ } });
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

        for (const auto& target : m_buildTargetList->GetProductionTargetList().GetTargets())
        {
            mapBuildTargetSources(&target);
        }

        for (const auto& target : m_buildTargetList->GetTestTargetList().GetTargets())
        {
            mapBuildTargetSources(&target);
            m_testTargetSourceCoverage[&target] = {};
        }
    }

    template<typename ProductionTarget, typename TestTarget>
    const BuildTargetList<ProductionTarget, TestTarget>* DynamicDependencyMap<ProductionTarget, TestTarget>::GetBuildTargetList() const
    {
        return m_buildTargetList;
    }

    template<typename ProductionTarget, typename TestTarget>
    size_t DynamicDependencyMap<ProductionTarget, TestTarget>::GetNumSources() const
    {
        return m_sourceDependencyMap.size();
    }

    template<typename ProductionTarget, typename TestTarget>
    void DynamicDependencyMap<ProductionTarget, TestTarget>::ReplaceSourceCoverageInternal(
        const SourceCoveringTestsList& sourceCoverageDelta, bool pruneIfNoParentsOrCoverage)
    {
        AZStd::vector<AZStd::string> killList;
        for (const auto& sourceCoverage : sourceCoverageDelta.GetCoverage())
        {
            // Autogen input files are not compiled sources and thus supplying coverage data for them makes no sense
            AZ_TestImpact_Eval(
                m_autogenInputToOutputMap.find(sourceCoverage.GetPath().String()) == m_autogenInputToOutputMap.end(), DependencyException,
                AZStd::string::format(
                    "Couldn't replace source coverage for %s, source file is an autogen input file", sourceCoverage.GetPath().c_str())
                    .c_str());

            auto [sourceDependencyIt, inserted] = m_sourceDependencyMap.insert(sourceCoverage.GetPath().String());
            auto& [source, sourceDependency] = *sourceDependencyIt;

            // Before we can replace the coverage for this source dependency, we must:
            // 1. Remove the source from the test target covering sources map
            // 2. Prune the covered targets for the parent test target(s) of this source dependency
            // 3. Clear any existing coverage for the delta

            // 1.
            for (const auto& testTarget : sourceDependency.m_coveringTestTargets)
            {
                if (auto coveringTestTargetIt = m_testTargetSourceCoverage.find(testTarget);
                    coveringTestTargetIt != m_testTargetSourceCoverage.end())
                {
                    coveringTestTargetIt->second.erase(source);
                }
            }

            // 2.
            // This step is prohibitively expensive as it requires iterating over all of the sources of the build targets covered by
            // the parent test targets of this source dependency to ensure that this is in fact the last source being cleared and thus
            // it can be determined that the parent test target is no longer covering the given build target
            //
            // The implications of this are that multiple calls to the test selector and prioritizor's SelectTestTargets method will end
            // up pulling in more test targets than needed for newly-created production sources until the next time the dynamic dependency
            // map reconstructed, however until this use case materializes the implications described will not be addressed

            // 3.
            sourceDependency.m_coveringTestTargets.clear();

            // Update the dependency with any new coverage data
            for (const auto& unresolvedTestTarget : sourceCoverage.GetCoveringTestTargets())
            {
                if (const TestTarget* testTarget =
                        m_buildTargetList->GetTestTargetList().GetTarget(unresolvedTestTarget);
                    testTarget)
                {
                    // Source to covering test target mapping
                    sourceDependency.m_coveringTestTargets.insert(testTarget);

                    // Add the source to the test target covering sources map
                    m_testTargetSourceCoverage[testTarget].insert(source);

                    // Build target to covering test target mapping
                    for (const auto& parentTarget : sourceDependency.m_parentTargets)
                    {
                        m_buildTargetCoverage[parentTarget].insert(testTarget);
                    }
                }
                else
                {
                    AZ_Warning(
                        "ReplaceSourceCoverage",
                        false,
                        "Test target %s exists in the coverage data but has since been removed from the build system",
                        unresolvedTestTarget.c_str());
                }
            }

            // If the new coverage data results in a parentless and coverageless entry, consider it a dead entry and remove accordingly
            if (sourceDependency.m_coveringTestTargets.empty() && sourceDependency.m_parentTargets.empty() && pruneIfNoParentsOrCoverage)
            {
                m_sourceDependencyMap.erase(sourceDependencyIt);
            }
        }
    }

    template<typename ProductionTarget, typename TestTarget>
    void DynamicDependencyMap<ProductionTarget, TestTarget>::ReplaceSourceCoverage(const SourceCoveringTestsList& sourceCoverageDelta)
    {
        ReplaceSourceCoverageInternal(sourceCoverageDelta, true);
    }

    template<typename ProductionTarget, typename TestTarget>
    void DynamicDependencyMap<ProductionTarget, TestTarget>::ClearSourceCoverage(const AZStd::vector<RepoPath>& paths)
    {
        for (const auto& path : paths)
        {
            if (const auto outputSources = m_autogenInputToOutputMap.find(path.String()); outputSources != m_autogenInputToOutputMap.end())
            {
                // Clearing the coverage data of an autogen input source instead clears the coverage data of its output sources
                for (const auto& outputSource : outputSources->second)
                {
                    ReplaceSourceCoverage(
                        SourceCoveringTestsList(AZStd::vector<SourceCoveringTests>{ SourceCoveringTests(RepoPath(outputSource)) }));
                }
            }
            else
            {
                ReplaceSourceCoverage(SourceCoveringTestsList(AZStd::vector<SourceCoveringTests>{ SourceCoveringTests(RepoPath(path)) }));
            }
        }
    }

    template<typename ProductionTarget, typename TestTarget>
    void DynamicDependencyMap<ProductionTarget, TestTarget>::ClearAllSourceCoverage()
    {
        for (auto it = m_sourceDependencyMap.begin(); it != m_sourceDependencyMap.end(); ++it)
        {
            const auto& [path, coverage] = *it;
            ReplaceSourceCoverageInternal(
                SourceCoveringTestsList(AZStd::vector<SourceCoveringTests>{ SourceCoveringTests(RepoPath(path)) }), false);
            if (coverage.m_coveringTestTargets.empty() && coverage.m_parentTargets.empty())
            {
                it = m_sourceDependencyMap.erase(it);
            }
        }
    }

    template<typename ProductionTarget, typename TestTarget>
    AZStd::vector<const TestTarget*> DynamicDependencyMap<ProductionTarget, TestTarget>::
        GetCoveringTestTargetsForProductionTarget(const ProductionTarget& productionTarget) const
    {
        AZStd::vector<const TestTarget*> coveringTestTargets;
        if (const auto coverage = m_buildTargetCoverage.find(&productionTarget); coverage != m_buildTargetCoverage.end())
        {
            coveringTestTargets.reserve(coverage->second.size());
            AZStd::copy(coverage->second.begin(), coverage->second.end(), AZStd::back_inserter(coveringTestTargets));
        }

        return coveringTestTargets;
    }

    template<typename ProductionTarget, typename TestTarget>
    AZStd::optional<SourceDependency<ProductionTarget, TestTarget>> DynamicDependencyMap<ProductionTarget, TestTarget>::GetSourceDependency(const RepoPath& path) const
    {
        AZStd::unordered_set<BuildTarget<ProductionTarget, TestTarget>> parentTargets;
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
            return SourceDependency<ProductionTarget, TestTarget>(path, DependencyData<ProductionTarget, TestTarget>{ AZStd::move(parentTargets), AZStd::move(coveringTestTargets) });
        }

        return AZStd::nullopt;
    }

    template<typename ProductionTarget, typename TestTarget>
    SourceDependency<ProductionTarget, TestTarget> DynamicDependencyMap<ProductionTarget, TestTarget>::GetSourceDependencyOrThrow(const RepoPath& path) const
    {
        auto sourceDependency = GetSourceDependency(path);
        AZ_TestImpact_Eval(
            sourceDependency.has_value(), DependencyException, AZStd::string::format("Couldn't find source %s", path.c_str()).c_str());
        return sourceDependency.value();
    }

    template<typename ProductionTarget, typename TestTarget>
    SourceCoveringTestsList DynamicDependencyMap<ProductionTarget, TestTarget>::ExportSourceCoverage() const
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

    template<typename ProductionTarget, typename TestTarget>
    AZStd::vector<AZStd::string> DynamicDependencyMap<ProductionTarget, TestTarget>::GetOrphanedSourceFiles() const
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

    template<typename ProductionTarget, typename TestTarget>
    ChangeDependencyList<ProductionTarget, TestTarget> DynamicDependencyMap<ProductionTarget, TestTarget>::ApplyAndResoveChangeList(
        const ChangeList& changeList, Policy::IntegrityFailure integrityFailurePolicy)
    {
        AZStd::vector<SourceDependency<ProductionTarget, TestTarget>> createDependencies;
        AZStd::vector<SourceDependency<ProductionTarget, TestTarget>> updateDependencies;
        AZStd::vector<SourceDependency<ProductionTarget, TestTarget>> deleteDependencies;

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
                    // Parent Targets: Yes or no
                    // Coverage Data : Yes
                    // Source Type   : Indeterminate
                    //
                    // Scenario
                    // 1. This file previously existed in one or more source to target mapping artifacts
                    // 2. The file has since been deleted yet no delete crud operation acted upon:
                    //    a) The coverage data for this file was not deleted from the Source Covering Test List
                    // 3. The file has since been recreated
                    // 4. This file does not exist in any source to target mapping artifacts
                    //
                    // Action (handled prior by DynamicDependencyMap::ApplyAndResoveChangeList)
                    // 1. Log Source Covering Test List compromised error
                    // 2. Throw exception
                    const AZStd::string msg = AZStd::string::format(
                        "The newly-created file '%s' belongs to a build target yet "
                        "still has coverage data in the source covering test list implying that a delete CRUD operation has been "
                        "missed, thus the integrity of the source covering test list has been compromised.",
                        createdFile.c_str());
                    AZ_Error("DynamicDependencyMap", false, msg.c_str());

                    if (integrityFailurePolicy == Policy::IntegrityFailure::Abort)
                    {
                        throw DependencyException(msg);
                    }

                    coverageToDelete.push_back(createdFile);
                }
                else if (sourceDependency->GetNumParentTargets())
                {
                    // Parent Targets: Yes
                    // Coverage Data : No
                    // Source Type   : Any
                    //
                    // Note: see TestSelectorAndPrioritizer::SelectTestTargets for scenarios and actions
                    createDependencies.emplace_back(AZStd::move(*sourceDependency));
                }
                else
                {
                    // Parent Targets: No
                    // Coverage Data : No
                    // Source Type   : Any
                    //
                    // Scenario
                    // 1. The existing file has been modified
                    // 2. This file does not exist in any source to target mapping artifacts
                    // 3. There exists no coverage data for this file in the Source Covering Test List
                    // 
                    // Action
                    // 1. Skip the file
                    continue;
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
                    // Parent Targets: Yes
                    // Coverage Data : Yes or no
                    // Source Type   : Any
                    //
                    // Note: see TestSelectorAndPrioritizer::SelectTestTargets for scenarios and actions
                    updateDependencies.emplace_back(AZStd::move(*sourceDependency));
                }
                else
                {
                    if (sourceDependency->GetNumCoveringTestTargets())
                    {
                        // Parent Targets: No
                        // Coverage Data : Yes
                        // Source Type   : Indeterminate
                        //
                        // Note: coverage will be deleted upon updating of coverage after test runs (if any)
                        //       see TestSelectorAndPrioritizer::SelectTestTargets for scenarios and actions
                        AZ_Warning(
                            "DynamicDependencyMap",
                            false,
                            "Source file '%s' is potentially an orphan (used by build targets "
                            "without explicitly being added to the build system, e.g. an include directive pulling in a header from "
                            "the "
                            "repository). Running the covering tests for this file with instrumentation will confirm whether or nor "
                            "this "
                            "is the case.\n",
                            updatedFile.c_str());

                        updateDependencies.emplace_back(AZStd::move(*sourceDependency));
                    }
                    else
                    {
                        // Parent Targets: No
                        // Coverage Data : No
                        // Source Type   : Indeterminate
                        //
                        // Scenario
                        // 1. The existing file has been modified
                        // 2. This file does not exist in any source to target mapping artifacts
                        // 3. There exists no coverage data for this file in the Source Covering Test List
                        //
                        // Action (handled prior by DynamicDependencyMap::ApplyAndResoveChangeList)
                        // 1. Skip the file
                        continue;
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
                // Parent Targets: Yes
                // Coverage Data : Yes or no
                // Source Type   : Irrelevant
                //
                // Scenario
                // 1. The existing file has been deleted.
                // 2. This file still exists in one or more source to target mapping artifacts
                // 3. There may or not exist coverage data for this file in the Source Covering Test List
                //
                // Action
                // 1. Log source to target mapping and Source Covering Test List integrity compromised error
                // 2. Throw exception

                const AZStd::string msg = AZStd::string::format(
                    "The deleted file '%s' still belongs to a build target implying "
                    "that the integrity of the source to target mappings has been compromised.",
                    deletedFile.c_str());
                AZ_Error("DynamicDependencyMap", false, msg.c_str());

                if (integrityFailurePolicy == Policy::IntegrityFailure::Abort)
                {
                    throw DependencyException(msg);
                }

                coverageToDelete.push_back(deletedFile);
            }
            else
            {
                if (sourceDependency->GetNumCoveringTestTargets())
                {
                    // Parent Targets: No
                    // Coverage Data : Yes
                    // Source Type   : Indeterminate
                    //
                    // Scenario
                    // 1. The file has been deleted.
                    // 2. This file previously existed in one or more source to target mapping artifacts
                    // 3. This file no longer exists in any source to target mapping artifacts
                    // 4. The coverage data for this file was has yet to be deleted from the Source Covering Test List
                    //
                    // Action
                    // 1. Select all test targets covering this file.
                    // 2. Delete the existing coverage data from the Source Covering Test List
                    //
                    // Note: coverage will be deleted upon updating of coverage after test runs (if any)
                    deleteDependencies.emplace_back(AZStd::move(*sourceDependency));
                }
                else
                {
                    // Parent Targets: No
                    // Coverage Data : No
                    // Source Type   : Indeterminate
                    //
                    // Scenario
                    // 1. The file has been deleted
                    // 2. This file does not exist in any source to target mapping artifacts
                    // 3. There exists no coverage data for this file in the Source Covering Test List
                    //
                    // Action
                    // 1. Skip the file
                    continue;
                }
            }
        }

        if (!coverageToDelete.empty())
        {
            ClearSourceCoverage(coverageToDelete);
        }

        return ChangeDependencyList<ProductionTarget, TestTarget>(AZStd::move(createDependencies), AZStd::move(updateDependencies), AZStd::move(deleteDependencies));
    }

    template<typename ProductionTarget, typename TestTarget>
    void DynamicDependencyMap<ProductionTarget, TestTarget>::RemoveTestTargetFromSourceCoverage(const TestTarget* testTarget)
    {
        if (const auto& it = m_testTargetSourceCoverage.find(testTarget); it != m_testTargetSourceCoverage.end())
        {
            for (const auto& source : it->second)
            {
                const auto sourceDependency = m_sourceDependencyMap.find(source);
                AZ_TestImpact_Eval(
                    sourceDependency != m_sourceDependencyMap.end(), DependencyException,
                    AZStd::string::format(
                        "Test target '%s' has covering source '%s' yet cannot be found in the dependency map",
                        testTarget->GetName().c_str(), source.c_str()));

                sourceDependency->second.m_coveringTestTargets.erase(testTarget);
            }

            m_testTargetSourceCoverage.erase(testTarget);
        }
    }

    template<typename ProductionTarget, typename TestTarget>
    AZStd::vector<const TestTarget*> DynamicDependencyMap<ProductionTarget, TestTarget>::GetCoveringTests() const
    {
        AZStd::vector<const TestTarget*> covering;
        for (const auto& [testTarget, coveringSources] : m_testTargetSourceCoverage)
        {
            if (!coveringSources.empty())
            {
                covering.push_back(testTarget);
            }
        }

        return covering;
    }

    template<typename ProductionTarget, typename TestTarget>
    AZStd::vector<const TestTarget*> DynamicDependencyMap<ProductionTarget, TestTarget>::GetNotCoveringTests() const
    {
        AZStd::vector<const TestTarget*> notCovering;
        for (const auto& [testTarget, coveringSources] : m_testTargetSourceCoverage)
        {
            if (coveringSources.empty())
            {
                notCovering.push_back(testTarget);
            }
        }

        return notCovering;
    }

    template<typename ProductionTarget, typename TestTarget>
    AZStd::unordered_set<const TestTarget*> DynamicDependencyMap<ProductionTarget, TestTarget>::GetCoveredProductionTargets() const
    {
        AZStd::unordered_set coveredProductionTargets;
        for (const auto& [source, dependencyData] : m_sourceDependencyMap)
        {
            for(const auto* parentTarget : dependencyData.GetParentTargets())
            {
                coveredProductionTargets.insert(parentTarget);
            }
        }

        return coveredProductionTargets;
    }
} // namespace TestImpact
