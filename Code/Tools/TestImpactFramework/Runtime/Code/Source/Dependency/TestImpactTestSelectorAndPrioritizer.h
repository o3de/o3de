/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactPolicy.h>

#include <Artifact/Static/TestImpactDependencyGraphData.h>
#include <Dependency/TestImpactChangeDependencyList.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>

namespace TestImpact
{
    class DynamicDependencyMap;
    class BuildTarget;
    class TestTarget;

    //! Map of build targets and their dependency graph data.
    //! For test targets, the dependency graph data is that of the build targets which the test target depends on.
    //! For production targets, the dependency graph is that of the build targets that depend on it (dependers).
    //! @note No dependency graph data is not an error, it simple means that the target cannot be prioritized.
    using DependencyGraphDataMap = AZStd::unordered_map<const BuildTarget*, DependencyGraphData>;

    //! Selects the test targets that cover a given set of changes based on the CRUD rules and optionally prioritizes the test
    //! selection according to their locality of their covering production targets in the their dependency graphs.
    //! @note the CRUD rules for how tests are selected can be found in the MicroRepo header file.
    class TestSelectorAndPrioritizer
    {
    public:
        //! Constructs the test selector and prioritizer for the given dynamic dependency map.
        //! @param dynamicDependencyMap The dynamic dependency map representing the repository source tree.
        //! @param dependencyGraphDataMap The map of build targets and their dependency graph data for use in test prioritization.
        TestSelectorAndPrioritizer(const DynamicDependencyMap* dynamicDependencyMap, DependencyGraphDataMap&& dependencyGraphDataMap);

        //! Select the covering test targets for the given set of source changes and optionally prioritizes said test selection.
        //! @param changeDependencyList The resolved list of source dependencies for the CRUD source changes.
        //! @param testSelectionStrategy The test selection and prioritization strategy to apply to the given CRUD source changes.
        AZStd::vector<const TestTarget*> SelectTestTargets(const ChangeDependencyList& changeDependencyList, Policy::TestPrioritization testSelectionStrategy);

    private:
        //! Map of selected test targets and the production targets they cover for the given set of source changes.
        using SelectedTestTargetAndDependerMap = AZStd::unordered_map<const TestTarget*, AZStd::unordered_set<const ProductionTarget*>>;

        //! Selects the test targets covering the set of source changes in the change dependency list.
        //! @param changeDependencyList The change dependency list containing the CRUD source changes to select tests for.
        //! @returns The selected tests and their covering production targets for the given set of source changes.
        SelectedTestTargetAndDependerMap SelectTestTargets(const ChangeDependencyList& changeDependencyList);

        //! Prioritizes the selected tests according to the specified test selection strategy,
        //! @note If no dependency graph data exists for a given test target then that test target still be selected albeit not prioritized.
        //! @param selectedTestTargetAndDependerMap The selected tests to prioritize.
        //! @param testSelectionStrategy The test selection strategy to prioritize the selected tests.
        //! @returns The selected tests either in either arbitrary order or in prioritized with highest priority first.
        AZStd::vector<const TestTarget*> PrioritizeSelectedTestTargets(
            const SelectedTestTargetAndDependerMap& selectedTestTargetAndDependerMap, Policy::TestPrioritization testSelectionStrategy);

        const DynamicDependencyMap* m_dynamicDependencyMap;
        DependencyGraphDataMap m_dependencyGraphDataMap;
    };
} // namespace TestImpact
