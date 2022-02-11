/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/span.h>
#include <AzFramework/SurfaceData/SurfaceData.h>
#include <SurfaceData/SurfaceDataTypes.h>
#include <SurfaceData/SurfaceTag.h>

namespace SurfaceData
{
    //! SurfacePointList stores a collection of surface point data, which consists of positions, normals, and surface tag weights.
    //! This class is specifically designed to be used in the following ways.
    //!
    //! List construction:
    //!   StartListConstruction() - This clears the structure, temporarily holds on to the list of input positions, and preallocates the data.
    //!   AddSurfacePoint() - Add surface points to the list. They're expected to get added in input position order.
    //!   ModifySurfaceWeights() - Modify the surface weights for the set of input points.
    //!   FilterPoints() - Remove any generated surface points that don't fit the filter criteria
    //!   EndListConstruction() - "Freeze" and compact the data.
    //!
    //! List usage:
    //!   
    class SurfacePointList
    {
    public:
        AZ_CLASS_ALLOCATOR(SurfacePointList, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(SurfacePointList, "{DBA02848-2131-4279-BDEF-3581B76AB736}");

        SurfacePointList() = default;
        ~SurfacePointList() = default;

        // ---------- List Construction APIs -------------

        //! Clear the surface point list.
        void Clear();

        //! Constructor for creating a SurfacePointList from a list of SurfacePoint data.
        //! Primarily used as a convenience for unit tests.
        //! @param surfacePoints - A set of SurfacePoint points to store in the SurfacePointList.
        //! Each point that's passed in will be treated as both the input and output position.
        //! The list will be fully constructed and queryable after this runs.
        SurfacePointList(AZStd::initializer_list<const AzFramework::SurfaceData::SurfacePoint> surfacePoints);

        //! Start construction of a SurfacePointList from a list of SurfacePoint data.
        //! Primarily used as a convenience for unit tests.
        //! @param surfacePoints - A set of SurfacePoint points to store in the SurfacePointList.
        //! The list will remain in the "constructing" state after this is called, so it will still be possible to add/modify
        //! points, and EndListConstruction() will still need to be called.
        void StartListConstruction(AZStd::initializer_list<const AzFramework::SurfaceData::SurfacePoint> surfacePoints);

        //! Start construction of a SurfacePointList.
        //! @param inPositions - the list of input positions that will be used to generate this list. This list is expected to remain
        //! valid until EndListConstruction() is called.
        //! @param maxPointsPerInput - the maximum number of potential surface points that will be generated for each input. This is used
        //! for allocating internal structures during list construction and is enforced to be correct.
        //! @param filterTags - optional list of tags to filter the generated surface points by. If this list is provided, every surface
        //! point remaining in the list after construction will contain at least one of these tags. The list is expected to remain
        //! valid until EndListConstruction() is called.
        void StartListConstruction(AZStd::span<const AZ::Vector3> inPositions, size_t maxPointsPerInput,
            AZStd::span<const SurfaceTag> filterTags);

        //! Add a surface point to the list.
        //! To use this method optimally, the points should get added in increasing inPosition index order.
        //! @param entityId - The entity creating the surface point.
        //! @param inPosition - The input position that produced this surface point.
        //! @param position - The position of the surface point.
        //! @param normal - The normal for the surface point.
        //! @param weights - The surface tags and weights for this surface point.
        void AddSurfacePoint(const AZ::EntityId& entityId, const AZ::Vector3& inPosition,
            const AZ::Vector3& position, const AZ::Vector3& normal, const SurfaceTagWeights& weights);

        //! Modify the surface weights for each surface point in the list.
        void ModifySurfaceWeights(
            const AZ::EntityId& currentEntityId,
            AZStd::function<void(const AZ::Vector3& position, SurfaceTagWeights& surfaceWeights)> modificationWeightCallback);

        //! End construction of the SurfacePointList.
        //! After this is called, surface points can no longer be added or modified, and all of the query APIs can start getting used.
        void EndListConstruction();

        // ---------- List Query APIs -------------

        bool IsEmpty() const;

        //! Check if the surface point list is empty.
        //! @return - true if empty, false if it contains points.
        bool IsEmpty(size_t inputPositionIndex) const;

        size_t GetSize() const;

        //! Get the size of the surface point list.
        //! @return - The number of valid points in the list.
        size_t GetSize(size_t inputPositionIndex) const;

        size_t GetInputPositionSize() const
        {
            return m_inputPositionSize;
        }

        //! Enumerate every surface point and call a callback for each point found.
        void EnumeratePoints(
            size_t inputPositionIndex,
            AZStd::function<bool(
                const AZ::Vector3& position, const AZ::Vector3& normal, const SurfaceTagWeights& surfaceWeights)> pointCallback) const;

        //! Enumerate every surface point and call a callback for each point found.
        void EnumeratePoints(
            AZStd::function<bool(
                size_t inputPositionIndex, const AZ::Vector3& position,
                const AZ::Vector3& normal, const SurfaceTagWeights& surfaceWeights)> pointCallback) const;

        //! Get the surface point with the highest Z value.
        AzFramework::SurfaceData::SurfacePoint GetHighestSurfacePoint(size_t inputPositionIndex) const;

        AZ::Aabb GetSurfacePointAabb() const
        {
            return m_surfacePointBounds;
        }

    protected:
        //! Remove any points that don't contain any of the provided surface tags.
        void FilterPoints(AZStd::span<const SurfaceTag> desiredTags);

        size_t GetInPositionIndexFromPosition(const AZ::Vector3& inPosition) const;
        size_t GetSurfacePointStartIndexFromInPositionIndex(size_t inPositionIndex) const;

        AZStd::span<const SurfaceTag> m_filterTags;

        AZStd::span<const AZ::Vector3> m_inputPositions;
        mutable size_t m_lastInputPositionIndex = 0;

        AZStd::vector<size_t> m_numSurfacePointsPerInput;

        // These are kept in separate parallel vectors instead of a single struct so that it's possible to pass just specific data
        // "channels" into other methods as span<> without having to pass the full struct into the span<>. Specifically, we want to be
        // able to pass spans of the positions down through nesting gradient/surface calls.
        // A side benefit is that profiling showed the data access to be faster than packing all the fields into a single struct.
        AZStd::vector<size_t> m_indirectIndex;

        AZStd::vector<AZ::Vector3> m_surfacePositionList;
        AZStd::vector<SurfaceTagWeights> m_surfaceWeightsList;
        AZStd::vector<AZ::EntityId> m_surfaceCreatorIdList;
        AZStd::vector<AZ::Vector3> m_surfaceNormalList;

        AZ::Aabb m_surfacePointBounds = AZ::Aabb::CreateNull();
        size_t m_inputPositionSize = 0;
        size_t m_maxSurfacePointsPerInput = 0;

        bool m_listIsBeingConstructed = false;
    };
}
