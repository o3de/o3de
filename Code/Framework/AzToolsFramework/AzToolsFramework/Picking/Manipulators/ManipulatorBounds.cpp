/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Math/Spline.h>
#include <AzCore/std/limits.h>
#include <AzToolsFramework/Picking/ContextBoundAPI.h>
#include <AzToolsFramework/Picking/Manipulators/ManipulatorBounds.h>

namespace AzToolsFramework
{
    namespace Picking
    {
        // this intersection algorithm is adapted from 'Capped Cone' by Inigo Quilez
        // ref: https://www.iquilezles.org/www/articles/intersectors/intersectors.htm and https://www.shadertoy.com/view/llcfRf
        // all algorithms/code snippets are kindly made available under the MIT License - https://www.iquilezles.org/www/index.htm
        bool IntersectRayCone(
            const AZ::Vector3& rayOrigin,
            const AZ::Vector3& rayDirection,
            const AZ::Vector3& coneApex,
            const AZ::Vector3& coneAxis,
            float coneHeight,
            float coneBaseRadius,
            float& t)
        {
            const AZ::Vector3& pa = coneApex;
            const AZ::Vector3& pb = coneApex + coneHeight * coneAxis;
            const AZ::Vector3& ro = rayOrigin;
            const AZ::Vector3& rd = rayDirection;
            float rb = coneBaseRadius;

            AZ::Vector3 ba = pb - pa;
            AZ::Vector3 oa = ro - pa;
            AZ::Vector3 ob = ro - pb;

            float m0 = ba.Dot(ba);
            float m1 = oa.Dot(ba);
            float m2 = ob.Dot(ba);
            float m3 = rd.Dot(ba);

            auto dot2 = [](const AZ::Vector3& v)
            {
                return v.Dot(v);
            };

            // cap
            if (m2 > 0.0f)
            {
                if (dot2(ob * m3 - rd * m2) < (rb * rb * m3 * m3))
                {
                    t = -m2 / m3;
                    return true;
                }
            }

            // body
            float m4 = rd.Dot(oa);
            float m5 = oa.Dot(oa);
            float hy = m0 + rb * rb;

            float k2 = m0 * m0 - m3 * m3 * hy;
            float k1 = m0 * m0 * m4 - m1 * m3 * hy;
            float k0 = m0 * m0 * m5 - m1 * m1 * hy;

            // note: solving for simultaneously being on the sloping surface of the cone and being on the ray boils down
            // to a quadratic equation - the discriminant of the quadratic determines if there are 1, 2 or no solutions
            //
            // if the discriminant is less than 0 the ray is not intersecting the cone, if it is equal to 0 then the ray
            // is intersecting the cone once and if it is greater than 0 the ray is intersecting the cone twice
            float discriminant = k1 * k1 - k2 * k0;
            if (discriminant < 0.0f)
            {
                return false;
            }

            float tt = (-k1 - AZ::Sqrt(discriminant)) / k2;
            float y = m1 + tt * m3;
            if (y >= 0.0f && y < m0)
            {
                t = tt;
                return true;
            }

            return false;
        }

        bool ManipulatorBoundSphere::IntersectRay(
            const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float& rayIntersectionDistance) const
        {
            float vecRayIntersectionDistance;
            if (AZ::Intersect::IntersectRaySphere(rayOrigin, rayDirection, m_center, m_radius, vecRayIntersectionDistance) > 0)
            {
                rayIntersectionDistance = vecRayIntersectionDistance;
                return true;
            }

            return false;
        }

        void ManipulatorBoundSphere::SetShapeData(const BoundRequestShapeBase& shapeData)
        {
            if (const auto* sphereData = azrtti_cast<const BoundShapeSphere*>(&shapeData))
            {
                m_center = sphereData->m_center;
                m_radius = sphereData->m_radius;
            }
        }

