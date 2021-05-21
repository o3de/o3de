/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
