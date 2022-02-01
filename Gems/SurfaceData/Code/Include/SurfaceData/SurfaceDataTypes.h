/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_set.h>
#include <SurfaceData/SurfaceTag.h>

namespace SurfaceData
{
    //map of id or crc to contribution factor
    using SurfaceTagWeightMap = AZStd::unordered_map<AZ::Crc32, float>;
    using SurfaceTagNameSet = AZStd::unordered_set<AZStd::string>;
    using SurfaceTagVector = AZStd::vector<SurfaceTag>;

    struct SurfacePoint final
    {
        AZ_CLASS_ALLOCATOR(SurfacePoint, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(SurfacePoint, "{0DC7E720-68D6-47D4-BB6D-B89EF23C5A5C}");

        AZ::EntityId m_entityId;
        AZ::Vector3 m_position;
        AZ::Vector3 m_normal;
        SurfaceTagWeightMap m_masks;
    };

    class SurfacePointList
    {
    public:
        AZ_CLASS_ALLOCATOR(SurfacePointList, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(SurfacePointList, "{DBA02848-2131-4279-BDEF-3581B76AB736}");

        SurfacePointList() = default;
        ~SurfacePointList() = default;

        //! Constructor for creating a SurfacePointList from a list of SurfacePoint data.
        //! Primarily used as a convenience for unit tests.
        //! @param surfacePoints - An initial set of SurfacePoint points to store in the SurfacePointList.
        SurfacePointList(AZStd::initializer_list<const SurfacePoint> surfacePoints);

        //! Add a surface point to the list.
        //! @param surfacePoint - The point to add to the list.
        void AddSurfacePoint(const SurfacePoint& surfacePoint);

        //! Add a surface point to the list.
        //! @param surfacePoint - The point to add to the list. It will be moved, so surfacePoint will no longer be valid after this call.
        void AddSurfacePoint(SurfacePoint&& surfacePoint);

        //! Clear the surface point list.
        void Clear();

        //! Preallocate space in the list based on the maximum number of output points per input point we can generate.
        //! @param maxPointsPerInput - The maximum number of output points per input point.
        void ReserveSpace(size_t maxPointsPerInput);

        //! Check if the surface point list is empty.
        //! @return - true if empty, false if it contains points.
        bool IsEmpty() const;

        //! Get the size of the surface point list.
        //! @return - The number of valid points in the list.
        size_t GetSize() const;

        void EnumeratePoints(AZStd::function<bool(SurfacePoint&)> pointCallback);
        void EnumeratePoints(AZStd::function<bool(const SurfacePoint&)> pointCallback) const;

        const SurfacePoint& GetHighestSurfacePoint() const;

        void FilterPoints(const SurfaceTagVector& desiredTags);
        void SortAndCombineNeighboringPoints();

    protected:

        AZStd::vector<SurfacePoint> m_surfacePointList;

        // Controls whether we combine neighboring points and sort by height when adding points, or as a post-process step.
        bool m_sortAndCombineOnPointInsertion{ true };
    };

    using SurfacePointLists = AZStd::vector<SurfacePointList>;

    struct SurfaceDataRegistryEntry
    {
        AZ::EntityId m_entityId;
        AZ::Aabb m_bounds = AZ::Aabb::CreateNull();
        SurfaceTagVector m_tags;
    };

    using SurfaceDataRegistryHandle = AZ::u32;
    const SurfaceDataRegistryHandle InvalidSurfaceDataRegistryHandle = 0;
}
