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
    //! Artifact produced by the build system for each build target. Contains source and output information about said targets.
    struct BuildTargetDescriptor
        : public TargetDescriptor
    {
        BuildTargetDescriptor() = default;
        BuildTargetDescriptor(TargetDescriptor&& targetDescriptor, const AZStd::string& outputName);

        AZStd::string m_outputName; //!< Output name (sans extension) of build target binary.
    };
} // namespace TestImpact
