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
    AZ_MATH_INLINE Plane::Plane(Simd::Vec4::FloatArgType plane)
        : m_plane(plane)
    {
        ;
    }


    AZ_MATH_INLINE Plane Plane::CreateFromNormalAndPoint(const Vector3& normal, const Vector3& point)
    {
        AZ_MATH_ASSERT(normal.IsNormalized(), "This normal is not normalized");
        return Plane(Simd::Vec4::ConstructPlane(normal.GetSimdValue(), point.GetSimdValue()));
    }


    AZ_MATH_INLINE Plane Plane::CreateFromNormalAndDistance(const Vector3& normal, float dist)
    {
        Plane result;
        result.Set(normal, dist);
        return result;
    }


    AZ_MATH_INLINE Plane Plane::CreateFromCoefficients(const float a, const float b, const float c, const float d)
    {
        Plane result;
        result.Set(a, b, c, d);
        return result;
    }


    AZ_MATH_INLINE Plane Plane::CreateFromTriangle(const Vector3& v0, const Vector3& v1, const Vector3& v2)
    {
        Plane result;
        Vector3 normal = ((v1 - v0).Cross(v2 - v0)).GetNormalized();
        float dist = -(normal.Dot(v0));
        result.Set(normal, dist);
        return result;
    }


    AZ_MATH_INLINE Plane Plane::CreateFromVectorCoefficients(const Vector4& coefficients)
    {
        Plane result;
        result.Set(coefficients);
        return result;
    }


    AZ_MATH_INLINE void Plane::Set(const Vector4& plane)
    {
        m_plane = plane;
    }


    AZ_MATH_INLINE void Plane::Set(const Vector3& normal, float d)
    {
        AZ_MATH_ASSERT(normal.IsNormalized(), "This normal is not normalized");
        m_plane.Set(normal, d);
    }


    AZ_MATH_INLINE void Plane::Set(float a, float b, float c, float d)
    {
        AZ_MATH_ASSERT(Vector3(a, b, c).IsNormalized(), "This normal is not normalized");
        m_plane.Set(a, b, c, d);
    }


    AZ_MATH_INLINE void Plane::SetNormal(const Vector3& normal)
    {
        AZ_MATH_ASSERT(normal.IsNormalized(), "This normal is not normalized");
        m_plane.SetX(normal.GetX());
        m_plane.SetY(normal.GetY());
        m_plane.SetZ(normal.GetZ());
    }


    AZ_MATH_INLINE void Plane::SetDistance(float d)
    {
        m_plane.SetW(d);
    }


    AZ_MATH_INLINE Plane Plane::GetTransform(const Transform& tm) const
    {
        float newDist = -GetDistance();
        newDist += m_plane.GetX() * (tm.GetBasisX().Dot(tm.GetTranslation()));
        newDist += m_plane.GetY() * (tm.GetBasisY().Dot(tm.GetTranslation()));
        newDist += m_plane.GetZ() * (tm.GetBasisZ().Dot(tm.GetTranslation()));
        Vector3 normal = GetNormal();
        normal = tm.TransformVector(normal);
        return CreateFromNormalAndDistance(normal, -newDist);
    }


    AZ_MATH_INLINE void Plane::ApplyTransform(const Transform& tm)
    {
        *this = GetTransform(tm);
    }


    AZ_MATH_INLINE const Vector4& Plane::GetPlaneEquationCoefficients() const
    {
        return m_plane;
    }


    AZ_MATH_INLINE Vector3 Plane::GetNormal() const
    {
        return m_plane.GetAsVector3();
    }


    AZ_MATH_INLINE float Plane::GetDistance() const
    {
        return m_plane.GetW();
    }


    AZ_MATH_INLINE float Plane::GetPointDist(const Vector3& pos) const
    {
        return Simd::Vec1::SelectIndex0(Simd::Vec4::PlaneDistance(m_plane.GetSimdValue(), pos.GetSimdValue()));
    }


    AZ_MATH_INLINE Vector3 Plane::GetProjected(const Vector3& v) const
    {
        Vector3 n = m_plane.GetAsVector3();
        return v - (n * v.Dot(n));
    }


    AZ_MATH_INLINE bool Plane::CastRay(const Vector3& start, const Vector3& dir, Vector3& hitPoint) const
    {
        float t;
        if (!CastRay(start, dir, t))
        {
            return false;
        }
        hitPoint = start + dir * t;
        return true;
    }


    AZ_MATH_INLINE bool Plane::CastRay(const Vector3& start, const Vector3& dir, float& hitTime) const
    {
        float nDotDir = m_plane.Dot3(dir);
        if (IsClose(nDotDir, 0.0f, Constants::FloatEpsilon))
        {
            return false;
        }
        hitTime = -(m_plane.GetW() + m_plane.Dot3(start)) / nDotDir;
        return true;
    }


    AZ_MATH_INLINE bool Plane::IntersectSegment(const Vector3& start, const Vector3& end, Vector3& hitPoint) const
    {
        Vector3 dir = end - start;
        float hitTime;
        if (!CastRay(start, dir, hitTime))
        {
            return false;
        }
        if (hitTime >= 0.0f && hitTime <= 1.0f)
        {
            hitPoint = start + dir * hitTime;
            return true;
        }
        return false;
    }


    AZ_MATH_INLINE bool Plane::IntersectSegment(const Vector3& start, const Vector3& end, float& hitTime) const
    {
        Vector3 dir = end - start;
        if (!CastRay(start, dir, hitTime))
        {
            return false;
        }
        if (hitTime >= 0.0f && hitTime <= 1.0f)
        {
            return true;
        }
        return false;
    }


    AZ_MATH_INLINE bool Plane::IsFinite() const
    {
        return m_plane.IsFinite();
    }


    AZ_MATH_INLINE bool Plane::operator==(const Plane& rhs) const
    {
        return m_plane.IsClose(rhs.m_plane);
    }


    AZ_MATH_INLINE bool Plane::operator!=(const Plane& rhs) const
    {
        return !(*this == rhs);
    }


    AZ_MATH_INLINE Simd::Vec4::FloatType Plane::GetSimdValue() const
    {
        return m_plane.GetSimdValue();
    }
}
