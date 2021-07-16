/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxTextureUtil.h"

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Math/Plane.h>

namespace WhiteBox
{
    int FindLargestElement(const AZ::Vector3& vector)
    {
        AZ::Vector3 absVector = vector.GetAbs();

        // if any components are equal, for convention, favor components in increasing order
        if (absVector.GetElement(0) >= absVector.GetElement(1) && absVector.GetElement(0) >= absVector.GetElement(2))
        {
            return 0;
        }
        else if (
            absVector.GetElement(1) >= absVector.GetElement(0) && absVector.GetElement(1) >= absVector.GetElement(2))
        {
            return 1;
        }
        else
        {
            return 2;
        }
    }

    AZ::Vector2 CreatePlanarUVFromVertex(
        const AZ::Vector3& normal, const AZ::Vector3& position, const AZ::Vector2& offset, const AZ::Vector2& scale)
    {
        // truncates a float to 3 decimal places
        // note: there are the Round and Round3 methods in ManipulatorSnapping.h (which may very well be the
        // source of the noise) but for this specific issue, we want to discard the information after 3 decimal
        // places to ensure consistent behaviour (rounding could cause flipping between planes when the noise is
        // around the rounding threshold)
        auto truncateComponent = [](const float f)
        {
            const float factor3Places = 1000.0f;
            AZ::s32 i = azlossy_cast<AZ::s32, float>(f * factor3Places);
            return i / factor3Places;
        };

        // noise from grid snapping can manifest in the normal even if the positioning itself does not change so
        // truncate the normal to 3 decimal places in order to ensure that the favoured component order for equal
        // component values is consistent
        const AZ::Vector3 truncatedNormal = AZ::Vector3(
            truncateComponent(normal.GetX()), truncateComponent(normal.GetY()), truncateComponent(normal.GetZ()));

        const int largestElem = FindLargestElement(truncatedNormal);

        // swizzled vertex positions for each of the basis vector planes
        const AZ::Vector2 uvX(position.GetZ(), position.GetY());
        const AZ::Vector2 uvY(position.GetX(), position.GetZ());
        const AZ::Vector2 uvZ(position.GetX(), position.GetY());

        AZ::Vector2 uv;
        if (largestElem == 0)
        {
            uv = uvX;
        }
        else if (largestElem == 1)
        {
            uv = uvY;
        }
        else
        {
            uv = uvZ;
        }

        return uv * scale + offset;
    }

} // namespace WhiteBox
