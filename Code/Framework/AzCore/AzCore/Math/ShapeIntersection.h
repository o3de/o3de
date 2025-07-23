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
        AZCORE_API bool Overlaps(const Aabb& aabb1, const Aabb& aabb2);
        AZCORE_API bool Overlaps(const Aabb& aabb, const Sphere& sphere);
        AZCORE_API bool Overlaps(const Sphere& sphere, const Aabb& aabb);
        AZCORE_API bool Overlaps(const Sphere& sphere, const Frustum& frustum);
        AZCORE_API bool Overlaps(const Sphere& sphere, const Plane& plane);
        AZCORE_API bool Overlaps(const Sphere& sphere1, const Sphere& sphere2);
        AZCORE_API bool Overlaps(const Sphere& sphere, const Obb& obb);
        AZCORE_API bool Overlaps(const Sphere& sphere, const Capsule& capsule);
        AZCORE_API bool Overlaps(const Hemisphere& hemisphere, const Sphere& sphere);
        AZCORE_API bool Overlaps(const Hemisphere& hemisphere, const Aabb& aabb); // Can have false positives for near intersections.
        AZCORE_API bool Overlaps(const Frustum& frustum, const Sphere& sphere);
        AZCORE_API bool Overlaps(const Frustum& frustum, const Obb& obb);
        AZCORE_API bool Overlaps(const Frustum& frustum, const Aabb& aabb);
        AZCORE_API bool Overlaps(const Capsule& capsule1, const Capsule& capsule2);
        AZCORE_API bool Overlaps(const Capsule& capsule, const Obb& obb);
        AZCORE_API bool Overlaps(const Capsule& capsule, const Sphere& sphere);
        AZCORE_API bool Overlaps(const Capsule& capsule, const Aabb& aabb);
        AZCORE_API bool Overlaps(const Aabb& aabb, const Capsule& capsule);
        AZCORE_API bool Overlaps(const Obb& obb1, const Obb& obb2);
        AZCORE_API bool Overlaps(const Obb& obb, const Capsule& capsule);
        AZCORE_API bool Overlaps(const Obb& obb, const Sphere& sphere);
        //! @}

        //! Tests to see if Arg1 contains Arg2. Non Symmetric.
        //! @{
        AZCORE_API bool Contains(const Aabb& aabb1, const Aabb& aabb2);
        AZCORE_API bool Contains(const Aabb& aabb, const Sphere& sphere);
        AZCORE_API bool Contains(const Sphere& sphere,  const Aabb& aabb);
        AZCORE_API bool Contains(const Sphere& sphere,  const Vector3& point);
        AZCORE_API bool Contains(const Sphere& sphere1, const Sphere& sphere2);
        AZCORE_API bool Contains(const Hemisphere& hemisphere, const Aabb& aabb);
        AZCORE_API bool Contains(const Capsule& capsule, const Sphere& sphere);
        AZCORE_API bool Contains(const Capsule& capsule, const Aabb& aabb);
        AZCORE_API bool Contains(const Frustum& frustum,  const Aabb& aabb);
        AZCORE_API bool Contains(const Frustum& frustum,  const Sphere& sphere);
        AZCORE_API bool Contains(const Frustum& frustum,  const Vector3& point);
        //! @}
    }
}

#include <AzCore/Math/ShapeIntersection.inl>
