/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Target/Common/TestImpactTargetException.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace TestImpact
{
    template<typename BuildSystemType>
    class BuildTargetList
    {
    public:
        //! Constructs the dependency map with entries for each build target's source files with empty test coverage data.
        BuildTargetList(
            AZStd::vector<AZStd::unique_ptr<typename BuildSystemType::TestTarget::Descriptor>>&& testTargetDescriptors,
            AZStd::vector<AZStd::unique_ptr<typename BuildSystemType::ProductionTarget::Descriptor>>&& productionTargetDescriptors);

        //! Gets the total number of production and test targets in the repository.
        size_t GetNumTargets() const;

        //! Attempts to get the specified target's specialized type.
        //! @param name The name of the target to get.
        //! @returns If found, the pointer to the specialized target, otherwise AZStd::monostate.
        typename BuildSystemType::OptionalBuildTarget GetBuildTarget(const AZStd::string& name) const;

        //! Attempts to get the specified target's specialized type or throw TargetException.
        //! @param name The name of the target to get.
        typename BuildSystemType::BuildTarget GetBuildTargetOrThrow(const AZStd::string& name) const;

        //! Get the list of test targets in the repository.
        const typename BuildSystemType::TestTargetList& GetTestTargetList() const;

        //! Get the list of production targets in the repository.
        const typename BuildSystemType::ProductionTargetList& GetProductionTargetList() const;
    private:
        //! The sorted list of unique test targets in the repository.
        typename BuildSystemType::TestTargetList m_testTargets;

        //! The sorted list of unique production targets in the repository.
        typename BuildSystemType::ProductionTargetList m_productionTargets;
    };

    template<typename BuildSystemType>
    BuildTargetList<BuildSystemType>::BuildTargetList(
        AZStd::vector<AZStd::unique_ptr<typename BuildSystemType::TestTarget::Descriptor>>&& testTargetDescriptors,
        AZStd::vector<AZStd::unique_ptr<typename BuildSystemType::ProductionTarget::Descriptor>>&& productionTargetDescriptors)
        : m_testTargets(AZStd::move(testTargetDescriptors))
        , m_productionTargets(AZStd::move(productionTargetDescriptors))
    {
    }

    template<typename BuildSystemType>
    size_t BuildTargetList<BuildSystemType>::GetNumTargets() const
    {
        return m_productionTargets.GetNumTargets() + m_testTargets.GetNumTargets();
    }

    template<typename BuildSystemType>
    typename BuildSystemType::OptionalBuildTarget BuildTargetList<BuildSystemType>::GetBuildTarget(const AZStd::string& name) const
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

    template<typename BuildSystemType>
    typename typename BuildSystemType::BuildTarget BuildTargetList<BuildSystemType>::GetBuildTargetOrThrow(const AZStd::string& name) const
    {
        typename BuildSystemType::BuildTarget buildTarget;
        AZStd::visit([&buildTarget, &name](auto&& target)
        {
            if constexpr (BuildSystemType::template IsProductionTarget<decltype(target)> || BuildSystemType::template IsTestTarget<decltype(target)>)
            {
                buildTarget = target;
            }
            else
            {
                throw(TargetException(AZStd::string::format("Couldn't find target %s", name.c_str()).c_str()));
            }
        }, GetBuildTarget(name));

        return buildTarget;
    }

    template<typename BuildSystemType>
    const typename BuildSystemType::TestTargetList& BuildTargetList<BuildSystemType>::GetTestTargetList() const
    {
        return m_testTargets;
    }

    template<typename BuildSystemType>
    const typename BuildSystemType::ProductionTargetList& BuildTargetList<BuildSystemType>::GetProductionTargetList() const
    {
        return m_productionTargets;
    }
} // namespace TestImpact
