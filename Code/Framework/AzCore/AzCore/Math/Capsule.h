/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/LineSegment.h>

namespace AZ
{
    //! A class representing a capsule (a cylinder with a hemispherical cap at each end).
    class Capsule
    {
    public:
        AZ_TYPE_INFO(Capsule, "{4B7E9154-B258-40D2-81A4-A27A25030588}");

        Capsule() = default; // Creates an uninitialized Capsule. Will not be usable until its data is set.

        //! Constructs a Capsule from the centers of the two hemispherical caps (the order is arbitrary), and the radius.
        Capsule(const Vector3& firstHemisphereCenter, const Vector3& secondHemisphereCenter, float radius);

        //! Constructs a Capsule using the ends of the provided LineSegment as the centers of the hemispherical caps, and
        //! using the provided radius.
        Capsule(const LineSegment& lineSegment, float radius);

        //! Gets the center of the first hemispherical cap (the order of the caps is arbitrary).
        const Vector3& GetFirstHemisphereCenter() const;

        //! Gets the center of the second hemispherical cap (the order of the caps is arbitrary).
        const Vector3& GetSecondHemisphereCenter() const;

        //! Gets the overall center of the capsule.
        Vector3 GetCenter() const;

        //! Gets the radius of the capsule.
        float GetRadius() const;

        //! Gets the height of the cylindrical section of the capsule.
        float GetCylinderHeight() const;

        //! Gets the total height of the capsule, including the cylindrical section and both hemispherical caps.
        float GetTotalHeight() const;

        //! Sets the center of the first hemispherical cap (the order of the caps is arbitrary).
        void SetFirstHemisphereCenter(const Vector3& firstHemisphereCenter);

        //! Sets the center of the second hemispherical cap (the order of the caps is arbitrary).
        void SetSecondHemisphereCenter(const Vector3& secondHemisphereCenter);

        //! Sets the radius of the capsule.
        void SetRadius(float radius);

        //! Returns whether this capsule is identical to another within the tolerance (allowing either order for the caps).
        bool IsClose(const Capsule& rhs, float tolerance = Constants::Tolerance) const;

        //! Returns true if the point is inside or on the surface of the capsule.
        bool Contains(const AZ::Vector3& point) const;

    private:

        Vector3 m_firstHemisphereCenter; //!< The center of one of the hemispherical ends (order is interchangeable). 
        Vector3 m_secondHemisphereCenter; //!< The center of the other hemispherical end (order is interchangeable).
        float m_radius; //!< The radius of the cylindrical and hemispherical sections of the capsule.
    };
} // namespace AZ

#include <AzCore/Math/Capsule.inl>
