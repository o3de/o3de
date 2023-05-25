/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <BuildTarget/Common/TestImpactBuildGraph.h>
#include <TestImpactFramework/TestImpactPolicy.h>

#include <Dependency/TestImpactChangeDependencyList.h>

#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>

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

    protected:
        //! Map of selected test targets and the production targets they cover for the given set of source changes.
        using SelectedTestTargetAndDependerMap = AZStd::unordered_map<const TestTarget*, AZStd::unordered_set<const ProductionTarget*>>;
    private:
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

    protected:
        //! Result of source dependency action.
        enum class SourceOperationActionResult : AZ::u8
        {
            Continue, //!< Continue with actions for this CRUD operation.
            ConcludeSelection, //!< Conclude selection (no further actions).
        };

        //! Iterate over the production and test targets for newly created sources with no coverage and act on them.
        [[nodiscard]] virtual SourceOperationActionResult IterateCreateParentedSourcesWithNoCoverage(
            const SourceDependency<ProductionTarget, TestTarget>& sourceDependency,
            SelectedTestTargetAndDependerMap& selectedTestTargetMap);

        //! Iterate over the production and test targets for updated sources with coverage and act on them.
        [[nodiscard]] virtual SourceOperationActionResult IterateUpdateParentedSourcesWithCoverage(
            const SourceDependency<ProductionTarget, TestTarget>& sourceDependency,
            SelectedTestTargetAndDependerMap& selectedTestTargetMap);

        //! Iterate over the production and test targets for updated sources with no coverage and act on them.
        [[nodiscard]] virtual SourceOperationActionResult IterateUpdateParentedSourcesWithoutCoverage(
            const SourceDependency<ProductionTarget, TestTarget>& sourceDependency,
            SelectedTestTargetAndDependerMap& selectedTestTargetMap);

        //! Action to perform when production sources are created.
        [[nodiscard]] virtual SourceOperationActionResult CreateProductionSourceWithoutCoverageAction(
            const ProductionTarget* target, SelectedTestTargetAndDependerMap& selectedTestTargetMap);

        //! Action to perform when test sources are created.
        [[nodiscard]] virtual SourceOperationActionResult CreateTestSourceCreateProductionSourceWithoutCoverageAction(
            const TestTarget* target, SelectedTestTargetAndDependerMap& selectedTestTargetMap);

        //! Action to perform when production sources with coverage are updated.
        [[nodiscard]] virtual SourceOperationActionResult UpdateProductionSourceWithCoverageAction(
            const ProductionTarget* target,
            SelectedTestTargetAndDependerMap& selectedTestTargetMap,
            const SourceDependency<ProductionTarget, TestTarget>& sourceDependency);

        //! Action to perform when test sources with coverage are updated.
        [[nodiscard]] virtual SourceOperationActionResult UpdateTestSourceWithCoverageAction(
            const TestTarget* target,
            SelectedTestTargetAndDependerMap& selectedTestTargetMap);

        //! Action to perform when production sources without coverage are updated.
        [[nodiscard]] virtual SourceOperationActionResult UpdateProductionSourceWithoutCoverageAction(
            const ProductionTarget* target,
            SelectedTestTargetAndDependerMap& selectedTestTargetMap);

        //! Action to perform when test sources without coverage are updated.
        [[nodiscard]] virtual SourceOperationActionResult UpdateTestSourceWithoutCoverageAction(
            const TestTarget* target,
            SelectedTestTargetAndDependerMap& selectedTestTargetMap);

        //! Action to perform when sources that cannot be determined to be production or test sources with coverage are updated.
        [[nodiscard]] virtual SourceOperationActionResult UpdateIndeterminateSourceWithCoverageAction(
            SelectedTestTargetAndDependerMap& selectedTestTargetMap,
            const SourceDependency<ProductionTarget, TestTarget>& sourceDependency);

        //! Action to perform when sources that cannot be determined to be production or test sources with coverage are deleted.
        [[nodiscard]] virtual SourceOperationActionResult DeleteIndeterminateSourceWithCoverageAction(
            SelectedTestTargetAndDependerMap& selectedTestTargetMap, const SourceDependency<ProductionTarget, TestTarget>& sourceDependency);

        const DynamicDependencyMap<ProductionTarget, TestTarget>* m_dynamicDependencyMap;
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
    typename TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::SourceOperationActionResult
    TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::IterateCreateParentedSourcesWithNoCoverage(
        const SourceDependency<ProductionTarget, TestTarget>& sourceDependency,
        SelectedTestTargetAndDependerMap& selectedTestTargetMap)
    {
        for (const auto& parentTarget : sourceDependency.GetParentTargets())
        {
            if (parentTarget.GetTarget()->GetType() == TargetType::ProductionTarget)
            {
                // Parent Targets: Yes
                // Coverage Data : No
                // Source Type   : Production
                //
                // Scenario
                // 1. The file has been newly created
                // 2. This file exists in one or more source to production target mapping artifacts
                // 3. There exists no coverage data for this file in the source covering test list
                if (SourceOperationActionResult::ConcludeSelection ==
                    CreateProductionSourceWithoutCoverageAction(parentTarget.GetProductionTarget(), selectedTestTargetMap))
                {
                    return SourceOperationActionResult::ConcludeSelection;
                }
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
                if (SourceOperationActionResult::ConcludeSelection ==
                    CreateTestSourceCreateProductionSourceWithoutCoverageAction(parentTarget.GetTestTarget(), selectedTestTargetMap))
                {
                    return SourceOperationActionResult::ConcludeSelection;
                }
            }
        }

        return SourceOperationActionResult::Continue;
    }

    template<typename ProductionTarget, typename TestTarget>
    typename TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::SourceOperationActionResult
    TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::IterateUpdateParentedSourcesWithCoverage(
        const SourceDependency<ProductionTarget, TestTarget>& sourceDependency,
        SelectedTestTargetAndDependerMap& selectedTestTargetMap)
    {
        for (const auto& parentTarget : sourceDependency.GetParentTargets())
        {
            if (parentTarget.GetTarget()->GetType() == TargetType::ProductionTarget)
            {
                // Parent Targets: Yes
                // Coverage Data : Yes
                // Source Type   : Production
                //
                // Scenario
                // 1. The existing file has been modified
                // 2. This file exists in one or more source to production target mapping artifacts
                // 3. There exists coverage data for this file in the source covering test list
                if (SourceOperationActionResult::ConcludeSelection ==
                    UpdateProductionSourceWithCoverageAction(parentTarget.GetProductionTarget(), selectedTestTargetMap, sourceDependency))
                {
                    return SourceOperationActionResult::ConcludeSelection;
                }
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
                if (SourceOperationActionResult::ConcludeSelection ==
                    UpdateTestSourceWithCoverageAction(parentTarget.GetTestTarget(), selectedTestTargetMap))
                {
                    return SourceOperationActionResult::ConcludeSelection;
                }
            }
        }

        return SourceOperationActionResult::Continue;
    }

    template<typename ProductionTarget, typename TestTarget>
    typename TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::SourceOperationActionResult
    TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::IterateUpdateParentedSourcesWithoutCoverage(
        const SourceDependency<ProductionTarget, TestTarget>& sourceDependency,
        SelectedTestTargetAndDependerMap& selectedTestTargetMap)
    {
        for (const auto& parentTarget : sourceDependency.GetParentTargets())
        {
            if (parentTarget.GetTarget()->GetType() == TargetType::ProductionTarget)
            {
                // Parent Targets: Yes
                // Coverage Data : No
                // Source Type   : Production
                //
                // Scenario
                // 1. The existing file has been modified
                // 2. This file exists in one or more source to test target mapping artifacts
                // 3. There exists no coverage data for this file in the source covering test list
                if (SourceOperationActionResult::ConcludeSelection ==
                    UpdateProductionSourceWithoutCoverageAction(parentTarget.GetProductionTarget(), selectedTestTargetMap))
                {
                    return SourceOperationActionResult::ConcludeSelection;
                }
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
                if (SourceOperationActionResult::ConcludeSelection ==
                    UpdateTestSourceWithoutCoverageAction(parentTarget.GetTestTarget(), selectedTestTargetMap))
                {
                    return SourceOperationActionResult::ConcludeSelection;
                }
            }
        }

        return SourceOperationActionResult::Continue;
    }

    template<typename ProductionTarget, typename TestTarget>
    typename TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::SourceOperationActionResult
    TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::CreateProductionSourceWithoutCoverageAction(
        const ProductionTarget* target, SelectedTestTargetAndDependerMap& selectedTestTargetMap)
    {
        // Action
        // 1. Select all test targets covering the parent production targets
        const auto coverage = m_dynamicDependencyMap->GetCoveringTestTargetsForProductionTarget(*target);
        for (const auto* testTarget : coverage)
        {
            selectedTestTargetMap[testTarget].insert(target);
        }

        return SourceOperationActionResult::Continue;
    }

    template<typename ProductionTarget, typename TestTarget>
    typename TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::SourceOperationActionResult
    TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::CreateTestSourceCreateProductionSourceWithoutCoverageAction(
        const TestTarget* target, SelectedTestTargetAndDependerMap& selectedTestTargetMap)
    {
        // Action
        // 1. Select all parent test targets
        selectedTestTargetMap.insert(target);

        return SourceOperationActionResult::Continue;
    }

    template<typename ProductionTarget, typename TestTarget>
    typename TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::SourceOperationActionResult
    TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::
        UpdateProductionSourceWithCoverageAction(
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

        return SourceOperationActionResult::Continue;
    }

    template<typename ProductionTarget, typename TestTarget>
    typename TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::SourceOperationActionResult
    TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::
        UpdateTestSourceWithCoverageAction(
        const TestTarget* target, SelectedTestTargetAndDependerMap& selectedTestTargetMap)
    {
        // Action
        // 1. Select the parent test targets for this file
        selectedTestTargetMap.insert(target);

        return SourceOperationActionResult::Continue;
    }

    template<typename ProductionTarget, typename TestTarget>
    typename TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::SourceOperationActionResult
    TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::
        UpdateProductionSourceWithoutCoverageAction(
        [[maybe_unused]] const ProductionTarget* target,
        [[maybe_unused]] SelectedTestTargetAndDependerMap& selectedTestTargetMap)
    {
        // Action
        // 1. Skip the file
        return SourceOperationActionResult::Continue;
    }

    template<typename ProductionTarget, typename TestTarget>
    typename TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::SourceOperationActionResult
    TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::
        UpdateTestSourceWithoutCoverageAction(
        const TestTarget* target, SelectedTestTargetAndDependerMap& selectedTestTargetMap)
    {
        // Action
        // 1. Select the parent test targets for this file
        selectedTestTargetMap.insert(target);

        return SourceOperationActionResult::Continue;
    }

    template<typename ProductionTarget, typename TestTarget>
    typename TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::SourceOperationActionResult
    TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::
        UpdateIndeterminateSourceWithCoverageAction(
        SelectedTestTargetAndDependerMap& selectedTestTargetMap, const SourceDependency<ProductionTarget, TestTarget>& sourceDependency)
    {
        // Action
        // 1. Log potential orphaned source file warning (handled prior by DynamicDependencyMap::ApplyAndResoveChangeList)
        // 2. Select all test targets covering this file
        // 3. Delete the existing coverage data from the source covering test list (handled prior by DynamicDependencyMap::ApplyAndResoveChangeList)

        for (const auto* testTarget : sourceDependency.GetCoveringTestTargets())
        {
            selectedTestTargetMap.insert(testTarget);
        }

        return SourceOperationActionResult::Continue;
    }

    template<typename ProductionTarget, typename TestTarget>
    typename TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::SourceOperationActionResult
    TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::DeleteIndeterminateSourceWithCoverageAction(
        SelectedTestTargetAndDependerMap& selectedTestTargetMap, const SourceDependency<ProductionTarget, TestTarget>& sourceDependency)
    {
        // Action
        // 1. Select all test targets covering this file
        // 2. Delete the existing coverage data from the source covering test list (handled prior by DynamicDependencyMap::ApplyAndResoveChangeList)
        for (const auto* testTarget : sourceDependency.GetCoveringTestTargets())
        {
            selectedTestTargetMap.insert(testTarget);
        }

        return SourceOperationActionResult::Continue;
    }

    template<typename ProductionTarget, typename TestTarget>
    typename TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::SelectedTestTargetAndDependerMap TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::SelectTestTargets(
        const ChangeDependencyList<ProductionTarget, TestTarget>& changeDependencyList)
    {
        SelectedTestTargetAndDependerMap selectedTestTargetMap;

        // Create operations
        for (const auto& sourceDependency : changeDependencyList.GetCreateSourceDependencies())
        {
            if (sourceDependency.GetNumParentTargets())
            {
                if (sourceDependency.GetNumCoveringTestTargets())
                {
                    // Parent Targets: Yes
                    // Coverage Data : Yes
                    // Source Type   : Irrelevant
                    //
                    // Scenario
                    // 1. This file previously existed in one or more source to target mapping artifacts
                    // 2. The file has since been deleted yet no delete crud operation acted upon:
                    //    a) The coverage data for this file was not deleted from the Source Covering Test List
                    // 3. The file has since been recreated
                    // 4. This file exists in one or more source to target mapping artifacts
                    //
                    // Action (handled prior by DynamicDependencyMap::ApplyAndResoveChangeList)
                    // 1. Log Source Covering Test List compromised error
                    // 2. Throw exception
                    AZ_Warning(
                        "TestSelectorAndPrioritizer",
                        false,
                        AZStd::string::format(
                            "Create operation for file with parent targets and coverage data was not expected to be handled here for "
                            "file '%s'",
                            sourceDependency.GetPath().c_str())
                            .c_str());
                    continue;
                }
                else
                {
                    // Parent Targets: Yes
                    // Coverage Data : No
                    // Source Type   : Production or test
                    if (SourceOperationActionResult::ConcludeSelection ==
                        IterateCreateParentedSourcesWithNoCoverage(sourceDependency, selectedTestTargetMap))
                    {
                        return selectedTestTargetMap;
                    }
                }
            }
            else
            {
                if (sourceDependency.GetNumCoveringTestTargets())
                {
                    // Parent Targets: No
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
                    AZ_Warning(
                        "TestSelectorAndPrioritizer",
                        false,
                        "Create operation for file with no parent targets but coverage data was not expected to be handled here for "
                        "file '%s'",
                        sourceDependency.GetPath().c_str());
                    continue;
                }
                else
                {
                    // Parent Targets: No
                    // Coverage Data : No
                    // Source Type   : Irrelevant
                    //
                    // Scenario
                    // 1. The file has been newly created
                    // 2. This file does not exists in any source to target mapping artifacts.
                    //
                    // Action
                    // 1. Skip the file
                    AZ_Warning(
                        "TestSelectorAndPrioritizer",
                        false,
                        "Create operation for file with no parent targets or coverage data was not expected to be handled here for "
                        "file '%s'",
                        sourceDependency.GetPath().c_str());
                    continue;
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
                    // Parent Targets: Yes
                    // Coverage Data : Yes
                    // Source Type   : Production or test
                    if (SourceOperationActionResult::ConcludeSelection ==
                        IterateUpdateParentedSourcesWithCoverage(sourceDependency, selectedTestTargetMap))
                    {
                        return selectedTestTargetMap;
                    }
                }
                else
                {
                    // Parent Targets: Yes
                    // Coverage Data : No
                    // Source Type   : Production or test
                    if (SourceOperationActionResult::ConcludeSelection ==
                        IterateUpdateParentedSourcesWithoutCoverage(sourceDependency, selectedTestTargetMap))
                    {
                        return selectedTestTargetMap;
                    }
                }
            }
            else
            {
                if (sourceDependency.GetNumCoveringTestTargets())
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
                    if (SourceOperationActionResult::ConcludeSelection ==
                        UpdateIndeterminateSourceWithCoverageAction(selectedTestTargetMap, sourceDependency))
                    {
                        return selectedTestTargetMap;
                    }
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
                    AZ_Warning(
                        "TestSelectorAndPrioritizer",
                        false,
                        "Update operation for file with no parent targets or coverage data was not expected to be handled here for "
                        "file '%s'",
                        sourceDependency.GetPath().c_str());
                    continue;
                }
            }
        }

        // Delete operations
        for (const auto& sourceDependency : changeDependencyList.GetDeleteSourceDependencies())
        {
            if (sourceDependency.GetNumParentTargets())
            {
                if (sourceDependency.GetNumCoveringTestTargets())
                {
                    // Parent Targets: Yes
                    // Coverage Data : Yes
                    // Source Type   : Irrelevant
                    //
                    // Scenario
                    // 1. The file has been deleted.
                    // 2. This file still exists in one or more source to target mapping artifacts
                    // 2. There exists coverage data for this file in the Source Covering Test List
                    //
                    // Action (handled prior by DynamicDependencyMap::ApplyAndResoveChangeList)
                    // 1. Log source to target mapping and Source Covering Test List integrity compromised error
                    // 2. Throw exception
                    AZ_Warning(
                        "TestSelectorAndPrioritizer",
                        false,
                        "Delete operation for file with parent targets and coverage data was not expected to be handled here for "
                        "file '%s'",
                        sourceDependency.GetPath().c_str());
                    continue;
                }
                else
                {
                    // Parent Targets: Yes
                    // Coverage Data : No
                    // Source Type   : Irrelevant
                    //
                    // Scenario
                    // 1. The file has been deleted.
                    // 2. This file still exists in one or more source to target mapping artifacts
                    // 2. There exists no coverage data for this file in the Source Covering Test List
                    //
                    // Action (handled prior by DynamicDependencyMap::ApplyAndResoveChangeList)
                    // 1. Log source to target mapping and Source Covering Test List integrity compromised error
                    // 2. Throw exception
                    AZ_Warning(
                        "TestSelectorAndPrioritizer",
                        false,
                        "Delete operation for file with parent targets but no coverage data was not expected to be handled here for "
                        "file '%s'",
                        sourceDependency.GetPath().c_str());
                    continue;
                }
            }
            else
            {
                if (sourceDependency.GetNumCoveringTestTargets())
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
                    //
                    // Action (handled prior by DynamicDependencyMap::ApplyAndResoveChangeList)
                    // 1. Delete the existing coverage data from the Source Covering Test List
                    // 2. Skip the file
                    if (SourceOperationActionResult::ConcludeSelection ==
                        DeleteIndeterminateSourceWithCoverageAction(selectedTestTargetMap, sourceDependency))
                    {
                        return selectedTestTargetMap;
                    }
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
                    // Action (handled prior by DynamicDependencyMap::ApplyAndResoveChangeList)
                    // 1. Skip the file
                    AZ_Warning(
                        "TestSelectorAndPrioritizer",
                        false,
                        AZStd::string::format(
                            "Delete operation for file with no parent targets or coverage data was not expected to be handled here for "
                            "file '%s'",
                            sourceDependency.GetPath().c_str())
                            .c_str());
                    continue;
                }
            }
        }

        return selectedTestTargetMap;
    }

    template<typename ProductionTarget, typename TestTarget>
    AZStd::vector<const TestTarget*> TestSelectorAndPrioritizer<ProductionTarget, TestTarget>::PrioritizeSelectedTestTargets(
        const SelectedTestTargetAndDependerMap& selectedTestTargetAndDependerMap, const Policy::TestPrioritization testSelectionStrategy)
    {
        AZStd::vector<const TestTarget*> selectedTestTargets;
        selectedTestTargets.reserve(selectedTestTargetAndDependerMap.size());

        const auto fillSelectedTestTargets = [&selectedTestTargets](const auto& containerOfPairs)
        {
            AZStd::transform(
                containerOfPairs.begin(),
                containerOfPairs.end(),
                AZStd::back_inserter(selectedTestTargets),
                [](const auto& pair)
                {
                    return pair.first;
                });
        };

        if (testSelectionStrategy == Policy::TestPrioritization::DependencyLocality)
        {
            const BuildTargetList<ProductionTarget, TestTarget>* buildTargetList = m_dynamicDependencyMap->GetBuildTargetList();
            const BuildGraph<ProductionTarget, TestTarget>& buildGraph = buildTargetList->GetBuildGraph();

            AZStd::vector<AZStd::pair<const TestTarget*, AZStd::optional<AZStd::size_t>>> testTargetDistancePairs;
            testTargetDistancePairs.reserve(selectedTestTargetAndDependerMap.size());

            // Loop through each test target in our map, walk the build dependency graph for that target and retrieve the vertices for the
            // production targets in productionTargets. Find and store the distance to the closest productionTarget if one exists.
            for (const auto [testTarget, productionTargets] : selectedTestTargetAndDependerMap)
            {
                AZStd::optional<AZStd::size_t> minimum;
                buildGraph.WalkBuildDependencies(
                    buildTargetList->GetBuildTargetOrThrow(testTarget->GetName()),
                    // Workaround for C++ Standard bug where structured bindings names aren't capturable in lambdas
                    // https://stackoverflow.com/a/46115028
                    [&, productionTargets = productionTargets](const BuildGraphVertex<ProductionTarget, TestTarget>& vertex, AZStd::size_t distance)
                    {
                        if (const auto productionTarget = vertex.m_buildTarget.GetProductionTarget();
                            productionTarget && productionTargets.contains(productionTarget))
                        {
                            if (!minimum.has_value() || distance < minimum)
                            {
                                minimum = distance;
                            };
                        }
                        return BuildGraphVertexVisitResult::Continue;
                    });

                // If we found any productionTargets, store the distance to the closest one.
                // Else we didn't find any vertices, store the testTarget with no distance. This will be interpreted as infinite distance
                // and the test targets priority will be lower
                if (minimum.has_value())
                {
                    testTargetDistancePairs.emplace_back(testTarget, minimum);
                }
                else
                {
                    testTargetDistancePairs.emplace_back(testTarget);
                }
            }

            // Sort our pairs by distance and put our test targets into the vector in order of distance (closest first)
            AZStd::sort(
                testTargetDistancePairs.begin(),
                testTargetDistancePairs.end(),
                [](const auto& left, const auto& right)
                {
                    if (!left.second.has_value())
                    {
                        return false;
                    }
                    else if (!right.second.has_value())
                    {
                        return true;
                    }

                    return left.second.value() < right.second.value();
                });

            fillSelectedTestTargets(testTargetDistancePairs);
        }
        else
        {
            fillSelectedTestTargets(selectedTestTargetAndDependerMap);
        }

        return selectedTestTargets;
    }
} // namespace TestImpact
