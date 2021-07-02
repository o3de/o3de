/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Transform.h>

namespace AZ
{
    //! Intersection classifications.
    enum class IntersectResult
    {
        Interior // Object is fully contained within the bounding primitive
    ,   Overlaps // Object intersects the bounding primitive
    ,   Exterior // Object lies completely outside the bounding primitive
    };

    //! Plane defined by a Vector4 with components (A,B,C,D), the plane equation used is Ax+By+Cz+D=0.
    //! The distance of a point from the plane is given by Ax+By+Cz+D, or just simply plane.Dot4(point).
    //!
    //! Note that the D component is often referred to as 'distance' below, in the sense that it is the distance of the
    //! origin from the plane. This is not the same as what many other people refer to as the plane distance, which
    //! would be the negation of this.
    class Plane
    {
    public:

        AZ_TYPE_INFO(Plane, "{847DD984-9DBF-4789-8E25-E0334402E8AD}");

        //! AzCore Reflection.
        //! @param context reflection context
        static void Reflect(ReflectContext* context);

        //! Default construction, no initialization.
        Plane() = default;

        //! Construct from a simd vector representing a plane.
        explicit Plane(Simd::Vec4::FloatArgType plane);

        static Plane CreateFromNormalAndPoint(const Vector3& normal, const Vector3& point);

        static Plane CreateFromNormalAndDistance(const Vector3& normal, float dist);

        static Plane CreateFromCoefficients(const float a, const float b, const float c, const float d);

        //! Assumes counter clockwise(right handed) winding.
        static Plane CreateFromTriangle(const Vector3& v0, const Vector3& v1, const Vector3& v2);

        static Plane CreateFromVectorCoefficients(const Vector4& coefficients);

        void Set(const Vector4& plane);
        void Set(const Vector3& normal, float d);
        void Set(float a, float b, float c, float d);

        void SetNormal(const Vector3& normal);

        void SetDistance(float d);

        //! Applies the specified transform to the plane, returns the transformed plane.
        Plane GetTransform(const Transform& tm) const;

        void ApplyTransform(const Transform& tm);

        //! Returns the coefficients of the plane equation as a Vector4, i.e. (A,B,C,D).
        const Vector4& GetPlaneEquationCoefficients() const;

        Vector3 GetNormal() const;
        float GetDistance() const;

        //! Calculates the distance between a point and the plane.
        float GetPointDist(const Vector3& pos) const;

        //! Project vector onto a plane.
        Vector3 GetProjected(const Vector3& v) const;

        //! Returns true if ray plane intersect, otherwise false. If true hitPoint contains the result.
        bool CastRay(const Vector3& start, const Vector3& dir, Vector3& hitPoint) const;

        //! Returns true if ray plane intersect, otherwise false. If true hitTime contains the result.
        //! To produce the point of intersection using hitTime, hitPoint = start + dir * hitTime.
        bool CastRay(const Vector3& start, const Vector3& dir, float& hitTime) const;

        //! Intersect the plane with line segment. Return true if they intersect otherwise false.
        bool IntersectSegment(const Vector3& start, const Vector3& end, Vector3& hitPoint) const;

        //! Intersect the plane with line segment. Return true if they intersect otherwise false.
        //! To produce the point of intersection using hitTime, hitPoint = start + dir * hitTime.
        bool IntersectSegment(const Vector3& start, const Vector3& end, float& hitTime) const;

        bool IsFinite() const;

        bool operator==(const Plane& rhs) const;

        bool operator!=(const Plane& rhs) const;

        //! Exposes the internal simd representation of the plane
        Simd::Vec4::FloatType GetSimdValue() const;

    private:

        Vector4 m_plane; //!< plane normal (x,y,z) and negative distance to the origin (w).

    };
}

#include <AzCore/Math/Plane.inl>
