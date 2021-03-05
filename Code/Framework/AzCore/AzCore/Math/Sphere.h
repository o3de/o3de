/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
