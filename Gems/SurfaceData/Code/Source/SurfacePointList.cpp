/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SurfaceData/Utility/SurfaceDataUtility.h>
#include <SurfaceData/SurfacePointList.h>
#include <SurfaceData/SurfaceDataModifierRequestBus.h>

namespace SurfaceData
{
    size_t SurfacePointList::GetInPositionIndexFromPosition(const AZ::Vector3& inPosition) const
    {
        // Given an input position, find the input position index that's associated with it.
        // We'll bias towards always having a position that's the same or further in our input list than before,
        // so we'll do a linear search that starts with the last input position we used, and goes forward (and wraps around)
        // until we've searched them all.
        // Our expectation is that most of the time, we'll only have to compare 0-1 input positions.

        size_t inPositionIndex = m_lastInputPositionIndex;
        [[maybe_unused]] bool foundMatch = false;
        for (size_t indexCounter = 0; indexCounter < m_inputPositions.size(); indexCounter++)
        {
            if (m_inputPositions[inPositionIndex] == inPosition)
            {
                foundMatch = true;
                break;
            }

            inPositionIndex = (inPositionIndex + 1) % m_inputPositions.size();
        }

        AZ_Assert(
            foundMatch,
            "Couldn't find input position: (%0.7f, %0.7f, %0.7f), m_lastInputPositionIndex = %zu, m_inputPositions.size() = %zu",
            inPosition.GetX(), inPosition.GetY(), inPosition.GetZ(), m_lastInputPositionIndex, m_inputPositions.size());

        m_lastInputPositionIndex = inPositionIndex;
        return inPositionIndex;
    }

    size_t SurfacePointList::GetSurfacePointStartIndexFromInPositionIndex(size_t inPositionIndex) const
    {
        // Index to the first output surface point for this input position.
        return inPositionIndex * m_maxSurfacePointsPerInput;
    }

    SurfacePointList::SurfacePointList(AZStd::span<const AzFramework::SurfaceData::SurfacePoint> surfacePoints)
    {
        // Construct and finalize the list with the set of passed-in surface points.
        // This is primarily a convenience for unit tests.
        StartListConstruction(surfacePoints);
        EndListConstruction();
    }

    void SurfacePointList::StartListConstruction(AZStd::span<const AzFramework::SurfaceData::SurfacePoint> surfacePoints)
    {
        // Construct the list with the set of passed-in surface points but don't finalize it.
        // This is primarily a convenience for unit tests that want to test surface modifiers with specific inputs.

        surfacePoints.begin();
        StartListConstruction(AZStd::span<const AZ::Vector3>(&(surfacePoints.begin()->m_position), 1), surfacePoints.size(), {});

        for (auto& point : surfacePoints)
        {
            SurfaceTagWeights weights(point.m_surfaceTags);
            AddSurfacePoint(AZ::EntityId(), point.m_position, point.m_position, point.m_normal, weights);
        }
    }

    void SurfacePointList::StartListConstruction(
        AZStd::span<const AZ::Vector3> inPositions, size_t maxPointsPerInput, AZStd::span<const SurfaceTag> filterTags)
    {
        AZ_Assert(!m_listIsBeingConstructed, "Trying to start list construction on a list currently under construction.");
        AZ_Assert(m_surfacePositionList.empty(), "Trying to reserve space on a list that is already being used.");

        Clear();

        m_listIsBeingConstructed = true;

        // Save off working references to the data we'll need during list construction.
        // These references need to remain valid during construction, but not afterwards.
        m_filterTags = filterTags;
        m_inputPositions = inPositions;
        m_inputPositionSize = inPositions.size();
        m_maxSurfacePointsPerInput = maxPointsPerInput;

        size_t outputReserveSize = inPositions.size() * m_maxSurfacePointsPerInput;

        // Reserve enough space to have one value per input position, and initialize it to 0.
        m_numSurfacePointsPerInput.resize(m_inputPositionSize);

        // Reserve enough space to have maxSurfacePointsPerInput entries per input position, and initialize them all to 0.
        m_sortedSurfacePointIndices.resize(outputReserveSize);

        // Reserve enough space for all our possible output surface points, but don't initialize them.
        m_surfaceCreatorIdList.reserve(outputReserveSize);
        m_surfacePositionList.reserve(outputReserveSize);
        m_surfaceNormalList.reserve(outputReserveSize);
        m_surfaceWeightsList.reserve(outputReserveSize);
    }

    void SurfacePointList::Clear()
    {
        m_listIsBeingConstructed = false;

        m_lastInputPositionIndex = 0;
        m_inputPositionSize = 0;
        m_maxSurfacePointsPerInput = 0;

        m_filterTags = {};
        m_inputPositions = {};

        m_sortedSurfacePointIndices.clear();
        m_numSurfacePointsPerInput.clear();
        m_surfacePositionList.clear();
        m_surfaceNormalList.clear();
        m_surfaceWeightsList.clear();
        m_surfaceCreatorIdList.clear();

        m_surfacePointBounds = AZ::Aabb::CreateNull();
    }

