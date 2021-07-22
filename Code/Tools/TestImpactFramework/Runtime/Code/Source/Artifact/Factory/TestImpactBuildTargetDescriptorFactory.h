/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Static/TestImpactBuildTargetDescriptor.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace TestImpact
{
    //! Constructs a build target artifact from the specified build target data.
    //! @param buildTargetData The raw build target data in JSON format.
    //! @param staticSourceIncludes The list of file extensions to include for static sources.
    //! @param autogenInputExtentsionIncludes The list of file extensions to include for autogen input sources.
    //! @param autogenMatcher The regex pattern used to match autogen input filenames with output filenames.
    //! @return The constructed build target artifact.
    BuildTargetDescriptor BuildTargetDescriptorFactory(
        const AZStd::string& buildTargetData,
        const AZStd::vector<AZStd::string>& staticSourceExtentsionIncludes,
        const AZStd::vector<AZStd::string>& autogenInputExtentsionIncludes,
        const AZStd::string& autogenMatcher);
} // namespace TestImpact
