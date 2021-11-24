/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Dependency/TestImpactDependencyException.h>
#include <Dependency/TestImpactDynamicDependencyMap.h>
#include <Dependency/TestImpactTestSelectorAndPrioritizer.h>
#include <Target/Native/TestImpactNativeTestTarget.h>
#include <Target/Native/TestImpactNativeTestTargetList.h>
#include <Target/Native/TestImpactNativeProductionTargetList.h>

namespace TestImpact
{
    TestSelectorAndPrioritizer::TestSelectorAndPrioritizer(
        const DynamicDependencyMap<NativeBuildSystem>* dynamicDependencyMap,
        DependencyGraphDataMap&& dependencyGraphDataMap)
        : m_dynamicDependencyMap(dynamicDependencyMap)
        , m_dependencyGraphDataMap(AZStd::move(dependencyGraphDataMap))
    {
    }

    AZStd::vector<const NativeTestTarget*> TestSelectorAndPrioritizer::SelectTestTargets(
        const ChangeDependencyList& changeDependencyList, Policy::TestPrioritization testSelectionStrategy)
    {
        const auto selectedTestTargetAndDependerMap = SelectTestTargets(changeDependencyList);
        const auto prioritizedSelectedTests = PrioritizeSelectedTestTargets(selectedTestTargetAndDependerMap, testSelectionStrategy);
        return prioritizedSelectedTests;
    }

    void TestSelectorAndPrioritizer::CreateProductionSourceAction(
        const NativeProductionTarget* target, SelectedTestTargetAndDependerMap& selectedTestTargetMap)
    {
        // Action
        // 1. Select all test targets covering the parent production targets
        const auto coverage = m_dynamicDependencyMap->GetCoveringTestTargetsForProductionTarget(*target);
        for (const auto* testTarget : coverage)
        {
            selectedTestTargetMap[testTarget].insert(target);
        }
    }

    void TestSelectorAndPrioritizer::CreateTestSourceAction(
        const NativeTestTarget* target, SelectedTestTargetAndDependerMap& selectedTestTargetMap)
    {
        // Action
        // 1. Select all parent test targets
        selectedTestTargetMap.insert(target);
    }

    void TestSelectorAndPrioritizer::UpdateProductionSourceWithCoverageAction(
        const NativeProductionTarget* target, SelectedTestTargetAndDependerMap& selectedTestTargetMap, const SourceDependency& sourceDependency)
    {
        // Action
        // 1. Select all test targets covering this file
        for (const auto* testTarget : sourceDependency.GetCoveringTestTargets())
        {
            selectedTestTargetMap[testTarget].insert(target);
        }
    }

    void TestSelectorAndPrioritizer::UpdateTestSourceWithCoverageAction(
        const NativeTestTarget* target, SelectedTestTargetAndDependerMap& selectedTestTargetMap)
    {
        // Action
        // 1. Select the parent test targets for this file
        selectedTestTargetMap.insert(target);
    }

    void TestSelectorAndPrioritizer::UpdateProductionSourceWithoutCoverageAction(
        [[maybe_unused]] const NativeProductionTarget* target, [[maybe_unused]] SelectedTestTargetAndDependerMap& selectedTestTargetMap)
    {
        // Action
        // 1. Do nothing
    }

    void TestSelectorAndPrioritizer::UpdateTestSourceWithoutCoverageAction(
        const NativeTestTarget* target, SelectedTestTargetAndDependerMap& selectedTestTargetMap)
    {
        // Action
        // 1. Select the parent test targets for this file
        selectedTestTargetMap.insert(target);
    }

    void TestSelectorAndPrioritizer::UpdateIndeterminateSourceWithoutCoverageAction(
        SelectedTestTargetAndDependerMap& selectedTestTargetMap, const SourceDependency& sourceDependency)
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

