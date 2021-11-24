/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Static/TestImpactNativeTargetDescriptor.h>
#include <BuildSystem/Common/TestImpactBuildTarget.h>
#include <Target/Common/TestImpactTarget.h>

namespace TestImpact
{
    class NativeTestTarget;
    class NativeProductionTarget;

    //! Holder for specializations of NativeTarget.
    using SpecializedNativeTarget = BuildTarget<NativeTestTarget, NativeProductionTarget>; // TODO: KILL 00000000000000000000000000000000000000000000000000000000000000000000

    //! Optional holder for specializations of NativeTarget.
    using OptionalSpecializedNativeTarget = OptionalBuildTarget<NativeTestTarget, NativeProductionTarget>; // TODO: KILL 00000000000000000000000000000000000000000000000000000000000000000000

    //! Type id for querying specialized derived target types from base pointer/reference.
    enum class SpecializedNativeTargetType : bool
    {
        Production, //!< Production build target.
        Test //!< Test build target.
    };

    //! Representation of a generic build target in the repository.
    class NativeTarget
        : public Target
    {
    public:
        NativeTarget(NativeTargetDescriptor* descriptor, SpecializedNativeTargetType type);

        //! Returns the build target's compiled binary name.
        const AZStd::string& GetOutputName() const;

        //! Returns the build target type.
        SpecializedNativeTargetType GetSpecializedBuildTargetType() const;

    private:
        const NativeTargetDescriptor* m_descriptor;
        SpecializedNativeTargetType m_type;
    };
} // namespace TestImpact
