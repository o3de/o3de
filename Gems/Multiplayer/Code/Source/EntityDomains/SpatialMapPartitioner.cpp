/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <Source/EntityDomains/SpatialMapPartitioner.h>
#include <AzCore/std/containers/fixed_vector.h>

namespace Multiplayer
{
    static constexpr uint32_t MaxFactors = 32;
    using FactorSet = AZStd::fixed_vector<uint32_t, MaxFactors>;

    // Computes factors of N, but does not return the factors 1 and N
    void ComputeFactors(uint32_t integer, FactorSet& output)
    {
        output.clear();

        while (integer > 2)
        {
            for (uint32_t test = 2; test <= (integer / 2); ++test)
            {
                if ((integer % test) == 0)
                {
                    output.push_back(test);
                    integer /= test;
                    break;
                }
            }
        }

        if (integer > 1)
        {
            output.push_back(integer);
        }
    }

    ServerMapPartitioner::ServerMapPartitioner()
        : m_regionCount(0)
        , m_shardCount(0)
    {
        ;
    }

    void ServerMapPartitioner::PartitionMap(uint32_t totalRegions, uint32_t shardCount)
    {
        AZ_Assert(totalRegions > 0, "Total number of regions for map partitioner must be positive");

        // Factor totalRegions
        FactorSet factors;
        ComputeFactors(totalRegions, factors);

        // Compute map extents
        AZ::Vector3 mapMinBounds, mapMaxBounds;
        //AZ::Interface<IPhysics>::Get()->GetWorldBounds(mapMinBounds, mapMaxBounds);
        //m_wholeMap = AZ::Aabb::CreateFromMinMax(mapMinBounds, mapMaxBounds);

        // This part could use some work
        // Basically we want to now distribute the factors in some way related to the extents of the map
        // The ideal result I guess being that the output regions are as close to square as possible?

        // For now distribute the factors evenly, and then bias for the larger map extent..
        const AZ::Vector3 delta = mapMaxBounds - mapMinBounds;

        uint32_t divisions[2] = { 1, 1 };
        uint32_t divIndex = 0;

        for (uint32_t i = 0; i < factors.size(); ++i)
        {
            divisions[divIndex] *= factors[i];
            divIndex = 1 - divIndex;
        }

        // Sort.. since I don't know which came out greater
        if (divisions[1] > divisions[0])
        {
            uint32_t temp = divisions[1];
            divisions[1] = divisions[0];
            divisions[0] = temp;
        }

        const bool xGreater = (delta.GetX() >= delta.GetY());

        const uint32_t xAxisDiv = xGreater ? divisions[0] : divisions[1];
        const uint32_t yAxisDiv = xGreater ? divisions[1] : divisions[0];

        const float mapWidth = mapMaxBounds.GetX() - mapMinBounds.GetX();
        const float mapHeight = mapMaxBounds.GetY() - mapMinBounds.GetY();

        const float partitionWidth = mapWidth / xAxisDiv;
        const float partitionHeight = mapHeight / yAxisDiv;

        const uint32_t regionCount = xAxisDiv * yAxisDiv;

        AZ_Assert(regionCount == totalRegions, "Was not able to partition the map into the requested region count, invalid region count specified");

        m_regions.resize(regionCount);
        m_shardCount = shardCount;
        m_regionCount = totalRegions;

        AZ::Vector3 regionMinBounds;
        AZ::Vector3 regionMaxBounds;
        uint32_t regionIndex = 0;

        regionMaxBounds.SetY(mapMinBounds.GetY());

        for (int32_t y = yAxisDiv - 1; y >= 0; --y)
        {
            regionMinBounds.SetY(regionMaxBounds.GetY());

            if (y > 0)
            {
                regionMaxBounds.SetY(regionMaxBounds.GetY() + partitionHeight);
            }
            else
            {
                regionMaxBounds.SetY(mapMaxBounds.GetY());
            }

            regionMaxBounds.SetX(mapMinBounds.GetX());

            for (int32_t x = xAxisDiv - 1; x >= 0; --x)
            {
                regionMinBounds.SetX(regionMaxBounds.GetX());

                if (x > 0)
                {
                    regionMaxBounds.SetX(regionMaxBounds.GetX() + partitionWidth);
                }
                else
                {
                    regionMaxBounds.SetX(mapMaxBounds.GetX());
                }

                m_regions[regionIndex++] = AZ::Aabb::CreateFromMinMax(regionMinBounds, regionMaxBounds);
            }
        }
    }

    uint32_t ServerMapPartitioner::GetRegionCount() const
    {
        return static_cast<uint32_t>(m_regions.size());
    }

    AZ::Aabb ServerMapPartitioner::GetMapRegion(uint32_t index) const
    {
        if ((index >= 0) && (index < GetRegionCount()))
        {
            return m_regions[index];
        }

        return AZ::Aabb();
    }

    AZ::Aabb ServerMapPartitioner::GetMapRegionForHost(HostId hostId)
    {
        // TODO: This is not the best place for this logic, though I'm not sure where is yet -- potentially this should be a parameter returned by the game service?
        // If there are global shards, those will be the lowest index, so we need to offset correctly to find the correct region for an EntityManagerId()
        int offset = m_shardCount - m_regionCount;
        int32_t index = aznumeric_cast<int32_t>(hostId) - 1 - offset;
        AZ_Assert(index >= 0 && index < GetRegionCount(), "No region for Entity Manager");
        return GetMapRegion(index);
    }

    const AZ::Aabb& ServerMapPartitioner::GetWholeMap() const
    {
        return m_wholeMap;
    }
}
