/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/ShapeIntersection.h>

namespace AZ::ShapeIntersection
{
    bool Overlaps(const Capsule& capsule, const Obb& obb)
    {
        // if the capsule is sufficiently close to being a sphere, just test a sphere against the OBB
        if (capsule.GetCylinderHeight() < AZ::Constants::FloatEpsilon * capsule.GetRadius())
        {
            return Overlaps(Sphere(capsule.GetCenter(), capsule.GetRadius()), obb);
        }

        // transform capsule points into a space where the obb is centred at the origin with identity rotation
        const Vector3 capsulePoint1 =
            obb.GetRotation().GetInverseFast().TransformVector(capsule.GetFirstHemisphereCenter() - obb.GetPosition());
        const Vector3 capsulePoint2 =
            obb.GetRotation().GetInverseFast().TransformVector(capsule.GetSecondHemisphereCenter() - obb.GetPosition());
        const float radius = capsule.GetRadius();
        const Vector3& halfLengths = obb.GetHalfLengths();

        // early out if the distance from either of the capsule hemisphere centers is less than radius
        for (const Vector3& capsulePoint : { capsulePoint1, capsulePoint2 })
        {
            const Vector3 closest = capsulePoint.GetClamp(-halfLengths, halfLengths);
            if (capsulePoint.GetDistanceSq(closest) < radius * radius)
            {
                return true;
            }
        }

        // use separating axis theorem
        // up to 16 axes need to be tested:
        // - the 3 face normals of the box (x, y, and z)
        // - the 3 cross products of those directions with the capsule axis
        // - the 2 directions from the two capsule hemispheres to their closest points on the box
        // - the 8 directions from the capsule axis to the box vertices, orthogonal to the capsule axis
        // if the projections of the two shapes onto any of those axes do not overlap then the shapes do not overlap

        // test the x, y and z axes
        const Vector3 capsulePointMinValues = capsulePoint1.GetMin(capsulePoint2) - Vector3(radius);
        const Vector3 capsulePointMaxValues = capsulePoint1.GetMax(capsulePoint2) + Vector3(radius);
        const bool overlapsXYZ = capsulePointMaxValues.IsGreaterEqualThan(-halfLengths) &&
            capsulePointMinValues.IsLessEqualThan(halfLengths);
        if (!overlapsXYZ)
        {
            return false;
        }

        // test the axes formed by the cross product of the capsule axis with x, y and z
        const Vector3 capsuleAxis = (capsulePoint2 - capsulePoint1).GetNormalized();
        auto overlapsAxis = [&capsulePoint1, &capsulePoint2, &radius, &halfLengths](const Vector3& axis)
        {
            const float capsulePoint1Projected = capsulePoint1.Dot(axis);
            const float capsulePoint2Projected = capsulePoint2.Dot(axis);
            const float capsuleProjectedMin = AZ::GetMin(capsulePoint1Projected, capsulePoint2Projected) - radius;
            const float capsuleProjectedMax = AZ::GetMax(capsulePoint1Projected, capsulePoint2Projected) + radius;
            const float obbProjectedHalfExtent = halfLengths.Dot(axis.GetAbs());
            return capsuleProjectedMax > -obbProjectedHalfExtent && capsuleProjectedMin < obbProjectedHalfExtent;
        };

        for (const Vector3& testAxis : { capsuleAxis.CrossXAxis(), capsuleAxis.CrossYAxis(), capsuleAxis.CrossZAxis() })
        {
            if (!overlapsAxis(testAxis))
            {
                return false;
            }
        }

        // test the directions from the two capsule hemispheres to their closest points on the box
        for (const Vector3& point : { capsulePoint1, capsulePoint2 })
        {
            const Vector3 closestPointInObb = point.GetClamp(-halfLengths, halfLengths);
            const Vector3 testAxis = (point - closestPointInObb).GetNormalized();
            if (!overlapsAxis(testAxis))
            {
                return false;
            }
        }

        // test the 8 directions from the capsule axis to the box vertices, orthogonal to the capsule axis
        for (int vertexIndex = 0; vertexIndex < 8; vertexIndex++)
        {
            const Vector3 vertex(
                (vertexIndex & 4) ? obb.GetHalfLengthX() : -obb.GetHalfLengthX(),
                (vertexIndex & 2) ? obb.GetHalfLengthY() : -obb.GetHalfLengthY(),
                (vertexIndex & 1) ? obb.GetHalfLengthZ() : -obb.GetHalfLengthZ());
            // get a vector from the capsule axis to the vertex
            const Vector3 vertexRelative = vertex - capsulePoint1;
            // subtract the part parallel to the axis to get an axis orthogonal to the axis
            const Vector3 testAxis = (vertexRelative - vertexRelative.Dot(capsuleAxis) * capsuleAxis).GetNormalized();

            if (!overlapsAxis(testAxis))
            {
                return false;
            }
        }

        // none of the tested axes were separating axes, the shapes must overlap
        return true;
    }
} // namespace AZ::ShapeIntersection
