/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Target/Common/TestImpactTargetException.h>

#include <AzCore/std/containers/variant.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace TestImpact
{
    template<typename BuildTargetTraits>
    class BuildTargetList
    {
    public:
        //! Constructs the dependency map with entries for each build target's source files with empty test coverage data.
        BuildTargetList(
            AZStd::vector<AZStd::unique_ptr<typename BuildTargetTraits::TestTarget::Descriptor>>&& testTargetDescriptors,
            AZStd::vector<AZStd::unique_ptr<typename BuildTargetTraits::ProductionTarget::Descriptor>>&& productionTargetDescriptors);

        //! Gets the total number of production and test targets in the repository.
        size_t GetNumTargets() const;

        //! Attempts to get the specified target's specialized type.
        //! @param name The name of the target to get.
        //! @returns If found, the pointer to the specialized target, otherwise AZStd::monostate.
        typename BuildTargetTraits::OptionalBuildTarget GetBuildTarget(const AZStd::string& name) const;

        //! Attempts to get the specified target's specialized type or throw TargetException.
        //! @param name The name of the target to get.
        typename BuildTargetTraits::BuildTarget GetBuildTargetOrThrow(const AZStd::string& name) const;

        //! Get the list of test targets in the repository.
        const typename BuildTargetTraits::TestTargetList& GetTestTargetList() const;

        //! Get the list of production targets in the repository.
        const typename BuildTargetTraits::ProductionTargetList& GetProductionTargetList() const;
    private:
        //! The sorted list of unique test targets in the repository.
        typename BuildTargetTraits::TestTargetList m_testTargets;

        //! The sorted list of unique production targets in the repository.
        typename BuildTargetTraits::ProductionTargetList m_productionTargets;
    };

    template<typename BuildTargetTraits>
    BuildTargetList<BuildTargetTraits>::BuildTargetList(
        AZStd::vector<AZStd::unique_ptr<typename BuildTargetTraits::TestTarget::Descriptor>>&& testTargetDescriptors,
        AZStd::vector<AZStd::unique_ptr<typename BuildTargetTraits::ProductionTarget::Descriptor>>&& productionTargetDescriptors)
        : m_testTargets(AZStd::move(testTargetDescriptors))
        , m_productionTargets(AZStd::move(productionTargetDescriptors))
    {
    }

    template<typename BuildTargetTraits>
    size_t BuildTargetList<BuildTargetTraits>::GetNumTargets() const
    {
        return m_productionTargets.GetNumTargets() + m_testTargets.GetNumTargets();
    }

    template<typename BuildTargetTraits>
    typename BuildTargetTraits::OptionalBuildTarget BuildTargetList<BuildTargetTraits>::GetBuildTarget(const AZStd::string& name) const
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

    template<typename BuildTargetTraits>
    typename typename BuildTargetTraits::BuildTarget BuildTargetList<BuildTargetTraits>::GetBuildTargetOrThrow(const AZStd::string& name) const
    {
        auto buildTarget = GetBuildTarget(name);
        AZ_TestImpact_Eval(buildTarget.has_value(), TargetException, AZStd::string::format("Couldn't find target %s", name.c_str()).c_str());
        return buildTarget.value();
    }

    template<typename BuildTargetTraits>
    const typename BuildTargetTraits::TestTargetList& BuildTargetList<BuildTargetTraits>::GetTestTargetList() const
    {
        return m_testTargets;
    }

    template<typename BuildTargetTraits>
    const typename BuildTargetTraits::ProductionTargetList& BuildTargetList<BuildTargetTraits>::GetProductionTargetList() const
    {
        return m_productionTargets;
    }
} // namespace TestImpact
