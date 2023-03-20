/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Static/TestImpactTargetDescriptor.h>

namespace TestImpact
{
    //! Representation of a generic build target in the repository.
    class Target
    {
    public:
        Target(TargetDescriptor&& descriptor);
        virtual ~Target() = default;

        //! Returns the build target name.
        [[nodiscard]] const AZStd::string& GetName() const;

        //! Returns the target type at runtime.
        [[nodiscard]] TargetType GetType() const;

        //! Returns the path in the source tree to the build target location.
        [[nodiscard]] const RepoPath& GetPath() const;

        //! Returns the build target's dependencies.
        [[nodiscard]] const TargetDependencies& GetDependencies() const;

        //! Returns the build target's sources.
        [[nodiscard]] const TargetSources& GetSources() const;

        //! Returns the build target's compiled binary name.
        [[nodiscard]] const AZStd::string& GetOutputName() const;

    private:
       TargetDescriptor m_descriptor;
    };
} // namespace TestImpact
