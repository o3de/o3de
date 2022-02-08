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
        for (auto& weight : weights)
        {
            AddSurfaceTagWeight(weight.m_surfaceType, weight.m_weight);
        }
    }

    void SurfaceTagWeights::AssignSurfaceTagWeights(const SurfaceTagVector& tags, float weight)
    {
        m_weights.clear();
        for (auto& tag : tags)
        {
            AddSurfaceTagWeight(tag.operator AZ::Crc32(), weight);
        }
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

        // The lists are stored in sorted order, so we can compare every entry in order for equivalence.
        return (m_weights == rhs.m_weights);
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
                    return ((weight.first == compareWeight.m_surfaceType) && (weight.second == compareWeight.m_weight));
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
        for (const auto& weight : m_weights)
        {
            if (weight.first != Constants::s_unassignedTagCrc)
            {
                return true;
            }
        }
        return false;
    }

    bool SurfaceTagWeights::HasMatchingTag(AZ::Crc32 sampleTag) const
    {
        return FindTag(sampleTag) != m_weights.end();
    }

    bool SurfaceTagWeights::HasAnyMatchingTags(const SurfaceTagVector& sampleTags) const
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

    bool SurfaceTagWeights::HasMatchingTag(AZ::Crc32 sampleTag, float weightMin, float weightMax) const
    {
        auto weightEntry = FindTag(sampleTag);
        return weightEntry != m_weights.end() && weightMin <= weightEntry->second && weightMax >= weightEntry->second;
    }

    bool SurfaceTagWeights::HasAnyMatchingTags(const SurfaceTagVector& sampleTags, float weightMin, float weightMax) const
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

    const AZStd::pair<AZ::Crc32, float>* SurfaceTagWeights::FindTag(AZ::Crc32 tag) const
    {
        for (auto weightItr = m_weights.begin(); weightItr != m_weights.end(); ++weightItr)
        {
            if (weightItr->first == tag)
            {
                // Found the tag, return a pointer to the entry.
                return weightItr;
            }
            else if (weightItr->first > tag)
            {
                // Our list is stored in sorted order by surfaceType, so early-out if our values get too high.
                break;
            }
        }

        // The tag wasn't found, so return end().
        return m_weights.end();
    }


    SurfacePointList::SurfacePointList(AZStd::initializer_list<const AzFramework::SurfaceData::SurfacePoint> surfacePoints)
    {
        StartListQuery({ { surfacePoints.begin()->m_position } }, surfacePoints.size());

        for (auto& point : surfacePoints)
        {
            SurfaceTagWeights weights(point.m_surfaceTags);
            AddSurfacePoint(AZ::EntityId(), point.m_position, point.m_position, point.m_normal, weights);
        }

        EndListQuery();
    }

    size_t SurfacePointList::GetInPositionIndexFromPosition(const AZ::Vector3& inPosition) const
    {
        size_t inPositionIndex = m_lastInputPositionIndex;
        bool foundMatch = false;
        for (size_t indexCounter = 0; indexCounter < m_inputPositions.size(); indexCounter++)
        {
            if (m_inputPositions[inPositionIndex] == inPosition)
            {
                foundMatch = true;
                break;
            }

            inPositionIndex = (inPositionIndex + 1) % m_inputPositions.size();
        }

        AZ_Assert(foundMatch, "Couldn't find input position!");
        m_lastInputPositionIndex = inPositionIndex;
        return inPositionIndex;
    }

    size_t SurfacePointList::GetSurfacePointStartIndexFromInPositionIndex(size_t inPositionIndex) const
    {
        // Find the first SurfacePoint that either matches the inPosition, or that starts the range for the next inPosition after this one.
        if (m_inputPositionIndex.empty() || 
            (m_inputPositionIndex[m_sortedOutputIndexList[m_lastSurfacePointStartIndex]] > inPositionIndex))
        {
            m_lastSurfacePointStartIndex = 0;
        }

        for (; m_lastSurfacePointStartIndex < m_inputPositionIndex.size(); m_lastSurfacePointStartIndex++)
        {
            if (m_inputPositionIndex[m_sortedOutputIndexList[m_lastSurfacePointStartIndex]] >= inPositionIndex)
            {
                break;
            }
        }

        return m_lastSurfacePointStartIndex;
    }


    void SurfacePointList::AddSurfacePoint(
        const AZ::EntityId& entityId, const AZ::Vector3& inPosition,
        const AZ::Vector3& position, const AZ::Vector3& normal, const SurfaceTagWeights& masks)
    {

        // Find the inPositionIndex that matches the inPosition.
        size_t inPositionIndex = GetInPositionIndexFromPosition(inPosition);

        // Find the first SurfacePoint that either matches the inPosition, or that starts the range for the next inPosition after this one.
        size_t surfacePointStartIndex = GetSurfacePointStartIndexFromInPositionIndex(inPositionIndex);

        // When adding a surface point, we'll either merge it with a similar existing point, or else add it in order of
        // decreasing Z, so that our final results are sorted.
        size_t surfacePointInsertIndex = surfacePointStartIndex;

        if ((surfacePointInsertIndex < m_inputPositionIndex.size()) &&
            m_inputPositionIndex[m_sortedOutputIndexList[surfacePointStartIndex]] == inPositionIndex)
        {
            for (; (surfacePointInsertIndex < m_inputPositionIndex.size()) &&
                 (m_inputPositionIndex[m_sortedOutputIndexList[surfacePointInsertIndex]] == inPositionIndex);
                 ++surfacePointInsertIndex)
            {
                // (Someday we should add a configurable tolerance for comparison)
                if (m_surfacePositionList[m_sortedOutputIndexList[surfacePointInsertIndex]].IsClose(position) &&
                    m_surfaceNormalList[m_sortedOutputIndexList[surfacePointInsertIndex]].IsClose(normal))
                {
                    // consolidate points with similar attributes by adding masks/weights to the similar point instead of adding a new one.
                    m_surfaceWeightsList[m_sortedOutputIndexList[surfacePointInsertIndex]].AddSurfaceTagWeights(masks);
                    return;
                }
                else if (m_surfacePositionList[m_sortedOutputIndexList[surfacePointInsertIndex]].GetZ() < position.GetZ())
                {
                    break;
                }
            }
        }

        m_surfacePointBounds.AddPoint(position);
        m_sortedOutputIndexList.insert(m_sortedOutputIndexList.begin() + surfacePointInsertIndex, m_inputPositionIndex.size());
        m_inputPositionIndex.emplace_back(inPositionIndex);
        m_surfacePositionList.emplace_back(position);
        m_surfaceNormalList.emplace_back(normal);
        m_surfaceWeightsList.emplace_back(masks);
        m_surfaceCreatorIdList.emplace_back(entityId);
    }

    void SurfacePointList::Clear()
    {
        m_lastInputPositionIndex = 0;
        m_lastSurfacePointStartIndex = 0;
        m_inputPositionSize = 0;

        m_inputPositions = {};
        m_inputPositionIndex.clear();

        m_sortedOutputIndexList.clear();
        m_surfacePositionList.clear();
        m_surfaceNormalList.clear();
        m_surfaceWeightsList.clear();
        m_surfaceCreatorIdList.clear();

        m_surfacePointBounds = AZ::Aabb::CreateNull();
    }

    void SurfacePointList::StartListQuery(AZStd::span<const AZ::Vector3> inPositions, size_t maxPointsPerInput)
    {
        AZ_Assert(m_surfacePositionList.empty(), "Trying to reserve space on a list that is already being used.");

        Clear();

        m_inputPositions = inPositions;
        m_inputPositionSize = inPositions.size();

        size_t outputReserveSize = inPositions.size() * maxPointsPerInput;

        m_sortedOutputIndexList.reserve(outputReserveSize);
        m_inputPositionIndex.reserve(outputReserveSize);
        m_surfaceCreatorIdList.reserve(outputReserveSize);
        m_surfacePositionList.reserve(outputReserveSize);
        m_surfaceNormalList.reserve(outputReserveSize);
        m_surfaceWeightsList.reserve(outputReserveSize);
    }

    void SurfacePointList::EndListQuery()
    {
        m_inputPositions = {};
    }

    bool SurfacePointList::IsEmpty(size_t inputPositionIndex) const
    {
        size_t surfacePointStartIndex = GetSurfacePointStartIndexFromInPositionIndex(inputPositionIndex);
        return ((m_surfacePositionList.size() <= surfacePointStartIndex) ||
            (m_inputPositionIndex[m_sortedOutputIndexList[surfacePointStartIndex]] != inputPositionIndex));
    }

    size_t SurfacePointList::GetSize(size_t inputPositionIndex) const
    {
        size_t surfacePointStartIndex = GetSurfacePointStartIndexFromInPositionIndex(inputPositionIndex);
        if ((m_surfacePositionList.size() <= surfacePointStartIndex) ||
            (m_inputPositionIndex[m_sortedOutputIndexList[surfacePointStartIndex]] != inputPositionIndex))
        {
            return 0;
        }

        size_t outputListSize = 0;
        for (size_t index = surfacePointStartIndex;
             (index < m_inputPositionIndex.size()) && (m_inputPositionIndex[m_sortedOutputIndexList[index]] == inputPositionIndex); index++)
        {
            outputListSize++;
        }

        return outputListSize;
    }

    void SurfacePointList::EnumeratePoints(
        size_t inputPositionIndex, 
        AZStd::function<bool(const AZ::Vector3&, const AZ::Vector3&, const SurfaceData::SurfaceTagWeights&)>
            pointCallback) const
    {
        size_t surfacePointStartIndex = GetSurfacePointStartIndexFromInPositionIndex(inputPositionIndex);
        if ((m_surfacePositionList.size() <= surfacePointStartIndex) ||
            (m_inputPositionIndex[m_sortedOutputIndexList[surfacePointStartIndex]] != inputPositionIndex))
        {
            return;
        }

        for (size_t index = surfacePointStartIndex;
             (index < m_inputPositionIndex.size()) && (m_inputPositionIndex[m_sortedOutputIndexList[index]] == inputPositionIndex); index++)
        {
            if (!pointCallback(
                    m_surfacePositionList[m_sortedOutputIndexList[index]], m_surfaceNormalList[m_sortedOutputIndexList[index]],
                    m_surfaceWeightsList[m_sortedOutputIndexList[index]]))
            {
                break;
            }
        }
    }

    void SurfacePointList::EnumeratePoints(
        AZStd::function<bool(size_t inputPositionIndex, const AZ::Vector3&, const AZ::Vector3&, const SurfaceData::SurfaceTagWeights&)>
            pointCallback) const
    {
        for (size_t index = 0; (index < m_inputPositionIndex.size()); index++)
        {
            if (!pointCallback(
                    m_inputPositionIndex[m_sortedOutputIndexList[index]], m_surfacePositionList[m_sortedOutputIndexList[index]],
                    m_surfaceNormalList[m_sortedOutputIndexList[index]], m_surfaceWeightsList[m_sortedOutputIndexList[index]]))
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
            if (m_surfaceCreatorIdList[m_sortedOutputIndexList[index]] != currentEntityId)
            {
                modificationWeightCallback(
                    m_surfacePositionList[m_sortedOutputIndexList[index]], m_surfaceWeightsList[m_sortedOutputIndexList[index]]);
            }
        }
    }

    AzFramework::SurfaceData::SurfacePoint SurfacePointList::GetHighestSurfacePoint([[maybe_unused]] size_t inputPositionIndex) const
    {
        size_t surfacePointStartIndex = GetSurfacePointStartIndexFromInPositionIndex(inputPositionIndex);
        if ((m_surfacePositionList.size() <= surfacePointStartIndex) ||
            (m_inputPositionIndex[m_sortedOutputIndexList[surfacePointStartIndex]] != inputPositionIndex))
        {
            return {};
        }

        AzFramework::SurfaceData::SurfacePoint point;
        point.m_position = m_surfacePositionList[m_sortedOutputIndexList[surfacePointStartIndex]];
        point.m_normal = m_surfaceNormalList[m_sortedOutputIndexList[surfacePointStartIndex]];
        point.m_surfaceTags = m_surfaceWeightsList[m_sortedOutputIndexList[surfacePointStartIndex]].GetSurfaceTagWeightList();

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
            if (!m_surfaceWeightsList[index].HasAnyMatchingTags(desiredTags))
            {
                break;
            }
        }

        if (index != listSize)
        {
            size_t next = index + 1;
            for (; next < listSize; ++next)
            {
                if (m_surfaceWeightsList[index].HasAnyMatchingTags(desiredTags))
                {
                    m_sortedOutputIndexList[index] = m_sortedOutputIndexList[next];
                    m_inputPositionIndex[index] = m_inputPositionIndex[next];
                    m_surfaceCreatorIdList[index] = m_surfaceCreatorIdList[next];
                    m_surfacePositionList[index] = m_surfacePositionList[next];
                    m_surfaceNormalList[index] = m_surfaceNormalList[next];
                    m_surfaceWeightsList[index] = m_surfaceWeightsList[next];
                    ++index;
                }
            }

            m_sortedOutputIndexList.resize(index);
            m_inputPositionIndex.resize(index);
            m_surfaceCreatorIdList.resize(index);
            m_surfacePositionList.resize(index);
            m_surfaceNormalList.resize(index);
            m_surfaceWeightsList.resize(index);
        }
    }
}
