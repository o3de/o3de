/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/ColorManagement/TransformColor.h>
#include "./GeneratedTransforms/LinearSrgb_To_AcesCg.inl"
#include "./GeneratedTransforms/AcesCg_To_LinearSrgb.inl"
#include "./GeneratedTransforms/XYZ_To_AcesCg.inl"

namespace AZ
{
    namespace RPI
    {
        Color TransformColor(Color color, ColorSpaceId fromColorSpace, ColorSpaceId toColorSpace)
        {
            if (fromColorSpace == toColorSpace)
            {
                return color;
            }

            if (fromColorSpace == ColorSpaceId::LinearSRGB && toColorSpace == ColorSpaceId::ACEScg)
            {
                return Color::CreateFromVector3AndFloat(LinearSrgb_To_AcesCg(color.GetAsVector3()), color.GetA());
            }
            else if (fromColorSpace == ColorSpaceId::ACEScg && toColorSpace == ColorSpaceId::LinearSRGB)
            {
                return Color::CreateFromVector3AndFloat(AcesCg_To_LinearSrgb(color.GetAsVector3()), color.GetA());
            }
            else if (fromColorSpace == ColorSpaceId::XYZ && toColorSpace == ColorSpaceId::ACEScg)
            {
                return Color::CreateFromVector3AndFloat(XYZ_To_AcesCg(color.GetAsVector3()), color.GetA());
            }

            AZ_Error("TransformColor", false, "Unsupported color transfromation(%d -> %d).", fromColorSpace, toColorSpace);

            return Color::CreateFromVector3AndFloat(Vector3(1.0f, 0.0, 1.0f), color.GetA());
        }
    } // namespace Render
} // namespace AZ
