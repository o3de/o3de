/*
 * Copyright (c) Contributors to the Open 3D Engine Project
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
        Vector3 XYZ_To_AcesCg(Vector3 color)
        {
            return XYZToACEScgMatrix * color;
        }
    } // namespace Render
} // namespace AZ
