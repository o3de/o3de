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
    inline Frustum::PlaneId operator++(Frustum::PlaneId& planeId)
    {
        planeId = (Frustum::PlaneId)(planeId + 1);
        return planeId;
    }

    AZ_MATH_INLINE Frustum::Frustum()
    {
#ifdef AZ_DEBUG_BUILD
        for (PlaneId i = PlaneId::Near; i < PlaneId::MAX; ++i)
        {
            m_planes[i] = Simd::Vec4::Splat(std::numeric_limits<float>::signaling_NaN());
        }
#endif
    }

    AZ_MATH_INLINE Plane Frustum::GetPlane(PlaneId planeId) const
    {
        return Plane(m_planes[planeId]);
    }

    AZ_MATH_INLINE void Frustum::SetPlane(PlaneId planeId, const Plane& plane)
    {
        using namespace Simd;
        m_serializedPlanes[planeId] = plane;

        //normalize the plane by dividing each element by the length of the normal
        const Vec4::FloatType lengthSquared = Vec4::FromVec1(Vec3::Dot(plane.GetNormal().GetSimdValue(), plane.GetNormal().GetSimdValue()));
        const Vec4::FloatType length = Vec4::Sqrt(lengthSquared);
        m_planes[planeId] = Vec4::Div(plane.GetSimdValue(), length);
    }

    AZ_MATH_INLINE IntersectResult Frustum::IntersectSphere(const Vector3& center, float radius) const
    {
        bool intersect = false;

        for (PlaneId i = PlaneId::Near; i < PlaneId::MAX; ++i)
        {
            const float distance = Simd::Vec1::SelectIndex0(Simd::Vec4::PlaneDistance(m_planes[i], center.GetSimdValue()));

            if (distance < -radius)
            {
                return IntersectResult::Exterior;
            }

            intersect |= (fabsf(distance) < radius);
        }

        return intersect ? IntersectResult::Overlaps : IntersectResult::Interior;
    }

    AZ_MATH_INLINE IntersectResult Frustum::IntersectSphere(const Sphere& sphere) const
    {
        return IntersectSphere(sphere.GetCenter(), sphere.GetRadius());
    }

    AZ_MATH_INLINE IntersectResult Frustum::IntersectAabb(const Vector3& minimum, const Vector3& maximum) const
    {
        return IntersectAabb(Aabb::CreateFromMinMax(minimum, maximum));
    }

    AZ_MATH_INLINE IntersectResult Frustum::IntersectAabb(const Aabb& aabb) const
    {
        // Count the number of planes where the AABB is inside
        uint32_t numInterior = 0;

        for (PlaneId i = PlaneId::Near; i < PlaneId::MAX; ++i)
        {
            const Vector3 disjointSupport = aabb.GetSupport(-Vector3(Simd::Vec4::ToVec3(m_planes[i])));
            const float   disjointDistance = Simd::Vec1::SelectIndex0(Simd::Vec4::PlaneDistance(m_planes[i], disjointSupport.GetSimdValue()));

            if (disjointDistance < 0.0f)
            {
                return IntersectResult::Exterior;
            }

            // We now know the interior point we just checked passes the plane check..
            // Check an exterior support point to determine whether or not the whole AABB is contained or if this is an intersection
            const Vector3 intersectSupport = aabb.GetSupport(Vector3(Simd::Vec4::ToVec3(m_planes[i])));
            const float   intersectDistance = Simd::Vec1::SelectIndex0(Simd::Vec4::PlaneDistance(m_planes[i], intersectSupport.GetSimdValue()));

            if (intersectDistance >= 0.0f)
            {
                // If the whole AABB passes the plane check, increment the number of planes the AABB is 'interior' to
                ++numInterior;
            }
        }

        // If the AABB is interior to all planes, we're contained, else we intersect
        return (numInterior < PlaneId::MAX) ? IntersectResult::Overlaps : IntersectResult::Interior;
    }

    AZ_MATH_INLINE bool Frustum::IsClose(const Frustum& rhs, float tolerance) const
    {
        return Vector4(m_planes[PlaneId::Near  ]).IsClose(Vector4(rhs.m_planes[PlaneId::Near  ]), tolerance)
            && Vector4(m_planes[PlaneId::Far   ]).IsClose(Vector4(rhs.m_planes[PlaneId::Far   ]), tolerance)
            && Vector4(m_planes[PlaneId::Left  ]).IsClose(Vector4(rhs.m_planes[PlaneId::Left  ]), tolerance)
            && Vector4(m_planes[PlaneId::Right ]).IsClose(Vector4(rhs.m_planes[PlaneId::Right ]), tolerance)
            && Vector4(m_planes[PlaneId::Top   ]).IsClose(Vector4(rhs.m_planes[PlaneId::Top   ]), tolerance)
            && Vector4(m_planes[PlaneId::Bottom]).IsClose(Vector4(rhs.m_planes[PlaneId::Bottom]), tolerance);
    }

} // namespace AZ
