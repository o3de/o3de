/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ
{
    AZ_MATH_INLINE Aabb Aabb::CreateNull()
    {
        Aabb result;
        result.m_min = Vector3(Constants::FloatMax);
        result.m_max = Vector3(-Constants::FloatMax);
        return result;
    }


    AZ_MATH_INLINE Aabb Aabb::CreateFromPoint(const Vector3& p)
    {
        Aabb aabb;
        aabb.m_min = p;
        aabb.m_max = p;
        return aabb;
    }


    AZ_MATH_INLINE Aabb Aabb::CreateFromMinMax(const Vector3& min, const Vector3& max)
    {
        Aabb aabb;
        aabb.m_min = min;
        aabb.m_max = max;
        AZ_MATH_ASSERT(aabb.IsValid(), "Min must be less than Max");
        return aabb;
    }

    AZ_MATH_INLINE Aabb Aabb::CreateFromMinMaxValues(float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
    {
        Aabb aabb;
        aabb.m_min.Set(minX, minY, minZ);
        aabb.m_max.Set(maxX, maxY, maxZ);
        return aabb;
    }

    AZ_MATH_INLINE Aabb Aabb::CreateCenterHalfExtents(const Vector3& center, const Vector3& halfExtents)
    {
        Aabb aabb;
        aabb.m_min = center - halfExtents;
        aabb.m_max = center + halfExtents;
        return aabb;
    }


    AZ_MATH_INLINE Aabb Aabb::CreateCenterRadius(const Vector3& center, float radius)
    {
        return CreateCenterHalfExtents(center, Vector3(radius));
    }


    AZ_MATH_INLINE Aabb Aabb::CreatePoints(const Vector3* pts, int numPts)
    {
        Aabb aabb = Aabb::CreateFromPoint(pts[0]);
        for (int i = 1; i < numPts; ++i)
        {
            aabb.AddPoint(pts[i]);
        }
        return aabb;
    }


    AZ_MATH_INLINE const Vector3& Aabb::GetMin() const
    {
        return m_min;
    }


    AZ_MATH_INLINE const Vector3& Aabb::GetMax() const
    {
        return m_max;
    }


    AZ_MATH_INLINE void Aabb::Set(const Vector3& min, const Vector3& max)
    {
        m_min = min;
        m_max = max;
        AZ_MATH_ASSERT(IsValid(), "Min must be less than Max");
    }


    AZ_MATH_INLINE void Aabb::SetMin(const Vector3& min)
    {
        m_min = min;
        AZ_MATH_ASSERT(IsValid(), "Min must be less than Max");
    }


    AZ_MATH_INLINE void Aabb::SetMax(const Vector3& max)
    {
        m_max = max;
        AZ_MATH_ASSERT(IsValid(), "Min must be less than Max");
    }


    AZ_FORCE_INLINE bool Aabb::operator==(const Aabb& aabb) const
    {
        return (GetMin() == aabb.GetMin()) && (GetMax() == aabb.GetMax());
    }


    AZ_FORCE_INLINE bool Aabb::operator!=(const Aabb& aabb) const
    {
        return !(*this == aabb);
    }


    AZ_MATH_INLINE float Aabb::GetXExtent() const
    {
        return m_max.GetX() - m_min.GetX();
    }


    AZ_MATH_INLINE float Aabb::GetYExtent() const
    {
        return m_max.GetY() - m_min.GetY();
    }


    AZ_MATH_INLINE float Aabb::GetZExtent() const
    {
        return m_max.GetZ() - m_min.GetZ();
    }


    AZ_MATH_INLINE Vector3 Aabb::GetExtents() const
    {
        return m_max - m_min;
    }


    AZ_MATH_INLINE Vector3 Aabb::GetCenter() const
    {
        return 0.5f * (m_min + m_max);
    }


    AZ_MATH_INLINE Vector3 Aabb::GetSupport(const Vector3& normal) const
    {
        // We want the point on the AABB 'deepest' into the plane normal
        // Put another way, the point on the AABB that would maximize the dot product with -normal
        const Simd::Vec3::FloatType selectMaxMask = Simd::Vec3::CmpLt(normal.GetSimdValue(), Simd::Vec3::ZeroFloat());
        return Vector3(Simd::Vec3::Select(m_max.GetSimdValue(), m_min.GetSimdValue(), selectMaxMask));
    }


    AZ_MATH_INLINE void Aabb::GetAsSphere(Vector3& center, float& radius) const
    {
        center = GetCenter();
        radius = (m_max - center).GetLength();
    }


    AZ_MATH_INLINE bool Aabb::Contains(const Vector3& v) const
    {
        return v.IsGreaterEqualThan(m_min) && v.IsLessEqualThan(m_max);
    }


    AZ_MATH_INLINE bool Aabb::Contains(const Aabb& aabb) const
    {
        return aabb.GetMin().IsGreaterEqualThan(m_min) && aabb.GetMax().IsLessEqualThan(m_max);
    }


    AZ_MATH_INLINE bool Aabb::Overlaps(const Aabb& aabb) const
    {
        return m_min.IsLessEqualThan(aabb.m_max) && m_max.IsGreaterEqualThan(aabb.m_min);
    }


    AZ_MATH_INLINE bool Aabb::Disjoint(const Aabb& aabb) const
    {
        return !(m_max.IsGreaterEqualThan(aabb.m_min) && m_min.IsLessEqualThan(aabb.m_max));
    }


    AZ_MATH_INLINE void Aabb::Expand(const Vector3& delta)
    {
        AZ_MATH_ASSERT(delta.IsGreaterEqualThan(Vector3::CreateZero()), "delta must be positive");
        m_min -= delta;
        m_max += delta;
    }


    AZ_MATH_INLINE const Aabb Aabb::GetExpanded(const Vector3& delta) const
    {
        AZ_MATH_ASSERT(delta.IsGreaterEqualThan(Vector3::CreateZero()), "delta must be positive");
        return Aabb::CreateFromMinMax(m_min - delta, m_max + delta);
    }


    AZ_MATH_INLINE void Aabb::AddPoint(const Vector3& p)
    {
        m_min = m_min.GetMin(p);
        m_max = m_max.GetMax(p);
    }


    AZ_MATH_INLINE void Aabb::AddAabb(const Aabb& box)
    {
        m_min = m_min.GetMin(box.GetMin());
        m_max = m_max.GetMax(box.GetMax());
    }


    AZ_MATH_INLINE float Aabb::GetDistance(const Vector3& p) const
    {
        Vector3 closest = p.GetClamp(m_min, m_max);
        return p.GetDistance(closest);
    }


    AZ_MATH_INLINE float Aabb::GetDistanceSq(const Vector3& p) const
    {
        Vector3 closest = p.GetClamp(m_min, m_max);
        return p.GetDistanceSq(closest);
    }


    AZ_MATH_INLINE float Aabb::GetMaxDistance(const Vector3& p) const
    {
        Vector3 farthest(Vector3::CreateSelectCmpGreaterEqual(p, GetCenter(), m_min, m_max));
        return p.GetDistance(farthest);
    }


    AZ_MATH_INLINE float Aabb::GetMaxDistanceSq(const Vector3& p) const
    {
        Vector3 farthest(Vector3::CreateSelectCmpGreaterEqual(p, GetCenter(), m_min, m_max));
        return p.GetDistanceSq(farthest);
    }


    AZ_MATH_INLINE Aabb Aabb::GetClamped(const Aabb& clamp) const
    {
        Aabb clampedAabb = Aabb::CreateFromMinMax(m_min, m_max);
        clampedAabb.Clamp(clamp);
        return clampedAabb;
    }


    AZ_MATH_INLINE void Aabb::Clamp(const Aabb& clamp)
    {
        m_min = m_min.GetClamp(clamp.m_min, clamp.m_max);
        m_max = m_max.GetClamp(clamp.m_min, clamp.m_max);
    }


    AZ_MATH_INLINE void Aabb::SetNull()
    {
        m_min = Vector3(Constants::FloatMax);
        m_max = Vector3(-Constants::FloatMax);
    }


    AZ_MATH_INLINE void Aabb::Translate(const Vector3& offset)
    {
        m_min += offset;
        m_max += offset;
    }


    AZ_MATH_INLINE Aabb Aabb::GetTranslated(const Vector3& offset) const
    {
        return Aabb::CreateFromMinMax(m_min + offset, m_max + offset);
    }


    AZ_MATH_INLINE float Aabb::GetSurfaceArea() const
    {
        float dx = GetXExtent();
        float dy = GetYExtent();
        float dz = GetZExtent();
        return 2.0f * (dx * dy + dy * dz + dz * dx);
    }


    AZ_MATH_INLINE void Aabb::MultiplyByScale(const Vector3& scale)
    {
        m_min *= scale;
        m_max *= scale;
        AZ_MATH_ASSERT(IsValid(), "Min must be less than Max");
    }


    AZ_MATH_INLINE Aabb Aabb::GetTransformedAabb(const Transform& transform) const
    {
        Aabb aabb = Aabb::CreateFromMinMax(m_min, m_max);
        aabb.ApplyTransform(transform);
        return aabb;
    }


    AZ_MATH_INLINE Aabb Aabb::GetTransformedAabb(const Matrix3x4& matrix3x4) const
    {
        Aabb aabb = Aabb::CreateFromMinMax(m_min, m_max);
        aabb.ApplyMatrix3x4(matrix3x4);
        return aabb;
    }


    AZ_MATH_INLINE bool Aabb::IsClose(const Aabb& rhs, float tolerance) const
    {
        return m_min.IsClose(rhs.m_min, tolerance) && m_max.IsClose(rhs.m_max, tolerance);
    }


    AZ_MATH_INLINE bool Aabb::IsValid() const
    {
        return m_min.IsLessEqualThan(m_max);
    }


    AZ_MATH_INLINE bool Aabb::IsFinite() const
    {
        return m_min.IsFinite() && m_max.IsFinite();
    }
}
