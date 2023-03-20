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
    //!   Any of the query APIs can be used in any order after the list has finished being constructed.
    //!
    //! This class is specifically designed around the usage patterns described above to minimize the amount of allocations and data
    //! shifting that needs to occur. There are some tricky bits that need to be accounted for:
    //! * Tracking which input positions each output point belongs to.
    //! * Support for merging similar surface points together, which causes us to keep them sorted for easier comparisons.
    //! * Each surface provider will add points in input position order, but we call each provider separately, so the added points will
    //!   show up like (0, 1, 2, 3), (0, 1, 3), (0, 0, 1, 2, 3), etc. We don't want to call each surface provider per-point, because that
    //!   incurs a lot of avoidable overhead in each provider.
    //! * Output points get optionally filtered out at the very end if they don't match any of the filter tags passed in.
    //! 
    //! The solution is that we always add new surface point data to the end of their respective vectors, but we also keep a helper
    //! structure that's a list of lists of sorted indices. We can incrementally re-sort the indices quickly without having to shift
    //! all the surface point data around.
    class SurfacePointList
    {
    public:
        AZ_CLASS_ALLOCATOR(SurfacePointList, AZ::SystemAllocator);
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
        SurfacePointList(AZStd::span<const AzFramework::SurfaceData::SurfacePoint> surfacePoints);

        //! Start construction of a SurfacePointList from a list of SurfacePoint data.
        //! Primarily used as a convenience for unit tests.
        //! @param surfacePoints - A set of SurfacePoint points to store in the SurfacePointList.
        //! The list will remain in the "constructing" state after this is called, so it will still be possible to add/modify
        //! points, and EndListConstruction() will still need to be called.
        void StartListConstruction(AZStd::span<const AzFramework::SurfaceData::SurfacePoint> surfacePoints);

        //! Start construction of a SurfacePointList.
        //! @param inPositions - the list of input positions that will be used to generate this list. This list is expected to remain
        //! valid until EndListConstruction() is called.
        //! @param maxPointsPerInput - the maximum number of potential surface points that will be generated for each input. This is used
        //! for allocating internal structures during list construction and is enforced to be correct.
        //! @param filterTags - optional list of tags to filter the generated surface points by. If this list is provided, every surface
        //! point remaining in the list after construction will contain at least one of these tags. If the list is empty, all generated
        //! points will remain in the list. The filterTags list is expected to remain valid until EndListConstruction() is called.
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
        //! @param surfaceModifierHandle - The handle to the surface modifier that will modify the surface weights.
        void ModifySurfaceWeights(const SurfaceDataRegistryHandle& surfaceModifierHandle);

        //! End construction of the SurfacePointList.
        //! After this is called, surface points can no longer be added or modified, and all of the query APIs can start getting used.
        void EndListConstruction();

        // ---------- List Query APIs -------------

        //! Return whether or not the entire surface point list is empty.
        //! @return True if empty, false if it contains any points.
        bool IsEmpty() const;

        //! Return whether or not a given input position index has any output points associated with it.
        //! @param inputPositionIndex - The input position to look for output points for.
        //! @return True if empty, false if there is at least one output point associated with the input position.
        bool IsEmpty(size_t inputPositionIndex) const;

        //! Return the total number of output points generated.
        //! @return The total number of output points generated.
        size_t GetSize() const;

        //! Return the total number of output points generated from a specific input position index.
        //! @param inputPositionIndex - The input position to look for output points for.
        //! @return The total number of output points generated from a specific input position index.
        size_t GetSize(size_t inputPositionIndex) const;

        //! Return the total number of input positions.
        //! Normally the caller would already be expected to know this, but in the case of using region-based queries, the number of
        //! input positions might not be entirely obvious.
        //! @return The total number of input positions used to generate the outputs.
        size_t GetInputPositionSize() const
        {
            return m_inputPositionSize;
        }

        //! Enumerate every surface point and call a callback for each point found.
        //! Note: There is no guaranteed order to which the points will be enumerated.
        //! @pointCallback - The method to call with each surface point.
        void EnumeratePoints(
            AZStd::function<bool(
                size_t inputPositionIndex, const AZ::Vector3& position, const AZ::Vector3& normal, const SurfaceTagWeights& surfaceWeights)>
                pointCallback) const;

        //! Enumerate every surface point for a given input position and call a callback for each point found.
        //! Note: There is no guaranteed order to which the points will be enumerated.
        //! @inputPositionIndex - The input position to get the outputs for.
        //! @pointCallback - The method to call with each surface point.
        void EnumeratePoints(
            size_t inputPositionIndex,
            AZStd::function<bool(
                const AZ::Vector3& position, const AZ::Vector3& normal, const SurfaceTagWeights& surfaceWeights)> pointCallback) const;

        //! Get the surface point with the highest Z value for a given input position.
        //! @inputPositionIndex - The input position to get the highest surface point for.
        //! @return The surface point with the highest Z value for the given input position.
        AzFramework::SurfaceData::SurfacePoint GetHighestSurfacePoint(size_t inputPositionIndex) const;

        //! Get the AABB that encapsulates all of the generated output surface points.
        //! @return The AABB surrounding all the output surface points.
        AZ::Aabb GetSurfacePointAabb() const
        {
            return m_surfacePointBounds;
        }

    protected:
        // Remove any output surface points that don't contain any of the provided surface tags.
        void FilterPoints(AZStd::span<const SurfaceTag> desiredTags);

        // Get the input position index associated with a specific input position.
        size_t GetInPositionIndexFromPosition(const AZ::Vector3& inPosition) const;

        // Get the first entry in the sortedSurfacePointIndices list for the given input position index.
        size_t GetSurfacePointStartIndexFromInPositionIndex(size_t inPositionIndex) const;

        // During list construction, keep track of the tags to filter the output points to.
        // These will be used at the end of list construction to remove any output points that don't contain any of these tags.
        // (If the list is empty, all output points will be retained)
        AZStd::span<const SurfaceTag> m_filterTags;

        // During list construction, keep track of all the input positions that we'll generate outputs for.
        // Note that after construction is complete, we'll only know how *many* input positions, but not their values.
        // This keeps us from copying data that the caller should already have. We can't assume the lifetime of that data though,
        // so we won't hold on to the span<> after construction.
        AZStd::span<const AZ::Vector3> m_inputPositions;

        // The total number of input positions that we have. We keep this value separately so that we can still know the quantity
        // after list construction when our m_inputPositions span<> has become invalid.
        size_t m_inputPositionSize = 0;

        // The last input position index that we used when adding points.
        // This is used by GetInPositionIndexFromPosition() as an optimization to reduce search times for converting input positions
        // to indices without needing to construct a separate search structure.
        // Because we know surface points will get added in input position order, we'll always start looking for our next input position
        // with the last one we used.
        mutable size_t m_lastInputPositionIndex = 0;

        // This list is the size of m_inputPositions.size() and contains the number of output surface points that we've generated for
        // each input point.
        AZStd::vector<size_t> m_numSurfacePointsPerInput;

        // The AABB surrounding all the surface points. We build this up incrementally as we add each surface point into the list.
        AZ::Aabb m_surfacePointBounds = AZ::Aabb::CreateNull();

        // The maximum number of output points that can be generated for each input.
        size_t m_maxSurfacePointsPerInput = 0;

        // State tracker to determine whether or not the list is currently under construction.
        // This is used to verify that the construction APIs are only used during construction, and the query APIs are only used
        // after construction is complete.
        bool m_listIsBeingConstructed = false;

        // List of lists that's used to index into our storage vectors for all the surface point data.
        // The surface points are stored sequentially in creation order in the storage vectors.
        // This should be thought of as a nested array - m_sortedSurfacePointIndices[m_inputPositionSize][m_maxSurfacePointsPerInput].
        // For each input position, the list of indices are kept sorted in decreasing Z order.
        AZStd::vector<size_t> m_sortedSurfacePointIndices;

        // Storage vectors for keeping track of all the created surface point data.
        // These are kept in separate parallel vectors instead of a single struct so that it's possible to pass just specific data
        // "channels" into other methods as span<> without having to pass the full struct into the span<>. Specifically, we want to be
        // able to pass spans of the positions down through nesting gradient/surface calls.
        AZStd::vector<AZ::Vector3> m_surfacePositionList;
        AZStd::vector<AZ::Vector3> m_surfaceNormalList;
        AZStd::vector<SurfaceTagWeights> m_surfaceWeightsList;
        AZStd::vector<AZ::EntityId> m_surfaceCreatorIdList;
    };
}
