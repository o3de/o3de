/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Static/TestImpactBuildTargetDescriptor.h>
#include <Target/TestImpactTarget.h>

#include <AzCore/std/containers/variant.h>

namespace TestImpact
{
    class TestTarget;
    class ProductionTarget;

    //! Holder for specializations of BuildTarget.
    using SpecializedBuildTarget = AZStd::variant<const TestTarget*, const ProductionTarget*>;

    //! Optional holder for specializations of BuildTarget.
    using OptionalSpecializedBuildTarget = AZStd::variant<AZStd::monostate, const TestTarget*, const ProductionTarget*>;

    //! Type id for querying specialized derived target types from base pointer/reference.
    enum class SpecializedBuildTargetType : bool
    {
        Production, //!< Production build target.
        Test //!< Test build target.
    };

    //! Representation of a generic build target in the repository.
    class BuildTarget
        : public Target
    {
    public:
        BuildTarget(BuildTargetDescriptor* descriptor, SpecializedBuildTargetType type);

        //! Returns the build target's compiled binary name.
        const AZStd::string& GetOutputName() const;

        //! Returns the build target type.
        SpecializedBuildTargetType GetSpecializedBuildTargetType() const;

    private:
        const BuildTargetDescriptor* m_descriptor;
        SpecializedBuildTargetType m_type;
    };
} // namespace TestImpact
