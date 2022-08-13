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
    template<typename ProductionTarget, typename TestTarget>
    class BuildTargetList
    {
    public:
        //! Constructs the list of production and test targets in the respository.
        BuildTargetList(
            TargetList<TestTarget>&& testTargetList, TargetList<ProductionTarget>&& productionTargetList);

        //! Gets the total number of production and test targets in the repository.
        size_t GetNumTargets() const;

        //! Attempts to get the specified target's specialized type.
        //! @param name The name of the target to get.
        //! @returns If found, the pointer to the specialized target, otherwise AZStd::monostate.
        OptionalBuildTarget<ProductionTarget, TestTarget> GetBuildTarget(const AZStd::string& name) const;

        //! Attempts to get the specified target's specialized type or throw TargetException.
        //! @param name The name of the target to get.
        BuildTarget<ProductionTarget, TestTarget> GetBuildTargetOrThrow(const AZStd::string& name) const;

        //! Attempts to get the target name from the target's output name.
        //! @param outputName The output name of the target to get.
        //! @returns If found, the name of the build target, otherwise an empty string.
        AZStd::string GetTargetNameFromOutputName(const AZStd::string& outputName) const;
        
        //! Attempts to get the target name from the target's output name or throw TargetException.
        //! @param outputName The output name of the target to get.
        AZStd::string GetTargetNameFromOutputNameOrThrow(const AZStd::string& outputName) const;

        //! Get the list of test targets in the repository.
        const TargetList<TestTarget>& GetTestTargetList() const;

        //! Get the list of production targets in the repository.
        const TargetList<ProductionTarget>& GetProductionTargetList() const;
    private:
        //! The sorted list of unique test targets in the repository.
        TargetList<TestTarget> m_testTargets;

        //! The sorted list of unique production targets in the repository.
        TargetList<ProductionTarget> m_productionTargets;

        //! Mapping of target output names to their targets.
        AZStd::unordered_map<AZStd::string, AZStd::string> m_outputNameToTargetNameMapping;
    };

    template<typename ProductionTarget, typename TestTarget>
    BuildTargetList<ProductionTarget, TestTarget>::BuildTargetList(
        TargetList<TestTarget>&& testTargetList, TargetList<ProductionTarget>&& productionTargetList)
        : m_testTargets(AZStd::move(testTargetList))
        , m_productionTargets(AZStd::move(productionTargetList))
    {
        for (const auto& target : m_testTargets.GetTargets())
        {
            m_outputNameToTargetNameMapping[target.GetOutputName()] = target.GetName();
        }

        for (const auto& target : m_productionTargets.GetTargets())
        {
            m_outputNameToTargetNameMapping[target.GetOutputName()] = target.GetName();
        }
    }

    template<typename ProductionTarget, typename TestTarget>
    size_t BuildTargetList<ProductionTarget, TestTarget>::GetNumTargets() const
    {
        return m_productionTargets.GetNumTargets() + m_testTargets.GetNumTargets();
    }

    template<typename ProductionTarget, typename TestTarget>
    OptionalBuildTarget<ProductionTarget, TestTarget> BuildTargetList<ProductionTarget, TestTarget>::GetBuildTarget(const AZStd::string& name) const
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

    template<typename ProductionTarget, typename TestTarget>
    BuildTarget<ProductionTarget, TestTarget> BuildTargetList<ProductionTarget, TestTarget>::GetBuildTargetOrThrow(
        const AZStd::string& name) const
    {
        auto buildTarget = GetBuildTarget(name);
        AZ_TestImpact_Eval(buildTarget.has_value(), TargetException, AZStd::string::format("Couldn't find target %s", name.c_str()).c_str());
        return buildTarget.value();
    }

    template<typename ProductionTarget, typename TestTarget>
    const TargetList<TestTarget>& BuildTargetList<ProductionTarget, TestTarget>::GetTestTargetList() const
    {
        return m_testTargets;
    }

    template<typename ProductionTarget, typename TestTarget>
    const TargetList<ProductionTarget>& BuildTargetList<ProductionTarget, TestTarget>::GetProductionTargetList() const
    {
        return m_productionTargets;
    }

    template<typename ProductionTarget, typename TestTarget>
    AZStd::string BuildTargetList<ProductionTarget, TestTarget>::GetTargetNameFromOutputName(const AZStd::string& outputName) const
    {
        const auto it = m_outputNameToTargetNameMapping.find(outputName);
        if (it != m_outputNameToTargetNameMapping.end())
        {
            return it->second;
        }

        return "";
    }

    template<typename ProductionTarget, typename TestTarget>
    AZStd::string BuildTargetList<ProductionTarget, TestTarget>::GetTargetNameFromOutputNameOrThrow(const AZStd::string& outputName) const
    {
        const auto targetName = GetTargetNameFromOutputName(outputName);
        AZ_TestImpact_Eval(
            !targetName.empty(),
            TargetException,
            AZStd::string::format("Couldn't find target with output name %s", outputName.c_str()).c_str());
        return targetName;
    }
} // namespace TestImpact
