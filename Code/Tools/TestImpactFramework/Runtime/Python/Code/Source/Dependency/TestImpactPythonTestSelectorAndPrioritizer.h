/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Dependency/TestImpactTestSelectorAndPrioritizer.h>
#include <Dependency/TestImpactDynamicDependencyMap.h>
#include <Target/Python/TestImpactPythonProductionTarget.h>
#include <Target/Python/TestImpactPythonTestTarget.h>

namespace TestImpact
{
    //! Test selector and prioritizer for the Python tests.
    class PythonTestSelectorAndPrioritizer
        : public TestSelectorAndPrioritizer<PythonProductionTarget, PythonTestTarget>
    {
        using ProductionTarget = PythonProductionTarget;
        using TestTarget = PythonTestTarget;
        using TestSelectorAndPrioritizerBase = TestSelectorAndPrioritizer<ProductionTarget, TestTarget>;
    public:
        using TestSelectorAndPrioritizerBase::TestSelectorAndPrioritizer;

    protected:
        // TestSelectorAndPrioritizer overrides ...
        [[nodiscard]] SourceOperationActionResult CreateProductionSourceWithoutCoverageAction(
            const ProductionTarget* target, SelectedTestTargetAndDependerMap& selectedTestTargetMap) override;
        [[nodiscard]] SourceOperationActionResult UpdateProductionSourceWithoutCoverageAction(
            const ProductionTarget* target, SelectedTestTargetAndDependerMap& selectedTestTargetMap) override;
        [[nodiscard]] virtual SourceOperationActionResult UpdateProductionSourceWithCoverageAction(
            const ProductionTarget* target,
            SelectedTestTargetAndDependerMap& selectedTestTargetMap,
            const SourceDependency<ProductionTarget, TestTarget>& sourceDependency) override;
    };

    typename PythonTestSelectorAndPrioritizer::SourceOperationActionResult
    PythonTestSelectorAndPrioritizer::CreateProductionSourceWithoutCoverageAction(
        const ProductionTarget* target, SelectedTestTargetAndDependerMap& selectedTestTargetMap)
    {
        if (m_dynamicDependencyMap->GetCoveringTestTargetsForProductionTarget(*target).empty())
        {
            // Parent Targets      : Yes
            // Coverage Data       : No
            // Parent Coverage Data: Mixed to None
            // Source Type         : Production
            //
            // Scenario
            // 1. The file has been newly created
            // 2. This file exists in one or more source to production target mapping artifacts
            // 3. There exists no coverage data for this file in the Source Covering Test List
            // 4. One or more parent targets do not have coverage data in the Source Covering Test List
            //
            // Action
            // 1. Select all test targets
            for (const auto& testTarget : m_dynamicDependencyMap->GetBuildTargetList()->GetTestTargetList().GetTargets())
            {
                selectedTestTargetMap.insert(&testTarget);
            }

            return SourceOperationActionResult::ConcludeSelection;
        }

        return TestSelectorAndPrioritizerBase::CreateProductionSourceWithoutCoverageAction(target, selectedTestTargetMap);
    }

    typename PythonTestSelectorAndPrioritizer::SourceOperationActionResult
        PythonTestSelectorAndPrioritizer::UpdateProductionSourceWithoutCoverageAction(
            const ProductionTarget* target, SelectedTestTargetAndDependerMap& selectedTestTargetMap)
    {
        if (m_dynamicDependencyMap->GetCoveringTestTargetsForProductionTarget(*target).empty())
        {
            // Parent Targets      : Yes
            // Coverage Data       : No
            // Parent Coverage Data: Mixed to None
            // Source Type         : Production
            //
            // Scenario
            // 1. The file has been modifed
            // 2. This file exists in one or more source to production target mapping artifacts
            // 3. There exists no coverage data for this file in the Source Covering Test List
            // 4. One or more parent targets do not have coverage data in the Source Covering Test List
            //
            // Action
            // 1. Select all test targets
            for (const auto& testTarget : m_dynamicDependencyMap->GetBuildTargetList()->GetTestTargetList().GetTargets())
            {
                selectedTestTargetMap.insert(&testTarget);
            }

            return SourceOperationActionResult::ConcludeSelection;
        }

        return TestSelectorAndPrioritizerBase::UpdateProductionSourceWithoutCoverageAction(target, selectedTestTargetMap);
    }

    typename PythonTestSelectorAndPrioritizer::SourceOperationActionResult
        PythonTestSelectorAndPrioritizer::UpdateProductionSourceWithCoverageAction(
        const ProductionTarget* target,
        SelectedTestTargetAndDependerMap& selectedTestTargetMap,
        const SourceDependency<ProductionTarget, TestTarget>& sourceDependency)
    {
        if (m_dynamicDependencyMap->GetCoveringTestTargetsForProductionTarget(*target).empty())
        {
            // Parent Targets      : Yes
            // Coverage Data       : Yes
            // Parent Coverage Data: Mixed to None
            // Source Type         : Production
            //
            // Scenario
            // 1. The file has been modifed
            // 2. This file exists in one or more source to production target mapping artifacts
            // 3. There exists coverage data for this file in the Source Covering Test List
            // 4. One or more parent targets do not have coverage data in the Source Covering Test List
            //
            // Action
            // 1. Select all test targets
            for (const auto& testTarget : m_dynamicDependencyMap->GetBuildTargetList()->GetTestTargetList().GetTargets())
            {
                selectedTestTargetMap.insert(&testTarget);
            }

            return SourceOperationActionResult::ConcludeSelection;
        }

        return TestSelectorAndPrioritizerBase::UpdateProductionSourceWithCoverageAction(target, selectedTestTargetMap, sourceDependency);
    }
} // namespace TestImpact