        bool ManipulatorBoundBox::IntersectRay(
            const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float& rayIntersectionDistance) const
        {
            return AZ::Intersect::IntersectRayBox(
                       rayOrigin, rayDirection, m_center, m_axis1, m_axis2, m_axis3, m_halfExtents.GetX(), m_halfExtents.GetY(),
                       m_halfExtents.GetZ(), rayIntersectionDistance);
        }

        void ManipulatorBoundBox::SetShapeData(const BoundRequestShapeBase& shapeData)
        {
            if (const auto* boxData = azrtti_cast<const BoundShapeBox*>(&shapeData))
            {
                m_center = boxData->m_center;
                m_axis1 = boxData->m_orientation.TransformVector(AZ::Vector3::CreateAxisX());
                m_axis2 = boxData->m_orientation.TransformVector(AZ::Vector3::CreateAxisY());
                m_axis3 = boxData->m_orientation.TransformVector(AZ::Vector3::CreateAxisZ());
                m_halfExtents = boxData->m_halfExtents;
            }
        }

        bool ManipulatorBoundCylinder::IntersectRay(
            const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float& rayIntersectionDistance) const
        {
            float t1 = std::numeric_limits<float>::max();
            float t2 = std::numeric_limits<float>::max();
            if (AZ::Intersect::IntersectRayCappedCylinder(rayOrigin, rayDirection, m_base, m_axis, m_height, m_radius, t1, t2) > 0)
            {
                rayIntersectionDistance = AZStd::GetMin(t1, t2);
                return true;
            }

            return false;
        }

        void ManipulatorBoundCylinder::SetShapeData(const BoundRequestShapeBase& shapeData)
        {
            if (const auto* cylinderData = azrtti_cast<const BoundShapeCylinder*>(&shapeData))
            {
                m_base = cylinderData->m_base;
                m_axis = cylinderData->m_axis;
                m_height = cylinderData->m_height;
                m_radius = cylinderData->m_radius;
            }
        }

        bool ManipulatorBoundCone::IntersectRay(
            const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float& rayIntersectionDistance) const
        {
            float t = std::numeric_limits<float>::max();
            if (IntersectRayCone(rayOrigin, rayDirection, m_apexPosition, m_dir, m_height, m_radius, t))
            {
                rayIntersectionDistance = t;
                return true;
            }

            return false;
        }

        void ManipulatorBoundCone::SetShapeData(const BoundRequestShapeBase& shapeData)
        {
            if (const auto* coneData = azrtti_cast<const BoundShapeCone*>(&shapeData))
            {
                m_apexPosition = coneData->m_base + coneData->m_axis * coneData->m_height;
                m_dir = -coneData->m_axis;
                m_height = coneData->m_height;
                m_radius = coneData->m_radius;
            }
        }

        bool ManipulatorBoundQuad::IntersectRay(
            const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float& rayIntersectionDistance) const
        {
            return AZ::Intersect::IntersectRayQuad(
                       rayOrigin, rayDirection, m_corner1, m_corner2, m_corner3, m_corner4, rayIntersectionDistance) > 0;
        }

        void ManipulatorBoundQuad::SetShapeData(const BoundRequestShapeBase& shapeData)
        {
            if (const auto* quadData = azrtti_cast<const BoundShapeQuad*>(&shapeData))
            {
                m_corner1 = quadData->m_corner1;
                m_corner2 = quadData->m_corner2;
                m_corner3 = quadData->m_corner3;
                m_corner4 = quadData->m_corner4;
            }
        }

        bool ManipulatorBoundTorus::IntersectRay(
            const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float& rayIntersectionDistance) const
        {
            return IntersectHollowCylinder(
                rayOrigin, rayDirection, m_center, m_axis, m_minorRadius, m_majorRadius, rayIntersectionDistance);
        }

        void ManipulatorBoundTorus::SetShapeData(const BoundRequestShapeBase& shapeData)
        {
            if (const auto* torusData = azrtti_cast<const BoundShapeTorus*>(&shapeData))
            {
                m_center = torusData->m_center;
                m_axis = torusData->m_axis;
                m_minorRadius = torusData->m_minorRadius;
                m_majorRadius = torusData->m_majorRadius;
            }
        }

