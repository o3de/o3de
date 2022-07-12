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
#include <AzCore/std/containers/unordered_set.h>
#include <AzFramework/SurfaceData/SurfaceData.h>
#include <SurfaceData/SurfaceTag.h>

namespace SurfaceData
{
    //map of id or crc to contribution factor
    using SurfaceTagNameSet = AZStd::unordered_set<AZStd::string>;
    using SurfaceTagVector = AZStd::vector<SurfaceTag>;
    using SurfaceTagSet = AZStd::unordered_set<SurfaceTag>;

    //! SurfaceTagWeights stores a collection of surface tags and weights.
    //! A surface tag can only appear once in the collection. Attempting to add it multiple times will always preserve the
    //! highest weight value.
    class SurfaceTagWeights
    {
    public:
        SurfaceTagWeights() = default;

        //! Construct a collection of SurfaceTagWeights from the given SurfaceTagWeightList.
        //! @param weights - The list of weights to assign to the new instance.
        SurfaceTagWeights(const AzFramework::SurfaceData::SurfaceTagWeightList& weights)
        {
            AssignSurfaceTagWeights(weights);
        }

        //! Replace the existing surface tag weights with the given set.
        //! @param weights - The list of weights to assign to this instance.
        void AssignSurfaceTagWeights(const AzFramework::SurfaceData::SurfaceTagWeightList& weights);

        //! Replace the existing surface tag weights with the given set.
        //! @param tags - The list of tags to assign to this instance.
        //! @param weight - The weight to assign to each tag.
        void AssignSurfaceTagWeights(const SurfaceTagVector& tags, float weight);

        //! Add a surface tag weight to this collection. If the tag already exists, the higher weight will be preserved.
        //! (This method is intentionally inlined for its performance impact)
        //! @param tag - The surface tag.
        //! @param weight - The surface tag weight.
        void AddSurfaceTagWeight(const AZ::Crc32 tag, const float weight)
        {
            for (auto weightItr = m_weights.begin(); weightItr != m_weights.end(); ++weightItr)
            {
                // Since we need to scan for duplicate surface types, store the entries sorted by surface type so that we can
                // early-out once we pass the location for the entry instead of always searching every entry.
                if (weightItr->m_surfaceType > tag)
                {
                    if (m_weights.size() != AzFramework::SurfaceData::Constants::MaxSurfaceWeights)
                    {
                        // We didn't find the surface type, so add the new entry in sorted order.
                        m_weights.insert(weightItr, { tag, weight });
                    }
                    else
                    {
                        AZ_Assert(false, "SurfaceTagWeights has reached max capacity, it cannot add a new tag / weight.");
                    }
                    return;
                }
                else if (weightItr->m_surfaceType == tag)
                {
                    // We found the surface type, so just keep the higher of the two weights.
                    weightItr->m_weight = AZ::GetMax(weight, weightItr->m_weight);
                    return;
                }
            }

            // We didn't find the surface weight, and the sort order for it is at the end, so add it to the back of the list.
            if (m_weights.size() != AzFramework::SurfaceData::Constants::MaxSurfaceWeights)
            {
                m_weights.emplace_back(tag, weight);
            }
            else
            {
                AZ_Assert(false, "SurfaceTagWeights has reached max capacity, it cannot add a new tag / weight.");
            }
        }

        //! Add surface tags and weights to this collection. If a tag already exists, the higher weight will be preserved.
        //! (This method is intentionally inlined for its performance impact)
        //! @param tags - The surface tags to replace/add.
        //! @param weight - The surface tag weight to use for each tag.
        void AddSurfaceTagWeights(const SurfaceTagVector& tags, const float weight)
        {
            for (const auto& tag : tags)
            {
                AddSurfaceTagWeight(tag, weight);
            }
        }

        //! Add surface tags and weights to this collection. If a tag already exists, the higher weight will be preserved.
        //! (This method is intentionally inlined for its performance impact)
        //! @param weights - The surface tags and weights to replace/add.
        void AddSurfaceTagWeights(const SurfaceTagWeights& weights)
        {
            for (const auto& [tag, weight] : weights.m_weights)
            {
                AddSurfaceTagWeight(tag, weight);
            }
        }

        //! Equality comparison operator for SurfaceTagWeights.
        bool operator==(const SurfaceTagWeights& rhs) const;

