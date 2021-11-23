/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Static/TestImpactProductionTargetDescriptor.h>
#include <Artifact/Static/TestImpactTestTargetDescriptor.h>
#include <Target/TestImpactProductionTargetList.h>
#include <Target/TestImpactTestScriptTargetList.h>
#include <Target/TestImpactTestTargetList.h>

namespace TestImpact
{
    template<typename ProductionTargetListType, typename TestTargetListType>
    class BuildSystemTargets
    {
    public:
        //! Gets the total number of production and test targets in the repository.
        size_t GetNumBuildTargets() const;

        //! Attempts to get the specified build target.
        //! @param name The name of the build target to get.
        //! @returns If found, the pointer to the specified build target, otherwise nullptr.
        const NativeTarget* GetBuildTarget(const AZStd::string& name) const;

        //! Attempts to get the specified build target or throw TargetException.
        //! @param name The name of the build target to get.
        const NativeTarget* GetBuildTargetOrThrow(const AZStd::string& name) const;

        //! Attempts to get the specified target's specialized type.
        //! @param name The name of the target to get.
        //! @returns If found, the pointer to the specialized target, otherwise AZStd::monostate.
        OptionalSpecializedNativeTarget GetSpecializedBuildTarget(const AZStd::string& name) const;

        //! Attempts to get the specified target's specialized type or throw TargetException.
        //! @param name The name of the target to get.
        SpecializedNativeTarget GetSpecializedBuildTargetOrThrow(const AZStd::string& name) const;

        //! Get the list of production targets in the repository.
        const NativeProductionTargetList& GetProductionTargetList() const;

        //! Get the list of test targets in the repository.
        const NativeTestTargetList& GetTestTargetList() const;

    private:
        //! The sorted list of unique production targets in the repository.
        NativeProductionTargetList m_productionTargets;

        //! The sorted list of unique test targets in the repository.
        NativeTestTargetList m_testTargets;
    };
} // namespace TestImpact
