/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <BuildTarget/Common/TestImpactBuildTargetTraits.h>
#include <Target/Native/TestImpactNativeTestTarget.h>
#include <Target/Native/TestImpactNativeProductionTarget.h>

namespace TestImpact
{
    struct NativeBuildTargetTraits
        : public BuildTargetTraits<NativeTestTarget, NativeProductionTarget>
    {
    };
} // namespace TestImpact
