/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/Math/Vector3.h>

namespace AZ
{
    class Obb;
    class ReflectContext;

    //! An axis aligned bounding box.
    //! It is defined as a closed set, i.e. it includes the boundary, so it will always include at least one point.
    class Aabb
    {
    public:

        AZ_TYPE_INFO(Aabb, "{A54C2B36-D5B8-46A1-A529-4EBDBD2450E7}");

        //! AzCore Reflection.
        //! @param context reflection context
        static void Reflect(ReflectContext* context);

        //! Creates a null AABB.
        //! This is an invalid AABB which has no size, but is useful as adding a point to it will make it valid
        static Aabb CreateNull();

        static Aabb CreateFromPoint(const Vector3& p);

        static Aabb CreateFromMinMax(const Vector3& min, const Vector3& max);

        static Aabb CreateFromMinMaxValues(float minX, float minY, float minZ, float maxX, float maxY, float maxZ);

        static Aabb CreateCenterHalfExtents(const Vector3& center, const Vector3& halfExtents);

        static Aabb CreateCenterRadius(const Vector3& center, float radius);

        //! Creates an AABB which contains the specified points.
        static Aabb CreatePoints(const Vector3* pts, size_t numPts);

        //! Creates an AABB which contains the specified OBB.
        static Aabb CreateFromObb(const Obb& obb);

        Aabb() = default;

        const Vector3& GetMin() const;

        const Vector3& GetMax() const;

        void Set(const Vector3& min, const Vector3& max);

        void SetMin(const Vector3& min);

        void SetMax(const Vector3& max);

        bool operator==(const AZ::Aabb& aabb) const;

        bool operator!=(const AZ::Aabb& aabb) const;

        float GetXExtent() const;

        float GetYExtent() const;

        float GetZExtent() const;

        Vector3 GetExtents() const;

        Vector3 GetCenter() const;

        Vector3 GetSupport(const Vector3& normal) const;

        void GetAsSphere(Vector3& center, float& radius) const;

        bool Contains(const Vector3& v) const;

        bool Contains(const Aabb& aabb) const;

        bool Overlaps(const Aabb& aabb) const;

        bool Disjoint(const Aabb& aabb) const;

        //! Expands all dimensions by delta, minimum and maximum, such that GetExtents would return values 2 x delta larger.
        void Expand(const Vector3& delta);

        //! Expands all dimensions by delta, minimum and maximum, such that GetExtents would return values 2 x delta larger.
        const Aabb GetExpanded(const Vector3& delta) const;

        void AddPoint(const Vector3& p);

        void AddAabb(const Aabb& box);

        //! Calculates distance from the AABB to specified point, a point inside the AABB will return zero.
        float GetDistance(const Vector3& p) const;

        //! Calculates squared distance from the AABB to specified point, a point inside the AABB will return zero.
        float GetDistanceSq(const Vector3& p) const;

        //! Calculates maximum distance from the AABB to specified point.
        //! This will always be at least the distance from the center to the corner, even for points inside the AABB.
        float GetMaxDistance(const Vector3& p) const;

        //! Calculates maximum squared distance from the AABB to specified point.
        //! This will always be at least the squared distance from the center to the corner, even for points inside the AABB.
        float GetMaxDistanceSq(const Vector3& p) const;

        //! Clamps the AABB to be contained within the specified AABB.
        Aabb GetClamped(const Aabb& clamp) const;

        void Clamp(const Aabb& clamp);

        void SetNull();

        void Translate(const Vector3& offset);

        Aabb GetTranslated(const Vector3& offset) const;

        float GetSurfaceArea() const;

        void ApplyTransform(const Transform& transform);

        void ApplyMatrix3x4(const Matrix3x4& matrix3x4);

        void MultiplyByScale(const Vector3& scale);

        //! Transforms an Aabb and returns the resulting Obb.
        [[nodiscard]] Obb GetTransformedObb(const Transform& transform) const;

        //! Transforms an Aabb and returns the resulting Obb.
        [[nodiscard]] Obb GetTransformedObb(const Matrix3x4& matrix3x4) const;

        //! Returns a new AABB containing the transformed AABB.
        [[nodiscard]] Aabb GetTransformedAabb(const Transform& transform) const;

        //! Returns a new AABB containing the transformed AABB.
        [[nodiscard]] Aabb GetTransformedAabb(const Matrix3x4& matrix3x4) const;

        //! Checks if this aabb is equal to another within a floating point tolerance.
        bool IsClose(const Aabb& rhs, float tolerance = Constants::Tolerance) const;

        bool IsValid() const;

        bool IsFinite() const;

    protected:

        Vector3 m_min;
        Vector3 m_max;

    };
}

#include <AzCore/Math/Aabb.inl>
