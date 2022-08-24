/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactPolicy.h>

#include <Dependency/TestImpactChangeDependencyList.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>

namespace TestImpact
{
    template<typename ProductionTarget, typename TestTarget>
    class DynamicDependencyMap;

    class Target;

    //! Selects the test targets that cover a given set of changes based on the CRUD rules and optionally prioritizes the test
    //! selection according to their locality of their covering production targets in the their dependency graphs.
    //! @note the CRUD rules for how tests are selected can be found in the MicroRepo header file.
    template<typename ProductionTarget, typename TestTarget>
    class TestSelectorAndPrioritizer
    {
    public:
        //! Constructs the test selector and prioritizer for the given dynamic dependency map.
        //! @param dynamicDependencyMap The dynamic dependency map representing the repository source tree.
        TestSelectorAndPrioritizer(
            const DynamicDependencyMap<ProductionTarget, TestTarget>& dynamicDependencyMap);

        virtual ~TestSelectorAndPrioritizer() = default;

        //! Select the covering test targets for the given set of source changes and optionally prioritizes said test selection.
        //! @param changeDependencyList The resolved list of source dependencies for the CRUD source changes.
        //! @param testSelectionStrategy The test selection and prioritization strategy to apply to the given CRUD source changes.
        AZStd::vector<const TestTarget*> SelectTestTargets(
            const ChangeDependencyList<ProductionTarget, TestTarget>& changeDependencyList, Policy::TestPrioritization testSelectionStrategy);

    private:
        //! Map of selected test targets and the production targets they cover for the given set of source changes.
        using SelectedTestTargetAndDependerMap = AZStd::unordered_map<const TestTarget*, AZStd::unordered_set<const ProductionTarget*>>;

        //! Selects the test targets covering the set of source changes in the change dependency list.
        //! @param changeDependencyList The change dependency list containing the CRUD source changes to select tests for.
        //! @returns The selected tests and their covering production targets for the given set of source changes.
        SelectedTestTargetAndDependerMap SelectTestTargets(const ChangeDependencyList<ProductionTarget, TestTarget>& changeDependencyList);

        //! Prioritizes the selected tests according to the specified test selection strategy,
        //! @param selectedTestTargetAndDependerMap The selected tests to prioritize.
        //! @param testSelectionStrategy The test selection strategy to prioritize the selected tests.
        //! @returns The selected tests either in either arbitrary order or in prioritized with highest priority first.
        AZStd::vector<const TestTarget*> PrioritizeSelectedTestTargets(
            const SelectedTestTargetAndDependerMap& selectedTestTargetAndDependerMap, Policy::TestPrioritization testSelectionStrategy);

        const DynamicDependencyMap<ProductionTarget, TestTarget>* m_dynamicDependencyMap;

    protected:
        //! Action to perform when production sources are created.
        virtual void CreateProductionSourceAction(const ProductionTarget* target, SelectedTestTargetAndDependerMap& selectedTestTargetMap);

        //! Action to perform when test sources are created.
        virtual void CreateTestSourceAction(const TestTarget* target, SelectedTestTargetAndDependerMap& selectedTestTargetMap);

        //! Action to perform when production sources with coverage are updated.
        virtual void UpdateProductionSourceWithCoverageAction(
            const ProductionTarget* target,
            SelectedTestTargetAndDependerMap& selectedTestTargetMap,
            const SourceDependency<ProductionTarget, TestTarget>& sourceDependency);

        //! Action to perform when test sources with coverage are updated.
        virtual void UpdateTestSourceWithCoverageAction(
            const TestTarget* target,
            SelectedTestTargetAndDependerMap& selectedTestTargetMap);

        //! Action to perform when production sources without coverage are updated.
        virtual void UpdateProductionSourceWithoutCoverageAction(
            const ProductionTarget* target,
            SelectedTestTargetAndDependerMap& selectedTestTargetMap);

        //! Action to perform when test sources without coverage are updated.
        virtual void UpdateTestSourceWithoutCoverageAction(
            const TestTarget* target,
            SelectedTestTargetAndDependerMap& selectedTestTargetMap);

        //! Action to perform when sources that cannot be determined to be production or test sources without coverage are updated.
        virtual void UpdateIndeterminateSourceWithoutCoverageAction(
            SelectedTestTargetAndDependerMap& selectedTestTargetMap,
            const SourceDependency<ProductionTarget, TestTarget>& sourceDependency);

        //! Action to perform when sources that cannot be determined to be production or test sources without coverage are deleted.
        virtual void DeleteIndeterminateSourceWithoutCoverageAction(
            SelectedTestTargetAndDependerMap& selectedTestTargetMap, const SourceDependency<ProductionTarget, TestTarget>& sourceDependency);
    };

    template<typename ProductionTarget, typename TestTarget>
    TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::TestSelectorAndPrioritizer(
        const DynamicDependencyMap<ProductionTarget, TestTarget>& dynamicDependencyMap)
        : m_dynamicDependencyMap(&dynamicDependencyMap)
    {
    }

