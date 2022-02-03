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
    void SurfaceTagWeights::AssignSurfaceTagWeights(const AzFramework::SurfaceData::SurfaceTagWeightList& weights)
    {
        m_weights.clear();
        m_weights.reserve(weights.size());
        for (auto& weight : weights)
        {
            m_weights.emplace(weight.m_surfaceType, weight.m_weight);
        }
    }

    void SurfaceTagWeights::AssignSurfaceTagWeights(const SurfaceTagVector& tags, float weight)
    {
        m_weights.clear();
        m_weights.reserve(tags.size());
        for (auto& tag : tags)
        {
            m_weights[tag] = weight;
        }
    }

    void SurfaceTagWeights::AddSurfaceTagWeight(const AZ::Crc32 tag, const float value)
    {
        m_weights[tag] = value;
    }

    void SurfaceTagWeights::Clear()
    {
        m_weights.clear();
    }

    size_t SurfaceTagWeights::GetSize() const
    {
        return m_weights.size();
    }

    AzFramework::SurfaceData::SurfaceTagWeightList SurfaceTagWeights::GetSurfaceTagWeightList() const
    {
        AzFramework::SurfaceData::SurfaceTagWeightList weights;
        weights.reserve(m_weights.size());
        for (auto& weight : m_weights)
        {
            weights.emplace_back(weight.first, weight.second);
        }
        return weights;
    }

    bool SurfaceTagWeights::operator==(const SurfaceTagWeights& rhs) const
    {
        // If the lists are different sizes, they're not equal.
        if (m_weights.size() != rhs.m_weights.size())
        {
            return false;
        }

        for (auto& weight : m_weights)
        {
            auto rhsWeight = rhs.m_weights.find(weight.first);
            if ((rhsWeight == rhs.m_weights.end()) || (rhsWeight->second != weight.second))
            {
                return false;
            }
        }

        // All the entries matched, and the lists are the same size, so they're equal.
        return true;
    }

    bool SurfaceTagWeights::SurfaceWeightsAreEqual(const AzFramework::SurfaceData::SurfaceTagWeightList& compareWeights) const
    {
        // If the lists are different sizes, they're not equal.
        if (m_weights.size() != compareWeights.size())
        {
            return false;
        }

        for (auto& weight : m_weights)
        {
            auto maskEntry = AZStd::find_if(
                compareWeights.begin(), compareWeights.end(),
                [weight](const AzFramework::SurfaceData::SurfaceTagWeight& compareWeight) -> bool
                {
                    return (weight.first == compareWeight.m_surfaceType) && (weight.second == compareWeight.m_weight);
                });

            // If we didn't find a match, they're not equal.
            if (maskEntry == compareWeights.end())
            {
                return false;
            }
        }

        // All the entries matched, and the lists are the same size, so they're equal.
        return true;
    }

    void SurfaceTagWeights::EnumerateWeights(AZStd::function<bool(AZ::Crc32 tag, float weight)> weightCallback) const
    {
        for (auto& [tag, weight] : m_weights)
        {
            if (!weightCallback(tag, weight))
            {
                break;
            }
        }
    }

    bool SurfaceTagWeights::HasValidTags() const
    {
        for (const auto& sourceTag : m_weights)
        {
            if (sourceTag.first != Constants::s_unassignedTagCrc)
            {
                return true;
            }
        }
        return false;
    }

    bool SurfaceTagWeights::HasMatchingTag(const AZ::Crc32& sampleTag) const
    {
        return m_weights.find(sampleTag) != m_weights.end();
    }

    bool SurfaceTagWeights::HasMatchingTags(const SurfaceTagVector& sampleTags) const
    {
        for (const auto& sampleTag : sampleTags)
        {
            if (HasMatchingTag(sampleTag))
            {
                return true;
            }
        }

        return false;
    }

    bool SurfaceTagWeights::HasMatchingTag(const AZ::Crc32& sampleTag, float weightMin, float weightMax) const
    {
        auto maskItr = m_weights.find(sampleTag);
        return maskItr != m_weights.end() && weightMin <= maskItr->second && weightMax >= maskItr->second;
    }

    bool SurfaceTagWeights::HasMatchingTags(const SurfaceTagVector& sampleTags, float weightMin, float weightMax) const
    {
        for (const auto& sampleTag : sampleTags)
        {
            if (HasMatchingTag(sampleTag, weightMin, weightMax))
            {
                return true;
            }
        }

        return false;
    }




    SurfacePointList::SurfacePointList(AZStd::initializer_list<const AzFramework::SurfaceData::SurfacePoint> surfacePoints)
    {
        ReserveSpace(surfacePoints.size());

        for (auto& point : surfacePoints)
        {
            SurfaceTagWeights weights(point.m_surfaceTags);
            AddSurfacePoint(AZ::EntityId(), point.m_position, point.m_normal, weights);
        }
    }

    void SurfacePointList::AddSurfacePoint(const AZ::EntityId& entityId,
        const AZ::Vector3& position, const AZ::Vector3& normal, const SurfaceTagWeights& masks)
    {
        // When adding a surface point, we'll either merge it with a similar existing point, or else add it in order of
        // decreasing Z, so that our final results are sorted.

        for (size_t index = 0; index < m_surfacePositionList.size(); ++index)
        {
            // (Someday we should add a configurable tolerance for comparison)
            if (m_surfacePositionList[index].IsClose(position) && m_surfaceNormalList[index].IsClose(normal))
            {
                // consolidate points with similar attributes by adding masks/weights to the similar point instead of adding a new one.
                m_surfaceWeightsList[index].AddMaxValueForMasks(masks);
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
        AZStd::function<bool(const AZ::Vector3&, const AZ::Vector3&, const SurfaceData::SurfaceTagWeights&)>
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
        AZStd::function<void(const AZ::Vector3& position, SurfaceData::SurfaceTagWeights& surfaceWeights)> modificationWeightCallback)
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
        point.m_surfaceTags = m_surfaceWeightsList.front().GetSurfaceTagWeightList();

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
            if (!m_surfaceWeightsList[index].HasMatchingTags(desiredTags))
            {
                break;
            }
        }

        if (index != listSize)
        {
            size_t next = index + 1;
            for (; next < listSize; ++next)
            {
                if (m_surfaceWeightsList[index].HasMatchingTags(desiredTags))
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
