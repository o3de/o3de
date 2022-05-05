/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/TypeInfoSimple.h>

#include <AzCore/Math/Sphere.h>

namespace O3DE
{
    class Ray
    {
    public:
        AZ_TYPE_INFO(Ray, "{0301a872-5bea-4563-8070-85ed243cb57c}");

        explicit Ray(const AZ::Vector3& origin, const AZ::Vector3& direction);

        bool Intersects(const AZ::Aabb aabb) const;

        const AZ::Vector3& GetOrigin() const;
        const AZ::Vector3& GetDirection() const;

        Ray& operator=(const Ray& rhs);
        bool operator==(const Ray& rhs) const;
        bool operator!=(const Ray& rhs) const;

    private:
        AZ::Vector3 m_origin;
        AZ::Vector3 m_direction;
    };
} // namespace O3DE