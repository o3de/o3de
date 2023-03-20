/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/math.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>

namespace AzFramework
{
    class DebugDisplayRequests;
}

namespace AzToolsFramework
{
    //! Structure to encapsulate grid snapping properties.
    struct GridSnapParameters
    {
        GridSnapParameters(bool gridSnap, float gridSize);

        bool m_gridSnap;
        float m_gridSize;
    };

    //! Structure to hold transformed incoming viewport interaction from world space to manipulator space.
    struct ManipulatorInteraction
    {
        AZ::Vector3 m_localRayOrigin; //!< The ray origin (start) in the reference from of the manipulator.
        AZ::Vector3 m_localRayDirection; //!< The ray direction in the reference from of the manipulator.
        AZ::Vector3 m_nonUniformScaleReciprocal; //!< Handles inverting any non-uniform scale which was applied
                                                 //!< separately from the transform.
        float m_scaleReciprocal; //!< The scale reciprocal (1.0 / scale) of the transform used to move the
                                 //!< ray from world space to local space.
    };

    //! Build a ManipulatorInteraction structure from the incoming viewport interaction.
    ManipulatorInteraction BuildManipulatorInteraction(
        const AZ::Transform& worldFromLocal,
        const AZ::Vector3& nonUniformScale,
        const AZ::Vector3& worldRayOrigin,
        const AZ::Vector3& worldRayDirection);

    //! Calculate the offset along an axis to adjust a position to stay snapped to a given grid size.
    //! @note This is snap up or down to the nearest grid segment (e.g. 0.2 snaps to 0.0 -> delta 0.2,
    //! 0.7 snaps to 1.0 -> delta 0.3).
    AZ::Vector3 CalculateSnappedOffset(const AZ::Vector3& unsnappedPosition, const AZ::Vector3& axis, float size);

    //! Return the amount to snap from the starting position given the current grid size.
    //! @note A movement of more than half size (in either direction) will cause a snap by size.
    AZ::Vector3 CalculateSnappedAmount(const AZ::Vector3& unsnappedPosition, const AZ::Vector3& axis, float size);

    //! Overload of CalculateSnappedOffset taking multiple axes.
    AZ::Vector3 CalculateSnappedOffset(
        const AZ::Vector3& unsnappedPosition, const AZ::Vector3* snapAxes, size_t snapAxesCount, float size);

    //! Return the final snapped position according to size (unsnappedPosition + CalculateSnappedOffset).
    AZ::Vector3 CalculateSnappedPosition(
        const AZ::Vector3& unsnappedPosition, const AZ::Vector3* snapAxes, size_t snapAxesCount, float size);

    //! Wrapper for grid snapping and grid size bus calls.
    GridSnapParameters GridSnapSettings(int viewportId);

    //! Wrapper for angle snapping enabled bus call.
    bool AngleSnapping(int viewportId);

    //! Wrapper for angle snapping increment bus call.
    //! @return Angle in degrees.
    float AngleStep(int viewportId);

    //! Wrapper for grid rendering check call.
    bool ShowingGrid(int viewportId);

    //! Render the grid used for snapping.
    void DrawSnappingGrid(AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Transform& worldFromLocal, float squareSize);

    //! Round to x number of significant digits.
    //! @param value Number to round.
    //! @param exponent Precision to use when rounding.
    inline float Round(const float value, const float exponent)
    {
        const float precision = AZStd::pow(10.0f, exponent);
        return AZStd::round(value * precision) / precision;
    }

    //! Round to 3 significant digits (3 digits common usage).
    inline float Round3(const float value)
    {
        return Round(value, 3.0f);
    }

    //! Util to return sign of floating point number.
    //! value > 0 return 1.0
    //! value < 0 return -1.0
    //! value == 0 return 0.0
    inline float Sign(const float value)
    {
        return static_cast<float>((0.0f < value) - (value < 0.0f));
    }

    //! Find the max scale element and return the reciprocal of it.
    //! Note: The reciprocal will be rounded to three significant digits to eliminate
    //! noise in the value returned when dealing with values far from the origin.
    inline float ScaleReciprocal(const AZ::Transform& transform)
    {
        return Round3(1.0f / transform.GetUniformScale());
    }

    //! Find the reciprocal of the non-uniform scale.
    //! Each element will be rounded to three significant digits to eliminate noise
    //! when dealing with values far from the origin.
    inline AZ::Vector3 NonUniformScaleReciprocal(const AZ::Vector3& nonUniformScale)
    {
        const AZ::Vector3 scaleReciprocal = nonUniformScale.GetReciprocal();
        return AZ::Vector3(Round3(scaleReciprocal.GetX()), Round3(scaleReciprocal.GetY()), Round3(scaleReciprocal.GetZ()));
    }
} // namespace AzToolsFramework
