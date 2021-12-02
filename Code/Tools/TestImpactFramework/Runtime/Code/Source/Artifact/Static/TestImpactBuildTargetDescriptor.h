/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactRepoPath.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/string.h>

namespace TestImpact
{
    //! Pairing between a given autogen input source and the generated output source(s).
    struct AutogenPairs
    {
        RepoPath m_input;
        AZStd::vector<RepoPath> m_outputs;
    };

    using AutogenSources = AZStd::vector<AutogenPairs>;

    //! Representation of a given built target's source list.
    struct TargetSources
    {
        AZStd::vector<RepoPath> m_staticSources; //!< Source files used to build this target (if any).
        AutogenSources m_autogenSources; //!< Autogen source files (if any).
    };

    //! Representation of a given build target's basic build infotmation.
    struct BuildMetaData
    {
        AZStd::string m_name; //!< Build target name.
        AZStd::string m_outputName; //!< Output name (sans extension) of build target binary.
        RepoPath m_path; //!< Path to build target location in source tree (relative to repository root).
    };

    //! Artifact produced by the build system for each build target. Contains source and output information about said targets.
    struct BuildTargetDescriptor
    {
        BuildTargetDescriptor() = default;
        BuildTargetDescriptor(BuildMetaData&& buildMetaData, TargetSources&& sources);

        BuildMetaData m_buildMetaData;
        TargetSources m_sources;
    };
} // namespace TestImpact
