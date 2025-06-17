/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Color.h>
#include <Atom/RPI.Public/Configuration.h>

namespace AZ
{
    namespace RPI
    {
        enum class ColorSpaceId
        {
            SRGB = 0,
            LinearSRGB,
            ACEScg,
            ACES2065,
            XYZ,

            ColorSpaceIdCount
        };

        ATOM_RPI_PUBLIC_API Color TransformColor(Color color, ColorSpaceId fromColorSpace, ColorSpaceId toColorSpace);
    } // namespace Render
} // namespace AZ
