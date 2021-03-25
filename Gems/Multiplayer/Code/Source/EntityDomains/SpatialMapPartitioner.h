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

#pragma once

#include <AzCore/Math/Aabb.h>
#include <Source/MultiplayerTypes.h>

namespace Multiplayer
{
    class ServerMapPartitioner
    {
    public:
        ServerMapPartitioner();

        void PartitionMap(uint32_t regionCount, uint32_t shardCount);

        uint32_t GetRegionCount() const;
        AZ::Aabb GetMapRegion(uint32_t index) const;
        AZ::Aabb GetMapRegionForHost(HostId hostId);

        const AZ::Aabb& GetWholeMap() const;
    private:
        AZ::Aabb m_wholeMap;
        AZStd::vector<AZ::Aabb> m_regions;
        uint32_t m_regionCount;
        uint32_t m_shardCount;
    };
}
