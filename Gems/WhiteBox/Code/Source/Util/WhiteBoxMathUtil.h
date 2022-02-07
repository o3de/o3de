/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ
{
    class Quaternion;
    class Transform;
    class Vector3;
}

namespace WhiteBox
{
    //! Intersect a segment with a cylinder
    //! Reference: Real-Time Collision Detection - 5.3.7 Intersecting Ray or Segment Against Cylinder
    //! @note: This exists because the version in AzCore has bugs. Should consider moving this to AzCore
    //! @param sa The start of the line segment
    //! @param sb The end of the line segment
    //! @param p The base of the cylinder
    //! @param q The top of the cylinder
    //! @param r The radius of the cylinder
    //! @param t The normalized distance along the line segment
    //! @return if an intersection occured or not
    bool IntersectSegmentCylinder(
        const AZ::Vector3& sa, const AZ::Vector3& sb, const AZ::Vector3& p, const AZ::Vector3& q, float r, float& t);

    //! Take a point in 'local' space, transform it to the new space, scale it uniformly, then return to 'local' space.
    AZ::Vector3 ScalePosition(float scale, const AZ::Vector3& localPosition, const AZ::Transform& localFromSpace);

    //! Hughes/Moeller calculate orthonormal basis.
    void CalculateOrthonormalBasis(const AZ::Vector3& n, AZ::Vector3& b1, AZ::Vector3& b2);

    //! Calculates local orientation from the orthonormal basis of the specified normal vector.
    AZ::Quaternion CalculateLocalOrientation(const AZ::Vector3& normal);

} // namespace WhiteBox
