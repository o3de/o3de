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
