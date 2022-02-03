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
#include <AzCore/std/containers/unordered_set.h>
#include <AzFramework/SurfaceData/SurfaceData.h>
#include <SurfaceData/SurfaceTag.h>

namespace SurfaceData
{
    //map of id or crc to contribution factor
    using SurfaceTagNameSet = AZStd::unordered_set<AZStd::string>;
    using SurfaceTagVector = AZStd::vector<SurfaceTag>;

    class SurfaceTagWeights
    {
    public:
        SurfaceTagWeights() = default;

        SurfaceTagWeights(const AzFramework::SurfaceData::SurfaceTagWeightList& weights)
        {
            AssignSurfaceTagWeights(weights);
        }

        void AssignSurfaceTagWeights(const AzFramework::SurfaceData::SurfaceTagWeightList& weights);
        void AssignSurfaceTagWeights(const SurfaceTagVector& tags, float weight);

        void AddSurfaceTagWeight(const AZ::Crc32 tag, const float value);

        void AddMaxValueForMasks(const AZ::Crc32 tag, const float value)
        {
            const auto maskItr = m_weights.find(tag);
            const float valueOld = maskItr != m_weights.end() ? maskItr->second : 0.0f;
            m_weights[tag] = AZ::GetMax(value, valueOld);
        }

        void AddMaxValueForMasks(const SurfaceTagVector& tags, const float value)
        {
            for (const auto& tag : tags)
            {
                AddMaxValueForMasks(tag, value);
            }
        }

        void AddMaxValueForMasks(const SurfaceTagWeights& inMasks)
        {
            for (const auto& inMask : inMasks.m_weights)
            {
                AddMaxValueForMasks(inMask.first, inMask.second);
            }
        }

        bool operator==(const SurfaceTagWeights& rhs) const;
        AZ_FORCE_INLINE bool operator!=(const SurfaceTagWeights& rhs) const
        {
            return !(*this == rhs);
        }

        bool SurfaceWeightsAreEqual(const AzFramework::SurfaceData::SurfaceTagWeightList& compareWeights) const;

        void Clear();

        size_t GetSize() const;
        AzFramework::SurfaceData::SurfaceTagWeightList GetSurfaceTagWeightList() const;
        void EnumerateWeights(AZStd::function<bool(AZ::Crc32, float)> weightCallback) const;

        bool HasValidTags() const;
        bool HasMatchingTag(const AZ::Crc32& sampleTag) const;
        bool HasMatchingTags(const SurfaceTagVector& sampleTags) const;
        bool HasMatchingTag(const AZ::Crc32& sampleTag, float valueMin, float valueMax) const;
        bool HasMatchingTags(const SurfaceTagVector& sampleTags, float valueMin, float valueMax) const;

    private:
        AZStd::unordered_map<AZ::Crc32, float> m_weights;
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
        SurfacePointList(AZStd::initializer_list<const AzFramework::SurfaceData::SurfacePoint> surfacePoints);

        //! Add a surface point to the list.
        void AddSurfacePoint(const AZ::EntityId& entityId,
            const AZ::Vector3& position, const AZ::Vector3& normal, const SurfaceTagWeights& weights);

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

        void EnumeratePoints(AZStd::function<bool(const AZ::Vector3&, const AZ::Vector3&, const SurfaceTagWeights&)> pointCallback) const;

        void ModifySurfaceWeights(
            const AZ::EntityId& currentEntityId,
            AZStd::function<void(const AZ::Vector3&, SurfaceTagWeights&)> modificationWeightCallback);

        AzFramework::SurfaceData::SurfacePoint GetHighestSurfacePoint() const;

        void FilterPoints(const SurfaceTagVector& desiredTags);

    protected:
        // These are kept in separate parallel vectors instead of a single struct so that it's possible to pass just specific data
        // "channels" into other methods as span<> without having to pass the full struct into the span<>. Specifically, we want to be
        // able to pass spans of the positions down through nesting gradient/surface calls.
        // A side benefit is that profiling showed the data access to be faster than packing all the fields into a single struct.
        AZStd::vector<AZ::EntityId> m_surfaceCreatorIdList;
        AZStd::vector<AZ::Vector3> m_surfacePositionList;
        AZStd::vector<AZ::Vector3> m_surfaceNormalList;
        AZStd::vector<SurfaceTagWeights> m_surfaceWeightsList;

        AZ::Aabb m_pointBounds = AZ::Aabb::CreateNull();
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
