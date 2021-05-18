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

#include <TestImpactPath.h>

#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/string.h>

namespace TestImpact
{
    //! Pairing between a given autogen input source and the generated output source(s).
    struct AutogenPairs
    {
        Path m_input;
        AZStd::vector<Path> m_outputs;
    };

    using AutogenSources = AZStd::vector<AutogenPairs>;

    //! Representation of a given built target's source list.
    struct TargetSources
    {
        AZStd::vector<Path> m_staticSources; //!< Source files used to build this target (if any).
        AutogenSources m_autogenSources; //!< Autogen source files (if any).
    };

    //! Representation of a given build target's basic build infotmation.
    struct BuildMetaData
    {
        AZStd::string m_name; //!< Build target name.
        AZStd::string m_outputName; //!< Output name (sans extension) of build target binary.
        AZ::IO::Path m_path; //!< Path to build target location in source tree (relative to repository root).
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
