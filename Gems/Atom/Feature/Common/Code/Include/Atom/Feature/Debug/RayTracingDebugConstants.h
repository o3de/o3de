/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>

namespace AZ::Render
{
    // The view modes of the ray tracing debug view
    // This enum must be the same as in RayTracingDebugCommon.azsli
    enum class RayTracingDebugViewMode : u32
    {
        InstanceIndex,
        InstanceID,
        PrimitiveIndex,
        Barycentrics,
        Normals,
        UVs,
    };
} // namespace AZ::Render
