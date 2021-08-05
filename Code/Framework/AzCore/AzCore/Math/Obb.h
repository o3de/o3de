/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Quaternion.h>

namespace AZ
{
    class Aabb;

    //! An oriented bounding box.
    class Obb
    {
    public:

        struct Axis
        {
            Vector3 m_axis;
            float m_halfLength;
        };

        AZ_TYPE_INFO(Obb, "{004abd25-cf14-4eb3-bd41-022c247c07fa}");

        //! AzCore Reflection.
        //! @param context reflection context
        static void Reflect(ReflectContext* context);

        Obb() = default;

        static Obb CreateFromPositionRotationAndHalfLengths(
            const Vector3& position, const Quaternion& rotation, const Vector3& halfLengths);

        //! Converts an AABB into an OBB.
        static Obb CreateFromAabb(const Aabb& aabb);

        const Vector3& GetPosition() const;
        const Quaternion& GetRotation() const;
        void SetRotation(const Quaternion& rotation);
        const Vector3& GetHalfLengths() const;
        void SetHalfLengths(const Vector3& halfLengths);

        Vector3 GetAxisX() const;
        Vector3 GetAxisY() const;
        Vector3 GetAxisZ() const;
        Vector3 GetAxis(int32_t index) const;
        float GetHalfLengthX() const;
        float GetHalfLengthY() const;
        float GetHalfLengthZ() const;
        float GetHalfLength(int32_t index) const;

        void SetPosition(const Vector3& position);
        void SetHalfLengthX(float halfLength);
        void SetHalfLengthY(float halfLength);
        void SetHalfLengthZ(float halfLength);
        void SetHalfLength(int32_t index, float halfLength);

        //! Returns whether a point is contained inside the OBB.
        //! @param point The point to be tested against the OBB.
        //! @return Returns true if the point is inside the OBB, otherwise false.
        bool Contains(const Vector3& point) const;

        //! Calculates distance from the OBB to specified point, a point inside the OBB will return zero.
        //! @param point The point for which to find the distance from the OBB.
        //! @return The distance from the OBB, or 0 if the point is inside the OBB.
        float GetDistance(const Vector3& point) const;

        //! Calculates squared distance from the OBB to specified point, a point inside the OBB will return zero.
        //! @param point The point for which to find the squared distance from the OBB.
        //! @return The squared distance from the OBB, or 0 if the point is inside the OBB.
        float GetDistanceSq(const Vector3& point) const;

        bool IsFinite() const;

        bool operator==(const Obb& rhs) const;
        bool operator!=(const Obb& rhs) const;

    private:
        Vector3 m_position;
        Quaternion m_rotation;
        Vector3 m_halfLengths;
    };

    Obb operator*(const class Transform& transform, const Obb& obb);
}

#include <AzCore/Math/Obb.inl>
