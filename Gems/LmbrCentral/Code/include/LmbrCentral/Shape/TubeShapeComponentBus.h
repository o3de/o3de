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

#include <AzCore/Math/Spline.h>
#include <LmbrCentral/Shape/SplineAttribute.h>

namespace LmbrCentral
{
    /// Type ID for TubeShapeComponent
    static const AZ::Uuid TubeShapeComponentTypeId = "{9C39E3A4-EEE8-4047-ADE2-376A1BFCB3D1}";

    /// Type ID for EditorTubeShapeComponent
    static const AZ::Uuid EditorTubeShapeComponentTypeId = "{F969BE9D-08E3-4E6B-B16D-E73E1F3C740A}";

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