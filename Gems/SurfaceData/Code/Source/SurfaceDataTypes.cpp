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
        // Index to the first output surface point for this input position.
        return inPositionIndex * m_maxSurfacePointsPerInput;
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

        for (; surfacePointInsertIndex < (surfacePointStartIndex + m_numSurfacePointsPerInput[inPositionIndex]); ++surfacePointInsertIndex)
        {
            // (Someday we should add a configurable tolerance for comparison)
            if (m_surfacePositionList[m_indirectIndex[surfacePointInsertIndex]].IsClose(position) &&
                m_surfaceNormalList[m_indirectIndex[surfacePointInsertIndex]].IsClose(normal))
            {
                // consolidate points with similar attributes by adding masks/weights to the similar point instead of adding a new one.
                m_surfaceWeightsList[m_indirectIndex[surfacePointInsertIndex]].AddSurfaceTagWeights(masks);
                return;
            }
            else if (m_surfacePositionList[m_indirectIndex[surfacePointInsertIndex]].GetZ() < position.GetZ())
            {
                break;
            }
        }

        AZ_Assert(m_numSurfacePointsPerInput[inPositionIndex] < m_maxSurfacePointsPerInput, "Adding too many surface points.");

        m_surfacePointBounds.AddPoint(position);


        if (m_numSurfacePointsPerInput[inPositionIndex] > 0)
        {
            size_t startIndex = surfacePointInsertIndex;
            size_t endIndex = surfacePointStartIndex + m_numSurfacePointsPerInput[inPositionIndex];

            AZStd::move_backward(&m_indirectIndex[startIndex], &m_indirectIndex[endIndex], &m_indirectIndex[endIndex + 1]);
        }

        m_indirectIndex[surfacePointInsertIndex] = m_surfacePositionList.size();
        m_surfacePositionList.emplace_back(position);
        m_surfaceNormalList.emplace_back(normal);
        m_surfaceWeightsList.emplace_back(masks);
        m_surfaceCreatorIdList.emplace_back(entityId);
        m_numSurfacePointsPerInput[inPositionIndex]++;
    }

    void SurfacePointList::Clear()
    {
        m_lastInputPositionIndex = 0;
        m_inputPositionSize = 0;
        m_maxSurfacePointsPerInput = 0;

        m_inputPositions = {};

        m_indirectIndex.clear();
        m_numSurfacePointsPerInput.clear();
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
        m_maxSurfacePointsPerInput = maxPointsPerInput * 4;

        size_t outputReserveSize = inPositions.size() * m_maxSurfacePointsPerInput;

        m_numSurfacePointsPerInput.resize(m_inputPositionSize);
        m_indirectIndex.resize(outputReserveSize);
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
        return (m_numSurfacePointsPerInput[inputPositionIndex] == 0);
    }

    size_t SurfacePointList::GetSize(size_t inputPositionIndex) const
    {
        return (m_numSurfacePointsPerInput[inputPositionIndex]);
    }

    void SurfacePointList::EnumeratePoints(
        size_t inputPositionIndex, 
        AZStd::function<bool(const AZ::Vector3&, const AZ::Vector3&, const SurfaceData::SurfaceTagWeights&)>
            pointCallback) const
    {
        size_t surfacePointStartIndex = GetSurfacePointStartIndexFromInPositionIndex(inputPositionIndex);
        for (size_t index = surfacePointStartIndex;
             (index < (surfacePointStartIndex + m_numSurfacePointsPerInput[inputPositionIndex])); index++)
        {
            if (!pointCallback(
                    m_surfacePositionList[m_indirectIndex[index]], m_surfaceNormalList[m_indirectIndex[index]],
                    m_surfaceWeightsList[m_indirectIndex[index]]))
            {
                break;
            }
        }
    }

    void SurfacePointList::EnumeratePoints(
        AZStd::function<bool(size_t inputPositionIndex, const AZ::Vector3&, const AZ::Vector3&, const SurfaceData::SurfaceTagWeights&)>
            pointCallback) const
    {
        for (size_t inputIndex = 0; (inputIndex < m_inputPositionSize); inputIndex++)
        {
            size_t surfacePointStartIndex = GetSurfacePointStartIndexFromInPositionIndex(inputIndex);
            for (size_t index = surfacePointStartIndex; (index < (surfacePointStartIndex + m_numSurfacePointsPerInput[inputIndex]));
                 index++)
            {
                if (!pointCallback(
                        inputIndex, m_surfacePositionList[m_indirectIndex[index]], m_surfaceNormalList[m_indirectIndex[index]],
                        m_surfaceWeightsList[m_indirectIndex[index]]))
                {
                    break;
                }
            }
        }
    }

    void SurfacePointList::ModifySurfaceWeights(
        const AZ::EntityId& currentEntityId,
        AZStd::function<void(const AZ::Vector3& position, SurfaceData::SurfaceTagWeights& surfaceWeights)> modificationWeightCallback)
    {
        for (size_t inputIndex = 0; (inputIndex < m_inputPositionSize); inputIndex++)
        {
            size_t surfacePointStartIndex = GetSurfacePointStartIndexFromInPositionIndex(inputIndex);
            for (size_t index = surfacePointStartIndex; (index < (surfacePointStartIndex + m_numSurfacePointsPerInput[inputIndex]));
                 index++)
            {
                if (m_surfaceCreatorIdList[m_indirectIndex[index]] != currentEntityId)
                {
                    modificationWeightCallback(
                        m_surfacePositionList[m_indirectIndex[index]], m_surfaceWeightsList[m_indirectIndex[index]]);
                }
            }
        }
    }

    AzFramework::SurfaceData::SurfacePoint SurfacePointList::GetHighestSurfacePoint([[maybe_unused]] size_t inputPositionIndex) const
    {
        if (m_numSurfacePointsPerInput[inputPositionIndex] == 0)
        {
            return {};
        }

        size_t surfacePointStartIndex = GetSurfacePointStartIndexFromInPositionIndex(inputPositionIndex);
        AzFramework::SurfaceData::SurfacePoint point;
        point.m_position = m_surfacePositionList[m_indirectIndex[surfacePointStartIndex]];
        point.m_normal = m_surfaceNormalList[m_indirectIndex[surfacePointStartIndex]];
        point.m_surfaceTags = m_surfaceWeightsList[m_indirectIndex[surfacePointStartIndex]].GetSurfaceTagWeightList();

        return point;
    }

    void SurfacePointList::FilterPoints(const SurfaceTagVector& desiredTags)
    {
        // Filter out any points that don't match our search tags.
        // This has to be done after the Surface Modifiers have processed the points, not at point insertion time, because
        // Surface Modifiers add tags to existing points.
        for (size_t inputIndex = 0; (inputIndex < m_inputPositionSize); inputIndex++)
        {
            size_t surfacePointStartIndex = GetSurfacePointStartIndexFromInPositionIndex(inputIndex);
            size_t listSize = (surfacePointStartIndex + m_numSurfacePointsPerInput[inputIndex]);
            size_t index = surfacePointStartIndex;
            for (; index < listSize; index++)
            {
                if (!m_surfaceWeightsList[m_indirectIndex[index]].HasAnyMatchingTags(desiredTags))
                {
                    break;
                }
            }

            if (index != listSize)
            {
                size_t next = index + 1;
                for (; next < listSize; ++next)
                {
                    if (m_surfaceWeightsList[m_indirectIndex[index]].HasAnyMatchingTags(desiredTags))
                    {
                        m_indirectIndex[index] = AZStd::move(m_indirectIndex[next]);
                        m_surfaceCreatorIdList[m_indirectIndex[index]] = AZStd::move(m_surfaceCreatorIdList[m_indirectIndex[next]]);
                        m_surfacePositionList[m_indirectIndex[index]] = AZStd::move(m_surfacePositionList[m_indirectIndex[next]]);
                        m_surfaceNormalList[m_indirectIndex[index]] = AZStd::move(m_surfaceNormalList[m_indirectIndex[next]]);
                        m_surfaceWeightsList[m_indirectIndex[index]] = AZStd::move(m_surfaceWeightsList[m_indirectIndex[next]]);

                        m_numSurfacePointsPerInput[inputIndex]--;

                        ++index;
                    }
                }

            }
        }
    }
}