    void TestSelectorAndPrioritizer::DeleteIndeterminateSourceWithoutCoverageAction(
        SelectedTestTargetAndDependerMap& selectedTestTargetMap, const SourceDependency& sourceDependency)
    {
        // Action
        // 1. Select all test targets covering this file
        // 2. Delete the existing coverage data from the source covering test list (handled prior by DynamicDependencyMap)
        for (const auto* testTarget : sourceDependency.GetCoveringTestTargets())
        {
            selectedTestTargetMap.insert(testTarget);
        }
    }

    TestSelectorAndPrioritizer::SelectedTestTargetAndDependerMap TestSelectorAndPrioritizer::SelectTestTargets(
        const ChangeDependencyList& changeDependencyList)
    {
        SelectedTestTargetAndDependerMap selectedTestTargetMap;

        // Create operations
        for (const auto& sourceDependency : changeDependencyList.GetCreateSourceDependencies())
        {
            for (const auto& parentTarget : sourceDependency.GetParentTargets())
            {
                AZStd::visit([&selectedTestTargetMap, this](auto&& target)
                {
                    if constexpr (NativeBuildSystem::IsProductionTarget<decltype(target)>)
                    {
                        // Parent Targets: Yes
                        // Coverage Data : No
                        // Source Type   : Production
                        //
                        // Scenario
                        // 1. The file has been newly created
                        // 2. This file exists in one or more source to production target mapping artifacts
                        // 3. There exists no coverage data for this file in the source covering test list
                        CreateProductionSourceAction(target, selectedTestTargetMap);
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
                        CreateTestSourceAction(target, selectedTestTargetMap);
                    }
                }, parentTarget.GetTarget());
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
                        AZStd::visit([&selectedTestTargetMap, &sourceDependency, this](auto&& target)
                        {
                            if constexpr (NativeBuildSystem::IsProductionTarget<decltype(target)>)
                            {
                                // Parent Targets: Yes
                                // Coverage Data : Yes
                                // Source Type   : Production
                                //
                                // Scenario
                                // 1. The existing file has been modified
                                // 2. This file exists in one or more source to production target mapping artifacts
                                // 3. There exists coverage data for this file in the source covering test list
                                UpdateProductionSourceWithCoverageAction(target, selectedTestTargetMap, sourceDependency);
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
                                UpdateTestSourceWithCoverageAction(target, selectedTestTargetMap);
                            }
                        }, parentTarget.GetTarget());
                    }
                }
                else
                {
                    for (const auto& parentTarget : sourceDependency.GetParentTargets())
                    {
                        AZStd::visit([&selectedTestTargetMap, this](auto&& target)
                        {
                            if constexpr (NativeBuildSystem::IsProductionTarget<decltype(target)>)
                            {
                                // Parent Targets: Yes
                                // Coverage Data : No
                                // Source Type   : Production
                                //
                                // Scenario
                                // 1. The existing file has been modified
                                // 2. This file exists in one or more source to test target mapping artifacts
                                // 3. There exists no coverage data for this file in the source covering test list
                                UpdateProductionSourceWithoutCoverageAction(target, selectedTestTargetMap);
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
                                UpdateTestSourceWithoutCoverageAction(target, selectedTestTargetMap);
                            }
                        }, parentTarget.GetTarget());
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

    AZStd::vector<const NativeTestTarget*> TestSelectorAndPrioritizer::PrioritizeSelectedTestTargets(
        const SelectedTestTargetAndDependerMap& selectedTestTargetAndDependerMap,
        [[maybe_unused]] Policy::TestPrioritization testSelectionStrategy)
    {
        AZStd::vector<const NativeTestTarget*> selectedTestTargets;

        // Prioritization disabled for now
        // SPEC-6563
        for (const auto& [testTarget, dependerTargets] : selectedTestTargetAndDependerMap)
        {
            selectedTestTargets.push_back(testTarget);
        }
    
        return selectedTestTargets;
    }
} // namespace TestImpact
