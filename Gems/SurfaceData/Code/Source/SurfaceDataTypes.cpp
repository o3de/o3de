/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SurfaceData/Utility/SurfaceDataUtility.h>
#include <Atom/RPI.Reflect/Model/ModelAssetCreator.h>

namespace SurfaceData
{
    SurfacePointList::SurfacePointList(AZStd::initializer_list<const SurfacePoint> surfacePoints)
    {
        m_surfacePointList.reserve(surfacePoints.size());
        for (auto& point : surfacePoints)
        {
            AddSurfacePoint(AZ::EntityId(), point.m_position, point.m_normal, point.m_masks);
        }
    }

    void SurfacePointList::AddSurfacePoint(const AZ::EntityId& entityId,
        const AZ::Vector3& position, const AZ::Vector3& normal, const SurfaceTagWeightMap& masks)
    {
        if (m_sortAndCombineOnPointInsertion)
        {
            // When adding a surface point, we'll either merge it with a similar existing point, or else add it in order of
            // decreasing Z, so that our final results are sorted.

            auto pointItr = m_surfacePointList.begin();
            for (; pointItr < m_surfacePointList.end(); ++pointItr)
            {
                SurfacePoint& point = *pointItr;

                // (Someday we should add a configurable tolerance for comparison)
                if (point.m_position.IsClose(position) && point.m_normal.IsClose(normal))
                {
                    // consolidate points with similar attributes by adding masks/weights to the similar point instead of adding a new one.
                    AddMaxValueForMasks(point.m_masks, masks);
                    return;
                }
                else if (point.m_position.GetZ() < position.GetZ())
                {
                    m_pointBounds.AddPoint(position);
                    SurfacePoint surfacePoint;
                    surfacePoint.m_position = position;
                    surfacePoint.m_normal = normal;
                    surfacePoint.m_masks = masks;
                    surfacePoint.m_entityId = entityId;
                    m_surfacePointList.insert(pointItr, AZStd::move(surfacePoint));
                    return;
                }
            }

            // The point wasn't merged and the sort puts it at the end, so just fall through to adding the point to the end of the list.
        }

        // We're adding this point to the end of the list.
        m_pointBounds.AddPoint(position);
        SurfacePoint point;
        point.m_position = position;
        point.m_normal = normal;
        point.m_masks = masks;
        point.m_entityId = entityId;
        m_surfacePointList.emplace_back(AZStd::move(point));
    }

    void SurfacePointList::Clear()
    {
        m_surfacePointList.clear();
    }

    void SurfacePointList::ReserveSpace(size_t maxPointsPerInput)
    {
        AZ_Assert(
            m_surfacePointList.size() < maxPointsPerInput,
            "Trying to reserve space on a list that is already using more points than requested.");
        m_surfacePointList.reserve(maxPointsPerInput);
    }

    bool SurfacePointList::IsEmpty() const
    {
        return m_surfacePointList.empty();
    }

    size_t SurfacePointList::GetSize() const
    {
        return m_surfacePointList.size();
    }

    void SurfacePointList::EnumeratePoints(AZStd::function<bool(const SurfacePoint&)> pointCallback) const
    {
        for (auto& point : m_surfacePointList)
        {
            if (!pointCallback(point))
            {
                break;
            }
        }
    }

    void SurfacePointList::ModifySurfaceWeights(
        const AZ::EntityId& currentEntityId,
        AZStd::function<void(SurfacePoint&)> modificationWeightCallback)
    {
        for (auto& point : m_surfacePointList)
        {
            if (point.m_entityId != currentEntityId)
            {
                modificationWeightCallback(point);
            }
        }
    }

    const SurfacePoint& SurfacePointList::GetHighestSurfacePoint() const
    {
        return m_surfacePointList.front();
    }

    void SurfacePointList::FilterPoints(const SurfaceTagVector& desiredTags)
    {
        // Filter out any points that don't match our search tags.
        // This has to be done after the Surface Modifiers have processed the points, not at point insertion time, because
        // Surface Modifiers add tags to existing points.
        m_surfacePointList.erase(
            AZStd::remove_if(
                m_surfacePointList.begin(), m_surfacePointList.end(),
                [desiredTags](SurfacePoint& point) -> bool
                {
                    return !HasMatchingTags(point.m_masks, desiredTags);
                }),
            m_surfacePointList.end());
    }

    void SurfacePointList::SortAndCombineNeighboringPoints()
    {
        // If we've sorted and combined on input, then there's nothing to do here.
        if (m_sortAndCombineOnPointInsertion)
        {
            return;
        }

        // If there's only 0 or 1 point, there is no sorting or combining that needs to happen, so just return.
        if (GetSize() <= 1)
        {
            return;
        }

        // Efficient point consolidation requires the points to be pre-sorted so we are only comparing/combining neighbors.
        // Sort XY points together, with decreasing Z.
        AZStd::sort(
            m_surfacePointList.begin(), m_surfacePointList.end(),
            [](const SurfacePoint& a, const SurfacePoint& b)
            {
                // Our goal is to have identical XY values sorted adjacent to each other with decreasing Z.
                // We sort increasing Y, then increasing X, then decreasing Z, because we need to compare all 3 values for a
                // stable sort. The choice of increasing Y first is because we'll often generate the points as ranges of X values within
                // ranges of Y values, so this will produce the most usable and expected output sort.
                if (a.m_position.GetY() != b.m_position.GetY())
                {
                    return a.m_position.GetY() < b.m_position.GetY();
                }
                if (a.m_position.GetX() != b.m_position.GetX())
                {
                    return a.m_position.GetX() < b.m_position.GetX();
                }
                if (a.m_position.GetZ() != b.m_position.GetZ())
                {
                    return a.m_position.GetZ() > b.m_position.GetZ();
                }

                // If we somehow ended up with two points with identical positions getting generated, use the entity ID as the tiebreaker
                // to guarantee a stable sort. We should never have two identical positions generated from the same entity.
                return a.m_entityId < b.m_entityId;
            });

        // iterate over subsequent source points for comparison and consolidation with the last added target/unique point
        for (auto pointItr = m_surfacePointList.begin() + 1; pointItr < m_surfacePointList.end();)
        {
            auto prevPointItr = pointItr - 1;

            // (Someday we should add a configurable tolerance for comparison)
            if (pointItr->m_position.IsClose(prevPointItr->m_position) &&
                pointItr->m_normal.IsClose(prevPointItr->m_normal))
            {
                // consolidate points with similar attributes by adding masks/weights to the previous point and deleting this point.
                AddMaxValueForMasks(prevPointItr->m_masks, pointItr->m_masks);

                pointItr = m_surfacePointList.erase(pointItr);
            }
            else
            {
                pointItr++;
            }
        }
    }
}
