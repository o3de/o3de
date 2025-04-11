/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SurfaceData/SurfaceDataTypes.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>

AZ_DECLARE_BUDGET(SurfaceData);

namespace AZ
{
    namespace RPI
    {
        class ModelAsset;
    }
}

namespace SurfaceData
{
    AZ_INLINE bool GetQuadListRayIntersection(
        const AZStd::vector<AZ::Vector3>& vertices,
        const AZ::Vector3& rayOrigin,
        const AZ::Vector3& rayDirection,
        const float& rayMaxRange,
        AZ::Vector3& outPosition,
        AZ::Vector3& outNormal)
    {
        AZ_PROFILE_FUNCTION(SurfaceData);

        const size_t vertexCount = vertices.size();
        if (vertexCount > 0 && vertexCount % 4 == 0)
        {
            // Make sure our raycast segment is at least 1 mm long.  If we have a 0-length ray, we'll never intersect.
            const float adjustedMaxRange = AZStd::max(0.001f, rayMaxRange);
            const AZ::Vector3 rayLength = rayDirection * adjustedMaxRange;
            const AZ::Vector3 rayEnd = rayOrigin + rayLength;

            AZ::Intersect::SegmentTriangleHitTester hitTester(rayOrigin, rayEnd);

            for (size_t vertexIndex = 0; vertexIndex < vertexCount; vertexIndex += 4)
            {
                // This could potentially be optimized further with a single segment / quad intersection check.
                // Unfortunately, AZ::Intersect::IntersectRayQuad() currently returns different
                // (and worse) results than IntersectSegmentTriangle.  It might be that our surface
                // quads aren't actually planar, or it might just be a precision or winding order issue.

                float resultDistance = 0.0f;
                if (hitTester.IntersectSegmentTriangle(
                    vertices[vertexIndex + 0],
                    vertices[vertexIndex + 2],
                    vertices[vertexIndex + 3],
                    outNormal,
                    resultDistance))
                {
                    outPosition = rayOrigin + (rayLength * resultDistance);
                    return true;
                }

                resultDistance = 0.0f;
                if (hitTester.IntersectSegmentTriangle(
                    vertices[vertexIndex + 0],
                    vertices[vertexIndex + 3],
                    vertices[vertexIndex + 1],
                    outNormal,
                    resultDistance))
                {
                    outPosition = rayOrigin + (rayLength * resultDistance);
                    return true;
                }
            }
        }

        return false;
    }

    bool GetMeshRayIntersection(
        const AZ::RPI::ModelAsset& meshAsset, const AZ::Transform& meshTransform,
        const AZ::Transform& meshTransformInverse, const AZ::Vector3& nonUniformScale,
        const AZ::Vector3& rayStart, const AZ::Vector3& rayEnd,
        AZ::Vector3& outPosition, AZ::Vector3& outNormal);

    template<typename SourceContainer>
    AZ_INLINE bool HasMatchingTag(const SourceContainer& sourceTags, const AZ::Crc32& sampleTag)
    {
        return AZStd::find(sourceTags.begin(), sourceTags.end(), sampleTag) != sourceTags.end();
    }

    template<typename SourceContainer, typename SampleContainer>
    AZ_INLINE bool HasAnyMatchingTags(const SourceContainer& sourceTags, const SampleContainer& sampleTags)
    {
        for (const auto& sampleTag : sampleTags)
        {
            if (HasMatchingTag(sourceTags, sampleTag))
            {
                return true;
            }
        }

        return false;
    }
     
    AZ_INLINE bool HasValidTags(const SurfaceTagVector& sourceTags)
    {
        for (const auto& sourceTag : sourceTags)
        {
            if (sourceTag != Constants::s_unassignedTagCrc)
            {
                return true;
            }
        }
        return false;
    }

    // Utility method to compare two AABBs for overlapping XY coordinates while ignoring the Z coordinates.
    AZ_INLINE bool AabbOverlaps2D(const AZ::Aabb& box1, const AZ::Aabb& box2)
    {
        return box1.GetMin().GetX() <= box2.GetMax().GetX() &&
               box1.GetMin().GetY() <= box2.GetMax().GetY() &&
               box1.GetMax().GetX() >= box2.GetMin().GetX() &&
               box1.GetMax().GetY() >= box2.GetMin().GetY();
    }

    // Utility method to compare an AABB and a point for overlapping XY coordinates while ignoring the Z coordinates.
    template<typename VectorType>
    AZ_INLINE bool AabbContains2D(const AZ::Aabb& box, const VectorType& point)
    {
        return box.GetMin().GetX() <= point.GetX() &&
               box.GetMin().GetY() <= point.GetY() &&
               box.GetMax().GetX() >= point.GetX() &&
               box.GetMax().GetY() >= point.GetY();
    }

    // Utility method to compare an AABB and a point for overlapping XY coordinates while ignoring the Z coordinates.
    // This method includes points that land on the min edge but excludes points that land on the max edge.
    template<typename VectorType>
    AZ_INLINE bool AabbContains2DMaxExclusive(const AZ::Aabb& box, const VectorType& point)
    {
        return box.GetMin().GetX() <= point.GetX() &&
               box.GetMin().GetY() <= point.GetY() &&
               box.GetMax().GetX() > point.GetX() &&
               box.GetMax().GetY() > point.GetY();
    }
}
