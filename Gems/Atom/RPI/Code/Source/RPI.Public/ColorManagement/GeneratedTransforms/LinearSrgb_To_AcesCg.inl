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