        bool ManipulatorBoundLineSegment::IntersectRay(
            const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float& rayIntersectionDistance) const
        {
            const float rayLength = 1000.0f;
            AZ::Vector3 closestPosRay, closestPosLineSegment;
            float rayProportion, lineSegmentProportion;
            // note: here out param is proportion/percentage of line
            AZ::Intersect::ClosestSegmentSegment(
                rayOrigin, rayOrigin + rayDirection * rayLength, m_worldStart, m_worldEnd, rayProportion, lineSegmentProportion,
                closestPosRay, closestPosLineSegment);

            float distanceFromLine = (closestPosRay - closestPosLineSegment).GetLength();
            if (distanceFromLine <= m_width)
            {
                // rayIntersectionDistance is expected to be distance so we must scale rayProportion by its length.
                // add distance from line to give more accurate rayIntersectionDistance value.
                rayIntersectionDistance = rayProportion * rayLength + distanceFromLine;
                return true;
            }

            return false;
        }

        void ManipulatorBoundLineSegment::SetShapeData(const BoundRequestShapeBase& shapeData)
        {
            if (const auto* lineSegmentData = azrtti_cast<const BoundShapeLineSegment*>(&shapeData))
            {
                m_worldStart = lineSegmentData->m_start;
                m_worldEnd = lineSegmentData->m_end;
                m_width = lineSegmentData->m_width;
            }
        }

        bool ManipulatorBoundSpline::IntersectRay(
            const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float& rayIntersectionDistance) const
        {
            if (const AZStd::shared_ptr<const AZ::Spline> spline = m_spline.lock())
            {
                AZ::RaySplineQueryResult splineQueryResult = AZ::IntersectSpline(m_transform, rayOrigin, rayDirection, *spline);

                if (splineQueryResult.m_distanceSq <= m_width * m_width)
                {
                    rayIntersectionDistance = splineQueryResult.m_rayDistance;
                    return true;
                }

                return false;
            }

            return false;
        }

        void ManipulatorBoundSpline::SetShapeData(const BoundRequestShapeBase& shapeData)
        {
            if (const auto* splineData = azrtti_cast<const BoundShapeSpline*>(&shapeData))
            {
                m_spline = splineData->m_spline;
                m_transform = splineData->m_transform;
                m_width = splineData->m_width;
            }
        }

        bool IntersectHollowCylinder(
            const AZ::Vector3& rayOrigin,
            const AZ::Vector3& rayDirection,
            const AZ::Vector3& center,
            const AZ::Vector3& axis,
            const float minorRadius,
            const float majorRadius,
            float& rayIntersectionDistance)
        {
            float t1 = AZStd::numeric_limits<float>::max();
            float t2 = AZStd::numeric_limits<float>::max();
            const AZ::Vector3 base = center - axis * minorRadius;

            if (AZ::Intersect::IntersectRayCappedCylinder(
                    rayOrigin, rayDirection, base, axis, minorRadius * 2.0f, majorRadius + minorRadius, t1, t2) > 0)
            {
                const float threshold = majorRadius - minorRadius;
                const float thresholdSq = threshold * threshold;
                // util lambda used for distance checks at both 't' values
                const auto validHolowCylinderHit = [&rayOrigin, &rayDirection, &center, thresholdSq](const float t)
                {
                    // only return a valid intersection if the hit was
                    // not in the 'hollow' part of the cylinder
                    const AZ::Vector3 intersection = rayOrigin + rayDirection * t;
                    const float distanceSq = (intersection - center).GetLengthSq();
                    return distanceSq > thresholdSq;
                };

                if (validHolowCylinderHit(t1))
                {
                    rayIntersectionDistance = t1;
                    return true;
                }

                if (validHolowCylinderHit(t2))
                {
                    rayIntersectionDistance = t2;
                    return true;
                }
            }

            return false;
        }
    } // namespace Picking
} // namespace AzToolsFramework
