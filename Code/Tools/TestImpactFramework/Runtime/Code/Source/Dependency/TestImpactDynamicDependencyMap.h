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

#include <Artifact/Static/TestImpactProductionTargetDescriptor.h>
#include <Artifact/Static/TestImpactTestTargetDescriptor.h>
#include <Dependency/TestImpactSourceCoveringTestsList.h>
#include <Dependency/TestImpactSourceDependency.h>
#include <Dependency/TestImpactChangeDependencyList.h>
#include <Target/TestImpactProductionTargetList.h>
#include <Target/TestImpactTestTargetList.h>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>

namespace TestImpact
{
    //! Representation of the repository source tree and its relation to the build targets and coverage data.
    class DynamicDependencyMap
    {
    public:
        //! Constructs the dependency map with entries for each build target's source files with empty test coverage data.
        DynamicDependencyMap(
            AZStd::vector<ProductionTargetDescriptor>&& productionTargetDescriptors,
            AZStd::vector<TestTargetDescriptor>&& testTargetDescriptors);

        //! Gets the total number of production and test targets in the repository.
        size_t GetNumTargets() const;

        //! Gets the total number of unique source files in the repository.
        //! @note This includes autogen output sources.
        size_t GetNumSources() const;

        //! Attempts to get the specified build target.
        //! @param name The name of the build target to get.
        //! @returns If found, the pointer to the specified build target, otherwise nullptr.
        const BuildTarget* GetBuildTarget(const AZStd::string& name) const;

        //! Attempts to get the specified build target or throw TargetException.
        //! @param name The name of the build target to get.
        const BuildTarget* GetBuildTargetOrThrow(const AZStd::string& name) const;

        //! Attempts to get the specified target's specialized type.
        //! @param name The name of the target to get.
        //! @returns If found, the pointer to the specialized target, otherwise AZStd::monostate.
        OptionalTarget GetTarget(const AZStd::string& name) const;

        //! Attempts to get the specified target's specialized type or throw TargetException.
        //! @param name The name of the target to get.
        Target GetTargetOrThrow(const AZStd::string& name) const;

        //! Get the list of production targets in the repository.
        const ProductionTargetList& GetProductionTargetList() const;

        //! Get the list of test targets in the repository.
        const TestTargetList& GetTestTargetList() const;

        //! Gets the test targets covering the specified production target.
        //! @param productionTarget The production target to retrieve the covering tests for.
        AZStd::vector<const TestTarget*> GetCoveringTestTargetsForProductionTarget(const ProductionTarget& productionTarget) const;

        //! Gets the source dependency for the specified source file.
        //! @note Autogen input source dependencies are the consolidated source dependencies of all of their generated output sources.
        //! @returns If found, the source dependency information for the specified source file, otherwise empty.
        AZStd::optional<SourceDependency> GetSourceDependency(const RepoPath& path) const;

        //! Gets the source dependency for the specified source file or throw DependencyException.
        SourceDependency GetSourceDependencyOrThrow(const RepoPath& path) const;

        //! Replaces the source coverage of the specified sources with the specified source coverage.
        //! @param sourceCoverageDelta The source coverage delta to replace in the dependency map.
        void ReplaceSourceCoverage(const SourceCoveringTestsList& sourceCoverageDelta);

        //! Clears all of the existing source coverage in the dependency map.
        void ClearAllSourceCoverage();

        //! Exports the coverage of all sources in the dependency map.
        SourceCoveringTestsList ExportSourceCoverage() const;

        //! Gets the list of orphaned source files in the dependency map that have coverage data but belong to no parent build targets.
        AZStd::vector<AZStd::string> GetOrphanSourceFiles() const;

        //! Applies the specified change list to the dependency map and resolves the change list to a change dependency list
        //! containing the updated source dependencies for each source file in the change list.
        //! @param changeList The change list to apply and resolve.
        //! @param integrityFailurePolicy The policy to use for handling integrity errors in the source dependency map.
        //! @returns The change list as resolved to the appropriate source dependencies.
        [[nodiscard]] ChangeDependencyList ApplyAndResoveChangeList(
            const ChangeList& changeList, Policy::IntegrityFailure integrityFailurePolicy);

        //! Removes the specified test target from all source coverage.
        void RemoveTestTargetFromSourceCoverage(const TestTarget* testTarget);

        //! Returns the test targets that cover one or more sources in the repository.
        AZStd::vector<const TestTarget*> GetCoveringTests() const;

        //! Returns the test targets that do not cover any sources in the repository.
        AZStd::vector<const TestTarget*> GetNotCoveringTests() const;

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

        //! The sorted list of unique production targets in the repository.
        ProductionTargetList m_productionTargets;

        //! The sorted list of unique test targets in the repository.
        TestTargetList m_testTargets;

        //! The dependency map of sources to their parent build targets and covering test targets.
        AZStd::unordered_map<AZStd::string, DependencyData> m_sourceDependencyMap;

        //! Map of all test targets and the sources they cover.
        AZStd::unordered_map<const TestTarget*, AZStd::unordered_set<AZStd::string>> m_testTargetSourceCoverage;

        //! The map of build targets and their covering test targets.
        //! @note As per the note for ReplaceSourceCoverageInternal, this map is currently not pruned when source coverage is replaced.
        AZStd::unordered_map<const BuildTarget*, AZStd::unordered_set<const TestTarget*>> m_buildTargetCoverage;

        //! Mapping of autogen input sources to their generated output sources.
        AZStd::unordered_map<AZStd::string, AZStd::vector<AZStd::string>> m_autogenInputToOutputMap;
    };
} // namespace TestImpact
