/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Sphere.h>
#include <AzCore/Math/Vector3.h>

namespace AZ
{
    class Aabb;

    //! A simple bounding hemisphere class for fast intersection testing.
    class Hemisphere
    {
    public:
        AZ_TYPE_INFO(Hemisphere, "{B246E336-780E-494B-B330-0985025B7888}");

        Hemisphere() = default;
        Hemisphere(const Vector3& center, float radius, const Vector3& normalizedDirection);

        static Hemisphere CreateFromSphereAndDirection(const Sphere& sphere, const Vector3& normalizedDirection);

        Vector3 GetCenter() const;
        float GetRadius() const;
        const Vector3& GetDirection() const;
        void SetCenter(const Vector3& center);
        void SetRadius(float radius);
        void SetDirection(const Vector3& direction);

        bool operator==(const Hemisphere& rhs) const;
        bool operator!=(const Hemisphere& rhs) const;

    private:
        Vector4 m_centerRadius;
        Vector3 m_direction;
    };
} // namespace AZ

#include <AzCore/Math/Hemisphere.inl>