        //! Inequality comparison operator for SurfaceTagWeights.
        bool operator!=(const SurfaceTagWeights& rhs) const
        {
            return !(*this == rhs);
        }

        //! Compares a SurfaceTagWeightList with a SurfaceTagWeights instance to look for equality.
        //! They will be equal if they have the exact same set of tags and weights.
        //! @param compareWeights - the set of weights to compare against.
        bool SurfaceWeightsAreEqual(const AzFramework::SurfaceData::SurfaceTagWeightList& compareWeights) const;

        //! Clear the surface tag weight collection.
        void Clear();

        //! Get the size of the surface tag weight collection.
        //! @return The size of the collection.
        size_t GetSize() const;

        //! Get the collection of surface tag weights as a SurfaceTagWeightList.
        //! @return SurfaceTagWeightList containing the same tags and weights as this collection.
        AzFramework::SurfaceData::SurfaceTagWeightList GetSurfaceTagWeightList() const;

        //! Enumerate every tag and weight and call a callback for each one found.
        //! Callback params:
        //!     AZ::Crc32 - The surface tag.
        //!     float - The surface tag weight.
        //!     return - true to keep enumerating, false to stop.
        //! @param weightCallback - the callback to use for each surface tag / weight found.
        void EnumerateWeights(AZStd::function<bool(AZ::Crc32 tag, float weight)> weightCallback) const;

        //! Check to see if the collection has any valid tags stored within it.
        //! A tag of "Unassigned" is considered an invalid tag.
        //! @return True if there is at least one valid tag, false if there isn't.
        bool HasValidTags() const;

        //! Check to see if the collection contains the given tag.
        //! @param sampleTag - The tag to look for.
        //! @return True if the tag is found, false if it isn't.
        bool HasMatchingTag(AZ::Crc32 sampleTag) const;

        //! Check to see if the collection contains the given tag with the given weight range.
        //! The range check is inclusive on both sides of the range: [weightMin, weightMax]
        //! @param sampleTag - The tag to look for.
        //! @param weightMin - The minimum weight for this tag.
        //! @param weightMax - The maximum weight for this tag.
        //! @return True if the tag is found, false if it isn't.
        bool HasMatchingTag(AZ::Crc32 sampleTag, float weightMin, float weightMax) const;

        //! Check to see if the collection contains any of the given tags.
        //! @param sampleTags - The tags to look for.
        //! @return True if any of the tags is found, false if none are found.
        bool HasAnyMatchingTags(AZStd::span<const SurfaceTag> sampleTags) const;

        //! Check to see if the collection contains the given tag with the given weight range.
        //! The range check is inclusive on both sides of the range: [weightMin, weightMax]
        //! @param sampleTags - The tags to look for.
        //! @param weightMin - The minimum weight for this tag.
        //! @param weightMax - The maximum weight for this tag.
        //! @return True if any of the tags is found, false if none are found.
        bool HasAnyMatchingTags(AZStd::span<const SurfaceTag> sampleTags, float weightMin, float weightMax) const;

    private:
        //! Search for the given tag entry.
        //! @param tag - The tag to search for.
        //! @return The pointer to the tag that's found, or end() if it wasn't found.
        const AzFramework::SurfaceData::SurfaceTagWeight* FindTag(AZ::Crc32 tag) const;

        AZStd::fixed_vector<AzFramework::SurfaceData::SurfaceTagWeight, AzFramework::SurfaceData::Constants::MaxSurfaceWeights> m_weights;
    };


    struct SurfaceDataRegistryEntry
    {
        //! The entity ID of the surface provider / modifier
        AZ::EntityId m_entityId;

        //! The AABB bounds that this surface provider / modifier can affect, or null if it has infinite bounds.
        AZ::Aabb m_bounds = AZ::Aabb::CreateNull();

        //! The set of surface tags that this surface provider / modifier can create or add to a point.
        SurfaceTagVector m_tags;

        //! The maximum number of surface points that this will create per input position.
        //! For surface modifiers, this is always expected to be 0, and for surface providers it's expected to be > 0.
        size_t m_maxPointsCreatedPerInput = 0;
    };

    using SurfaceDataRegistryHandle = AZ::u32;
    const SurfaceDataRegistryHandle InvalidSurfaceDataRegistryHandle = 0;
}
