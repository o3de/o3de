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
    SurfacePointList::SurfacePointList(AZStd::initializer_list<const AzFramework::SurfaceData::SurfacePoint> surfacePoints)
    {
        ReserveSpace(surfacePoints.size());

        for (auto& point : surfacePoints)
        {
            SurfaceTagWeightMap weights;
            for (auto& weight : point.m_surfaceTags)
            {
                weights.emplace(weight.m_surfaceType, weight.m_weight);
            }
            AddSurfacePoint(AZ::EntityId(), point.m_position, point.m_normal, weights);
        }
    }

    void SurfacePointList::AddSurfacePoint(const AZ::EntityId& entityId,
        const AZ::Vector3& position, const AZ::Vector3& normal, const SurfaceTagWeightMap& masks)
    {
        // When adding a surface point, we'll either merge it with a similar existing point, or else add it in order of
        // decreasing Z, so that our final results are sorted.

        for (size_t index = 0; index < m_surfacePositionList.size(); ++index)
        {
            // (Someday we should add a configurable tolerance for comparison)
            if (m_surfacePositionList[index].IsClose(position) && m_surfaceNormalList[index].IsClose(normal))
            {
                // consolidate points with similar attributes by adding masks/weights to the similar point instead of adding a new one.
                AddMaxValueForMasks(m_surfaceWeightsList[index], masks);
                return;
            }
            else if (m_surfacePositionList[index].GetZ() < position.GetZ())
            {
                m_pointBounds.AddPoint(position);
                m_surfacePositionList.insert(m_surfacePositionList.begin() + index, position);
                m_surfaceNormalList.insert(m_surfaceNormalList.begin() + index, normal);
                m_surfaceWeightsList.insert(m_surfaceWeightsList.begin() + index, masks);
                m_surfaceCreatorIdList.insert(m_surfaceCreatorIdList.begin() + index, entityId);
                return;
            }
        }

        // The point wasn't merged and the sort puts it at the end, so just add the point to the end of the list.
        m_pointBounds.AddPoint(position);
        m_surfacePositionList.emplace_back(position);
        m_surfaceNormalList.emplace_back(normal);
        m_surfaceWeightsList.emplace_back(masks);
        m_surfaceCreatorIdList.emplace_back(entityId);
    }

    void SurfacePointList::Clear()
    {
        m_surfacePositionList.clear();
        m_surfaceNormalList.clear();
        m_surfaceWeightsList.clear();
        m_surfaceCreatorIdList.clear();
    }

    void SurfacePointList::ReserveSpace(size_t maxPointsPerInput)
    {
        AZ_Assert(
            m_surfacePositionList.size() < maxPointsPerInput,
            "Trying to reserve space on a list that is already using more points than requested.");

        m_surfaceCreatorIdList.reserve(maxPointsPerInput);
        m_surfacePositionList.reserve(maxPointsPerInput);
        m_surfaceNormalList.reserve(maxPointsPerInput);
        m_surfaceWeightsList.reserve(maxPointsPerInput);
    }

    bool SurfacePointList::IsEmpty() const
    {
        return m_surfacePositionList.empty();
    }

    size_t SurfacePointList::GetSize() const
    {
        return m_surfacePositionList.size();
    }

    void SurfacePointList::EnumeratePoints(
        AZStd::function<bool(const AZ::Vector3&, const AZ::Vector3&, const SurfaceData::SurfaceTagWeightMap&)>
            pointCallback) const
    {
        for (size_t index = 0; index < m_surfacePositionList.size(); index++)
        {
            if (!pointCallback(m_surfacePositionList[index], m_surfaceNormalList[index], m_surfaceWeightsList[index]))
            {
                break;
            }
        }
    }

    void SurfacePointList::ModifySurfaceWeights(
        const AZ::EntityId& currentEntityId,
        AZStd::function<void(const AZ::Vector3&, SurfaceData::SurfaceTagWeightMap&)> modificationWeightCallback)
    {
        for (size_t index = 0; index < m_surfacePositionList.size(); index++)
        {
            if (m_surfaceCreatorIdList[index] != currentEntityId)
            {
                modificationWeightCallback(m_surfacePositionList[index], m_surfaceWeightsList[index]);
            }
        }
    }

    AzFramework::SurfaceData::SurfacePoint SurfacePointList::GetHighestSurfacePoint() const
    {
        AzFramework::SurfaceData::SurfacePoint point;
        point.m_position = m_surfacePositionList.front();
        point.m_normal = m_surfaceNormalList.front();
        const SurfaceTagWeightMap& weights = m_surfaceWeightsList.front();
        for (auto& weight : weights)
        {
            point.m_surfaceTags.emplace_back(weight.first, weight.second);
        }

        return point;
    }

    void SurfacePointList::FilterPoints(const SurfaceTagVector& desiredTags)
    {
        // Filter out any points that don't match our search tags.
        // This has to be done after the Surface Modifiers have processed the points, not at point insertion time, because
        // Surface Modifiers add tags to existing points.
        size_t listSize = m_surfacePositionList.size();
        size_t index = 0;
        for (; index < listSize; index++)
        {
            if (!HasMatchingTags(m_surfaceWeightsList[index], desiredTags))
            {
                break;
            }
        }

        if (index != listSize)
        {
            size_t next = index + 1;
            for (; next < listSize; ++next)
            {
                if (HasMatchingTags(m_surfaceWeightsList[index], desiredTags))
                {
                    m_surfaceCreatorIdList[index] = m_surfaceCreatorIdList[next];
                    m_surfacePositionList[index] = m_surfacePositionList[next];
                    m_surfaceNormalList[index] = m_surfaceNormalList[next];
                    m_surfaceWeightsList[index] = m_surfaceWeightsList[next];
                    ++index;
                }
            }

            m_surfaceCreatorIdList.resize(index);
            m_surfacePositionList.resize(index);
            m_surfaceNormalList.resize(index);
            m_surfaceWeightsList.resize(index);
        }
    }
}
