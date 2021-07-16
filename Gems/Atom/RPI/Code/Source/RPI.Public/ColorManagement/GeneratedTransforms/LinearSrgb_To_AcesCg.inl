/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "ColorConversionConstants.inl"

namespace AZ
{
    namespace RPI
    {
        ////////////////////////////////////////////////////////////////////////////////
        // Color space conversion matrices

        Vector3 LinearSrgb_To_AcesCg(Vector3 color)
        {
            static bool cached = false;
            static AZ::Matrix3x3 cachedMatrix;
            if (!cached)
            {
                 cached = true;
                 cachedMatrix = XYZToACEScgMatrix * D65ToD60Matrix * SRGBToXYZMatrix;
            }

            return cachedMatrix * color;
        }
    } // namespace Render
} // namespace AZ
