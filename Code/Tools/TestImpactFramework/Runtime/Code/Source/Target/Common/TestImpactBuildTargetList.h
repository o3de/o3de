/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Target/Common/TestImpactBuildTarget.h>
#include <Target/Common/TestImpactTargetException.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace TestImpact
{
    template<typename TestTargetListType, typename ProductionTargetListType>
    class BuildTargetList
    {
    public:
        using TestTargetList = TestTargetListType;
        using ProductionTargetList = ProductionTargetListType;
        using TestTarget = typename TestTargetList::TargetType;
        using ProductionTarget = typename ProductionTargetList::TargetType;
        using Target = BuildTarget<TestTarget, ProductionTarget>;
        using OptionalTarget = OptionalBuildTarget<TestTarget, ProductionTarget>;

        //! Constructs the dependency map with entries for each build target's source files with empty test coverage data.
        BuildTargetList(
            AZStd::vector<AZStd::unique_ptr<typename TestTarget::Descriptor>>&& testTargetDescriptors,
            AZStd::vector<AZStd::unique_ptr<typename ProductionTarget::Descriptor>>&& productionTargetDescriptors);

        //! Gets the total number of production and test targets in the repository.
        size_t GetNumTargets() const;

        //! Attempts to get the specified target's specialized type.
        //! @param name The name of the target to get.
        //! @returns If found, the pointer to the specialized target, otherwise AZStd::monostate.
        OptionalTarget GetTarget(const AZStd::string& name) const;

        //! Attempts to get the specified target's specialized type or throw TargetException.
        //! @param name The name of the target to get.
        Target GetTargetOrThrow(const AZStd::string& name) const;

        //! Get the list of test targets in the repository.
        const TestTargetList& GetTestTargetList() const;

        //! Get the list of production targets in the repository.
        const ProductionTargetList& GetProductionTargetList() const;

        template<typename Target>
        static constexpr bool IsProductionTarget =
            AZStd::is_same_v<ProductionTarget, AZStd::remove_const_t<AZStd::remove_pointer_t<AZStd::decay_t<Target>>>>;

        template<typename Target>
        static constexpr bool IsTestTarget =
            AZStd::is_same_v<TestTarget, AZStd::remove_const_t<AZStd::remove_pointer_t<AZStd::decay_t<Target>>>>;

    private:
        //! The sorted list of unique test targets in the repository.
        TestTargetList m_testTargets;

        //! The sorted list of unique production targets in the repository.
        ProductionTargetList m_productionTargets;
    };

    template<typename TestTargetList, typename ProductionTargetList>
    BuildTargetList<TestTargetList, ProductionTargetList>::BuildTargetList(
        AZStd::vector<AZStd::unique_ptr<typename TestTarget::Descriptor>>&& testTargetDescriptors,
        AZStd::vector<AZStd::unique_ptr<typename ProductionTarget::Descriptor>>&& productionTargetDescriptors)
        : m_testTargets(AZStd::move(testTargetDescriptors))
        , m_productionTargets(AZStd::move(productionTargetDescriptors))
    {
    }

    template<typename TestTargetList, typename ProductionTargetList>
    size_t BuildTargetList<TestTargetList, ProductionTargetList>::GetNumTargets() const
    {
        return m_productionTargets.GetNumTargets() + m_testTargets.GetNumTargets();
    }

    template<typename TestTargetList, typename ProductionTargetList>
    typename BuildTargetList<TestTargetList, ProductionTargetList>::OptionalTarget BuildTargetList<
        TestTargetList,
        ProductionTargetList>::
        GetTarget(const AZStd::string& name) const
    {
        if (const auto testTarget = m_testTargets.GetTarget(name); testTarget != nullptr)
        {
            return testTarget;
        }
        else if (auto productionTarget = m_productionTargets.GetTarget(name); productionTarget != nullptr)
        {
            return productionTarget;
        }

        return AZStd::monostate{};
    }

    template<typename TestTargetList, typename ProductionTargetList>
    typename BuildTargetList<TestTargetList, ProductionTargetList>::Target BuildTargetList<TestTargetList, ProductionTargetList>::
        GetTargetOrThrow(const AZStd::string& name) const
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

    template<typename TestTargetList, typename ProductionTargetList>
    const TestTargetList& BuildTargetList<TestTargetList, ProductionTargetList>::GetTestTargetList() const
    {
        return m_testTargets;
    }

    template<typename TestTargetList, typename ProductionTargetList>
    const ProductionTargetList& BuildTargetList<TestTargetList, ProductionTargetList>::GetProductionTargetList() const
    {
        return m_productionTargets;
    }
} // namespace TestImpact
