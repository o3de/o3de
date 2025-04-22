/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Spline.h>
#include <LmbrCentral/Shape/SplineAttribute.h>

namespace LmbrCentral
{
    /// Type ID for TubeShapeComponent
    inline constexpr AZ::TypeId TubeShapeComponentTypeId{ "{9C39E3A4-EEE8-4047-ADE2-376A1BFCB3D1}" };

    /// Type ID for EditorTubeShapeComponent
    inline constexpr AZ::TypeId EditorTubeShapeComponentTypeId{ "{F969BE9D-08E3-4E6B-B16D-E73E1F3C740A}" };

    /// Services provided by the TubeShapeComponent
    class TubeShapeComponentRequests
        : public AZ::ComponentBus
    {
    public:
        /// @brief Sets the radius for the Tube.
        /// @param radius of the Tube.
        virtual void SetRadius(float radius) = 0;

        /// @brief Gets the radius of the Tube.
        /// @return the radius of the Tube.
        virtual float GetRadius() const = 0;

        /// @brief Sets the variable radius along the tube.
        /// @param index The index of a spline point.
        /// @param radius The variable radius to set for the spline point.
        virtual void SetVariableRadius(int index, float radius) = 0;
        
        /// @brief Sets the variable radius of all vertices along the tube.
        /// @param radius The variable radius to set for the spline point.
        virtual void SetAllVariableRadii(float radius) = 0;

        /// @brief Gets the variable radius of the tube for a given spline point.
        /// @param index An index of a point into the attached spline.
        virtual float GetVariableRadius(int index) const = 0;

        /// @brief Gets the final interpolated radius of the tube for a given spline address.
        /// @param address The address along the spline.
        virtual float GetTotalRadius(const AZ::SplineAddress& address) const = 0;

        /// @brief Gets the radius spline attribute
        virtual const SplineAttribute<float>& GetRadiusAttribute() const = 0;
    };

    /// Bus to service the TubeShapeComponent event group
    using TubeShapeComponentRequestsBus = AZ::EBus<TubeShapeComponentRequests>;
} // namespace LmbrCentral
