/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SurfaceData/Utility/SurfaceDataUtility.h>

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

        for (auto& weight : m_weights)
        {
            weights.emplace_back(weight);
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
                    return (weight == compareWeight);
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
            if (weight.m_surfaceType != Constants::s_unassignedTagCrc)
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

    bool SurfaceTagWeights::HasAnyMatchingTags(AZStd::span<const SurfaceTag> sampleTags) const
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
        return weightEntry != m_weights.end() && weightMin <= weightEntry->m_weight && weightMax >= weightEntry->m_weight;
    }

    bool SurfaceTagWeights::HasAnyMatchingTags(AZStd::span<const SurfaceTag> sampleTags, float weightMin, float weightMax) const
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

    const AzFramework::SurfaceData::SurfaceTagWeight* SurfaceTagWeights::FindTag(AZ::Crc32 tag) const
    {
        for (auto weightItr = m_weights.begin(); weightItr != m_weights.end(); ++weightItr)
        {
            if (weightItr->m_surfaceType == tag)
            {
                // Found the tag, return a pointer to the entry.
                return weightItr;
            }
            else if (weightItr->m_surfaceType > tag)
            {
                // Our list is stored in sorted order by surfaceType, so early-out if our values get too high.
                break;
            }
        }

        // The tag wasn't found, so return end().
        return m_weights.end();
    }
}