    template<typename ProductionTarget, typename TestTarget>
    AZStd::vector<const TestTarget*> TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::SelectTestTargets(
        const ChangeDependencyList<ProductionTarget, TestTarget>& changeDependencyList, Policy::TestPrioritization testSelectionStrategy)
    {
        const auto selectedTestTargetAndDependerMap = SelectTestTargets(changeDependencyList);
        const auto prioritizedSelectedTests = PrioritizeSelectedTestTargets(selectedTestTargetAndDependerMap, testSelectionStrategy);
        return prioritizedSelectedTests;
    }

    template<typename ProductionTarget, typename TestTarget>
    void TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::CreateProductionSourceAction(
        const ProductionTarget* target, SelectedTestTargetAndDependerMap& selectedTestTargetMap)
    {
        // Action
        // 1. Select all test targets covering the parent production targets
        const auto coverage = m_dynamicDependencyMap->GetCoveringTestTargetsForProductionTarget(*target);
        for (const auto* testTarget : coverage)
        {
            selectedTestTargetMap[testTarget].insert(target);
        }
    }

    template<typename ProductionTarget, typename TestTarget>
    void TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::CreateTestSourceAction(
        const TestTarget* target, SelectedTestTargetAndDependerMap& selectedTestTargetMap)
    {
        // Action
        // 1. Select all parent test targets
        selectedTestTargetMap.insert(target);
    }

    template<typename ProductionTarget, typename TestTarget>
    void TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::UpdateProductionSourceWithCoverageAction(
        const ProductionTarget* target,
        SelectedTestTargetAndDependerMap& selectedTestTargetMap,
        const SourceDependency<ProductionTarget, TestTarget>& sourceDependency)
    {
        // Action
        // 1. Select all test targets covering this file
        for (const auto* testTarget : sourceDependency.GetCoveringTestTargets())
        {
            selectedTestTargetMap[testTarget].insert(target);
        }
    }

    template<typename ProductionTarget, typename TestTarget>
    void TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::UpdateTestSourceWithCoverageAction(
        const TestTarget* target, SelectedTestTargetAndDependerMap& selectedTestTargetMap)
    {
        // Action
        // 1. Select the parent test targets for this file
        selectedTestTargetMap.insert(target);
    }

    template<typename ProductionTarget, typename TestTarget>
    void TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::UpdateProductionSourceWithoutCoverageAction(
        [[maybe_unused]] const ProductionTarget* target,
        [[maybe_unused]] SelectedTestTargetAndDependerMap& selectedTestTargetMap)
    {
        // Action
        // 1. Do nothing
    }

    template<typename ProductionTarget, typename TestTarget>
    void TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::UpdateTestSourceWithoutCoverageAction(
        const TestTarget* target, SelectedTestTargetAndDependerMap& selectedTestTargetMap)
    {
        // Action
        // 1. Select the parent test targets for this file
        selectedTestTargetMap.insert(target);
    }

    template<typename ProductionTarget, typename TestTarget>
    void TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::UpdateIndeterminateSourceWithoutCoverageAction(
        SelectedTestTargetAndDependerMap& selectedTestTargetMap, const SourceDependency<ProductionTarget, TestTarget>& sourceDependency)
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

    template<typename ProductionTarget, typename TestTarget>
    void TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::DeleteIndeterminateSourceWithoutCoverageAction(
        SelectedTestTargetAndDependerMap& selectedTestTargetMap, const SourceDependency<ProductionTarget, TestTarget>& sourceDependency)
    {
        // Action
        // 1. Select all test targets covering this file
        // 2. Delete the existing coverage data from the source covering test list (handled prior by DynamicDependencyMap)
        for (const auto* testTarget : sourceDependency.GetCoveringTestTargets())
        {
            selectedTestTargetMap.insert(testTarget);
        }
    }

    template<typename ProductionTarget, typename TestTarget>
    typename TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::SelectedTestTargetAndDependerMap TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::SelectTestTargets(
        const ChangeDependencyList<ProductionTarget, TestTarget>& changeDependencyList)
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
                    CreateProductionSourceAction(parentTarget.GetProductionTarget(), selectedTestTargetMap);
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
                    CreateTestSourceAction(parentTarget.GetTestTarget(), selectedTestTargetMap);
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
                            UpdateProductionSourceWithCoverageAction(parentTarget.GetProductionTarget(), selectedTestTargetMap, sourceDependency);
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
                            UpdateTestSourceWithCoverageAction(parentTarget.GetTestTarget(), selectedTestTargetMap);
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
                            UpdateProductionSourceWithoutCoverageAction(parentTarget.GetProductionTarget(), selectedTestTargetMap);
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
                            UpdateTestSourceWithoutCoverageAction(parentTarget.GetTestTarget(), selectedTestTargetMap);
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

    template<typename ProductionTarget, typename TestTarget>
    AZStd::vector<const TestTarget*> TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::PrioritizeSelectedTestTargets(
        const SelectedTestTargetAndDependerMap& selectedTestTargetAndDependerMap,
        [[maybe_unused]] Policy::TestPrioritization testSelectionStrategy)
    {
        AZStd::vector<const TestTarget*> selectedTestTargets;

        // Prioritization disabled for now
        // SPEC-6563
        for (const auto& [testTarget, dependerTargets] : selectedTestTargetAndDependerMap)
        {
            selectedTestTargets.push_back(testTarget);
        }

        return selectedTestTargets;
    }
} // namespace TestImpact
