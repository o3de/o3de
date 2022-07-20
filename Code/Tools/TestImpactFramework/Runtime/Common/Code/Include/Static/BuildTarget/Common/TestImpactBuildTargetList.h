/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <BuildTarget/Common/TestImpactBuildTargetException.h>
#include <Target/Common/TestImpactTargetList.h>

#include <AzCore/std/containers/variant.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace TestImpact
{
    template<typename TestTarget, typename ProductionTarget>
    class BuildTargetList
    {
    public:
        //! Constructs the dependency map with entries for each build target's source files with empty test coverage data.
        BuildTargetList(
            AZStd::vector<AZStd::unique_ptr<typename TestTarget::Descriptor>>&& testTargetDescriptors,
            AZStd::vector<AZStd::unique_ptr<typename ProductionTarget::Descriptor>>&& productionTargetDescriptors);

        //! Gets the total number of production and test targets in the repository.
        size_t GetNumTargets() const;

        //! Attempts to get the specified target's specialized type.
        //! @param name The name of the target to get.
        //! @returns If found, the pointer to the specialized target, otherwise AZStd::monostate.
        OptionalBuildTarget<TestTarget, ProductionTarget> GetBuildTarget(const AZStd::string& name) const;

        //! Attempts to get the specified target's specialized type or throw TargetException.
        //! @param name The name of the target to get.
        BuildTarget<TestTarget, ProductionTarget> GetBuildTargetOrThrow(const AZStd::string& name) const;

        //! Get the list of test targets in the repository.
        const TargetList<TestTarget>& GetTestTargetList() const;

        //! Get the list of production targets in the repository.
        const TargetList<ProductionTarget>& GetProductionTargetList() const;
    private:
        //! The sorted list of unique test targets in the repository.
        TargetList<TestTarget> m_testTargets;

        //! The sorted list of unique production targets in the repository.
        TargetList<ProductionTarget> m_productionTargets;
    };

    template<typename TestTarget, typename ProductionTarget>
    BuildTargetList<TestTarget, ProductionTarget>::BuildTargetList(
        AZStd::vector<AZStd::unique_ptr<typename TestTarget::Descriptor>>&& testTargetDescriptors,
        AZStd::vector<AZStd::unique_ptr<typename ProductionTarget::Descriptor>>&& productionTargetDescriptors)
        : m_testTargets(AZStd::move(testTargetDescriptors))
        , m_productionTargets(AZStd::move(productionTargetDescriptors))
    {
    }

    template<typename TestTarget, typename ProductionTarget>
    size_t BuildTargetList<TestTarget, ProductionTarget>::GetNumTargets() const
    {
        return m_productionTargets.GetNumTargets() + m_testTargets.GetNumTargets();
    }

    template<typename TestTarget, typename ProductionTarget>
    OptionalBuildTarget<TestTarget, ProductionTarget> BuildTargetList<TestTarget, ProductionTarget>::GetBuildTarget(const AZStd::string& name) const
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

        return AZStd::nullopt;
    }

    template<typename TestTarget, typename ProductionTarget>
    BuildTarget<TestTarget, ProductionTarget> BuildTargetList<TestTarget, ProductionTarget>::GetBuildTargetOrThrow(
        const AZStd::string& name) const
    {
        auto buildTarget = GetBuildTarget(name);
        AZ_TestImpact_Eval(buildTarget.has_value(), TargetException, AZStd::string::format("Couldn't find target %s", name.c_str()).c_str());
        return buildTarget.value();
    }

    template<typename TestTarget, typename ProductionTarget>
    const TargetList<TestTarget>& BuildTargetList<TestTarget, ProductionTarget>::GetTestTargetList() const
    {
        return m_testTargets;
    }

    template<typename TestTarget, typename ProductionTarget>
    const TargetList<ProductionTarget>& BuildTargetList<TestTarget, ProductionTarget>::GetProductionTargetList() const
    {
        return m_productionTargets;
    }
} // namespace TestImpact
