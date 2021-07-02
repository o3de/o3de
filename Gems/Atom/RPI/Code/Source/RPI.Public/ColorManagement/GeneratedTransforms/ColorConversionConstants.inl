/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Matrix3x3.h>

namespace AZ
{
    namespace RPI
    {
        ////////////////////////////////////////////////////////////////////////////////
        // Color space conversion matrices
        // These color transform matrices are using same value as the ACES implementation below.
        // Atom/Feature/Common/Assets/ShaderLib/PostProcessing/AcesColorSpaceConversion.azsli

        // XYZ-to-ACEScg conversion matrix
        static const AZ::Matrix3x3 XYZToACEScgMatrix =
            Matrix3x3::CreateFromRows(
                Vector3(1.6410233797f, -0.3248032942f, -0.2364246952f),
                Vector3(-0.6636628587f, 1.6153315917f, 0.0167563477),
                Vector3(0.0117218943f, -0.0082844420f, 0.9883948585));

        // ACEScg-to-XYZ conversion matrix
        static const AZ::Matrix3x3 ACEScgToXYZMatrix =
            Matrix3x3::CreateFromRows(
                Vector3(0.66245413f, 0.13400421f, 0.15618768f),
                Vector3(0.27222872f, 0.67408168f, 0.05368952f),
                Vector3(-0.00557466f, 0.00406073f, 1.01033902f));

        // sRGB-to-XYZ conversion matrix
        static const AZ::Matrix3x3 SRGBToXYZMatrix =
            Matrix3x3::CreateFromRows(
                Vector3(0.41239089f, 0.35758430f, 0.18048084f),
                Vector3(0.21263906f, 0.71516860f, 0.07219233f),
                Vector3(0.01933082f, 0.11919472f, 0.95053232f));

        // XYZ-to-sRGB conversion matrix
        static const AZ::Matrix3x3 XYZToSRGBMatrix =
            Matrix3x3::CreateFromRows(
                Vector3(3.24096942f, -1.53738296f, -0.49861076f),
                Vector3(-0.96924388f, 1.87596786f, 0.04155510f),
                Vector3(0.05563002f, -0.20397684f, 1.05697131f));

        // Bradford Chromatic Adaptation Transform Matrix from D65 to D60
        static const AZ::Matrix3x3 D65ToD60Matrix =
            Matrix3x3::CreateFromRows(
                Vector3(1.01303f, 0.00610531f, -0.014971f),
                Vector3(0.00769823f, 0.998165f, -0.00503203f),
                Vector3(-0.00284131f, 0.00468516f, 0.924507f));

        // Bradford Chromatic Adaptation Transform Matrix from D60 to D65
        static const AZ::Matrix3x3 D60ToD65Matrix =
            Matrix3x3::CreateFromRows(
                Vector3(0.987224f, -0.00611327f, 0.0159533f),
                Vector3(-0.00759836f, 1.00186f, 0.00533002f),
                Vector3(0.00307257f, -0.00509595f, 1.08168f));

    } // namespace Render
} // namespace AZ
