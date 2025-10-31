/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AABB.h"

namespace ScriptCanvas
{
    namespace AABBFunctions
    {
        AZ::Aabb AddAABB(AZ::Aabb a, const AZ::Aabb& b)
        {
            a.AddAabb(b);
            return a;
        }

        AZ::Aabb AddPoint(AZ::Aabb source, AZ::Vector3 point)
        {
            source.AddPoint(point);
            return source;
        }

        AZ::Aabb ApplyTransform(AZ::Aabb source, const AZ::Transform& transformType)
        {
            source.ApplyTransform(transformType);
            return source;
        }

        AZ::Vector3 Center(const AZ::Aabb& source)
        {
            return source.GetCenter();
        }

        AZ::Aabb Clamp(const AZ::Aabb& source, const AZ::Aabb& clamp)
        {
            return source.GetClamped(clamp);
        }

        bool ContainsAABB(const AZ::Aabb& source, const AZ::Aabb& candidate)
        {
            return source.Contains(candidate);
        }

        bool ContainsVector3(const AZ::Aabb& source, const AZ::Vector3& candidate)
        {
            return source.Contains(candidate);
        }

        double Distance(const AZ::Aabb& source, const AZ::Vector3 point)
        {
            return source.GetDistance(point);
        }

        AZ::Aabb Expand(const AZ::Aabb& source, const AZ::Vector3 delta)
        {
            return source.GetExpanded(delta.GetAbs());
        }

        AZ::Vector3 Extents(const AZ::Aabb& source)
        {
            return source.GetExtents();
        }

        AZ::Aabb FromCenterHalfExtents(const AZ::Vector3 center, const AZ::Vector3 halfExtents)
        {
            return AZ::Aabb::CreateCenterHalfExtents(center, halfExtents);
        }

        AZ::Aabb FromCenterRadius(const AZ::Vector3 center, const double radius)
        {
            return AZ::Aabb::CreateCenterRadius(center, static_cast<float>(radius));
        }

        AZ::Aabb FromMinMax(const AZ::Vector3 min, const AZ::Vector3 max)
        {
            return min.IsLessEqualThan(max) ? AZ::Aabb::CreateFromMinMax(min, max) : AZ::Aabb::CreateFromPoint(max);
        }

        AZ::Aabb FromOBB(const AZ::Obb& source)
        {
            return AZ::Aabb::CreateFromObb(source);
        }

        AZ::Aabb FromPoint(const AZ::Vector3& source)
        {
            return AZ::Aabb::CreateFromPoint(source);
        }

        AZ::Vector3 GetMax(const AZ::Aabb& source)
        {
            return source.GetMax();
        }

        AZ::Vector3 GetMin(const AZ::Aabb& source)
        {
            return source.GetMin();
        }

        bool IsFinite(const AZ::Aabb& source)
        {
            return source.IsFinite();
        }

        bool IsValid(const AZ::Aabb& source)
        {
            return source.IsValid();
        }

        AZ::Aabb Null()
        {
            return AZ::Aabb::CreateNull();
        }

        bool Overlaps(const AZ::Aabb& a, const AZ::Aabb& b)
        {
            return a.Overlaps(b);
        }

        double SurfaceArea(const AZ::Aabb& source)
        {
            return source.GetSurfaceArea();
        }

        AZStd::tuple<AZ::Vector3, double> ToSphere(const AZ::Aabb& source)
        {
            AZ::Vector3 center;
            float radius;
            source.GetAsSphere(center, radius);
            return AZStd::make_tuple(center, radius);
        }

        AZ::Aabb Translate(const AZ::Aabb& source, const AZ::Vector3 translation)
        {
            return source.GetTranslated(translation);
        }

        double XExtent(const AZ::Aabb& source)
        {
            return source.GetXExtent();
        }

        double YExtent(const AZ::Aabb& source)
        {
            return source.GetYExtent();
        }

        double ZExtent(const AZ::Aabb& source)
        {
            return source.GetZExtent();
        }
    } // namespace AABBFunctions
} // namespace ScriptCanvas