    void SurfacePointList::AddSurfacePoint(
        const AZ::EntityId& entityId, const AZ::Vector3& inPosition,
        const AZ::Vector3& position, const AZ::Vector3& normal, const SurfaceTagWeights& masks)
    {
        AZ_Assert(m_listIsBeingConstructed, "Trying to add surface points to a SurfacePointList that isn't under construction.");

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
            if (m_surfacePositionList[m_sortedSurfacePointIndices[surfacePointInsertIndex]].IsClose(position) &&
                m_surfaceNormalList[m_sortedSurfacePointIndices[surfacePointInsertIndex]].IsClose(normal))
            {
                // consolidate points with similar attributes by adding masks/weights to the similar point instead of adding a new one.
                m_surfaceWeightsList[m_sortedSurfacePointIndices[surfacePointInsertIndex]].AddSurfaceTagWeights(masks);
                return;
            }
            else if (m_surfacePositionList[m_sortedSurfacePointIndices[surfacePointInsertIndex]].GetZ() < position.GetZ())
            {
                break;
            }
        }

        // If we've made it here, we're adding the point, not merging it.

        // Verify we aren't adding more points than expected.
        AZ_Assert(m_numSurfacePointsPerInput[inPositionIndex] < m_maxSurfacePointsPerInput, "Adding too many surface points.");

        // Expand our output AABB to include this point.
        m_surfacePointBounds.AddPoint(position);

        // If this isn't the first output for this input position, shift our sorted indices for this input position to make room for
        // the new entry.
        if (m_numSurfacePointsPerInput[inPositionIndex] > 0)
        {
            size_t startIndex = surfacePointInsertIndex;
            size_t endIndex = surfacePointStartIndex + m_numSurfacePointsPerInput[inPositionIndex];

            AZStd::move_backward(
                m_sortedSurfacePointIndices.begin() + startIndex, m_sortedSurfacePointIndices.begin() + endIndex,
                m_sortedSurfacePointIndices.begin() + endIndex + 1);
        }

        m_numSurfacePointsPerInput[inPositionIndex]++;

        // Insert the new sorted index that references into our storage vectors.
        m_sortedSurfacePointIndices[surfacePointInsertIndex] = m_surfacePositionList.size();

