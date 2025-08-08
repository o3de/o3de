
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AzCore/Math/Vector2.h"
namespace AZ
{
    class Ray;
    class Vector3;

    namespace RayIntersection
    {
        bool RayTriangle(const AZ::Ray& ray, const AZ::Vector3& p1, const AZ::Vector3 p2, const AZ::Vector3& p3, AZ::Vector3& hitPoint, AZ::Vector2& barycentricUV);
    }
}

#include <AzCore/Math/RayIntersection.inl>
