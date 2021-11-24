/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <BuildSystem/Common/TestImpactBuildSystemTraits.h>
#include <Target/Native/TestImpactNativeTestTargetList.h>
#include <Target/Native/TestImpactNativeProductionTargetList.h>

namespace TestImpact
{
    class NativeTestTargetList;
    class NativeProductionTargetList;

    struct NativeBuildSystem
        : public BuildSystem<NativeTestTargetList, NativeProductionTargetList>
    {
    };
} // namespace TestImpact
