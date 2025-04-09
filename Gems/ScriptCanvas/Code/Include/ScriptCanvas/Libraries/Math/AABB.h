/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/tuple.h>


namespace ScriptCanvas
{
    namespace AABBFunctions
    {
        AZ::Aabb AddAABB(AZ::Aabb a, const AZ::Aabb& b);

        AZ::Aabb AddPoint(AZ::Aabb source, AZ::Vector3 point);

        AZ::Aabb ApplyTransform(AZ::Aabb source, const AZ::Transform& transformType);

        AZ::Vector3 Center(const AZ::Aabb& source);

        AZ::Aabb Clamp(const AZ::Aabb& source, const AZ::Aabb& clamp);

        bool ContainsAABB(const AZ::Aabb& source, const AZ::Aabb& candidate);

        bool ContainsVector3(const AZ::Aabb& source, const AZ::Vector3& candidate);

        double Distance(const AZ::Aabb& source, const AZ::Vector3 point);

        AZ::Aabb Expand(const AZ::Aabb& source, const AZ::Vector3 delta);

        AZ::Vector3 Extents(const AZ::Aabb& source);

        AZ::Aabb FromCenterHalfExtents(const AZ::Vector3 center, const AZ::Vector3 halfExtents);

        AZ::Aabb FromCenterRadius(const AZ::Vector3 center, const double radius);

        AZ::Aabb FromMinMax(const AZ::Vector3 min, const AZ::Vector3 max);

        AZ::Aabb FromOBB(const AZ::Obb& source);

        AZ::Aabb FromPoint(const AZ::Vector3& source);

        AZ::Vector3 GetMax(const AZ::Aabb& source);

        AZ::Vector3 GetMin(const AZ::Aabb& source);

        bool IsFinite(const AZ::Aabb& source);

        bool IsValid(const AZ::Aabb& source);

        AZ::Aabb Null();

        bool Overlaps(const AZ::Aabb& a, const AZ::Aabb& b);

        double SurfaceArea(const AZ::Aabb& source);

        AZStd::tuple<AZ::Vector3, double> ToSphere(const AZ::Aabb& source);

        AZ::Aabb Translate(const AZ::Aabb& source, const AZ::Vector3 translation);

        double XExtent(const AZ::Aabb& source);

        double YExtent(const AZ::Aabb& source);

        double ZExtent(const AZ::Aabb& source);
    } // namespace AABBFunctions
} // namespace ScriptCanvas


#include <Include/ScriptCanvas/Libraries/Math/AABB.generated.h>