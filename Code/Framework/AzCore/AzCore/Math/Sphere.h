/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>

namespace AZ
{
    class Aabb;

    //! A simple bounding sphere class for fast intersection testing.
    class Sphere
    {
    public:

        AZ_TYPE_INFO(Sphere, "{34BB6527-81AE-4854-99ED-D1A319DCD0A9}");

        Sphere() = default;

        explicit Sphere(const Vector3& center, float radius);

        static Sphere CreateUnitSphere();
        static Sphere CreateFromAabb(const Aabb& aabb);

        const Vector3& GetCenter() const;
        float GetRadius() const;
        void SetCenter(const Vector3& center);
        void SetRadius(float radius);

        void Set(const Sphere& sphere);

        Sphere& operator=(const Sphere& rhs);

        bool operator==(const Sphere& rhs) const;

        bool operator!=(const Sphere& rhs) const;

    private:

        Vector3 m_center;
        float m_radius;

    };
}

#include <AzCore/Math/Sphere.inl>
