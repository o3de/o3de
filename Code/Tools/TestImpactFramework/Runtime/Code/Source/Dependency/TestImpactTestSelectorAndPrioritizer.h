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
    template<typename BuildTargetTraits>
    class DynamicDependencyMap;

    class Target;

    //! Map of build targets and their dependency graph data.
    //! For test targets, the dependency graph data is that of the build targets which the test target depends on.
    //! For production targets, the dependency graph is that of the build targets that depend on it (dependers).
    //! @note No dependency graph data is not an error, it simple means that the target cannot be prioritized.
    using DependencyGraphDataMap = AZStd::unordered_map<const AZStd::string, DependencyGraphData>;

    //! Selects the test targets that cover a given set of changes based on the CRUD rules and optionally prioritizes the test
    //! selection according to their locality of their covering production targets in the their dependency graphs.
    //! @note the CRUD rules for how tests are selected can be found in the MicroRepo header file.
    template<typename BuildTargetTraits>
    class TestSelectorAndPrioritizer
    {
    public:
        //! Constructs the test selector and prioritizer for the given dynamic dependency map.
        //! @param dynamicDependencyMap The dynamic dependency map representing the repository source tree.
        //! @param dependencyGraphDataMap The map of build targets and their dependency graph data for use in test prioritization.
        TestSelectorAndPrioritizer(
            const DynamicDependencyMap<BuildTargetTraits>* dynamicDependencyMap,
            DependencyGraphDataMap&& dependencyGraphDataMap);

        virtual ~TestSelectorAndPrioritizer() = default;

        //! Select the covering test targets for the given set of source changes and optionally prioritizes said test selection.
        //! @param changeDependencyList The resolved list of source dependencies for the CRUD source changes.
        //! @param testSelectionStrategy The test selection and prioritization strategy to apply to the given CRUD source changes.
        AZStd::vector<const typename BuildTargetTraits::TestTarget*> SelectTestTargets(
            const ChangeDependencyList<BuildTargetTraits>& changeDependencyList, Policy::TestPrioritization testSelectionStrategy);

    private:
        //! Map of selected test targets and the production targets they cover for the given set of source changes.
        using SelectedTestTargetAndDependerMap = AZStd::unordered_map<const typename BuildTargetTraits::TestTarget*, AZStd::unordered_set<const typename BuildTargetTraits::ProductionTarget*>>;

        //! Selects the test targets covering the set of source changes in the change dependency list.
        //! @param changeDependencyList The change dependency list containing the CRUD source changes to select tests for.
        //! @returns The selected tests and their covering production targets for the given set of source changes.
        SelectedTestTargetAndDependerMap SelectTestTargets(const ChangeDependencyList<BuildTargetTraits>& changeDependencyList);

        //! Prioritizes the selected tests according to the specified test selection strategy,
        //! @note If no dependency graph data exists for a given test target then that test target still be selected albeit not prioritized.
        //! @param selectedTestTargetAndDependerMap The selected tests to prioritize.
        //! @param testSelectionStrategy The test selection strategy to prioritize the selected tests.
        //! @returns The selected tests either in either arbitrary order or in prioritized with highest priority first.
        AZStd::vector<const typename BuildTargetTraits::TestTarget*> PrioritizeSelectedTestTargets(
            const SelectedTestTargetAndDependerMap& selectedTestTargetAndDependerMap, Policy::TestPrioritization testSelectionStrategy);

        const DynamicDependencyMap<BuildTargetTraits>* m_dynamicDependencyMap;
        DependencyGraphDataMap m_dependencyGraphDataMap;

    protected:
        //!
        virtual void CreateProductionSourceAction(const typename BuildTargetTraits::ProductionTarget* target, SelectedTestTargetAndDependerMap& selectedTestTargetMap);

        //!
        virtual void CreateTestSourceAction(const typename BuildTargetTraits::TestTarget* target, SelectedTestTargetAndDependerMap& selectedTestTargetMap);

        //!
        virtual void UpdateProductionSourceWithCoverageAction(
            const typename BuildTargetTraits::ProductionTarget* target,
            SelectedTestTargetAndDependerMap& selectedTestTargetMap,
            const SourceDependency<BuildTargetTraits>& sourceDependency);

        //!
        virtual void UpdateTestSourceWithCoverageAction(
            const typename BuildTargetTraits::TestTarget* target,
            SelectedTestTargetAndDependerMap& selectedTestTargetMap);

        //!
        virtual void UpdateProductionSourceWithoutCoverageAction(
            const typename BuildTargetTraits::ProductionTarget* target,
            SelectedTestTargetAndDependerMap& selectedTestTargetMap);

        //!
        virtual void UpdateTestSourceWithoutCoverageAction(
            const typename BuildTargetTraits::TestTarget* target,
            SelectedTestTargetAndDependerMap& selectedTestTargetMap);

        //!
        virtual void UpdateIndeterminateSourceWithoutCoverageAction(
            SelectedTestTargetAndDependerMap& selectedTestTargetMap,
            const SourceDependency<BuildTargetTraits>& sourceDependency);

        //!
        virtual void DeleteIndeterminateSourceWithoutCoverageAction(
            SelectedTestTargetAndDependerMap& selectedTestTargetMap, const SourceDependency<BuildTargetTraits>& sourceDependency);
    };

    template<typename BuildTargetTraits>
    TestSelectorAndPrioritizer<BuildTargetTraits>::TestSelectorAndPrioritizer(
        const DynamicDependencyMap<BuildTargetTraits>* dynamicDependencyMap, DependencyGraphDataMap&& dependencyGraphDataMap)
        : m_dynamicDependencyMap(dynamicDependencyMap)
        , m_dependencyGraphDataMap(AZStd::move(dependencyGraphDataMap))
    {
    }

    template<typename BuildTargetTraits>
    AZStd::vector<const typename BuildTargetTraits::TestTarget*> TestSelectorAndPrioritizer<BuildTargetTraits>::SelectTestTargets(
        const ChangeDependencyList<BuildTargetTraits>& changeDependencyList, Policy::TestPrioritization testSelectionStrategy)
    {
        const auto selectedTestTargetAndDependerMap = SelectTestTargets(changeDependencyList);
        const auto prioritizedSelectedTests = PrioritizeSelectedTestTargets(selectedTestTargetAndDependerMap, testSelectionStrategy);
        return prioritizedSelectedTests;
    }

    template<typename BuildTargetTraits>
    void TestSelectorAndPrioritizer<BuildTargetTraits>::CreateProductionSourceAction(
        const typename BuildTargetTraits::ProductionTarget* target, SelectedTestTargetAndDependerMap& selectedTestTargetMap)
    {
        // Action
        // 1. Select all test targets covering the parent production targets
        const auto coverage = m_dynamicDependencyMap->GetCoveringTestTargetsForProductionTarget(*target);
        for (const auto* testTarget : coverage)
        {
            selectedTestTargetMap[testTarget].insert(target);
        }
    }

    template<typename BuildTargetTraits>
    void TestSelectorAndPrioritizer<BuildTargetTraits>::CreateTestSourceAction(
        const typename BuildTargetTraits::TestTarget* target, SelectedTestTargetAndDependerMap& selectedTestTargetMap)
    {
        // Action
        // 1. Select all parent test targets
        selectedTestTargetMap.insert(target);
    }

    template<typename BuildTargetTraits>
    void TestSelectorAndPrioritizer<BuildTargetTraits>::UpdateProductionSourceWithCoverageAction(
        const typename BuildTargetTraits::ProductionTarget* target,
        SelectedTestTargetAndDependerMap& selectedTestTargetMap,
        const SourceDependency<BuildTargetTraits>& sourceDependency)
    {
        // Action
        // 1. Select all test targets covering this file
        for (const auto* testTarget : sourceDependency.GetCoveringTestTargets())
        {
            selectedTestTargetMap[testTarget].insert(target);
        }
    }

    template<typename BuildTargetTraits>
    void TestSelectorAndPrioritizer<BuildTargetTraits>::UpdateTestSourceWithCoverageAction(
        const typename BuildTargetTraits::TestTarget* target, SelectedTestTargetAndDependerMap& selectedTestTargetMap)
    {
        // Action
        // 1. Select the parent test targets for this file
        selectedTestTargetMap.insert(target);
    }

    template<typename BuildTargetTraits>
    void TestSelectorAndPrioritizer<BuildTargetTraits>::UpdateProductionSourceWithoutCoverageAction(
        [[maybe_unused]] const typename BuildTargetTraits::ProductionTarget* target,
        [[maybe_unused]] SelectedTestTargetAndDependerMap& selectedTestTargetMap)
    {
        // Action
        // 1. Do nothing
    }

    template<typename BuildTargetTraits>
    void TestSelectorAndPrioritizer<BuildTargetTraits>::UpdateTestSourceWithoutCoverageAction(
        const typename BuildTargetTraits::TestTarget* target, SelectedTestTargetAndDependerMap& selectedTestTargetMap)
    {
        // Action
        // 1. Select the parent test targets for this file
        selectedTestTargetMap.insert(target);
    }

    template<typename BuildTargetTraits>
    void TestSelectorAndPrioritizer<BuildTargetTraits>::UpdateIndeterminateSourceWithoutCoverageAction(
        SelectedTestTargetAndDependerMap& selectedTestTargetMap, const SourceDependency<BuildTargetTraits>& sourceDependency)
    {
        // Action
        // 1. Log potential orphaned source file warning (handled prior by DynamicDependencyMap)
        // 2. Select all test targets covering this file
        // 3. Delete the existing coverage data from the source covering test list (handled prior by DynamicDependencyMap)

        for (const auto* testTarget : sourceDependency.GetCoveringTestTargets())
        {
            selectedTestTargetMap.insert(testTarget);
        }
    }

    template<typename BuildTargetTraits>
    void TestSelectorAndPrioritizer<BuildTargetTraits>::DeleteIndeterminateSourceWithoutCoverageAction(
        SelectedTestTargetAndDependerMap& selectedTestTargetMap, const SourceDependency<BuildTargetTraits>& sourceDependency)
    {
        // Action
        // 1. Select all test targets covering this file
        // 2. Delete the existing coverage data from the source covering test list (handled prior by DynamicDependencyMap)
        for (const auto* testTarget : sourceDependency.GetCoveringTestTargets())
        {
            selectedTestTargetMap.insert(testTarget);
        }
    }

    template<typename BuildTargetTraits>
    typename TestSelectorAndPrioritizer<BuildTargetTraits>::SelectedTestTargetAndDependerMap TestSelectorAndPrioritizer<BuildTargetTraits>::SelectTestTargets(
        const ChangeDependencyList<BuildTargetTraits>& changeDependencyList)
    {
        SelectedTestTargetAndDependerMap selectedTestTargetMap;

        // Create operations
        for (const auto& sourceDependency : changeDependencyList.GetCreateSourceDependencies())
        {
            for (const auto& parentTarget : sourceDependency.GetParentTargets())
            {

                if (parentTarget.GetTargetType() == BuildTargetType::ProductionTarget)
                {
                    // Parent Targets: Yes
                    // Coverage Data : No
                    // Source Type   : Production
                    //
                    // Scenario
                    // 1. The file has been newly created
                    // 2. This file exists in one or more source to production target mapping artifacts
                    // 3. There exists no coverage data for this file in the source covering test list
                    CreateProductionSourceAction(parentTarget.GetProductionTarget().value(), selectedTestTargetMap);
                }
                else
                {
                    // Parent Targets: Yes
                    // Coverage Data : No
                    // Source Type   : Test
                    //
                    // Scenario
                    // 1. The file has been newly created
                    // 2. This file exists in one or more source to test target mapping artifacts
                    // 3. There exists no coverage data for this file in the source covering test list
                    CreateTestSourceAction(parentTarget.GetTestTarget().value(), selectedTestTargetMap);
                }
            }
        }

        // Update operations
        for (const auto& sourceDependency : changeDependencyList.GetUpdateSourceDependencies())
        {
            if (sourceDependency.GetNumParentTargets())
            {
                if (sourceDependency.GetNumCoveringTestTargets())
                {
                    for (const auto& parentTarget : sourceDependency.GetParentTargets())
                    {
                        if (parentTarget.GetTargetType() == BuildTargetType::ProductionTarget)
                        {
                            // Parent Targets: Yes
                            // Coverage Data : Yes
                            // Source Type   : Production
                            //
                            // Scenario
                            // 1. The existing file has been modified
                            // 2. This file exists in one or more source to production target mapping artifacts
                            // 3. There exists coverage data for this file in the source covering test list
                            UpdateProductionSourceWithCoverageAction(parentTarget.GetProductionTarget().value(), selectedTestTargetMap, sourceDependency);
                        }
                        else
                        {
                            // Parent Targets: Yes
                            // Coverage Data : Yes
                            // Source Type   : Test
                            //
                            // Scenario
                            // 1. The existing file has been modified
                            // 2. This file exists in one or more source to test target mapping artifacts
                            // 3. There exists coverage data for this file in the source covering test list
                            UpdateTestSourceWithCoverageAction(parentTarget.GetTestTarget().value(), selectedTestTargetMap);
                        }
                    }
                }
                else
                {
                    for (const auto& parentTarget : sourceDependency.GetParentTargets())
                    {
                        if (parentTarget.GetTargetType() == BuildTargetType::ProductionTarget)
                        {
                            // Parent Targets: Yes
                            // Coverage Data : No
                            // Source Type   : Production
                            //
                            // Scenario
                            // 1. The existing file has been modified
                            // 2. This file exists in one or more source to test target mapping artifacts
                            // 3. There exists no coverage data for this file in the source covering test list
                            UpdateProductionSourceWithoutCoverageAction(parentTarget.GetProductionTarget().value(), selectedTestTargetMap);
                        }
                        else
                        {
                            // Parent Targets: Yes
                            // Coverage Data : No
                            // Source Type   : Test
                            //
                            // Scenario
                            // 1. The existing file has been modified
                            // 2. This file exists in one or more source to test target mapping artifacts
                            // 3. There exists no coverage data for this file in the source covering test list
                            UpdateTestSourceWithoutCoverageAction(parentTarget.GetTestTarget().value(), selectedTestTargetMap);
                        }
                    }
                }
            }
            else
            {
                // Parent Targets: No
                // Coverage Data : Yes
                // Source Type   : Indeterminate
                //
                // Scenario
                // 1. The existing file has been modified
                // 2. Either:
                //  a) This file previously existed in one or more source to target mapping artifacts
                //  b) This file no longer exists in any source to target mapping artifacts
                //  c) The coverage data for this file was has yet to be deleted from the source covering test list
                // 3. Or:
                //  a) The file is being used by build targets but has erroneously not been explicitly added to the build
                //     system (e.g. include directive pulling in a header from the repository that has not been added to
                //     any build targets due to an oversight)
                UpdateIndeterminateSourceWithoutCoverageAction(selectedTestTargetMap, sourceDependency);
            }
        }

        // Delete operations
        for (const auto& sourceDependency : changeDependencyList.GetDeleteSourceDependencies())
        {
            // Parent Targets: No
            // Coverage Data : Yes
            // Source Type   : Indeterminate
            //
            // Scenario
            // 1. The existing file has been deleted
            // 2. This file previously existed in one or more source to target mapping artifacts
            // 2. This file does not exist in any source to target mapping artifacts
            // 4. The coverage data for this file was has yet to be deleted from the source covering test list
            DeleteIndeterminateSourceWithoutCoverageAction(selectedTestTargetMap, sourceDependency);
        }

        return selectedTestTargetMap;
    }

    template<typename BuildTargetTraits>
    AZStd::vector<const typename BuildTargetTraits::TestTarget*> TestSelectorAndPrioritizer<BuildTargetTraits>::PrioritizeSelectedTestTargets(
        const SelectedTestTargetAndDependerMap& selectedTestTargetAndDependerMap,
        [[maybe_unused]] Policy::TestPrioritization testSelectionStrategy)
    {
        AZStd::vector<const typename BuildTargetTraits::TestTarget*> selectedTestTargets;

        // Prioritization disabled for now
        // SPEC-6563
        for (const auto& [testTarget, dependerTargets] : selectedTestTargetAndDependerMap)
        {
            selectedTestTargets.push_back(testTarget);
        }

        return selectedTestTargets;
    }
} // namespace TestImpact
