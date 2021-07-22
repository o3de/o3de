/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Static/TestImpactBuildTargetDescriptor.h>

#include <AzCore/std/containers/variant.h>
#include <AzCore/std/string/string.h>

namespace TestImpact
{
    class TestTarget;
    class ProductionTarget;

    //! Holder for specializations of BuildTarget.
    using Target = AZStd::variant<const TestTarget*, const ProductionTarget*>;

    //! Optional holder for specializations of BuildTarget.
    using OptionalTarget = AZStd::variant<AZStd::monostate, const TestTarget*, const ProductionTarget*>;

    //! Type id for querying specialized derived target types from base pointer/reference.
    enum class TargetType : bool
    {
        Production, //!< Production build target.
        Test //!< Test build target.
    };

    //! Representation of a generic build target in the repository.
    class BuildTarget
    {
    public:
        BuildTarget(BuildTargetDescriptor&& descriptor, TargetType type);
        virtual ~BuildTarget() = default;

        //! Returns the build target name.
        const AZStd::string& GetName() const;

        //! Returns the build target's compiled binary name.
        const AZStd::string& GetOutputName() const;

        //! Returns the path in the source tree to the build target location.
        const RepoPath& GetPath() const;

        //! Returns the build target's sources.
        const TargetSources& GetSources() const;

        //! Returns the build target type.
        TargetType GetType() const;

    private:
        BuildMetaData m_buildMetaData;
        TargetSources m_sources;
        TargetType m_type;
    };
} // namespace TestImpact
