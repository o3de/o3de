/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Capsule.h>
#include <AzCore/Math/Frustum.h>
#include <AzCore/Math/Hemisphere.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Plane.h>
#include <AzCore/Math/Sphere.h>
#include <AzCore/Math/Vector3.h>

namespace AZ
{
    namespace ShapeIntersection
    {
        bool IntersectThreePlanes(const Plane& p1, const Plane& p2, const Plane& p3, Vector3& outP);

        //! Tests to see which halfspace Arg2 is in relative to Arg1.
        IntersectResult Classify(const Plane& plane, const Sphere& sphere);
        IntersectResult Classify(const Plane& plane, const Obb& obb);

        //! Tests to see how Arg2 relates to frustum Arg1.
        IntersectResult Classify(const Frustum& frustum, const Sphere& sphere);

        //! Tests to see if Arg1 overlaps Arg2. Symmetric.
        //! @{
        bool Overlaps(const Aabb& aabb1, const Aabb& aabb2);
        bool Overlaps(const Aabb& aabb, const Sphere& sphere);
        bool Overlaps(const Sphere& sphere, const Aabb& aabb);
        bool Overlaps(const Sphere& sphere, const Frustum& frustum);
        bool Overlaps(const Sphere& sphere, const Plane& plane);
        bool Overlaps(const Sphere& sphere1, const Sphere& sphere2);
        bool Overlaps(const Sphere& sphere, const Obb& obb);
        bool Overlaps(const Sphere& sphere, const Capsule& capsule);
        bool Overlaps(const Hemisphere& hemisphere, const Sphere& sphere);
        bool Overlaps(const Hemisphere& hemisphere, const Aabb& aabb); // Can have false positives for near intersections.
        bool Overlaps(const Frustum& frustum, const Sphere& sphere);
        bool Overlaps(const Frustum& frustum, const Obb& obb);
        bool Overlaps(const Frustum& frustum, const Aabb& aabb);
        bool Overlaps(const Capsule& capsule1, const Capsule& capsule2);
        bool Overlaps(const Capsule& capsule, const Obb& obb);
        bool Overlaps(const Capsule& capsule, const Sphere& sphere);
        bool Overlaps(const Capsule& capsule, const Aabb& aabb);
        bool Overlaps(const Aabb& aabb, const Capsule& capsule);
        bool Overlaps(const Obb& obb1, const Obb& obb2);
        bool Overlaps(const Obb& obb, const Capsule& capsule);
        bool Overlaps(const Obb& obb, const Sphere& sphere);
        //! @}

        //! Tests to see if Arg1 contains Arg2. Non Symmetric.
        //! @{
        bool Contains(const Aabb& aabb1, const Aabb& aabb2);
        bool Contains(const Aabb& aabb, const Sphere& sphere);
        bool Contains(const Sphere& sphere,  const Aabb& aabb);
        bool Contains(const Sphere& sphere,  const Vector3& point);
        bool Contains(const Sphere& sphere1, const Sphere& sphere2);
        bool Contains(const Hemisphere& hemisphere, const Aabb& aabb);
        bool Contains(const Capsule& capsule, const Sphere& sphere);
        bool Contains(const Capsule& capsule, const Aabb& aabb);
        bool Contains(const Frustum& frustum,  const Aabb& aabb);
        bool Contains(const Frustum& frustum,  const Sphere& sphere);
        bool Contains(const Frustum& frustum,  const Vector3& point);
        //! @}
    }
}

#include <AzCore/Math/ShapeIntersection.inl>