        // Add the new point to the back of our storage vectors.
        m_surfacePositionList.emplace_back(position);
        m_surfaceNormalList.emplace_back(normal);
        m_surfaceWeightsList.emplace_back(masks);
        m_surfaceCreatorIdList.emplace_back(entityId);
    }

    void SurfacePointList::ModifySurfaceWeights(const SurfaceDataRegistryHandle& surfaceModifierHandle)
    {
        AZ_Assert(m_listIsBeingConstructed, "Trying to modify surface weights on a SurfacePointList that isn't under construction.");

        SurfaceDataModifierRequestBus::Event( surfaceModifierHandle,
            &SurfaceDataModifierRequestBus::Events::ModifySurfacePoints,
            m_surfacePositionList, m_surfaceCreatorIdList, m_surfaceWeightsList);
    }

    void SurfacePointList::FilterPoints(AZStd::span<const SurfaceTag> desiredTags)
    {
        AZ_Assert(m_listIsBeingConstructed, "Trying to filter a SurfacePointList that isn't under construction.");

        // Filter out any points that don't match our search tags.
        // This has to be done after the Surface Modifiers have processed the points, not at point insertion time, because
        // Surface Modifiers add tags to existing points.
        // The algorithm below is basically an "erase_if" that's operating across multiple storage vectors and using one level of
        // indirection to keep our sorted indices valid.
        // At some point we might want to consider modifying this to compact the final storage to the minimum needed.
        for (size_t inputIndex = 0; (inputIndex < m_inputPositionSize); inputIndex++)
        {
            size_t surfacePointStartIndex = GetSurfacePointStartIndexFromInPositionIndex(inputIndex);
            size_t listSize = (surfacePointStartIndex + m_numSurfacePointsPerInput[inputIndex]);
            size_t index = surfacePointStartIndex;
            for (; index < listSize; index++)
            {
                if (!m_surfaceWeightsList[m_sortedSurfacePointIndices[index]].HasAnyMatchingTags(desiredTags))
                {
                    break;
                }
            }

            if (index != listSize)
            {
                size_t next = index + 1;
                for (; next < listSize; ++next)
                {
                    if (m_surfaceWeightsList[m_sortedSurfacePointIndices[next]].HasAnyMatchingTags(desiredTags))
                    {
                        m_sortedSurfacePointIndices[index] = AZStd::move(m_sortedSurfacePointIndices[next]);
                        m_surfaceCreatorIdList[m_sortedSurfacePointIndices[index]] =
                            AZStd::move(m_surfaceCreatorIdList[m_sortedSurfacePointIndices[next]]);
                        m_surfacePositionList[m_sortedSurfacePointIndices[index]] =
                            AZStd::move(m_surfacePositionList[m_sortedSurfacePointIndices[next]]);
                        m_surfaceNormalList[m_sortedSurfacePointIndices[index]] =
                            AZStd::move(m_surfaceNormalList[m_sortedSurfacePointIndices[next]]);
                        m_surfaceWeightsList[m_sortedSurfacePointIndices[index]] =
                            AZStd::move(m_surfaceWeightsList[m_sortedSurfacePointIndices[next]]);

                        ++index;
                    }
                }

                m_numSurfacePointsPerInput[inputIndex] = index - surfacePointStartIndex;
            }
        }
    }

    void SurfacePointList::EndListConstruction()
    {
        AZ_Assert(m_listIsBeingConstructed, "Trying to end list construction on a SurfacePointList that isn't under construction.");

        // Now that we've finished adding and modifying points, filter out any points that don't match the filterTags list, if we have one.
        if (!m_filterTags.empty())
        {
            FilterPoints(m_filterTags);
        }

        m_listIsBeingConstructed = false;
        m_inputPositions = {};
        m_filterTags = {};
    }

    bool SurfacePointList::IsEmpty() const
    {
        AZ_Assert(!m_listIsBeingConstructed, "Trying to query a SurfacePointList that's still under construction.");

        return m_surfacePositionList.empty();
    }

    bool SurfacePointList::IsEmpty(size_t inputPositionIndex) const
    {
        AZ_Assert(!m_listIsBeingConstructed, "Trying to query a SurfacePointList that's still under construction.");

        return (m_inputPositionSize == 0) || (m_numSurfacePointsPerInput[inputPositionIndex] == 0);
    }

    size_t SurfacePointList::GetSize() const
    {
        AZ_Assert(!m_listIsBeingConstructed, "Trying to query a SurfacePointList that's still under construction.");

        size_t numValidEntries = 0;

        for (size_t inputIndex = 0; (inputIndex < m_inputPositionSize); inputIndex++)
        {
            numValidEntries += m_numSurfacePointsPerInput[inputIndex];
        }

        return numValidEntries;
    }

    size_t SurfacePointList::GetSize(size_t inputPositionIndex) const
    {
        AZ_Assert(!m_listIsBeingConstructed, "Trying to query a SurfacePointList that's still under construction.");

        return (m_inputPositionSize == 0) ? 0 : (m_numSurfacePointsPerInput[inputPositionIndex]);
    }

    void SurfacePointList::EnumeratePoints(
        size_t inputPositionIndex,
        AZStd::function<bool(const AZ::Vector3&, const AZ::Vector3&, const SurfaceData::SurfaceTagWeights&)>
            pointCallback) const
    {
        AZ_Assert(!m_listIsBeingConstructed, "Trying to query a SurfacePointList that's still under construction.");

        size_t surfacePointStartIndex = GetSurfacePointStartIndexFromInPositionIndex(inputPositionIndex);
        for (size_t index = surfacePointStartIndex;
             (index < (surfacePointStartIndex + m_numSurfacePointsPerInput[inputPositionIndex])); index++)
        {
            if (!pointCallback(
                    m_surfacePositionList[m_sortedSurfacePointIndices[index]], m_surfaceNormalList[m_sortedSurfacePointIndices[index]],
                    m_surfaceWeightsList[m_sortedSurfacePointIndices[index]]))
            {
                break;
            }
        }
    }

    void SurfacePointList::EnumeratePoints(
        AZStd::function<bool(size_t inputPositionIndex, const AZ::Vector3&, const AZ::Vector3&, const SurfaceData::SurfaceTagWeights&)>
            pointCallback) const
    {
        AZ_Assert(!m_listIsBeingConstructed, "Trying to query a SurfacePointList that's still under construction.");

        for (size_t inputIndex = 0; (inputIndex < m_inputPositionSize); inputIndex++)
        {
            size_t surfacePointStartIndex = GetSurfacePointStartIndexFromInPositionIndex(inputIndex);
            for (size_t index = surfacePointStartIndex; (index < (surfacePointStartIndex + m_numSurfacePointsPerInput[inputIndex]));
                 index++)
            {
                if (!pointCallback(
                        inputIndex, m_surfacePositionList[m_sortedSurfacePointIndices[index]],
                        m_surfaceNormalList[m_sortedSurfacePointIndices[index]], m_surfaceWeightsList[m_sortedSurfacePointIndices[index]]))
                {
                    break;
                }
            }
        }
    }

    AzFramework::SurfaceData::SurfacePoint SurfacePointList::GetHighestSurfacePoint([[maybe_unused]] size_t inputPositionIndex) const
    {
        AZ_Assert(!m_listIsBeingConstructed, "Trying to query a SurfacePointList that's still under construction.");

        if (m_numSurfacePointsPerInput[inputPositionIndex] == 0)
        {
            return {};
        }

        size_t surfacePointStartIndex = GetSurfacePointStartIndexFromInPositionIndex(inputPositionIndex);
        AzFramework::SurfaceData::SurfacePoint point;
        point.m_position = m_surfacePositionList[m_sortedSurfacePointIndices[surfacePointStartIndex]];
        point.m_normal = m_surfaceNormalList[m_sortedSurfacePointIndices[surfacePointStartIndex]];
        point.m_surfaceTags = m_surfaceWeightsList[m_sortedSurfacePointIndices[surfacePointStartIndex]].GetSurfaceTagWeightList();

        return point;
    }

}
